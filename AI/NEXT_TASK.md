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
