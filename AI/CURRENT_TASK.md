# Current Task — Compact Player HUD

## Status

In progress. The compact GUI Parts layout was assembled in the connected editor, but the asset-scoped save API returned false. Do not mark complete until the saved assets and runtime behaviors are verified.

## Target

- `/Game/UI/WBP_PlayerHUD`
- `/Game/XRFramework/Blueprints/BP_XRPawn`
- Test only in `/Game/Maps/L_Test`; do not save the map.

## Implemented in the current editor session

- Existing `WBP_PlayerHUD` reused and expanded with a corner-based Canvas layout.
- GUI Parts reused for HP, three spell slots/icons, and the circular minimap frame.
- Three float values added: `MagicSlot1Charge`, `MagicSlot2Charge`, `MagicSlot3Charge`.
- HUD widget component changed to a compact camera-attached curved world-space HUD.
- Existing health function and player-owned gameplay components were preserved.

## Still required

- Save only `WBP_PlayerHUD` and `BP_XRPawn`.
- Connect three magic values to their independent progress bars.
- Connect the nearby-enemy warning and event-driven blink without modifying enemy assets.
- Connect `IA_Menu_Toggle_Right` Started to minimap visibility with one toggle per press.
- Create and connect SceneCapture2D/RenderTarget if supported.
- Compile, run L_Test PIE, check runtime errors, and capture a screenshot.

## Restrictions

Do not modify enemy, goblin, boss, wave manager, enemy health widget, damage interface, Marketplace source assets, or `L_Test`.
