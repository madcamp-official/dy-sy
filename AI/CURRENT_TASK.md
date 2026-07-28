# Current Task — End-to-End Enemy Damage Validation

## Status

Not started. This is the single recommended next task.

## Objective

Prove one existing player attack damages the real Enemy and updates its HP bar in `/Game/Maps/L_Test`; if it fails, repair only the smallest broken execution/collision link.

## Exact target assets

- `/Game/Blueprints/Weapons/BP_Sword`
- `/Game/Blueprints/Magic/BP_Fireball`
- `/Game/Blueprints/Enemies/BP_Enemy`
- `/Game/UI/WBP_EnemyHealthBar`
- `/Game/Blueprints/Interfaces/BPI_Damage2` (inspect only)
- `/Game/Maps/L_Test` (PIE test only; do not save)

## Allowed modifications

- Only the smallest wiring or collision correction needed in Sword or Fireball to deliver the already configured damage.
- HP-bar refresh wiring only if health changes but the bar does not.

## Restrictions

- Do not change damage values, enemy MaxHealth, AI, animation, locomotion, XR tracking, or Marketplace assets.
- Do not modify `BPI_Damage2`, Wave Manager, boss work, project settings, or Level Blueprint.
- Do not save `/Game/Maps/L_Test`; Git already contains an uncommitted map change.

## Implementation requirements

1. Inspect the saved attack-to-interface execution chain and compatible overlap settings.
2. Run one deterministic PIE hit against `/Game/Blueprints/Enemies/BP_Enemy`.
3. Confirm health changes from 60 to the expected value and the visible bar changes in the same event.
4. If the test fails, change only the first proven broken link and repeat.

## Compile/save tests

- Compile every modified Blueprint with warnings treated as errors.
- Save only modified Blueprint assets.
- Confirm modified Blueprints are `dirty=false`.
- No Compile Error, Broken Reference, or new warning in the modified assets.

## PIE completion criteria

- One Sword or Fireball hit reaches `BPI_Damage2` exactly once.
- Enemy health decreases from 60 by the configured attack damage.
- `WBP_EnemyHealthBar` visibly updates immediately.
- No Blueprint Runtime Error, Accessed None, or collision spam.
- PIE stops cleanly and L_Test remains unsaved.

## Current blockers

- Physical OpenXR input cannot be relied upon for automated testing; use a deterministic in-editor trigger.
- L_Test is modified in Git despite reporting clean in the editor.
- Fireball and Sword have saved structural wiring but lack current successful end-to-end PIE proof.
