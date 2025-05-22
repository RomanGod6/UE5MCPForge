#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FBlueprintAnalyzerModule : public IModuleInterface
{
public:
	/** Delegate handle for delayed initialization */
	FDelegateHandle PostEngineInitHandle;
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
    
	/**
	 * Singleton-like access to this module's interface.
	 * Use this function to access the Blueprint Analyzer module.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FBlueprintAnalyzerModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FBlueprintAnalyzerModule>("BlueprintAnalyzer");
	}
    
	/**
	 * Checks to see if this module is loaded and ready.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("BlueprintAnalyzer");
	}
    
	/**
	 * Start the HTTP server for MCP communication
	 * @param Port The port to listen on
	 * @return Whether the server was successfully started
	 */
	bool StartHttpServer(uint32 Port = 8080);
    
	/**
	 * Stop the HTTP server
	 */
	void StopHttpServer();
};