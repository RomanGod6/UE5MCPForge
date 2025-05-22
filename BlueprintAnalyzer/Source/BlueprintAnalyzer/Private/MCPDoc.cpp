#include "MCPDoc.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

FString FMCPDoc::GetDetailLevelDocs()
{
    // Create root object
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    
    // Add information about detail levels
    TArray<TSharedPtr<FJsonValue>> DetailLevelsArray;
    
    // Basic level
    TSharedPtr<FJsonObject> BasicLevelObj = MakeShareable(new FJsonObject);
    BasicLevelObj->SetNumberField(TEXT("level"), 0);
    BasicLevelObj->SetStringField(TEXT("name"), TEXT("Basic"));
    BasicLevelObj->SetStringField(TEXT("description"), TEXT("Basic information only: name, path, parent class"));
    BasicLevelObj->SetStringField(TEXT("usage"), TEXT("Best for listing many blueprints where only basic identification is needed"));
    DetailLevelsArray.Add(MakeShareable(new FJsonValueObject(BasicLevelObj)));
    
    // Medium level
    TSharedPtr<FJsonObject> MediumLevelObj = MakeShareable(new FJsonObject);
    MediumLevelObj->SetNumberField(TEXT("level"), 1);
    MediumLevelObj->SetStringField(TEXT("name"), TEXT("Medium"));
    MediumLevelObj->SetStringField(TEXT("description"), TEXT("Medium detail: basic info plus simplified functions and variables (without default values)"));
    MediumLevelObj->SetStringField(TEXT("usage"), TEXT("Good for getting an overview of a blueprint's capabilities without excess detail"));
    DetailLevelsArray.Add(MakeShareable(new FJsonValueObject(MediumLevelObj)));
    
    // Full level
    TSharedPtr<FJsonObject> FullLevelObj = MakeShareable(new FJsonObject);
    FullLevelObj->SetNumberField(TEXT("level"), 2);
    FullLevelObj->SetStringField(TEXT("name"), TEXT("Full"));
    FullLevelObj->SetStringField(TEXT("description"), TEXT("Full detail: complete information about functions and variables with all metadata"));
    FullLevelObj->SetStringField(TEXT("usage"), TEXT("For thorough analysis of blueprint functionality without visual graph data"));
    DetailLevelsArray.Add(MakeShareable(new FJsonValueObject(FullLevelObj)));
    
    // Graph level
    TSharedPtr<FJsonObject> GraphLevelObj = MakeShareable(new FJsonObject);
    GraphLevelObj->SetNumberField(TEXT("level"), 3);
    GraphLevelObj->SetStringField(TEXT("name"), TEXT("Graph"));
    GraphLevelObj->SetStringField(TEXT("description"), TEXT("Graph detail: everything including visual graph data with nodes and connections"));
    GraphLevelObj->SetStringField(TEXT("usage"), TEXT("For complete blueprint analysis including visual representation of the execution flow"));
    GraphLevelObj->SetStringField(TEXT("note"), TEXT("This level produces much larger responses and should be used with pagination parameters"));
    DetailLevelsArray.Add(MakeShareable(new FJsonValueObject(GraphLevelObj)));
    
    // Events level
    TSharedPtr<FJsonObject> EventsLevelObj = MakeShareable(new FJsonObject);
    EventsLevelObj->SetNumberField(TEXT("level"), 4);
    EventsLevelObj->SetStringField(TEXT("name"), TEXT("Events"));
    EventsLevelObj->SetStringField(TEXT("description"), TEXT("Events detail: focuses on event nodes and their associated graphs"));
    EventsLevelObj->SetStringField(TEXT("usage"), TEXT("For analyzing event-driven behavior and response patterns in blueprints"));
    EventsLevelObj->SetStringField(TEXT("note"), TEXT("Can be filtered by specific event names for detailed analysis of event graphs"));
    DetailLevelsArray.Add(MakeShareable(new FJsonValueObject(EventsLevelObj)));
    
    RootObject->SetArrayField(TEXT("detailLevels"), DetailLevelsArray);
    
    // Add examples of usage
    TSharedPtr<FJsonObject> ExamplesObj = MakeShareable(new FJsonObject);
    ExamplesObj->SetStringField(TEXT("listAll"), TEXT("/blueprints/all?detailLevel=0"));
    ExamplesObj->SetStringField(TEXT("getBlueprintMedium"), TEXT("/blueprints/path?path=/Game/MyBlueprint&detailLevel=1"));
    ExamplesObj->SetStringField(TEXT("getBlueprintFull"), TEXT("/blueprints/path?path=/Game/MyBlueprint&detailLevel=2"));
    ExamplesObj->SetStringField(TEXT("getBlueprintGraph"), TEXT("/blueprints/path?path=/Game/MyBlueprint&detailLevel=3&maxGraphs=5&maxNodes=20"));
    ExamplesObj->SetStringField(TEXT("getBlueprintEvents"), TEXT("/blueprints/path?path=/Game/MyBlueprint&detailLevel=4"));
    ExamplesObj->SetStringField(TEXT("getSpecificEventNodes"), TEXT("/blueprints/events?path=/Game/MyBlueprint&eventName=BeginPlay"));
    ExamplesObj->SetStringField(TEXT("getSpecificEventGraph"), TEXT("/blueprints/event-graph?path=/Game/MyBlueprint&eventName=BeginPlay&maxNodes=50"));
    
    RootObject->SetObjectField(TEXT("examples"), ExamplesObj);
    
    // Convert to string
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    return JsonString;
}

FString FMCPDoc::GetFullApiDocs()
{
    // Create root object
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    
    // Add blueprint analyzer version
    RootObject->SetStringField(TEXT("apiVersion"), TEXT("1.0.0"));
    RootObject->SetStringField(TEXT("name"), TEXT("Blueprint Analyzer API"));
    
    // Add endpoints documentation
    TArray<TSharedPtr<FJsonValue>> EndpointsArray;
    
    // List all blueprints endpoint
    TSharedPtr<FJsonObject> ListAllEndpoint = MakeShareable(new FJsonObject);
    ListAllEndpoint->SetStringField(TEXT("path"), TEXT("/blueprints/all"));
    ListAllEndpoint->SetStringField(TEXT("method"), TEXT("GET"));
    ListAllEndpoint->SetStringField(TEXT("description"), TEXT("Lists all blueprints in the project"));
    
    TArray<TSharedPtr<FJsonValue>> ListAllParamsArray;
    
    TSharedPtr<FJsonObject> DetailLevelParam = MakeShareable(new FJsonObject);
    DetailLevelParam->SetStringField(TEXT("name"), TEXT("detailLevel"));
    DetailLevelParam->SetStringField(TEXT("type"), TEXT("integer"));
    DetailLevelParam->SetBoolField(TEXT("required"), false);
    DetailLevelParam->SetStringField(TEXT("default"), TEXT("0"));
    DetailLevelParam->SetStringField(TEXT("description"), TEXT("Level of detail to extract (0=Basic, 1=Medium, 2=Full, 3=Graph, 4=Events)"));
    ListAllParamsArray.Add(MakeShareable(new FJsonValueObject(DetailLevelParam)));
    
    TSharedPtr<FJsonObject> LimitParam = MakeShareable(new FJsonObject);
    LimitParam->SetStringField(TEXT("name"), TEXT("limit"));
    LimitParam->SetStringField(TEXT("type"), TEXT("integer"));
    LimitParam->SetBoolField(TEXT("required"), false);
    LimitParam->SetStringField(TEXT("description"), TEXT("Maximum number of blueprints to return"));
    ListAllParamsArray.Add(MakeShareable(new FJsonValueObject(LimitParam)));
    
    TSharedPtr<FJsonObject> OffsetParam = MakeShareable(new FJsonObject);
    OffsetParam->SetStringField(TEXT("name"), TEXT("offset"));
    OffsetParam->SetStringField(TEXT("type"), TEXT("integer"));
    OffsetParam->SetBoolField(TEXT("required"), false);
    OffsetParam->SetStringField(TEXT("default"), TEXT("0"));
    OffsetParam->SetStringField(TEXT("description"), TEXT("Starting index for pagination"));
    ListAllParamsArray.Add(MakeShareable(new FJsonValueObject(OffsetParam)));
    
    ListAllEndpoint->SetArrayField(TEXT("parameters"), ListAllParamsArray);
    EndpointsArray.Add(MakeShareable(new FJsonValueObject(ListAllEndpoint)));
    
    // Get blueprint by path endpoint
    TSharedPtr<FJsonObject> GetBlueprintEndpoint = MakeShareable(new FJsonObject);
    GetBlueprintEndpoint->SetStringField(TEXT("path"), TEXT("/blueprints/path"));
    GetBlueprintEndpoint->SetStringField(TEXT("method"), TEXT("GET"));
    GetBlueprintEndpoint->SetStringField(TEXT("description"), TEXT("Gets a specific blueprint by its asset path"));
    
    TArray<TSharedPtr<FJsonValue>> GetBlueprintParamsArray;
    
    TSharedPtr<FJsonObject> PathParam = MakeShareable(new FJsonObject);
    PathParam->SetStringField(TEXT("name"), TEXT("path"));
    PathParam->SetStringField(TEXT("type"), TEXT("string"));
    PathParam->SetBoolField(TEXT("required"), true);
    PathParam->SetStringField(TEXT("description"), TEXT("Asset path of the blueprint (e.g. /Game/MyBlueprint)"));
    GetBlueprintParamsArray.Add(MakeShareable(new FJsonValueObject(PathParam)));
    
    TSharedPtr<FJsonObject> DetailLevelParam2 = MakeShareable(new FJsonObject);
    DetailLevelParam2->SetStringField(TEXT("name"), TEXT("detailLevel"));
    DetailLevelParam2->SetStringField(TEXT("type"), TEXT("integer"));
    DetailLevelParam2->SetBoolField(TEXT("required"), false);
    DetailLevelParam2->SetStringField(TEXT("default"), TEXT("2"));
    DetailLevelParam2->SetStringField(TEXT("description"), TEXT("Level of detail to extract (0=Basic, 1=Medium, 2=Full, 3=Graph, 4=Events)"));
    GetBlueprintParamsArray.Add(MakeShareable(new FJsonValueObject(DetailLevelParam2)));
    
    GetBlueprintEndpoint->SetArrayField(TEXT("parameters"), GetBlueprintParamsArray);
    EndpointsArray.Add(MakeShareable(new FJsonValueObject(GetBlueprintEndpoint)));
    
    // Get event nodes endpoint
    TSharedPtr<FJsonObject> GetEventNodesEndpoint = MakeShareable(new FJsonObject);
    GetEventNodesEndpoint->SetStringField(TEXT("path"), TEXT("/blueprints/events"));
    GetEventNodesEndpoint->SetStringField(TEXT("method"), TEXT("GET"));
    GetEventNodesEndpoint->SetStringField(TEXT("description"), TEXT("Gets all event nodes from a specific blueprint"));
    
    TArray<TSharedPtr<FJsonValue>> GetEventNodesParamsArray;
    
    TSharedPtr<FJsonObject> EventsPathParam = MakeShareable(new FJsonObject);
    EventsPathParam->SetStringField(TEXT("name"), TEXT("path"));
    EventsPathParam->SetStringField(TEXT("type"), TEXT("string"));
    EventsPathParam->SetBoolField(TEXT("required"), true);
    EventsPathParam->SetStringField(TEXT("description"), TEXT("Asset path of the blueprint (e.g. /Game/MyBlueprint)"));
    GetEventNodesParamsArray.Add(MakeShareable(new FJsonValueObject(EventsPathParam)));
    
    TSharedPtr<FJsonObject> EventNameParam = MakeShareable(new FJsonObject);
    EventNameParam->SetStringField(TEXT("name"), TEXT("eventName"));
    EventNameParam->SetStringField(TEXT("type"), TEXT("string"));
    EventNameParam->SetBoolField(TEXT("required"), false);
    EventNameParam->SetStringField(TEXT("description"), TEXT("Filter to only include a specific event by name (e.g. BeginPlay)"));
    GetEventNodesParamsArray.Add(MakeShareable(new FJsonValueObject(EventNameParam)));
    
    GetEventNodesEndpoint->SetArrayField(TEXT("parameters"), GetEventNodesParamsArray);
    EndpointsArray.Add(MakeShareable(new FJsonValueObject(GetEventNodesEndpoint)));
    
    // Get event graph endpoint
    TSharedPtr<FJsonObject> GetEventGraphEndpoint = MakeShareable(new FJsonObject);
    GetEventGraphEndpoint->SetStringField(TEXT("path"), TEXT("/blueprints/event-graph"));
    GetEventGraphEndpoint->SetStringField(TEXT("method"), TEXT("GET"));
    GetEventGraphEndpoint->SetStringField(TEXT("description"), TEXT("Gets a specific event graph by name from a blueprint"));
    
    TArray<TSharedPtr<FJsonValue>> GetEventGraphParamsArray;
    
    TSharedPtr<FJsonObject> EventGraphPathParam = MakeShareable(new FJsonObject);
    EventGraphPathParam->SetStringField(TEXT("name"), TEXT("path"));
    EventGraphPathParam->SetStringField(TEXT("type"), TEXT("string"));
    EventGraphPathParam->SetBoolField(TEXT("required"), true);
    EventGraphPathParam->SetStringField(TEXT("description"), TEXT("Asset path of the blueprint (e.g. /Game/MyBlueprint)"));
    GetEventGraphParamsArray.Add(MakeShareable(new FJsonValueObject(EventGraphPathParam)));
    
    TSharedPtr<FJsonObject> EventGraphNameParam = MakeShareable(new FJsonObject);
    EventGraphNameParam->SetStringField(TEXT("name"), TEXT("eventName"));
    EventGraphNameParam->SetStringField(TEXT("type"), TEXT("string"));
    EventGraphNameParam->SetBoolField(TEXT("required"), true);
    EventGraphNameParam->SetStringField(TEXT("description"), TEXT("Name of the event to get graph data for (e.g. BeginPlay)"));
    GetEventGraphParamsArray.Add(MakeShareable(new FJsonValueObject(EventGraphNameParam)));
    
    TSharedPtr<FJsonObject> MaxNodesParam = MakeShareable(new FJsonObject);
    MaxNodesParam->SetStringField(TEXT("name"), TEXT("maxNodes"));
    MaxNodesParam->SetStringField(TEXT("type"), TEXT("integer"));
    MaxNodesParam->SetBoolField(TEXT("required"), false);
    MaxNodesParam->SetStringField(TEXT("default"), TEXT("0"));
    MaxNodesParam->SetStringField(TEXT("description"), TEXT("Maximum number of nodes to return (0 = unlimited)"));
    GetEventGraphParamsArray.Add(MakeShareable(new FJsonValueObject(MaxNodesParam)));
    
    GetEventGraphEndpoint->SetArrayField(TEXT("parameters"), GetEventGraphParamsArray);
    EndpointsArray.Add(MakeShareable(new FJsonValueObject(GetEventGraphEndpoint)));
    
    // Add all endpoints to root
    RootObject->SetArrayField(TEXT("endpoints"), EndpointsArray);
    
    // Add detail levels documentation
    RootObject->SetStringField(TEXT("detailLevelDocs"), GetDetailLevelDocs());
    
    // Convert to string
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    return JsonString;
}