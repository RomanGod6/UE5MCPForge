#pragma once

#include "CoreMinimal.h"
#include "BlueprintData.h"

/**
 * A class that handles extraction of blueprint data, including functions, variables, and connections
 */
class BLUEPRINTANALYZER_API FBlueprintDataExtractor
{
public:
	/**
	 * Get all blueprints in the project
	 * @param DetailLevel Level of detail to extract (Basic, Medium, Full)
	 * @return Array of all blueprint data in the project
	 */
	static TArray<FBlueprintData> GetAllBlueprints(EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Basic);
	   
	/**
	 * Get blueprint by path
	 * @param Path Asset path of the blueprint to retrieve
	 * @param DetailLevel Level of detail to extract (Basic, Medium, Full)
	 * @param GraphName Optional name of a specific graph to extract (if empty, extracts all graphs)
	 * @param MaxGraphs Maximum number of graphs to extract (0 = unlimited)
	 * @param MaxNodes Maximum number of nodes per graph to extract (0 = unlimited)
	 * @return Optional blueprint data, empty if not found
	 */
	static TOptional<FBlueprintData> GetBlueprintByPath(const FString& Path,
	                                                   EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Full,
	                                                   const FString& GraphName = TEXT(""),
	                                                   int32 MaxGraphs = 0,
	                                                   int32 MaxNodes = 0);
	   
	/**
	 * Extract detailed data from a blueprint asset
	 * @param Blueprint The blueprint object to extract data from
	 * @param DetailLevel Level of detail to extract (Basic, Medium, Full)
	 * @param GraphName Optional name of a specific graph to extract (if empty, extracts all graphs)
	 * @param MaxGraphs Maximum number of graphs to extract (0 = unlimited)
	 * @param MaxNodes Maximum number of nodes per graph to extract (0 = unlimited)
	 * @return Structured blueprint data
	 */
	static FBlueprintData ExtractBlueprintData(UBlueprint* Blueprint,
	                                          EBlueprintDetailLevel DetailLevel = EBlueprintDetailLevel::Full,
	                                          const FString& GraphName = TEXT(""),
	                                          int32 MaxGraphs = 0,
	                                          int32 MaxNodes = 0);
	
	/**
	 * Get all references to and from a blueprint
	 * @param Path Asset path of the blueprint to find references for
	 * @param bIncludeIndirect Whether to include indirect references (second-level dependencies)
	 * @return Array of reference data structures
	 */
	static TArray<FBlueprintReferenceData> GetBlueprintReferences(const FString& Path, bool bIncludeIndirect = false);
    
private:
	/**
	 * Extract function data from a blueprint
	 * @param Blueprint The blueprint to extract functions from
	 * @return Array of function data
	 */
	static TArray<FBlueprintFunctionData> ExtractFunctions(UBlueprint* Blueprint);
    
	/**
	 * Extract variable data from a blueprint
	 * @param Blueprint The blueprint to extract variables from
	 * @return Array of variable data
	 */
	static TArray<FBlueprintVariableData> ExtractVariables(UBlueprint* Blueprint);
	
	/**
	 * Extract graph data from a blueprint
	 * @param Blueprint The blueprint to extract graph data from
	 * @param GraphName Optional name of a specific graph to extract (if empty, extracts all graphs)
	 * @param MaxGraphs Maximum number of graphs to extract (0 = unlimited)
	 * @param MaxNodes Maximum number of nodes per graph to extract (0 = unlimited)
	 * @return Array of graph data structures containing nodes and connections
	 */
	static TArray<FBlueprintGraphData> ExtractGraphs(UBlueprint* Blueprint,
	                                                const FString& GraphName = TEXT(""),
	                                                int32 MaxGraphs = 0,
	                                                int32 MaxNodes = 0);
												
	/**
	 * Extract only event nodes from a blueprint
	 * @param Blueprint The blueprint to extract event nodes from
	 * @param EventName Optional name of a specific event to filter by (if empty, returns all events)
	 * @return Array of event nodes with their graph information
	 */
	static TArray<FBlueprintNodeData> ExtractEventNodes(UBlueprint* Blueprint,
	                                                  const FString& EventName = TEXT(""));
												
	/**
	 * Get a specific event graph by event name
	 * @param Blueprint The blueprint to extract the event graph from
	 * @param EventName Name of the event to extract (e.g., "BeginPlay", "Construct")
	 * @param MaxNodes Maximum number of nodes to extract (0 = unlimited)
	 * @return Optional graph data for the requested event, empty if not found
	 */
	static TOptional<FBlueprintGraphData> GetEventGraph(UBlueprint* Blueprint,
	                                                  const FString& EventName,
	                                                  int32 MaxNodes = 0);
											  
private:
	/**
	 * Extract reference data from a blueprint
	 * @param Blueprint The blueprint to extract references from
	 * @param bIncludeIndirect Whether to include indirect references (second-level dependencies)
	 * @return Array of reference data structures
	 */
	static TArray<FBlueprintReferenceData> ExtractReferences(UBlueprint* Blueprint, bool bIncludeIndirect = false);
	
	/**
	 * Extract inheritance references from a blueprint
	 * @param Blueprint The blueprint to extract references from
	 * @return Array of inheritance reference data structures
	 */
	static TArray<FBlueprintReferenceData> ExtractInheritanceReferences(UBlueprint* Blueprint);
	
	/**
	 * Extract function call references from a blueprint
	 * @param Blueprint The blueprint to extract references from
	 * @return Array of function call reference data structures
	 */
	static TArray<FBlueprintReferenceData> ExtractFunctionCallReferences(UBlueprint* Blueprint);
	
	/**
	 * Extract variable type references from a blueprint
	 * @param Blueprint The blueprint to extract references from
	 * @return Array of variable type reference data structures
	 */
	static TArray<FBlueprintReferenceData> ExtractVariableTypeReferences(UBlueprint* Blueprint);
	
	/**
	 * Extract direct references from a blueprint (spawns, direct usage)
	 * @param Blueprint The blueprint to extract references from
	 * @return Array of direct reference data structures
	 */
	static TArray<FBlueprintReferenceData> ExtractDirectReferences(UBlueprint* Blueprint);
	
	/**
	 * Build a cache of blueprint references to avoid rescanning
	 * @param Blueprint The blueprint to cache references for
	 */
	static void CacheBlueprintReferences(UBlueprint* Blueprint);
	
	/** Cache of blueprint references for faster lookup */
	static TMap<FString, TArray<FBlueprintReferenceData>> ReferenceCache;
};