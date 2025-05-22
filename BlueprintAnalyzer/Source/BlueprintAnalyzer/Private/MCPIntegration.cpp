#include "BlueprintAnalyzer/Public/MCPIntegration.h"
#include "HttpModule.h"
#include "BlueprintAnalyzer/Public/BlueprintData.h"
#include "BlueprintAnalyzer/Public/BlueprintDataExtractor.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"

// Initialize static members
FString FMCPIntegration::ServerURL;
FString FMCPIntegration::APIKey;
bool FMCPIntegration::bInitialized = false;

void FMCPIntegration::Initialize(const FString& InServerURL, const FString& InAPIKey)
{
    ServerURL = InServerURL;
    APIKey = InAPIKey;
    bInitialized = true;
    
    // Set up timer for automatic exports
    if (GEngine && GEngine->GetWorld())
    {
        UWorld* World = GEngine->GetWorld();
        if (World)
        {
            // Initial export
            ExportBlueprintsToFile();
            
            // Set timer for periodic exports
            FTimerDelegate TimerDelegate;
            TimerDelegate.BindStatic(&FMCPIntegration::ExportBlueprintsTimerCallback);
            World->GetTimerManager().SetTimer(ExportTimerHandle, TimerDelegate, ExportIntervalSeconds, true);
            
            UE_LOG(LogTemp, Log, TEXT("MCP Integration initialized with automatic blueprint exports every %f seconds"), ExportIntervalSeconds);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No valid world available for setting up export timer"));
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("MCP Integration initialized with server URL: %s"), *ServerURL);
}

bool FMCPIntegration::IsConnected()
{
    // Basic check if we're initialized
    if (!bInitialized || ServerURL.IsEmpty())
    {
        return false;
    }
    
    // Could perform a ping here to verify actual connection
    return true;
}

void FMCPIntegration::SendBlueprintData(const FBlueprintData& BlueprintData, TFunction<void(bool)> Callback)
{
    if (!bInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("MCP Integration not initialized. Call Initialize first."));
        if (Callback)
        {
            Callback(false);
        }
        return;
    }
    
    // Convert blueprint data to JSON for the tool parameter
    FString BlueprintJsonPayload = BlueprintDataToJSON(BlueprintData);
    
    // Create MCP tool request payload
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    RootObject->SetStringField(TEXT("id"), FGuid::NewGuid().ToString());
    RootObject->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
    RootObject->SetStringField(TEXT("method"), TEXT("callTool"));
    
    // Create params object
    TSharedPtr<FJsonObject> ParamsObject = MakeShareable(new FJsonObject);
    ParamsObject->SetStringField(TEXT("name"), TEXT("store_blueprint_data"));
    
    // Create arguments object
    TSharedPtr<FJsonObject> ArgsObject = MakeShareable(new FJsonObject);
    ArgsObject->SetStringField(TEXT("blueprint_json"), BlueprintJsonPayload);
    ParamsObject->SetObjectField(TEXT("arguments"), ArgsObject);
    
    // Add params to root
    RootObject->SetObjectField(TEXT("params"), ParamsObject);
    
    // Convert to string
    FString JsonPayload;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonPayload);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), JsonWriter);
    
    // Create HTTP request
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    // Use the MCP HTTP endpoint
    HttpRequest->SetURL(ServerURL);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // Add API key if provided
    if (!APIKey.IsEmpty())
    {
        HttpRequest->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + APIKey);
    }
    
    HttpRequest->SetContentAsString(JsonPayload);
    
    // Set up response handler
    HttpRequest->OnProcessRequestComplete().BindStatic(&FMCPIntegration::OnResponseReceived, Callback);
    
    // Send the request
    HttpRequest->ProcessRequest();
}

void FMCPIntegration::SendBlueprintsData(const TArray<FBlueprintData>& BlueprintsData, TFunction<void(bool)> Callback)
{
    if (!bInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("MCP Integration not initialized. Call Initialize first."));
        if (Callback)
        {
            Callback(false);
        }
        return;
    }
    
    // Create a JSON array of blueprint data
    TSharedPtr<FJsonObject> BlueprintsObject = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> BlueprintArray;
    
    for (const FBlueprintData& BlueprintData : BlueprintsData)
    {
        // Parse individual blueprint JSON
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(BlueprintDataToJSON(BlueprintData));
        TSharedPtr<FJsonObject> BlueprintObject;
        if (FJsonSerializer::Deserialize(JsonReader, BlueprintObject))
        {
            BlueprintArray.Add(MakeShareable(new FJsonValueObject(BlueprintObject)));
        }
    }
    
    BlueprintsObject->SetArrayField(TEXT("blueprints"), BlueprintArray);
    
    // Convert to string for the tool parameter
    FString BlueprintsJsonPayload;
    TSharedRef<TJsonWriter<>> BlueprintsJsonWriter = TJsonWriterFactory<>::Create(&BlueprintsJsonPayload);
    FJsonSerializer::Serialize(BlueprintsObject.ToSharedRef(), BlueprintsJsonWriter);
    
    // Create MCP tool request payload
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    RootObject->SetStringField(TEXT("id"), FGuid::NewGuid().ToString());
    RootObject->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
    RootObject->SetStringField(TEXT("method"), TEXT("callTool"));
    
    // Create params object
    TSharedPtr<FJsonObject> ParamsObject = MakeShareable(new FJsonObject);
    ParamsObject->SetStringField(TEXT("name"), TEXT("store_blueprints_data"));
    
    // Create arguments object
    TSharedPtr<FJsonObject> ArgsObject = MakeShareable(new FJsonObject);
    ArgsObject->SetStringField(TEXT("blueprints_json"), BlueprintsJsonPayload);
    ParamsObject->SetObjectField(TEXT("arguments"), ArgsObject);
    
    // Add params to root
    RootObject->SetObjectField(TEXT("params"), ParamsObject);
    
    // Convert to string
    FString JsonPayload;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonPayload);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), JsonWriter);
    
    // Create HTTP request
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    // Use the MCP HTTP endpoint
    HttpRequest->SetURL(ServerURL);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // Add API key if provided
    if (!APIKey.IsEmpty())
    {
        HttpRequest->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + APIKey);
    }
    
    HttpRequest->SetContentAsString(JsonPayload);
    
    // Set up response handler
    HttpRequest->OnProcessRequestComplete().BindStatic(&FMCPIntegration::OnResponseReceived, Callback);
    
    // Send the request
    HttpRequest->ProcessRequest();
}

void FMCPIntegration::ProcessQuery(const FString& Query, TFunction<void(const TArray<FBlueprintData>&)> Callback)
{
    if (!bInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("MCP Integration not initialized. Call Initialize first."));
        if (Callback)
        {
            Callback(TArray<FBlueprintData>());
        }
        return;
    }
    
    // Create MCP tool request payload
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    RootObject->SetStringField(TEXT("id"), FGuid::NewGuid().ToString());
    RootObject->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
    RootObject->SetStringField(TEXT("method"), TEXT("callTool"));
    
    // Create params object
    TSharedPtr<FJsonObject> ParamsObject = MakeShareable(new FJsonObject);
    ParamsObject->SetStringField(TEXT("name"), TEXT("search_blueprints"));
    
    // Create arguments object
    TSharedPtr<FJsonObject> ArgsObject = MakeShareable(new FJsonObject);
    ArgsObject->SetStringField(TEXT("query"), Query);
    ParamsObject->SetObjectField(TEXT("arguments"), ArgsObject);
    
    // Add params to root
    RootObject->SetObjectField(TEXT("params"), ParamsObject);
    
    // Convert to string
    FString JsonPayload;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonPayload);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), JsonWriter);
    
    // Create HTTP request
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    // Use the MCP HTTP endpoint
    HttpRequest->SetURL(ServerURL);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // Add API key if provided
    if (!APIKey.IsEmpty())
    {
        HttpRequest->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + APIKey);
    }
    
    HttpRequest->SetContentAsString(JsonPayload);
    
    // Define a specific response handler for queries
    HttpRequest->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
    {
        TArray<FBlueprintData> Results;
        
        if (bSucceeded && HttpResponse.IsValid() && HttpResponse->GetResponseCode() == 200)
        {
            // Parse the JSON response
            FString ResponseContent = HttpResponse->GetContentAsString();
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);
            
            if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject->HasField(TEXT("result")))
            {
                FString ResultStr = JsonObject->GetStringField(TEXT("result"));
                
                // Try to parse the result as JSON
                TSharedPtr<FJsonObject> ResultObject;
                TSharedRef<TJsonReader<>> ResultReader = TJsonReaderFactory<>::Create(ResultStr);
                
                if (FJsonSerializer::Deserialize(ResultReader, ResultObject))
                {
                    // Extract blueprint data from response if it's properly formatted
                    if (ResultObject->HasField(TEXT("blueprints")))
                    {
                        TArray<TSharedPtr<FJsonValue>> BlueprintsJson = ResultObject->GetArrayField(TEXT("blueprints"));
                        
                        for (const TSharedPtr<FJsonValue>& BlueprintValue : BlueprintsJson)
                        {
                            if (BlueprintValue->Type == EJson::Object)
                            {
                                TSharedPtr<FJsonObject> BlueprintObject = BlueprintValue->AsObject();
                                FString BlueprintJson;
                                TSharedRef<TJsonWriter<>> BlueprintWriter = TJsonWriterFactory<>::Create(&BlueprintJson);
                                FJsonSerializer::Serialize(BlueprintObject.ToSharedRef(), BlueprintWriter);
                                
                                // Convert to FBlueprintData
                                FBlueprintData BlueprintData = JSONToBlueprintData(BlueprintJson);
                                Results.Add(BlueprintData);
                            }
                        }
                    }
                }
            }
        }
        
        // Call the callback with results
        if (Callback)
        {
            Callback(Results);
        }
    });
    
    // Send the request
    HttpRequest->ProcessRequest();
}

void FMCPIntegration::Shutdown()
{
    // Reset the integration
    ServerURL = FString();
    APIKey = FString();
    bInitialized = false;
    
    UE_LOG(LogTemp, Log, TEXT("MCP Integration shut down"));
}

void FMCPIntegration::OnResponseReceived(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, TFunction<void(bool)> Callback)
{
    bool bSuccess = false;
    
    if (bSucceeded && HttpResponse.IsValid())
    {
        // Check response code
        if (HttpResponse->GetResponseCode() == 200)
        {
            // Parse the MCP response
            FString ResponseContent = HttpResponse->GetContentAsString();
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);
            
            if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
            {
                // Check for MCP errors
                if (JsonObject->HasField(TEXT("error")))
                {
                    TSharedPtr<FJsonObject> ErrorObject = JsonObject->GetObjectField(TEXT("error"));
                    FString ErrorMessage = ErrorObject->GetStringField(TEXT("message"));
                    UE_LOG(LogTemp, Warning, TEXT("MCP request failed: %s"), *ErrorMessage);
                }
                else if (JsonObject->HasField(TEXT("result")))
                {
                    bSuccess = true;
                    UE_LOG(LogTemp, Log, TEXT("MCP request succeeded: %s"), *ResponseContent);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to parse MCP response: %s"), *ResponseContent);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MCP request failed with response code %d: %s"), 
                HttpResponse->GetResponseCode(), *HttpResponse->GetContentAsString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MCP request failed"));
    }
    
    // Call the callback with result
    if (Callback)
    {
        Callback(bSuccess);
    }
}

FString FMCPIntegration::BlueprintDataToJSON(const FBlueprintData& BlueprintData)
{
    // Create the JSON structure
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    
    // Add basic info
    RootObject->SetStringField(TEXT("name"), BlueprintData.Name);
    RootObject->SetStringField(TEXT("path"), BlueprintData.Path);
    RootObject->SetStringField(TEXT("parentClass"), BlueprintData.ParentClass);
    
    // Add functions
    TArray<TSharedPtr<FJsonValue>> FunctionsArray;
    for (const FBlueprintFunctionData& Function : BlueprintData.Functions)
    {
        TSharedPtr<FJsonObject> FunctionObject = MakeShareable(new FJsonObject);
        FunctionObject->SetStringField(TEXT("name"), Function.Name);
        FunctionObject->SetBoolField(TEXT("isEvent"), Function.IsEvent);
        FunctionObject->SetStringField(TEXT("returnType"), Function.ReturnType);
        FunctionObject->SetStringField(TEXT("description"), Function.Description);
        FunctionObject->SetBoolField(TEXT("isCallable"), Function.IsCallable);
        FunctionObject->SetBoolField(TEXT("isPure"), Function.IsPure);
        
        // Add parameters
        TArray<TSharedPtr<FJsonValue>> ParamsArray;
        for (const FBlueprintParamData& Param : Function.Params)
        {
            TSharedPtr<FJsonObject> ParamObject = MakeShareable(new FJsonObject);
            ParamObject->SetStringField(TEXT("name"), Param.Name);
            ParamObject->SetStringField(TEXT("type"), Param.Type);
            ParamObject->SetBoolField(TEXT("isOutput"), Param.IsOutput);
            ParamObject->SetStringField(TEXT("defaultValue"), Param.DefaultValue);
            
            ParamsArray.Add(MakeShareable(new FJsonValueObject(ParamObject)));
        }
        
        FunctionObject->SetArrayField(TEXT("params"), ParamsArray);
        FunctionsArray.Add(MakeShareable(new FJsonValueObject(FunctionObject)));
    }
    
    RootObject->SetArrayField(TEXT("functions"), FunctionsArray);
    
    // Add variables
    TArray<TSharedPtr<FJsonValue>> VariablesArray;
    for (const FBlueprintVariableData& Variable : BlueprintData.Variables)
    {
        TSharedPtr<FJsonObject> VariableObject = MakeShareable(new FJsonObject);
        VariableObject->SetStringField(TEXT("name"), Variable.Name);
        VariableObject->SetStringField(TEXT("type"), Variable.Type);
        VariableObject->SetStringField(TEXT("defaultValue"), Variable.DefaultValue);
        VariableObject->SetBoolField(TEXT("isExposed"), Variable.IsExposed);
        VariableObject->SetBoolField(TEXT("isReadOnly"), Variable.IsReadOnly);
        VariableObject->SetBoolField(TEXT("isReplicated"), Variable.IsReplicated);
        VariableObject->SetStringField(TEXT("category"), Variable.Category);
        
        VariablesArray.Add(MakeShareable(new FJsonValueObject(VariableObject)));
    }
    
    RootObject->SetArrayField(TEXT("variables"), VariablesArray);
    
    // Add graph data
    if (BlueprintData.Graphs.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> GraphsArray;
        for (const FBlueprintGraphData& Graph : BlueprintData.Graphs)
        {
            TSharedPtr<FJsonObject> GraphObject = MakeShareable(new FJsonObject);
            GraphObject->SetStringField(TEXT("name"), Graph.Name);
            GraphObject->SetStringField(TEXT("graphType"), Graph.GraphType);
            
            // Add graph metadata if available
            if (Graph.Metadata.Num() > 0)
            {
                TSharedPtr<FJsonObject> MetadataObject = MakeShareable(new FJsonObject);
                for (const TPair<FString, FString>& MetadataItem : Graph.Metadata)
                {
                    MetadataObject->SetStringField(MetadataItem.Key, MetadataItem.Value);
                }
                GraphObject->SetObjectField(TEXT("metadata"), MetadataObject);
            }
            
            // Add nodes
            TArray<TSharedPtr<FJsonValue>> NodesArray;
            for (const FBlueprintNodeData& Node : Graph.Nodes)
            {
                TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject);
                NodeObject->SetStringField(TEXT("nodeId"), Node.NodeId);
                NodeObject->SetStringField(TEXT("nodeType"), Node.NodeType);
                NodeObject->SetStringField(TEXT("title"), Node.Title);
                NodeObject->SetNumberField(TEXT("positionX"), Node.PositionX);
                NodeObject->SetNumberField(TEXT("positionY"), Node.PositionY);
                NodeObject->SetStringField(TEXT("comment"), Node.Comment);
                
                // Add input pins
                TArray<TSharedPtr<FJsonValue>> InputPinsArray;
                for (const FBlueprintPinData& Pin : Node.InputPins)
                {
                    TSharedPtr<FJsonObject> PinObject = MakeShareable(new FJsonObject);
                    PinObject->SetStringField(TEXT("pinId"), Pin.PinId);
                    PinObject->SetStringField(TEXT("name"), Pin.Name);
                    PinObject->SetBoolField(TEXT("isExecution"), Pin.IsExecution);
                    PinObject->SetStringField(TEXT("dataType"), Pin.DataType);
                    PinObject->SetBoolField(TEXT("isConnected"), Pin.IsConnected);
                    PinObject->SetStringField(TEXT("defaultValue"), Pin.DefaultValue);
                    PinObject->SetBoolField(TEXT("isInput"), Pin.IsInput);
                    
                    InputPinsArray.Add(MakeShareable(new FJsonValueObject(PinObject)));
                }
                NodeObject->SetArrayField(TEXT("inputPins"), InputPinsArray);
                
                // Add output pins
                TArray<TSharedPtr<FJsonValue>> OutputPinsArray;
                for (const FBlueprintPinData& Pin : Node.OutputPins)
                {
                    TSharedPtr<FJsonObject> PinObject = MakeShareable(new FJsonObject);
                    PinObject->SetStringField(TEXT("pinId"), Pin.PinId);
                    PinObject->SetStringField(TEXT("name"), Pin.Name);
                    PinObject->SetBoolField(TEXT("isExecution"), Pin.IsExecution);
                    PinObject->SetStringField(TEXT("dataType"), Pin.DataType);
                    PinObject->SetBoolField(TEXT("isConnected"), Pin.IsConnected);
                    PinObject->SetStringField(TEXT("defaultValue"), Pin.DefaultValue);
                    PinObject->SetBoolField(TEXT("isInput"), Pin.IsInput);
                    
                    OutputPinsArray.Add(MakeShareable(new FJsonValueObject(PinObject)));
                }
                NodeObject->SetArrayField(TEXT("outputPins"), OutputPinsArray);
                
                // Add node properties
                TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject);
                for (const TPair<FString, FString>& Property : Node.Properties)
                {
                    PropertiesObject->SetStringField(Property.Key, Property.Value);
                }
                NodeObject->SetObjectField(TEXT("properties"), PropertiesObject);
                
                NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObject)));
            }
            GraphObject->SetArrayField(TEXT("nodes"), NodesArray);
            
            // Add connections
            TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
            for (const FBlueprintConnectionData& Connection : Graph.Connections)
            {
                TSharedPtr<FJsonObject> ConnectionObject = MakeShareable(new FJsonObject);
                ConnectionObject->SetStringField(TEXT("sourceNodeId"), Connection.SourceNodeId);
                ConnectionObject->SetStringField(TEXT("sourcePinId"), Connection.SourcePinId);
                ConnectionObject->SetStringField(TEXT("targetNodeId"), Connection.TargetNodeId);
                ConnectionObject->SetStringField(TEXT("targetPinId"), Connection.TargetPinId);
                
                ConnectionsArray.Add(MakeShareable(new FJsonValueObject(ConnectionObject)));
            }
            GraphObject->SetArrayField(TEXT("connections"), ConnectionsArray);
            
            GraphsArray.Add(MakeShareable(new FJsonValueObject(GraphObject)));
        }
        RootObject->SetArrayField(TEXT("graphs"), GraphsArray);
    }
    
    // Add description
    RootObject->SetStringField(TEXT("description"), BlueprintData.Description);
    
    // Add blueprint metadata if available
    if (BlueprintData.Metadata.Num() > 0)
    {
        TSharedPtr<FJsonObject> MetadataObject = MakeShareable(new FJsonObject);
        for (const TPair<FString, FString>& MetadataItem : BlueprintData.Metadata)
        {
            MetadataObject->SetStringField(MetadataItem.Key, MetadataItem.Value);
        }
        RootObject->SetObjectField(TEXT("metadata"), MetadataObject);
    }
    
    // Convert to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), JsonWriter);
    
    return OutputString;
}

FBlueprintData FMCPIntegration::JSONToBlueprintData(const FString& JSON)
{
    FBlueprintData BlueprintData;
    
    // Parse the JSON
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JSON);
    
    if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
    {
        // Basic info
        BlueprintData.Name = JsonObject->GetStringField(TEXT("name"));
        BlueprintData.Path = JsonObject->GetStringField(TEXT("path"));
        
        if (JsonObject->HasField(TEXT("parentClass")))
        {
            BlueprintData.ParentClass = JsonObject->GetStringField(TEXT("parentClass"));
        }
        
        // Functions
        if (JsonObject->HasField(TEXT("functions")))
        {
            TArray<TSharedPtr<FJsonValue>> FunctionsArray = JsonObject->GetArrayField(TEXT("functions"));
            
            for (const TSharedPtr<FJsonValue>& FunctionValue : FunctionsArray)
            {
                if (FunctionValue->Type == EJson::Object)
                {
                    TSharedPtr<FJsonObject> FunctionObject = FunctionValue->AsObject();
                    FBlueprintFunctionData FunctionData;
                    
                    FunctionData.Name = FunctionObject->GetStringField(TEXT("name"));
                    FunctionData.IsEvent = FunctionObject->GetBoolField(TEXT("isEvent"));
                    FunctionData.ReturnType = FunctionObject->GetStringField(TEXT("returnType"));
                    FunctionData.Description = FunctionObject->GetStringField(TEXT("description"));
                    FunctionData.IsCallable = FunctionObject->GetBoolField(TEXT("isCallable"));
                    FunctionData.IsPure = FunctionObject->GetBoolField(TEXT("isPure"));
                    
                    // Parameters
                    if (FunctionObject->HasField(TEXT("params")))
                    {
                        TArray<TSharedPtr<FJsonValue>> ParamsArray = FunctionObject->GetArrayField(TEXT("params"));
                        
                        for (const TSharedPtr<FJsonValue>& ParamValue : ParamsArray)
                        {
                            if (ParamValue->Type == EJson::Object)
                            {
                                TSharedPtr<FJsonObject> ParamObject = ParamValue->AsObject();
                                FBlueprintParamData ParamData;
                                
                                ParamData.Name = ParamObject->GetStringField(TEXT("name"));
                                ParamData.Type = ParamObject->GetStringField(TEXT("type"));
                                ParamData.IsOutput = ParamObject->GetBoolField(TEXT("isOutput"));
                                ParamData.DefaultValue = ParamObject->GetStringField(TEXT("defaultValue"));
                                
                                FunctionData.Params.Add(ParamData);
                            }
                        }
                    }
                    
                    BlueprintData.Functions.Add(FunctionData);
                }
            }
        }
        
        // Variables
        if (JsonObject->HasField(TEXT("variables")))
        {
            TArray<TSharedPtr<FJsonValue>> VariablesArray = JsonObject->GetArrayField(TEXT("variables"));
            
            for (const TSharedPtr<FJsonValue>& VariableValue : VariablesArray)
            {
                if (VariableValue->Type == EJson::Object)
                {
                    TSharedPtr<FJsonObject> VariableObject = VariableValue->AsObject();
                    FBlueprintVariableData VariableData;
                    
                    VariableData.Name = VariableObject->GetStringField(TEXT("name"));
                    VariableData.Type = VariableObject->GetStringField(TEXT("type"));
                    VariableData.DefaultValue = VariableObject->GetStringField(TEXT("defaultValue"));
                    VariableData.IsExposed = VariableObject->GetBoolField(TEXT("isExposed"));
                    VariableData.IsReadOnly = VariableObject->GetBoolField(TEXT("isReadOnly"));
                    VariableData.IsReplicated = VariableObject->GetBoolField(TEXT("isReplicated"));
                    VariableData.Category = VariableObject->GetStringField(TEXT("category"));
                    
                    BlueprintData.Variables.Add(VariableData);
                }
            }
        }
        
        // Description
        if (JsonObject->HasField(TEXT("description")))
        {
            BlueprintData.Description = JsonObject->GetStringField(TEXT("description"));
        }
        
        // Metadata
        if (JsonObject->HasField(TEXT("metadata")))
        {
            TSharedPtr<FJsonObject> MetadataObject = JsonObject->GetObjectField(TEXT("metadata"));
            for (const auto& MetadataItem : MetadataObject->Values)
            {
                if (MetadataItem.Value->Type == EJson::String)
                {
                    BlueprintData.Metadata.Add(MetadataItem.Key, MetadataItem.Value->AsString());
                }
            }
        }
        
        // Graphs
        if (JsonObject->HasField(TEXT("graphs")))
        {
            TArray<TSharedPtr<FJsonValue>> GraphsArray = JsonObject->GetArrayField(TEXT("graphs"));
            
            for (const TSharedPtr<FJsonValue>& GraphValue : GraphsArray)
            {
                if (GraphValue->Type == EJson::Object)
                {
                    TSharedPtr<FJsonObject> GraphObject = GraphValue->AsObject();
                    FBlueprintGraphData GraphData;
                    
                    GraphData.Name = GraphObject->GetStringField(TEXT("name"));
                    GraphData.GraphType = GraphObject->GetStringField(TEXT("graphType"));
                    
                    // Graph metadata
                    if (GraphObject->HasField(TEXT("metadata")))
                    {
                        TSharedPtr<FJsonObject> MetadataObject = GraphObject->GetObjectField(TEXT("metadata"));
                        for (const auto& MetadataItem : MetadataObject->Values)
                        {
                            if (MetadataItem.Value->Type == EJson::String)
                            {
                                GraphData.Metadata.Add(MetadataItem.Key, MetadataItem.Value->AsString());
                            }
                        }
                    }
                    
                    // Process nodes and connections (add when implementing graph loading)
                    
                    BlueprintData.Graphs.Add(GraphData);
                }
            }
        }
    }
    
    return BlueprintData;
}

// Declare static members
FTimerHandle FMCPIntegration::ExportTimerHandle;
float FMCPIntegration::ExportIntervalSeconds = 60.0f; // Default 60 seconds

// Export all blueprints to a JSON file
bool FMCPIntegration::ExportBlueprintsToFile()
{
    // Get all blueprints from the extractor
    TArray<FBlueprintData> AllBlueprints;
    
    // Use the BlueprintDataExtractor to get all blueprints
    AllBlueprints = FBlueprintDataExtractor::GetAllBlueprints();
    
    // Convert to JSON
    FString BlueprintsJson;
    
    // Create a JSON array
    TArray<TSharedPtr<FJsonValue>> JsonArray;
    
    // Add each blueprint to the array
    for (const FBlueprintData& Blueprint : AllBlueprints)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        
        // Basic blueprint info
        JsonObject->SetStringField(TEXT("name"), Blueprint.Name);
        JsonObject->SetStringField(TEXT("path"), Blueprint.Path);
        JsonObject->SetStringField(TEXT("description"), Blueprint.Description);
        JsonObject->SetStringField(TEXT("parent_class"), Blueprint.ParentClass);
        
        // Functions
        TArray<TSharedPtr<FJsonValue>> FunctionsArray;
        for (const FBlueprintFunctionData& Function : Blueprint.Functions)
        {
            TSharedPtr<FJsonObject> FunctionObject = MakeShareable(new FJsonObject);
            FunctionObject->SetStringField(TEXT("name"), Function.Name);
            FunctionObject->SetBoolField(TEXT("is_event"), Function.IsEvent);
            FunctionObject->SetBoolField(TEXT("is_pure"), Function.IsPure);
            FunctionObject->SetBoolField(TEXT("is_callable"), Function.IsCallable);
            FunctionObject->SetStringField(TEXT("return_type"), Function.ReturnType);
            FunctionObject->SetStringField(TEXT("description"), Function.Description);
            
            // Function parameters
            TArray<TSharedPtr<FJsonValue>> ParametersArray;
            for (const FBlueprintParamData& Param : Function.Params)
            {
                TSharedPtr<FJsonObject> ParamObject = MakeShareable(new FJsonObject);
                ParamObject->SetStringField(TEXT("name"), Param.Name);
                ParamObject->SetStringField(TEXT("type"), Param.Type);
                ParamObject->SetBoolField(TEXT("is_output"), Param.IsOutput);
                ParamObject->SetStringField(TEXT("default_value"), Param.DefaultValue);
                
                ParametersArray.Add(MakeShareable(new FJsonValueObject(ParamObject)));
            }
            
            FunctionObject->SetArrayField(TEXT("parameters"), ParametersArray);
            FunctionsArray.Add(MakeShareable(new FJsonValueObject(FunctionObject)));
        }
        
        JsonObject->SetArrayField(TEXT("functions"), FunctionsArray);
        
        // Variables
        TArray<TSharedPtr<FJsonValue>> VariablesArray;
        for (const FBlueprintVariableData& Variable : Blueprint.Variables)
        {
            TSharedPtr<FJsonObject> VariableObject = MakeShareable(new FJsonObject);
            VariableObject->SetStringField(TEXT("name"), Variable.Name);
            VariableObject->SetStringField(TEXT("type"), Variable.Type);
            VariableObject->SetStringField(TEXT("category"), Variable.Category);
            VariableObject->SetStringField(TEXT("default_value"), Variable.DefaultValue);
            VariableObject->SetBoolField(TEXT("is_exposed"), Variable.IsExposed);
            VariableObject->SetBoolField(TEXT("is_read_only"), Variable.IsReadOnly);
            VariableObject->SetBoolField(TEXT("is_replicated"), Variable.IsReplicated);
            
            VariablesArray.Add(MakeShareable(new FJsonValueObject(VariableObject)));
        }
        
        JsonObject->SetArrayField(TEXT("variables"), VariablesArray);
        
        // Add to main array
        JsonArray.Add(MakeShareable(new FJsonValueObject(JsonObject)));
    }
    
    // Convert to string
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BlueprintsJson);
    FJsonSerializer::Serialize(JsonArray, Writer);
    
    // Get the file path
    FString FilePath = GetExportFilePath();
    
    // Ensure directory exists
    FString DirectoryPath = FPaths::GetPath(FilePath);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    if (!PlatformFile.DirectoryExists(*DirectoryPath))
    {
        PlatformFile.CreateDirectoryTree(*DirectoryPath);
    }
    
    // Write to file
    bool bResult = FFileHelper::SaveStringToFile(BlueprintsJson, *FilePath);
    
    if (bResult)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully exported %d blueprints to %s"), AllBlueprints.Num(), *FilePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to export blueprints to %s"), *FilePath);
    }
    
    return bResult;
}

// Get the path to the export file
FString FMCPIntegration::GetExportFilePath()
{
    // Use project saved directory
    FString SavedDir = FPaths::ProjectSavedDir();
    FString ExportPath = FPaths::Combine(SavedDir, TEXT("BlueprintAnalyzer"), TEXT("blueprints_export.json"));
    
    return ExportPath;
}

// Timer callback to export blueprints
void FMCPIntegration::ExportBlueprintsTimerCallback()
{
    ExportBlueprintsToFile();
}

// Set the export interval
void FMCPIntegration::SetExportInterval(float IntervalInSeconds)
{
    if (IntervalInSeconds < 1.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Export interval cannot be less than 1 second, setting to 1 second"));
        IntervalInSeconds = 1.0f;
    }
    
    ExportIntervalSeconds = IntervalInSeconds;
    
    // If we have a timer running, update it
    if (bInitialized && GEngine && GEngine->GetWorld())
    {
        UWorld* World = GEngine->GetWorld();
        if (World)
        {
            World->GetTimerManager().ClearTimer(ExportTimerHandle);
            FTimerDelegate TimerDelegate;
            TimerDelegate.BindStatic(&FMCPIntegration::ExportBlueprintsTimerCallback);
            World->GetTimerManager().SetTimer(ExportTimerHandle, TimerDelegate, ExportIntervalSeconds, true);
            
            UE_LOG(LogTemp, Log, TEXT("Updated blueprint export interval to %f seconds"), ExportIntervalSeconds);
        }
    }
}

// Get the current export interval
float FMCPIntegration::GetExportInterval()
{
    return ExportIntervalSeconds;
}

// This Shutdown method was duplicated - removed duplicate

// Convert multiple blueprints to JSON
FString FMCPIntegration::BlueprintsToJSON(const TArray<FBlueprintData>& Blueprints)
{
    // Create a JSON array to store blueprint JSON objects
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> BlueprintJsonArray;
    
    // Convert each blueprint to a JSON object and add to array
    for (const FBlueprintData& Blueprint : Blueprints)
    {
        // Get JSON string for this blueprint
        FString BlueprintJson = BlueprintDataToJSON(Blueprint);
        
        // Parse the JSON string back to a JSON object
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(BlueprintJson);
        TSharedPtr<FJsonObject> BlueprintJsonObject;
        
        if (FJsonSerializer::Deserialize(JsonReader, BlueprintJsonObject) && BlueprintJsonObject.IsValid())
        {
            // Add JSON object to array
            BlueprintJsonArray.Add(MakeShareable(new FJsonValueObject(BlueprintJsonObject)));
        }
    }
    
    // Add the array to the root object
    RootObject->SetArrayField(TEXT("blueprints"), BlueprintJsonArray);
    
    // Serialize to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), JsonWriter);
    
    return OutputString;
}