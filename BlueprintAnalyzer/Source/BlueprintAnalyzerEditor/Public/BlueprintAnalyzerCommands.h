
#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

class FBlueprintAnalyzerCommands : public TCommands<FBlueprintAnalyzerCommands>
{
public:
	FBlueprintAnalyzerCommands()
		: TCommands<FBlueprintAnalyzerCommands>(
			TEXT("BlueprintAnalyzer"),  // Context name for fast lookup
			NSLOCTEXT("Contexts", "BlueprintAnalyzer", "Blueprint Analyzer Plugin"),  // Localized context name for displaying
			NAME_None,  // Parent
			FAppStyle::GetAppStyleSetName()  // Icon Style Set
			)
	{
	}

	// TCommand<> interface
	virtual void RegisterCommands() override;
	// End of TCommand<> interface

	/** Command to list all blueprints */
	TSharedPtr<FUICommandInfo> ListBlueprints;
    
	/** Command to analyze current blueprint */
	TSharedPtr<FUICommandInfo> AnalyzeCurrentBlueprint;
    
	/** Command to send blueprint data to MCP */
	TSharedPtr<FUICommandInfo> SendToMCP;
};