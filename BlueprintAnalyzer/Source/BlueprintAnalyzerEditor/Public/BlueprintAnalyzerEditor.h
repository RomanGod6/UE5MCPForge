#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "BlueprintAnalyzerCommands.h"

class FBlueprintAnalyzerEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
    
	/** Menu extension methods */
	void RegisterMenus();
    
	/** Command handlers */
	void ListBlueprintsHandler();
	void AnalyzeCurrentBlueprintHandler();
	void SendToMCPHandler();
    
private:
	TSharedPtr<FUICommandList> PluginCommands;
	TSharedPtr<FExtender> ToolbarExtender;
    
	/** Adds toolbar extensions */
	void AddToolbarExtension(FToolBarBuilder& Builder);
};