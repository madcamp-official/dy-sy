# Current Task — Sword Combat

## Status
VR Player Health has passed PIE validation and is complete. Sword Combat is now the active task.

## Goal
Implement right-hand sword combat and connect sword hits to the verified Damage System.

## Preconditions
- [x] Damage System complete
- [x] VR Player Health complete
- [ ] Git checkpoint created

## Scope
- Inspect the saved right-controller and sword asset configuration before editing.
- Use only sword assets that actually exist in the project.
- Preserve existing XR locomotion, snap turn, tracking, and player-health behavior.
- Use collision events for sword attacks.
- Call `BPI_Damage2.ApplyDamage` on valid damageable targets.
- Prevent duplicate damage from a single swing/contact.
- Prioritize Quest performance.

## Restrictions
- Do not recreate, replace, delete, or rename existing XR assets.
- Do not put core combat logic in the Level Blueprint.
- Do not rewrite existing locomotion or controller tracking.
- Do not continue automatically to Fireball Combat.
- Do not claim completion without Compile, Save, and PIE evidence.

## Completion Report
- inspected assets
- modified assets
- compile and save results
- PIE evidence
- Output Log and dirty state
- remaining blockers
- suggested Git commit message
