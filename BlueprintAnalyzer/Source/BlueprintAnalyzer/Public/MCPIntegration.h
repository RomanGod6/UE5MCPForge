#pragma once

#include "CoreMinimal.h"
#include "BlueprintData.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

/**
 * A class that handles integration with an MCP (Master Control Program) server
 * for AI communication about blueprint data
 */
class BLUEPRINTANALYZER_API FMCPIntegration
{
public:
    /**
     * Initialize the MCP integration with a server URL
     * @param InServerURL The URL of the MCP server
     * @param InAPIKey Optional API key for authentication
     */
    static void Initialize(const FString& InServerURL, const FString& InAPIKey = FString());
    
    /**
     * Check if the MCP integration is initialized and connected
     * @return True if connected to an MCP server
     */
    static bool IsConnected();
    
    /**
     * Send blueprint data to the MCP server
     * @param BlueprintData The blueprint data to send
     * @param Callback Optional callback for when the operation completes
     */
    static void SendBlueprintData(const FBlueprintData& BlueprintData, TFunction<void(bool)> Callback = nullptr);
    
    /**
     * Send multiple blueprint data entries to the MCP server
     * @param BlueprintsData Array of blueprint data to send
     * @param Callback Optional callback for when the operation completes
     */
    static void SendBlueprintsData(const TArray<FBlueprintData>& BlueprintsData, TFunction<void(bool)> Callback = nullptr);
    
    /**
     * Process a query from the MCP server
     * @param Query The query string
     * @param Callback Callback with the query results
     */
    static void ProcessQuery(const FString& Query, TFunction<void(const TArray<FBlueprintData>&)> Callback);
    
    /**
     * Shutdown the MCP integration
     */
    static void Shutdown();

    /**
 * Convert blueprint data to JSON
 * @param BlueprintData The blueprint data to convert
 * @return JSON string representing the blueprint data
 */
    static FString BlueprintDataToJSON(const FBlueprintData& BlueprintData);
/**
     * Convert multiple blueprints to JSON
     * @param Blueprints Array of blueprint data to convert
     * @return JSON string representing the blueprints
     */
    static FString BlueprintsToJSON(const TArray<FBlueprintData>& Blueprints);
    
    /**
     * Export all blueprints to a JSON file
/** Timer handle for periodic exports */
    static FTimerHandle ExportTimerHandle;
    
    /** Export interval in seconds */
    static float ExportIntervalSeconds;
    
    /**
     * Timer callback to export blueprints
     */
    static void ExportBlueprintsTimerCallback();
    
    /**
     * Export all blueprints to a JSON file
     * @return True if export was successful
     */
    static bool ExportBlueprintsToFile();
    
    /**
     * Get the path to the exported blueprints JSON file
     * @return Full path to the export file
     */
    static FString GetExportFilePath();
    
    /**
     * Set the interval for automatic blueprint exports
     * @param IntervalInSeconds Interval in seconds (minimum 1)
     */
    static void SetExportInterval(float IntervalInSeconds);
    
    /**
     * Get the current export interval in seconds
     * @return Current export interval
     */
    static float GetExportInterval();
    
private:
    /** Server URL for the MCP */
    static FString ServerURL;
    
    /** API key for authentication */
    static FString APIKey;
    
    /** Whether the integration is initialized */
    static bool bInitialized;
    

    
    /**
     * Parse JSON to blueprint data
     * @param JSON The JSON string to parse
     * @return Blueprint data parsed from JSON
     */
    static FBlueprintData JSONToBlueprintData(const FString& JSON);
    
    /**
     * Handle HTTP response from the MCP server
     * @param HttpRequest The HTTP request
     * @param HttpResponse The HTTP response
     * @param bSucceeded Whether the request succeeded
     * @param Callback Callback to invoke with result
     */
    static void OnResponseReceived(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, TFunction<void(bool)> Callback);
};