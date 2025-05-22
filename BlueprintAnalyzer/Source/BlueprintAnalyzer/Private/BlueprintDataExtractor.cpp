#include "../Public/BlueprintDataExtractor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_SpawnActor.h"
#include "K2Node_DynamicCast.h"
#include "UObject/NameTypes.h"
#include "BlueprintAnalyzer/Public/BlueprintData.h"

// Initialize the static reference cache
TMap<FString, TArray<FBlueprintReferenceData>> FBlueprintDataExtractor::ReferenceCache;


TArray<FBlueprintData> FBlueprintDataExtractor::GetAllBlueprints(EBlueprintDetailLevel DetailLevel)
{
    TArray<FBlueprintData> Results;
    
#if WITH_EDITOR
    // Get the asset registry module
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Make sure the asset registry is loaded
    TArray<FString> ContentPaths;
    ContentPaths.Add(TEXT("/Game"));
    AssetRegistry.ScanPathsSynchronous(ContentPaths);
    
    // Query for all blueprint assets
    FARFilter Filter;
    Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
    Filter.bRecursiveClasses = true;
    
    TArray<FAssetData> AssetData;
    AssetRegistry.GetAssets(Filter, AssetData);
    
    // Process each blueprint asset
    for (const FAssetData& Asset : AssetData)
    {
        // For basic detail level, we can extract minimal data from asset data without loading the asset
        if (DetailLevel == EBlueprintDetailLevel::Basic)
        {
            FBlueprintData BlueprintData;
            BlueprintData.Name = Asset.AssetName.ToString();
            BlueprintData.Path = Asset.ObjectPath.ToString();
            
            // Explicitly ensure functions and variables are empty for Basic detail level
            BlueprintData.Functions.Empty();
            BlueprintData.Variables.Empty();
            BlueprintData.Description.Empty();
            
            // Log that we're using basic detail level
            UE_LOG(LogTemp, Log, TEXT("GetAllBlueprints: Using basic detail level for blueprint %s"), *BlueprintData.Name);
            
            // We can try to get parent class from asset data tags
            FString ParentClassName;
            if (Asset.GetTagValue(FName("ParentClass"), ParentClassName))
            {
                // Clean up the parent class string (remove prefix like "Class'/Script/Engine.")
                FString CleanParentClass = ParentClassName;
                CleanParentClass.RemoveFromStart(TEXT("Class'/Script/"));
                int32 QuoteIndex;
                if (CleanParentClass.FindChar('\'', QuoteIndex))
                {
                    CleanParentClass = CleanParentClass.Left(QuoteIndex);
                }
                
                // Remove the module prefix if any
                int32 DotIndex;
                if (CleanParentClass.FindChar('.', DotIndex))
                {
                    BlueprintData.ParentClass = CleanParentClass.Mid(DotIndex + 1);
                }
                else
                {
                    BlueprintData.ParentClass = CleanParentClass;
                }
            }
            
            Results.Add(BlueprintData);
        }
        else
        {
            // For higher detail levels, we need to load the asset
            UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
            if (Blueprint)
            {
                // Extract the data with the specified detail level
                FBlueprintData BlueprintData = ExtractBlueprintData(Blueprint, DetailLevel);
                Results.Add(BlueprintData);
            }
        }
    }
#endif
    
    return Results;
}

TOptional<FBlueprintData> FBlueprintDataExtractor::GetBlueprintByPath(const FString& Path,
                                                                     EBlueprintDetailLevel DetailLevel,
                                                                     const FString& GraphName,
                                                                     int32 MaxGraphs,
                                                                     int32 MaxNodes)
{
#if WITH_EDITOR
    // For basic detail level, try to extract minimal data without loading the blueprint
    if (DetailLevel == EBlueprintDetailLevel::Basic)
    {
        // Try to find asset data without loading the asset
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FName(*Path));
        
        if (AssetData.IsValid())
        {
            // Create basic blueprint data from asset data
            FBlueprintData BlueprintData;
            BlueprintData.Name = AssetData.AssetName.ToString();
            BlueprintData.Path = AssetData.ObjectPath.ToString();
            
            // Explicitly ensure these arrays are empty for basic detail level
            BlueprintData.Functions.Empty();
            BlueprintData.Variables.Empty();
            BlueprintData.Description.Empty();
            
            // Try to get parent class from asset data tags
            FString ParentClassName;
            if (AssetData.GetTagValue(FName("ParentClass"), ParentClassName))
            {
                // Clean up the parent class string
                FString CleanParentClass = ParentClassName;
                CleanParentClass.RemoveFromStart(TEXT("Class'/Script/"));
                int32 QuoteIndex;
                if (CleanParentClass.FindChar('\'', QuoteIndex))
                {
                    CleanParentClass = CleanParentClass.Left(QuoteIndex);
                }
                
                int32 DotIndex;
                if (CleanParentClass.FindChar('.', DotIndex))
                {
                    BlueprintData.ParentClass = CleanParentClass.Mid(DotIndex + 1);
                }
                else
                {
                    BlueprintData.ParentClass = CleanParentClass;
                }
            }
            
            return BlueprintData;
        }
    }
    
    // For medium and full detail levels, load the blueprint
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
    if (Blueprint)
    {
        // Log the detail level for debugging
        UE_LOG(LogTemp, Warning, TEXT("GetBlueprintByPath: Loading blueprint with detail level %d"), static_cast<int32>(DetailLevel));
        return ExtractBlueprintData(Blueprint, DetailLevel, GraphName, MaxGraphs, MaxNodes);
    }
#endif
    
    return TOptional<FBlueprintData>();
}

FBlueprintData FBlueprintDataExtractor::ExtractBlueprintData(UBlueprint* Blueprint,
                                                            EBlueprintDetailLevel DetailLevel,
                                                            const FString& GraphName,
                                                            int32 MaxGraphs,
                                                            int32 MaxNodes)
{
    FBlueprintData Data;
    
    if (!Blueprint)
    {
        return Data;
    }
    
    // Basic info (always included)
    Data.Name = Blueprint->GetName();
    Data.Path = Blueprint->GetPathName();
    Data.ParentClass = Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : FString();
    
#if WITH_EDITOR
    // For Basic detail level, explicitly ensure functions and variables are empty
    if (DetailLevel == EBlueprintDetailLevel::Basic)
    {
        // Explicitly clear these arrays to ensure they're empty for Basic detail level
        Data.Functions.Empty();
        Data.Variables.Empty();
        Data.Description.Empty();
    }
    // For higher detail levels, extract more information
    else if (DetailLevel >= EBlueprintDetailLevel::Medium)
    {
        // Get blueprint description if available
        if (Blueprint->BlueprintDescription.Len() > 0)
        {
            Data.Description = Blueprint->BlueprintDescription;
        }
        
        // Extract functions and variables
        Data.Functions = ExtractFunctions(Blueprint);
        Data.Variables = ExtractVariables(Blueprint);
        
        // For medium detail level, we may want to limit the information
        if (DetailLevel == EBlueprintDetailLevel::Medium)
        {
            // Simplify function data - keep only necessary info
            for (FBlueprintFunctionData& Function : Data.Functions)
            {
                // Clear detailed descriptions for medium level
                Function.Description.Empty();
                
                // Keep only parameter names and types, skip default values
                for (FBlueprintParamData& Param : Function.Params)
                {
                    Param.DefaultValue.Empty();
                }
            }
            
            // Simplify variable data
            for (FBlueprintVariableData& Variable : Data.Variables)
            {
                // Skip default values for medium level
                Variable.DefaultValue.Empty();
                Variable.Category.Empty();
            }
        }
        
        // For Graph detail level, add graph data
        if (DetailLevel == EBlueprintDetailLevel::Graph)
        {
            // Extract graph data with pagination options
            Data.Graphs = ExtractGraphs(Blueprint, GraphName, MaxGraphs, MaxNodes);
            
            // Add metadata about the extraction
            Data.Metadata.Add(TEXT("DetailLevel"), FString::FromInt(static_cast<int32>(DetailLevel)));
            if (!GraphName.IsEmpty())
            {
                Data.Metadata.Add(TEXT("FilteredByGraph"), GraphName);
            }
            if (MaxGraphs > 0)
            {
                Data.Metadata.Add(TEXT("MaxGraphs"), FString::FromInt(MaxGraphs));
            }
            if (MaxNodes > 0)
            {
                Data.Metadata.Add(TEXT("MaxNodes"), FString::FromInt(MaxNodes));
            }
            
            UE_LOG(LogTemp, Log, TEXT("Extracted %d graphs from blueprint %s"), Data.Graphs.Num(), *Data.Name);
        }
        
        // For Events detail level, extract just the event nodes
        if (DetailLevel == EBlueprintDetailLevel::Events)
        {
            // Create a special graph to hold the event nodes
            FBlueprintGraphData EventsGraph;
            EventsGraph.Name = TEXT("Events");
            EventsGraph.GraphType = TEXT("EventsList");
            
            // Extract all event nodes or filter by specific event
            TArray<FBlueprintNodeData> EventNodes = ExtractEventNodes(Blueprint, GraphName);
            EventsGraph.Nodes = EventNodes;
            
            // Add the events graph to the blueprint data
            Data.Graphs.Add(EventsGraph);
            
            // Add metadata
            Data.Metadata.Add(TEXT("DetailLevel"), FString::FromInt(static_cast<int32>(DetailLevel)));
            Data.Metadata.Add(TEXT("EventCount"), FString::FromInt(EventNodes.Num()));
            
            if (!GraphName.IsEmpty())
            {
                Data.Metadata.Add(TEXT("FilteredByEvent"), GraphName);
            }
            
            UE_LOG(LogTemp, Log, TEXT("Extracted %d event nodes from blueprint %s"), EventNodes.Num(), *Data.Name);
        }
        
        // For References detail level, extract references to and from this blueprint
        if (DetailLevel == EBlueprintDetailLevel::References)
        {
            // Extract references
            Data.References = ExtractReferences(Blueprint, true);
            
            // Add metadata
            Data.Metadata.Add(TEXT("DetailLevel"), FString::FromInt(static_cast<int32>(DetailLevel)));
            Data.Metadata.Add(TEXT("ReferenceCount"), FString::FromInt(Data.References.Num()));
            
            UE_LOG(LogTemp, Log, TEXT("Extracted %d references for blueprint %s"), Data.References.Num(), *Data.Name);
        }
        
        // For Full detail level (DetailLevel == EBlueprintDetailLevel::Full)
        // We are already extracting everything available in the ExtractFunctions
        // and ExtractVariables methods, so no additional work needed
    }
#endif
    
    return Data;
}

TArray<FBlueprintFunctionData> FBlueprintDataExtractor::ExtractFunctions(UBlueprint* Blueprint)
{
    TArray<FBlueprintFunctionData> Functions;
    
#if WITH_EDITOR
    if (!Blueprint)
    {
        return Functions;
    }
    
    // Iterate through all function graphs in the blueprint
    for (UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
    {
        FBlueprintFunctionData FunctionData;
        FunctionData.Name = FunctionGraph->GetName();
        
        // Find the function entry node to get parameters
        UK2Node_FunctionEntry* EntryNode = nullptr;
        for (UEdGraphNode* Node : FunctionGraph->Nodes)
        {
            EntryNode = Cast<UK2Node_FunctionEntry>(Node);
            if (EntryNode)
            {
                break;
            }
        }
        
        if (EntryNode)
        {
            // Get input parameters
            for (UEdGraphPin* Pin : EntryNode->Pins)
            {
                if (Pin->Direction == EGPD_Output && Pin->PinName != TEXT("then"))
                {
                    FBlueprintParamData ParamData;
                    ParamData.Name = Pin->PinName.ToString();
                    ParamData.Type = Pin->PinType.PinCategory.ToString();
                    ParamData.IsOutput = false;
                    
                    FunctionData.Params.Add(ParamData);
                }
            }
            
            // Try to determine return type if any
            UEdGraphPin* ReturnPin = EntryNode->FindPin(TEXT("ReturnValue"), EGPD_Input);
            if (ReturnPin)
            {
                FunctionData.ReturnType = ReturnPin->PinType.PinCategory.ToString();
            }
        }
        
        Functions.Add(FunctionData);
    }
    
    // Also check for event graphs
    for (UEdGraph* EventGraph : Blueprint->UbergraphPages)
    {
        // Look for event nodes (like BeginPlay, Tick, etc)
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
            if (EventNode)
            {
                FBlueprintFunctionData EventData;
                EventData.Name = EventNode->EventReference.GetMemberName().ToString();
                EventData.IsEvent = true;
                
                // Add event parameters if any
                for (UEdGraphPin* Pin : EventNode->Pins)
                {
                    if (Pin->Direction == EGPD_Output && Pin->PinName != TEXT("then"))
                    {
                        FBlueprintParamData ParamData;
                        ParamData.Name = Pin->PinName.ToString();
                        ParamData.Type = Pin->PinType.PinCategory.ToString();
                        ParamData.IsOutput = false;
                        
                        EventData.Params.Add(ParamData);
                    }
                }
                
                Functions.Add(EventData);
            }
        }
    }
#endif
    
    return Functions;
}

TArray<FBlueprintVariableData> FBlueprintDataExtractor::ExtractVariables(UBlueprint* Blueprint)
{
    TArray<FBlueprintVariableData> Variables;
    
#if WITH_EDITOR
    if (!Blueprint)
    {
        return Variables;
    }
    
    // Get all variables declared in the blueprint
    for (FBPVariableDescription& VarDesc : Blueprint->NewVariables)
    {
        FBlueprintVariableData VariableData;
        VariableData.Name = VarDesc.VarName.ToString();
        VariableData.Type = UEdGraphSchema_K2::TypeToText(VarDesc.VarType).ToString();
        
        // Get variable properties
        VariableData.IsExposed = (VarDesc.PropertyFlags & CPF_BlueprintVisible) != 0;
        VariableData.IsReadOnly = (VarDesc.PropertyFlags & CPF_BlueprintReadOnly) != 0;
        VariableData.IsReplicated = (VarDesc.PropertyFlags & CPF_Net) != 0;
        
        // Get default value if available
        if (VarDesc.DefaultValue.Len() > 0)
        {
            VariableData.DefaultValue = VarDesc.DefaultValue;
        }
        
        if (!VarDesc.Category.IsEmpty())
        {
            VariableData.Category = VarDesc.Category.ToString();
        }
        
        Variables.Add(VariableData);
    }
#endif
    
    return Variables;
}

TArray<FBlueprintReferenceData> FBlueprintDataExtractor::GetBlueprintReferences(const FString& Path, bool bIncludeIndirect)
{
    TArray<FBlueprintReferenceData> References;
    
#if WITH_EDITOR
    // Check the cache first
    if (ReferenceCache.Contains(Path))
    {
        // Return cached references
        References = ReferenceCache[Path];
        
        // If cached results don't include indirect references and we need them,
        // we'll need to re-extract the references
        bool bNeedToReextract = false;
        if (bIncludeIndirect)
        {
            for (const FBlueprintReferenceData& Reference : References)
            {
                if (Reference.bIsIndirect)
                {
                    // We have indirect references, so we don't need to re-extract
                    bNeedToReextract = false;
                    break;
                }
                bNeedToReextract = true;
            }
        }
        
        if (!bNeedToReextract)
        {
            // Filter the references if needed
            if (!bIncludeIndirect)
            {
                // If we don't need indirect references, filter them out
                TArray<FBlueprintReferenceData> DirectReferences;
                for (const FBlueprintReferenceData& Reference : References)
                {
                    if (!Reference.bIsIndirect)
                    {
                        DirectReferences.Add(Reference);
                    }
                }
                return DirectReferences;
            }
            
            return References;
        }
    }
    
    // Load the blueprint asset
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
    if (Blueprint)
    {
        // Extract references
        References = ExtractReferences(Blueprint, bIncludeIndirect);
        
        // Cache the references
        ReferenceCache.Add(Path, References);
    }
#endif
    
    return References;
}

TArray<FBlueprintReferenceData> FBlueprintDataExtractor::ExtractReferences(UBlueprint* Blueprint, bool bIncludeIndirect)
{
    TArray<FBlueprintReferenceData> References;
    
#if WITH_EDITOR
    if (!Blueprint)
    {
        return References;
    }
    
    // Extract direct inheritance references
    TArray<FBlueprintReferenceData> InheritanceRefs = ExtractInheritanceReferences(Blueprint);
    References.Append(InheritanceRefs);
    
    // Extract function call references
    TArray<FBlueprintReferenceData> FunctionCallRefs = ExtractFunctionCallReferences(Blueprint);
    References.Append(FunctionCallRefs);
    
    // Extract variable type references
    TArray<FBlueprintReferenceData> VariableTypeRefs = ExtractVariableTypeReferences(Blueprint);
    References.Append(VariableTypeRefs);
    
    // Extract direct references (spawns, direct usage)
    TArray<FBlueprintReferenceData> DirectRefs = ExtractDirectReferences(Blueprint);
    References.Append(DirectRefs);
    
    // If we need indirect references, we need to process the reference chain
    if (bIncludeIndirect)
    {
        TArray<FBlueprintReferenceData> IndirectRefs;
        
        // Process each direct reference to find second-level dependencies
        for (const FBlueprintReferenceData& Ref : References)
        {
            // Only process outgoing references to avoid infinite loops
            if (Ref.Direction == EBlueprintReferenceDirection::Outgoing && !Ref.bIsIndirect)
            {
                // Load the referenced blueprint
                UBlueprint* ReferencedBP = LoadObject<UBlueprint>(nullptr, *Ref.BlueprintPath);
                if (ReferencedBP)
                {
                    // Get its outgoing references
                    TArray<FBlueprintReferenceData> SecondLevelRefs = ExtractReferences(ReferencedBP, false);
                    
                    // Add them as indirect references
                    for (FBlueprintReferenceData& SecondRef : SecondLevelRefs)
                    {
                        // Only include outgoing references from the second-level blueprint
                        if (SecondRef.Direction == EBlueprintReferenceDirection::Outgoing)
                        {
                            SecondRef.bIsIndirect = true;
                            SecondRef.ReferenceChain.Add(Ref.BlueprintPath);
                            IndirectRefs.Add(SecondRef);
                        }
                    }
                }
            }
        }
        
        // Add the indirect references
        References.Append(IndirectRefs);
    }
#endif
    
    return References;
}

TArray<FBlueprintReferenceData> FBlueprintDataExtractor::ExtractInheritanceReferences(UBlueprint* Blueprint)
{
    TArray<FBlueprintReferenceData> References;
    
#if WITH_EDITOR
    if (!Blueprint)
    {
        return References;
    }
    
    // Get the asset registry module
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // For the parent class (outgoing reference)
    if (Blueprint->ParentClass)
    {
        UBlueprint* ParentBlueprint = Cast<UBlueprint>(Blueprint->ParentClass->ClassGeneratedBy);
        if (ParentBlueprint)
        {
            FBlueprintReferenceData ParentRef;
            ParentRef.ReferenceType = EBlueprintReferenceType::Inheritance;
            ParentRef.Direction = EBlueprintReferenceDirection::Outgoing;
            ParentRef.BlueprintPath = ParentBlueprint->GetPathName();
            ParentRef.BlueprintName = ParentBlueprint->GetName();
            ParentRef.Context = TEXT("Parent Class");
            ParentRef.bIsIndirect = false;
            
            References.Add(ParentRef);
        }
    }
    
    // For child classes (incoming references)
    TArray<FAssetData> ChildAssets;
    FARFilter Filter;
    Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
    AssetRegistry.GetAssets(Filter, ChildAssets);
    
    for (const FAssetData& ChildAsset : ChildAssets)
    {
        // Check if this blueprint inherits from our blueprint
        FString ParentClassPath;
        if (ChildAsset.GetTagValue(FName("ParentClass"), ParentClassPath))
        {
            // Get the generated class path for our blueprint
            FString OurGeneratedClassPath = Blueprint->GeneratedClass->GetPathName();
            
            // If the parent class path matches our generated class path, this is a child blueprint
            if (ParentClassPath.Contains(OurGeneratedClassPath))
            {
                UBlueprint* ChildBlueprint = Cast<UBlueprint>(ChildAsset.GetAsset());
                if (ChildBlueprint)
                {
                    FBlueprintReferenceData ChildRef;
                    ChildRef.ReferenceType = EBlueprintReferenceType::Inheritance;
                    ChildRef.Direction = EBlueprintReferenceDirection::Incoming;
                    ChildRef.BlueprintPath = ChildBlueprint->GetPathName();
                    ChildRef.BlueprintName = ChildBlueprint->GetName();
                    ChildRef.Context = TEXT("Child Class");
                    ChildRef.bIsIndirect = false;
                    
                    References.Add(ChildRef);
                }
            }
        }
    }
#endif
    
    return References;
}

TArray<FBlueprintReferenceData> FBlueprintDataExtractor::ExtractFunctionCallReferences(UBlueprint* Blueprint)
{
    TArray<FBlueprintReferenceData> References;
    
#if WITH_EDITOR
    if (!Blueprint)
    {
        return References;
    }
    
    // We'll need to examine all graphs to find function calls
    TArray<UEdGraph*> AllGraphs;
    AllGraphs.Append(Blueprint->FunctionGraphs);
    AllGraphs.Append(Blueprint->UbergraphPages);
    
    // Examine each graph
    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph)
        {
            continue;
        }
        
        // Look for function call nodes
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Node);
            if (CallFunctionNode && CallFunctionNode->GetTargetFunction())
            {
                UClass* FunctionOwnerClass = CallFunctionNode->GetTargetFunction()->GetOwnerClass();
                if (FunctionOwnerClass)
                {
                    // See if this function is owned by a blueprint class
                    UBlueprint* TargetBlueprint = Cast<UBlueprint>(FunctionOwnerClass->ClassGeneratedBy);
                    if (TargetBlueprint && TargetBlueprint != Blueprint)
                    {
                        FBlueprintReferenceData FunctionRef;
                        FunctionRef.ReferenceType = EBlueprintReferenceType::FunctionCall;
                        FunctionRef.Direction = EBlueprintReferenceDirection::Outgoing;
                        FunctionRef.BlueprintPath = TargetBlueprint->GetPathName();
                        FunctionRef.BlueprintName = TargetBlueprint->GetName();
                        FunctionRef.Context = FString::Printf(TEXT("Function: %s"), *CallFunctionNode->GetTargetFunction()->GetName());
                        FunctionRef.bIsIndirect = false;
                        
                        // Add additional properties
                        FunctionRef.Properties.Add(TEXT("FunctionName"), CallFunctionNode->GetTargetFunction()->GetName());
                        FunctionRef.Properties.Add(TEXT("SourceGraph"), Graph->GetName());
                        
                        References.Add(FunctionRef);
                    }
                }
            }
        }
    }
#endif
    
    return References;
}

TArray<FBlueprintReferenceData> FBlueprintDataExtractor::ExtractVariableTypeReferences(UBlueprint* Blueprint)
{
    TArray<FBlueprintReferenceData> References;
    
#if WITH_EDITOR
    if (!Blueprint)
    {
        return References;
    }
    
    // Examine variable types
    for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
    {
        UClass* VariableClass = nullptr;
        
        // Handle different variable types
        if (VarDesc.VarType.PinCategory == UEdGraphSchema_K2::PC_Object ||
            VarDesc.VarType.PinCategory == UEdGraphSchema_K2::PC_Class ||
            VarDesc.VarType.PinCategory == UEdGraphSchema_K2::PC_Interface)
        {
            // For object references, get the class
            VariableClass = Cast<UClass>(VarDesc.VarType.PinSubCategoryObject.Get());
        }
        
        // Check if this is a blueprint-generated class
        if (VariableClass && VariableClass->ClassGeneratedBy)
        {
            UBlueprint* VariableBlueprint = Cast<UBlueprint>(VariableClass->ClassGeneratedBy);
            if (VariableBlueprint && VariableBlueprint != Blueprint)
            {
                FBlueprintReferenceData VariableRef;
                VariableRef.ReferenceType = EBlueprintReferenceType::VariableType;
                VariableRef.Direction = EBlueprintReferenceDirection::Outgoing;
                VariableRef.BlueprintPath = VariableBlueprint->GetPathName();
                VariableRef.BlueprintName = VariableBlueprint->GetName();
                VariableRef.Context = FString::Printf(TEXT("Variable: %s"), *VarDesc.VarName.ToString());
                VariableRef.bIsIndirect = false;
                
                // Add additional properties
                VariableRef.Properties.Add(TEXT("VariableName"), VarDesc.VarName.ToString());
                
                References.Add(VariableRef);
            }
        }
    }
#endif
    
    return References;
}

TArray<FBlueprintReferenceData> FBlueprintDataExtractor::ExtractDirectReferences(UBlueprint* Blueprint)
{
    TArray<FBlueprintReferenceData> References;
    
#if WITH_EDITOR
    if (!Blueprint)
    {
        return References;
    }
    
    // We'll need to examine all graphs to find direct references like spawns
    TArray<UEdGraph*> AllGraphs;
    AllGraphs.Append(Blueprint->FunctionGraphs);
    AllGraphs.Append(Blueprint->UbergraphPages);
    
    // Examine each graph
    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph)
        {
            continue;
        }
        
        // Look for nodes that might contain direct references
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            // Check for SpawnActor nodes
            UK2Node_SpawnActor* SpawnNode = Cast<UK2Node_SpawnActor>(Node);
            if (SpawnNode)
            {
                // Since GetClassToSpawn is private, we need to use the public API
                UEdGraphPin* BlueprintPin = SpawnNode->GetBlueprintPin();
                if (BlueprintPin && BlueprintPin->LinkedTo.Num() > 0)
                {
                    // If the pin is connected, we can't statically determine the class
                    // So we'll just skip it for now
                    continue;
                }
                else if (BlueprintPin && !BlueprintPin->DefaultObject.IsNull())
                {
                    // Try to get the class from the default object on the pin
                    UClass* SpawnClass = Cast<UClass>(BlueprintPin->DefaultObject);
                    if (SpawnClass && SpawnClass->ClassGeneratedBy)
                    {
                        UBlueprint* SpawnedBlueprint = Cast<UBlueprint>(SpawnClass->ClassGeneratedBy);
                        if (SpawnedBlueprint && SpawnedBlueprint != Blueprint)
                        {
                            FBlueprintReferenceData SpawnRef;
                            SpawnRef.ReferenceType = EBlueprintReferenceType::DirectReference;
                            SpawnRef.Direction = EBlueprintReferenceDirection::Outgoing;
                            SpawnRef.BlueprintPath = SpawnedBlueprint->GetPathName();
                            SpawnRef.BlueprintName = SpawnedBlueprint->GetName();
                            SpawnRef.Context = TEXT("Spawn Actor");
                            SpawnRef.bIsIndirect = false;
                            
                            // Add additional properties
                            SpawnRef.Properties.Add(TEXT("NodeType"), TEXT("SpawnActor"));
                            SpawnRef.Properties.Add(TEXT("SourceGraph"), Graph->GetName());
                            
                            References.Add(SpawnRef);
                        }
                    }
                }
            }
            
            // Check for DynamicCast nodes
            UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node);
            if (CastNode && CastNode->TargetType)
            {
                UBlueprint* CastBlueprint = Cast<UBlueprint>(CastNode->TargetType->ClassGeneratedBy);
                if (CastBlueprint && CastBlueprint != Blueprint)
                {
                    FBlueprintReferenceData CastRef;
                    CastRef.ReferenceType = EBlueprintReferenceType::DirectReference;
                    CastRef.Direction = EBlueprintReferenceDirection::Outgoing;
                    CastRef.BlueprintPath = CastBlueprint->GetPathName();
                    CastRef.BlueprintName = CastBlueprint->GetName();
                    CastRef.Context = TEXT("Dynamic Cast");
                    CastRef.bIsIndirect = false;
                    
                    // Add additional properties
                    CastRef.Properties.Add(TEXT("NodeType"), TEXT("DynamicCast"));
                    CastRef.Properties.Add(TEXT("SourceGraph"), Graph->GetName());
                    
                    References.Add(CastRef);
                }
            }
        }
    }
#endif
    
    return References;
}

void FBlueprintDataExtractor::CacheBlueprintReferences(UBlueprint* Blueprint)
{
#if WITH_EDITOR
    if (!Blueprint)
    {
        return;
    }
    
    // Get the path
    FString Path = Blueprint->GetPathName();
    
    // Extract references with indirect references
    TArray<FBlueprintReferenceData> References = ExtractReferences(Blueprint, true);
    
    // Cache them
    ReferenceCache.Add(Path, References);
#endif
}

TArray<FBlueprintGraphData> FBlueprintDataExtractor::ExtractGraphs(UBlueprint* Blueprint,
                                                                  const FString& GraphName,
                                                                  int32 MaxGraphs,
                                                                  int32 MaxNodes)
{
    TArray<FBlueprintGraphData> Graphs;
    
#if WITH_EDITOR
    if (!Blueprint)
    {
        return Graphs;
    }
    
    // Track the total number of graphs for pagination metadata
    int32 TotalGraphs = Blueprint->FunctionGraphs.Num() + Blueprint->UbergraphPages.Num();
    int32 GraphsAdded = 0;
    
    // Process each function graph
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        // If we've hit the maximum number of graphs, stop processing
        if (MaxGraphs > 0 && GraphsAdded >= MaxGraphs)
        {
            break;
        }
        
        if (!Graph)
        {
            continue;
        }
        
        // If a specific graph name is requested, skip others
        if (!GraphName.IsEmpty() && !Graph->GetName().Equals(GraphName, ESearchCase::IgnoreCase))
        {
            continue;
        }
        
        FBlueprintGraphData GraphData;
        GraphData.Name = Graph->GetName();
        GraphData.GraphType = TEXT("Function");
        
        // Add metadata for pagination
        GraphData.Metadata.Add(TEXT("TotalGraphs"), FString::FromInt(TotalGraphs));
        if (MaxGraphs > 0)
        {
            GraphData.Metadata.Add(TEXT("MaxGraphs"), FString::FromInt(MaxGraphs));
        }
        
        // Track the total number of nodes for pagination metadata
        int32 TotalNodes = Graph->Nodes.Num();
        int32 NodesAdded = 0;
        
        // Add metadata about nodes
        GraphData.Metadata.Add(TEXT("TotalNodes"), FString::FromInt(TotalNodes));
        if (MaxNodes > 0)
        {
            GraphData.Metadata.Add(TEXT("MaxNodes"), FString::FromInt(MaxNodes));
        }
        
        // Extract nodes in the graph, respecting the MaxNodes limit
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            // If we've hit the maximum number of nodes, stop processing
            if (MaxNodes > 0 && NodesAdded >= MaxNodes)
            {
                // Add metadata about pagination
                GraphData.Metadata.Add(TEXT("Paginated"), TEXT("true"));
                GraphData.Metadata.Add(TEXT("NodesShown"), FString::FromInt(NodesAdded));
                break;
            }
            
            if (!Node)
            {
                continue;
            }
            
            FBlueprintNodeData NodeData;
            NodeData.NodeId = FString::Printf(TEXT("%lld"), (int64)Node);
            NodeData.NodeType = Node->GetClass()->GetName();
            NodeData.Title = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
            NodeData.PositionX = Node->NodePosX;
            NodeData.PositionY = Node->NodePosY;
            
            // Increment node counter
            NodesAdded++;
            
            // Extract node comment if available
            if (Node->NodeComment.Len() > 0)
            {
                NodeData.Comment = Node->NodeComment;
            }
            
            // Extract pins
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (!Pin)
                {
                    continue;
                }
                
                FBlueprintPinData PinData;
                PinData.PinId = FString::Printf(TEXT("%lld"), (int64)Pin);
                PinData.Name = Pin->PinName.ToString();
                PinData.IsExecution = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
                PinData.DataType = Pin->PinType.PinCategory.ToString();
                PinData.IsConnected = (Pin->LinkedTo.Num() > 0);
                PinData.DefaultValue = Pin->DefaultValue;
                
                // Determine pin direction
                if (Pin->Direction == EEdGraphPinDirection::EGPD_Input)
                {
                    PinData.IsInput = true;
                    NodeData.InputPins.Add(PinData);
                }
                else
                {
                    PinData.IsInput = false;
                    NodeData.OutputPins.Add(PinData);
                }
                
                // Add connections
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (!LinkedPin || !LinkedPin->GetOwningNode())
                    {
                        continue;
                    }
                    
                    FBlueprintConnectionData Connection;
                    
                    // Set source and target based on pin direction
                    if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
                    {
                        // This pin is the source (output)
                        Connection.SourceNodeId = NodeData.NodeId;
                        Connection.SourcePinId = PinData.PinId;
                        Connection.TargetNodeId = FString::Printf(TEXT("%lld"), (int64)LinkedPin->GetOwningNode());
                        Connection.TargetPinId = FString::Printf(TEXT("%lld"), (int64)LinkedPin);
                    }
                    else
                    {
                        // This pin is the target (input)
                        Connection.SourceNodeId = FString::Printf(TEXT("%lld"), (int64)LinkedPin->GetOwningNode());
                        Connection.SourcePinId = FString::Printf(TEXT("%lld"), (int64)LinkedPin);
                        Connection.TargetNodeId = NodeData.NodeId;
                        Connection.TargetPinId = PinData.PinId;
                    }
                    
                    // Add connection to the graph
                    GraphData.Connections.Add(Connection);
                }
            }
            
            // Extract additional node properties (specific to node types)
            if (NodeData.NodeType.Contains(TEXT("K2Node_CallFunction")))
            {
                UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(Node);
                if (FunctionNode && FunctionNode->FunctionReference.GetMemberName() != NAME_None)
                {
                    NodeData.Properties.Add(TEXT("FunctionName"), FunctionNode->FunctionReference.GetMemberName().ToString());
                }
            }
            else if (NodeData.NodeType.Contains(TEXT("K2Node_VariableGet")))
            {
                UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(Node);
                if (VarGetNode && VarGetNode->VariableReference.GetMemberName() != NAME_None)
                {
                    NodeData.Properties.Add(TEXT("VariableName"), VarGetNode->VariableReference.GetMemberName().ToString());
                }
            }
            else if (NodeData.NodeType.Contains(TEXT("K2Node_VariableSet")))
            {
                UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node);
                if (VarSetNode && VarSetNode->VariableReference.GetMemberName() != NAME_None)
                {
                    NodeData.Properties.Add(TEXT("VariableName"), VarSetNode->VariableReference.GetMemberName().ToString());
                }
            }
            
            // Add node to graph
            GraphData.Nodes.Add(NodeData);
        }
        
        // Add graph to results
        Graphs.Add(GraphData);
    }
    
    // Also include the event graph if it exists
    if (Blueprint->UbergraphPages.Num() > 0)
    {
        UEdGraph* EventGraph = Blueprint->UbergraphPages[0];
        if (EventGraph)
        {
            FBlueprintGraphData GraphData;
            GraphData.Name = EventGraph->GetName();
            GraphData.GraphType = TEXT("EventGraph");
            
            // Extract nodes and connections using the same method as above
            for (UEdGraphNode* Node : EventGraph->Nodes)
            {
                if (!Node)
                {
                    continue;
                }
                
                FBlueprintNodeData NodeData;
                NodeData.NodeId = FString::Printf(TEXT("%lld"), (int64)Node);
                NodeData.NodeType = Node->GetClass()->GetName();
                NodeData.Title = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                NodeData.PositionX = Node->NodePosX;
                NodeData.PositionY = Node->NodePosY;
                
                if (Node->NodeComment.Len() > 0)
                {
                    NodeData.Comment = Node->NodeComment;
                }
                
                // Process pins and connections (similar code to above)
                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (!Pin)
                    {
                        continue;
                    }
                    
                    FBlueprintPinData PinData;
                    PinData.PinId = FString::Printf(TEXT("%lld"), (int64)Pin);
                    PinData.Name = Pin->PinName.ToString();
                    PinData.IsExecution = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
                    PinData.DataType = Pin->PinType.PinCategory.ToString();
                    PinData.IsConnected = (Pin->LinkedTo.Num() > 0);
                    PinData.DefaultValue = Pin->DefaultValue;
                    
                    if (Pin->Direction == EEdGraphPinDirection::EGPD_Input)
                    {
                        PinData.IsInput = true;
                        NodeData.InputPins.Add(PinData);
                    }
                    else
                    {
                        PinData.IsInput = false;
                        NodeData.OutputPins.Add(PinData);
                    }
                    
                    // Process connections
                    for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                    {
                        if (!LinkedPin || !LinkedPin->GetOwningNode())
                        {
                            continue;
                        }
                        
                        FBlueprintConnectionData Connection;
                        
                        if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
                        {
                            Connection.SourceNodeId = NodeData.NodeId;
                            Connection.SourcePinId = PinData.PinId;
                            Connection.TargetNodeId = FString::Printf(TEXT("%lld"), (int64)LinkedPin->GetOwningNode());
                            Connection.TargetPinId = FString::Printf(TEXT("%lld"), (int64)LinkedPin);
                        }
                        else
                        {
                            Connection.SourceNodeId = FString::Printf(TEXT("%lld"), (int64)LinkedPin->GetOwningNode());
                            Connection.SourcePinId = FString::Printf(TEXT("%lld"), (int64)LinkedPin);
                            Connection.TargetNodeId = NodeData.NodeId;
                            Connection.TargetPinId = PinData.PinId;
                        }
                        
                        GraphData.Connections.Add(Connection);
                    }
                }
                
                // Extract additional node properties
                if (NodeData.NodeType.Contains(TEXT("K2Node_CallFunction")))
                {
                    UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(Node);
                    if (FunctionNode && FunctionNode->FunctionReference.GetMemberName() != NAME_None)
                    {
                        NodeData.Properties.Add(TEXT("FunctionName"), FunctionNode->FunctionReference.GetMemberName().ToString());
                    }
                }
                else if (NodeData.NodeType.Contains(TEXT("K2Node_VariableGet")))
                {
                    UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(Node);
                    if (VarGetNode && VarGetNode->VariableReference.GetMemberName() != NAME_None)
                    {
                        NodeData.Properties.Add(TEXT("VariableName"), VarGetNode->VariableReference.GetMemberName().ToString());
                    }
                }
                else if (NodeData.NodeType.Contains(TEXT("K2Node_VariableSet")))
                {
                    UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node);
                    if (VarSetNode && VarSetNode->VariableReference.GetMemberName() != NAME_None)
                    {
                        NodeData.Properties.Add(TEXT("VariableName"), VarSetNode->VariableReference.GetMemberName().ToString());
                    }
                }
                else if (NodeData.NodeType.Contains(TEXT("K2Node_Event")))
                {
                    UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
                    if (EventNode && EventNode->EventReference.GetMemberName() != NAME_None)
                    {
                        NodeData.Properties.Add(TEXT("EventName"), EventNode->EventReference.GetMemberName().ToString());
                    }
                }
                
                GraphData.Nodes.Add(NodeData);
            }
            
            Graphs.Add(GraphData);
        }
    }
#endif
    
    return Graphs;
}

TArray<FBlueprintNodeData> FBlueprintDataExtractor::ExtractEventNodes(UBlueprint* Blueprint, const FString& EventName)
{
    TArray<FBlueprintNodeData> EventNodes;
    
#if WITH_EDITOR
    if (!Blueprint)
    {
        return EventNodes;
    }
    
    // Go through all event graphs (Ubergraph pages)
    for (UEdGraph* EventGraph : Blueprint->UbergraphPages)
    {
        if (!EventGraph)
        {
            continue;
        }
        
        // Look for event nodes
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
            if (EventNode)
            {
                // Get the event name
                FString NodeEventName = EventNode->EventReference.GetMemberName().ToString();
                
                // If an event name filter is provided, check if this node matches
                if (!EventName.IsEmpty() && !NodeEventName.Equals(EventName, ESearchCase::IgnoreCase))
                {
                    continue;
                }
                
                // Create node data structure
                FBlueprintNodeData NodeData;
                NodeData.NodeId = FString::Printf(TEXT("%lld"), (int64)Node);
                NodeData.NodeType = Node->GetClass()->GetName();
                NodeData.Title = NodeEventName;
                NodeData.PositionX = Node->NodePosX;
                NodeData.PositionY = Node->NodePosY;
                
                // Add the graph name as a property
                NodeData.Properties.Add(TEXT("GraphName"), EventGraph->GetName());
                
                // Add more details about the event
                UClass* MemberScopeClass = Blueprint->GeneratedClass;
                if (MemberScopeClass)
                {
                    UClass* EventScopeClass = Cast<UClass>(EventNode->EventReference.GetMemberScope(MemberScopeClass));
                    if (EventScopeClass)
                    {
                        NodeData.Properties.Add(TEXT("EventScope"), EventScopeClass->GetName());
                    }
                }
                
                // Add comment if available
                if (Node->NodeComment.Len() > 0)
                {
                    NodeData.Comment = Node->NodeComment;
                }
                
                // Extract pins
                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (!Pin)
                    {
                        continue;
                    }
                    
                    if (Pin->Direction == EGPD_Input)
                    {
                        FBlueprintPinData PinData;
                        PinData.PinId = FString::Printf(TEXT("%lld"), (int64)Pin);
                        PinData.Name = Pin->PinName.ToString();
                        PinData.IsExecution = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
                        PinData.DataType = Pin->PinType.PinCategory.ToString();
                        PinData.IsInput = true;
                        PinData.IsConnected = (Pin->LinkedTo.Num() > 0);
                        
                        NodeData.InputPins.Add(PinData);
                    }
                    else
                    {
                        FBlueprintPinData PinData;
                        PinData.PinId = FString::Printf(TEXT("%lld"), (int64)Pin);
                        PinData.Name = Pin->PinName.ToString();
                        PinData.IsExecution = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
                        PinData.DataType = Pin->PinType.PinCategory.ToString();
                        PinData.IsInput = false;
                        PinData.IsConnected = (Pin->LinkedTo.Num() > 0);
                        
                        NodeData.OutputPins.Add(PinData);
                    }
                }
                
                EventNodes.Add(NodeData);
            }
        }
    }
#endif
    
    return EventNodes;
}

TOptional<FBlueprintGraphData> FBlueprintDataExtractor::GetEventGraph(UBlueprint* Blueprint, const FString& EventName, int32 MaxNodes)
{
#if WITH_EDITOR
    if (!Blueprint || EventName.IsEmpty())
    {
        return TOptional<FBlueprintGraphData>();
    }
    
    // Go through all event graphs (Ubergraph pages)
    for (UEdGraph* EventGraph : Blueprint->UbergraphPages)
    {
        if (!EventGraph)
        {
            continue;
        }
        
        // Try to find the requested event node
        UK2Node_Event* TargetEventNode = nullptr;
        
        // Look for event nodes matching the requested name
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
            if (EventNode)
            {
                FString NodeEventName = EventNode->EventReference.GetMemberName().ToString();
                
                // Check if this is the event we're looking for
                if (NodeEventName.Equals(EventName, ESearchCase::IgnoreCase))
                {
                    TargetEventNode = EventNode;
                    break;
                }
            }
        }
        
        // If we found the target event node
        if (TargetEventNode)
        {
            // Create a graph data structure for this event
            FBlueprintGraphData GraphData;
            GraphData.Name = EventName;
            GraphData.GraphType = TEXT("Event");
            
            // Add metadata for the graph
            GraphData.Metadata.Add(TEXT("GraphName"), EventGraph->GetName());
            
            // Set of nodes we've already processed
            TSet<UEdGraphNode*> ProcessedNodes;
            
            // Queue of nodes to process (starting with our event node)
            TArray<UEdGraphNode*> NodesToProcess;
            NodesToProcess.Add(TargetEventNode);
            
            // Track number of nodes added
            int32 NodesAdded = 0;
            
            // Process nodes from the queue
            while (NodesToProcess.Num() > 0)
            {
                // Check if we've hit the maximum number of nodes
                if (MaxNodes > 0 && NodesAdded >= MaxNodes)
                {
                    // Add metadata about pagination
                    GraphData.Metadata.Add(TEXT("Paginated"), TEXT("true"));
                    GraphData.Metadata.Add(TEXT("NodesShown"), FString::FromInt(NodesAdded));
                    GraphData.Metadata.Add(TEXT("MaxNodes"), FString::FromInt(MaxNodes));
                    break;
                }
                
                // Get the next node to process
                UEdGraphNode* CurrentNode = NodesToProcess[0];
                NodesToProcess.RemoveAt(0);
                
                // Skip if already processed
                if (ProcessedNodes.Contains(CurrentNode))
                {
                    continue;
                }
                
                // Mark as processed
                ProcessedNodes.Add(CurrentNode);
                
                // Create node data
                FBlueprintNodeData NodeData;
                NodeData.NodeId = FString::Printf(TEXT("%lld"), (int64)CurrentNode);
                NodeData.NodeType = CurrentNode->GetClass()->GetName();
                NodeData.Title = CurrentNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                NodeData.PositionX = CurrentNode->NodePosX;
                NodeData.PositionY = CurrentNode->NodePosY;
                
                // Add comment if available
                if (CurrentNode->NodeComment.Len() > 0)
                {
                    NodeData.Comment = CurrentNode->NodeComment;
                }
                
                // Process pins and connections
                for (UEdGraphPin* Pin : CurrentNode->Pins)
                {
                    if (!Pin)
                    {
                        continue;
                    }
                    
                    FBlueprintPinData PinData;
                    PinData.PinId = FString::Printf(TEXT("%lld"), (int64)Pin);
                    PinData.Name = Pin->PinName.ToString();
                    PinData.IsExecution = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
                    PinData.DataType = Pin->PinType.PinCategory.ToString();
                    PinData.IsInput = (Pin->Direction == EGPD_Input);
                    PinData.IsConnected = (Pin->LinkedTo.Num() > 0);
                    
                    // Add default value if any
                    if (Pin->DefaultValue.Len() > 0)
                    {
                        PinData.DefaultValue = Pin->DefaultValue;
                    }
                    
                    // Add pin to node
                    if (Pin->Direction == EGPD_Input)
                    {
                        NodeData.InputPins.Add(PinData);
                    }
                    else
                    {
                        NodeData.OutputPins.Add(PinData);
                    }
                    
                    // Add connections and queue connected nodes (only for execution flow)
                    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                    {
                        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                        {
                            if (LinkedPin && LinkedPin->GetOwningNode())
                            {
                                // Add connection data
                                FBlueprintConnectionData ConnectionData;
                                ConnectionData.SourceNodeId = NodeData.NodeId;
                                ConnectionData.SourcePinId = PinData.PinId;
                                ConnectionData.TargetNodeId = FString::Printf(TEXT("%lld"), (int64)LinkedPin->GetOwningNode());
                                ConnectionData.TargetPinId = FString::Printf(TEXT("%lld"), (int64)LinkedPin);
                                
                                GraphData.Connections.Add(ConnectionData);
                                
                                // Queue the connected node for processing
                                NodesToProcess.Add(LinkedPin->GetOwningNode());
                            }
                        }
                    }
                }
                
                // Add node to graph
                GraphData.Nodes.Add(NodeData);
                NodesAdded++;
            }
            
            // Return the constructed graph data
            return GraphData;
        }
    }
#endif
    
    // Event not found
    return TOptional<FBlueprintGraphData>();
}