#include "BlueprintAnalyzer/Public/BlueprintSearcher.h"
#include "BlueprintAnalyzer/Public/BlueprintData.h"
#include "BlueprintAnalyzer/Public/BlueprintDataExtractor.h"

TArray<FBlueprintData> FBlueprintSearcher::SearchByName(const FString& NameQuery, EBlueprintDetailLevel DetailLevel)
{
    TArray<FBlueprintData> Results;
    
    // Get all blueprints first with basic detail level for efficiency
    TArray<FBlueprintData> AllBlueprints = FBlueprintDataExtractor::GetAllBlueprints(EBlueprintDetailLevel::Basic);
    
    // Filter by name
    for (const FBlueprintData& Blueprint : AllBlueprints)
    {
        if (Blueprint.Name.Contains(NameQuery, ESearchCase::IgnoreCase))
        {
            Results.Add(Blueprint);
        }
    }
    
    // If a higher detail level is requested and we have results, get more detailed information
    if (DetailLevel != EBlueprintDetailLevel::Basic && Results.Num() > 0)
    {
        for (int32 i = 0; i < Results.Num(); ++i)
        {
            // Use the path to fetch more detailed information
            TOptional<FBlueprintData> DetailedData = FBlueprintDataExtractor::GetBlueprintByPath(Results[i].Path, DetailLevel);
            if (DetailedData.IsSet())
            {
                Results[i] = DetailedData.GetValue();
            }
        }
    }
    
    return Results;
}

TArray<FBlueprintData> FBlueprintSearcher::SearchByParentClass(const FString& ParentClassName, EBlueprintDetailLevel DetailLevel)
{
    TArray<FBlueprintData> Results;
    
    // Get all blueprints first with basic detail level for efficiency
    TArray<FBlueprintData> AllBlueprints = FBlueprintDataExtractor::GetAllBlueprints(EBlueprintDetailLevel::Basic);
    
    // Filter by parent class
    for (const FBlueprintData& Blueprint : AllBlueprints)
    {
        if (Blueprint.ParentClass.Contains(ParentClassName, ESearchCase::IgnoreCase))
        {
            Results.Add(Blueprint);
        }
    }
    
    // If a higher detail level is requested and we have results, get more detailed information
    if (DetailLevel != EBlueprintDetailLevel::Basic && Results.Num() > 0)
    {
        for (int32 i = 0; i < Results.Num(); ++i)
        {
            // Use the path to fetch more detailed information
            TOptional<FBlueprintData> DetailedData = FBlueprintDataExtractor::GetBlueprintByPath(Results[i].Path, DetailLevel);
            if (DetailedData.IsSet())
            {
                Results[i] = DetailedData.GetValue();
            }
        }
    }
    
    return Results;
}

TArray<FBlueprintData> FBlueprintSearcher::SearchByFunction(const FString& FunctionName, const TArray<FString>& ParamTypes, EBlueprintDetailLevel DetailLevel)
{
    TArray<FBlueprintData> Results;
    
    // For function searches, we need at least medium detail level to access function information
    // We'll use medium detail even for basic searches, then enhance if full detail is requested
    EBlueprintDetailLevel SearchLevel = EBlueprintDetailLevel::Medium;
    TArray<FBlueprintData> AllBlueprints = FBlueprintDataExtractor::GetAllBlueprints(SearchLevel);
    
    // Filter by function name and optionally parameter types
    for (const FBlueprintData& Blueprint : AllBlueprints)
    {
        bool bFoundMatch = false;
        
        for (const FBlueprintFunctionData& Function : Blueprint.Functions)
        {
            if (Function.Name.Contains(FunctionName, ESearchCase::IgnoreCase))
            {
                // If parameter types are specified, check those too
                if (ParamTypes.Num() > 0)
                {
                    if (Function.Params.Num() >= ParamTypes.Num())
                    {
                        bool bAllParamsMatch = true;
                        
                        // Check each parameter type
                        for (int32 i = 0; i < ParamTypes.Num(); ++i)
                        {
                            if (i >= Function.Params.Num() || 
                                !Function.Params[i].Type.Contains(ParamTypes[i], ESearchCase::IgnoreCase))
                            {
                                bAllParamsMatch = false;
                                break;
                            }
                        }
                        
                        if (bAllParamsMatch)
                        {
                            bFoundMatch = true;
                            break;
                        }
                    }
                }
                else
                {
                    // If no parameter types specified, just match by name
                    bFoundMatch = true;
                    break;
                }
            }
        }
        
        if (bFoundMatch)
        {
            Results.Add(Blueprint);
        }
    }
    
    // If full detail level is requested and we have results, get more detailed information
    if (DetailLevel == EBlueprintDetailLevel::Full && Results.Num() > 0)
    {
        for (int32 i = 0; i < Results.Num(); ++i)
        {
            // Use the path to fetch more detailed information
            TOptional<FBlueprintData> DetailedData = FBlueprintDataExtractor::GetBlueprintByPath(Results[i].Path, DetailLevel);
            if (DetailedData.IsSet())
            {
                Results[i] = DetailedData.GetValue();
            }
        }
    }
    
    return Results;
}

TArray<FBlueprintData> FBlueprintSearcher::SearchByVariable(const FString& VariableName, const FString& VariableType, EBlueprintDetailLevel DetailLevel)
{
    TArray<FBlueprintData> Results;
    
    // For variable searches, we need at least medium detail level to access variable information
    // We'll use medium detail even for basic searches, then enhance if full detail is requested
    EBlueprintDetailLevel SearchLevel = EBlueprintDetailLevel::Medium;
    TArray<FBlueprintData> AllBlueprints = FBlueprintDataExtractor::GetAllBlueprints(SearchLevel);
    
    // Filter by variable name and optionally type
    for (const FBlueprintData& Blueprint : AllBlueprints)
    {
        bool bFoundMatch = false;
        
        for (const FBlueprintVariableData& Variable : Blueprint.Variables)
        {
            if (Variable.Name.Contains(VariableName, ESearchCase::IgnoreCase))
            {
                // If variable type is specified, check that too
                if (!VariableType.IsEmpty())
                {
                    if (Variable.Type.Contains(VariableType, ESearchCase::IgnoreCase))
                    {
                        bFoundMatch = true;
                        break;
                    }
                }
                else
                {
                    // If no type specified, just match by name
                    bFoundMatch = true;
                    break;
                }
            }
        }
        
        if (bFoundMatch)
        {
            Results.Add(Blueprint);
        }
    }
    
    // If full detail level is requested and we have results, get more detailed information
    if (DetailLevel == EBlueprintDetailLevel::Full && Results.Num() > 0)
    {
        for (int32 i = 0; i < Results.Num(); ++i)
        {
            // Use the path to fetch more detailed information
            TOptional<FBlueprintData> DetailedData = FBlueprintDataExtractor::GetBlueprintByPath(Results[i].Path, DetailLevel);
            if (DetailedData.IsSet())
            {
                Results[i] = DetailedData.GetValue();
            }
        }
    }
    
    return Results;
}

TArray<FBlueprintData> FBlueprintSearcher::SearchWithParameters(const TMap<FString, FString>& SearchParams, EBlueprintDetailLevel DetailLevel)
{
    TArray<FBlueprintData> Results;
    
    // For parameter searches, use medium detail level if we might need to search functions or variables
    // otherwise basic level is more efficient
    EBlueprintDetailLevel SearchLevel = EBlueprintDetailLevel::Basic;
    
    // Check if we need function or variable data for the search
    for (const TPair<FString, FString>& Param : SearchParams)
    {
        const FString& Key = Param.Key;
        if (Key.Equals(TEXT("Function"), ESearchCase::IgnoreCase) ||
            Key.Equals(TEXT("Variable"), ESearchCase::IgnoreCase))
        {
            SearchLevel = EBlueprintDetailLevel::Medium;
            break;
        }
    }
    
    // Get blueprints with appropriate detail level
    TArray<FBlueprintData> AllBlueprints = FBlueprintDataExtractor::GetAllBlueprints(SearchLevel);
    Results = AllBlueprints;
    
    // Apply filters based on provided parameters
    for (const TPair<FString, FString>& Param : SearchParams)
    {
        const FString& Key = Param.Key;
        const FString& Value = Param.Value;
        
        // Filter current results
        TArray<FBlueprintData> FilteredResults;
        
        if (Key.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
        {
            // Filter by blueprint name
            for (const FBlueprintData& Blueprint : Results)
            {
                if (Blueprint.Name.Contains(Value, ESearchCase::IgnoreCase))
                {
                    FilteredResults.Add(Blueprint);
                }
            }
        }
        else if (Key.Equals(TEXT("ParentClass"), ESearchCase::IgnoreCase))
        {
            // Filter by parent class
            for (const FBlueprintData& Blueprint : Results)
            {
                if (Blueprint.ParentClass.Contains(Value, ESearchCase::IgnoreCase))
                {
                    FilteredResults.Add(Blueprint);
                }
            }
        }
        else if (Key.Equals(TEXT("Function"), ESearchCase::IgnoreCase))
        {
            // Filter by function name
            for (const FBlueprintData& Blueprint : Results)
            {
                bool bFoundMatch = false;
                
                for (const FBlueprintFunctionData& Function : Blueprint.Functions)
                {
                    if (Function.Name.Contains(Value, ESearchCase::IgnoreCase))
                    {
                        bFoundMatch = true;
                        break;
                    }
                }
                
                if (bFoundMatch)
                {
                    FilteredResults.Add(Blueprint);
                }
            }
        }
        else if (Key.Equals(TEXT("Variable"), ESearchCase::IgnoreCase))
        {
            // Filter by variable name
            for (const FBlueprintData& Blueprint : Results)
            {
                bool bFoundMatch = false;
                
                for (const FBlueprintVariableData& Variable : Blueprint.Variables)
                {
                    if (Variable.Name.Contains(Value, ESearchCase::IgnoreCase))
                    {
                        bFoundMatch = true;
                        break;
                    }
                }
                
                if (bFoundMatch)
                {
                    FilteredResults.Add(Blueprint);
                }
            }
        }
        else if (Key.Equals(TEXT("Path"), ESearchCase::IgnoreCase))
        {
            // Filter by asset path
            for (const FBlueprintData& Blueprint : Results)
            {
                if (Blueprint.Path.Contains(Value, ESearchCase::IgnoreCase))
                {
                    FilteredResults.Add(Blueprint);
                }
            }
        }
        
        // Update results for next filter
        Results = FilteredResults;
    }
    
    // If a higher detail level is requested than what we used for searching,
    // and we have results, get more detailed information
    if (DetailLevel > SearchLevel && Results.Num() > 0)
    {
        for (int32 i = 0; i < Results.Num(); ++i)
        {
            // Use the path to fetch more detailed information
            TOptional<FBlueprintData> DetailedData = FBlueprintDataExtractor::GetBlueprintByPath(Results[i].Path, DetailLevel);
            if (DetailedData.IsSet())
            {
                Results[i] = DetailedData.GetValue();
            }
        }
    }
    
    return Results;
}