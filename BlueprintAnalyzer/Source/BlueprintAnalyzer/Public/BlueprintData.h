#pragma once

#include "CoreMinimal.h"

/**
 * Enum defining the detail level for blueprint data extraction
 */
enum class EBlueprintDetailLevel : uint8
{
    /** Level 0: Basic information only (name, path, parent class) */
    Basic = 0,
    
    /** Level 1: Medium detail (basic + functions and variables overview) */
    Medium = 1,
    
    /** Level 2: Full detail (all available information except graph data) */
    Full = 2,
    
    /** Level 3: Graph detail (includes visual graph data for nodes and connections) */
    Graph = 3,
    
    /** Level 4: Events only (lists only the event nodes used in the blueprint) */
    Events = 4,
    
    /** Level 5: References (lists blueprints referencing or referenced by this blueprint) */
    References = 5
};

/**
 * Enum defining the type of blueprint reference
 */
enum class EBlueprintReferenceType : uint8
{
    /** Blueprint inherits from target blueprint */
    Inheritance = 0,
    
    /** Blueprint calls functions from target blueprint */
    FunctionCall = 1,
    
    /** Blueprint uses target as a variable type */
    VariableType = 2,
    
    /** Blueprint spawns or directly references the target */
    DirectReference = 3,
    
    /** Blueprint is used by something that uses the target (second-level dependency) */
    IndirectReference = 4,
    
    /** Blueprint is part of an event chain that triggers target */
    EventChain = 5,
    
    /** Blueprint is part of a data flow path to/from target */
    DataFlow = 6
};

/**
 * Enum defining the direction of blueprint reference
 */
enum class EBlueprintReferenceDirection : uint8
{
    /** Reference is from this blueprint to others (outgoing) */
    Outgoing = 0,
    
    /** Reference is from other blueprints to this one (incoming) */
    Incoming = 1
};

/**
 * Data structure representing a blueprint parameter
 */
struct BLUEPRINTANALYZER_API FBlueprintParamData
{
    /** Name of the parameter */
    FString Name;
    
    /** Type of the parameter as a string */
    FString Type;
    
    /** Whether this parameter is an output parameter */
    bool IsOutput = false;
    
    /** Default value of the parameter if any */
    FString DefaultValue;
};

/**
 * Data structure representing a blueprint function
 */
struct BLUEPRINTANALYZER_API FBlueprintFunctionData
{
    /** Name of the function */
    FString Name;
    
    /** Whether this is an event (like BeginPlay, Tick) rather than a function */
    bool IsEvent = false;
    
    /** Array of parameters */
    TArray<FBlueprintParamData> Params;
    
    /** Return type of the function as a string */
    FString ReturnType;
    
    /** Function metadata or comments */
    FString Description;
    
    /** Whether this function is exposed to other blueprints */
    bool IsCallable = false;
    
    /** Whether this function is pure (no state changes) */
    bool IsPure = false;
};

/**
 * Data structure representing a blueprint variable
 */
struct BLUEPRINTANALYZER_API FBlueprintVariableData
{
    /** Name of the variable */
    FString Name;
    
    /** Type of the variable as a string */
    FString Type;
    
    /** Default value of the variable if any */
    FString DefaultValue;
    
    /** Whether this variable is exposed to the editor */
    bool IsExposed = false;
    
    /** Whether this variable is read-only in blueprints */
    bool IsReadOnly = false;
    
    /** Whether this variable is replicated in multiplayer */
    bool IsReplicated = false;
    
    /** Category of the variable in the editor */
    FString Category;
};

/**
 * Data structure representing a pin on a blueprint node
 */
struct BLUEPRINTANALYZER_API FBlueprintPinData
{
    /** Unique identifier for the pin */
    FString PinId;
    
    /** Name of the pin */
    FString Name;
    
    /** Whether this is an execution pin or data pin */
    bool IsExecution = false;
    
    /** Data type for data pins */
    FString DataType;
    
    /** Whether this pin is connected to another pin */
    bool IsConnected = false;
    
    /** Default value for the pin if not connected */
    FString DefaultValue;
    
    /** Direction of the pin (input or output) */
    bool IsInput = true;
};

/**
 * Data structure representing a node in a blueprint graph
 */
struct BLUEPRINTANALYZER_API FBlueprintNodeData
{
    /** Unique identifier for the node */
    FString NodeId;
    
    /** Type/class of the node (K2Node_CallFunction, K2Node_IfThenElse, etc.) */
    FString NodeType;
    
    /** Title or display name of the node */
    FString Title;
    
    /** X position in the graph */
    int32 PositionX = 0;
    
    /** Y position in the graph */
    int32 PositionY = 0;
    
    /** Comment or description for the node */
    FString Comment;
    
    /** Input pins on this node */
    TArray<FBlueprintPinData> InputPins;
    
    /** Output pins on this node */
    TArray<FBlueprintPinData> OutputPins;
    
    /** Additional node properties as key-value pairs */
    TMap<FString, FString> Properties;
};

/**
 * Data structure representing a connection between pins
 */
struct BLUEPRINTANALYZER_API FBlueprintConnectionData
{
    /** Source node identifier */
    FString SourceNodeId;
    
    /** Source pin identifier */
    FString SourcePinId;
    
    /** Target node identifier */
    FString TargetNodeId;
    
    /** Target pin identifier */
    FString TargetPinId;
};

/**
 * Data structure representing a complete graph in a blueprint
 */
struct BLUEPRINTANALYZER_API FBlueprintGraphData
{
    /** Name of the graph (usually the function name) */
    FString Name;
    
    /** Type of the graph (ubergraph, function, macro, etc.) */
    FString GraphType;
    
    /** Array of nodes in the graph */
    TArray<FBlueprintNodeData> Nodes;
    
    /** Array of connections between nodes */
    TArray<FBlueprintConnectionData> Connections;
    
    /** Additional metadata such as pagination information */
    TMap<FString, FString> Metadata;
};

/**
 * Data structure representing a blueprint reference
 */
struct BLUEPRINTANALYZER_API FBlueprintReferenceData
{
    /** Type of reference (inheritance, function call, variable, etc.) */
    EBlueprintReferenceType ReferenceType;
    
    /** Direction of reference (incoming or outgoing) */
    EBlueprintReferenceDirection Direction;
    
    /** Path to the blueprint being referenced or referencing */
    FString BlueprintPath;
    
    /** Name of the blueprint being referenced or referencing */
    FString BlueprintName;
    
    /** Additional context about the reference (which function, variable, etc.) */
    FString Context;
    
    /** Whether this is a direct or indirect reference */
    bool bIsIndirect = false;
    
    /** Reference chain for indirect references (paths through which the reference occurs) */
    TArray<FString> ReferenceChain;
    
    /** Additional metadata or properties for this reference */
    TMap<FString, FString> Properties;
};

/**
 * Data structure representing a complete blueprint
 */
struct BLUEPRINTANALYZER_API FBlueprintData
{
    /** Name of the blueprint */
    FString Name;
    
    /** Asset path of the blueprint */
    FString Path;
    
    /** Parent class name */
    FString ParentClass;
    
    /** Array of functions defined in the blueprint */
    TArray<FBlueprintFunctionData> Functions;
    
    /** Array of variables defined in the blueprint */
    TArray<FBlueprintVariableData> Variables;
    
    /** Array of graphs in the blueprint (only included in Full+ detail level) */
    TArray<FBlueprintGraphData> Graphs;
    
    /** Array of references to and from this blueprint (only included in References detail level) */
    TArray<FBlueprintReferenceData> References;
    
    /** Blueprint description or comments */
    FString Description;
    
    /** Additional metadata such as pagination information */
    TMap<FString, FString> Metadata;
};