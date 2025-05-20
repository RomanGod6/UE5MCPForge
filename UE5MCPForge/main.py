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
    
    # Use requests to fetch the data
    response = requests.get(f"{UE5_PLUGIN_URL}/blueprints/all", timeout=10)
    
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
                    
            logger.info(f"Successfully loaded {count} blueprints from UE5 plugin HTTP server")
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
    
    # Use requests to fetch the data
    try:
        response = requests.get(f"{UE5_PLUGIN_URL}/blueprints/all", timeout=10)
        
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
                    
            logger.info(f"Successfully loaded {count} blueprints from UE5 plugin HTTP server")
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

    Finally, provide:
    - A summary of what this blueprint does in the game
    - Potential optimization suggestions
    - Any patterns or anti-patterns observed
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

    Then provide:
    - A summary of the overall architecture
    - Hierarchy diagrams of blueprint inheritance
    - Recommendations for better organization
    - Potential refactoring opportunities
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

    Then provide:
    - A list of all blueprints containing this function
    - An analysis of how the function is used across different blueprints
    - Suggestions for standardizing or improving its implementation
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
        response = requests.get(f"{UE5_PLUGIN_URL}/blueprints/all", timeout=2)
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
        print("Available resources:")
        print("- blueprints://all: Access all loaded blueprints")
        mcp.run(transport="streamable-http", port=3000)