#include "BlueprintAnalyzer/Public/MCPHttpServer.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "HttpPath.h"
#include "BlueprintAnalyzer/Public/BlueprintDataExtractor.h"
#include "BlueprintAnalyzer/Public/BlueprintSearcher.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "BlueprintAnalyzer/Public/MCPIntegration.h"
#include "Modules/ModuleManager.h"

// Initialize static members
TSharedPtr<IHttpRouter> FMCPHttpServer::HttpRouter = nullptr;

bool FMCPHttpServer::Initialize(uint32 Port)
{
    // Get the HTTP server module
    FHttpServerModule& HttpServerModule = FModuleManager::LoadModuleChecked<FHttpServerModule>("HTTPServer");
    
    // Start all listeners - this is important to ensure the server is running
    HttpServerModule.StartAllListeners();
    
    // Create a new router
    HttpRouter = HttpServerModule.GetHttpRouter(Port);
    
    if (!HttpRouter.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create HTTP router on port %d"), Port);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("HTTP router created successfully on port %d"), Port);
    
    // Bind routes
    
    // GET /blueprints/all - List all blueprints
    auto AllBlueprintsDelegate = FHttpRequestHandler::CreateStatic(&FMCPHttpServer::HandleListAllBlueprints);
    HttpRouter->BindRoute(FHttpPath("/blueprints/all"), EHttpServerRequestVerbs::VERB_GET, AllBlueprintsDelegate);
    
    // GET /blueprints/search?query=X&type=Y - Search blueprints
    auto SearchBlueprintsDelegate = FHttpRequestHandler::CreateStatic(&FMCPHttpServer::HandleSearchBlueprints);
    HttpRouter->BindRoute(FHttpPath("/blueprints/search"), EHttpServerRequestVerbs::VERB_GET, SearchBlueprintsDelegate);
    
    // GET /blueprints/path - Get blueprint by path (using query parameter)
    auto GetBlueprintByPathDelegate = FHttpRequestHandler::CreateStatic(&FMCPHttpServer::HandleGetBlueprintByPath);
    HttpRouter->BindRoute(FHttpPath("/blueprints/path"), EHttpServerRequestVerbs::VERB_GET, GetBlueprintByPathDelegate);
    
    // GET /blueprints/function - Get function graph data
    auto GetFunctionGraphDelegate = FHttpRequestHandler::CreateStatic(&FMCPHttpServer::HandleGetFunctionGraph);
    HttpRouter->BindRoute(FHttpPath("/blueprints/function"), EHttpServerRequestVerbs::VERB_GET, GetFunctionGraphDelegate);
    
    // GET /blueprints/graph/nodes - Get nodes by type
    auto GetNodesByTypeDelegate = FHttpRequestHandler::CreateStatic(&FMCPHttpServer::HandleGetNodesByType);
    HttpRouter->BindRoute(FHttpPath("/blueprints/graph/nodes"), EHttpServerRequestVerbs::VERB_GET, GetNodesByTypeDelegate);
    
    // GET /blueprints/events - Get all event nodes from a blueprint
    auto GetEventNodesDelegate = FHttpRequestHandler::CreateStatic(&FMCPHttpServer::HandleGetEventNodes);
    HttpRouter->BindRoute(FHttpPath("/blueprints/events"), EHttpServerRequestVerbs::VERB_GET, GetEventNodesDelegate);
    
    // GET /blueprints/event-graph - Get a specific event graph by name
    auto GetEventGraphDelegate = FHttpRequestHandler::CreateStatic(&FMCPHttpServer::HandleGetEventGraph);
    HttpRouter->BindRoute(FHttpPath("/blueprints/event-graph"), EHttpServerRequestVerbs::VERB_GET, GetEventGraphDelegate);
    
    // GET /blueprints/references - Get references to and from a blueprint
    auto GetBlueprintReferencesDelegate = FHttpRequestHandler::CreateStatic(&FMCPHttpServer::HandleGetBlueprintReferences);
    HttpRouter->BindRoute(FHttpPath("/blueprints/references"), EHttpServerRequestVerbs::VERB_GET, GetBlueprintReferencesDelegate);
    
    // GET /docs - Get API documentation
    auto GetDocumentationDelegate = FHttpRequestHandler::CreateStatic(&FMCPHttpServer::HandleGetDocumentation);
    HttpRouter->BindRoute(FHttpPath("/docs"), EHttpServerRequestVerbs::VERB_GET, GetDocumentationDelegate);
    
    UE_LOG(LogTemp, Log, TEXT("HTTP routes for Blueprint Analyzer registered on port %d"), Port);
    return true;
}

void FMCPHttpServer::Shutdown()
{
    if (HttpRouter.IsValid())
    {
        // In UE5.5, we need to manually unbind each route
        // This may require storing route handles when binding routes
        // For now, just reset the router
        HttpRouter.Reset();
    }
}

bool FMCPHttpServer::HandleListAllBlueprints(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    // Parse detail level from query parameters (default to Basic if not specified)
    EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Basic;
    int32 Limit = 0; // 0 means no limit
    int32 Offset = 0;
    
    const TMap<FString, FString>& QueryParams = Request.QueryParams;
    
    // Parse detail level
    if (QueryParams.Contains(TEXT("detailLevel")))
    {
        FString DetailLevelStr = QueryParams.FindChecked(TEXT("detailLevel"));
        int32 DetailLevelValue = FCString::Atoi(*DetailLevelStr);
        
        // Convert string to enum (safely clamped to valid range)
        DetailLevelValue = FMath::Clamp(DetailLevelValue, 0, 4);  // Updated to include Events level
        DetailLevel = static_cast<EBlueprintDetailLevel>(DetailLevelValue);
    }
    
    // Parse pagination parameters
    if (QueryParams.Contains(TEXT("limit")))
    {
        FString LimitStr = QueryParams.FindChecked(TEXT("limit"));
        Limit = FCString::Atoi(*LimitStr);
        Limit = FMath::Max(0, Limit); // Ensure limit is non-negative
    }
    
    if (QueryParams.Contains(TEXT("offset")))
    {
        FString OffsetStr = QueryParams.FindChecked(TEXT("offset"));
        Offset = FCString::Atoi(*OffsetStr);
        Offset = FMath::Max(0, Offset); // Ensure offset is non-negative
    }
    
    // Get all blueprints with the specified detail level
    TArray<FBlueprintData> AllBlueprints = FBlueprintDataExtractor::GetAllBlueprints(DetailLevel);
    
    // Total count before pagination
    int32 TotalCount = AllBlueprints.Num();
    
    // Apply pagination if limit is specified
    if (Limit > 0 && AllBlueprints.Num() > 0)
    {
        // Make sure offset is within range
        Offset = FMath::Min(Offset, AllBlueprints.Num() - 1);
        
        // Calculate the end index
        int32 EndIndex = (Limit > 0) ? FMath::Min(Offset + Limit, AllBlueprints.Num()) : AllBlueprints.Num();
        
        // Create a new array with only the selected range
        TArray<FBlueprintData> PaginatedBlueprints;
        for (int32 i = Offset; i < EndIndex; ++i)
        {
            PaginatedBlueprints.Add(AllBlueprints[i]);
        }
        
        // Replace the original array with the paginated one
        AllBlueprints = PaginatedBlueprints;
    }
    
    // Create response with pagination metadata
    OnComplete(CreateJsonResponseWithMetadata(AllBlueprints, TotalCount, Limit, Offset));
    return true;
}

bool FMCPHttpServer::HandleSearchBlueprints(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    // Extract query parameters
    FString Query;
    FString SearchType = TEXT("name");
    int32 Limit = 0; // 0 means no limit
    int32 Offset = 0;
    
    // Parse query string
    const TMap<FString, FString>& QueryParams = Request.QueryParams;
    
    if (QueryParams.Contains(TEXT("query")))
    {
        Query = QueryParams.FindChecked(TEXT("query"));
    }
    else
    {
        OnComplete(CreateErrorResponse(400, TEXT("Missing 'query' parameter")));
        return true;
    }
    
    if (QueryParams.Contains(TEXT("type")))
    {
        SearchType = QueryParams.FindChecked(TEXT("type"));
    }
    
    // Parse pagination parameters
    if (QueryParams.Contains(TEXT("limit")))
    {
        FString LimitStr = QueryParams.FindChecked(TEXT("limit"));
        Limit = FCString::Atoi(*LimitStr);
        Limit = FMath::Max(0, Limit); // Ensure limit is non-negative
    }
    
    if (QueryParams.Contains(TEXT("offset")))
    {
        FString OffsetStr = QueryParams.FindChecked(TEXT("offset"));
        Offset = FCString::Atoi(*OffsetStr);
        Offset = FMath::Max(0, Offset); // Ensure offset is non-negative
    }
    
    // Parse detail level from query parameters (default to Basic if not specified)
    EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Basic;
    
    if (QueryParams.Contains(TEXT("detailLevel")))
    {
        FString DetailLevelStr = QueryParams.FindChecked(TEXT("detailLevel"));
        int32 DetailLevelValue = FCString::Atoi(*DetailLevelStr);
        
        // Convert string to enum (safely clamped to valid range)
        DetailLevelValue = FMath::Clamp(DetailLevelValue, 0, 3);  // Updated to include Graph level
        DetailLevel = static_cast<EBlueprintDetailLevel>(DetailLevelValue);
    }
    
    // Perform the search based on search type
    TArray<FBlueprintData> Results;
    
    if (SearchType == TEXT("name"))
    {
        Results = FBlueprintSearcher::SearchByName(Query, DetailLevel);
    }
    else if (SearchType == TEXT("parentClass"))
    {
        Results = FBlueprintSearcher::SearchByParentClass(Query, DetailLevel);
    }
    else if (SearchType == TEXT("function"))
    {
        Results = FBlueprintSearcher::SearchByFunction(Query, TArray<FString>(), DetailLevel);
    }
    else if (SearchType == TEXT("variable"))
    {
        Results = FBlueprintSearcher::SearchByVariable(Query, FString(), DetailLevel);
    }
    else
    {
        TMap<FString, FString> SearchParams;
        SearchParams.Add(SearchType, Query);
        Results = FBlueprintSearcher::SearchWithParameters(SearchParams, DetailLevel);
    }
    
    // Total count before pagination
    int32 TotalCount = Results.Num();
    
    // Apply pagination if limit is specified
    if (Limit > 0 && Results.Num() > 0)
    {
        // Make sure offset is within range
        Offset = FMath::Min(Offset, Results.Num() - 1);
        
        // Calculate the end index
        int32 EndIndex = (Limit > 0) ? FMath::Min(Offset + Limit, Results.Num()) : Results.Num();
        
        // Create a new array with only the selected range
        TArray<FBlueprintData> PaginatedResults;
        for (int32 i = Offset; i < EndIndex; ++i)
        {
            PaginatedResults.Add(Results[i]);
        }
        
        // Replace the original array with the paginated one
        Results = PaginatedResults;
    }
    
    // Create response with search results including pagination metadata
    OnComplete(CreateJsonResponseWithMetadata(Results, TotalCount, Limit, Offset));
    return true;
}

bool FMCPHttpServer::HandleGetBlueprintByPath(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    // Extract path from URL
    FString Path;
    
    // Get path from query parameter
    const TMap<FString, FString>& QueryParams = Request.QueryParams;
    
    if (QueryParams.Contains(TEXT("path")))
    {
        Path = QueryParams.FindChecked(TEXT("path"));
    }
    else
    {
        OnComplete(CreateErrorResponse(400, TEXT("Missing 'path' query parameter")));
        return true;
    }
    
    // Parse detail level from query parameters (default to Full for individual blueprint)
    EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Full;
    
    if (QueryParams.Contains(TEXT("detailLevel")))
    {
        FString DetailLevelStr = QueryParams.FindChecked(TEXT("detailLevel"));
        int32 DetailLevelValue = FCString::Atoi(*DetailLevelStr);
        
        // Convert string to enum (safely clamped to valid range)
        DetailLevelValue = FMath::Clamp(DetailLevelValue, 0, 3);  // Updated to include Graph level
        DetailLevel = static_cast<EBlueprintDetailLevel>(DetailLevelValue);
        
        // Log the detail level value for debugging
        UE_LOG(LogTemp, Warning, TEXT("GetBlueprintByPath: Using detail level %d"), DetailLevelValue);
    }
    
    // Parse graph pagination parameters (only applicable for detail level 3)
    FString GraphNameFilter;
    int32 MaxNodesPerGraph = 0; // 0 means no limit
    int32 MaxGraphs = 0; // 0 means no limit
    int32 GraphOffset = 0;
    
    if (QueryParams.Contains(TEXT("graphName")))
    {
        GraphNameFilter = QueryParams.FindChecked(TEXT("graphName"));
    }
    
    if (QueryParams.Contains(TEXT("maxNodes")))
    {
        FString MaxNodesStr = QueryParams.FindChecked(TEXT("maxNodes"));
        MaxNodesPerGraph = FCString::Atoi(*MaxNodesStr);
        MaxNodesPerGraph = FMath::Max(0, MaxNodesPerGraph); // Ensure non-negative
    }
    
    if (QueryParams.Contains(TEXT("maxGraphs")))
    {
        FString MaxGraphsStr = QueryParams.FindChecked(TEXT("maxGraphs"));
        MaxGraphs = FCString::Atoi(*MaxGraphsStr);
        MaxGraphs = FMath::Max(0, MaxGraphs); // Ensure non-negative
    }
    
    if (QueryParams.Contains(TEXT("graphOffset")))
    {
        FString GraphOffsetStr = QueryParams.FindChecked(TEXT("graphOffset"));
        GraphOffset = FCString::Atoi(*GraphOffsetStr);
        GraphOffset = FMath::Max(0, GraphOffset); // Ensure non-negative
    }
    
    // Get blueprint by path with the specified detail level
    TOptional<FBlueprintData> BlueprintData = FBlueprintDataExtractor::GetBlueprintByPath(Path, DetailLevel);
    
    if (BlueprintData.IsSet())
    {
        FBlueprintData Blueprint = BlueprintData.GetValue();
        
        // Apply graph pagination and filtering for detail level 3
        if (DetailLevel == EBlueprintDetailLevel::Graph)
        {
            // Original graph count for metadata
            int32 TotalGraphCount = Blueprint.Graphs.Num();
            
            // Filter by graph name if specified
            if (!GraphNameFilter.IsEmpty())
            {
                TArray<FBlueprintGraphData> FilteredGraphs;
                for (const FBlueprintGraphData& Graph : Blueprint.Graphs)
                {
                    if (Graph.Name.Contains(GraphNameFilter))
                    {
                        FilteredGraphs.Add(Graph);
                    }
                }
                Blueprint.Graphs = FilteredGraphs;
            }
            
            // Apply graph pagination
            if (Blueprint.Graphs.Num() > 0)
            {
                // Apply offset
                if (GraphOffset > 0 && GraphOffset < Blueprint.Graphs.Num())
                {
                    Blueprint.Graphs.RemoveAt(0, GraphOffset);
                }
                else if (GraphOffset >= Blueprint.Graphs.Num())
                {
                    // If offset is beyond available graphs, return empty array
                    Blueprint.Graphs.Empty();
                }
                
                // Apply maximum graphs limit
                if (MaxGraphs > 0 && Blueprint.Graphs.Num() > MaxGraphs)
                {
                    Blueprint.Graphs.SetNum(MaxGraphs);
                }
                
                // Apply node limit to each graph
                if (MaxNodesPerGraph > 0)
                {
                    for (FBlueprintGraphData& Graph : Blueprint.Graphs)
                    {
                        int32 TotalNodeCount = Graph.Nodes.Num();
                        
                        if (Graph.Nodes.Num() > MaxNodesPerGraph)
                        {
                            // Limit nodes
                            Graph.Nodes.SetNum(MaxNodesPerGraph);
                            
                            // Filter connections to only reference included nodes
                            TArray<FBlueprintConnectionData> FilteredConnections;
                            TSet<FString> IncludedNodeIds;
                            
                            // Create a set of included node IDs for quick lookup
                            for (const FBlueprintNodeData& Node : Graph.Nodes)
                            {
                                IncludedNodeIds.Add(Node.NodeId);
                            }
                            
                            // Filter connections to only those where both source and target nodes are included
                            for (const FBlueprintConnectionData& Connection : Graph.Connections)
                            {
                                if (IncludedNodeIds.Contains(Connection.SourceNodeId) &&
                                    IncludedNodeIds.Contains(Connection.TargetNodeId))
                                {
                                    FilteredConnections.Add(Connection);
                                }
                            }
                            
                            Graph.Connections = FilteredConnections;
                            
                            // Add node pagination metadata to the graph
                            TSharedPtr<FJsonObject> NodePaginationMetadata = MakeShareable(new FJsonObject);
                            NodePaginationMetadata->SetNumberField(TEXT("totalNodes"), TotalNodeCount);
                            NodePaginationMetadata->SetNumberField(TEXT("returnedNodes"), Graph.Nodes.Num());
                            NodePaginationMetadata->SetNumberField(TEXT("maxNodes"), MaxNodesPerGraph);
                            
                            FString MetadataJson;
                            TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&MetadataJson);
                            FJsonSerializer::Serialize(NodePaginationMetadata.ToSharedRef(), JsonWriter);
                            
                            Graph.Metadata.Add(TEXT("pagination"), MetadataJson);
                        }
                    }
                }
            }
            
            // Add graph pagination metadata to the blueprint
            TSharedPtr<FJsonObject> GraphPaginationMetadata = MakeShareable(new FJsonObject);
            GraphPaginationMetadata->SetNumberField(TEXT("totalGraphs"), TotalGraphCount);
            GraphPaginationMetadata->SetNumberField(TEXT("returnedGraphs"), Blueprint.Graphs.Num());
            GraphPaginationMetadata->SetNumberField(TEXT("maxGraphs"), MaxGraphs);
            GraphPaginationMetadata->SetNumberField(TEXT("graphOffset"), GraphOffset);
            
            FString MetadataJson;
            TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&MetadataJson);
            FJsonSerializer::Serialize(GraphPaginationMetadata.ToSharedRef(), JsonWriter);
            
            Blueprint.Metadata.Add(TEXT("graphPagination"), MetadataJson);
        }
        
        // Create array with single blueprint
        TArray<FBlueprintData> BlueprintArray;
        BlueprintArray.Add(Blueprint);
        
        // Create response with blueprint data
        OnComplete(CreateJsonResponse(BlueprintArray));
    }
    else
    {
        OnComplete(CreateErrorResponse(404, FString::Printf(TEXT("Blueprint not found at path: %s"), *Path)));
    }
    return true;
}

bool FMCPHttpServer::HandleGetFunctionGraph(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    // Extract parameters from URL
    FString Path, FunctionName;
    
    // Get parameters from query
    const TMap<FString, FString>& QueryParams = Request.QueryParams;
    
    if (QueryParams.Contains(TEXT("path")))
    {
        Path = QueryParams.FindChecked(TEXT("path"));
    }
    else
    {
        OnComplete(CreateErrorResponse(400, TEXT("Missing 'path' parameter")));
        return true;
    }
    
    if (QueryParams.Contains(TEXT("function")))
    {
        FunctionName = QueryParams.FindChecked(TEXT("function"));
    }
    else
    {
        OnComplete(CreateErrorResponse(400, TEXT("Missing 'function' parameter")));
        return true;
    }
    
    // Always use Graph detail level for this endpoint
    EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Graph;
    
    // Get the blueprint by path
    TOptional<FBlueprintData> BlueprintData = FBlueprintDataExtractor::GetBlueprintByPath(Path, DetailLevel);
    
    if (!BlueprintData.IsSet())
    {
        OnComplete(CreateErrorResponse(404, FString::Printf(TEXT("Blueprint not found at path: %s"), *Path)));
        return true;
    }
    
    // Find the function graph that matches the name
    FBlueprintData Blueprint = BlueprintData.GetValue();
    TOptional<FBlueprintGraphData> FunctionGraph;
    
    for (const FBlueprintGraphData& Graph : Blueprint.Graphs)
    {
        if (Graph.Name.Equals(FunctionName, ESearchCase::IgnoreCase) && Graph.GraphType.Equals(TEXT("function")))
        {
            FunctionGraph = Graph;
            break;
        }
    }
    
    if (!FunctionGraph.IsSet())
    {
        OnComplete(CreateErrorResponse(404, FString::Printf(TEXT("Function '%s' not found in blueprint: %s"),
            *FunctionName, *Blueprint.Name)));
        return true;
    }
    
    // Create a blueprint with just this function's graph
    FBlueprintData FunctionBlueprint = Blueprint;
    FunctionBlueprint.Graphs.Empty();
    FunctionBlueprint.Graphs.Add(FunctionGraph.GetValue());
    
    // Create array with single blueprint
    TArray<FBlueprintData> BlueprintArray;
    BlueprintArray.Add(FunctionBlueprint);
    
    // Create response with blueprint data
    OnComplete(CreateJsonResponse(BlueprintArray));
    return true;
}

bool FMCPHttpServer::HandleGetNodesByType(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    // Extract parameters from URL
    FString Path, NodeType;
    
    // Get parameters from query
    const TMap<FString, FString>& QueryParams = Request.QueryParams;
    
    if (QueryParams.Contains(TEXT("path")))
    {
        Path = QueryParams.FindChecked(TEXT("path"));
    }
    else
    {
        OnComplete(CreateErrorResponse(400, TEXT("Missing 'path' parameter")));
        return true;
    }
    
    if (QueryParams.Contains(TEXT("nodeType")))
    {
        NodeType = QueryParams.FindChecked(TEXT("nodeType"));
    }
    else
    {
        OnComplete(CreateErrorResponse(400, TEXT("Missing 'nodeType' parameter")));
        return true;
    }
    
    // Always use Graph detail level for this endpoint
    EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Graph;
    
    // Get the blueprint by path
    TOptional<FBlueprintData> BlueprintData = FBlueprintDataExtractor::GetBlueprintByPath(Path, DetailLevel);
    
    if (!BlueprintData.IsSet())
    {
        OnComplete(CreateErrorResponse(404, FString::Printf(TEXT("Blueprint not found at path: %s"), *Path)));
        return true;
    }
    
    // Filter graphs to only include nodes of the specified type
    FBlueprintData Blueprint = BlueprintData.GetValue();
    FBlueprintData FilteredBlueprint = Blueprint;
    FilteredBlueprint.Graphs.Empty();
    
    for (const FBlueprintGraphData& Graph : Blueprint.Graphs)
    {
        FBlueprintGraphData FilteredGraph = Graph;
        FilteredGraph.Nodes.Empty();
        
        // Filter nodes of the specified type
        for (const FBlueprintNodeData& Node : Graph.Nodes)
        {
            if (Node.NodeType.Contains(NodeType))
            {
                FilteredGraph.Nodes.Add(Node);
            }
        }
        
        // Filter connections to only include those connected to the filtered nodes
        FilteredGraph.Connections.Empty();
        for (const FBlueprintConnectionData& Connection : Graph.Connections)
        {
            bool SourceNodeIncluded = false;
            bool TargetNodeIncluded = false;
            
            // Check if source and target nodes are included in the filtered nodes
            for (const FBlueprintNodeData& Node : FilteredGraph.Nodes)
            {
                if (Node.NodeId == Connection.SourceNodeId)
                {
                    SourceNodeIncluded = true;
                }
                if (Node.NodeId == Connection.TargetNodeId)
                {
                    TargetNodeIncluded = true;
                }
            }
            
            // Only include connections where both source and target nodes are included
            if (SourceNodeIncluded && TargetNodeIncluded)
            {
                FilteredGraph.Connections.Add(Connection);
            }
        }
        
        // Only add graph if it has nodes
        if (FilteredGraph.Nodes.Num() > 0)
        {
            FilteredBlueprint.Graphs.Add(FilteredGraph);
        }
    }
    
    // Create array with single blueprint
    TArray<FBlueprintData> BlueprintArray;
    BlueprintArray.Add(FilteredBlueprint);
    
    // Create response with blueprint data
    OnComplete(CreateJsonResponse(BlueprintArray));
    return true;
}

bool FMCPHttpServer::HandleGetDocumentation(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    // Check if a specific documentation type is requested
    FString DocType = TEXT("all"); // Default to "all" documentation
    
    const TMap<FString, FString>& QueryParams = Request.QueryParams;
    if (QueryParams.Contains(TEXT("type")))
    {
        DocType = QueryParams.FindChecked(TEXT("type"));
    }
    
    FString JsonContent;
    
    // Return different documentation based on requested type
    if (DocType.Equals(TEXT("detailLevels"), ESearchCase::IgnoreCase))
    {
        // Detail level documentation only
        JsonContent = FMCPDoc::GetDetailLevelDocs();
    }
    else
    {
        // Full API documentation (default)
        JsonContent = FMCPDoc::GetFullApiDocs();
    }
    
    // Create and return the response
    OnComplete(CreateJsonResponse(JsonContent));
    return true;
}

TUniquePtr<FHttpServerResponse> FMCPHttpServer::CreateJsonResponse(const FString& JsonContent)
{
    // In UE 5.5, we need to use the FHttpServerResponse::Create method
    TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(JsonContent, TEXT("application/json"));
    
    // Set the response code
    Response->Code = EHttpServerResponseCodes::Ok;
    
    // Add CORS headers
    Response->Headers.Add(TEXT("Access-Control-Allow-Origin"), TArray<FString>{TEXT("*")});
    Response->Headers.Add(TEXT("Access-Control-Allow-Methods"), TArray<FString>{TEXT("GET, OPTIONS")});
    Response->Headers.Add(TEXT("Access-Control-Allow-Headers"), TArray<FString>{TEXT("Content-Type, Authorization")});
    
    return Response;
}

TUniquePtr<FHttpServerResponse> FMCPHttpServer::CreateJsonResponse(const TArray<FBlueprintData>& BlueprintsData)
{
    // Create a JSON array to store blueprint JSON objects
    TArray<TSharedPtr<FJsonValue>> BlueprintJsonArray;
    
    // Convert each blueprint to a JSON object and add to array
    for (const FBlueprintData& Blueprint : BlueprintsData)
    {
        // Get JSON string for this blueprint
        FString BlueprintJson = FMCPIntegration::BlueprintDataToJSON(Blueprint);
        
        // Parse the JSON string back to a JSON object
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(BlueprintJson);
        TSharedPtr<FJsonObject> BlueprintJsonObject;
        
        if (FJsonSerializer::Deserialize(JsonReader, BlueprintJsonObject) && BlueprintJsonObject.IsValid())
        {
            // Add JSON object to array
            BlueprintJsonArray.Add(MakeShareable(new FJsonValueObject(BlueprintJsonObject)));
        }
    }
    
    // Create root object
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    RootObject->SetArrayField(TEXT("blueprints"), BlueprintJsonArray);
    
    // Serialize to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), JsonWriter);
    
    // Create response
    return CreateJsonResponse(OutputString);
}

TUniquePtr<FHttpServerResponse> FMCPHttpServer::CreateErrorResponse(int32 ErrorCode, const FString& ErrorMessage)
{
    // Create error JSON
    TSharedPtr<FJsonObject> ErrorObject = MakeShareable(new FJsonObject);
    ErrorObject->SetNumberField(TEXT("code"), ErrorCode);
    ErrorObject->SetStringField(TEXT("message"), ErrorMessage);
    
    // Serialize to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObject.ToSharedRef(), JsonWriter);
    
    // In UE 5.5, we need to use the FHttpServerResponse::Create method
    TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(OutputString, TEXT("application/json"));
    
    // Set the response code
    Response->Code = static_cast<EHttpServerResponseCodes>(ErrorCode);
    
    // Add CORS headers
    Response->Headers.Add(TEXT("Access-Control-Allow-Origin"), TArray<FString>{TEXT("*")});
    
    return Response;
}

TUniquePtr<FHttpServerResponse> FMCPHttpServer::CreateJsonResponseWithMetadata(
    const TArray<FBlueprintData>& BlueprintsData,
    int32 TotalCount,
    int32 Limit,
    int32 Offset)
{
    // Create a JSON array to store blueprint JSON objects
    TArray<TSharedPtr<FJsonValue>> BlueprintJsonArray;
    
    // Convert each blueprint to a JSON object and add to array
    for (const FBlueprintData& Blueprint : BlueprintsData)
    {
        // Get JSON string for this blueprint
        FString BlueprintJson = FMCPIntegration::BlueprintDataToJSON(Blueprint);
        
        // Parse the JSON string back to a JSON object
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(BlueprintJson);
        TSharedPtr<FJsonObject> BlueprintJsonObject;
        
        if (FJsonSerializer::Deserialize(JsonReader, BlueprintJsonObject) && BlueprintJsonObject.IsValid())
        {
            // Add JSON object to array
            BlueprintJsonArray.Add(MakeShareable(new FJsonValueObject(BlueprintJsonObject)));
        }
    }
    
    // Create root object
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    RootObject->SetArrayField(TEXT("blueprints"), BlueprintJsonArray);
    
    // Add metadata
    TSharedPtr<FJsonObject> MetadataObject = MakeShareable(new FJsonObject);
    MetadataObject->SetNumberField(TEXT("totalCount"), TotalCount);
    MetadataObject->SetNumberField(TEXT("count"), BlueprintsData.Num());
    MetadataObject->SetNumberField(TEXT("limit"), Limit);
    MetadataObject->SetNumberField(TEXT("offset"), Offset);
    
    RootObject->SetObjectField(TEXT("metadata"), MetadataObject);
    
    // Serialize to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), JsonWriter);
    
    // Create response
    return CreateJsonResponse(OutputString);
}

bool FMCPHttpServer::HandleGetEventNodes(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    // Parse query parameters
    const TMap<FString, FString>& QueryParams = Request.QueryParams;
    
    // Check required parameter: path
    if (!QueryParams.Contains(TEXT("path")))
    {
        UE_LOG(LogTemp, Error, TEXT("Missing required parameter 'path' in /blueprints/events request"));
        OnComplete(CreateErrorResponse(400, TEXT("Missing required parameter: path")));
        return true;
    }
    
    // Get blueprint path
    FString BlueprintPath = QueryParams.FindChecked(TEXT("path"));
    
    // Get optional event name filter
    FString EventName;
    if (QueryParams.Contains(TEXT("eventName")))
    {
        EventName = QueryParams.FindChecked(TEXT("eventName"));
    }
    
    // Get blueprint asset
    TOptional<FBlueprintData> BlueprintData = FBlueprintDataExtractor::GetBlueprintByPath(
        BlueprintPath,
        EBlueprintDetailLevel::Events,  // Use Events detail level
        EventName);                      // Filter by event name if provided
    
    if (!BlueprintData.IsSet())
    {
        UE_LOG(LogTemp, Error, TEXT("Blueprint not found: %s"), *BlueprintPath);
        OnComplete(CreateErrorResponse(404, FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintPath)));
        return true;
    }
    
    // Create JSON response with the event nodes
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    
    // Add basic blueprint information
    RootObject->SetStringField(TEXT("name"), BlueprintData.GetValue().Name);
    RootObject->SetStringField(TEXT("path"), BlueprintData.GetValue().Path);
    RootObject->SetStringField(TEXT("parentClass"), BlueprintData.GetValue().ParentClass);
    
    // Event nodes are stored in a special graph in the Graphs array
    if (BlueprintData.GetValue().Graphs.Num() > 0)
    {
        // Convert event nodes to JSON
        TArray<TSharedPtr<FJsonValue>> EventNodesArray;
        for (const FBlueprintNodeData& NodeData : BlueprintData.GetValue().Graphs[0].Nodes)
        {
            TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject);
            NodeObject->SetStringField(TEXT("eventName"), NodeData.Title);
            NodeObject->SetStringField(TEXT("graphName"), NodeData.Properties.Contains(TEXT("GraphName")) ?
                                                       NodeData.Properties[TEXT("GraphName")] : TEXT(""));
            
            // Add scope information if available
            if (NodeData.Properties.Contains(TEXT("EventScope")))
            {
                NodeObject->SetStringField(TEXT("eventScope"), NodeData.Properties[TEXT("EventScope")]);
            }
            
            EventNodesArray.Add(MakeShareable(new FJsonValueObject(NodeObject)));
        }
        
        RootObject->SetArrayField(TEXT("events"), EventNodesArray);
        
        // Add event count
        RootObject->SetNumberField(TEXT("eventCount"), BlueprintData.GetValue().Graphs[0].Nodes.Num());
    }
    else
    {
        RootObject->SetArrayField(TEXT("events"), TArray<TSharedPtr<FJsonValue>>());
        RootObject->SetNumberField(TEXT("eventCount"), 0);
    }
    
    // Add filter information if provided
    if (!EventName.IsEmpty())
    {
        RootObject->SetStringField(TEXT("filteredByEvent"), EventName);
    }
    
    // Serialize JSON to string
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    OnComplete(CreateJsonResponse(JsonString));
    return true;
}

bool FMCPHttpServer::HandleGetEventGraph(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    // Parse query parameters
    const TMap<FString, FString>& QueryParams = Request.QueryParams;
    
    // Check required parameters: path and eventName
    if (!QueryParams.Contains(TEXT("path")))
    {
        UE_LOG(LogTemp, Error, TEXT("Missing required parameter 'path' in /blueprints/event-graph request"));
        OnComplete(CreateErrorResponse(400, TEXT("Missing required parameter: path")));
        return true;
    }
    
    if (!QueryParams.Contains(TEXT("eventName")))
    {
        UE_LOG(LogTemp, Error, TEXT("Missing required parameter 'eventName' in /blueprints/event-graph request"));
        OnComplete(CreateErrorResponse(400, TEXT("Missing required parameter: eventName")));
        return true;
    }
    
    // Get blueprint path and event name
    FString BlueprintPath = QueryParams.FindChecked(TEXT("path"));
    FString EventName = QueryParams.FindChecked(TEXT("eventName"));
    
    // Get optional maxNodes parameter (default to 0 = unlimited)
    int32 MaxNodes = 0;
    if (QueryParams.Contains(TEXT("maxNodes")))
    {
        FString MaxNodesStr = QueryParams.FindChecked(TEXT("maxNodes"));
        MaxNodes = FCString::Atoi(*MaxNodesStr);
    }
    
    // Load the blueprint asset
    UObject* BlueprintAsset = nullptr;
    
#if WITH_EDITOR
    BlueprintAsset = LoadObject<UObject>(nullptr, *BlueprintPath);
#endif
    
    if (!BlueprintAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("Blueprint not found: %s"), *BlueprintPath);
        OnComplete(CreateErrorResponse(404, FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintPath)));
        return true;
    }
    
    // Cast to UBlueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintAsset);
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("Asset is not a blueprint: %s"), *BlueprintPath);
        OnComplete(CreateErrorResponse(400, FString::Printf(TEXT("Asset is not a blueprint: %s"), *BlueprintPath)));
        return true;
    }
    
    // Get the event graph data
    // Since GetEventGraph is private, use GetBlueprintByPath with proper filtering
    TOptional<FBlueprintData> BlueprintData = FBlueprintDataExtractor::GetBlueprintByPath(
        BlueprintPath,
        EBlueprintDetailLevel::Events,
        EventName,
        1,    // Get only one graph (the event graph)
        MaxNodes);
    
    if (!BlueprintData.IsSet() || BlueprintData.GetValue().Graphs.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Event node not found: %s in blueprint %s"), *EventName, *BlueprintPath);
        OnComplete(CreateErrorResponse(404, FString::Printf(TEXT("Event node not found: %s in blueprint %s"), *EventName, *BlueprintPath)));
        return true;
    }
    
    // Get the first graph which should be our event graph
    FBlueprintGraphData EventGraph = BlueprintData.GetValue().Graphs[0];
    
    // Create JSON response with the event graph
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    
    // Add basic information
    RootObject->SetStringField(TEXT("blueprintName"), Blueprint->GetName());
    RootObject->SetStringField(TEXT("blueprintPath"), BlueprintPath);
    RootObject->SetStringField(TEXT("eventName"), EventName);
    
    // Add graph information
    TSharedPtr<FJsonObject> GraphObject = MakeShareable(new FJsonObject);
    
    // Set graph properties
    GraphObject->SetStringField(TEXT("name"), EventGraph.Name);
    GraphObject->SetStringField(TEXT("type"), EventGraph.GraphType);
    
    // Add nodes array
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const FBlueprintNodeData& NodeData : EventGraph.Nodes)
    {
        TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject);
        
        // Add node properties
        NodeObject->SetStringField(TEXT("id"), NodeData.NodeId);
        NodeObject->SetStringField(TEXT("type"), NodeData.NodeType);
        NodeObject->SetStringField(TEXT("title"), NodeData.Title);
        NodeObject->SetNumberField(TEXT("positionX"), NodeData.PositionX);
        NodeObject->SetNumberField(TEXT("positionY"), NodeData.PositionY);
        
        if (!NodeData.Comment.IsEmpty())
        {
            NodeObject->SetStringField(TEXT("comment"), NodeData.Comment);
        }
        
        // Add input pins
        TArray<TSharedPtr<FJsonValue>> InputPinsArray;
        for (const FBlueprintPinData& PinData : NodeData.InputPins)
        {
            TSharedPtr<FJsonObject> PinObject = MakeShareable(new FJsonObject);
            PinObject->SetStringField(TEXT("id"), PinData.PinId);
            PinObject->SetStringField(TEXT("name"), PinData.Name);
            PinObject->SetBoolField(TEXT("isExecution"), PinData.IsExecution);
            PinObject->SetStringField(TEXT("dataType"), PinData.DataType);
            PinObject->SetBoolField(TEXT("isConnected"), PinData.IsConnected);
            
            if (!PinData.DefaultValue.IsEmpty())
            {
                PinObject->SetStringField(TEXT("defaultValue"), PinData.DefaultValue);
            }
            
            InputPinsArray.Add(MakeShareable(new FJsonValueObject(PinObject)));
        }
        NodeObject->SetArrayField(TEXT("inputPins"), InputPinsArray);
        
        // Add output pins
        TArray<TSharedPtr<FJsonValue>> OutputPinsArray;
        for (const FBlueprintPinData& PinData : NodeData.OutputPins)
        {
            TSharedPtr<FJsonObject> PinObject = MakeShareable(new FJsonObject);
            PinObject->SetStringField(TEXT("id"), PinData.PinId);
            PinObject->SetStringField(TEXT("name"), PinData.Name);
            PinObject->SetBoolField(TEXT("isExecution"), PinData.IsExecution);
            PinObject->SetStringField(TEXT("dataType"), PinData.DataType);
            PinObject->SetBoolField(TEXT("isConnected"), PinData.IsConnected);
            
            if (!PinData.DefaultValue.IsEmpty())
            {
                PinObject->SetStringField(TEXT("defaultValue"), PinData.DefaultValue);
            }
            
            OutputPinsArray.Add(MakeShareable(new FJsonValueObject(PinObject)));
        }
        NodeObject->SetArrayField(TEXT("outputPins"), OutputPinsArray);
        
        // Add node to array
        NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObject)));
    }
    GraphObject->SetArrayField(TEXT("nodes"), NodesArray);
    
    // Add connections array
    TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
    for (const FBlueprintConnectionData& ConnectionData : EventGraph.Connections)
    {
        TSharedPtr<FJsonObject> ConnectionObject = MakeShareable(new FJsonObject);
        ConnectionObject->SetStringField(TEXT("sourceNodeId"), ConnectionData.SourceNodeId);
        ConnectionObject->SetStringField(TEXT("sourcePinId"), ConnectionData.SourcePinId);
        ConnectionObject->SetStringField(TEXT("targetNodeId"), ConnectionData.TargetNodeId);
        ConnectionObject->SetStringField(TEXT("targetPinId"), ConnectionData.TargetPinId);
        
        ConnectionsArray.Add(MakeShareable(new FJsonValueObject(ConnectionObject)));
    }
    GraphObject->SetArrayField(TEXT("connections"), ConnectionsArray);
    
    // Add metadata if available
    if (EventGraph.Metadata.Num() > 0)
    {
        TSharedPtr<FJsonObject> MetadataObject = MakeShareable(new FJsonObject);
        for (const TPair<FString, FString>& Pair : EventGraph.Metadata)
        {
            MetadataObject->SetStringField(Pair.Key, Pair.Value);
        }
        GraphObject->SetObjectField(TEXT("metadata"), MetadataObject);
    }
    
    // Add graph to root object
    RootObject->SetObjectField(TEXT("graph"), GraphObject);
    
    // Serialize JSON to string
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    OnComplete(CreateJsonResponse(JsonString));
    return true;
}

bool FMCPHttpServer::HandleGetBlueprintReferences(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    // Extract the required path parameter
    FString BlueprintPath;
    bool bIncludeIndirect = false;

    const TMap<FString, FString>& QueryParams = Request.QueryParams;
    
    // Check for the required path parameter
    if (!QueryParams.Contains(TEXT("path")))
    {
        UE_LOG(LogTemp, Error, TEXT("Missing required parameter: path"));
        OnComplete(CreateErrorResponse(400, TEXT("Missing required parameter: path")));
        return true;
    }
    
    BlueprintPath = QueryParams.FindChecked(TEXT("path"));
    
    // Check for the optional includeIndirect parameter
    if (QueryParams.Contains(TEXT("includeIndirect")))
    {
        const FString& IncludeIndirectStr = QueryParams.FindChecked(TEXT("includeIndirect"));
        bIncludeIndirect = IncludeIndirectStr.Equals(TEXT("true"), ESearchCase::IgnoreCase);
    }
    
    // Get references for the blueprint
    TArray<FBlueprintReferenceData> References = FBlueprintDataExtractor::GetBlueprintReferences(BlueprintPath, bIncludeIndirect);
    
    // Create JSON response
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    
    // Add basic information
    RootObject->SetStringField(TEXT("blueprintPath"), BlueprintPath);
    RootObject->SetBoolField(TEXT("includeIndirect"), bIncludeIndirect);
    RootObject->SetNumberField(TEXT("referenceCount"), References.Num());
    
    // Add references array
    TArray<TSharedPtr<FJsonValue>> ReferencesArray;
    for (const FBlueprintReferenceData& Reference : References)
    {
        TSharedPtr<FJsonObject> ReferenceObject = MakeShareable(new FJsonObject);
        
        // Add reference properties
        ReferenceObject->SetNumberField(TEXT("referenceType"), static_cast<int32>(Reference.ReferenceType));
        ReferenceObject->SetNumberField(TEXT("direction"), static_cast<int32>(Reference.Direction));
        ReferenceObject->SetStringField(TEXT("blueprintPath"), Reference.BlueprintPath);
        ReferenceObject->SetStringField(TEXT("blueprintName"), Reference.BlueprintName);
        ReferenceObject->SetStringField(TEXT("context"), Reference.Context);
        ReferenceObject->SetBoolField(TEXT("isIndirect"), Reference.bIsIndirect);
        
        // Add reference chain for indirect references
        if (Reference.bIsIndirect && Reference.ReferenceChain.Num() > 0)
        {
            TArray<TSharedPtr<FJsonValue>> ChainArray;
            for (const FString& ChainPath : Reference.ReferenceChain)
            {
                ChainArray.Add(MakeShareable(new FJsonValueString(ChainPath)));
            }
            ReferenceObject->SetArrayField(TEXT("referenceChain"), ChainArray);
        }
        
        // Add properties if any
        if (Reference.Properties.Num() > 0)
        {
            TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject);
            for (const TPair<FString, FString>& Pair : Reference.Properties)
            {
                PropertiesObject->SetStringField(Pair.Key, Pair.Value);
            }
            ReferenceObject->SetObjectField(TEXT("properties"), PropertiesObject);
        }
        
        // Add to array
        ReferencesArray.Add(MakeShareable(new FJsonValueObject(ReferenceObject)));
    }
    
    // Add references array to root
    RootObject->SetArrayField(TEXT("references"), ReferencesArray);
    
    // Add reference type mapping for easier client-side parsing
    TSharedPtr<FJsonObject> ReferenceTypesObject = MakeShareable(new FJsonObject);
    ReferenceTypesObject->SetStringField(TEXT("0"), TEXT("Inheritance"));
    ReferenceTypesObject->SetStringField(TEXT("1"), TEXT("FunctionCall"));
    ReferenceTypesObject->SetStringField(TEXT("2"), TEXT("VariableType"));
    ReferenceTypesObject->SetStringField(TEXT("3"), TEXT("DirectReference"));
    ReferenceTypesObject->SetStringField(TEXT("4"), TEXT("IndirectReference"));
    ReferenceTypesObject->SetStringField(TEXT("5"), TEXT("EventChain"));
    ReferenceTypesObject->SetStringField(TEXT("6"), TEXT("DataFlow"));
    RootObject->SetObjectField(TEXT("referenceTypes"), ReferenceTypesObject);
    
    // Add direction mapping
    TSharedPtr<FJsonObject> DirectionsObject = MakeShareable(new FJsonObject);
    DirectionsObject->SetStringField(TEXT("0"), TEXT("Outgoing"));
    DirectionsObject->SetStringField(TEXT("1"), TEXT("Incoming"));
    RootObject->SetObjectField(TEXT("directions"), DirectionsObject);
    
    // Serialize to JSON
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    OnComplete(CreateJsonResponse(JsonString));
    return true;
}