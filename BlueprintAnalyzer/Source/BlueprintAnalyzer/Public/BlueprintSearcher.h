#pragma once

#include "CoreMinimal.h"
#include "BlueprintData.h"

/**
 * A class that handles searching for blueprints with various criteria
 */
class BLUEPRINTANALYZER_API FBlueprintSearcher
{
public:
	/**
	 * Search blueprints by name
	 * @param NameQuery The name or partial name to search for
	 * @param DetailLevel Level of detail to extract (Basic, Medium, Full)
	 * @return Array of matching blueprint data
	 */
	static TArray<FBlueprintData> SearchByName(const FString& NameQuery, EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Basic);
	   
	/**
	 * Search blueprints by parent class
	 * @param ParentClassName The parent class name to search for
	 * @param DetailLevel Level of detail to extract (Basic, Medium, Full)
	 * @return Array of matching blueprint data
	 */
	static TArray<FBlueprintData> SearchByParentClass(const FString& ParentClassName, EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Basic);
	   
	/**
	 * Search blueprints by function signature
	 * @param FunctionName The function name to search for
	 * @param ParamTypes Optional array of parameter types to match
	 * @param DetailLevel Level of detail to extract (Basic, Medium, Full)
	 * @return Array of matching blueprint data
	 */
	static TArray<FBlueprintData> SearchByFunction(const FString& FunctionName, const TArray<FString>& ParamTypes = TArray<FString>(), EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Basic);
	   
	/**
	 * Search blueprints by variable
	 * @param VariableName The variable name to search for
	 * @param VariableType Optional variable type to match
	 * @param DetailLevel Level of detail to extract (Basic, Medium, Full)
	 * @return Array of matching blueprint data
	 */
	static TArray<FBlueprintData> SearchByVariable(const FString& VariableName, const FString& VariableType = FString(), EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Basic);
	   
	/**
	 * Search blueprints using custom parameters
	 * @param SearchParams Map of search parameters and their values
	 * @param DetailLevel Level of detail to extract (Basic, Medium, Full)
	 * @return Array of matching blueprint data
	 */
	static TArray<FBlueprintData> SearchWithParameters(const TMap<FString, FString>& SearchParams, EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Basic);
};