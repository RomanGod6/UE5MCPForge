# Core Flow Blueprints Documentation

This document provides **extreme** detailed documentation for the core flow blueprints in EasySurvivalRPG, including full lists of functions with parameters and all variables with types and categories.

## BP_GameInstance

**Path**: /Game/EasySurvivalRPG/Blueprints/Game/GameInstance/BP_GameInstance.BP_GameInstance

**Parent Class**: AdvancedFriendsGameInstance

### Functions (72)

- SpawnItems(Items: struct, Location: struct, MinRange: real, MaxRange: real, MinHeight: real, MaxHeight: real)
- SpawnLootItems(LootHandle: struct, Location: struct, MinRange: real, MaxRange: real, MinHeight: real, MaxHeight: real)
- SaveSingleplayerGame(SlotName: string, PlayerData: struct)
- SaveLevelData(GameSave: object)
- LoadLevelData(GameSave: object, LevelData: struct)
- SaveMultiplayerGame(SlotName: string, HostPlayerData: struct)
- AddLoadingScreen(PlayShowAnimation: bool, AnimationSpeed: real)
- InitializeGameSettings()
- CheckHardware()
- ApplyVolumeSettings(MasterVolume: real, MusicVolume: real, EffectsVolume: real, VoiceVolume: real)
- UpdateVolumeSettings()
- ResetVolumeSettings()
- ApplyGameSettings(AutosaveMode: byte, ShowFloatingText: bool, ShowCharacterState: bool, ShowEnemyCharacterStates: bool, ShowMinimap: bool, ShowMarks: bool, ShowBuildingDurability: bool)
- StartNewGame(IsMultiplayer: bool)
- StartAndContinueGame(IsMultiplayer: bool)
- StartAndLoadGame(IsMultiplayer: bool, SlotName: string)
- OpenMainMenuLevel()
- CreateNewGameSaveSlot(SlotName: string, IsMultiplayer: bool, NewPlayerData: struct)
- ExecutePlayerLevelEvent(PlayerController: object, EventName: name)
- AddGameTag(GameTag: struct)
- RemoveGameTag(GameTag: struct)
- SetGameTags(GameTags: struct)
- UpdateGameTags()
- SetCurrentLevelName(LevelName: name)
- TransitToLevel(LevelName: name, PointName: name)
- CheckCurrentLevelName()
- ClearTransitPoint()
- UpdateGraphicSettings()
- ApplyGraphicSettings(VolumetricFogQuality: int)
- SetVolumetricFogQuality(Value: int)
- SetGameStarted(IsGameStarted: bool, IsMultiplayer: bool)
- TryToLoadDedicatedGameSave()
- CreateNewDedicatedGameSave(SlotName: string)
- SpawnSleepCharacter(PlayerData: struct)
- SpawnSleepCharacters(PlayersData: struct)
- SaveDedicatedGame(SlotName: string)
- GetPlayersSaveData()
- LoadLevelFromGameSave(GameSave: object)
- SaveGameToSlot(SaveSlot: string, HostPlayerData: struct)
- TryToLoadCurrentGameSave()
- DedicatedAutosaveTick()
- UpdateSavedPlayerData(PlayerData: struct)
- KillSavedPlayerCharacter(PlayerID: string)
- HideLoadingScreen()
- ClearLoadingScreen()
- StartNewGame_BPI(OutputDelegate: delegate, IsMultiplayer: bool)
- CreateNewGameSaveSlot_BPI(OutputDelegate: delegate, SlotName: string, IsMultiplayer: bool, NewPlayerData: struct)
- CheckHardware_BPI(OutputDelegate: delegate)
- InitializeGameSettings_BPI(OutputDelegate: delegate)
- ReceiveInit(OutputDelegate: delegate)
- ApplyVolumeSettings_BPI(OutputDelegate: delegate, MasterVolume: real, MusicVolume: real, EffectsVolume: real, VoiceVolume: real)
- ResetVolumeSettings_BPI(OutputDelegate: delegate)
- SpawnItems_BPI(OutputDelegate: delegate, Items: struct, Location: struct, MinRange: real, MaxRange: real, MinHeight: real, MaxHeight: real)
- SpawnLootItems_BPI(OutputDelegate: delegate, LootHandle: struct, Location: struct, MinRange: real, MaxRange: real, MinHeight: real, MaxHeight: real)
- ContinueGame_BPI(OutputDelegate: delegate, IsMultiplayer: bool)
- LoadGame_BPI(OutputDelegate: delegate, IsMultiplayer: bool, SlotName: string)
- DeleteSingleplayerGameSave_BPI(OutputDelegate: delegate, SaveName: string)
- DeleteMultiplayerGameSave_BPI(OutputDelegate: delegate, SaveName: string)
- SetTransferPlayerData_BPI(OutputDelegate: delegate, PlayerData: struct)
- OpenMainMenuLevel_BPI(OutputDelegate: delegate)
- SetGameStarted_BPI(OutputDelegate: delegate, GameStarted: bool, IsMultiplayer: bool)
- ApplyGameSettings_BPI(OutputDelegate: delegate, AutosaveMode: byte, ShowFloatingText: bool, ShowCharacterState: bool, ShowEnemyCharacterStates: bool, ShowMinimap: bool, ShowMarks: bool, ShowBuildingDurability: bool)
- LoadLevelData_BPI(OutputDelegate: delegate, GameSave: object, LevelData: struct)
- HideLoadingScreen_BPI(OutputDelegate: delegate)
- SaveSingleplayerGame_BPI(OutputDelegate: delegate, SlotName: string, PlayerData: struct, PlayersData: struct)
- SaveMultiplayerGame_BPI(OutputDelegate: delegate, SlotName: string, HostPlayerData: struct, ClientsPlayerData: struct)
- ExecutePlayerLevelEvent_BPI(OutputDelegate: delegate, PlayerController: object, EventName: name)
- AddGameTag_BPI(OutputDelegate: delegate, GameTag: struct)
- RemoveGameTag_BPI(OutputDelegate: delegate, GameTag: struct)
- SetGameTags_BPI(OutputDelegate: delegate, GameTags: struct)
- SetCurrentLevelName_BPI(OutputDelegate: delegate, LevelName: name)
- TransitToLevel_BPI(OutputDelegate: delegate, LevelName: name, PointName: name)
- ClearTransitPoint_BPI(OutputDelegate: delegate)
- ResetGraphicSettings_BPI(OutputDelegate: delegate)
- ApplyGraphicSettings_BPI(OutputDelegate: delegate, VolumetricFogQuality: int)
- SetVolumetricFogQuality_BPI(OutputDelegate: delegate, Value: int)
- SaveDedicatedGame_BPI(OutputDelegate: delegate, SlotName: string)
- OnSessionInviteReceived(OutputDelegate: delegate, LocalPlayerNum: int, PersonInviting: struct, AppId: string, SessionToJoin: struct)
- ComponentBeginOverlapSignature__DelegateSignature(OutputDelegate: delegate, OverlappedComponent: object, OtherActor: object, OtherComp: object, OtherBodyIndex: int, bFromSweep: bool, SweepResult: struct)
- ComponentEndOverlapSignature__DelegateSignature(OutputDelegate: delegate, OverlappedComponent: object, OtherActor: object, OtherComp: object, OtherBodyIndex: int)
- AddStatusEffect_BPI(OutputDelegate: delegate, StatusEffectData: struct)
- RemoveStatusEffect_BPI(OutputDelegate: delegate, StatusEffectHandle: struct)
- UpdateStatusEffect_BPI(OutputDelegate: delegate, StatusEffectData: struct)
- TryToLearnCharacterSkill_BPI(OutputDelegate: delegate, SkillHandle: struct)
- TryToAddAbstractItemToSlot_BPI(OutputDelegate: delegate, Item: struct, Container: object, Slot: int)
- TryToAddAbstractItemToHotbar_BPI(OutputDelegate: delegate, Item: struct)
- AddLoadingScreen_BPI(OutputDelegate: delegate, PlayShowAnimation: bool, AnimationSpeed: real)
- HideLoadingScreen_BPI(OutputDelegate: delegate)
- None(OutputDelegate: delegate) (occurs twice)
- ChangePlayerAttribute_BPI(OutputDelegate: delegate, AttributeTag: struct, Value: real, Operation: byte)
- TryPickupBuilding_BPI(OutputDelegate: delegate, BuildingObject: object)
- OpenSubMenu_BPI(OutputDelegate: delegate)
- TryToPlayEmotion_BPI(OutputDelegate: delegate, EmotionName: name)
- InputAxisHandlerDynamicSignature__DelegateSignature(OutputDelegate: delegate, AxisValue: real) (x4)

### Variables (30)

- IsGameStarted: Boolean (Category: State)
- IsMultiplayer: Boolean (Category: State)
- CurrentGameSave: BP Game Save Session Object Reference (Category: State)
- SettingsGameSave: BP Game Save Settings Object Reference (Category: State)
- SettingsGameSaveSlot: String (Category: Settings|Game)
- DedicatedServerSlotName: String (Category: Settings|Game)
- DedicatedAutosaveMode: E_AutosaveMode Enum (Category: Settings|Game)
- SingleplayerStartLevel: Name (Category: Settings|Game)
- MultiplayerStartLevel: Name (Category: Settings|Game)
- CurrentLevel: Name (Category: State)
- TransitActive: Boolean (Category: State)
- TransitPoint: Name (Category: State)
- TransferPlayerData: STR Save Data Player Structure (Category: State)
- MainMenuLevel: Name (Category: Settings|Game)
- LoadingScreen: UI Loading Screen Object Reference (Category: State)
- GameTags: Gameplay Tag Container Structure (Category: State)
- PlayerCharacterClass: Character Class Reference (Category: Settings|Game)
- PlayerCharacterClasses: Array of Character Class References (Category: Settings|Game)
- StartingCharacterLevel: Integer (Category: Settings|Player Character|Starting Data)
- StartingAttributes: Array of STR Attribute Structures (Category: Settings|Player Character|Starting Data)
- StartingJournalNotes: Array of Data Table Row Handle Structures (Category: Settings|Player Character|Starting Data)
- StartingPlayerTags: Gameplay Tag Container Structure (Category: Settings|Player Character|Starting Data)
- StartingAdditionalBlueprints: Array of STR Additional Blueprint Structures (Category: Settings|Player Character|Starting Data)
- StartingResources: Array of STR Resource Value Structures (Category: Settings|Player Character|Starting Data)
- StartingActiveQuests: Array of STR Quest Structures (Category: Settings|Player Character|Starting Data)
- StartingCharacterSkills: Array of STR Skill Structures (Category: Settings|Player Character|Starting Data)
- UseStartingItemsFromInstance: Boolean (Category: Settings|Player Character|Starting Items)
- UseRespawnItemsFromInstance: Boolean (Category: Settings|Player Character|Respawn Items)
- StartingEquipmentHandle: Data Table Row Handle Structure (Category: Settings|Player Character|Starting Items)
- RespawnEquipmentHandle: Data Table Row Handle Structure (Category: Settings|Player Character|Respawn Items)

## BP_GameMode_Game

**Path**: /Game/EasySurvivalRPG/Blueprints/Game/GameModes/BP_GameMode_Game.BP_GameMode_Game

**Parent Class**: ClanGameMode

### Functions (2)

- UserConstructionScript()
- K2_OnLogout(OutputDelegate: delegate, ExitingController: object)

### Variables (1)

- NewVar: Guid Structure (Category: Default)

## BP_GameState_Base

**Path**: /Game/EasySurvivalRPG/Blueprints/Game/GameStates/BP_GameState_Base.BP_GameState_Base

**Parent Class**: GameStateBase

### Functions (1)

- UserConstructionScript()

### Variables (0)

## BP_PlayerController

**Path**: /Game/EasySurvivalRPG/Blueprints/Game/PlayerControllers/BP_PlayerController.BP_PlayerController

**Parent Class**: MyPlayerController

### Functions (106)

- UserConstructionScript()
- ReceiveBeginPlay()
- OpenInventory_BPI(OutputDelegate: delegate)
- OpenContainer_BPI(OutputDelegate: delegate, Container: object)
- CloseHUD_BPI(OutputDelegate: delegate)
- TryPickUpItem_BPI(OutputDelegate: delegate, ItemObject: object)
- TryMoveItemToContainerSlot_BPI(OutputDelegate: delegate, FromContainer: object, FromSlot: int, FromSlotType: byte, ToContainer: object, ToSlot: int, Amount: int)
- TryEquipItem_BPI(OutputDelegate: delegate, FromContainer: object, FromSlot: int)
- TryUnequipItem_BPI(OutputDelegate: delegate, FromSlot: int)
- TryMoveItemToContainer_BPI(OutputDelegate: delegate, FromContainer: object, FromSlot: int)
- QuickActionContainerSlot_BPI(OutputDelegate: delegate, Container: object, Slot: int)
- QuickActionEquipmentSlot_BPI(OutputDelegate: delegate, Equipment: object, Slot: int)
- TryMoveItemToHotbar_BPI(OutputDelegate: delegate, FromContainer: object, FromSlot: int)
- QuickActionHotbarSlot_BPI(OutputDelegate: delegate, Container: object, Slot: int)
- TryDropItemFromSlot_BPI(OutputDelegate: delegate, FromContainer: object, FromSlot: int, FromSlotType: byte, Amount: int)
- StartCrafting_BPI(OutputDelegate: delegate, Blueprint: struct, Amount: int)
- CancelCrafting_BPI(OutputDelegate: delegate, IndexInQueue: int)
- TryStartTrade_BPI(OutputDelegate: delegate, TradeComponent: object)
- TrySellItem_BPI(OutputDelegate: delegate, Item: struct)
- TryBuyItem_BPI(OutputDelegate: delegate, Item: struct)
- SelectItem_BPI(OutputDelegate: delegate, ItemHandle: struct, Container: object, Slot: int)
- UpdateContainerSlot_BPI(OutputDelegate: delegate, Container: object, Slot: int, Item: struct)
- OpenInteractionSwitcher_BPI(OutputDelegate: delegate, InteractionObject: object)
- TryTurnOn_BPI(OutputDelegate: delegate, InteractionObject: object)
- TryTurnOff_BPI(OutputDelegate: delegate, InteractionObject: object)
- OpenCraftingPlace_BPI(OutputDelegate: delegate, CraftingComponent: object, CraftingContainer: object, OnlyCraftingQueue: bool, DisableManualCrafting: bool)
- UpdateCraftingQueue_BPI(OutputDelegate: delegate)
- UpdateCraftingQueueBlueprint_BPI(OutputDelegate: delegate, IndexInQueue: int, QueueBlueprint: struct)
- TryStartBuild_BPI(OutputDelegate: delegate, BuildingHandle: struct)
- RemoveRequiredResources_BPI(OutputDelegate: delegate, RequiredResources: struct)
- RemoveRequiredItems_BPI(OutputDelegate: delegate, RequiredItems: struct)
- AddExperience_BPI(OutputDelegate: delegate, Experience: real)
- SpendSkillPointsToAttribute_BPI(OutputDelegate: delegate, Attribute: struct, Value: real, SkillPoints: int)
- UpdateCharacterInfo_BPI(OutputDelegate: delegate)
- CreateFloatingTextAtLocation_BPI(OutputDelegate: delegate, Location: struct, Text: text, TextColor: struct, TextType: byte)
- UpdateContainerResources_BPI(OutputDelegate: delegate, Container: object, Resources: struct)
- AddResources_BPI(OutputDelegate: delegate, Resources: struct)
- TryToStartDialogue_BPI(OutputDelegate: delegate, DialogueHandle: struct, NpcCharacter: object)
- UpdateDialogue_BPI(OutputDelegate: delegate, SpeakerName: text, SpeakerText: text, SpeakerIcon: object, PlayerReplies: struct)
- SelectDialogueReply_BPI(OutputDelegate: delegate, Reply: struct)
- FinishDialogue_BPI(OutputDelegate: delegate)
- TakeAllFromContainer_BPI(OutputDelegate: delegate)
- ShowInterestPoint_BPI(OutputDelegate: delegate, PointText: text)
- AddGameplayTags_BPI(OutputDelegate: delegate, TagContainer: struct)
- Death_BPI(OutputDelegate: delegate)
- Respawn_BPI(OutputDelegate: delegate)
- OpenPauseMenu_BPI(OutputDelegate: delegate)
- ClosePauseMenu_BPI(OutputDelegate: delegate)
- UpdateWeight_BPI(OutputDelegate: delegate, Weight: real, MaxWeight: real, OverloadFactor: real)
- SelectHotbarSlot_BPI(OutputDelegate: delegate, SelectedSlot: int)
- TryRotate_BPI(OutputDelegate: delegate)
- TryDestruct_BPI(OutputDelegate: delegate)
- TryUpgrade_BPI(OutputDelegate: delegate)
- TryRepair_BPI(OutputDelegate: delegate)
- CloseSubMenu_BPI(OutputDelegate: delegate)
- AddJournalNote_BPI(OutputDelegate: delegate, JournalNote: struct)
- CreateMark_BPI(OutputDelegate: delegate, Transform: struct, MarkTag: name, MarkName: text, Icon: object, ShowWorldMark: bool, ShowMinimapMark: bool, ShowMapMark: bool, ShowDistance: bool, TargetActor: object, TargetHeight: real, RotateMark: bool)
- OpenMap_BPI(OutputDelegate: delegate)
- OpenJournal_BPI(OutputDelegate: delegate)
- SelectMapMark_BPI(OutputDelegate: delegate, Mark: object)
- OpenCodelock_BPI(OutputDelegate: delegate, Codelock: object)
- TrySetCodelockPassword_BPI(OutputDelegate: delegate, Password: string)
- UpdateCodelock_BPI(OutputDelegate: delegate, IsLocked: bool, IsAutorized: bool, Password: string)
- None(OutputDelegate: delegate) (occurs twice)
- AddBlueprintToList_BPI(OutputDelegate: delegate, TargetBlueprintList: struct, Blueprint: struct)
- RemoveBlueprintFromList_BPI(OutputDelegate: delegate, TargetBlueprintList: struct, Blueprint: struct)
- ItemPicked_BPI(OutputDelegate: delegate, Item: struct)
- SaveGameToSlot_BPI(OutputDelegate: delegate, SlotName: string)
- SetAutosaveMode_BPI(OutputDelegate: delegate, AutosaveMode: byte)
- SetShowCharacterState_BPI(OutputDelegate: delegate, ShowCharacterState: bool)
- SetShowEnemyCharacterStates_BPI(OutputDelegate: delegate, ShowEnemyCharacterStates: bool)
- SetShowMinimap_BPI(OutputDelegate: delegate, ShowMinimap: bool)
- SetShowMarks_BPI(OutputDelegate: delegate, ShowMarks: bool)
- SetShowBuildingDurability_BPI(OutputDelegate: delegate, ShowBuildingDurability: bool)
- BlockHotbarSlot_BPI(OutputDelegate: delegate, Block: bool, Slot: int)
- PlayerLogout_BPI(OutputDelegate: delegate)
- SendChatMessage_BPI(OutputDelegate: delegate, Message: struct)
- AddChatMessage_BPI(OutputDelegate: delegate, Message: struct)
- SavePlayerData_BPI(OutputDelegate: delegate)
- PlayDamageReaction_BPI(OutputDelegate: delegate)
- PlayBlockReaction_BPI(OutputDelegate: delegate)
- PlayHitReaction_BPI(OutputDelegate: delegate)
- StartQuest_BPI(OutputDelegate: delegate, QuestClass: class)
- UpdateQuestProgress_BPI(OutputDelegate: delegate, QuestID: name, QuestStage: name, QuestProgress: string, QuestProgressText: text)
- PlayerDestructTree_BPI(OutputDelegate: delegate, Tree: object)
- CompleteCrafting_BPI(OutputDelegate: delegate, Blueprint: struct, Amount: int)
- PlayerFullyDestructTree_BPI(OutputDelegate: delegate, Tree: object)
- PlayerDestructMineStage_BPI(OutputDelegate: delegate, Mine: object, Stage: byte)
- PlayerFullyDestructMine_BPI(OutputDelegate: delegate, Mine: object)
- PlayerKillCharacter_BPI(OutputDelegate: delegate, Character: object)
- UpdateQuestTrackingState_BPI(OutputDelegate: delegate, QuestNoteID: name, QuestIsTracked: bool)
- TryToConsumeItem_BPI(OutputDelegate: delegate, Container: object, Slot: int)
- SaveCurrentGame_BPI(OutputDelegate: delegate)
- SetMapEnabled_BPI(OutputDelegate: delegate, MapEnabled: bool)
- SetMinimapEnabled_BPI(OutputDelegate: delegate, MinimapEnabled: bool)
- TryToRepairItem_BPI(OutputDelegate: delegate, FromContainer: object, FromSlot: int, ToContainer: object, ToSlot: int)
- PlayShotReaction_BPI(OutputDelegate: delegate, CameraShakeScale: real, CameraPitchOffset: real)
- HeadshotNotification_BPI(OutputDelegate: delegate)
- ResizeContainer_BPI(OutputDelegate: delegate, Container: object, NewSlots: int)
- InputAxisHandlerDynamicSignature__DelegateSignature(OutputDelegate: delegate, AxisValue: real) (x4)

### Variables (5)

- ChangeBuildingHeight: Boolean (Category: State)
- DebugCommonLootHandle: Data Table Row Handle Structure (Category: Settings)
- IsShiftPressed: Boolean (Category: State)
- DebugRangedLootHandle: Data Table Row Handle Structure (Category: Settings)
- DebugSecretLootHandle: Data Table Row Handle Structure (Category: Settings)

## Next Steps

- Documentation expanded. Review and integrate additional blueprints as needed.