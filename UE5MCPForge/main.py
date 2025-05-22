from mcp.server.fastmcp import FastMCP, Context
import json
import sys
import asyncio
import logging
import os
import time
import requests
from typing import Dict, List, Optional, Any
import threading

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("BlueprintAnalyzer")

# Create an MCP server
mcp = FastMCP("BlueprintAnalyzer")

# Storage for blueprint data
blueprint_storage = {}

# Configuration
SYNC_INTERVAL = 60  # seconds between sync operations
UE5_PLUGIN_URL = "http://localhost:8080"  # UE5 C++ plugin HTTP server URL
DEFAULT_DETAIL_LEVEL = 2  # Default detail level for blueprint data (0=Basic, 1=Medium, 2=Full, 3=Graph, 4=Events, 5=References)
MAX_NODES_PER_GRAPH = 50  # Maximum number of nodes to fetch per graph
MAX_GRAPHS = 10  # Maximum number of graphs to fetch

# Flag to control background sync
sync_running = False

# ========== DATA ACCESS FUNCTIONALITY ==========

async def file_polling_task():
    """Background task to periodically check for blueprint data from HTTP server"""
    global sync_running
    
    logger.info("HTTP polling task started")
    sync_running = True
    
    while sync_running:
        try:
            # Fetch from UE5 plugin HTTP server
            await fetch_blueprints_from_http()
        except Exception as e:
            logger.error(f"Error fetching blueprints from HTTP server: {str(e)}")
        
        # Wait before checking again
        await asyncio.sleep(SYNC_INTERVAL)

async def fetch_blueprints_from_http():
    """Fetch blueprint data from the UE5 plugin HTTP server"""
    global blueprint_storage
    
    logger.info(f"Fetching blueprints from UE5 plugin at {UE5_PLUGIN_URL}")
    
    # Use requests to fetch the data with detail level
    response = requests.get(f"{UE5_PLUGIN_URL}/blueprints/all?detailLevel={DEFAULT_DETAIL_LEVEL}", timeout=10)
    
    if response.status_code != 200:
        logger.error(f"Failed to fetch blueprints: HTTP {response.status_code}")
        return False
        
    try:
        data = response.json()
        
        # Check if the response is an array or an object with a "blueprints" field
        blueprints = None
        if isinstance(data, list):
            blueprints = data
        elif isinstance(data, dict) and "blueprints" in data and isinstance(data["blueprints"], list):
            blueprints = data["blueprints"]
        else:
            logger.warning("Invalid blueprint data format from UE5 plugin")
            return False
            
        if blueprints:
            # Update storage
            count = 0
            for blueprint in blueprints:
                if "path" in blueprint:
                    blueprint_storage[blueprint["path"]] = blueprint
                    count += 1
                    
            logger.info(f"Successfully loaded {count} blueprints from UE5 plugin HTTP server (detail level: {DEFAULT_DETAIL_LEVEL})")
            return True
        else:
            logger.warning("No blueprints found in UE5 plugin response")
            return False
            
    except json.JSONDecodeError:
        logger.error("Failed to parse JSON from UE5 plugin HTTP server")
        return False
    except Exception as e:
        logger.error(f"Error processing blueprints from UE5 plugin: {str(e)}")
        return False

def start_http_polling():
    """Start background HTTP polling for blueprint data"""
    global sync_running
    
    logger.info("Starting HTTP polling for blueprint data...")
    
    # Create event loop in the thread
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    
    # Start the polling task
    loop.create_task(file_polling_task())
    
    # Run the event loop
    loop.run_forever()

# ========== TOOLS ==========

@mcp.tool()
def store_blueprint_data(blueprint_json: str) -> str:
    """
    Store blueprint data received from the UE5 plugin.
    Expects a JSON string with blueprint information including name, path, functions, and variables.
    """
    try:
        blueprint = json.loads(blueprint_json)
        if "name" in blueprint and "path" in blueprint:
            blueprint_storage[blueprint["path"]] = blueprint
            return f"Successfully stored blueprint: {blueprint['name']}"
        else:
            return "Error: Invalid blueprint data format (missing name or path)"
    except json.JSONDecodeError:
        return "Error: Invalid JSON format"

@mcp.tool()
def store_blueprints_data(blueprints_json: str) -> str:
    """
    Store multiple blueprint data entries received from the UE5 plugin.
    Expects a JSON string with a 'blueprints' array containing blueprint objects.
    """
    try:
        data = json.loads(blueprints_json)
        if "blueprints" in data and isinstance(data["blueprints"], list):
            count = 0
            for blueprint in data["blueprints"]:
                if "name" in blueprint and "path" in blueprint:
                    blueprint_storage[blueprint["path"]] = blueprint
                    count += 1
            return f"Successfully stored {count} blueprints"
        else:
            return "Error: Invalid blueprints data format"
    except json.JSONDecodeError:
        return "Error: Invalid JSON format"

@mcp.tool()
def search_blueprints(query: str, search_type: str = "name") -> str:
    """
    Search for blueprints based on various criteria.

    query: The search term
    search_type: One of "name", "parentClass", "function", "variable", or "path"
    """
    results = []

    for path, blueprint in blueprint_storage.items():
        if search_type == "name" and query.lower() in blueprint.get("name", "").lower():
            results.append(blueprint)
        elif search_type == "parentClass" and query.lower() in blueprint.get("parentClass", "").lower():
            results.append(blueprint)
        elif search_type == "path" and query.lower() in path.lower():
            results.append(blueprint)
        elif search_type == "function":
            for function in blueprint.get("functions", []):
                if query.lower() in function.get("name", "").lower():
                    results.append(blueprint)
                    break
        elif search_type == "variable":
            for variable in blueprint.get("variables", []):
                if query.lower() in variable.get("name", "").lower():
                    results.append(blueprint)
                    break

    if not results:
        return f"No blueprints found matching '{query}' in {search_type}"

    # Return a simplified list for readability
    summary = {
        "blueprints": [
            {
                "name": bp.get("name", "Unknown"),
                "path": bp.get("path", "Unknown"),
                "parentClass": bp.get("parentClass", "None")
            } for bp in results
        ]
    }

    return json.dumps(summary, indent=2)

@mcp.tool()
def analyze_blueprint_complexity(blueprint_path: str) -> str:
    """
    Analyze the complexity of a blueprint based on metrics like function count,
    variable count, and other structural features.
    """
    if blueprint_path not in blueprint_storage:
        return f"Blueprint with path '{blueprint_path}' not found"

    blueprint = blueprint_storage[blueprint_path]
    functions = blueprint.get("functions", [])
    variables = blueprint.get("variables", [])

    # Calculate complexity metrics
    function_count = len(functions)
    variable_count = len(variables)
    event_count = sum(1 for f in functions if f.get("isEvent", False))
    exposed_variable_count = sum(1 for v in variables if v.get("isExposed", False))
    replicated_variable_count = sum(1 for v in variables if v.get("isReplicated", False))
    pure_function_count = sum(1 for f in functions if f.get("isPure", False))

    # Determine complexity level
    total_score = function_count + variable_count
    complexity_level = "Low" if total_score < 10 else "Medium" if total_score < 20 else "High"

    analysis = {
        "name": blueprint.get("name", "Unknown"),
        "path": blueprint_path,
        "metrics": {
            "function_count": function_count,
            "event_count": event_count,
            "variable_count": variable_count,
            "exposed_variable_count": exposed_variable_count,
            "replicated_variable_count": replicated_variable_count,
            "pure_function_count": pure_function_count,
            "total_score": total_score,
            "complexity_level": complexity_level
        }
    }

    return json.dumps(analysis, indent=2)

@mcp.tool()
def list_blueprint_functions(blueprint_path: str) -> str:
    """List all functions in a specific blueprint"""
    if blueprint_path not in blueprint_storage:
        return f"Blueprint with path '{blueprint_path}' not found"

    blueprint = blueprint_storage[blueprint_path]
    functions = blueprint.get("functions", [])

    if not functions:
        return f"No functions found in blueprint: {blueprint.get('name', 'Unknown')}"

    result = []
    for function in functions:
        func_info = {
            "name": function.get("name", "Unknown"),
            "isEvent": function.get("isEvent", False),
            "returnType": function.get("returnType", ""),
            "paramCount": len(function.get("params", [])),
            "isPure": function.get("isPure", False)
        }
        result.append(func_info)

    return json.dumps(result, indent=2)

@mcp.tool()
def list_blueprints() -> str:
    """List all stored blueprints"""
    if not blueprint_storage:
        return "No blueprints are currently stored"

    blueprints = []
    for path, bp in blueprint_storage.items():
        blueprints.append({
            "name": bp.get("name", "Unknown"),
            "path": path,
            "parentClass": bp.get("parentClass", "Unknown")
        })

    return json.dumps({"blueprints": blueprints}, indent=2)

async def sync_blueprints():
    """Sync blueprints from HTTP server"""
    try:
        return await fetch_blueprints_from_http()
    except Exception as e:
        logger.error(f"Error fetching blueprints from HTTP server during sync: {str(e)}")
        return False

def sync_blueprints_sync():
    """Synchronous version of the blueprint sync for direct tool calls"""
    global blueprint_storage
    logger.info(f"Performing synchronous fetch from UE5 plugin at {UE5_PLUGIN_URL}")
    
    # Use requests to fetch the data with detail level
    try:
        response = requests.get(f"{UE5_PLUGIN_URL}/blueprints/all?detailLevel={DEFAULT_DETAIL_LEVEL}", timeout=10)
        
        if response.status_code != 200:
            logger.error(f"Failed to fetch blueprints: HTTP {response.status_code}")
            return False
            
        data = response.json()
        
        # Check if the response is an array or an object with a "blueprints" field
        blueprints = None
        if isinstance(data, list):
            blueprints = data
        elif isinstance(data, dict) and "blueprints" in data and isinstance(data["blueprints"], list):
            blueprints = data["blueprints"]
        else:
            logger.warning("Invalid blueprint data format from UE5 plugin")
            return False
            
        if blueprints:
            # Update storage
            count = 0
            for blueprint in blueprints:
                if "path" in blueprint:
                    blueprint_storage[blueprint["path"]] = blueprint
                    count += 1
                    
            logger.info(f"Successfully loaded {count} blueprints from UE5 plugin HTTP server (detail level: {DEFAULT_DETAIL_LEVEL})")
            return True
        else:
            logger.warning("No blueprints found in UE5 plugin response")
            return False
            
    except requests.RequestException as e:
        logger.error(f"Error connecting to UE5 plugin HTTP server: {str(e)}")
        return False
    except json.JSONDecodeError:
        logger.error("Failed to parse JSON from UE5 plugin HTTP server")
        return False
    except Exception as e:
        logger.error(f"Error processing blueprints from UE5 plugin: {str(e)}")
        return False

@mcp.tool()
def force_blueprint_sync() -> str:
    """Force an immediate synchronization with the UE5 plugin"""
    # Don't check for sync_running - this works whether background sync is running or not
    
    # Use the synchronous version that works independently
    try:
        success = sync_blueprints_sync()
        
        if success:
            return f"Blueprint synchronization successful, {len(blueprint_storage)} blueprints loaded"
        else:
            return "Blueprint synchronization failed - see logs for details. Check if UE5 plugin is running on port 8080."
    except Exception as e:
        logger.error(f"Error during forced sync: {str(e)}")
        return f"Error during synchronization: {str(e)}"

@mcp.tool()
def set_sync_interval(seconds: int) -> str:
    """Set the interval between automatic blueprint synchronizations"""
    global SYNC_INTERVAL
    
    if seconds < 5:
        return "Error: Sync interval must be at least 5 seconds"
    
    SYNC_INTERVAL = seconds
    return f"Sync interval set to {seconds} seconds"

@mcp.tool()
def set_detail_level(level: int) -> str:
    """
    Set the detail level for blueprint data extraction
    
    0 = Basic (name, path, parent class only)
    1 = Medium (basic + simplified functions and variables)
    2 = Full (complete information without graph data)
    3 = Graph (includes visual graph data)
    4 = Events (focuses on event nodes and their graphs)
    5 = References (includes blueprint references)
    """
    global DEFAULT_DETAIL_LEVEL
    
    if level < 0 or level > 5:
        return "Error: Detail level must be between 0 and 5"
    
    DEFAULT_DETAIL_LEVEL = level
    return f"Detail level set to {level} ({['Basic', 'Medium', 'Full', 'Graph', 'Events', 'References'][level]})"

@mcp.tool()
def get_blueprint_with_detail(blueprint_path: str, detail_level: int = None) -> str:
    """
    Get a blueprint with a specific detail level
    
    blueprint_path: Path to the blueprint
    detail_level: Detail level (0-5, see set_detail_level documentation)
    """
    if detail_level is None:
        detail_level = DEFAULT_DETAIL_LEVEL
        
    if detail_level < 0 or detail_level > 5:
        return "Error: Detail level must be between 0 and 5"
    
    try:
        # Fetch the blueprint with the specified detail level
        response = requests.get(
            f"{UE5_PLUGIN_URL}/blueprints/path?path={blueprint_path}&detailLevel={detail_level}",
            timeout=10
        )
        
        if response.status_code != 200:
            return f"Error: Failed to fetch blueprint (HTTP {response.status_code})"
            
        blueprint = response.json()
        
        # Update storage with this detailed blueprint
        if "path" in blueprint:
            blueprint_storage[blueprint["path"]] = blueprint
            
        return json.dumps(blueprint, indent=2)
    except Exception as e:
        return f"Error fetching blueprint: {str(e)}"

@mcp.tool()
def list_blueprint_events(blueprint_path: str) -> str:
    """
    List all event nodes in a specific blueprint
    
    blueprint_path: Path to the blueprint
    """
    try:
        # Fetch events for the blueprint
        response = requests.get(
            f"{UE5_PLUGIN_URL}/blueprints/events?path={blueprint_path}",
            timeout=10
        )
        
        if response.status_code != 200:
            return f"Error: Failed to fetch blueprint events (HTTP {response.status_code})"
            
        events = response.json()
        return json.dumps(events, indent=2)
    except Exception as e:
        return f"Error fetching blueprint events: {str(e)}"

@mcp.tool()
def get_event_graph(blueprint_path: str, event_name: str, max_nodes: int = None) -> str:
    """
    Get a specific event graph from a blueprint
    
    blueprint_path: Path to the blueprint
    event_name: Name of the event (e.g., BeginPlay, Tick)
    max_nodes: Maximum number of nodes to include (default: 50)
    """
    if max_nodes is None:
        max_nodes = MAX_NODES_PER_GRAPH
        
    try:
        # Fetch the event graph
        response = requests.get(
            f"{UE5_PLUGIN_URL}/blueprints/event-graph?path={blueprint_path}&eventName={event_name}&maxNodes={max_nodes}",
            timeout=10
        )
        
        if response.status_code != 200:
            return f"Error: Failed to fetch event graph (HTTP {response.status_code})"
            
        graph = response.json()
        return json.dumps(graph, indent=2)
    except Exception as e:
        return f"Error fetching event graph: {str(e)}"

@mcp.tool()
def get_function_graph(blueprint_path: str, function_name: str) -> str:
    """
    Get a specific function graph from a blueprint
    
    blueprint_path: Path to the blueprint
    function_name: Name of the function
    """
    try:
        # Fetch the function graph
        response = requests.get(
            f"{UE5_PLUGIN_URL}/blueprints/function?path={blueprint_path}&function={function_name}",
            timeout=10
        )
        
        if response.status_code != 200:
            return f"Error: Failed to fetch function graph (HTTP {response.status_code})"
            
        graph = response.json()
        return json.dumps(graph, indent=2)
    except Exception as e:
        return f"Error fetching function graph: {str(e)}"

@mcp.tool()
def get_nodes_by_type(blueprint_path: str, node_type: str) -> str:
    """
    Get all nodes of a specific type from a blueprint
    
    blueprint_path: Path to the blueprint
    node_type: Type of node to find (e.g., K2Node_CallFunction, K2Node_IfThenElse)
    """
    try:
        # Fetch nodes of the specified type
        response = requests.get(
            f"{UE5_PLUGIN_URL}/blueprints/graph/nodes?path={blueprint_path}&nodeType={node_type}",
            timeout=10
        )
        
        if response.status_code != 200:
            return f"Error: Failed to fetch nodes (HTTP {response.status_code})"
            
        nodes = response.json()
        return json.dumps(nodes, indent=2)
    except Exception as e:
        return f"Error fetching nodes: {str(e)}"

@mcp.tool()
def get_blueprint_references(blueprint_path: str, include_indirect: bool = False) -> str:
    """
    Get references to and from a blueprint
    
    blueprint_path: Path to the blueprint
    include_indirect: Whether to include indirect references
    """
    try:
        # Fetch blueprint references
        response = requests.get(
            f"{UE5_PLUGIN_URL}/blueprints/references?path={blueprint_path}&includeIndirect={str(include_indirect).lower()}",
            timeout=10
        )
        
        if response.status_code != 200:
            return f"Error: Failed to fetch blueprint references (HTTP {response.status_code})"
            
        references = response.json()
        return json.dumps(references, indent=2)
    except Exception as e:
        return f"Error fetching blueprint references: {str(e)}"

# ========== RESOURCES ==========

@mcp.resource("blueprints://all")
def get_all_blueprints() -> str:
    """Get a list of all blueprints in storage"""
    if not blueprint_storage:
        return json.dumps({"message": "No blueprints are currently stored"})

    blueprint_summary = []
    for path, bp in blueprint_storage.items():
        summary = {
            "name": bp.get("name", "Unknown"),
            "path": path,
            "parentClass": bp.get("parentClass", "None"),
            "functionCount": len(bp.get("functions", [])),
            "variableCount": len(bp.get("variables", []))
        }
        blueprint_summary.append(summary)

    return json.dumps(blueprint_summary, indent=2)

@mcp.resource("blueprint://{path}")
def get_blueprint(path: str) -> str:
    """Get detailed information about a specific blueprint by its path"""
    if path in blueprint_storage:
        return json.dumps(blueprint_storage[path], indent=2)
    else:
        return json.dumps({"error": f"Blueprint with path '{path}' not found"})

@mcp.resource("blueprint://{path}/functions")
def get_blueprint_functions(path: str) -> str:
    """Get all functions from a specific blueprint"""
    if path not in blueprint_storage:
        return json.dumps({"error": f"Blueprint with path '{path}' not found"})

    functions = blueprint_storage[path].get("functions", [])
    return json.dumps(functions, indent=2)

@mcp.resource("blueprint://{path}/variables")
def get_blueprint_variables(path: str) -> str:
    """Get all variables from a specific blueprint"""
    if path not in blueprint_storage:
        return json.dumps({"error": f"Blueprint with path '{path}' not found"})

    variables = blueprint_storage[path].get("variables", [])
    return json.dumps(variables, indent=2)

# ========== RESOURCES ==========

@mcp.resource("blueprint://{path}/events")
def get_blueprint_events(path: str) -> str:
    """Get all event nodes from a specific blueprint"""
    if path not in blueprint_storage:
        return json.dumps({"error": f"Blueprint with path '{path}' not found"})

    # Try to get events from the blueprint
    try:
        # First check if we already have the blueprint with sufficient detail level
        blueprint = blueprint_storage[path]
        
        # If we don't have functions or the detail level is too low, fetch with higher detail
        if "functions" not in blueprint or not blueprint.get("functions"):
            # Fetch the blueprint with events detail level
            response = requests.get(
                f"{UE5_PLUGIN_URL}/blueprints/events?path={path}",
                timeout=10
            )
            
            if response.status_code == 200:
                return response.text
            else:
                # Fall back to extracting events from existing data
                pass
        
        # Extract events from functions
        events = []
        for function in blueprint.get("functions", []):
            if function.get("isEvent", False):
                events.append(function)
                
        return json.dumps(events, indent=2)
    except Exception as e:
        return json.dumps({"error": f"Error getting events: {str(e)}"})

@mcp.resource("blueprint://{path}/references")
def get_blueprint_references_resource(path: str) -> str:
    """Get references to and from a specific blueprint"""
    if path not in blueprint_storage:
        return json.dumps({"error": f"Blueprint with path '{path}' not found"})

    try:
        # Fetch blueprint references
        response = requests.get(
            f"{UE5_PLUGIN_URL}/blueprints/references?path={path}",
            timeout=10
        )
        
        if response.status_code == 200:
            return response.text
        else:
            return json.dumps({"error": f"Failed to fetch blueprint references (HTTP {response.status_code})"})
    except Exception as e:
        return json.dumps({"error": f"Error fetching blueprint references: {str(e)}"})

@mcp.resource("blueprint://{path}/graphs")
def get_blueprint_graphs(path: str) -> str:
    """Get all graphs from a specific blueprint"""
    if path not in blueprint_storage:
        return json.dumps({"error": f"Blueprint with path '{path}' not found"})

    # Check if we have graph data
    blueprint = blueprint_storage[path]
    if "graphs" in blueprint and blueprint["graphs"]:
        return json.dumps(blueprint["graphs"], indent=2)
    else:
        # Try to fetch with graph detail level
        try:
            response = requests.get(
                f"{UE5_PLUGIN_URL}/blueprints/path?path={path}&detailLevel=3&maxGraphs={MAX_GRAPHS}&maxNodes={MAX_NODES_PER_GRAPH}",
                timeout=15
            )
            
            if response.status_code == 200:
                data = response.json()
                if "graphs" in data:
                    # Update storage with this detailed blueprint
                    blueprint_storage[path] = data
                    return json.dumps(data["graphs"], indent=2)
                else:
                    return json.dumps({"error": "No graph data available in response"})
            else:
                return json.dumps({"error": f"Failed to fetch blueprint graphs (HTTP {response.status_code})"})
        except Exception as e:
            return json.dumps({"error": f"Error fetching blueprint graphs: {str(e)}"})

# ========== PROMPTS ==========

@mcp.prompt()
def analyze_blueprint(blueprint_path: str) -> str:
    """Create a detailed analysis of a blueprint"""
    return f"""
    Please analyze the blueprint at path '{blueprint_path}' using the available tools and resources.

    First, check if the blueprint exists by using the get_blueprint resource.

    Then, analyze:
    1. The overall purpose based on its name and parent class
    2. Its structure and complexity (using analyze_blueprint_complexity)
    3. Key functions and their purposes
    4. Important variables and their roles
    5. Event handling patterns (using list_blueprint_events)
    6. References to and from this blueprint (using get_blueprint_references)

    Finally, provide:
    - A summary of what this blueprint does in the game
    - Potential optimization suggestions
    - Any patterns or anti-patterns observed
    - Recommendations for improving the blueprint's structure or functionality
    """

@mcp.prompt()
def blueprint_search_guide(search_term: str) -> str:
    """Guide for searching blueprints by various criteria"""
    return f"""
    Let's search for blueprints related to '{search_term}' using different search approaches:

    1. First, search by name: `search_blueprints('{search_term}', 'name')`
    2. Then by parent class: `search_blueprints('{search_term}', 'parentClass')`
    3. Next by function name: `search_blueprints('{search_term}', 'function')`
    4. Finally by variable name: `search_blueprints('{search_term}', 'variable')`

    For each set of results, analyze what the blueprints might be used for and how they relate to '{search_term}'.
    """

@mcp.prompt()
def blueprint_architecture_review() -> str:
    """Create a comprehensive review of the project's blueprint architecture"""
    return """
    Let's analyze the overall blueprint architecture of this project:

    1. First, list all blueprints using the list_blueprints tool
    2. Identify common parent classes and categorize blueprints
    3. Find the most complex blueprints using analyze_blueprint_complexity
    4. Look for patterns in naming conventions and structure
    5. Analyze blueprint references to understand dependencies
    6. Examine event usage patterns across blueprints

    Then provide:
    - A summary of the overall architecture
    - Hierarchy diagrams of blueprint inheritance
    - Dependency graphs showing blueprint relationships
    - Recommendations for better organization
    - Potential refactoring opportunities
    - Performance optimization suggestions
    """

@mcp.prompt()
def find_blueprint_references(function_name: str) -> str:
    """Find all blueprints that reference a specific function name"""
    return f"""
    Let's find all blueprints that reference the function '{function_name}':

    1. Use search_blueprints('{function_name}', 'function') to find direct implementations
    2. For each result, examine the full blueprint structure to understand:
       - How the function is used within each blueprint
       - Whether it's an event or regular function
       - What parameters it takes and returns
    3. For key blueprints, use get_function_graph to visualize the implementation
    4. Use get_blueprint_references to find other blueprints that might call this function

    Then provide:
    - A list of all blueprints containing this function
    - An analysis of how the function is used across different blueprints
    - Visualization of common implementation patterns
    - Suggestions for standardizing or improving its implementation
    - Potential performance optimizations
    """

@mcp.prompt()
def identify_blueprint_patterns(pattern_type: str = "all") -> str:
    """Identify common patterns or anti-patterns across blueprints"""
    pattern_types = {
        "all": "all patterns",
        "performance": "performance-related patterns",
        "structure": "structural patterns",
        "events": "event handling patterns",
        "replication": "network replication patterns"
    }
    
    pattern_name = pattern_types.get(pattern_type, "all patterns")
    
    return f"""
    Let's identify {pattern_name} across the project's blueprints:

    1. First, list all blueprints using list_blueprints()
    2. For a representative sample of blueprints:
       - Get detailed blueprint data with get_blueprint_with_detail
       - Analyze their structure, functions, and variables
       - Look for common patterns and anti-patterns

    For performance patterns, focus on:
    - Expensive operations in Tick events
    - Redundant calculations
    - Inefficient loops or array operations
    - Heavy operations on BeginPlay

    For structural patterns, focus on:
    - Inheritance hierarchies
    - Component composition
    - Function organization
    - Variable organization

    For event patterns, focus on:
    - Event binding and delegation
    - Event-driven architecture
    - Custom event usage
    - Event replication

    Then provide:
    - A catalog of common patterns found
    - Examples of each pattern with blueprint paths
    - Recommendations for best practices
    - Anti-patterns to avoid with alternatives
    - Suggestions for improving the overall blueprint architecture
    """

@mcp.prompt()
def optimize_blueprint(blueprint_path: str) -> str:
    """Provide optimization suggestions for a specific blueprint"""
    return f"""
    Let's analyze blueprint '{blueprint_path}' for optimization opportunities:

    1. First, get the blueprint with graph data using get_blueprint_with_detail('{blueprint_path}', 3)
    2. Analyze its event graphs, particularly Tick and BeginPlay
    3. Look for performance issues like:
       - Expensive operations in Tick
       - Redundant calculations
       - Inefficient loops or array operations
       - Excessive event dispatching
       - Complex constructions in BeginPlay
    4. Examine variable usage and potential for caching
    5. Check for proper use of timers vs polling

    Then provide:
    - A summary of current performance bottlenecks
    - Specific optimization recommendations with examples
    - Suggestions for refactoring problematic sections
    - Alternative approaches that would be more efficient
    - Estimated performance impact of each suggestion
    """

@mcp.prompt()
def compare_blueprints(blueprint_path1: str, blueprint_path2: str) -> str:
    """Compare two blueprints to identify similarities and differences"""
    return f"""
    Let's compare blueprints '{blueprint_path1}' and '{blueprint_path2}':

    1. First, get detailed information about both blueprints:
       - get_blueprint_with_detail('{blueprint_path1}', 2)
       - get_blueprint_with_detail('{blueprint_path2}', 2)
    
    2. Compare their structure:
       - Parent classes and inheritance
       - Functions and their implementations
       - Variables and their types
       - Event handling approaches
    
    3. Analyze similarities and differences in:
       - Overall purpose and functionality
       - Implementation approaches
       - Coding patterns used
       - Potential reuse opportunities

    Then provide:
    - A side-by-side comparison of key features
    - Analysis of common functionality that could be refactored
    - Identification of unique aspects of each blueprint
    - Recommendations for standardizing similar functionality
    - Suggestions for potential base classes or interfaces
    """

@mcp.prompt()
def analyze_event_flow(blueprint_path: str, event_name: str) -> str:
    """Analyze the execution flow of a specific event in a blueprint"""
    return f"""
    Let's analyze the execution flow of the '{event_name}' event in blueprint '{blueprint_path}':

    1. First, get the event graph using get_event_graph('{blueprint_path}', '{event_name}')
    2. Identify the entry point and execution paths
    3. Analyze key decision points and branches
    4. Examine function calls and their purposes
    5. Look for potential race conditions or performance bottlenecks

    Then provide:
    - A description of what this event does
    - A step-by-step breakdown of the execution flow
    - Visualization of the execution paths (as text/ASCII diagrams)
    - Potential optimization opportunities
    - Any error handling or edge cases that might be missing
    """

@mcp.prompt()
def analyze_blueprint_references(blueprint_path: str) -> str:
    """Analyze references to and from a blueprint to understand dependencies"""
    return f"""
    Let's analyze the references for blueprint '{blueprint_path}' to understand its dependencies:

    1. First, get the blueprint references using get_blueprint_references('{blueprint_path}', True)
    2. Identify incoming references (other blueprints that use this one)
    3. Identify outgoing references (blueprints that this one uses)
    4. Analyze the types of references (inheritance, function calls, variables, etc.)
    5. Look for potential circular dependencies

    Then provide:
    - A summary of how this blueprint is used in the project
    - A summary of what other blueprints this one depends on
    - Visualization of the dependency relationships
    - Recommendations for reducing tight coupling
    - Potential refactoring opportunities to improve the architecture
    """

@mcp.prompt()
def analyze_blueprint_graph_patterns(blueprint_path: str) -> str:
    """Analyze patterns in blueprint graphs to identify common structures and anti-patterns"""
    return f"""
    Let's analyze the graph patterns in blueprint '{blueprint_path}':

    1. First, get the blueprint with graph data using get_blueprint_with_detail('{blueprint_path}', 3)
    2. Identify common node patterns (sequences, branches, loops)
    3. Look for anti-patterns like:
       - Spaghetti connections
       - Deeply nested branches
       - Redundant calculations
       - Excessive event usage
    4. Analyze function usage and potential for refactoring

    Then provide:
    - A summary of the graph structure
    - Identification of common patterns used
    - List of potential anti-patterns found
    - Recommendations for improving the blueprint's structure
    - Suggestions for performance optimization
    """

if __name__ == "__main__":
    print("BlueprintAnalyzer MCP Server starting...")

    # Check requests is available
    try:
        import requests
    except ImportError:
        logger.error("Requests library not found")
        logger.error("Please install it with 'pip install requests'")
        sys.exit(1)
    
    # Check for UE5 plugin HTTP server
    try:
        response = requests.get(f"{UE5_PLUGIN_URL}/blueprints/all?detailLevel=0", timeout=2)
        if response.status_code == 200:
            logger.info(f"Successfully connected to UE5 plugin HTTP server at {UE5_PLUGIN_URL}")
            # Process blueprints from HTTP server
            data = response.json()
            
            # Check if the response is an array or an object with a "blueprints" field
            blueprints = None
            if isinstance(data, list):
                blueprints = data
            elif isinstance(data, dict) and "blueprints" in data and isinstance(data["blueprints"], list):
                blueprints = data["blueprints"]
                
            if blueprints:
                for blueprint in blueprints:
                    if "path" in blueprint:
                        blueprint_storage[blueprint["path"]] = blueprint
                logger.info(f"Loaded {len(blueprints)} blueprints from UE5 plugin HTTP server")
        else:
            logger.warning(f"UE5 plugin HTTP server returned status code {response.status_code}")
            logger.warning("Unable to connect to UE5 plugin HTTP server - use force_blueprint_sync tool to retry")
    except Exception as e:
        logger.warning(f"Failed to connect to UE5 plugin HTTP server: {str(e)}")
        logger.info("Use force_blueprint_sync tool to retry the connection")
    
    # Start HTTP polling in a background thread
    try:
        # First do an initial sync to load blueprints
        success = sync_blueprints_sync()
        if success:
            logger.info("Initial HTTP sync successful")
        
        # Then start the background polling thread
        http_polling_thread = threading.Thread(target=start_http_polling, daemon=True)
        http_polling_thread.start()
        logger.info("HTTP polling thread started")
    except Exception as e:
        logger.error(f"Failed to start HTTP polling: {str(e)}")

    # Check if running in dev mode
    if len(sys.argv) > 1 and "dev" in sys.argv:
        print("Running in dev mode on port 3000")
        # When running in dev mode, use command:
        # uv run mcp dev main.py 
    else:
        # When running directly, explicitly set port to 3000
        print("Starting MCP server on port 3000...")
        print("Available tools:")
        print("- force_blueprint_sync: Manually fetch blueprints from UE5 plugin HTTP server")
        print("- list_blueprints: Show all currently loaded blueprints")
        print("- search_blueprints: Search for blueprints by various criteria")
        print("- set_detail_level: Set the detail level for blueprint data (0-5)")
        print("- get_blueprint_with_detail: Get a blueprint with specific detail level")
        print("- list_blueprint_events: List all event nodes in a blueprint")
        print("- get_event_graph: Get a specific event graph from a blueprint")
        print("- get_function_graph: Get a specific function graph from a blueprint")
        print("- get_nodes_by_type: Get all nodes of a specific type from a blueprint")
        print("- get_blueprint_references: Get references to and from a blueprint")
        print("Available resources:")
        print("- blueprints://all: Access all loaded blueprints")
        print("- blueprint://{path}: Get detailed information about a specific blueprint")
        print("- blueprint://{path}/functions: Get all functions from a specific blueprint")
        print("- blueprint://{path}/variables: Get all variables from a specific blueprint")
        print("- blueprint://{path}/events: Get all event nodes from a specific blueprint")
        print("- blueprint://{path}/references: Get references to and from a specific blueprint")
        print("- blueprint://{path}/graphs: Get all graphs from a specific blueprint")
        mcp.run(transport="streamable-http", port=3000)