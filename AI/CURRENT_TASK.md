# Current Task — Player Magic and Sword VFX Package

## Status
In progress — independent projectile Blueprints implemented; player and sword integration remains blocked.

Verified on 2026-07-27 through the connected Unreal MCP:

- `/Game/Blueprints/Magic/BP_Fireball` exists, compiles, and is saved; PIE behavior remains unverified.
- `/Game/Blueprints/Weapons/BP_SwordWave` exists, compiles, and is saved; PIE behavior remains unverified.
- `BP_XRPawn` has no Niagara component for left-hand charge VFX.
- `BP_Sword` has only its existing sword mesh and `SwordCollision`; no Niagara trail component exists.
- Independent projectile implementations are saved, but package-specific PIE evidence is incomplete.

## Goal
Implement the complete Player Magic and Sword VFX package without recreating existing systems.

## Scope and exact target assets

### 1. Left-hand magic charge VFX

- Modify: `/Game/XRFramework/Blueprints/BP_XRPawn`
- Niagara: `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Aura`
- Attach to the existing left-hand tracking hierarchy.
- Hidden/inactive by default; activate while the chosen magic input is held and deactivate on release.

### 2. Fireball

- Create: `/Game/Blueprints/Magic/BP_Fireball`
- Travel Niagara: `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1`
- Impact Niagara: `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1`
- Spawn from the left hand; use projectile movement and sphere collision.
- Destroy after a valid hit or lifetime expiry.
- Call `/Game/Blueprints/Interfaces/BPI_Damage2` exactly once per valid hit.

### 3. Sword trail

- Modify, do not recreate: `/Game/Blueprints/Weapons/BP_Sword`
- Add a Niagara trail that is visible only during valid swings.
- Preserve existing sword mesh, collision, attachment, speed threshold, and damage behavior.

### 4. Sword wave

- Create: `/Game/Blueprints/Weapons/BP_SwordWave`
- Niagara: `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Slash`
- Launch forward from the sword with projectile movement and collision.
- Destroy after a short lifetime.
- Damage through `BPI_Damage2`, once per actor.

### 5. Combat hit effects

- Niagara: `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit2`
- Spawn only after successful sword, fireball, or sword-wave damage.
- Do not spawn when damage fails.

## Restrictions

- Do not modify `BP_Enemy`, `BP_Boss`, `BP_WaveManager`, `BPI_Damage2`, Marketplace assets, or teammate Blueprints.
- Do not replace `BP_XRPawn`.
- Do not recreate `BP_Sword` or the Damage System.
- Preserve locomotion, HMD tracking, controller tracking, grab system, and sword attachment.
- Do not place core game logic in the Level Blueprint.
- Prefer events, timers, and collision events over Tick where practical.
- Compile, save, and verify each feature before continuing.

## Completion tests

Run PIE in `/Game/Maps/L_Test` only after all feature assets compile and save successfully.

- [ ] Left-hand aura appears while charging.
- [ ] Aura disappears when released.
- [ ] Fireball spawns from the left hand.
- [ ] Fireball travels and expires correctly.
- [ ] Fireball impact effect appears.
- [ ] Fireball damages a valid target exactly once.
- [ ] Sword trail appears only during valid swings and is absent while idle.
- [ ] Existing sword collision and damage still work.
- [ ] Sword wave launches forward and expires correctly.
- [ ] Sword wave damages each actor at most once.
- [ ] Hit2 effect appears only after successful damage from all three attack types.
- [ ] No Blueprint compile errors or warnings treated as errors.
- [ ] No Blueprint runtime errors, broken references, or Accessed None.
- [ ] Locomotion, tracking, grabbing, and sword attachment regressions are absent.

## Current blockers and risks

- `BP_Sword.TrySwordDamage` was previously verified incomplete: its execution path and `BPI_Damage2.ApplyDamage` call require repair before sword-hit VFX can be considered valid.
- `BP_Fireball` and `BP_SwordWave` must be created; no saved implementations exist.
- The magic input choice and wiring are not yet implemented or validated.
- `/Game/Maps/L_Test` was dirty during the 2026-07-27 inspection; its unsaved change was not modified or saved by this documentation task.
- Physical OpenXR input and Quest behavior require device validation after editor PIE validation.


## Independent projectile implementation result

- BP_Fireball: created, compiled with warnings as errors, saved dirty=false.
- BP_SwordWave: created, compiled with warnings as errors, saved dirty=false.
- Structural movement, collision, Niagara, damage, duplicate prevention, and lifespan configuration is present.
- PIE runtime verification did not pass: temporary unsaved test actors were not observable through SceneTools in PIE, and the Fireball test did not reduce DamageDummy health.
- Do not mark this package complete.
- Remaining blocked work: BP_XRPawn input/spawn wiring, left-hand charge, BP_Sword trail, sword-wave launch integration, and full PIE/device validation.

## Fireball spawn/size correction

- Spawn position/direction fix is implemented and saved using a 25 cm child spawn point under MotionControllerLeftAim.
- NS_Free_Magic_Attack2 remains at uniform scale 0.5; collision radius is 14 cm.
- Compile/save and structural PIE validation passed.
- Physical controller firing validation remains; do not mark the overall VFX package complete.
