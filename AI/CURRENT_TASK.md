# Current Task — Player Magic and Sword VFX Package

## Status

Ready to start. Minimal Player HUD and Automatic Restart on Player Death are complete.

## Objective

Complete the remaining player-owned VFX integration:

- Left-hand charge Aura
- Sword Trail
- Sword Wave launch integration

Work in one small feature at a time. Inspect existing saved player assets and Niagara systems before changing them.

## Required preservation

- Existing player health and `BPI_Damage2` path
- `/Game/UI/WBP_PlayerHUD` and the left-wrist `PlayerHUDWidget`
- Event-driven `UpdatePlayerHUD`; do not add Tick-based HUD polling
- `IsDead` one-shot death state
- `DeathRestartDelay=2.0`
- Automatic `RestartCurrentLevel`
- Alive-only locomotion and Fireball gates
- Death-time Sword collision shutdown
- XR camera, HMD, controller, and hand tracking

## Team ownership restrictions

Do not modify Enemy, Goblin, enemy HP bar, enemy AI/collision/animation, Wave Manager, boss, or other teammate-owned assets without explicit coordination.

## Gate

Start with the left-hand charge Aura only. Compile, save, and validate that small feature before beginning Sword Trail or Sword Wave launch.
