# Project Status

Last live audit: 2026-07-28 (Unreal MCP, editor state read-only)

## Environment

- Project: `C:\UnrealProjects\dy\dy.uproject`
- Engine association: Unreal Engine 5.8
- Current map: `/Game/Maps/L_Test`
- GameMode: `/Game/XRFramework/Blueprints/BP_XRGameMode`
- Default Pawn: `/Game/XRFramework/Blueprints/BP_XRPawn`
- PIE: stopped
- Unreal MCP: connected and responsive
- Unreal dirty state: audited gameplay assets and `/Game/Maps/L_Test` report `dirty=false`; Git nevertheless reports `Content/Maps/L_Test.umap` modified on disk.
- Current log: no Blueprint Runtime Error, Accessed None, Compile Error, or Broken Reference match. `ABP_Goblin` has two compiler warnings because its state-machine entry is not connected. OpenXR also reports unavailable controller/eye-gaze extensions.

## Systems

| System | Status | Exact evidence / missing proof |
|---|---|---|
| Damage System / `BPI_Damage2` | Verified Complete | `/Game/Blueprints/Interfaces/BPI_Damage2` exists and is saved; prior saved compile plus L_Test PIE damage-dummy 100→50→destroy evidence is retained. |
| Player health | Verified Complete | `BP_XRPawn` saved; prior compile and L_Test PIE evidence records 100→50→0 without pawn destruction. |
| Player death | Verified Complete | Saved `BP_XRPawn` compile and PIE evidence: `CurrentHealth=0` sets `IsDead=true` once and gates player gameplay actions. Physical Quest input confirmation remains. |
| Restart | Verified Complete | One non-looping 2-second timer calls `RestartCurrentLevel`; PIE logs prove actual L_Test reloads and final PIE starts at health 100 with `IsDead=false`. |
| Left-stick movement | Verified Complete | Saved graph and prior PIE movement evidence; physical Quest input remains unverified. |
| Right-stick turning | Verified Complete | `IA_Turn` mappings were structurally removed; physical controller confirmation remains. |
| HMD/controller tracking | Not Verified | XR hierarchy exists and prior PIE spawned the pawn; physical OpenXR tracking was not audited on a headset. |
| Sword attachment | Verified Complete | Prior PIE observed equipped sword on spawned pawn. |
| Sword collision | Fixed, PIE unverified | Root cause found 2026-07-28: `BP_Enemy`/`BP_Goblin` `CollisionCylinder` used the stock `Pawn` profile, whose WorldDynamic response is Block; `SwordCollision` is object type WorldDynamic, so the pair always resolved to Block and `OnComponentBeginOverlap(SwordCollision)` never fired. Added an explicit `WorldDynamic:Overlap` response override on both capsules (`ObjectTools.set_properties` on `bodyInstance.collisionResponses`), compiled and saved. Needs a real headset swing to confirm. |
| Sword speed | Verified Complete | `UpdateSwordSpeed` self pin on `GetActorLocation` confirmed still connected; `SwordSpeed = Distance(loc, PreviousLocation)/DeltaSeconds` each Tick, threshold `MinimumDamageSpeed=100`. Logic was never the problem — the collision gate above was. |
| Sword damage | Fixed, PIE unverified | `TrySwordDamage` branch (`InRange(SwordSpeed, MinimumDamageSpeed, 1000000)`) → `ApplyDamage(OtherActor, SwordDamage=15)` confirmed correctly wired end-to-end; only blocked by the collision-response bug above, now fixed. |
| Sword Trail | Not Started | No verified Niagara trail component/activation. |
| Sword Wave | In Progress | `/Game/Blueprints/Weapons/BP_SwordWave` exists, saved and previously compiled; launch integration and damage PIE evidence are absent. |
| Left-hand magic Aura | Not Started | No verified charge Aura integration. |
| Fireball input/spawn | In Progress | `BP_XRPawn` has saved `FireballSpawnPoint` and SpawnActor wiring; physical button firing is not verified. |
| `BP_Fireball` movement | In Progress | Saved ProjectileMovement at 1500 cm/s and lifespan 4; runtime travel/expiry is not conclusively verified. |
| Fireball collision/damage | Fixed, PIE unverified | Root cause found 2026-07-28: `HandleProjectileOverlap` never destroyed the projectile or guarded against re-triggering, despite an unused `HitProcessed` bool already existing — one fireball could fly through and damage every enemy in its path (matches "everything disappears" report). Added `NOT(HitProcessed)` guard branch plus `SetHitProcessed(true)` + `DestroyActor(self)` after a successful hit. Compiled/saved. |
| Fireball hit effect | In Progress | Saved hit-effect wiring exists; runtime visual proof is absent. |
| Enemy HP bar | Fixed (2 bugs), PIE unverified | Bug 1 (billboarding, fixed earlier 2026-07-28): World-space `HealthBarWidget` inherited the rotating character mesh's orientation; fixed via `bAbsoluteRotation=true` + `FindLookAtRotation`/`SetWorldRotation`. Bug 2 (found later same day, more severe): `WBP_EnemyHealthBar.SetHealthPercent` took no parameters and instead tried to self-discover its owner via `Self→GetOuterObject→CastToWidgetComponent→GetOwner`, which always failed (widget's Outer is not the WidgetComponent since it's manually created in `BeginPlay`, not auto-instantiated) — the bar was permanently stuck at its design-time default of 1.0 regardless of damage. Fixed by giving `SetHealthPercent` a direct `Percent` float param (just calls `HealthProgressBar.SetPercent`), removing all the broken cast/lookup logic and the now-redundant 0.2s self-polling timer in its `EventGraph`, and having `BP_Enemy`/`BP_Goblin.HandleDamage` compute `CurrentHealth/MaxHealth` via `SafeDivide` and pass it directly. Also enlarged `DrawSize` 70×10 → 110×16 for readability. All three assets compiled/saved. |
| `BP_Enemy` health/death | Fixed, PIE unverified | Saved 60/60 health and damage/death logic; damage path was blocked upstream by the sword-collision bug (fixed) and the health-bar percent bug (fixed above) — both now reachable. Death no longer calls `DestroyActor` immediately: it stops movement, disables capsule collision, plays `axe_dead1` (non-looping), then `SetLifeSpan(4.0)` so the corpse lies visible for 4s before auto-destroying (`OnDestroyed`-based wave counting still fires normally). |
| `BP_Enemy` AI/animation | Fixed, PIE unverified | Root cause found 2026-07-28 for the "runs at player then snaps back, repeats forever" report: `ChaseTick`'s NavMesh `SimpleMoveToLocation` goal used the VR pawn's raw Z, which didn't match the NavMesh projection height and caused the goal to oscillate every tick. Goal is now `MakeVector(targetX, targetY, selfZ)` (X/Y follow target, Z stays at the enemy's own height) in both `BP_Enemy` and `BP_Goblin`. Compiled. Navigation being disabled in L_Test (noted below) still needs resolving for this to work in-level. |
| `BP_Goblin` health/death | Fixed, PIE unverified | Saved 40/40 health/death logic; same upstream fixes as `BP_Enemy` apply. Death no longer calls `DestroyActor` immediately: stops movement, disables capsule collision, then ragdolls via the existing `PHYS_swampgoblin_PhysicsAsset` (`SetCollisionProfileName(Ragdoll)` + `SetSimulatePhysics(true)` on the mesh — no dedicated death animation exists for this skeleton, so physics fall is used instead), then `SetLifeSpan(4.0)`. |
| `BP_Goblin` AI/animation | Broken | Prior PIE showed movement/run pose, but current log has two `ABP_Goblin` disconnected state-machine compiler warnings. Blueprint currently forces `ThirdPersonRun`, so the asset warning remains unresolved. |
| Orc assets/animations | In Progress | Mesh, Skeleton, and sequences exist; no Orc AnimBP or Montage was found. Enemy directly plays sequences. |
| Wave Manager | Not Verified | `/Game/Blueprints/Systems/BP_WaveManager` exists and is saved, but no compile/PIE behavior evidence was established in this audit. |
| Boss | Not Started | No `/Game/**/BP_Boss` asset found; no two-pattern boss evidence. |
| HUD | Verified Complete | Saved `/Game/UI/WBP_PlayerHUD` is attached once to `BP_XRPawn.MotionControllerLeftGrip`; BeginPlay and post-damage event paths update the green bar and `Current / Max` text. PIE exercised health `100→55`. Physical Quest readability remains. |
| Victory/defeat | Not Started | No verified flow. |
| Quest build | Not Verified | Android packaging/toolchain not tested. |
| Quest device test | Not Started | No physical-device evidence. |

## Important assets

- Pawn: `/Game/XRFramework/Blueprints/BP_XRPawn`
- Sword: `/Game/Blueprints/Weapons/BP_Sword`
- Fireball: `/Game/Blueprints/Magic/BP_Fireball`
- Sword Wave: `/Game/Blueprints/Weapons/BP_SwordWave`
- Enemy: `/Game/Blueprints/Enemies/BP_Enemy`
- Goblin: `/Game/Blueprints/Enemies/BP_Goblin`
- Damage Dummy: `/Game/Blueprints/Enemies/BP_DamageDummy`
- Damage interface: `/Game/Blueprints/Interfaces/BPI_Damage2`
- Enemy HP widget: `/Game/UI/WBP_EnemyHealthBar`
- Wave Manager: `/Game/Blueprints/Systems/BP_WaveManager`
- Maps: `/Game/Maps/L_Test`, `/Game/Maps/L_Dungeon`, `/Game/Maps/L_Arena`, `/Game/Maps/Sublevels/L_Dungeon_Gameplay`
- Connected Niagara: Fireball uses `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Attack2` and Hit1; Sword Wave uses Slash and Hit2. Aura and Projectile1 exist but are not verified as currently connected.
- Orc mesh: `/Game/Orc/Mesh/SK_Orc_brown`
- Orc Skeleton: `/Game/Orc/Mesh/SK_Orc_all_Skeleton`
- Orc AnimBP: Not Found
- Orc sequences currently used: `/Game/Orc/Animations/axe_run`, `/Game/Orc/Animations/axe_crit1`
- Orc Montages: Not Found

## Blockers

- P0: Combat cannot yet be called playable: successful Sword/Fireball damage, HP-bar reduction, and enemy death have no current end-to-end PIE evidence.
- P1: No boss or victory/defeat presentation flow.
- P1 (partially stale as of 2026-07-28): `L_Test` now contains `NavMeshBoundsVolume_1` and `RecastNavMesh-Default` (added during enemy chase-AI work), so the earlier "no NavMesh asset found" claim is outdated. Whether the mesh actually generates walkable surface under real headset movement is still unverified.
- P1: `ABP_Goblin` emits two disconnected state-machine compiler warnings.
- P1: `Content/Maps/L_Test.umap` is modified in Git although the loaded asset reports clean; do not overwrite it without reconciling ownership.
- P1: Quest/Android packaging and device input/tracking are unverified; OpenXR logs missing controller and eye-gaze extensions in the desktop session.
- P2: `dy-sy/` is an untracked nested repository and there is no `.gitmodules`; prior “mode 160000 gitlink” documentation was stale.
- P2: Git LFS status identifies the modified map as `LFS: 4381282 -> File: a91e5d5`; full `lfs fsck` did not finish within the audit command timeout.

## Recommended next task

Implement only the player-owned left-hand charge Aura first. Preserve the completed HUD and automatic restart flow, and do not modify teammate-owned enemy or boss assets.

## Completed minimal-player-HUD task record

Created `/Game/UI/WBP_PlayerHUD` with only a green health bar and `Current / Max` text. Added one world-space `PlayerHUDWidget` under `BP_XRPawn.MotionControllerLeftGrip` and an event-driven `UpdatePlayerHUD` call at BeginPlay and directly after player health changes. Both assets compile with warnings as errors and are saved. L_Test PIE exercised the damage path from health `100` to `55` without Blueprint runtime errors. L_Test and teammate-owned assets were not saved. Wrist readability/orientation still requires Quest verification.

## Completed automatic-restart task record

Implement Automatic Restart on Player Death using `/Game/XRFramework/Blueprints/BP_XRPawn`. Reuse the existing player-health path, enter death exactly once at zero health, block movement/combat, start one short timer, and automatically reload the current level so the game returns to its initial state without restart input. Compile/save only player-owned assets and verify death→automatic level restart→full starting health in L_Test PIE without saving the map. Enemy, Goblin, enemy HP-bar, AI, animation, Wave Manager, and boss assets are teammate-owned and must not be modified.
# Compact Player HUD update — 2026-07-28

Status: In progress, not yet saved or runtime-verified.

- GUI Parts asset inspection: complete.
- Compact HUD visual layout in connected editor: assembled.
- Existing health HUD reuse: complete.
- Three charge variables: added in the editor.
- Warning proximity/blink wiring: pending.
- Independent charge bar wiring: pending.
- Minimap capture and one-press toggle: pending.
- Asset-scoped save: blocked (`save_assets` returned false).
- L_Test PIE: not run.
