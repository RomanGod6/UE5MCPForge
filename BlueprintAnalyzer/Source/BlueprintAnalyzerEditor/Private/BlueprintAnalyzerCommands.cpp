#include "BlueprintAnalyzerEditor/Public/BlueprintAnalyzerCommands.h"

#define LOCTEXT_NAMESPACE "FBlueprintAnalyzerModule"

void FBlueprintAnalyzerCommands::RegisterCommands()
{
	UI_COMMAND(
		ListBlueprints, 
		"List Blueprints", 
		"Lists all blueprints in the project", 
		EUserInterfaceActionType::Button, 
		FInputChord()
	);
    
	UI_COMMAND(
		AnalyzeCurrentBlueprint, 
		"Analyze Current Blueprint", 
		"Analyzes the currently open blueprint", 
		EUserInterfaceActionType::Button, 
		FInputChord()
	);
    
	UI_COMMAND(
		SendToMCP, 
		"Send to MCP", 
		"Sends blueprint data to the MCP server", 
		EUserInterfaceActionType::Button, 
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE