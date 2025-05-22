# Harvesting Auto-Pickup Walkthrough

## Goal
Automatically add resources to player inventory when harvesting, instead of spawning drops.

## Steps
1. Open the blueprint [`BP_DiggingActor_Base`](blueprint:///Game/EasySurvivalRPG/Blueprints/InteractionObjects/Misc/BP_DiggingActor_Base.BP_DiggingActor_Base).
2. In the Event Graph, locate the event node `TryToDig_BPI`.
3. Replace or extend the existing drop logic:
   - Remove or disable the `SpawnDropItem` call.
   - Drag off the `then` execution pin and call the function [`TryAddItemsOrDropAtLocation`](blueprint:///Game/EasySurvivalRPG/Blueprints/Components/BP_ContainerComponent.BP_ContainerComponent:TryAddItemsOrDropAtLocation) on the Instigator.
     - Pass the resource array or value as input.
4. To get the player's container component:
   - Cast the `Instigator` pin to [`BP_Character_Player`](blueprint:///Game/EasySurvivalRPG/Blueprints/Characters/Base/BP_Character_Player.BP_Character_Player).
   - From the cast result, get the `ContainerComponent` reference.
5. Branch on the return value of `TryAddItemsOrDropAtLocation`:
   - If **true**, skip spawning drops (pickup succeeded).
   - If **false**, call `SpawnDropItem` as fallback.
6. Compile and save the blueprint.

## Validation
- Place the game in PIE.
- Hit a rock or tree actor.
- Confirm resources appear in your inventory UI.