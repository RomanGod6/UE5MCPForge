# UE5MCPForge

An MCP (Model Context Protocol) server for Unreal Engine 5 blueprint data analysis. This server connects to your UE5 C++ plugin's HTTP server to fetch and analyze blueprint data.

## Overview

UE5MCPForge provides a bridge between your UE5 projects and AI assistants through the Model Context Protocol. It fetches blueprint data from your UE5 plugin via HTTP, processes it, and exposes it through MCP tools and resources.

Features:
- HTTP-based synchronization with UE5 plugin
- Blueprint search and analysis tools
- Blueprint complexity metrics
- Function and variable listing
- MCP resources for AI access

## Requirements

- Python 3.11 or higher
- UV (Python Package Manager written in Rust)
- UE5 C++ plugin with an HTTP server running on port 8080
- MCP client (such as MCP Inspector)

## Installation

### Clone the repository

```bash
git clone https://github.com/yourusername/UE5MCPForge.git
cd UE5MCPForge
```

### Install dependencies

Using pip:

```bash
pip install -e .
```

Using uv (recommended):

```bash
uv pip install -e .
```

## Usage

### Running in development mode

Development mode is useful when you're making changes to the server code:

```bash
cd UE5MCPForge
uv run mcp dev main.py
```

### Running in production mode

For Easy Install To Use With Claude Desktop - Auto Adds to Claude Config:

```bash
cd UE5MCPForge
uv run mcp install main.py
```

The server will run on port 3000 by default.

## Connecting to Claude

To connect the MCP server to Claude:

1. Start your UE5 plugin's HTTP server
2. Run the UE5MCPForge server
3. In Claude, use the `Run tool with mcp` command to connect to your server

## MCP Tools

The server provides several tools for blueprint analysis:

- `force_blueprint_sync`: Manually fetch blueprints from UE5 plugin
- `list_blueprints`: List all stored blueprints
- `search_blueprints`: Search blueprints by name, class, function, or variable
- `analyze_blueprint_complexity`: Analyze blueprint complexity metrics
- `list_blueprint_functions`: List functions in a specific blueprint
- `set_sync_interval`: Set the interval for automatic synchronization

## MCP Resources

The server provides resources for accessing blueprint data:

- `blueprints://all`: List all blueprints
- `blueprint://{path}`: Get details for a specific blueprint
- `blueprint://{path}/functions`: Get functions for a blueprint
- `blueprint://{path}/variables`: Get variables for a blueprint

## Configuration

UE5MCPForge uses the following configuration parameters (defined in main.py):

- `SYNC_INTERVAL`: How often to poll for updates (default: 60 seconds)
- `UE5_PLUGIN_URL`: URL of your UE5 plugin's HTTP server (default: http://localhost:8080)

## Troubleshooting

### Cannot connect to UE5 plugin HTTP server

If you see "Failed to connect to UE5 plugin HTTP server" in the logs:

1. Ensure your UE5 plugin is running
2. Verify it's serving HTTP requests on port 8080
3. Test the connection using `curl -X GET "http://localhost:8080/blueprints/all"`
4. Use the `force_blueprint_sync` tool to retry the connection

### No blueprints are displayed

If you see "No blueprints are currently stored" when accessing resources:

1. Use the `force_blueprint_sync` tool to trigger synchronization
2. Check the server logs for errors
3. Verify your UE5 plugin is returning blueprint data in the expected format

## License

[MIT License](LICENSE)