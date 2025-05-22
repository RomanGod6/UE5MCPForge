#pragma once

#include "CoreMinimal.h"
#include "BlueprintData.h"
#include "HttpServerModule.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"
#include "MCPDoc.h"

/**
 * A HTTP server for handling MCP requests from external apps
 * Allows the Python MCP server to automatically fetch blueprint data
 */
class BLUEPRINTANALYZER_API FMCPHttpServer
{
public:
    /**
     * Initialize the HTTP server
     * @param Port Port number to listen on
     * @return True if initialization was successful
     */
    static bool Initialize(uint32 Port);
    
    /**
     * Shutdown the HTTP server
     */
    static void Shutdown();
    
private:
    /** The HTTP router instance */
    static TSharedPtr<IHttpRouter> HttpRouter;
    
    /**
     * Handle GET /blueprints/all request to retrieve all blueprints
     * Query parameters:
     *   - detailLevel: (optional) Level of detail to extract (0=Basic, 1=Medium, 2=Full, 3=Graph, 4=Events), defaults to 0 (Basic)
     *   - limit: (optional) Maximum number of blueprints to return, defaults to all
     *   - offset: (optional) Starting index for pagination, defaults to 0
     */
    static bool HandleListAllBlueprints(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    
    /**
     * Handle GET /blueprints/search request to search blueprints
     * Query parameters:
     *   - query: (required) The search query string
     *   - type: (optional) The search type (name, parentClass, function, variable), defaults to "name"
     *   - detailLevel: (optional) Level of detail to extract (0=Basic, 1=Medium, 2=Full, 3=Graph, 4=Events), defaults to 0 (Basic)
     *   - limit: (optional) Maximum number of blueprints to return, defaults to all
     *   - offset: (optional) Starting index for pagination, defaults to 0
     */
    static bool HandleSearchBlueprints(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    
    /**
     * Handle GET /blueprints/path request to get a specific blueprint
     * Query parameters:
     *   - path: (required) The asset path of the blueprint
     *   - detailLevel: (optional) Level of detail to extract (0=Basic, 1=Medium, 2=Full, 3=Graph, 4=Events), defaults to 2 (Full)
     *   - graphName: (optional) Filter to only include a specific graph by name (for detail level 3)
     *   - maxNodes: (optional) Maximum number of nodes to return per graph (for detail level 3)
     *   - maxGraphs: (optional) Maximum number of graphs to return (for detail level 3)
     *   - graphOffset: (optional) Starting index for graph pagination (for detail level 3)
     */
    static bool HandleGetBlueprintByPath(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    
    /**
     * Handle GET /blueprints/function request to get a specific function's graph data
     * Query parameters:
     *   - path: (required) The asset path of the blueprint
     *   - function: (required) The name of the function to get graph data for
     */
    static bool HandleGetFunctionGraph(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    
    /**
     * Handle GET /blueprints/graph/nodes request to get nodes of a specific type
     * Query parameters:
     *   - path: (required) The asset path of the blueprint
     *   - nodeType: (required) The type of nodes to filter for (e.g. K2Node_CallFunction)
     */
    static bool HandleGetNodesByType(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    
    /**
     * Handle GET /docs request to get API documentation
     * Query parameters:
     *   - type: (optional) Type of documentation to get (e.g., "detailLevels", "all"), defaults to "all"
     */
    static bool HandleGetDocumentation(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    
    /**
     * Handle GET /blueprints/events request to get all event nodes from a blueprint
     * Query parameters:
     *   - path: (required) The asset path of the blueprint
     *   - eventName: (optional) Filter to only include a specific event by name
     */
    static bool HandleGetEventNodes(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    
    /**
     * Handle GET /blueprints/event-graph request to get a specific event graph by name
     * Query parameters:
     *   - path: (required) The asset path of the blueprint
     *   - eventName: (required) The name of the event to get graph data for
     *   - maxNodes: (optional) Maximum number of nodes per graph to extract (0 = unlimited)
     */
    static bool HandleGetEventGraph(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    
    /**
     * Handle GET /blueprints/references request to get references to and from a blueprint
     * Query parameters:
     *   - path: (required) The asset path of the blueprint
     *   - includeIndirect: (optional) Whether to include indirect references (default: false)
     */
    static bool HandleGetBlueprintReferences(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    
    /**
     * Create HTTP response with blueprint data in JSON format
     */
    static TUniquePtr<FHttpServerResponse> CreateJsonResponse(const FString& JsonContent);
    
    /**
     * Create HTTP response with blueprint data array in JSON format
     */
    static TUniquePtr<FHttpServerResponse> CreateJsonResponse(const TArray<FBlueprintData>& BlueprintsData);
    
    /**
     * Create error response
     */
    static TUniquePtr<FHttpServerResponse> CreateErrorResponse(int32 ErrorCode, const FString& ErrorMessage);
    
    /**
     * Create JSON response with metadata (pagination, etc.)
     */
    static TUniquePtr<FHttpServerResponse> CreateJsonResponseWithMetadata(
        const TArray<FBlueprintData>& BlueprintsData,
        int32 TotalCount,
        int32 Limit,
        int32 Offset);
};