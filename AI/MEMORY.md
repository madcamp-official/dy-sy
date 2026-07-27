# Project Memory

## Project

- Unreal Engine 5.8
- Meta Quest / OpenXR
- Blueprint-centered implementation
- Main validation map: `/Game/Maps/L_Test`
- Team constraint: 2 people / 7 days / 3–5 minute play session

## Verified completed systems

### Damage System

- Interface: `/Game/Blueprints/Interfaces/BPI_Damage2`
- `BP_DamageDummy` applies damage to `CurrentHealth` and destroys at zero.
- Saved compile and L_Test PIE evidence exists for 100 → 50 → destroyed.

### Player health

- `/Game/XRFramework/Blueprints/BP_XRPawn` implements `BPI_Damage2`.
- `MaxHealth=100`, `CurrentHealth=100`.
- Damage clamps health to 0 without destroying the pawn.
- Saved compile and L_Test PIE evidence exists for 100 → 50 → 0.

### XR spawn and locomotion

- `BP_XRPawn.SpawnCollisionHandlingMethod=AlwaysSpawn` prevents the prior PlayerStart collision from suppressing the pawn and sword.
- Existing HMD, controller, hand, grabbing, and sword attachment hierarchy is present.
- Left-stick HMD-yaw-relative smooth locomotion is implemented at 300 cm/s with swept movement.
- `IA_Turn` mappings were removed, structurally disabling right-stick turning.
- Physical OpenXR/Quest input still requires device verification.

### L_Test enemy visibility

- Existing `BP_Enemy` is reused in `L_Test` and was previously observed visible in PIE.
- Enemy combat logic was not changed.

## Incomplete systems

- Sword Combat
- Player Magic and Sword VFX Package
- Player death and restart
- HUD
- Boss combat and required patterns
- Victory and defeat flow
- Quest packaging and device test

## Current package — verified state on 2026-07-27

- Current task: **Player Magic and Sword VFX Package**.
- Implementation has not started.
- `/Game/Blueprints/Magic/BP_Fireball`: missing.
- `/Game/Blueprints/Weapons/BP_SwordWave`: missing.
- `BP_XRPawn`: no left-hand charge Niagara component.
- `BP_Sword`: existing mesh and collision only; no Niagara trail component.
- No package-specific saved compile or PIE evidence exists.
- `L_Test` was dirty during inspection and was not modified or saved by the documentation task.

## Important asset paths

### Existing gameplay Blueprints

- `/Game/XRFramework/Blueprints/BP_XRPawn`
- `/Game/Blueprints/Weapons/BP_Sword`
- `/Game/Blueprints/Interfaces/BPI_Damage2`
- `/Game/Blueprints/Enemies/BP_DamageDummy`

### Package targets

- `/Game/Blueprints/Magic/BP_Fireball` — to create
- `/Game/Blueprints/Weapons/BP_SwordWave` — to create

### Verified Niagara systems

- `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Aura`
- `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1`
- `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1`
- `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Slash`
- `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit2`

## Known blockers and risks

- `BP_Sword.TrySwordDamage` was previously verified with a disconnected execution path and no effective `BPI_Damage2.ApplyDamage` call. Sword damage and sword-hit VFX cannot be validated until repaired.
- Magic input selection/wiring is not implemented or validated.
- Fireball and sword-wave Blueprints do not yet exist.
- Physical OpenXR axis/button injection is unavailable through the current MCP workflow; final input, tracking, and feel require Quest device testing.
- Android SDK and Quest packaging readiness remain unverified.
- Existing repository warning: `dy-sy` is recorded as mode 160000 without a matching `.gitmodules` entry.
- Do not modify Marketplace originals, restricted combat Blueprints, or teammate-owned Blueprints.

## Documentation update rule

Only mark a gameplay feature complete after its Blueprint compiles, the asset is saved, and relevant PIE evidence exists. Planned work alone is never completion evidence.

## 2026-07-27 Independent Projectile Blueprints

- Created and saved `/Game/Blueprints/Magic/BP_Fireball`.
  - Actor Blueprint with `CollisionSphere` root (18 cm, Query Only, OverlapAllDynamic).
  - `ProjectileMovement`: 1200 cm/s, zero gravity, rotation follows velocity.
  - `ProjectileFX`: `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1`.
  - `Damage=25`, `InitialLifeSpan=4.0`.
  - Actor Begin Overlap calls `HandleProjectileOverlap`.
  - Valid `BPI_Damage2` targets receive one ApplyDamage call, then Hit1 spawns and the projectile destroys.
- Created and saved `/Game/Blueprints/Weapons/BP_SwordWave`.
  - Actor Blueprint with `CollisionSphere` root (35 cm, Query Only, OverlapAllDynamic).
  - `ProjectileMovement`: 900 cm/s, zero gravity, rotation follows velocity.
  - `SlashFX`: `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Slash`.
  - `Damage=20`, `InitialLifeSpan=2.5`.
  - `DamagedActors` array uses Contains/AddUnique to prevent repeat damage per actor.
  - Valid targets receive ApplyDamage and Hit2.
- Both Blueprints compiled with warnings treated as errors and saved with dirty=false.
- PIE validation was attempted with temporary unsaved instances in `L_Test`. SceneTools did not expose the temporary projectile instances in the PIE world, so movement, collision damage, Niagara hit visibility, and lifetime destruction could not be conclusively verified. DamageDummy health remained 100 during the Fireball attempt.
- No temporary projectile actor remained after PIE, and `L_Test` was not saved.
- No current Blueprint Runtime Error or Accessed None was found. One historical authoring-time graph lookup error remains in the session log from before the graph was created.
- Input spawning, left-hand charge, sword trail, sword-wave spawning, and sword integration remain blocked because `BP_XRPawn` and `BP_Sword` existing graph wiring cannot be inspected safely.
- `BP_XRPawn`, `BP_Sword`, enemies, `BPI_Damage2`, and maps were not saved by this task.

## 2026-07-27 Fireball Spawn Transform and Size Fix

- Inspected the existing saved fireball SpawnActor wiring in `/Game/XRFramework/Blueprints/BP_XRPawn` before editing.
- The previous SpawnTransform used the left-aim world location at the component origin.
- Added `FireballSpawnPoint` as a child of `MotionControllerLeftAim` with relative location `(25, 0, 0)`, rotation `(0, 0, 0)`, and scale `(1, 1, 1)`.
- SpawnActor now receives `FireballSpawnPoint.GetWorldTransform`. This is equivalent to LeftAim World Location + LeftAim Forward Vector × 25, with LeftAim World Rotation.
- `BP_Fireball.ProjectileFX` was already using `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Attack2` at uniform relative scale `(0.5, 0.5, 0.5)`; preserved without modifying the Marketplace Niagara system.
- Reduced `BP_Fireball.CollisionSphere` radius from 18 cm to 14 cm.
- Preserved Projectile Movement (1500 cm/s, zero gravity, rotation follows velocity), Damage=25, lifespan=4, BPI_Damage2, Hit effect, duplicate protection, and destruction logic.
- `BP_XRPawn` and `BP_Fireball` compiled with warnings treated as errors and saved dirty=false.
- L_Test PIE spawned `BP_XRPawn_C_0`; runtime `FireballSpawnPoint` was confirmed under `MotionControllerLeftAim` with the expected 25 cm offset. No Blueprint Runtime Error or Accessed None occurred.
- Physical left-controller firing could not be injected through MCP, so repeated visual placement, direction, consistent scale, and travel require VR Preview/device confirmation.
