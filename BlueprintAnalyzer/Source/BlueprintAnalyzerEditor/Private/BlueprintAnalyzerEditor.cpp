#include "../Public/BlueprintAnalyzerEditor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ToolMenus.h"
#include "BlueprintAnalyzer/Public/BlueprintDataExtractor.h"
#include "BlueprintAnalyzer/Public/MCPIntegration.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "Selection.h" 
#include "Editor/EditorEngine.h"
#include "Engine/Engine.h"
#include "Editor/UnrealEd/Public/Editor.h"   
#include "Engine/Blueprint.h"
#include "EditorViewportClient.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "FBlueprintAnalyzerEditorModule"

void FBlueprintAnalyzerEditorModule::StartupModule()
{
    // Register commands, keybindings, and menu extensions
    FBlueprintAnalyzerCommands::Register();
    
    PluginCommands = MakeShareable(new FUICommandList);
    
    PluginCommands->MapAction(
        FBlueprintAnalyzerCommands::Get().ListBlueprints,
        FExecuteAction::CreateRaw(this, &FBlueprintAnalyzerEditorModule::ListBlueprintsHandler),
        FCanExecuteAction());
    
    PluginCommands->MapAction(
        FBlueprintAnalyzerCommands::Get().AnalyzeCurrentBlueprint,
        FExecuteAction::CreateRaw(this, &FBlueprintAnalyzerEditorModule::AnalyzeCurrentBlueprintHandler),
        FCanExecuteAction());
    
    PluginCommands->MapAction(
        FBlueprintAnalyzerCommands::Get().SendToMCP,
        FExecuteAction::CreateRaw(this, &FBlueprintAnalyzerEditorModule::SendToMCPHandler),
        FCanExecuteAction());
    
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBlueprintAnalyzerEditorModule::RegisterMenus));
    
    // Initialize MCP integration with a default URL (can be set via settings later)
    FMCPIntegration::Initialize(TEXT("http://localhost:3000"), TEXT(""));
}

void FBlueprintAnalyzerEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    
    FBlueprintAnalyzerCommands::Unregister();
    
    // Shutdown MCP integration
    FMCPIntegration::Shutdown();
}


void FBlueprintAnalyzerEditorModule::RegisterMenus()
{
    // Add a toolbar button
    FToolMenuOwnerScoped OwnerScoped(this);
    
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
    FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("BlueprintAnalyzer");
    
    // Add icons for buttons
    Section.AddMenuEntryWithCommandList(
        FBlueprintAnalyzerCommands::Get().ListBlueprints,
        PluginCommands
    ).Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Blueprint");
    
    Section.AddMenuEntryWithCommandList(
        FBlueprintAnalyzerCommands::Get().AnalyzeCurrentBlueprint,
        PluginCommands
    ).Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.BlueprintDefaults");
    
    Section.AddMenuEntryWithCommandList(
        FBlueprintAnalyzerCommands::Get().SendToMCP,
        PluginCommands
    ).Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Profiler.EventGraph.ExpandHotPath");
}


void FBlueprintAnalyzerEditorModule::ListBlueprintsHandler()
{
    // Get all blueprints in the project
    TArray<FBlueprintData> Blueprints = FBlueprintDataExtractor::GetAllBlueprints();
    
    FString ResultMessage = FString::Printf(TEXT("Found %d blueprints in the project:\n\n"), Blueprints.Num());
    
    // Add each blueprint to the message
    for (int32 i = 0; i < Blueprints.Num(); ++i)
    {
        ResultMessage += FString::Printf(TEXT("%d. %s (%s)\n"), i + 1, *Blueprints[i].Name, *Blueprints[i].ParentClass);
    }
    
    // Show the results
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
}

void FBlueprintAnalyzerEditorModule::AnalyzeCurrentBlueprintHandler()
{
#if WITH_EDITOR
    // Get the currently active blueprint editor
    UBlueprint* CurrentBlueprint = nullptr;
    
    // First try to get selected objects
    if (GEditor && GEditor->GetSelectedObjects())
    {
        TArray<UObject*> SelectedObjects;
        GEditor->GetSelectedObjects()->GetSelectedObjects(SelectedObjects);
        
        // Look for a blueprint in the selected objects
        for (UObject* Object : SelectedObjects)
        {
            CurrentBlueprint = Cast<UBlueprint>(Object);
            if (CurrentBlueprint)
            {
                break;
            }
        }
    }
    
    // If no blueprint is selected directly, try to find blueprints from selected actors
    if (!CurrentBlueprint && GEditor && GEditor->GetSelectedActors())
    {
        for (FSelectionIterator It = GEditor->GetSelectedActorIterator(); It; ++It)
        {
            AActor* Actor = Cast<AActor>(*It);
            if (Actor)
            {
                UClass* ActorClass = Actor->GetClass();
                if (ActorClass && ActorClass->ClassGeneratedBy)
                {
                    CurrentBlueprint = Cast<UBlueprint>(ActorClass->ClassGeneratedBy);
                    if (CurrentBlueprint)
                    {
                        break;
                    }
                }
            }
        }
    }
    
    if (!CurrentBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBlueprintSelected", "No blueprint is currently selected or open."));
        return;
    }
    
    // Extract data from the blueprint
    FBlueprintData BlueprintData = FBlueprintDataExtractor::ExtractBlueprintData(CurrentBlueprint);
    
    // Build the result message
    FString ResultMessage = FString::Printf(TEXT("Blueprint Analysis: %s\n\n"), *BlueprintData.Name);
    ResultMessage += FString::Printf(TEXT("Path: %s\n"), *BlueprintData.Path);
    ResultMessage += FString::Printf(TEXT("Parent Class: %s\n\n"), *BlueprintData.ParentClass);
    
    // Add functions
    ResultMessage += FString::Printf(TEXT("Functions (%d):\n"), BlueprintData.Functions.Num());
    for (const FBlueprintFunctionData& Function : BlueprintData.Functions)
    {
        ResultMessage += FString::Printf(TEXT("- %s%s\n"), *Function.Name, Function.IsEvent ? TEXT(" (Event)") : TEXT(""));
        
        if (Function.Params.Num() > 0)
        {
            ResultMessage += TEXT("  Parameters:\n");
            for (const FBlueprintParamData& Param : Function.Params)
            {
                ResultMessage += FString::Printf(TEXT("  - %s: %s\n"), *Param.Name, *Param.Type);
            }
        }
        
        if (!Function.ReturnType.IsEmpty())
        {
            ResultMessage += FString::Printf(TEXT("  Return Type: %s\n"), *Function.ReturnType);
        }
        
        ResultMessage += TEXT("\n");
    }
    
    // Add variables
    ResultMessage += FString::Printf(TEXT("Variables (%d):\n"), BlueprintData.Variables.Num());
    for (const FBlueprintVariableData& Variable : BlueprintData.Variables)
    {
        ResultMessage += FString::Printf(TEXT("- %s: %s\n"), *Variable.Name, *Variable.Type);
        
        if (!Variable.DefaultValue.IsEmpty())
        {
            ResultMessage += FString::Printf(TEXT("  Default Value: %s\n"), *Variable.DefaultValue);
        }
        
        if (Variable.IsExposed)
        {
            ResultMessage += TEXT("  Exposed to Editor\n");
        }
        
        if (Variable.IsReadOnly)
        {
            ResultMessage += TEXT("  Read Only\n");
        }
        
        if (Variable.IsReplicated)
        {
            ResultMessage += TEXT("  Replicated\n");
        }
        
        ResultMessage += TEXT("\n");
    }
    
    // Show the results
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
#else
    FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EditorOnly", "This feature is only available in the editor."));
#endif
}

void FBlueprintAnalyzerEditorModule::SendToMCPHandler()
{
    if (!FMCPIntegration::IsConnected())
    {
        // Prompt for server URL
        TSharedRef<SWindow> Window = SNew(SWindow)
            .Title(LOCTEXT("MCPConnectionTitle", "MCP Server Connection"))
            .ClientSize(FVector2D(400, 100))
            .SupportsMaximize(false)
            .SupportsMinimize(false);
        
        FString ServerURL = TEXT("http://localhost:3000");
        FString APIKey;
        
        TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
            + SVerticalBox::Slot()
              .Padding(10)
              .AutoHeight()
              [
                  SNew(STextBlock)
                  .Text(LOCTEXT("MCPServerURL", "Enter MCP Server URL:"))
              ]
            + SVerticalBox::Slot()
              .Padding(10)
              .AutoHeight()
              [
                  SNew(SEditableTextBox)
                  .Text(FText::FromString(ServerURL))
                  .OnTextCommitted_Lambda([&ServerURL](const FText& NewText, ETextCommit::Type CommitType)
                  {
                      ServerURL = NewText.ToString();
                  })
              ]
            + SVerticalBox::Slot()
              .Padding(10)
              .AutoHeight()
              [
                  SNew(STextBlock)
                  .Text(LOCTEXT("MCPAPIKey", "API Key (optional):"))
              ]
            + SVerticalBox::Slot()
              .Padding(10)
              .AutoHeight()
              [
                  SNew(SEditableTextBox)
                  .Text(FText::FromString(APIKey))
                  .OnTextCommitted_Lambda([&APIKey](const FText& NewText, ETextCommit::Type CommitType)
                  {
                      APIKey = NewText.ToString();
                  })
              ]
            + SVerticalBox::Slot()
              .Padding(10)
              .AutoHeight()
              [
                  SNew(SButton)
                  .Text(LOCTEXT("Connect", "Connect"))
                  .OnClicked_Lambda([&Window, &ServerURL, &APIKey]()
                  {
                      FMCPIntegration::Initialize(ServerURL, APIKey);
                      Window->RequestDestroyWindow();
                      return FReply::Handled();
                  })
              ];
        
        Window->SetContent(Content);
        FSlateApplication::Get().AddModalWindow(Window, nullptr, false);
    }
    
    // Only proceed if connected
    if (!FMCPIntegration::IsConnected())
    {
        return;
    }
    
    // Get all blueprints
    TArray<FBlueprintData> Blueprints = FBlueprintDataExtractor::GetAllBlueprints();
    
    // Show progress notification
    FNotificationInfo Info(LOCTEXT("SendingBlueprintData", "Sending Blueprint Data to MCP..."));
    Info.bFireAndForget = false;
    Info.ExpireDuration = 5.0f;
    Info.FadeOutDuration = 1.0f;
    TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
    
    if (NotificationPtr.IsValid())
    {
        NotificationPtr->SetCompletionState(SNotificationItem::CS_Pending);
    }
    
    // Send data to MCP server
    FMCPIntegration::SendBlueprintsData(Blueprints, [NotificationPtr](bool bSuccess)
    {
        // Update notification
        if (NotificationPtr.IsValid())
        {
            if (bSuccess)
            {
                NotificationPtr->SetText(LOCTEXT("SendSuccess", "Blueprint data successfully sent to MCP server."));
                NotificationPtr->SetCompletionState(SNotificationItem::CS_Success);
            }
            else
            {
                NotificationPtr->SetText(LOCTEXT("SendFailed", "Failed to send blueprint data to MCP server."));
                NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);
            }
            
            NotificationPtr->ExpireAndFadeout();
        }
    });
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintAnalyzerEditorModule, BlueprintAnalyzerEditor)