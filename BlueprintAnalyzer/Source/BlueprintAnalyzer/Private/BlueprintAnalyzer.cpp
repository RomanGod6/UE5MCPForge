#include "BlueprintAnalyzer/Public/BlueprintAnalyzer.h"
#include "BlueprintAnalyzer/Public/MCPHttpServer.h"
#include "BlueprintAnalyzer/Public/MCPIntegration.h"
#include "Misc/MessageDialog.h"
#include "Misc/CoreDelegates.h"

#define LOCTEXT_NAMESPACE "FBlueprintAnalyzerModule"

void FBlueprintAnalyzerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory
	UE_LOG(LogTemp, Log, TEXT("Blueprint Analyzer Module has been loaded"));
    
	// Initialize MCP integration with a default URL (can be set via settings later)
	FMCPIntegration::Initialize(TEXT("http://localhost:3000"), TEXT(""));
	
	// Set up automatic blueprint export every 30 seconds
	FMCPIntegration::SetExportInterval(30.0f);
	
	// Register a delegate to be called after engine initialization
	PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddLambda([this]()
	{
		// Perform initial export after engine initialization
		UE_LOG(LogTemp, Log, TEXT("Performing delayed blueprint export after engine init"));
		bool bExportSuccess = FMCPIntegration::ExportBlueprintsToFile();
		
		if (bExportSuccess)
		{
			UE_LOG(LogTemp, Log, TEXT("Successfully exported initial blueprints data to file"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to export initial blueprints data"));
		}
		
		// Start the HTTP server on port 8080
		StartHttpServer(8080);
	});
}

void FBlueprintAnalyzerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module
	UE_LOG(LogTemp, Log, TEXT("Blueprint Analyzer Module has been unloaded"));
    
	// Remove the delegate if it's still bound
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
	}
	
	// Stop the HTTP server
	StopHttpServer();
    
	// Shutdown MCP integration
	FMCPIntegration::Shutdown();
}

bool FBlueprintAnalyzerModule::StartHttpServer(uint32 Port)
{
	bool bSuccess = FMCPHttpServer::Initialize(Port);
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("Blueprint Analyzer HTTP Server started on port %d"), Port);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to start Blueprint Analyzer HTTP Server on port %d"), Port);
	}
	
	return bSuccess;
}

void FBlueprintAnalyzerModule::StopHttpServer()
{
	FMCPHttpServer::Shutdown();
	UE_LOG(LogTemp, Log, TEXT("Blueprint Analyzer HTTP Server stopped"));
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FBlueprintAnalyzerModule, BlueprintAnalyzer)