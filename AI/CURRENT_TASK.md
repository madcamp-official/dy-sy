# Current Task — Minimal Player HUD

## Status

Not started. Automatic Restart on Player Death is complete and must be preserved.

## Objective

Define and implement the smallest player-owned HUD required for the prototype.

## Initial target scope

- Inspect existing UI assets before choosing exact widget paths.
- Reuse `/Game/XRFramework/Blueprints/BP_XRPawn` only where player-owned HUD attachment or data access is required.
- Test in `/Game/Maps/L_Test` without saving the map.

## Required preservation

- Existing player health and `BPI_Damage2` path
- `IsDead` one-shot death state
- `DeathRestartDelay=2.0`
- Automatic `RestartCurrentLevel`
- Alive-only locomotion and Fireball gates
- Death-time Sword collision shutdown
- XR camera, HMD, controller, and hand tracking

## Team ownership restrictions

Do not modify Enemy, Goblin, enemy HP bar, enemy AI/collision/animation, Wave Manager, boss, or other teammate-owned assets without explicit coordination.

## Gate

Do not start implementation automatically. Inspect the current UI assets and agree on minimal HUD content and XR placement first.
