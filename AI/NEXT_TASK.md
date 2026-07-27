# Next Tasks

Begin these tasks only after the Player Magic and Sword VFX Package is compiled, saved, and verified in `L_Test` PIE.

## Ordered follow-up work

1. Player death and restart
2. HUD
3. Boss combat
4. Victory and defeat
5. Quest build and device test

## Transition rule

- Do not mark Player Magic or Sword VFX complete without saved assets, successful Blueprint compile results, and PIE evidence.
- Do not start a follow-up task automatically.
- Preserve completed XR tracking, locomotion, grabbing, player health, and Damage System behavior.


## Current-package gate

The independent projectile assets now exist, but the current package is not complete. Do not advance to player death/restart until input spawning, sword integration, and full PIE validation are unblocked and pass.

## Fireball validation gate

Before advancing the current package, verify repeated physical VR fireball shots spawn 25 cm in front of the left aim, follow aim direction, remain uniformly scaled at 0.5, and travel normally.

## Fireball visual consistency follow-up

- Editor-side visual consistency is now structurally complete: deterministic 25 cm forward spawn, inherited LeftAim world rotation, centered Niagara effect, fixed 0.5 uniform scale, and 14 cm collision radius.
- The next smallest task remains physical VR Preview/Quest repeated-shot validation only.
- Do not change fireball gameplay logic or advance to player death/restart until the current VFX package gate is satisfied.

## Enemy HP bar validation gate

- Editor implementation and initial full-health PIE validation are complete.
- The next smallest validation task is one successful damage event against `BP_DamageDummy`, confirming the bar changes immediately from 100%.
- After that, separately validate damage-to-zero, Destroy, and widget disappearance.
- Sword-specific HP bar validation remains blocked until the existing `BP_Sword.TrySwordDamage` execution path is repaired in a separately authorized task.

## Enemy HP bar corrected validation gate

- Do not use `BP_DamageDummy` for HP bar validation; it intentionally has no HP bar.
- The next smallest validation is one successful damage hit on either `BP_Enemy` or `BP_Goblin`, confirming the visible bar changes immediately from `40/40`.
- Validate actor destruction and automatic widget removal separately after a successful damage-to-zero path.
