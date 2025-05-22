#pragma once

#include "CoreMinimal.h"

/**
 * Documentation helper for Blueprint Analyzer API
 * Contains structured information about API endpoints and parameters
 */
class BLUEPRINTANALYZER_API FMCPDoc
{
public:
    /**
     * Get documentation about detail levels
     * @return JSON string with detail level documentation
     */
    static FString GetDetailLevelDocs();
    
    /**
     * Get full API documentation
     * @return JSON string with all API endpoint documentation
     */
    static FString GetFullApiDocs();
};