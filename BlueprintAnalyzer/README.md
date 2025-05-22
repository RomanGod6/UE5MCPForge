# Blueprint Analyzer MCP Integration

This plugin provides integration between Unreal Engine blueprints and MCP (Model Context Protocol) servers, allowing AI tools to access and analyze blueprint data.

## Automatic Blueprint Data Synchronization

The Blueprint Analyzer now supports automatic blueprint data synchronization between the UE5 plugin and your Python MCP server. This means the MCP server can automatically fetch blueprint data without any manual intervention.

### How It Works

1. The UE5 plugin exposes blueprint data via HTTP endpoints on port 8080
2. The Python MCP server automatically connects to these endpoints and fetches data periodically
3. Blueprint data is stored in the MCP server and made available through its tools and resources

## HTTP Endpoints

The UE5 plugin provides the following HTTP endpoints:

- `GET /blueprints/all` - List all blueprints in the project
- `GET /blueprints/search?query=X&type=Y` - Search blueprints by name, parent class, function, or variable
- `GET /blueprints/path?path=X` - Get a specific blueprint by path
- `GET /blueprints/events?path=X&eventName=Y` - Get all event nodes from a blueprint
- `GET /blueprints/event-graph?path=X&eventName=Y&maxNodes=Z` - Get a specific event graph by name
- `GET /blueprints/function?path=X&function=Y` - Get a specific function graph
- `GET /blueprints/graph/nodes?path=X&nodeType=Y` - Get nodes of a specific type
- `GET /docs` - Get API documentation including detail level descriptions
- `GET /docs?type=detailLevels` - Get specific documentation about detail levels

## Detail Levels

The plugin supports different levels of detail when requesting blueprint data:

1. **Basic (Level 0)** - Only basic information: name, path, parent class
   - Best for listing many blueprints where only basic identification is needed
   - Example: `GET /blueprints/all?detailLevel=0`

2. **Medium (Level 1)** - Basic info plus simplified functions and variables
   - Good for getting an overview of a blueprint's capabilities without excess detail
   - Example: `GET /blueprints/path?path=/Game/MyBlueprint&detailLevel=1`

3. **Full (Level 2)** - Complete information about functions and variables with all metadata
   - For thorough analysis of blueprint functionality without visual graph data
   - Example: `GET /blueprints/path?path=/Game/MyBlueprint&detailLevel=2`

4. **Graph (Level 3)** - Everything including visual graph data with nodes and connections
   - For complete blueprint analysis including visual representation of the execution flow
   - Example: `GET /blueprints/path?path=/Game/MyBlueprint&detailLevel=3&maxGraphs=5&maxNodes=20`

5. **Events (Level 4)** - Focuses on event nodes and their associated graphs
   - For analyzing event-driven behavior and response patterns in blueprints
   - Example: `GET /blueprints/path?path=/Game/MyBlueprint&detailLevel=4`

## MCP Tools for Blueprint Data

The MCP server provides the following tools:

- `list_blueprints()` - List all stored blueprints
- `search_blueprints(query, search_type)` - Search blueprints by various criteria
- `analyze_blueprint_complexity(blueprint_path)` - Analyze blueprint complexity
- `list_blueprint_functions(blueprint_path)` - List all functions in a blueprint
- `force_blueprint_sync()` - Force an immediate synchronization with the UE5 plugin
- `set_sync_interval(seconds)` - Set the synchronization interval (default: 60 seconds)

## MCP Resources for Blueprint Data

The MCP server provides the following resources:

- `blueprints://all` - List all blueprints
- `blueprint://{path}` - Get detailed information about a specific blueprint
- `blueprint://{path}/functions` - Get all functions from a specific blueprint
- `blueprint://{path}/variables` - Get all variables from a specific blueprint

## Configuration

The UE5 plugin and MCP server use the following default configuration:

- UE5 HTTP Server: `http://localhost:8080`
- MCP Server: `http://localhost:3000`

## Usage

1. Start your UE5 project with the Blueprint Analyzer plugin enabled
2. Start your Python MCP server: `python main.py`
3. The MCP server will automatically fetch blueprint data every 60 seconds
4. You can force a sync using the `force_blueprint_sync()` tool
5. You can adjust the sync interval using the `set_sync_interval(seconds)` tool

## Example MCP Queries

```python
# List all blueprints
list_blueprints()

# Search for character-related blueprints
search_blueprints("Character", "name")

# Analyze a specific blueprint's complexity
analyze_blueprint_complexity("/Game/Blueprints/BP_PlayerCharacter")

# Get a blueprint with medium detail level
get_blueprint("/Game/Blueprints/BP_PlayerCharacter", detail_level=1)

# Get a blueprint with full detail level
get_blueprint("/Game/Blueprints/BP_PlayerCharacter", detail_level=2)

# Get a blueprint with events detail level
get_blueprint("/Game/Blueprints/BP_PlayerCharacter", detail_level=4)

# List all event nodes in a blueprint
list_blueprint_events("/Game/Blueprints/BP_PlayerCharacter")

# Get a specific event graph
get_event_graph("/Game/Blueprints/BP_PlayerCharacter", "BeginPlay")

# Force an immediate synchronization
force_blueprint_sync()

# Set synchronization interval to 30 seconds
set_sync_interval(30)
```

## Troubleshooting

- If blueprint data is not being synchronized, check that both servers are running
- Ensure the UE5 HTTP server is accessible at `http://localhost:8080`
- Check the MCP server logs for any connection errors
- Try forcing a sync using the `force_blueprint_sync()` tool