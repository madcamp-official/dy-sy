# Next Task — Fireball Combat

Begin only after Sword Combat is complete and explicitly approved.

## Goal
Implement left-hand fireball combat and connect fireball hits to the verified Damage System.

## Preconditions
- Damage System complete
- VR Player Health complete
- Sword Combat complete
- Git checkpoint created

## Scope
- Inspect the saved left-controller and fireball assets before editing.
- Use only assets that actually exist in the project.
- Preserve existing XR locomotion, tracking, player health, and sword behavior.
- Use event-driven spawning, collision, and cleanup.
- Call `BPI_Damage2.ApplyDamage` on valid damageable targets.
- Prevent unintended repeated damage.
- Prioritize Quest performance.

## Restrictions
- Do not recreate, replace, delete, or rename existing XR assets.
- Do not put core combat logic in the Level Blueprint.
- Do not rewrite existing locomotion or controller tracking.
- Do not continue automatically to Enemy Combat.
- Do not claim completion without Compile, Save, and PIE evidence.
