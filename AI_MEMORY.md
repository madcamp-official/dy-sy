# AI Memory

## 2026-07-30 Independent Orc idle patrol and attack-animation change

- Updated only `/Game/Blueprints/Enemies/BP_Enemy`; the three WaveManager Orc spawns still use the same class and their existing mesh/color assignment, combat values, sensing, damage, hit reaction, death, and wave logic remain unchanged.
- Implemented the previously empty `IdlePatrolStep` and `RefreshIdleAnimation` functions so every spawned Orc runs its own timer-driven idle patrol:
  - BeginPlay still starts with looping `/Game/Orc/Animations/axe_IDLE1_1`.
  - Each instance starts its first patrol after an independent random `0.5-3.0s` delay.
  - While no player target is valid, it chooses its own reachable NavMesh point within `350cm` of its recorded spawn origin, moves there with `SimpleMoveToLocation`, and loops `/Game/Orc/Animations/axe_walk1`.
  - After a random `2.5-5.0s` movement window it stops, returns to looping `axe_IDLE1_1`, waits a random `1.5-4.0s`, then chooses another point.
  - Added `WanderOrigin` and `WanderRadius` (`350`) so patrol does not drift progressively away from each Orc's spawn area.
  - Both patrol functions stop changing movement/animation when `bIsDead` is true. While a combat `TargetActor` is valid, they only retry later and do not override the existing chase/attack animation logic.
- Changed the attack animation in `BeginAttack` from `/Game/Orc/Animations/axe_crit1` to `/Game/Orc/Animations/axe_hit1`, non-looping. Attack timing and damage remain `AttackWindupSeconds=1.0`, `HitDelaySeconds=0.5`, `RecoverySeconds=1.0`.
- Verified the BeginPlay splice preserves the original downstream `bUseControllerRotationYaw` and all later initialization; verified the patrol execution chains and animation/timer pins after editing.
- `BP_Enemy` compiled with warnings treated as errors and saved successfully.
- Ran `/Game/Maps/L_Test` in PIE and found no `Blueprint Runtime Error`, `Accessed None`, `Broken Reference`, or `Compile Error`; restored `/Game/Maps/L_Dungeon`.
- The three Orcs spawn only after the prior wave is cleared, so their simultaneous visual separation still requires playing through to the Orc wave in VR.

## 2026-07-30 Sword reach extension and enemy hit knockback reinforcement

- Extended `/Game/Blueprints/Weapons/BP_Sword.SwordCollision` farther toward the blade tip while preserving the handle-side boundary: `BoxExtent.Z 85 -> 100` and `RelativeLocation.Z 35.8 -> 50.8`. The collision box is now 200 cm long, adding approximately 30 cm of tip-side reach.
- Rechecked the live `HandleDamage` graphs for `/Game/Blueprints/Enemies/BP_Enemy`, `BP_Goblin`, and `BP_Boss`.
- Reinforced surviving-hit knockback so it is visually noticeable:
  - Orc (`BP_Enemy`): attacker-to-enemy launch magnitude `350 -> 650`.
  - Goblin (`BP_Goblin`): backward XY launch `180 -> 600`; vertical launch `180 -> 120` so the reaction reads more as retreat than a hop.
  - Boss (`BP_Boss`): attacker-to-boss launch magnitude `450 -> 550`.
- Explicitly connected Self to the `LaunchCharacter` target on `BP_Enemy` and `BP_Boss`; those target pins were previously left implicit/unconnected. `BP_Goblin` already had an explicit Character target.
- Re-read the affected nodes after editing and confirmed the damage reaction execution chains still continue through `LaunchCharacter` into the existing `EndHit` timer; no prior exec connection was replaced.
- Compiled `BP_Sword`, `BP_Enemy`, `BP_Goblin`, and `BP_Boss` with warnings treated as errors and saved all four assets.
- Ran `/Game/Maps/L_Test` in PIE and found no `Blueprint Runtime Error`, `Accessed None`, `Broken Reference`, or `Compile Error`; restored `/Game/Maps/L_Dungeon` afterward.
- Actual sword contact and knockback feel still require a VR playtest because the connected MCP cannot swing the motion controller or inject a live damage-interface call.

## 2026-07-30 Right-stick turn removal and L_Dungeon directional-light optimization

- Removed all four `IA_Turn` mappings from `/Game/XRFramework/Input/IMC_Default`: Valve Index right thumbstick X, Oculus Touch right thumbstick X, Vive right trackpad X, and PICO right thumbstick X. Left-stick movement and all other mappings remain unchanged.
- Verified the mapping context now contains zero mappings targeting `/Game/XRFramework/Input/Actions/IA_Turn`.
- Inspected `/Game/Maps/L_Dungeon` live through Unreal MCP: 661 total actors; `LightSource` is a DirectionalLight whose component was `Movable`, with `DynamicShadowDistanceMovableLight=20000` and 3 cascades. The level also contains `SkyLight_1`, contrary to the earlier claim that it had only one light.
- Changed `LightSource.LightComponent0` mobility from `Movable` to `Stationary`, set `DynamicShadowDistanceStationaryLight=0`, and reduced dynamic shadow cascades from 3 to 1. Saved `IMC_Default` and `L_Dungeon`.
- Project renderer configuration already has Lumen GI/reflections, Virtual Shadow Maps, ray tracing, and mesh distance fields disabled; static lighting is allowed.
- Did not merge prop actors: the project rule forbids deleting existing actors, while merging without removing sources would duplicate visible geometry and add render cost. MCP has `merge_actors` but no safe in-place conversion of existing actors to ISM/HISM.
- Did not run Build Lighting because the connected Unreal MCP exposes no lighting-build operation. The Stationary light is ready for a manual lighting build; until then Unreal may report unbuilt lighting.
- No Blueprint graph was modified, so Blueprint compile was not applicable.
- Ran `/Game/Maps/L_Test` in PIE and found no `Blueprint Runtime Error`, `Accessed None`, `Broken Reference`, or `Compile Error`; restored `/Game/Maps/L_Dungeon` afterward.

## 2026-07-30 VR Preview severe latency / GPU-timeout fix

- Diagnosed the current Unreal/OpenXR session before changing settings.
- PC VR Preview was rendering at `4128x2272` on an RTX 4050 Laptop GPU while Lumen GI, ray tracing, Virtual Shadow Maps, Substrate, mesh distance fields, and ray-tracing proxies were enabled together.
- The editor log confirmed a 5-second D3D12 GPU timeout during VR Preview. The stalled frame's breadcrumbs included `RayTracingGeometry` and GPU skin-cache work, making the rendering path the primary cause.
- Updated `Config/DefaultEngine.ini` for the existing Quest-oriented forward-rendering path: disabled dynamic GI/reflections, ray tracing and proxies, Virtual Shadow Maps, mesh distance fields, and Substrate.
- Kept Forward Shading, Mobile HDR off, Instanced Stereo, and Mobile Multi-View enabled.
- No Blueprint was modified, so Blueprint compile was not applicable.
- A full Unreal Editor restart is required before validating VR Preview. If motion-to-photon delay remains after stable frame rate is restored, diagnose Quest Link/Air Link transport separately.

## 2026-07-30 VR Preview latency follow-up after renderer restart

- Confirmed from the new editor log that the renderer change took effect: `Ray tracing is disabled` and profiler metadata reports `raytracing="0"`.
- The earlier 5-second GPU timeout did not recur in the new VR Preview session.
- VR Preview still allocated a large `4128x2272` stereo buffer.
- D3D12 logged several PSO creation waits of 100-200 ms. These explain intermittent hitches during first-time shader/material use, but not a steady motion-to-photon delay.
- Build/executable size itself does not create continuous head/controller tracking latency. It can affect installation, startup, loading, storage pressure, and runtime streaming only when assets are loaded.
- If tracking is continuously delayed even while the PC preview window appears smooth, the next primary suspect is Quest Link/Air Link transport. If both the PC preview and headset stutter, reduce PC VR render resolution/pixel density and profile frame timing.

## 2026-07-30 VR view appears as a head-following rectangular window - diagnosis only

- User reported that the scene was not naturally immersive around 360 degrees; instead it looked like a laptop-sized window in front, with the visible window shifting as they turned their head.
- No settings were changed in this diagnostic task.
- Latest log proves Unreal did start an OpenXR VR Preview rather than ordinary desktop PIE:
  - `XR: Instanced Stereo Rendering is Enabled`
  - OpenXR initialized on Oculus runtime `1.205.0`
  - VR render buffer resized to `4128x2272`
  - preview window identified as `PC D3D SM6 OpenXR Oculus`
- Therefore this is not caused by package/executable size or by a normal camera lacking a 360-degree projection.
- Most likely categories:
  1. The user is still viewing the Quest Link desktop panel rather than the active Unreal immersive application.
  2. The Oculus compositor is repeatedly presenting/reprojecting a stale Unreal frame because frames are not arriving reliably; this can look like a rectangular image that follows head direction or exposes borders.
- The log does not show OpenXR initialization failure. A headset-side observation is required to distinguish the Quest desktop-panel UI (visible panel border/controller UI) from stale-frame compositor behavior (game image itself freezes/warps and reveals dark borders).

## 2026-07-30 Minimal VR Preview resolution reduction

- User confirmed the second diagnosis case: the game image itself updates as delayed rectangular fragments when turning their head, consistent with stale-frame compositor reprojection caused by severely late frame delivery.
- Applied only one rendering change: added `vr.PixelDensity=0.7` under `[/Script/Engine.RendererSettings]` in `Config/DefaultEngine.ini`.
- Kept OpenXR, D3D12, Instanced Stereo, Forward Shading, and all gameplay/Blueprint logic unchanged.
- No Blueprint was modified, so compile was not applicable.
- Requires a full Unreal Editor restart. The next VR Preview log should show a render buffer materially smaller than the prior `4128x2272`; headset behavior must be tested after restart.

## 2026-07-30 Ray tracing / foveation configuration audit

- Rechecked the external claim that ray tracing was enabled and foveation was disabled.
- Ray tracing is already fully disabled in the current project: `r.RayTracing=False`, ray-tracing proxies are disabled, and the latest runtime log explicitly says `Ray tracing is disabled` with profiler metadata `raytracing=0`.
- `PointLight.RayTracing=True` and similar entries in `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` are editor visibility/show-flag preferences, not an override that re-enables the renderer while `r.RayTracing=0`.
- No `bEnableRayTracing` or `foveationLevel` setting exists in the project configuration.
- Foveated rendering is not active. The current Oculus OpenXR PC runtime reports `XR_FB_foveation`, `XR_FB_foveation_configuration`, and the Vulkan foveation extension as unavailable.
- The project uses Epic's OpenXR plugin and does not have a Meta XR plugin entry. Enabling a new plugin was not attempted because project rules prohibit adding plugins arbitrarily.
- The recently added `vr.PixelDensity=0.7` is a uniform resolution reduction, not foveated rendering. At early config load it was reported as a deferred/dummy CVar; runtime render-buffer dimensions after a new VR Preview are still needed to prove that the late-registered OpenXR CVar consumed the value.

## 2026-07-30 Right-stick turn vs physical head-turn diagnosis

- Diagnosis only; no input mapping or Blueprint was changed.
- Unreal MCP was unavailable during the check, so conclusions use the latest saved project memory/config evidence rather than a fresh live graph read.
- The latest documented state restored right-controller `IA_Turn` X-axis mappings in `/Game/XRFramework/Input/IMC_Default`.
- `BP_XRPawn` uses the existing `IA_Turn -> SnapTurn` path. It is snap turn, not interpolated smooth turn, and it does not directly rotate the Camera or override HMD tracking. Physical HMD head tracking is intended to remain independent.
- However, project-wide `DefaultInput.ini` configures Oculus Touch right-thumbstick X/Y with `DeadZone=0.0`. This leaves hardware drift unfiltered at the legacy axis-config layer. Whether the Enhanced Input action/mapping itself supplies its own dead-zone or threshold could not be freshly verified without MCP.
- Conclusion: snap-turn logic is not a plausible cause of constant renderer/compositor delay, but an overly sensitive/no-dead-zone right stick can cause unintended rig snap rotations and make physical turning feel discontinuous. This can explain why removing the turn mapping previously felt more natural, separately from the rectangular stale-frame reprojection issue.

## 2026-07-29 Boss still faced left after bCanStrafe fix — real cause was a missing mesh yaw offset (correction to the entry below)

User playtested the previous round's fixes: boss STILL always faced left while running (bCanStrafe=false alone did not fix it), Orc telegraph was better but wanted more range.

- **Orc**: `AttackRange` 400→**550** (SenseRadius=1500 still comfortably covers it).
- **Boss facing — actual root cause found**: swept every remaining `BP_Boss` function (`UserConstructionScript`, `BeginAttack`, `TryResumeChase`, `EndRecovery`, `EndHit`, `DealAttackDamage`, `DealSlamDamage`, `IdleFlourish`, `PlayApproachAnimation` — the ones not checked in the previous round) and confirmed **zero** `SetActorRotation`/`SetWorldRotation` calls exist anywhere in the Blueprint. So it was never a logic bug. Got a reliable component reference via `ActorTools.get_components(actor, component_type=SkeletalMeshComponent)` (works for native/C++ components like `CharacterMesh0` — this is a cleaner alternative to the CDO-direct-path / `_GEN_VARIABLE` addressing dance documented in [[unreal_mcp_dsl_technique]], at least for non-SCS components) and compared against the reference `/Game/ParagonRampage/Characters/Heroes/Rampage/RampagePlayerCharacter`'s own `CharacterMesh0`: the reference has `RelativeRotation.Yaw=270` (i.e. -90°) — **`BP_Boss`'s was `Yaw=0`**. A constant 90° mesh-vs-capsule misalignment is exactly what reads as "always faces left regardless of travel direction" (a movement-orientation bug would just make the character not turn at all, not turn 90° wrong consistently). Fixed by setting `BP_Boss.CharacterMesh0.RelativeRotation = (0, 270, 0)` to match the reference. **This corrects my own note in the entry below, written earlier the same session, which dismissed the mesh-rotation-offset theory as a "red herring" — that dismissal was itself wrong. The `bCanStrafe=true` fix was probably still a real, separate bug worth having fixed, but it was not sufficient on its own; the missing 270° yaw offset was the dominant cause.** Lesson: when a reference/template Blueprint of the same skeleton exists in the project, compare its component defaults directly instead of guessing — `ActorTools.get_components` + `ObjectTools.get_properties` is the reliable path for that, no addressing quirks.
## 2026-07-29 Sword collision reach extension

- Confirmed `BP_Sword.SwordCollision` was a `20 x 12 x 110 cm` box (`BoxExtent 10,6,55`) that almost exactly matched the sword mesh bounds, leaving no extra hit-range allowance beyond the visible blade.
- Expanded `SwordCollision` to `30 x 20 x 140 cm` (`BoxExtent 15,10,70`).
- Shifted its local Z center from `5.8` to `20.8 cm`, preserving the handle-side boundary while extending the tip-side hit range by approximately `30 cm`.
- Kept `MinimumDamageSpeed=60`, overlap settings, damage, and hit-effect logic unchanged.
- `BP_Sword` compiled with warnings treated as errors and saved successfully. A spawned PIE sword confirmed the new extent/location and overlap events enabled. No new sword-related runtime error was found; existing unrelated Goblin AnimBP and enemy timer warnings remain in the log.

## 2026-07-29 Five reported goblin/orc/boss bugs — real root causes found via graph tracing (not repeat guesses)

User reported the same categories of problems again after earlier "fixes" this session, explicitly asking for a confident, verified report. Traced every graph fully this time instead of re-guessing, and found each issue had a genuine, previously-undiagnosed root cause:

- **Goblins still clustered despite `WanderRadius` raised twice already (900→1300→1800)**: root cause was never the radius. `BP_WaveManager`'s 8 goblin spawn points (in its `EventGraph`, 8 `SpawnActorFromClass`→`MakeTransform`→`MakeVector` chains) were hardcoded literals packed into a ~950×700 unit area, 3 of them only 150–220 units apart. Since each goblin's `WanderOrigin` is presumably its own spawn point, a huge shared `WanderRadius` just meant every goblin's wander circle fully overlapped every other one's, regardless of size. Fixed by respacing all 8 `MakeVector` literals into a 4×2 grid (min pairwise spacing 850 units) spanning most of `L_Arena`'s actual `NavMeshBoundsVolume` bounds (queried via `ActorTools.get_actor_bounds`: X[-980,2020], Y[-1100,1100] — confirmed before moving anything, since exceeding NavMesh bounds silently breaks pathing per [[boss_fsm_no_ai_needed]]). Also **reduced** `BP_Goblin.WanderRadius` 1800→400, since with origins now 850+ apart, the old huge radius would have made them drift back into each other's zones — spacing and radius have to be tuned together, not radius alone.
- **Orc attack still not visible soon enough despite `AttackRange` raised 150→250 last round**: no logic bug, just needed to go further. Raised `AttackRange` 250→400 and `AttackWindupSeconds` 0.7→1.0s (confirmed `SenseRadius`=1500 still comfortably exceeds the new range, so the orc won't lose the target before reaching attack distance).
- **Boss fire still landing near itself despite last round's redesign to `GetRandomReachablePointInRadius`**: found the actual bug — the `Origin` input of that node was wired to `GetActorLocation(self=SelfPawn)`, i.e. **the boss's own position**, not the player's. It was never player-directed at all; "700 unit radius around itself" is exactly what it was doing. Fixed by adding a new `GetActorLocation` node fed by `GetPlayerPawn()` and rewiring only the `Origin` pin to it (left the fireball's spawn-transform `Location` and `FindLookAtRotation.Start` alone — those correctly still use the boss's own location as the throw origin).
- **Boss faces player only at first, then always faces left while running**: root-caused to a regression from *this session's own earlier fix*. When `ChaseTick` was upgraded from `SimpleMoveToLocation` to full `AIController::MoveToLocation` (to get `AcceptanceRadius`), the new node's `bCanStrafe` pin was left at its default `true`, which lets the pawn move toward its destination without necessarily rotating to face the travel direction — so the capsule/mesh just kept whatever rotation it had (from spawn or the last explicit facing) while sliding toward the player. Confirmed by comparing against the Orc, which never got this upgrade (still uses `SimpleMoveToLocation`, which has no such parameter) and has no facing complaints. Fixed by setting `bCanStrafe=false` on the Boss's `MoveToLocation` node. **Note for future sessions: the earlier `CharacterMesh0.RelativeRotation` guess from 2 sessions ago (documented in [[boss_fsm_no_ai_needed]] as unconfirmed) was a red herring — traced every graph this round and found no `SetActorRotation`/`SetWorldRotation` call targets the boss's mesh or capsule anywhere (the one `SetWorldRotation` call in `BeginPlay` billboards the `HealthBarWidget` to the camera, unrelated). Don't re-guess the mesh rotation offset for boss facing issues — check AI movement `bCanStrafe`/`bOrientRotationToMovement` first.**
- **Boss doesn't die no matter how much it's attacked**: `HandleDamage`'s death branch was traced node-by-node and found fully intact and structurally correct (health≤0 → `State=Dead`, `bIsDead=true`, `OnBossDeath` delegate, collision disabled, movement disabled, `Death_A` animation, `SetLifeSpan`, `OnEnemyDefeated` on the player). Not a logic bug — a balance bug. Confirmed via a subagent trace of `/Game/Blueprints/Weapons/BP_Sword.TrySwordDamage` that the player's sword deals a **flat 15 damage/hit** (`SwordDamage` variable, no scaling). Boss `MaxHealth` was 350 → ~24 hits to kill, which reads as "never dies" in a real fight. Reduced `MaxHealth`/`CurrentHealth` 350→180 (~12 hits, in line with Orc's own 15-damage attacks for comparison).

All five fixes compiled clean and were saved (`save_assets` on `BP_Boss`, `BP_Enemy`, `BP_Goblin`, `BP_WaveManager`). **Still not verified live in PIE** — same standing tooling limitation as every prior round (boss is wave-spawned only, editor-placed actors don't run `BeginPlay`, `add_to_scene_from_class` is blocked during PIE). Everything above was verified by full graph tracing (`get_connected_subgraph` from each relevant event/function entry point, cross-checked against sibling Blueprints like the Orc for comparison), which is a stronger verification method than earlier rounds' partial/spot-check tracing, but is still not the same as an actual playtest.
## 2026-07-29 Valid sword-hit Niagara effect

- Confirmed `/Game/Knife_light/VFX/NE_attack05` exists and is a `NiagaraSystem`.
- Updated `/Game/Blueprints/Weapons/BP_Sword.TrySwordDamage` so the hit effect is only eligible after the existing player exclusion, owner exclusion, and minimum sword-speed check pass.
- Added `DoesObjectImplementInterface(BPI_Damage2)` before the existing `ApplyDamage` message. Non-damageable props and unrelated overlap actors now exit without receiving damage or spawning the effect.
- On the valid interface branch, `ApplyDamage` runs first and then `SpawnSystemAtLocation(NE_attack05)` runs once at `OtherActor.GetActorLocation()`.
- Player overlaps still exit at the existing `CastToBP_XRPawn` guard, so the new Niagara effect cannot trigger from hitting the player pawn.
- `BP_Sword` compiled with warnings treated as errors and saved successfully. L_Test PIE reported no `Blueprint Runtime Error`, `Accessed None`, or `Broken Reference`. Physical sword-hit timing and the final effect scale/orientation still require VR/Quest visual confirmation.

## 2026-07-29 Robust sword self-damage prevention

- The earlier `OtherActor != GetOwner()` guard in `/Game/Blueprints/Weapons/BP_Sword.TrySwordDamage` was not sufficient because the ChildActor sword has no explicit runtime `SetOwner` setup and its live `Instigator` is `None`.
- Added an explicit `CastToBP_XRPawn` at the start of `TrySwordDamage`.
- If `OtherActor` is the player pawn, the successful cast execution path intentionally ends immediately, before owner comparison, speed threshold, or `ApplyDamage`.
- Only `CastFailed` continues into the existing owner check and sword-speed damage path, so enemies and other valid non-player targets retain the original behavior.
- `BP_Sword` compiled with warnings treated as errors and saved successfully. L_Test PIE reported no `Blueprint Runtime Error`, `Accessed None`, or `Broken Reference`.

## 2026-07-29 HUD flush frame and reversed gray-overlay cooldown

- Resized both `button_frame` widgets from 92x92 to 80x80 and moved them to the exact same coordinates as their 80x80 cooldown icons. Raised each frame slot to ZOrder 4 so the border overlays the icon instead of leaving a visible gap around it.
- Rebuilt the cooldown visual as a receding gray overlay to guarantee the requested direction:
  - ProgressBar background is now the original full-color icon.
  - ProgressBar fill image is the dark gray-tinted icon.
  - Fill direction is `TopToBottom`.
  - `WBP_PlayerHUD.SetMagicCharges` now maps each incoming cooldown progress from `0..1` to visual Percent `1..0`.
- Result: immediately after using a skill the icon is fully gray; as cooldown progress increases, the gray overlay retreats upward and reveals the original color from bottom to top. At cooldown completion the full icon is original color.
- Fill animation remains disabled, so the texture itself does not scroll.
- `WBP_PlayerHUD` compiled and saved successfully. L_Test PIE reported no `Blueprint Runtime Error`, `Accessed None`, or `Broken Reference`.

## 2026-07-29 Boss combat overhaul: fireball retaliation removed + redesigned as ambient ground hazard, wall-facing bug fixed via proper AIController MoveTo, attack-range/telegraph tuning, full idle/approach/attack animation variety system

- **Removed the instant retaliation fireball**: `BP_Boss.HandleDamage`'s non-lethal branch no longer calls `CastFireball()` on every hit taken (deleted the node). Combined with the previous session's fix making fireball damage collision-based, this was the main source of "melee always gets punished" — now melee is just melee, no auto-counter.
- **Fireball redesigned as a 20s-repeating ambient hazard, not a player-seeking attack**: `BeginPlay`'s `CastFireball` timer changed from one-shot `Time=1.0` to `bLooping=true, Time=20.0`. `CastFireball` itself no longer aims at the player's location — replaced the player-location `FindLookAtRotation` target with `GetRandomReachablePointInRadius(bossLocation, 700)`, so the boss periodically lobs fire at a random nearby NavMesh point (same ground-fire-on-impact and 10-damage-on-contact behavior from last session still applies if the player happens to be standing there).
- **Boss "occasionally runs facing a wall" root-caused further and fixed**: `ChaseTick`'s "still chasing" branch used `SimpleMoveToLocation` (a fire-and-forget helper with an effectively-zero acceptance radius, i.e. it tries to walk to the player's *exact* origin point every 0.15s). Replaced with the full `AIController::MoveTo` node (`AI|내비게이션|MoveToLocation`, needs a `CastToAIController` on `GetController()`'s return since that node's `self` pin requires `AIController` specifically, not the generic `Controller` type `GetController` returns) with `AcceptanceRadius=250` — the AI now naturally stops well short of the player instead of pathing into their exact position (and whatever's behind them, e.g. a wall). Also added a `StopMovement` call the instant `ChaseTick` transitions to `AttackWindup` (mirrors the fix already applied to Orc last session) so the boss fully halts before its swing animation plays, instead of coasting into melee/geometry mid-windup.
- **Attack range increased for both, purely to make the windup/swing visible from farther away** (`DealAttackDamage`'s hit-timer logic is unchanged — only *when the windup starts* moved out, not when the hit lands): Boss `AttackRange` 300→**420**, Orc `AttackRange` 150→**250**.
- **Boss idle-variety system** (new function `IdleFlourish`, self-rescheduling via `SetTimerByFunctionName` every `RandomFloatInRange(6,12)`s, started once at the end of `BeginPlay`): while `State=="Idle"` (checked via `GetState→ToString(Name)→EqualExactly(String) "Idle"` — **`Equal(Name)` is not a valid `create_node` type_id, confirmed again this session**; converting to String first and using `유틸리티|스트링|EqualExactly(String)` is the reliable workaround, now used in two places), picks one of the 7 user-specified animations at random via a `Utilities|선택` (`K2Node_Select`) node fed by `RandomIntegerInRange(0,6)` — **found that a generic wildcard `Select` node resolves its pin type correctly once you connect its `ReturnValue` to a concretely-typed consumer pin (here, `PlayAnimation`'s `NewAnimToPlay`) *before* setting the `Option N` pin literals; connecting `Index` to an Integer output is what unlocks `add_node_pin` to grow past the default 2 options** — plays it with `bLooping=true` uniformly (simplification: didn't attempt per-animation "play once then revert" timing since exact clip lengths aren't known; a few of the 7 — e.g. `Emote_Master_Roar_T3`, `Ability_GroundSmash_End` — will look repetitive if their multi-second flourish loops for the full idle window, but nothing breaks). If not idle, no animation plays but the cycle still reschedules itself.
- **Chase/approach animation swap**: new function `PlayApproachAnimation` (`GetDistanceTo(SelfPawn, TargetActor) > 750` → `Sprint_Biped_Fwd`, else `Jog_Biped_Fwd`, both looping) replaces the old hardcoded `Run_Fwd` `PlayAnimation` calls at all three chase-(re)start sites: `OnSenseBeginOverlap`, `EndRecovery`, `TryResumeChase`. Boss now sprints in from far away and downshifts to a jog as it closes in.
- **Attack animation variety**: `BeginAttack`'s existing 65%-melee/35%-slam `RandomBoolWithWeight` roll now has a second nested 50/50 `RandomBoolWithWeight` inside the melee branch, picking between `Attack_Melee_A` (existing) and the newly-added `Attack_Biped_Melee_B` — both feed into the same existing melee-trail VFX + `DealAttackDamage` timer, so damage/timing logic is untouched, only which swing animation plays.
- **Goblin wander radius** 1300→**1800** (speed left alone, per explicit instruction not to touch the user's own speed-scale tuning).
- All changes compiled clean (no new `LogBlueprint` warnings/errors) and were saved. **None of this was re-verified live in PIE** — boss is still only reachable via wave progression, not level-placed, so the usual caveat applies: ask for an in-game check, especially of (1) whether the wall-facing issue is actually gone now, (2) whether the idle-flourish animations look acceptable being looped rather than played-once, (3) whether 20s is a good cadence for the ambient fireball.

## 2026-07-29 Boss fireball damage made collision-based (was guaranteed-hit regardless of contact); goblin wander widened; boss detection re-audited

- **Goblin wander radius** 1300→**1800** (speed left unchanged per request).
- **Fireball damage was never actually tied to the fireball touching the player** — root-caused and fixed. `BP_Boss.CastFireball` spawned the projectile then immediately armed `SetTimerByFunctionName("DealFireballDamage", FireballTravelDelay)`, and `DealFireballDamage` just did `GetPlayerPawn(0) → ApplyDamage(15)` unconditionally — a guaranteed hit on a fixed timer, completely decoupled from `BP_BossFireball`'s actual flight/collision (this was flagged as a known decoupling in an earlier session's memory entry, but not treated as a bug until now). Fixed:
  - Deleted the `SetTimerByFunctionName("DealFireballDamage", ...)` node from `CastFireball` — no more guaranteed auto-hit. `DealFireballDamage` function itself left in place but now unreferenced/dead (harmless).
  - Wired real contact damage into `BP_BossFireball.OnProjectileStop` (already reliably fires on any blocking hit — `FireballMesh`'s collision profile is `BlockAllDynamic`, which blocks Pawns too, so this event already fires when the fireball physically touches the player, not just terrain): `CastToBP_XRPawn(HitActor)` → on success, `ApplyDamage(10)` via the `BPI_Damage2` interface message, then either path (cast success or fail) continues into the existing `SpawnSystemAtLocation(NS_Fire_Floor_01)` + `DestroyActor` — so ground-fire-on-impact behavior is unchanged, only *who* takes damage and *when* changed (now: only on genuine player contact, for a flat 10, instead of always, for 15).
  - **Found but did not touch**: `BP_Boss.HandleDamage`'s non-lethal branch also calls `CastFireball()` directly and unconditionally every time the boss takes a non-fatal hit (a "retaliate when struck" mechanic, separate from the proximity-based chase/melee FSM). Combined with the guaranteed-damage bug above, this meant *every single melee hit the player landed* provoked an unavoidable 15-damage counter-fireball — very likely the real reason the boss "felt like all it does is cast fire": approaching to melee was reliably punished regardless of positioning. Now that fireball damage requires actual contact, this retaliation mechanic is much fairer; left as-is since removing it wasn't requested and it's a reasonable design (boss punishes melee unless you dodge the retaliatory fireball).
- **Re-audited whether the boss detects/chases/melees the player** (user reported it "just stands and casts fire" even when approached, asked directly whether detection/aggro works at all): re-verified the entire `BeginPlay` exec chain end-to-end via `get_connected_subgraph` — confirmed **not broken**, the intro-roar `Delay` from the earlier animation-overhaul session correctly continues through to `SetTimerByFunctionName("CheckPlayerDistance", 0.2, looping)` and the one-shot `CastFireball` timer; `CheckPlayerDistance` → `OnSenseBeginOverlap` → `ChaseTick` → `BeginAttack` chain (traced in exhaustive detail in earlier sessions this same day) is structurally sound and should engage the moment the player is within `SenseRadius=1500`. **Could not confirm live in PIE** (boss is wave-spawned, still no instance reachable without playing through the waves) — the counter-fireball explanation above is the most concrete lead found; if the boss still doesn't visibly chase/melee after this fix, the next suspects are (in order): (1) whether `AutoPossessAI` actually results in a valid `AIController` for a `SpawnActorFromClass`-spawned actor at the specific point `BP_WaveManager` spawns it, (2) whether the boss's spawn point in `L_Arena` is actually inside the `NavMeshBoundsVolume` bounds.

## 2026-07-29 Orc (BP_Enemy) size/speed/attack-clarity tuning — confirmed same detect/chase/attack FSM as Boss/Goblin

- Confirmed `BP_Enemy` (the Orc) already has the identical hand-built FSM as Boss/Goblin (`SenseRadius=1500`, `AttackRange=150`, full `ChaseTick`/`BeginAttack`/`DealAttackDamage` chain) — it does detect and attack the player, nothing was missing structurally.
- **Size**: `CollisionCylinder.RelativeScale3D` 0.8→**1.6** (was previously *smaller* than Goblin's 1.3 — now sits between Goblin (1.3) and Boss (net visual ~2.2)). At base `CapsuleRadius=34`, 1.6x gives effective radius ~54, in the same safe range as Goblin's already-working ~55 and under Boss's ~59 — no NavMesh decoupling needed here (see the Boss nav-mismatch entry above for why that matters).
- **Speed**: `CharMoveComp.MaxWalkSpeed` 400→**340** (user: "약간" reduce chase speed).
- **Naturalness**: `bOrientRotationToMovement` false→**true** — same bug class as Goblin (wasn't turning to face its movement direction).
- **Attack motion "안 드러난다" root cause**: `ChaseTick`'s AttackWindup branch (like Boss's) never called `StopMovement` when transitioning out of chase — the last-issued `SimpleMoveToLocation` kept the Orc coasting forward through its entire windup + swing animation, smearing the attack into a slide. Added a `StopMovement(Controller)` call right after `SetState(AttackWindup)` (reusing the branch's existing `GetController` pure node) so the Orc actually plants and holds still while it winds up and swings — should make `axe_crit1` read as a clear, deliberate attack instead of a moving smear. **This same gap likely exists in Boss's and Goblin's `ChaseTick` too** (not yet fixed there) — worth applying the identical `StopMovement`-on-AttackWindup fix if a similar "attack is hard to notice" complaint comes up for them.
- Compiled clean, saved. Not re-verified in PIE.

## 2026-07-29 Boss "runs in place facing a wall" root-caused to NavMesh agent-size mismatch (2.2x capsule scale from an earlier session); goblin speed/naturalness tuning

- **Root cause of "보스가 등장 후 벽 보면서 제자리 러닝만 한다"**: earlier this same day, `BP_Boss.CollisionCylinder.RelativeScale3D` was bumped 1.0→2.2 (to make the boss look bigger). That capsule has `CharMoveComp.NavAgentProps.AgentRadius/AgentHeight = -1/-1` (auto-derive from the capsule), so at 2.2x scale the boss's effective nav agent became ~92 radius / ~418 height. The arena level (`/Game/Maps/L_Arena`, confirmed via its `NavMeshBoundsVolume`/`RecastNavMesh-Default`) only has NavMesh built for the project's one supported agent: **AgentRadius=35, AgentHeight=144**. A ~92-radius agent doesn't fit that NavMesh, so `SimpleMoveToLocation` in `ChaseTick` can't produce a usable path — the boss plays `Run_Fwd` (correctly, per the earlier animation fix) but never actually translates, and visually reads as running in place against a wall. Note: `/Game/Maps/L_Dungeon` (the default-loaded level) and its `L_Dungeon_Gameplay` sublevel have **no NavMeshBoundsVolume at all** — the real arena/combat level is `L_Arena`, not `L_Dungeon`; check there first for any future nav-related boss/enemy debugging.
- **Fix applied (decouples visual size from collision/pathing size)**: `CollisionCylinder.RelativeScale3D` 2.2→**1.4** (effective radius ~59, much closer to what the built NavMesh supports — matches the working Goblin's ~55 effective radius). To keep the boss looking exactly as big as before, `CharacterMesh0.RelativeScale3D` (previously 1.0, inheriting 100% of capsule scale for its visual size) bumped to **1.571** (1.4×1.571≈2.2, same net visual size). Also explicitly set `CharMoveComp.NavAgentProps` to `AgentRadius=40, AgentHeight=150` (close to the built 35/144) instead of leaving it at -1/-1 auto-derive, so nav queries reliably match the level's only registered NavData regardless of future capsule-scale tweaks.
- **Lesson for future scale-up requests on any Character-based enemy**: never scale `CollisionCylinder`/capsule alone for "make it bigger" — that changes the nav agent footprint too. Scale the mesh component instead (or scale both capsule and mesh but keep the capsule's effective radius under ~40-50 given this project's NavMesh is built for AgentRadius 35) and set `NavAgentProps` explicitly rather than relying on -1 auto-derive once a capsule is scaled non-trivially.
- **Not yet re-verified in PIE** (same tooling constraint as before — boss is wave-spawned, not level-placed). If the boss still doesn't chase/attack properly after this, the next things to check: (1) whether `L_Arena`'s NavMesh is actually built/up-to-date (stale nav data), (2) whether `BP_WaveManager`'s `SpawnActorFromClass` spawn point is even inside the `NavMeshBoundsVolume`'s bounds, (3) whether the AIController is actually possessing the spawned boss (`GetController()` validity) — ruled out `SpawnActorFromClass` itself as a cause (it's the standard node, doesn't bypass `AutoPossessAI`).
- **Goblin tuning**: `CharMoveComp.MaxWalkSpeed` 150→110 (user: "too fast"), `bOrientRotationToMovement` false→**true** (the goblin was never actually turning to face its movement direction while wandering/chasing — likely the real source of "unnatural" movement, not just speed; matches how Boss already had this set to true). `WanderRadius` 900→1300 (user: widen further, "more natural" motion — wander logic itself, `IdlePatrolStep`/`WanderPause`, was already reasonable: pick random NavMesh-reachable point, walk 3-6s or until `WanderPause` cuts it off, idle-pause 1.5-3.5s, repeat).
## 2026-07-29 HUD icon fit and static cooldown fill

- Enlarged `MagicSlot1ChargeBar` and `MagicSlot2ChargeBar` in `/Game/UI/WBP_PlayerHUD` from 60x60 to 80x80 and re-centered them with a 6 px inset inside each 92x92 `button_frame`.
- Updated both ProgressBar background/fill brush image sizes to 80x80 so the skill artwork fills the frame interior instead of appearing as a small square with wide empty margins.
- Disabled `WidgetStyle.EnableFillAnimation` on both cooldown ProgressBars. This removes the internal vertical scrolling/panning effect while preserving the static `BottomToTop` cooldown reveal driven by Percent.
- `WBP_PlayerHUD` compiled successfully. The updated widget asset is present on disk, and the running PIE session had no `Blueprint Runtime Error`, `Accessed None`, or `Broken Reference`.

## 2026-07-29 HUD button frame and cooldown icon execution fix

- Replaced `MagicSlot1Frame` and `MagicSlot2Frame` in `/Game/UI/WBP_PlayerHUD` from `lil_roundframe_ready2` to the existing project asset `/Game/GuiParts/UiElements/button_frame`.
- Found the actual reason the cooldown icons never changed: `WBP_PlayerHUD.SetMagicCharges` had all of its data pins connected, but the function entry and every setter/`SetPercent` node had no execution wiring. Calls to the function therefore performed no work even though the pawn-side 0.25-second timer and percentage calculations were correctly connected.
- Wired the full execution chain: function entry -> store slot 1 charge -> slot 1 `SetPercent` -> store slot 2 charge -> slot 2 `SetPercent` -> store slot 3 charge -> slot 3 `SetPercent`.
- The first two progress bars retain their dark background icon, full-color fill icon, and `BottomToTop` fill direction. Using a skill now resets its displayed percent to 0 and the 30-second timer progressively restores color from bottom to top.
- `WBP_PlayerHUD` compiled and saved successfully; `BP_XRPawn` also compiled with warnings treated as errors.
- L_Test PIE started and stopped with no `Blueprint Runtime Error`, `Accessed None`, or `Broken Reference`. Physical skill activation and final appearance still require VR/Quest visual confirmation.

## 2026-07-29 Sword owner self-damage exclusion

- Updated `/Game/Blueprints/Weapons/BP_Sword.TrySwordDamage` so damage processing only continues when `OtherActor != GetOwner()`.
- When the sword overlaps its owning player, the new owner-check branch ends without running the existing speed check or `ApplyDamage` interface call.
- Enemy damage behavior is unchanged: non-owner actors still pass through the original sword-speed threshold and damage path.
- `BP_Sword` compiled with warnings treated as errors and saved successfully.
- L_Test PIE started and stopped successfully with no `Blueprint Runtime Error`, `Accessed None`, or `Broken Reference` logged. A physical sword-to-player overlap still requires VR/Quest confirmation.

## 2026-07-29 Boss animation overhaul: fixed "crawling" bug (wrong-skeleton anim), added entrance roar + hit-react, confirmed no BT/AIController needed

- **Root cause of "보스가 기어다니는" complaint found and fixed**: `BP_Boss.EndRecovery` was calling `PlayAnimation(Mesh, /Game/Orc/Animations/axe_run.axe_run, looping=true)` — an **Orc-skeleton** run animation played on the Rampage-skeleton boss mesh. This fires every single time the boss finishes an attack's recovery window and resumes chasing (i.e. constantly during combat), producing a broken/incompatible retarget pose that reads as crawling. Almost certainly a copy-paste leftover from when BP_Boss was cloned from BP_Enemy (the Orc). Fixed by repointing that pin to `Run_Fwd` (Rampage's own run cycle), same asset `OnSenseBeginOverlap` already correctly uses when chase first begins.
- **BP_Boss already has a full hand-built combat FSM** — no AIController/BehaviorTree exists or is needed. `State` name variable drives Idle → Chase (via `SimpleMoveToLocation` off a stock `AIController`, `AutoPossessAI=PlacedInWorldOrSpawned`) → AttackWindup → Attack (65% `Attack_Melee_A` swing / 35% `Ability_GroundSmash_Start` slam, chosen by `RandomBoolWithWeight`) → Recovery → back to Chase, plus Hit/HitStun and Dead states, an Enrage threshold at 50% HP (`Ability_Enrage_Start`, 0.8x attack-speed multiplier), and an independent 1s-repeating `CastFireball` ranged timer. All of this already existed; the "does this need AI" question resolved to "no — it's a bug in one animation reference, not an architecture gap."
- **Added boss entrance sequence** in `EventGraph` (BeginPlay): now plays `SelectScreen_Emote` (non-looping, ~4.47s roar) → `Delay(4.47)` → `PlayAnimation(Idle, looping)` before the rest of BeginPlay continues (widget setup, `CheckPlayerDistance`/`CastFireball` timers). Practical side effect: the boss no longer starts sensing/attacking until its roar finishes, which reads as intentional.
- **Added hit-react animation**: `HandleDamage`'s non-lethal branch previously set `State=Hit` and started the `EndHit` timer with zero animation feedback (mesh just froze mid-whatever-it-was-doing). Inserted `PlayAnimation(HitReact_Front, non-looping)` between the state-set and the timer.
- **Added run animation on resume-from-hitstun**: `TryResumeChase` (called by both `EndHit` and `EndRecovery`) set `State=Chase` and restarted the `ChaseTick` timer but never replayed `Run_Fwd`, unlike the initial `OnSenseBeginOverlap` chase-start path. Added a `GetMesh`+`PlayAnimation(Run_Fwd, looping)` between the state-set and timer-restart for parity.
- Confirmed `Death_A` and the enrage animation were already wired correctly in `HandleDamage` — no bug there, don't re-touch.
- **Found and fixed two more bugs in `DealAttackDamage` while confirming the melee loop end-to-end** (user asked "is 1:1 melee really fully wired"): (1) the post-hit `LaunchCharacter` knockback had its `self`/target pin left unconnected, which in Blueprint silently defaults to the Blueprint's own `self` — meaning the **boss was knocking itself back**, not the player, on every landed hit. (2) `ApplyDamage` and `LaunchCharacter` were on **mutually exclusive branches** of an `IfThenElse` gated on `IsVisible(GetComponentByClass(TargetActor, ParticleSystemComponent))` — a nonsensical condition, so on any given hit either damage OR knockback fired, never both. Rewired to a straight sequential chain: SpawnEmitter → ApplyDamage → (CastToCharacter on TargetActor) → LaunchCharacter if it's a Character → SetState(Recovery) → SetTimer(EndRecovery), with the CastFailed branch also routed into SetState(Recovery) so the state machine can't get stuck.
- **Important discovered constraint**: `/Game/XRFramework/Blueprints/BP_XRPawn` (the player) extends `Pawn`, **not** `Character` — `LaunchCharacter` can never apply to it. So the boss's melee knockback is currently a no-op against the real player (cast fails safely, recovery still proceeds); it would only fire against a Character-based target. If real player knockback is wanted later, it needs a different mechanism (e.g. an interface call/`AddActorWorldOffset` tween) since VR pawns generally shouldn't be given raw physics launches anyway (motion sickness).
- Compiled clean (no new `LogBlueprint` warnings) and saved. **Not visually re-verified in PIE** — no `BP_Boss` instance was placed in the currently loaded level (it's spawned dynamically by `BP_WaveManager` on wave progression, not level-placed), and the standing per-project tooling limits still apply ([[unreal_mcp_dsl_technique]]-adjacent: `add_to_scene_from_class` is blocked during PIE, and editor-placed actors never run `BeginPlay`). User should confirm the fix by playing to the boss wave.

## 2026-07-29 Goblin wander/size tuning, Boss scale+rotation, fireball heat-smoke removal, dead-enemy double-fire guard

- **Goblin wander radius** 400→900, **goblin scale** 1.0→1.3 (`CollisionCylinder.RelativeScale3D`) — both direct CDO property bumps, no graph changes. Confirmed live in PIE (`WanderRadius=900`, `scale=1.3` on a running instance).
- **Boss scale** 1.0→2.2 on `CollisionCylinder` (user explicitly said okay to go oversized).
- **Boss "spawns lying down" — partially unresolved, see caveat below.** `CharacterMesh0.RelativeRotation` was `(pitch=0, yaw=-90, roll=90)`. Compared against `BP_Goblin`/`BP_Enemy` (both `SK_swampgoblin`/`SK_Orc_brown`, both display correctly), which both use `(0, -90, 0)` — tried that first, still looked wrong (floating/sideways) in a screenshot. **Root cause of why that screenshot was misleading**: `BP_Boss` uses `/Game/ParagonRampage/Characters/Heroes/Rampage` — a non-humanoid gargoyle/wyvern creature, not a retargeted-mannequin biped like the goblin/orc — and critically, its BeginPlay has **no `SetAnimInstanceClass` call**, only a raw `PlayAnimation(.../Rampage/Animations/Idle, looping=true)`. An actor placed in the editor via `add_to_scene_from_class` never runs `BeginPlay` (that only fires in PIE/game), so every screenshot taken of a non-PIE-placed `BP_Boss` shows the **unanimated bind pose**, not the actual in-game look — proven by pulling `CaptureAssetImage` on the `Idle` animation asset directly, which renders a normal-looking grounded creature, while the exact same skeleton with `(0,-90,0)` *or* `(0,0,0)` on the component both looked broken/floating as a bind pose in viewport screenshots. **Landed on `RelativeRotation=(0,0,0)`** as the best inference (thumbnail generation for the animation asset itself uses component-identity transform and looks correct), but this is a reasoned guess, not a confirmed fix — `add_to_scene_from_class` is also blocked while PIE is running ("Cannot create actors while PIE is active"), and reaching the real boss spawn requires clearing waves 1+2 first, so **this could not be verified with the actual Idle animation playing**. Flag clearly to user: check the boss's in-game standing pose specifically; if still wrong, the fix is almost certainly still in `CharacterMesh0.RelativeRotation` on `BP_Boss`, try `(0,-90,0)` (goblin/orc convention) or examine the Rampage skeleton's root bone axis directly via SkeletalMeshTools rather than more screenshot-guessing.
- **Boss fireball blue "smoke" = heat-distortion emitter, not smoke**: `NS_Fire_Medium` (the Niagara system used for the flying fireball) has 6 emitters (`NE_Flame_01/02`, `NE_Ashes`, `NE_Lights`, `NE_Heat`, `NE_Sparkles`) — no smoke emitter exists. `NE_Heat` renders with `MI_VFX_HeatDistortion_Strong`, a heat-haze/refraction material — almost certainly what read as "blue smoke". Disabled `NE_Heat` via `SetEmitterData(bIsEnabled=false)` directly on the shared Niagara asset (checked `get_referencers` first — only the asset pack's own unused demo map and `BP_BossFireball` reference it, so safe to edit in place rather than duplicating).
- **"Enemy Defeated" banner re-fire investigation**: confirmed each of `BP_Goblin`/`BP_Enemy`/`BP_Boss`'s `HandleDamage` calls `CallOnEnemyDefeated` exactly once inside the `Dead` branch (Boss also separately calls its own pre-existing `CallOnBossDeath` dispatcher — unrelated, not a duplicate). **However found a real re-fire bug**: the `Dead` branch is gated only by `CurrentHealth <= 0`, with no check for "already dead" — since dead bodies keep ragdoll-physics collision active for their `SetLifeSpan(4.0)` window (mesh gets `SetSimulatePhysics(true)` + `Ragdoll` collision profile, not `NoCollision`), further hits during that window could re-run the entire Dead branch (re-broadcast `CallOnEnemyDefeated`, re-trigger ragdoll setup, reset the destroy timer) every time the corpse is hit again — very plausible in VR sword combat swinging through a falling body. **Fix**: added a `bIsDead` bool to `BP_Enemy` and `BP_Boss` (mirroring the one already on `BP_Goblin` from the wander-guard work), each `Dead` branch now sets it `true` right after `SetState(Dead)`, and `HandleDamage` in all three now opens with `Branch(GetIsDead)` — `then` (already dead) dead-ends immediately, `else` (not yet dead) proceeds into the original first node. This makes a single kill provably fire the banner exactly once, uniformly across all three enemy types.
- All five touched blueprints (`BP_Goblin`, `BP_Enemy`, `BP_Boss`, plus the `NS_Fire_Medium` Niagara asset) compiled/saved cleanly, no new warnings. `L_Test` PIE run showed no `Blueprint Runtime Error`/`Accessed None`; the only log errors present were pre-existing/unrelated (`LogHttpListener`/`LogModelContextProtocol` — this session's earlier transient MCP disconnect, already resolved, not a project issue).
- **Tooling note**: `add_to_scene_from_class` cannot be used while PIE is running ("Cannot create actors while PIE is active") — for visually verifying something that only looks right once `BeginPlay`-driven animation/setup has run, there is currently no clean way to spawn a fresh, fully-initialized actor into a live PIE session via these tools; either ride out the actual gameplay trigger (wave progression, etc.) or accept the bind-pose-only limitation of editor-placed actors.

## 2026-07-29 Fixed: game-flow banners never appeared in VR (ShowBanner/HideBanner had no exec wiring)

- User reported the "Game Start"/"Enemy Defeated"/"Game Over"/"Victory" banner system (added in an earlier session, commit `4a62e36`) never shows up when testing in VR in `L_Test`. First confirmed the work wasn't lost — `4a62e36` is a real ancestor of current HEAD (`git merge-base --is-ancestor`), and `WBP_PlayerHUD`/`BP_XRPawn` still have `ShowBanner`/`HideBanner`/`BannerContainer`/the 3 event dispatchers live in the project. So the scaffolding survived; the bug was internal to it.
- **Root cause, found by reading `ShowBanner`'s actual node graph (not trusting the old memory description of what it "does")**: the function's `K2Node_FunctionEntry_0.then` exec pin had **zero connections** — none of its nodes (`SetText` on `BannerTextBlock`, `SetTimerByFunctionName(HideBanner, HideDelay)`) were wired into an execution chain at all, they just sat there as an unconnected pile only linked by data pins (`Message`→`ToText`→`SetText.InText`, `HideDelay`→`SetTimer.Time`). Critically, **there was no `SetVisibility` call anywhere in `ShowBanner`** — the container defaults to `Collapsed` and nothing ever flipped it to `Visible`. `HideBanner` was worse: completely empty, `Entry.then` connected to nothing. So banners could never physically appear regardless of how correctly everything upstream (the event dispatcher bindings, the 4 call sites in `EventGraph`) was wired — and it was: `Construct`→`GetOwningPlayerPawn`→`CastToBP_XRPawn`→3×`AssignOnX`→`ShowBanner("Game Start", 2.5)`, and each of `OnEnemyDefeated`/`OnGameOver`/`OnVictory` custom events correctly call `ShowBanner` with the right message/delay, all confirmed still fully connected. The break was isolated entirely inside the two banner functions themselves.
- This is the same class of bug as the `write_graph_dsl` silent-partial-graph issue documented earlier today for `BP_Goblin` — strong signal the original banner session used the same DSL path and never re-verified with `get_connected_subgraph` afterward. **Lesson reinforced**: after building ANY function graph (whether via DSL or otherwise), always re-open it with `get_connected_subgraph` from the entry node before considering it done — a clean `compile_blueprint` does NOT catch "nodes exist but aren't chained," since disconnected-but-otherwise-valid nodes are not a compile error, only unreachable/no-op code.
- **Fix**: added `Variables|WBP_PlayerHUD|GetBannerContainer` + `위젯|SetVisibility` (`InVisibility=Visible`) to `ShowBanner`, wired `Entry.then → SetVisibility → SetText(existing node) → SetTimerByFunctionName(existing node)`. Added the same `GetBannerContainer`+`SetVisibility(Collapsed)` pair to `HideBanner`, wired `Entry.then → SetVisibility`. `WBP_PlayerHUD` compiled with no new warnings/errors and was saved.
- User separately downloaded an "Easy Text Animation — FREE" asset in case it was needed as a fallback — **not used**, since the actual bug was a simple missing wiring/visibility call in existing pure-UMG code, not a limitation of the animation approach. No new assets required.
- **PIE-verified structurally** (`L_Test`): confirmed via `git`/live inspection that all supporting pieces (dispatchers, widget tree, call sites) were intact, and that the fixed functions compile clean with no `Blueprint Runtime Error`/`Accessed None`. **Did not confirm the banner is visually readable in-headset** — `WidgetComponent.Widget`/the live `UUserWidget` instance isn't exposed as a readable "property" through the available tooling (only `WidgetClass` is), so the actual on-screen/in-HMD appearance (position, size, readability at 55cm/1280×720 world-space canvas) still needs the user's own VR check, same caveat as the boss-fireball item above.

## 2026-07-29 Goblin speed/avoidance/wave-spawn + box-guard behavior + Boss fireball rework

- **Goblin walk speed**: `BP_Goblin.CharMoveComp.MaxWalkSpeed` 400 → 150 (400 was left over from the old chase-speed tuning; now that goblins only wander, it looked too fast).
- **Goblin-goblin avoidance**: enabled `CharMoveComp.bUseRVOAvoidance=true` (+ `avoidanceWeight=0.5`, `avoidanceConsiderationRadius=300`) so wandering goblins steer around each other instead of overlapping — built-in UE RVO, no custom BP logic needed.
- **Wave system now spawns 5 goblins, not 3**: found the real source of "only 3 goblins" — `BP_WaveManager`'s `BeginPlay` hardcodes wave 1 as 3 back-to-back `SpawnActorFromClass(BP_Goblin)` + `BindEventtoOnDestroyed` node pairs (no loop), and `Wave1Count` (used only to seed `RemainingInWave`, the kill-counter that advances to wave 2) was set to match at 3. There were **no manually-placed goblins in `L_Test`** — every goblin the user sees is wave-spawned already, contrary to what session's assumption might suggest; don't trust "placed vs spawned" claims without checking `find_actors` on the non-PIE level first. Added 2 more spawn+bind node pairs (same `Game|SpawnActorfromClass` + `게임|BindEventtoOnDestroyed` pattern, generic node — the class-specific-looking `Game|SpawnActorBPGoblin` label shown by `get_node_infos` on the existing 3 is **not** a valid `create_node` type_id, same class of gotcha as the earlier `GetMesh`/`Variables|디폴트|GetMesh` issue; use `Game|SpawnActorfromClass` and set the `Class` pin explicitly) and bumped `Wave1Count` 3→5.
- **Goblins now guard the box, not open space**: `L_Test` has a plain `StaticMeshActor` named `Cube13_3` at `(1443,0,25)`, bounds ~68×100×50cm, sitting right next to where the goblins spawn — confirmed via `CaptureViewport` screenshot, this is "the box" the user meant they'll later put a collectible spell on. Re-pointed all 5 wave-spawn coordinates in `BP_WaveManager` to a ring around it — `(1200,±250,100)`, `(1650,±250,100)`, `(1750,0,100)` — and lowered `BP_Goblin`'s `WanderRadius` default 700→400 so their random-wander loop (`[[goblin_wander_redesign]]` from the previous session) stays clustered around the box instead of roaming the whole room. Since `WanderOrigin` is already set to each goblin's own spawn location at `BeginPlay`, no `BP_Goblin` graph changes were needed for this — only where `BP_WaveManager` spawns them.
- **Boss fireball visual/impact overhaul** (`BP_BossFireball`): the "sphere" the user meant is literal — `FireballMesh` is `/Engine/BasicShapes/Sphere.Sphere` with a Paragon fire-bubble material (`M_FA_Bubble_Inst`) as the *entire* visual, no particle system was ever involved. Found genuinely fire-themed Niagara systems already in the project under `/Game/Vefects/Free_Fire/Shared/Particles/` (`NS_Fire_Big/Medium/Small` for flying fire, `NS_Fire_Floor_01/02` for fire-on-ground — exactly what was needed, no new assets required):
  - Kept `FireballMesh` (still needed as the collision/`UpdatedComponent` for `ProjectileMovementComponent`) but set `bVisible=false`; added a new `NiagaraComponent` (`FireVFX`, auto-attached to the same root so it moves with the projectile) playing `NS_Fire_Medium` as the actual visible fireball.
  - **Also discovered the fireball previously had zero impact handling at all** — `ActorBeginOverlap` and `Tick` were bound but empty, and `FireballMesh`'s collision profile (`BlockAllDynamic`) blocks rather than overlaps world geometry, so `ActorBeginOverlap` could never have fired anyway; it would have just silently stopped in midair forever. Implemented real impact handling via `ProjectileMovementComponent`'s `OnProjectileStop` delegate (fires reliably since `bShouldBounce=false`): on stop, break the `HitResult` for `ImpactPoint`, spawn `NS_Fire_Floor_01` there (persistent burning-ground visual, `bAutoDestroy=true`), then `DestroyActor` the fireball itself.
  - Set `ProjectileGravityScale` 0→1.0 and reduced `InitialSpeed`/`MaxSpeed` 1400→900 so it visibly arcs down to the ground instead of flying dead straight. Confirmed via reading `BP_Boss.CastFireball` that the boss already spawns/aims the fireball at its own root height toward the player's location (`FindLookAtRotation`, no separate targeting change needed) and that damage (`DealFireballDamage`) is applied on a **separate timer** (`FireballTravelDelay`) completely decoupled from the projectile's actual flight — so changing speed/gravity here has no effect on whether/when damage lands, made this a safe, self-contained change.
  - Tooling note: `EventDispatchers|AssignOnProjectileStop` (the "Assign" variant, not "Bind") is what auto-creates a correctly-signatured Custom Event (`ImpactResult: HitResult`) the first time you bind a given delegate — matches the `AssignOnDestroyed`/`BindEventtoOnDestroyed` split already documented from `BP_WaveManager`. Use Assign once, Bind for any subsequent listeners of the same delegate on other instances.
- All three blueprints (`BP_Goblin`, `BP_WaveManager`, `BP_BossFireball`) compiled with no new warnings/errors and were saved (`save_assets`).
- **Verified live in `L_Test` PIE**: 5 `BP_Goblin` instances spawn (`_C_0`..`_C_4`, up from 3), `WanderOrigin`/`WanderRadius` on the two new ones confirmed centered on the box ring coordinates (one landed at `(1448,153)`, right next to the box at `(1443,0)`), `CharMoveComp.MaxWalkSpeed=150` and `bUseRVOAvoidance=true` both confirmed on a live instance, no `Blueprint Runtime Error`/`Accessed None` in logs. **Not verified visually**: did not trigger an actual boss fireball cast in this session (would need to fight through to the boss wave) — the graph is fully wired and compiles clean, and the logic was traced by hand, but the in-game look of the new Niagara fireball + ground-fire hasn't been eyeballed. Flag this to the user as the one remaining thing to check visually.

## 2026-07-29 Goblin: wall-blocked perception fix + passive wander redesign + size increase

- **Root cause of "runs through walls" bug**: `BP_Goblin`'s `OnSenseBeginOverlap`/`ChaseTick` already had `LineOfSightTo` checks, but the actual trigger path was `EventGraph`'s `CheckPlayerDistance` custom event (looping every 0.2s from `BeginPlay`) calling `OnSenseBeginOverlap` purely on `GetDistanceTo(player) <= SenseRadius` (1500cm) with no line-of-sight gate at the trigger point itself — so a goblin within 15m but behind a wall would still initiate chase the instant LOS briefly cleared or via the distance-only path.
- **User's real ask superseded the LOS fix**: since Goblin is meant to be a non-threatening tutorial-tier enemy, removed the chase/attack trigger entirely rather than just hardening the LOS check. Deleted the `CheckPlayerDistance` branch that called `OnSenseBeginOverlap` (kept the rest of `CheckPlayerDistance`, which also drives the world-space health-bar billboard rotation toward the camera — untouched). `OnSenseBeginOverlap`/`ChaseTick`/`BeginAttack`/`TryResumeChase`/etc. functions were left intact but are now unreferenced/dead code (harmless, no compile warnings from unused BP functions) — safe to reuse later if aggression is ever re-enabled.
- **New passive wander system** (replaces the old empty `IdlePatrolStep` stub): `IdlePatrolStep` and a new `WanderPause` function alternate — `IdlePatrolStep` uses `UNavigationSystemV1::GetRandomReachablePointInRadius(WanderOrigin, WanderRadius)` + `SimpleMoveToLocation`, plays `ThirdPersonWalk` looping, then schedules `WanderPause` after a `RandomFloatInRange(3.0, 6.0)`s; `WanderPause` calls `AIController::StopMovement`, plays `ThirdPersonIdle` looping, then reschedules `IdlePatrolStep` after `RandomFloatInRange(1.5, 3.5)`s. This is genuine NavMesh-based free-roam (not fixed back-and-forth waypoints) — looks AI-driven. `WanderOrigin` (Vector, set to spawn location in `BeginPlay`) and `WanderRadius` (float, default 700, instance-editable) are new BP variables.
- **Guard against interrupting Hit/Death**: added a new bool variable `bIsDead` (not reusing `State=="Dead"` because the DSL/create_node tooling couldn't construct a working Name-equality node — see tooling note below). `HandleDamage`'s death branch now sets `bIsDead=true` right after `SetState(Dead)`. Both wander functions check `GetIsDead` first and `return` immediately (no reschedule) if true, so the wander timer chain cleanly self-terminates on death instead of fighting the ragdoll/`SetSimulatePhysics` logic.
- **Size increase**: `CollisionCylinder` (capsule root) `RelativeScale3D` raised from `0.8` to `1.0` (was already not `0.6667` as an older memory entry claimed — always verify current values, don't trust stale memory). The mesh offset is relative to the capsule and scales proportionally, so feet stayed grounded with no further adjustment needed.
- **Tooling notes for next session**: (1) `write_graph_dsl` silently produced an incomplete graph when a function body was `(if cond (return) (else <10+ nested statements>))` — it created only the outermost condition/branch skeleton with no exec-pin connections and no error, so **always verify with `get_connected_subgraph` from the entry node after any `write_graph_dsl` call**, don't trust a null/success return. Ended up rebuilding both `IdlePatrolStep` and `WanderPause` node-by-node via `create_node`/`connect_pins`/`set_pin_value` instead. (2) Generic `(== a b)` in the DSL fails for `Name`-typed variables (`Utilities|Operators|Equal(==) does not exist`), and the display type_id shown by `get_node_infos`/`get_connected_subgraph` for existing Name-equality nodes (`유틸리티|이름|Equal(Name)`) is also **not** a valid `create_node`/DSL type_id (same class of issue as the earlier documented `GetMesh` bare-name problem) — for Name comparisons, add a dedicated bool variable instead of fighting the tooling. (3) Confirmed again: BP-inherited getter node names strip the `b` prefix from bool variables (`bIsDead` → `Variables|디폴트|GetIsDead`/`SetIsDead`), and `ACharacter::GetMesh` must be created via `Variables|캐릭터|GetMesh`, not `Variables|디폴트|GetMesh`. (4) `get_node_type_pins` confirmed pure (no exec pins): `AI|내비게이션|GetRandomReachablePointinRadius`, `수학|랜덤|RandomFloatInRange` — safe to leave leftover discovery nodes from this call, though deleting them is still tidier.
- `BP_Goblin` compiled with no new warnings/errors and was saved (`save_assets`).
- **Verified live in PIE** (`L_Test`, 3 placed goblins): after ~10s all three goblins remained `State=Idle`, `TargetActor=None` throughout (confirmed no chase ever triggers, walls or not), and were observed actually walking to new positions away from their `WanderOrigin` (e.g. goblin_0 moved (900,-300)→(927,-231)→(759,38) over ~10s) — real NavMesh wander confirmed working, not just compiling. No `Blueprint Runtime Error`/`Accessed None` in logs. `L_Test` was loaded only for this PIE check and was not saved; the editor's previously-open level (`L_Dungeon`) was reloaded afterward to leave editor state as found.

## 2026-07-29 Sword trail removal

- Removed the `SwordTrailFX` Niagara component from `/Game/Blueprints/Weapons/BP_Sword`.
- Removed only the trail-specific Activate/Deactivate graph nodes; sword speed calculation, collision, and damage logic were preserved.
- The source Niagara asset `/Game/VFX/NS_SwordTrail` was not deleted or modified.
- `BP_Sword` compiled with warnings as errors and was saved.
- `L_Test` PIE confirmed the runtime sword has no Niagara component and logged no `Blueprint Runtime Error` or `Accessed None`; `L_Test` was not saved.

## 2026-07-28 Player HUD view coverage adjustment

- Moved `BP_XRPawn.PlayerHUDWidget` from 70 cm to 55 cm in front of the camera.
- Increased its uniform scale from `0.05` to `0.06` and widened the cylindrical arc from `55` to `75` degrees.
- Preserved the centered pivot, 1280x720 draw size, camera attachment, and all internal HUD anchors.
- `BP_XRPawn` compiled with warnings as errors and was saved.
- `L_Test` PIE confirmed the runtime values and logged no `Blueprint Runtime Error` or `Accessed None`; `L_Test` was not saved.

## 2026-07-28 Enemy behavior restoration on latest main

- Restored the enemy-behavior work without replacing the teammate's current `BP_Enemy` or `BP_Goblin`.
- Both enemies now use actor scale `0.6667`, chase speed `400`, slower turning, and approximately two-thirds idle-patrol movement.
- Initial sensing and active chase require controller `LineOfSightTo`; an obstacle clears the target and returns the enemy to Idle.
- Randomized idle/walk animation refreshes prevent groups from moving in sync.
- Added `/Game/UI/BP_DamageNumber`; `HandleDamage` spawns rising red text using the actual received damage.
- `BP_Enemy`, `BP_Goblin`, and `BP_DamageNumber` compiled with warnings as errors and were saved.
- `L_Test` PIE logged no `Blueprint Runtime Error` or `Accessed None`; the level was not saved.
- VR/Quest visual verification remains for animation feel and damage-number readability.

## 2026-07-28 Boss attack VFX/movement + pure-UMG game-flow banners

- Boss (`BP_Boss`): added `SpawnEmitterAtLocation` VFX (ParagonRampage Cascade particles) to `BeginAttack`/`DealAttackDamage`/`DealSlamDamage`; fixed initial movement/rotation snap by setting `bUseControllerRotationYaw=false`, `CharMoveComp.bOrientRotationToMovement=true`, `RotationRate.Yaw=200`, and calling `ChaseTick` immediately from `OnSenseBeginOverlap` instead of waiting for the first timer tick.
- **Asset-pack loss discovered mid-session**: an earlier attempt built 4 UI banners ("Game Start"/"Enemy Defeated"/"Game Over"/"Victory") on top of `EasyCustomButtonsV1/EasyCustomButtons/WBP_BaseButton`. That work lived only on a local `log` branch and was never merged (real Blueprint-asset conflict vs. teammate's `main` work — merge was aborted at user's request). Investigating afterward, the `EasyCustomButtons` subfolder itself turned out to be missing from disk and was never committed on any branch (`git ls-tree --all`, dangling stash inspection) — it is gone for good unless re-imported from Fab. Do not assume `WBP_BaseButton` exists; check before reusing this design.
- **Rebuilt the banner system from scratch on current `main`, using pure native UMG** (no external asset dependency), per explicit user instruction:
  - `WBP_PlayerHUD`: added `BannerContainer` (Overlay, child of `CanvasPanel_0`, anchored top-center, Collapsed by default) containing `BannerBackgroundImage` (Image, brush = `/Game/GuiParts/UiElements/big_background`) and `BannerTextBlock` (TextBlock, centered, white, size 48). Added functions `ShowBanner(Message: string, HideDelay: float)` (sets text, shows container, `SetTimerByFunctionName(HideBanner, HideDelay)`) and `HideBanner()` (collapses container). Construct now calls `ShowBanner("Game Start", 2.5)`.
  - `BP_XRPawn`: added 3 event dispatchers `OnEnemyDefeated`/`OnGameOver`/`OnVictory`. `WBP_PlayerHUD`'s Construct binds all three via `GetOwningPlayerPawn→CastToBP_XRPawn→AssignOnXXX` (each Assign auto-creates its bound Custom Event) → each custom event calls `ShowBanner` with its own message ("Enemy Defeated" 2s / "Game Over" 4s / "Victory" 4s). `BP_XRPawn` broadcasts `CallOnGameOver` right after `SetIsDead(true)` in its own death sequence.
  - `BP_Enemy` / `BP_Goblin` / `BP_Boss`: each `HandleDamage`'s Dead branch now ends with `GetPlayerPawn(0)→CastToBP_XRPawn→CallOnEnemyDefeated` right after the existing `SetLifeSpan`.
  - `BP_WaveManager`: added `GetPlayerPawn(0)→CastToBP_XRPawn→CallOnVictory` immediately after the existing `CallOnAllWavesCleared`.
  - All five Blueprints compiled with no new warnings/errors; PIE run in `L_Test` showed no Accessed-None/runtime errors (only pre-existing, unrelated warnings: `ABP_Goblin` state-machine entry warning, `L_Test` missing RecastNavMesh-for-crowd warning, EOS backend 403s).
- **New technique found this session — `BlueprintTools.read_graph_dsl` / `write_graph_dsl` / `get_graph_dsl_docs`**: an S-expression DSL that builds whole function-graph bodies in one call instead of node-by-node `create_node`/`connect_pins`. Much faster for straight-line logic (used for `ShowBanner`/`HideBanner`). Caveats found: (1) `read_graph_dsl` returns `""` even for graphs that clearly have nodes (e.g. `EventGraph` with existing PreConstruct/Construct/Tick) — don't trust it to reflect prior state; only rely on it for graphs you just wrote yourself, and use `find_nodes`/`get_node_infos` to inspect anything pre-existing. (2) The DSL's automatic `self` binding inside `(fn ...)` tries to create a node of type `Variables|Getareferencetoself`, which does not exist in this Korean-localized editor and errors out — work around it by manually binding `(bind selfRef (Variables|셀프레퍼런스가져오기))` and using `selfRef` instead of the bare `self` keyword. (3) Node type IDs containing literal parentheses (e.g. `위젯|SetText(Text)`) parsed fine as DSL tokens despite the general Lisp-style grammar — no quoting needed.
- Confirmed (again) that a plain `create_node` on a Custom-Event-derived node's exec output is index 1 (`then`), not index 0 (`OutputDelegate`) — mixing these up produces "Could not connect pin OutputDelegate to execute" errors (harmless, node just needs the right pin index).
- **Not committed yet.** This is all new work on top of the current `main` (the previous banner attempt on the abandoned `log` branch never merged, so there is no risk of re-encountering that conflict — this is a clean redo). Recommend a commit once the user has verified banners visually in VR.

## 2026-07-28 Enemy awareness and idle patrol

- Increased `SenseRadius` and the `SenseSphere` radius from 800 cm to 1500 cm on `/Game/Blueprints/Enemies/BP_Enemy` and `/Game/Blueprints/Enemies/BP_Goblin`.
- Added `IdlePatrolStep` to both normal-enemy Blueprints. A one-time Tick initializer starts a 0.25-second looping timer; the Tick path performs no repeated work after initialization.
- While `TargetActor` is invalid, enemies move forward at about 20 cm/s, apply a small random yaw change, and use swept movement so they slowly wander and look around instead of standing still.
- Idle patrol immediately stops contributing movement after a player target is acquired. Existing chase, attack, damage, health, collision, and death logic was preserved.
- Both Blueprints compiled with warnings treated as errors and were saved.
- L_Test PIE ran for five seconds with no new Blueprint Runtime Error or Accessed None. Final movement feel and 15 m acquisition distance should still be checked in VR.

## 2026-07-28 HUD distance and HP GUI correction

- Player HUD camera distance changed from 110 cm to 70 cm.
- HP now uses both GUI Parts `Hp_frame` and `Hp_line`.
- `WBP_PlayerHUD` and `BP_XRPawn` compiled and saved; L_Test was not saved.
- PIE exposed an unrelated existing invalid `BP_Fireball` reference in `HandleProjectileOverlap`; no Fireball or enemy asset was changed.

## 2026-07-28 Compact Player HUD

- `/Game/UI/WBP_PlayerHUD`, `/Game/UI/RT_PlayerMiniMap`, and `/Game/XRFramework/Blueprints/BP_XRPawn` were compiled/saved as applicable.
- HUD includes event-driven HP, triangle-only nearby-enemy warning, three independent charge bars, and a hidden-by-default minimap toggled from `IA_Menu_Toggle_Right.Started`.
- L_Test PIE produced no new Blueprint Runtime Error or Accessed None. L_Test was not saved.
- Next player-owned task: left-hand charge Aura. Do not modify teammate-owned enemy/boss assets.

## 현재 단계
(2026-07-28 세션 추가 #10) 사용자가 세션 #9(Codex)의 조사 결과를 전달하며 "피격 시 감소 로직 추가 + 완전 안 보임 해결 + 죽으면 바닥에 쓰러지게" 요청 → **전부 수정 완료**:
- **사망 연출 추가(원래 즉시 DestroyActor라 요청했던 "바닥에 쓰러져 있기"가 전혀 없었음)**: `HandleDamage`의 사망 분기에서 `DestroyActor` 즉시 호출을 제거하고, 대신 `SetMovementMode(MOVE_None)` → 캡슐 `SetCollisionEnabled(NoCollision)` → (오크: `PlayAnimation(axe_dead1, loop=false)` / 고블린: 스켈레톤에 이미 있던 `PHYS_swampgoblin_PhysicsAsset`를 이용해 `SetCollisionProfileName(Ragdoll)` + `SetSimulatePhysics(true)`로 레그돌 낙하) → `SetLifeSpan(self, 4.0)`(4초 뒤 자동 파괴, WaveManager의 OnDestroyed 카운팅은 그대로 유지됨) 순서로 재구성. 고블린은 전용 사망 애니메이션 에셋이 프로젝트에 없어서(오크는 `axe_dead1`/`axe_dead2` 존재) 레그돌 낙하를 선택함. 양쪽 컴파일 에러 없음, 저장 완료.
- **피격 시 게이지 감소가 항상 실패하던 진짜 근본 원인 확정 및 수정**: 세션 #9와 그보다 더 이전 세션의 기록이 동일하게 지목한 `WBP_EnemyHealthBar.SetHealthPercent` 내부의 `Self→GetOuterObject→CastToWidgetComponent→GetOwner→CastToBP_Enemy/BP_Goblin` 체인을 실제로 열어서 확인함 — 정확히 기록된 그대로 존재했고, 파라미터가 아예 없는 함수라 호출부(`BP_Enemy`/`BP_Goblin.HandleDamage`)에서 무엇을 넘겨도 반영되지 않는 구조였음. **해결책은 ForEachLoop 기반 소유자 탐색이 아니라 훨씬 단순하게 함수 시그니처에 `Percent`(float) 입력 파라미터를 추가하고 내부를 `HealthProgressBar.SetPercent(Percent)` 한 줄로 단순화**(Outer/GetOwner/양쪽 캐스트/자체 CurrentHealth·MaxHealth 조회 전부 삭제). 덩달아 `EventGraph`에 있던 "Construct 시 0.2초 반복 타이머로 자기 자신을 계속 호출"하던 폴링 로직(세션 #2/#3에서 남긴 임시방편)도 이제 완전히 불필요해져서 통째로 삭제함(이벤트 기반 갱신으로 전환, HUD와 동일한 패턴). 호출부인 `BP_Enemy`/`BP_Goblin.HandleDamage`는 `SetHealthPercent` 직전에 `GetMaxHealth`+`SafeDivide(CurrentHealth, MaxHealth)`를 추가해 계산한 값을 넘기도록 수정. 세 블루프린트 모두 컴파일 에러 없음, 저장 완료.
- **완전히 안 보이는 문제 대응**: World-space `HealthBarWidget`의 `DrawSize`가 BeginPlay에서 (70,10)cm로 하드코딩돼 있던 것(세션 #3 툴링 한계 A 우회 — SCS 기본값이 인스턴스에 전파 안 돼서 BeginPlay에서 매번 다시 지정해야 하는 구조)을 (110,16)cm로 확대. 배경/테두리 추가는 이번엔 손대지 않음(UMG 위젯 트리 편집 툴 이름을 이번 세션에서 확정 못함) — 세션 #8에서 이미 고친 빌보드 회전(카메라 항상 바라보기)과 이번 퍼센트 갱신 수정이 맞물리면 실제로는 대부분 해결될 것으로 예상, 그래도 VR에서 여전히 안 보이면 배경 추가가 다음 후보.
- **`BP_Enemy` dirty=true였던 이유**: 이번 세션에서 사망 연출 노드를 만들다가 중간에 끊긴 상태였을 뿐, 사람이나 다른 세션이 건드린 미상의 변경이 아니었음. 지금은 `BP_Enemy`/`BP_Goblin`/`WBP_EnemyHealthBar` 전부 `is_dirty=false`로 확인 완료(저장됨).
- **추가로 발견한 버그: 체력바가 머리 위가 아니라 훨씬 높은 허공에 떠 있었음**: 사용자가 "머리 위에 안 뜨면 어디 뜨는데?"라고 물어서 좌표를 직접 확인함. `HealthBarWidget`의 `RelativeLocation.Z=220`(루트인 캡슐 기준), 캡슐 `CapsuleHalfHeight=88`, 그리고 `CharacterMesh0`의 `RelativeLocation`이 `(0,0,0)`(캡슐 원점에서 오프셋 없음)임을 확인함 — 즉 위젯은 캡슐 중심에서 220cm 위, 캡슐 꼭대기(원점+88)보다도 132cm나 더 위에 떠 있었던 것. RelativeLocation은 (DrawSize/Space와 달리) BeginPlay에서 재설정되는 코드가 없어서 순수 SCS 기본값에만 의존하고 있었는데, 이 값 자체가 애초에 너무 높게 잡혀 있었음. **수정**: BeginPlay에 `SetRelativeLocation(HealthBarWidget, (0,0,110))` 호출을 새로 추가(SetDrawAtDesiredSize 직후, SetbAbsoluteRotation 직전)해서 런타임에 확실히 반영되게 했고, SCS 기본값도 동일하게 110으로 맞춰둠(에디터 프리뷰용, 툴링 한계 A상 인스턴스에 전파 안 될 수 있어 BeginPlay 쪽이 진짜 수정임). 다만 이 110이라는 값도 실제 메시 높이를 정밀 측정해서 낸 값이 아니라 캡슐 절반 높이(88)+여유(22cm) 추정치이므로, 사람이 VR로 보고 여전히 너무 높거나 낮으면 이 Z값을 미세조정할 것.
- **VR 재검증 필요 항목**: 사망 시 실제로 자연스럽게 쓰러지는지(특히 고블린 레그돌이 이상한 자세로 꼬이지 않는지), 게이지가 데미지마다 실시간으로 줄어드는지, 게이지가 이제 실제로 머리 바로 위쯤에 뜨는지(Z=110 추정치 검증) — 전부 사람이 직접 VR로 확인 필요(MCP로는 PIE 중 데미지 주입이 불가능해 구조적 검증까지만 가능).

(2026-07-28 세션 추가 #9) 사용자 리포트: "Claude Code가 적 게이지 표시/피격 시 감소를 완료했다고 했는데, 실제 VR(L_Test)에서 게이지가 아예 안 보임" → **조사만 수행, 수정 없음**:
- 현재 에디터가 처음엔 `/Game/Maps/L_Dungeon`을 열고 있었으나, 사용자가 실제 VR 테스트는 `/Game/Maps/L_Test`라고 명확히 알려줘서 L_Test 기준으로 재조사함.
- L_Test에는 배치형 `BP_Enemy_C_1`, `BP_Goblin_C_0/1/2`, `BP_WaveManager_C_0`가 존재. PIE 시작 후 WaveManager 스폰으로 `BP_Goblin_C_3/4/5`까지 총 6마리 고블린이 존재함을 확인.
- **표시 컴포넌트 자체는 런타임에 존재하고 BeginPlay 덮어쓰기도 적용됨**: PIE의 `BP_Enemy_C_1.HealthBarWidget`, `BP_Goblin_C_0.HealthBarWidget`, `BP_Goblin_C_3.HealthBarWidget` 모두 `space=World`, `widgetClass=/Game/UI/WBP_EnemyHealthBar.WBP_EnemyHealthBar_C`, `drawSize=(70,10)`, `bVisible=true`, `bHiddenInGame=false`, `bAbsoluteRotation=true`, `bDrawAtDesiredSize=false`로 확인됨. 에디터 배치/템플릿 기본값은 여전히 `space=Screen`, `drawSize=(200,24)`, `bAbsoluteRotation=false`지만 BeginPlay가 런타임에 World/(70,10)/AbsoluteRotation=true로 바꾸는 구조.
- `WBP_EnemyHealthBar` 위젯 트리는 정상: 루트 `ProgressBar HealthProgressBar`, `Percent=1.0`, `FillColorAndOpacity=(0,0.85,0,1)`, 변수 노출 true.
- **피격 시 감소 로직은 아직 깨져 있음**: `BP_Enemy.HandleDamage`와 `BP_Goblin.HandleDamage`는 `Set CurrentHealth` 직후 `HealthBarWidget -> GetUserWidgetObject -> CastToWBP_EnemyHealthBar -> SetHealthPercent`를 호출하도록 되어 있지만, `WBP_EnemyHealthBar.SetHealthPercent` 함수 내부가 여전히 `Self -> GetOuterObject -> CastToWidgetComponent -> GetOwner -> CastToBP_Enemy/BP_Goblin` 구조임. 이전에 확인했던 대로 `CreateWidget`으로 만든 위젯의 Outer는 WidgetComponent가 아니라 GameInstance라서 `CastToWidgetComponent`가 실패할 수 있음. 따라서 `SetHealthPercent`가 호출돼도 내부 초입에서 실패하여 ProgressBar Percent 갱신이 안 되는 구조적 버그가 남아 있음.
- **게이지가 "아예 안 보임"의 유력 후보**: 런타임 컴포넌트는 살아 있으므로 완전 미생성은 아님. 하지만 World widget 크기가 `(70,10)`으로 매우 작고, `bIsTwoSided=false`, `blendMode=Masked`, 루트가 배경/테두리 없는 초록 ProgressBar 하나뿐이라 VR에서 각도/거리/배경에 따라 사실상 안 보일 수 있음. 추가로 오크 배치 액터 `BP_Enemy_C_1`의 트랜스폼은 여전히 `rotation.pitch=-90` 상태라 이전에 기록된 수동 배치 오크 회전 이상도 남아 있음.
- **저장 상태 이상**: `/Game/Blueprints/Enemies/BP_Enemy`가 dirty=true로 확인됨. `BP_Goblin`, `WBP_EnemyHealthBar`, `L_Test`는 dirty=false. 즉 오크 쪽에는 저장되지 않은 변경이 남아 있을 가능성이 있음.
- 다음 수정 후보(아직 미수행): (1) `WBP_EnemyHealthBar.SetHealthPercent`를 Outer 기반이 아니라 입력 파라미터 기반 또는 Owner 명시 변수 기반으로 단순화하거나, `HandleDamage`에서 직접 Percent를 계산해 위젯 함수에 전달하도록 변경. (2) VR 가시성용으로 `HealthBarWidget`을 더 크게(예: 120x16 또는 150x20), `bIsTwoSided=true`, `blendMode=Transparent/Opaque` 중 실기에서 잘 보이는 설정, 배경/테두리 추가를 검토. (3) `BP_Enemy` dirty 상태를 컴파일/저장으로 정리.

(2026-07-28 세션 추가 #7) 사용자 지적: "데미지를 보내는 액션(타격 모션)이 있고, 그 액션을 받았을 때 데미지가 깎이게 수정해줘" — 기존엔 `BeginAttack`이 애니메이션 재생과 `ApplyDamage` 호출을 **같은 프레임에 동시에** 실행해서, 타격 모션이 실제로 "닿는" 시점과 무관하게 즉시 데미지가 들어가고 있었음(직전 답변에서 이 부분을 사용자에게 설명했었는데, 사용자가 이 인과관계 자체를 고쳐달라고 요청). **BeginAttack과 데미지 적용을 분리하는 구조로 수정**:
- 양쪽 블루프린트에 새 float 변수 `HitDelaySeconds` 추가(오크 0.5초/axe_crit1 길이 2.33초 기준, 고블린 0.25초/ThirdPersonJump_Start 길이 0.47초 기준 — 애니메이션이 대략 타격처럼 보일 시점을 어림잡은 값, 정확한 프레임은 아님).
- 새 함수 `DealAttackDamage`(파라미터 없음, 기존 멤버 변수만 사용)를 만들어 기존에 `BeginAttack` 안에 있던 "ApplyDamage 메시지 호출 → State=Recovery → EndRecovery 타이머 설정" 로직을 통째로 옮김.
- `BeginAttack`은 이제 State=Attack 설정 → 공격 애니메이션 재생 → `SetTimerByFunctionName(DealAttackDamage, HitDelaySeconds)`로 끝남. 즉 **애니메이션이 재생되고 `HitDelaySeconds` 뒤에야 실제로 `DealAttackDamage`가 실행되어 데미지가 들어가고 Recovery로 전환**됨.
- 인터페이스 메시지 노드는 `Class|BPIDamage2|ApplyDamage(메시지)` 문자열로 새로 생성해야 했음(get_node_infos가 보여주는 "|ApplyDamage" 축약형은 이번에도 create_node에 안 먹힘 — GetMesh 때와 동일한 패턴, 이제 세 번째로 확인된 규칙).
- 양쪽 블루프린트 컴파일 경고/에러 없음, 저장 완료. **PIE 실측으로 확인**: State가 `Attack`으로 전환된 뒤 바로 `Recovery`로 넘어가지 않고 실제로 몇 번의 연속 폴링에서 `Attack` 상태가 관측될 만큼 유지되는 것을 확인함(수정 전엔 같은 함수 안에서 즉시 Recovery로 넘어가서 이 상태가 절대 관측될 수 없었음) — 인과관계 분리가 실제로 작동한다는 실증적 증거.

(2026-07-28 세션 추가 #6) 사용자 요청: "L_Test의 leftover 테스트용 파이어볼은 내 테스트에서만 없애주고, 고블린은 때려서 공격하게 해줘" → 처리:
- **L_Test의 leftover BP_Fireball 제거**: `/Game/Maps/L_Test.L_Test:PersistentLevel.BP_Fireball_C_2`를 `remove_from_scene`으로 제거하고 레벨 저장 완료. 이제 PIE 시작 시 자동으로 고블린을 죽이던 부작용이 사라짐(웨이브 카운트 테스트가 더 이상 오염되지 않음).
- **고블린 "때려서 공격" — 임시방편으로 시각적 공격 동작 추가**: 위 세션(#5)에서 조사한 대로 고블린 스켈레톤(`SK_swampgoblin_Skeleton`)에 맞는 진짜 타격/펀치 애니메이션은 프로젝트 어디에도 없음(전체 프로젝트에서 "punch" 검색 결과 0건, Swampgoblin 자체 애니메이션도 로코모션뿐). **정식 타격 애니메이션은 만들 수 없어서, 대신 `BeginAttack`에 기존 `ThirdPersonJump_Start`(non-looping)를 재생하도록 삽입**해서 최소한 "무언가 공격 동작을 하는" 시각적 피드백은 생기도록 함(런지/돌진처럼 보이는 정도, 진짜 타격 모션은 아님 — 사람에게 명확히 알려야 함). `EndRecovery`에도 (오크의 axe_run 복귀 패턴과 동일하게) `ThirdPersonRun` 루프 재생을 `TryResumeChase` 호출 전에 추가해서, 공격 동작 후 다시 달리기 자세로 복귀하도록 함(안 하면 점프 자세로 얼어붙어 있었을 것). 두 함수 모두 컴파일 경고/에러 없이 정상, 저장 완료. **실제 화면에서 이 임시 애니메이션이 어떻게 보이는지는 사람이 직접 확인 필요** — 진짜 타격 모션을 원하면 이전 세션(#5)에 남긴 대로 사람이 직접 리타겟 작업을 해야 함.

(2026-07-28 세션 추가 #5) 사용자가 "적이 시간이 지나면 사라지는 것 같다"는 리포트 + "고블린/오크가 무기로 공격하게 해달라" 요청 → 조사 및 수정:

1. **"적이 사라짐" 근본 원인 진단 및 수정 — ChaseTick을 진짜 NavMesh 기반 이동으로 교체**: `InitialLifeSpan=0`(무한, 자동 파괴 없음) 확인, 대신 원인은 이동 방식 자체였음. 기존 ChaseTick은 `AddActorWorldOffset`으로 목표 방향으로 직선 이동시키는 "블라인드 데드레커닝"이라 바닥/낭떠러지를 전혀 인식하지 못함 — 이번 세션 초반에 실제로 고블린 하나를 임의 좌표로 텔레포트했을 때 Z가 -47412까지 떨어지는 낙사를 직접 목격한 바 있어(바로 그 자리에 바닥이 없었음), 추격 중 이런 낭떠러지/틈에 걸어 들어가면 캐릭터가 계속 낙하해 화면 밖으로 사라진 것처럼 보일 수 있음이 유력한 원인으로 판단됨. **L_Test에 이미 NavMeshBoundsVolume/RecastNavMesh-Default가 존재**하고, `BP_Enemy`/`BP_Goblin` 둘 다 `AutoPossessAI`가 이미 설정되어 있어(기존엔 `PlacedInWorld`) 이번에 `PlacedInWorldOrSpawned`로 바꿔서 WaveManager가 런타임에 스폰하는 개체도 AIController를 확실히 받도록 수정. **ChaseTick의 이동 로직(Normalize→ClampVectorSize→AddActorWorldOffset)을 통째로 제거하고 `AI|내비게이션|SimpleMoveToLocation`(Controller=GetController(), Goal=TargetActor의 GetActorLocation)으로 교체** — 이제 NavMesh를 따라 경로 이동하므로 낭떠러지로 안 걸어 들어감. BP_Enemy/BP_Goblin 둘 다 동일하게 수정, 컴파일 정상, PIE 실측으로 정상 추격→공격→Recovery 전체 사이클 확인(고블린이 (1100,-300)에서 플레이어 쪽 (1121,55)까지 이동, Z는 110.2로 고정 유지 — 낙사 없음, 회전도 이동 방향에 맞게 자연스럽게 따라감).
   - **새로 발견한 위험한 MCP 버그**: `BlueprintTools.get_node_type_pins`는 핀 스키마 조회용 "읽기 전용" 툴처럼 보이지만 **실제로는 그래프에 진짜 노드를 하나 생성하는 부작용이 있음**. 이 부작용으로 생성된 노드는 불안정해서, 그 노드를 그대로 배선해서 쓰면 컴파일 시 "Internal compiler error inside CreateExecutionSchedule (site 2)"로 컴파일 자체가 깨지고 그 직후 해당 노드가 그래프에서 통째로 사라짐(get_node_infos로 재조회하면 "not valid EdGraphNode" 에러). **해결책**: get_node_type_pins로 핀 이름/타입만 확인하고, 실제로 그래프에 넣어 쓸 노드는 반드시 별도로 create_node를 호출해서 새로 만들 것 — get_node_type_pins가 만든 노드를 재사용하지 말 것.
2. **무기 공격 조사 결과**:
   - **오크(BP_Enemy)는 이미 무기로 공격 중**: `SK_Orc_brown`의 머티리얼 슬롯을 확인해보니 `BodyMaterial`, `Weapons_material` 두 개가 있음 — 도끼가 스켈레탈 메시에 이미 baked-in되어 있고, 기존 `axe_crit1` 공격 애니메이션이 그 도끼를 이미 휘두르는 모션임. **추가 작업 불필요**.
   - **고블린(BP_Goblin)은 무기가 실제로 장착되어 있지 않음**: `SK_swampgoblin`의 머티리얼 슬롯은 `eyes/head/body/arms_legs`뿐 — 창(spear)이 스킨에 없고, `/Game/Swampgoblin/Mesh/SM_swampgoblin_spear`라는 별도 스태틱 메시로만 존재(부착된 적 없음). 게다가 고블린 스켈레톤(`SK_swampgoblin_Skeleton`, 본 이름이 표준 UE 매니킨과 동일: root/pelvis/spine_01~03/clavicle_l/r/hand_r 등)에 맞는 **창 휘두르기 애니메이션이 프로젝트에 없음** — `/Game/SpearsAnimationPack`에 방대한 창 애니메이션(swing/thrust/jab 등)이 있어 본 이름은 우연히 호환되어 보이지만, **Skeleton 에셋 자체가 다름**(`SK_Mannequin_Skeleton` vs `SK_swampgoblin_Skeleton`) — UE는 같은 Skeleton 에셋이어야 직접 재생 가능하므로 IK Retargeter 같은 정식 리타겟 작업이 필요한데, **이 MCP 툴셋엔 리타게팅 관련 툴이 전혀 없음**(SkeletalMeshTools에 소켓/본/머티리얼 조회는 있어도 리타겟 기능 없음). 창을 손 소켓에 부착하는 것 자체도 시도해봤으나(`add_socket`으로 `SK_swampgoblin`에 `WeaponSocket`(hand_r 본) 추가는 성공, 소켓 자체는 남겨둠), **`ActorTools.add_component`에는 특정 소켓에 부착하는 파라미터가 없어서** 블루프린트 컴포넌트를 정확히 손 본에 붙이는 것 자체가 이 MCP로는 안 됨(시도했던 `WeaponMesh` 컴포넌트는 손을 따라가지 않을 것이라 판단해 제거함). **결론: 고블린 무기 장착/애니메이션은 이 MCP 환경의 한계로 처리 불가 — 사람이 에디터에서 직접(소켓은 이미 만들어둔 `WeaponSocket` 사용) 스태틱메시 부착 + 선택적으로 IK Retargeter로 SpearsAnimationPack 리타겟 작업을 해야 함**.
3. **부수 확인**: L_Test에 예전부터 배치되어 있던 테스트용 `BP_Fireball` 액터가 **PIE를 시작할 때마다 자동으로 발사되어 근처 고블린(WaveManager가 스폰한 3번째 개체)을 매번 죽이는 것**을 이번에도 재확인함(재현성 있음, 일회성 우연이 아니었음) — 이건 버그라기보다 예전 테스트용으로 남겨둔 액터의 부작용이니, 웨이브 카운트 테스트가 매번 오염되지 않길 원하면 사람이 L_Test에서 그 Fireball 액터를 찾아 제거하는 게 좋음.

(2026-07-28 세션 추가 #4) 사용자가 "너가 해줘"(WaveManager를 L_Test에 배치하고 검증까지 직접 하라)라고 요청 → 수행:
- `/Game/Maps/L_Test`에 `BP_WaveManager` 액터를 (1000,-400,0)에 배치(기존 문서화된 좌표 규칙과 동일)하고 레벨 저장 완료. **이제 L_Test에 WaveManager가 영구적으로 존재함**.
- PIE 실행 후 Wave1(고블린 3마리, BP_Goblin_C_3/4/5로 스폰됨 — 기존에 수동 배치된 BP_Goblin_C_0/1/2와 공존)이 정상 스폰되는 것 확인.
- **실제 실시간 증거로 카운팅 파이프라인이 작동함을 확인**: PIE 시작 직후 BP_Goblin_C_3이 실제로 사망했고(레벨에 예전부터 남아있던 배치형 테스트용 `BP_Fireball` 액터 하나가 BeginPlay와 동시에 자체 발사되어 우연히 C_3과 충돌 → BPI_Damage2로 실제 데미지 적용 → 사망 → OnDestroyed 델리게이트 발동), `BP_WaveManager.RemainingInWave`가 3→2로 실제로 감소하는 것을 라이브 프로퍼티 조회로 확인함. 이는 이번 세션에 수정한 Wave2(오크 2마리) 로직과 완전히 동일한 카운팅 메커니즘(DecrementInt→CompareInt→CallDelegate 체인)이라 Wave2 쪽도 구조적으로 신뢰도가 높아짐.
  - **부수적으로 발견한 사소한 버그(고치지 않음, 범위 밖)**: `BP_Fireball`의 `HandleProjectileOverlap` 함수에서 `SpawnSystemAtLocation`이 `OtherActor`(피격 액터)의 위치를 참조하는데, 이미 그 시점에 `OtherActor`가 Destroy()되어 있어서 "OtherActor 프로퍼티를 통해 BP_Goblin_C_3에 접근하려고 했지만... 유효하지 않습니다" 런타임 경고/에러가 로그에 남음(치명적이지 않고 기능은 정상 작동하나, 피격 이펙트 위치 계산이 파괴 전에 캐시되도록 노드 순서를 바꾸면 깔끔해짐 — 이번 세션 범위 밖이라 손대지 않음).
- **남은 2마리(C_4, C_5)를 마저 죽여서 Wave2(오크 2마리 스폰)까지 실측하려 시도했으나 실패**: (a) PIE 중 `add_to_scene_from_asset`/`SpawnActor` 계열로 새 Fireball을 만들어 데미지를 주려 했으나 "Cannot create actors while PIE is active" 에러로 거부됨(이미 문서화된 `remove_from_scene` 제약과 동일한 종류의 PIE 중 액터 생성 금지 제약이 생성에도 적용됨을 이번에 새로 확인). (b) 이미 존재하는 `EquippedSword`를 반복 순간이동시켜 우연히 스윙 속도 판정을 맞춰보려 했으나(이전 세션에도 실패했던 방식) 이번에도 실패(수십 회 시도해도 안 맞음 — MCP 왕복 지연으로 인한 근본적 한계, 이전 세션에 기록한 것과 동일). (c) C_4를 C_5 위치로 강제 이동시켜 고블린-고블린 감지가 트리거되는지 시도했으나 서로 감지/전투가 발생하지 않음(SenseSphere 오버랩 기반 상호 감지는 이번엔 재현 안 됨 — C_3의 사망은 고블린간 전투가 아니라 순전히 우연한 Fireball 충돌이었던 것으로 최종 결론).
- **결론**: Wave1→Wave2 전환의 핵심 메커니즘(사망 카운트 감소, 0 도달 시 다음 웨이브 트리거)은 실제 런타임에서 최소 1건 실증됨. 하지만 이 MCP 환경에는 살아있는 액터에 데미지를 확실하게 주입할 방법이 없어서(신규 액터 스폰도 PIE 중 금지, 함수 직접 호출 툴도 없음) 나머지 2마리를 마저 죽여 오크 2마리 스폰까지 이어지는 전체 시퀀스는 **사람이 직접 검으로 고블린 3마리를 죽여서 최종 확인해야 함**. WaveManager는 이미 L_Test에 배치·저장되어 있으니 바로 플레이해서 확인 가능.

(2026-07-28 세션 추가 #3) 사용자 요청 3건 처리:
1. **체력바 Space Screen→World 전환**: "그냥 흰 박스만 떠다닌다"는 리포트의 원인으로 VR에서 `Space=Screen` 위젯이 스테레오 렌더링에서 3D 앵커링 없이 납작한 오버레이로 보이는 현상을 지목(스크린샷으로 100% 확증은 못 했으나 데이터/설정은 모두 정상이었어서 이게 유일한 설명 후보였음). BP_Enemy/BP_Goblin BeginPlay의 `SetWidgetSpace`를 `World`로, `SetDrawSize`를 (200,24)→(70,10)cm로 변경(World space는 픽셀=cm로 해석되어 기존 크기가 캐릭터보다 큼). PIE에서 라이브 인스턴스 프로퍼티로 Space=World, DrawSize=(70,10) 반영 확인, 컴파일/런타임 에러 없음. **크기가 실제로 적당한지는 사람이 헤드셋으로 봐야 확정**(World space 물리 크기 감각은 스크린샷으로 확인 못 함).
2. **오크(BP_Enemy)에 고블린과 동일한 Idle/Chase 애니메이션 전환 추가**: 고블린에 적용했던 것과 동일한 패턴(BeginPlay는 Idle 포즈, OnSenseBeginOverlap에서 Chase 진입 시 달리기, ChaseTick의 타겟 상실 분기에서 Idle 복귀)을 오크에도 적용. 오크는 기존에 axe_run을 시작부터 무한 루프하고 있었던 것을 axe_IDLE1_1로 교체하고, Chase 진입/이탈 지점에 axe_run/axe_IDLE1_1 PlayAnimation 노드를 새로 삽입(공격 시 axe_crit1로 전환하는 기존 로직은 이미 있었으므로 손대지 않음). 컴파일 경고/에러 없음.
3. **BP_WaveManager Wave2를 오크 5마리→2마리로 축소**: 고블린(Wave1) 전멸 시 오크가 5마리가 아니라 2마리만 스폰되도록 요청받음. `SpawnActorFromClass_6/7/8`(오크 3,4,5번째)과 그 좌표 노드(`CallFunction_10~15`, MakeVector+MakeTransform 쌍), `AddDelegate_5/6/7`(OnDestroyed 바인딩 3개)을 삭제하고, `AddDelegate_4`(오크 2번째의 OnDestroyed 바인딩)의 `then` 핀이 Wave1의 마지막 바인딩 노드와 동일하게 아무 데도 연결 안 된 채로 끝나도록 정리(구조적으로 Wave1 3마리 패턴과 대칭). `Wave2Count` 기본값도 5→2로 변경(RemainingInWave 초기화에 쓰이므로 실제 스폰 개수와 반드시 일치해야 함). 컴파일 경고/에러 없음. **주의**: 이 세션 시점 L_Test에는 BP_WaveManager 액터가 배치돼 있지 않아서(사람이 이전에 치웠거나 별도 세션 상태로 보임) Wave1→Wave2 실전 전환은 이번에도 실측 못 함 — 구조 검증(그래프 배선, 컴파일)만 완료. **사람이 L_Test에 BP_WaveManager를 다시 배치하고 실제로 고블린 3마리를 죽여서 오크 2마리가 스폰되는지 확인 필요**.

(2026-07-28 세션 추가 #2) 사용자가 "플레이어 공격(검)도 안 된다"고 리포트 → 근본 원인 확인 및 수정:
- **BP_Sword.UpdateSwordSpeed의 GetActorLocation self 핀이 처음부터 연결 안 된 상태였음**: `트랜스포메이션|GetActorLocation` 노드의 `self` 입력 핀이 어떤 노드에도 연결되어 있지 않아(값 없음, connected_pins 빈 배열), 항상 `target=None`으로 호출됨 → 매 프레임 CurrentLocation이 `(0,0,0)`으로 고정 → `Distance(CurrentLocation, PreviousLocation)`도 항상 0에 수렴 → `SwordSpeed`가 절대 0을 넘지 못함 → `TrySwordDamage`의 `InRange(SwordSpeed, MinimumDamageSpeed=100, ...)` 조건이 항상 거짓이라 `ApplyDamage` 메시지가 구조적으로 절대 발동할 수 없었음(검이 어떤 속도로 휘둘러져도 데미지가 들어갈 수 없는 상태). PROJECT_STATUS.md에 예전부터 기록되어 있던 "TrySwordDamage lacks a valid execution/damage path"가 바로 이 버그였음(실행 경로 자체는 존재했으나 속도 계산이 원천적으로 죽어있었던 것).
  - **수정**: `create_node`로 `Variables|셀프레퍼런스가져오기`(Self 레퍼런스) 노드를 새로 만들어 `GetActorLocation.self` 핀에 연결. Compile 경고/에러 없음, 저장 완료.
  - **주의(create_node 문자열 함정 재확인)**: get_node_infos가 표시하는 축약형 라벨(`"Variables|셀프-레퍼런스"`, `"|GetMesh"` 등)을 그대로 create_node에 넣으면 실패함("does not exist"). 반드시 `find_node_types`로 정확한 문자열(이번엔 `"Variables|셀프레퍼런스가져오기"`)을 먼저 조회할 것 — 이번 세션에서 GetMesh에 이어 두 번째로 발견된 동일 패턴.
  - **PIE 검증 한계**: MCP로 검(EquippedSword 인스턴스)을 순간이동시켜 던미에 겹치게 하는 방식으로 실제 데미지 발동을 재현하려 했으나, SwordSpeed는 "이동이 일어난 그 한 프레임"에만 반짝 높았다가 바로 다음 틱에 0으로 수렴하는 값이라 MCP 왕복 지연 때문에 그 순간을 못 잡음(여러 번 시도, DamageDummy 체력 100/100 그대로). 이는 **버그가 남아있다는 뜻이 아니라 이 MCP 환경의 근본적인 타이밍 한계**임 — self 핀 미연결이라는 원인은 명백하고 수정도 명확하므로 구조적으로는 확실히 고쳐졌으나, **사람이 실제로 VR 컨트롤러로 검을 빠르게 휘둘러서 던미/적 체력이 줄어드는지 최종 확인 필요**.

(2026-07-28 세션 추가 #1) 사용자가 실제 플레이 중 "오크는 X좌표만 바뀌는 것 같고 고블린은 시작부터 계속 달리기만 한다"고 버그 리포트 → 두 가지 실재하는 버그를 찾아 수정함:
1. **ChaseTick 이동 버그 (BP_Enemy와 BP_Goblin 둘 다, 근본 원인 확인)**: 이전 세션에서 AddMovementInput→AddActorWorldOffset로 재작성할 때 만든 `Normalize(방향벡터) * ChaseStepDistance` 계산용 PromotableOperator 곱하기 노드가 `vector*vector`로 타입 고착되어 있었는데, 실제 연결된 B핀은 GetChaseStepDistance(double/float) 변수였음 — A(vector)/B(double)가 섞인 채로 "vector*vector"라는 라벨의 노드가 존재하는 모순 상태(AI_MEMORY에 이미 기록된 "PromotableOperator 체이닝 버그"와 같은 계열이지만 이번엔 변수 Get→PromotableOperator 직결에서 발생). 결과적으로 실제 게임에서 오크가 X축으로만 이동하는 것으로 관찰됨(고블린도 동일 버그 보유, 다만 아래 2번 버그에 가려져 있었음). **수정**: 두 블루프린트의 ChaseTick에서 문제의 PromotableOperator(vector*vector) 노드를 삭제하고 `수학|벡터|ClampVectorSize(A=정규화된 방향벡터, Min=Max=ChaseStepDistance)`로 교체 — Min/Max가 모두 double이라 와일드카드 모호성이 없는 안전한 논-프로모터블 함수. Compile 경고/에러 없음. PIE에서 BP_Enemy_C_1을 플레이어 근처로 이동시켜 실측 검증: 수정 전엔 로그에 기록된 X전용 이동 버그였으나 수정 후 X와 Y 모두 유의미하게 변화(대각선 추적) 확인됨 — **실측 검증 완료**.
2. **BP_Goblin 애니메이션이 State와 무관하게 계속 재생되는 버그 (BP_Goblin 전용)**: BeginPlay에서 `SetAnimInstanceClass(ABP_Goblin)` 직후 `PlayAnimation(ThirdPersonRun, loop=true)`를 무조건 호출하고 있었음 — PlayAnimation은 SkeletalMeshComponent를 AnimationSingleNode 모드로 강제 전환해 AnimInstanceClass 할당을 무력화하며, 이후 State가 Idle/Attack/Dead 등으로 바뀌어도 애니메이션을 갱신하는 코드가 어디에도 없어 죽을 때까지 ThirdPersonRun 루프만 재생됨(대조: BP_Enemy/오크는 BeginAttack에서 axe_crit1, EndRecovery에서 axe_run으로 정상적으로 전환하는 로직이 이미 있었음 — 오크의 애니메이션 자체는 문제 없었고 이동만 버그였음). **수정**: (a) BeginPlay의 PlayAnimation 대상을 ThirdPersonRun→**ThirdPersonIdle**로 교체(시작 상태가 Idle이므로), (b) `OnSenseBeginOverlap` 함수(Idle→Chase 전이 지점)의 SetState(Chase) 직후에 `GetMesh()`+`PlayAnimation(ThirdPersonRun, loop=true)`를 새로 삽입, (c) `ChaseTick`의 타겟 상실 branch(SetState(Idle) 직후, SetTargetActor(None) 이전)에 `GetMesh()`+`PlayAnimation(ThirdPersonIdle, loop=true)`를 새로 삽입 — 이제 Idle↔Chase 전이마다 애니메이션이 함께 전환됨. Compile 경고/에러 없음. PIE에서 플레이어가 SenseRadius(800) 밖으로 벗어나자 State가 Chase→Idle로 정상 전환되는 것을 실측 확인(TargetActor도 None으로 복귀) — **Idle 복귀 애니메이션 전환 경로는 구조적+상태 전이 실측 모두 확인됨**. Chase 진입 시 Run 애니메이션 재생 자체는 구조적으로는 확인(노드 배선), 애니메이션이 실제로 화면에 보이는지는 사람이 육안 확인 필요.
   - **PlayAnimation 노드 생성 시 참고**: `create_node`에 `"|GetMesh"` 문자열은 실패함("does not exist") — 정확한 타입 문자열은 `find_node_types`로 확인한 `"Variables|캐릭터|GetMesh"`였음. get_node_infos가 표시하는 축약형 `"|GetMesh"`는 존재하는 노드를 보여줄 때의 표시일 뿐, create_node에 그대로 넣을 수 있는 문자열이 아님 — 이 축약 표시 패턴(`"|FunctionName"`)은 다른 노드에서도 마찬가지일 가능성이 있으니 새로 노드를 만들 때는 항상 find_node_types로 정확한 문자열을 먼저 확인할 것.
   - **새로 발견한 미해결 이상 현상(원인 불명, 재현 불확실)**: PIE 검증 중 BP_Goblin_C_0 인스턴스 하나가 State=Chase/TargetActor=플레이어로 정상 감지된 채로 약 15초 이상 완전히 정지(좌표 소수점까지 고정)해 있는 현상을 목격함 — 같은 세션에서 BP_Goblin_C_1은 스폰 위치에서 플레이어 근처(공격범위 이내)까지 정상적으로 이동한 것을 확인했고, BP_Enemy_C_1(오크)도 이번 수정 후 정상적으로 대각선 이동하는 것을 확인했으므로, ChaseTick 이동 매커니즘 자체는 구조적으로 정상 작동함이 입증됨. 이 특정 정지 현상은 재현 조건을 좁히지 못함(플레이어를 여러 번 빠르게 순간이동시키며 테스트하는 과정에서 발생 — 타이머 중복 등록이나 레이스 컨디션일 가능성 있음, 확실하지 않음). **사람이 실제 플레이로 고블린 여러 마리를 동시에 추적시켜보며 간헐적 정지가 재현되는지 확인 필요**.
   - 이 검증 도중 실수로 BP_Goblin_C_0를 (500,700) 같은 좌표로 강제 순간이동시켰다가 바닥이 없는 지점이라 Z가 -47412까지 떨어지는 것을 목격함 — 이는 PIE 세션 내(UEDPIE_0_L_Test)에서만 발생한 임시 상태이며 PIE 종료 시 폐기되어 저장된 L_Test 에셋에는 전혀 반영되지 않음(실제 L_Test 레벨은 손대지 않음, PIE만 시작→검증→종료).

(이전 세션) BP_Enemy/BP_Goblin 이동·비주얼·UI 개선 세션 완료:
1. BP_Goblin 메시 회전 버그 수정 (Roll -90으로 BP_Enemy와 일치)
2. ChaseTick 이동 로직을 AddMovementInput → AddActorWorldOffset 기반으로 전면 재작성 (양쪽 블루프린트 모두), PromotableOperator 와일드카드 체이닝 버그를 새로 발견/회피
3. 적 머리 위 체력바(WBP_EnemyHealthBar + WidgetComponent) 추가 — **후속 세션에서 실시간 갱신 문제 해결함**: 이전 세션이 이미 자가-갱신형 로직(WBP_EnemyHealthBar:SetHealthPercent 함수가 Self→GetOuterObject→CastToWidgetComponent→GetOwner→CastToBP_Enemy/CastToBP_Goblin 분기로 자기 소유 액터의 CurrentHealth/MaxHealth를 직접 읽어와 ProgressBar.SetPercent 호출)와 EventGraph의 Construct 이벤트+0.2초 반복 타이머(SetTimerByFunctionName)까지 다 만들어놨었는데, EventGraph에서 Construct 직후 SetHealthPercent를 직접 호출하는 노드의 "self" 입력 핀이 연결이 안 되어 있었음 (이번 세션에서 여러 번 발견한 그 패턴 — 로컬 함수 호출 노드의 self 핀은 create_node로 만들면 자동 연결 안 됨). Self 레퍼런스 노드를 연결해서 해결. Compile/런타임 에러 없음 확인. 단, 실제 데미지를 넣어서 퍼센트가 줄어드는 것 자체는 이 MCP로는 확인 못 함(살아있는 인스턴스에 데미지를 가할 방법이 없음) — 로직 구조상 정상 작동해야 하나 사람이 실제로 때려보고 확인 필요
   - 참고: 처음에 BP_Enemy/BP_Goblin의 HandleDamage에서 캐스트된 위젯 인스턴스의 멤버 변수(HealthProgressBar)를 직접 Get 하는 노드를 만들려 했는데 실패함 — "Variables|WBP_EnemyHealthBar|GetHealthProgressBar" 같은 "다른 블루프린트 인스턴스의 멤버 변수를 외부에서 Get" 하는 타입의 노드는 find_node_types로는 검색되는데 create_node로는 생성이 안 됨("does not exist" 에러). 반면 **같은 블루프린트 자기 자신의 컨텍스트에서는 동일한 노드가 정상 생성됨** — 이 구분이 중요: 외부 인스턴스의 멤버 변수 Get 노드는 이 MCP로 못 만들고, 그 대신 위 방식처럼 대상 블루프린트 자신에게 함수를 만들어서 호출하는 패턴(pull 방식)을 써야 함
4. ABP_Goblin(애니메이션 블루프린트) 시도 — 부분적으로 가능하나 TargetSkeleton 설정 불가로 포기, 생성한 에셋 삭제함
Git Commit 대기 중 (이전 07_WaveSystem 단계와 함께).

prompts/07_WaveSystem.md (BP_WaveManager) 구현 완료: Wave1 = BP_Goblin 3마리, Wave2 = BP_Enemy 5마리, 전멸 시 OnAllWavesCleared 디스패처. Compile 정상, PIE로 스폰/변수 확인됨. 실제 전투로 죽여서 Wave 전환이 실사격되는지는 사람 확인 필요(사유는 아래 "다음 작업" 참고).

## 완료
- Unreal Engine 5.8 프로젝트 생성
- Unreal MCP 연결
- Codex에서 Actor 생성 테스트 성공
- prompts/00_ProjectAudit.md 읽기 전용 감사 완료
- 표준 Content 폴더 생성
  - /Game/Maps
  - /Game/Blueprints/Player
  - /Game/Blueprints/Weapons
  - /Game/Blueprints/Enemies
  - /Game/Blueprints/Boss
  - /Game/Blueprints/Systems
  - /Game/Blueprints/Interfaces
  - /Game/UI
  - /Game/Materials
  - /Game/VFX
  - /Game/Audio
- /Game/Maps/L_Test 생성
- /Game/Maps/L_Arena 생성
- 두 레벨은 /Game/XRFramework/Levels/L_XRTemplate 기반이며 VR Template 기능 유지
- L_Test에서 바닥, 조명, PlayerStart 확인
- L_Test PIE 실행 및 BP_XRPawn 스폰 확인
- 수정된 Blueprint 없음; PIE 로그에서 재컴파일 필요 Blueprint 없음 확인
- (이전 세션에서 완료된 것으로 보임, 이 문서에 미기록 상태였음) prompts/02_DamageSystem.md: BPI_Damageable, BP_DamageDummy 생성. 단, 실제 코드 의존성 조사 결과 BP_DamageDummy와 BP_XRPawn이 실제로 구현/참조하는 인터페이스는 BPI_Damageable이 아니라 /Game/Blueprints/Interfaces/BPI_Damage2 (레퍼런스 0인 BPI_Damageable은 미사용 상태로 방치됨). 이후 프롬프트는 BPI_Damage2를 기준으로 작업해야 함.
- prompts/06_Enemy.md: BP_Enemy 생성 (/Game/Blueprints/Enemies/BP_Enemy, 부모 클래스 Character)
  - Health 40 (CurrentHealth/MaxHealth), BPI_Damage2 인터페이스 구현 (ApplyDamage, BP_DamageDummy 복제 후 리페어런트하는 방식으로 인터페이스 유지)
  - State 변수(Name 타입: Idle/Chase/AttackWindup/Attack/Recovery/Hit/Dead)로 상태 머신 구현. UserDefinedEnum 대신 Name 사용 (사유: MCP에 Enum 에셋 생성 툴 없음)
  - SenseSphere(SphereComponent, 반경 800, OverlapAllDynamic)의 OnComponentBeginOverlap로 Idle→Chase 감지
  - ChaseTick 함수(0.15초 반복 타이머)로 추적 이동(AddMovementInput) 및 SenseRadius/AttackRange(150cm) 판정 → AttackWindup 전이
  - BeginAttack 함수(0.7초 지연 타이머로 호출)에서 인터페이스 메시지 호출로 TargetActor에 15 데미지 적용 → Recovery 전이
  - HandleDamage 함수(ApplyDamage 이벤트에서 호출)에서 체력 0 이하 시 Dead 상태 전이 후 타이머 정리 + DestroyActor, 그렇지 않으면 Hit 상태로 잠시 전이 후 EndHit에서 Chase/Idle 복귀
  - 시각 에셋: /Game/Orc/Mesh/SK_Orc_brown 스켈레탈 메시 사용 (AssetManifest.md의 "TBD"는 stale 문서였음, 실제로는 Fab에서 임포트된 Orc/Skeleton_Necromancer/Swampgoblin 에셋이 이미 존재)
  - /Game/Maps/L_Test에 BP_Enemy 1개 배치, Compile/PIE 검증 완료 (아래 알려진 경고 참고)
- BP_Enemy 감지 실패 버그 2건 발견 및 수정
  1. **근본 원인 A (설계 문제)**: 플레이어 폰 BP_XRPawn(VR 템플릿 기반)은 일반 Character와 달리 캡슐 콜리전이 없음 (컴포넌트: VROrigin/Camera/양손 컨트롤러/HMD 메시뿐). 따라서 SenseSphere의 OnComponentBeginOverlap는 플레이어가 아무리 가까이 있어도 절대 발동하지 않음.
     - 수정: 팀원이 동시에 BP_XRPawn(손-검 연결)을 별도 로컬 에디터에서 작업 중이라 플레이어 쪽은 건드리지 않고, BP_Enemy의 EventGraph에 0.2초 반복 타이머(SetTimerByFunctionName)로 GetPlayerPawn(0)과의 GetDistanceTo 거리 체크(CheckPlayerDistance 커스텀 이벤트) → State=="Idle" AND 거리<=SenseRadius면 기존 OnSenseBeginOverlap 함수를 직접 호출하도록 변경. 오버랩 기반 코드(SenseSphere/OnSenseBeginOverlap 바인딩)는 그대로 두되 트리거 경로만 타이머로 대체.
  2. **근본 원인 B (숨어있던 버그, 훨씬 치명적)**: OnSenseBeginOverlap 함수 자체가 처음부터 실행되지 않고 있었음 — K2Node_FunctionEntry의 실행 핀("then")이 그 다음 노드(CastToPawn)에 전혀 연결되어 있지 않았음 (데이터 핀만 연결되고 실행 흐름 핀은 끊긴 상태). 즉 이 함수는 어떻게 호출하든(오버랩이든 직접 호출이든) 아무 동작도 하지 않는 상태였음. PrintString을 임시로 삽입해 실행 흐름을 추적해서 발견함 (Tick + 조건 강제 true로도 반응이 없어서 역추적함).
     - 수정: FunctionEntry.then → CastToPawn.execute 연결 추가로 해결.
  - 두 버그가 겹쳐 있어서 원인 A만 고쳤을 때도(SetTimerByEvent/CreateDelegate, 이후 Tick 이벤트까지 시도) 계속 반응이 없었음 — 최종적으로 원인 B를 찾고 나서야 정상 동작 확인됨.
  - PIE 자동 검증 결과(플레이어 폰을 원거리→중거리→근거리로 순간이동시키며 State/TargetActor/체력 확인):
    - 원거리(5000cm): State=Idle 유지 (정상, 오검출 없음)
    - 중거리(약 514cm, SenseRadius 800 이내·AttackRange 150 밖): State=Chase, TargetActor=플레이어 폰으로 정상 전이
    - 근거리(스폰 시 기본 약 122cm, AttackRange 이내): Idle→Chase→AttackWindup→Attack 전체 사이클 진행, 플레이어 CurrentHealth가 100→0까지 반복 공격으로 정상 감소 (BPI_Damage2 경유 데미지 적용 확인됨)
  - 디버그용으로 추가했던 PrintString 노드 3개와 임시 Event Tick 노드는 모두 제거하고 원래 설계(0.2초 타이머 기반)로 정리 완료. Compile 경고/에러 없음.

- prompts/07_WaveSystem.md: BP_WaveManager 생성 (/Game/Blueprints/Systems/BP_WaveManager, 부모 클래스 Actor)
  - 변수: Wave1Count(int, 기본 3), Wave2Count(int, 기본 5), RemainingInWave(int), CurrentWave(int, 1=Wave1/2=Wave2). 이벤트 디스패처: OnAllWavesCleared(파라미터 없음)
  - **중요한 설계 변경 사항 (MCP 한계로 인함)**: 원래 계획은 Wave1Class/Wave2Class를 "Class 레퍼런스" 타입 변수로 노출하는 것이었으나, 이 MCP 환경의 BlueprintTools.add_variable은 지정된 기본 타입(bool/int/float/byte/name/string/text/Vector/Rotator/Transform/Vector2D/LinearColor)만 지원하고 "class"는 명시적으로 거부됨. BlueprintTools.add_object_variable도 시도했으나 이것은 TSubclassOf가 아니라 순수 "오브젝트 레퍼런스"(인스턴스 참조) 타입을 만든다는 것을 get_node_infos로 확인함. 이 툴셋에는 Class 레퍼런스 변수를 만드는 방법이 없음. → 대신 스폰할 클래스(BP_Goblin/BP_Enemy)는 각 SpawnActorFromClass 노드의 Class 핀에 리터럴 값으로 직접 설정함(예: "/Game/Blueprints/Enemies/BP_Goblin.BP_Goblin_C"). Wave1Count/Wave2Count 변수는 정상적으로 만들어 RemainingInWave 초기화에 사용하지만, 실제 스폰 횟수는 (아래 이유로) 루프가 아니라 고정된 개수의 순차 노드 체인이라 이 변수를 바꿔도 스폰 개수는 안 바뀜 — 진짜 동적 웨이브 크기가 필요해지면 사람이 에디터에서 노드를 추가/제거해야 함
  - **또 다른 MCP 한계**: find_node_types로 정수 연산자(+,-,==,<= 등 PromotableOperator 계열)를 검색해도 전혀 나오지 않음(부동소수점은 BP_Enemy에서처럼 "float-float" 등으로 존재하지만 정수 버전은 검색으로 못 찾음). 대신 "Math|Integer|DecrementInt"(매크로, Value 핀에 레퍼런스로 연결), "Math|Integer|CompareInt"(매크로, Input/CompareWith 비교 후 >, ==, < 세 개의 실행 핀 분기)를 발견해 사용함 — 정수 증감/비교가 필요하면 이 두 매크로를 우선 사용할 것
  - **스폰 위치**: GetActorLocation + 벡터 덧셈 연산자도 find_node_types로 못 찾아서(수학|벡터|vector+vector 시도했으나 없음), WaveManager 위치에 상대적인 오프셋 계산 대신 **L_Test 월드 좌표계의 리터럴 절대 좌표**를 각 스폰의 MakeVector 노드에 직접 입력함 (X 800~1250, Y -300~-150, Z 100 부근, 기존 액터들과 겹치지 않는 바닥 위 빈 공간). WaveManager 액터 자체는 (1000, -400, 0)에 배치. 이후 레벨을 크게 바꾸면 이 좌표들도 사람이 손으로 조정해야 함
  - **OnDestroyed 바인딩 (사망 감지) 설계**: "게임|AssignOnDestroyed" 편의 노드는 호출할 때마다 새 커스텀 이벤트를 자동 생성하는데, 그 자동 생성 이름이 항상 "OnDestroyed_이벤트"(또는 "_0" 등 얕은 접미사)라서 스폰마다(8번) 이 노드를 반복 사용하면 이름 충돌로 컴파일 에러남 ("둘 이상의 함수가 이름이 같습니다: OnDestroyed_이벤트"). 이 MCP엔 커스텀 이벤트 이름을 바꾸는 툴이 없어서(ObjectTools.set_properties로 CustomFunctionName 시도했으나 리플렉션 불가), **첫 번째 스폰(고블린 1)에서만 AssignOnDestroyed를 써서 커스텀 이벤트를 하나 만들고("K2Node_CustomEvent_1", DestroyedActor 파라미터 포함), 나머지 7번의 스폰은 "게임|BindEventtoOnDestroyed"(K2Node_AddDelegate, self=스폰된 액터, Delegate=그 커스텀 이벤트의 OutputDelegate 핀)로 수동 바인딩**해서 이벤트 하나를 8번 재사용함. 델리게이트 출력 핀은 여러 입력에 팬아웃 가능(다른 액터 8개 각각의 OnDestroyed에 동일 이벤트를 바인딩), 실행 입력 핀도 여러 출력에서 팬인 가능함을 확인함(둘 다 정상 동작, UE 표준 동작임)
  - 로직: BeginPlay → CurrentWave=1, RemainingInWave=Wave1Count → 고블린 3마리 순차 스폰(각각 스폰 직후 OnDestroyed 바인딩) → (죽을 때마다) 공유 커스텀 이벤트 → RemainingInWave 1 감소 → CompareInt(==0) → 0이면 CompareInt(CurrentWave==1) → 참이면 Wave2 스폰(CurrentWave=2, RemainingInWave=Wave2Count, BP_Enemy 5마리 순차 스폰+바인딩), 거짓이면(이미 Wave2) OnAllWavesCleared 브로드캐스트
  - **툴링 함정 추가 발견**: 그래프에 새 K2Node_MacroInstance(DecrementInt, CompareInt 등)를 만든 뒤 또 다른 새 매크로 인스턴스를 만들면, 먼저 만든 매크로 인스턴스가 (연결된 배선을 포함해서) 완전히 사라지는 현상을 겪음(get_node_infos에서 "not valid EdGraphNode" 에러). 원인 불명(스켈레톤 재컴파일 시 매크로 인스턴스가 재구성/파괴되는 것으로 추정). **해결책**: 매크로 인스턴스 노드들은 전부 만든 직후 바로 완전히 배선하고, 그 이후에는 그 그래프에 새 매크로 인스턴스를 추가하지 않거나, 추가해야 한다면 매번 compile 후 get_node_infos로 이전 매크로 인스턴스들이 여전히 유효한지 확인할 것
  - BP_Goblin (/Game/Blueprints/Enemies/BP_Goblin): BP_Enemy를 AssetTools.duplicate로 복제 후, CharacterMesh0 컴포넌트(CDO)의 SkeletalMesh와 SkeletalMeshAsset 프로퍼티를 모두 SK_Orc_brown → SK_swampgoblin으로 변경(둘 다 따로 설정해야 실제로 반영됨 — SkeletalMesh만 설정하면 SkeletalMeshAsset엔 반영 안 됨). 로직/스탯(Health 40 등)은 BP_Enemy와 100% 동일, 손대지 않음
  - Compile: BP_Goblin, BP_WaveManager 둘 다 경고/에러 없이 정상 컴파일 확인(LogBlueprint 로그로 확인)
  - PIE 검증: L_Test에 BP_WaveManager 1개 배치(1000,-400,0). PIE 시작 후 find_actors로 BP_Goblin_C_0/1/2 3개 정상 스폰, BP_Enemy는 기존 BP_Enemy_C_0 1개만 존재(Wave2 미스폰) 확인. WaveManager의 RemainingInWave=3, CurrentWave=1 확인
  - **검증 미완료 부분**: 죽음 감지→Wave 전환 로직을 실제로 트리거해서 확인하지 못함. 원래 계획은 SceneTools.remove_from_scene으로 스폰된 액터를 강제 삭제해 OnDestroyed를 발동시키는 것이었으나, 이 MCP 환경에서는 **PIE/Simulate 세션이 활성화된 동안 remove_from_scene 호출이 전부 "Cannot remove actors while PIE is active" 에러로 거부됨**(PlayMode_InViewPort, PlayMode_Simulate 둘 다 시도했으나 동일). BPI_Damage2.ApplyDamage 등 살아있는 인스턴스의 함수를 외부에서 직접 호출할 수 있는 툴도 이 MCP 표면엔 없음(ObjectTools는 프로퍼티 get/set과 클래스 조회만 지원). 따라서 죽음→카운터 감소→Wave2 스폰→디스패처 발동의 실제 동작은 **get_node_infos로 그래프 배선을 노드/핀 단위까지 전부 수동 검토해서 구조적으로만 검증**했고 (연결 상태 전부 확인, 실행 핀 끊긴 곳 없음), **런타임에서 실제로 검을 들고 고블린 3마리와 오크 5마리를 죽여서 Wave2 스폰과 OnAllWavesCleared 발동을 사람이 직접 눈으로 확인해야 함**

- (이번 세션) BP_Enemy/BP_Goblin 이동·비주얼·헬스바 개선
  - **BP_Goblin 회전 버그 수정**: `Default__BP_Goblin_C:CharacterMesh0`의 RelativeRotation이 (0,-90,0)으로 BP_Enemy의 (0,-90,-90)과 달랐던 것을 Roll=-90으로 통일. `ObjectTools.set_properties`의 `values` 파라미터는 JSON **문자열**이어야 함(오브젝트 아님) — 예: `"{\"RelativeRotation\":{\"pitch\":0,\"yaw\":-90,\"roll\":-90}}"`.
  - **ChaseTick 이동 재작성 (AddMovementInput → AddActorWorldOffset)**: 두 블루프린트 모두 IfThenElse_1(공격범위 밖)의 else 분기에서 기존 AddMovementInput(및 BP_Enemy의 디버그 PrintString 2개)을 삭제하고, `트랜스포메이션|AddActorWorldOffset`(self=SelfPawn, DeltaLocation=정규화방향*ChaseStepDistance, bSweep=true, bTeleport=false)으로 교체. ChaseStepDistance는 새 float 변수(400.0 * ChaseTickInterval, BeginPlay 없이 ChaseTick 진입 시 매번 재계산해 SetChaseStepDistance에 저장)로 미리 계산.
  - **새로 발견한 심각한 툴링 버그 — PromotableOperator(와일드카드 연산자) 체이닝**: `K2Node_PromotableOperator`(곱하기/나누기 등)의 **출력 핀을 다른 PromotableOperator의 입력에 직접 연결하면**, 나중에 연결한 쪽의 타입이 먼저 만든 노드까지 역전파되어 **먼저 만든 노드의 타입이 통째로 바뀌어버림**(예: float*float로 이미 확인된 노드가 이후 vector*vector로 뒤바뀜, 리터럴 값도 타입 불일치로 깨짐). 컴파일은 경고 없이 통과되므로 발견하기 어려움. **해결책**: PromotableOperator의 결과값을 다른 PromotableOperator에 절대 직접 연결하지 말 것. 중간에 진짜 블루프린트 변수(SetXXX/GetXXX, add_variable로 생성)를 하나 끼워넣어 "굳히기"(concrete VariableGet/Set은 이 버그의 영향을 받지 않음, CallFunction 결과도 안전함 — 오직 PromotableOperator→PromotableOperator 직결만 위험함).
  - **헬스바 UI 추가**: `/Game/UI/WBP_EnemyHealthBar`(UserWidget, 루트=ProgressBar 하나, 기본 Percent=1.0, 초록 채우기색) 생성. 양쪽 블루프린트에 `WidgetComponent`("HealthBarWidget") 추가, RelativeLocation Z=220(캡슐 루트에 부착).
  - **새로 발견한 툴링 한계 A — SCS 템플릿 프로퍼티가 인스턴스에 전파 안 됨**: `ActorTools.add_component`로 새로 추가한 컴포넌트의 프로퍼티(WidgetClass/Space/DrawSize 등)를 CDO 레퍼런스(`BP_X_C:컴포넌트이름_GEN_VARIABLE`)에 `ObjectTools.set_properties`로 설정하면 **CDO 자체를 읽었을 땐 값이 맞게 나오지만, 실제로 스폰되거나 레벨에 배치된 액터 인스턴스는 그 값을 전혀 상속받지 못하고 클래스 기본값(None/World/500x500)을 사용함**. 컴파일을 여러 번 반복해도 동일. 이는 이 MCP의 컴포넌트 프로퍼티 편집이 실제 SCS(SimpleConstructionScript) 템플릿이 아니라 CDO 인스턴스만 건드리기 때문으로 추정됨. **해결책**: 이런 컴포넌트는 BeginPlay 이벤트그래프에서 매번 명시적으로 다시 설정해야 함 — WidgetComponent의 경우 `유저인터페이스|위젯생성`(CreateWidget, Class 핀에 클래스 리터럴 지정) → `유저인터페이스|SetWidget`(컴포넌트에 인스턴스 할당) → `SetWidgetSpace`/`SetDrawSize`("(X=150,Y=20)" 같은 괄호 포함 문자열 포맷 필요, "150,20"은 파싱 에러)/`SetDrawAtDesiredSize`를 BeginPlay에서 직접 호출. **주의: WidgetComponent에는 SetWidgetClass 같은 네이티브 함수가 없음(이 엔진 빌드 기준 find_node_types로 검색해도 없음)**, 반드시 CreateWidget+SetWidget 조합을 쓸 것.
  - **새로 발견한 툴링 한계 B — Widget Blueprint 클래스는 다른 클래스에서 create_node로 멤버 접근 불가**: `create_node`로 "Class|WBP_X|GetY"/"Class|WBP_X|SetY"/함수호출 형태의 노드를 만들려고 하면, `find_node_types`(context_pins 사용)는 후보를 찾아내지만 **`create_node`로 그 정확한 문자열을 넘겨도 항상 "does not exist" 에러**가 남. 이는 Widget Blueprint 클래스(UWidgetBlueprint 파생)에서만 발생하며, 일반 Actor 블루프린트(BP_Enemy/BP_Goblin) 간의 크로스클래스 멤버 접근(`Class|BPGoblin|GetChaseStepDistance`, `Class|BPGoblin|BeginAttack` 등)은 정상 작동함(변수/함수 모두, get/set 모두). **반대 방향(Widget Blueprint 자신의 그래프 안에서 다른 Actor 블루프린트의 멤버를 읽는 것)은 문제없이 작동함** — 이를 이용해 "위젯이 자신의 소유 액터를 스스로 찾아서 값을 읽어오는" 폴링 설계로 우회함(WBP_EnemyHealthBar의 `SetHealthPercent` 함수, Construct 이벤트 + 0.2초 루프 타이머로 호출).
  - **새로 발견한 툴링 한계 C(치명적, 미해결) — CreateWidget으로 만든 위젯의 Outer가 WidgetComponent가 아님**: 위 한계 A의 우회책으로 BeginPlay에서 `CreateWidget`+`SetWidget`을 쓰면서, 위젯이 "자신을 소유한 WidgetComponent"를 찾기 위해 `유틸리티|GetOuterObject(self)` → `CastToWidgetComponent` → `GetOwner()`로 소유 액터를 역추적하는 설계를 세웠으나, **디버그 PrintString으로 확인한 결과 CreateWidget으로 생성된 위젯의 Outer는 WidgetComponent가 아니라 GameInstance였음** (`UWidgetBlueprintLibrary::Create`가 내부적으로 WorldContextObject의 World만 사용하고 컴포넌트를 Outer로 넘기지 않는 것으로 추정). 따라서 CastToWidgetComponent가 항상 실패(CastFailed)하여 실시간 체력 퍼센트 갱신 로직이 전혀 실행되지 않음 — **체력바는 WBP_EnemyHealthBar ProgressBar의 디자인타임 기본값 Percent=1.0(풀피)으로 고정 표시되며, 데미지를 받아도 갱신되지 않음**. 올바른 해결책은 위젯이 `GetAllActorsOfClass(BP_Enemy)`/`GetAllActorsOfClass(BP_Goblin)`으로 후보를 모두 순회하며 각 액터의 `GetHealthBarWidget()→GetUserWidgetObject()`가 자기 자신(Self)과 같은지 비교해서 진짜 소유자를 찾는 것이지만(ForEachLoop 매크로 필요), 이번 세션 시간 내에 구현하지 못함. **다음 세션에서 이어서 할 일**: WBP_EnemyHealthBar의 `SetHealthPercent` 함수(`/Game/UI/WBP_EnemyHealthBar.WBP_EnemyHealthBar:SetHealthPercent`)에서 현재 죽어있는 GetOuterObject→CastToWidgetComponent→GetOwner 체인(K2Node_CallFunction_1, K2Node_DynamicCast_0, K2Node_CallFunction_2)을 삭제하고, ForEachLoop 기반 액터 매칭으로 교체할 것. MacroInstance(ForEachLoop 포함) 관련해서는 07_WaveSystem 세션에서 발견된 "매크로 인스턴스 여러 개를 연달아 만들면 먼저 만든 게 사라지는 버그"(아래 알려진 경고 참고)에 주의.
  - **애니메이션 블루프린트(ABP_Goblin) 시도 결과**: `BlueprintTools.create`에 `asset_type={refPath:"/Script/Engine.AnimInstance"}`를 넘기면 실제로 AnimGraph를 가진 진짜 UAnimBlueprint가 생성됨(list_graphs로 확인, EventGraph 외에 AnimGraph 존재) — 즉 이 MCP로 AnimBlueprint 생성 자체는 가능함. 하지만 **TargetSkeleton 프로퍼티를 설정할 방법을 찾지 못함**: `ObjectTools.list_properties`/`get_properties`/`set_properties`에 블루프린트 에셋 경로를 넘기면 항상 CDO(`Default__ABP_Goblin_C`)로 자동 리졸브되는데, TargetSkeleton은 UAnimBlueprint 오브젝트 자체(에셋 레벨)의 프로퍼티라 CDO에는 없음("could not be read/set" 에러). 스켈레톤이 지정 안 된 AnimBlueprint는 스켈레톤 전용 애니메이션 에셋(ThirdPersonIdle 등)을 노드로 넣을 수 없어 사실상 쓸모가 없으므로, **생성했던 ABP_Goblin 에셋은 삭제함**. BP_Goblin의 CharacterMesh0 AnimClass는 미할당 상태로 남음 — Goblin은 여전히 애니메이션 없이 위치만 이동함(Orc/BP_Enemy와 동일한 상황).

## 진행 중
- 사람이 실제 Unreal Editor 화면에서 06_Enemy.md 자동 검증 결과를 육안으로 재확인 (선택 사항 — MCP로 State/TargetActor/체력 변화를 직접 확인했으므로 핵심 동작은 검증됨)
- 사람이 실제 플레이로 BP_WaveManager의 Wave1→Wave2→OnAllWavesCleared 전환을 확인 (필수 — 위 "검증 미완료 부분" 참고)

## 다음 작업
- **(2026-07-28 세션 추가 #14, 필수) 보스 불 마법 공격 실측 확인**: 위 "세션 추가 #14" 참고 — 등장 시 첫 발사, 피격마다 반격 발사, 실제 명중 데미지, 머티리얼 비주얼을 사람이 직접 확인해야 함.
- **(2026-07-28 세션 추가 #13, 필수) 보스 공격 VFX/회전 실측 확인**: 위 "세션 추가 #13" 참고 — 사람이 직접 웨이브 클리어까지 진행해서 보스를 스폰시켜 VFX 위치/타이밍과 회전 부드러움을 확인해야 함.
- **(2026-07-28 세션 추가 #6, 권장) 고블린 공격 애니메이션 육안 확인**: 지금은 `ThirdPersonJump_Start`를 임시로 재생 중(진짜 타격 모션 아님). 사람이 PIE로 보고 어색하면 다른 임시 애니메이션으로 교체하거나 아래 리타겟 작업으로 진짜 타격 모션을 만들 것.
- **(2026-07-28 세션 추가 #5, 선택) 고블린 무기/공격 애니메이션 마무리**: `SK_swampgoblin`에 `WeaponSocket`(hand_r 본)을 이미 만들어뒀음. 사람이 에디터에서 `SM_swampgoblin_spear`를 그 소켓에 부착하는 StaticMeshComponent를 BP_Goblin에 추가하고(간단, MCP로는 소켓 지정 부착이 안 됨), 원하면 `/Game/SpearsAnimationPack`을 IK Retargeter로 `SK_swampgoblin_Skeleton`에 리타겟해서 공격 애니메이션도 추가 가능(본 이름이 표준 UE 매니킨과 동일해서 리타겟 자체는 수월할 것으로 예상).
- **(2026-07-28 세션 추가 #5, 선택) L_Test의 leftover BP_Fireball 액터 제거 고려**: PIE 시작 시마다 자동 발사되어 웨이브 스폰 고블린 하나를 매번 죽임(재현 확인됨). 웨이브 카운트 테스트를 깨끗하게 하려면 사람이 그 액터를 찾아 지우는 게 좋음.
- **(2026-07-28 세션 추가 #4, 필수) 사람이 직접 고블린 3마리를 죽여서 Wave2(오크 2마리) 스폰 최종 확인**: WaveManager는 이미 L_Test (1000,-400,0)에 배치·저장되어 있음. 카운팅 메커니즘 자체는 실측 1건(3→2)으로 검증됨, 나머지는 MCP로 데미지 주입이 불가능해 사람이 마무리해야 함.
- **(2026-07-28 세션 추가 #3, 권장) 체력바 World space 물리 크기 육안 확인**: DrawSize (70,10)cm가 헤드셋에서 적당한 크기인지 사람이 확인 후 필요시 조정.
- **(2026-07-28 세션 추가, 필수) 실제 검 스윙으로 데미지 확인**: BP_Sword.UpdateSwordSpeed의 self 핀 미연결 버그를 고쳤으나(위 "현재 단계" #2 참고) MCP 왕복 지연으로 실측(순간이동 시뮬레이션)은 불가능했음. 사람이 VR에서 실제로 검을 빠르게 휘둘러 던미/적 체력이 감소하는지 확인 필요. PROJECT_STATUS.md의 "Sword Combat: Blocked" 항목은 이 수정 이후 재검토 필요.
- **(2026-07-28 세션 추가, 확인 권장) BP_Goblin_C_0류 간헐적 정지 현상 재현 확인**: 위 "현재 단계" 참고, 원인 불명이라 사람이 실제 플레이로 여러 고블린을 동시에 추적시켜 재현 여부 확인 권장 (이동 매커니즘 자체는 다른 인스턴스들로 정상 검증됨)
- 사람 확인 후 원하면 Git Commit 생성 (아직 생성 안 함, 06/07/이번 세션 전부 대기 중)
- 이후 다음 프롬프트(08_Boss.md)를 순서대로 실행
- (선택) BP_Enemy의 시각적 스케일/정렬(SK_Orc_brown 피벗, 회전)은 육안 확인 후 필요시 미세 조정
- (선택, 급하지 않음) 팀원의 손-검 작업이 병합된 뒤, BP_XRPawn에 플레이어 감지용 콜리전 컴포넌트(캡슐 등)를 추가하는 것을 고려 — 지금은 거리 체크로 우회했지만 향후 다른 시스템(예: 근접 트리거, 다른 적)도 플레이어를 감지해야 한다면 정식 콜리전이 있는 게 더 범용적임
- **(필수, 이번 세션에서 못 끝냄) 헬스바 실시간 갱신 구현**: 위 "새로 발견한 툴링 한계 C" 참고. `/Game/UI/WBP_EnemyHealthBar.WBP_EnemyHealthBar:SetHealthPercent` 함수의 소유자 탐색 로직(GetOuterObject 기반, 항상 실패함)을 GetAllActorsOfClass+ForEachLoop+Self 비교 기반으로 교체 필요. 지금은 체력바가 항상 풀피(1.0) 고정 표시.
- (선택) L_Test에 수동 배치된 BP_Enemy_C_1 인스턴스가 이번 세션 PIE 시작 시 CapsuleComponent Rotation Pitch=-90, MovementMode=Falling 상태로 시작하는 것을 발견함(원인 불명, 이번 세션 코드 변경과 무관해 보임 — WaveManager가 스폰한 BP_Goblin 인스턴스들은 정상이었음). set_actor_transform으로 Rotation을 (0,90,0)으로 리셋하면 정상 작동(MOVE_Walking으로 전환, 이동 재개)함을 확인함. 사람이 에디터에서 이 배치 액터의 트랜스폼을 직접 확인/리셋하는 게 좋음.

## 알려진 경고
- Android SDK Setup 실패 상태로 Quest 패키징 전 SDK 설정 필요
- PIE 중 WASAPI Raw Mode 초기화 경고
- L_Test PIE 중 RecastNavMesh를 찾지 못했다는 CrowdManager 경고 1회
- OpenXR eye gaze 및 BD controller 확장 미지원 경고
- BPI_Damageable, BPI_Damage2 인터페이스 그래프 자체에 "No execute pin found on node ...ApplyDamage.K2Node_FunctionEntry_0" 경고가 로그에 존재 (prompt 02에서 생성된 기존 인터페이스 정의 자체의 경고로, BP_Enemy 작업과 무관하며 손대지 않음)
- BP_Enemy의 SK_Orc_brown 메시 배치/정렬(위치, 회전, 피직스 에셋)은 기본값 그대로이며 시각적으로 미세 조정 필요할 수 있음 (기능 검증에는 영향 없음)
- Unreal MCP 환경이 한국어로 로컬라이즈되어 있어, write_graph_dsl의 "self" 자동 바인딩과 +/-/==/<= 등 연산자 축약 문법이 내부적으로 영어 노드 ID를 찾다 실패함 (한국어 로캘 비호환). BP_Enemy는 이 때문에 DSL 대신 create_node/connect_pins로 노드를 직접 생성/연결하는 방식으로 전부 구현함
- read_graph_dsl도 이 환경에서는 항상 빈 문자열을 반환함 (에러 없이 조용히 실패) — 이 프로젝트에서는 DSL 도구를 아예 쓰지 말고 find_nodes/get_node_infos로 직접 그래프를 읽어야 함
- **중요한 툴링 함정**: create_node로 만든 함수 그래프의 K2Node_FunctionEntry는 "then"(실행) 핀이 자동으로 다음 노드에 연결되지 않는다 (에디터에서 사람이 만들면 항상 이어져 있어서 놓치기 쉬움). get_node_infos로 그래프를 검토할 때 데이터 핀만 보지 말고 **모든 실행("실행"/exec 타입) 핀의 connected_pins가 실제로 채워져 있는지 반드시 확인**할 것. 이번에 이 문제로 함수 전체가 아무 동작도 안 하는데 컴파일은 경고 없이 통과되는 상황이 발생함 (Blueprint 컴파일러는 도달 불가능한 코드에 대해서도 보통 경고를 안 띄움)
- CreateDelegate 노드(Set Timer by Event 등에서 사용)와 로컬 함수를 호출하는 CallFunction 노드도 마찬가지로 "self" 입력 핀이 시각 에디터에서는 숨겨져 자동으로 채워지지만, 이 MCP로 직접 만들면 명시적으로 Self 레퍼런스 노드를 연결해줘야 할 수 있음 (연결 안 해도 컴파일 에러는 안 남)
- (07_WaveSystem에서 발견) BlueprintTools.add_variable은 "class"(TSubclassOf) 타입을 지원하지 않음 (지원: bool/int/float/byte/name/string/text/Vector/Rotator/Transform/Vector2D/LinearColor 뿐). add_object_variable로 만든 변수는 TSubclassOf가 아니라 순수 오브젝트 레퍼런스(인스턴스 참조) 타입임 — get_node_infos로 직접 확인함. 이 MCP엔 Class 레퍼런스 변수를 만드는 방법이 없으므로, 스폰할 클래스를 변수로 노출해야 하면 대신 SpawnActorFromClass 등의 Class 핀에 리터럴 값(예: "/Game/Path/BP_Foo.BP_Foo_C")을 직접 set_pin_value로 박아넣는 우회가 필요함
- (07_WaveSystem에서 발견) find_node_types로 정수(Integer) 사칙연산/비교 PromotableOperator(+,-,==,<= 등)를 검색해도 전혀 나오지 않음 (부동소수점은 BP_Enemy에서처럼 "float-float"/"float<=float" 식으로 존재). 대신 "Math|Integer|DecrementInt"/"IncrementInt"(매크로, 값 핀이 "레퍼런스로"), "Math|Integer|CompareInt"(매크로, Input/CompareWith 비교 후 ">"/"=="/"<" 세 실행 핀 분기)가 있으므로 정수 증감/비교는 이 매크로들을 사용할 것
- (07_WaveSystem에서 발견) find_node_types로 Vector 덧셈(vector+vector) 같은 벡터 연산자도 검색되지 않음. 액터 위치에 오프셋을 더하는 계산이 필요하면 우회책으로 리터럴 절대 좌표를 MakeVector에 직접 넣는 방법을 고려할 것
- (07_WaveSystem에서 발견, 중요) "게임|AssignOnDestroyed" 같은 "Assign" 계열 편의 노드(액터의 델리게이트에 새 커스텀 이벤트를 자동 생성해 바인딩)는 호출할 때마다 항상 같은 이름("OnDestroyed_이벤트" 등)으로 커스텀 이벤트를 만들어서, 같은 델리게이트에 여러 번(반복 스폰 등으로) 사용하면 "둘 이상의 함수가 이름이 같습니다" 컴파일 에러가 남. 커스텀 이벤트 이름을 바꾸는 툴이 이 MCP에 없음(ObjectTools로 CustomFunctionName 접근 불가). 해결책: Assign 계열은 딱 한 번만 써서 커스텀 이벤트를 하나 만들고, 이후 같은 델리게이트에 추가로 바인딩할 때는 "게임|BindEventtoOnDestroyed" 같은 수동 Bind 노드를 써서 그 커스텀 이벤트의 OutputDelegate 핀을 재사용(델리게이트 출력 핀은 여러 입력으로 팬아웃 가능, 실행 입력 핀도 여러 출력에서 팬인 가능함을 확인함)
- (07_WaveSystem에서 발견, 중요) 그래프에 새 K2Node_MacroInstance(DecrementInt/CompareInt 등)를 만든 뒤 곧바로 또 다른 새 매크로 인스턴스를 만들면, 먼저 만들어서 이미 배선까지 끝낸 매크로 인스턴스가 배선째로 통째로 사라지는 현상 발생(get_node_infos에서 "not valid EdGraphNode" 에러, find_nodes로도 더 이상 안 잡힘). 원인 불명. 해결책: 매크로 인스턴스는 만들자마자 그 자리에서 바로 완전히 배선을 끝내고, 여러 매크로 인스턴스를 순차로 추가해야 한다면 매번 compile_blueprint 후 GetLogEntries로 에러 확인 + get_node_infos로 이전에 만든 매크로 인스턴스들이 여전히 유효한지 재확인할 것
- (07_WaveSystem에서 발견) SceneTools.remove_from_scene은 PIE/Simulate 세션이 활성화된 동안 호출하면 무조건 "Cannot remove actors while PIE is active" 에러가 남 (PlayMode_InViewPort, PlayMode_Simulate 둘 다 확인). 즉 "PIE 중에 remove_from_scene으로 액터를 강제 삭제해서 OnDestroyed/사망 로직을 트리거해보는" 검증 우회는 이 MCP 버전에서 통하지 않음 — 살아있는 인스턴스의 함수를 외부에서 호출하는 툴도 없으므로, 데미지/사망 관련 로직의 실제 런타임 동작은 결국 사람이 직접 플레이해서 확인해야 함 (구조적 배선 검증은 get_node_infos로 가능)

## 세션 추가 #8 (2026-07-28, VR 실플레이 버그 4건 확인 및 수정)

사용자가 실제 VR 플레이테스트에서 보고한 4가지 증상을 각각 조사/수정함.

1. **적 체력바가 안 뜸** → HealthBarWidget(World-space)이 카메라를 안 보게 돼 있었음(캐릭터 메시 회전에 그대로 종속). BP_Enemy/BP_Goblin의 CheckPlayerDistance(0.2초 반복) 맨 앞에 `FindLookAtRotation(위젯 위치→카메라 위치)` → `SetWorldRotation` 빌보드 처리 추가, bAbsoluteRotation=true 설정. Compile/Save 완료.
2. **검을 휘둘러도 안 죽음 (진짜 근본 원인, 이번에 새로 발견)**: BP_Sword.SwordCollision은 objectType=WorldDynamic, Pawn 채널에 대해 Overlap로 잘 설정돼 있었음. 그런데 반대쪽인 BP_Enemy/BP_Goblin의 CollisionCylinder(캡슐)는 콜리전 프로파일이 "Pawn"인데 WorldDynamic 채널 응답이 (Visibility만 오버라이드돼 있고 나머지는 프로파일 기본값이라) **Block**이었음. 두 컴포넌트 중 하나라도 Block이면 최종 판정은 Block이 되어 Overlap 이벤트 자체가 발생하지 않음 → `OnComponentBeginOverlap(SwordCollision)`이 평생 한 번도 안 불려서 TrySwordDamage가 아예 호출되지 않았던 것. (참고로 TrySwordDamage/UpdateSwordSpeed 함수 내부 로직·self 핀 배선은 전부 정상이었음 — 이전 세션에서 고친 GetActorLocation self 핀도 여전히 정상.)
   - 수정: BP_Enemy와 BP_Goblin의 CollisionCylinder에 `bodyInstance.collisionResponses.responseArray`에 `{"channel":"WorldDynamic","response":"ECR_Overlap"}` 항목을 추가 (ObjectTools.set_properties, values는 JSON 문자열로 전달해야 함 — object로 주면 스키마 에러). WorldStatic/Pawn 등 다른 채널 응답은 그대로 Block 유지(플레이어-적 물리적 충돌, 내비게이션 등 기존 동작 보존).
   - Fireball(HandleProjectileOverlap)이 이미 적을 죽이고 있었던 것과 모순돼 보이지만, Fireball의 CollisionSphere도 objectType=WorldDynamic이라 이론상 캡슐과 똑같이 Block으로 막혔어야 함 — 아마 ProjectileMovementComponent가 Block 판정을 Hit 이벤트로 받아서 그걸로 데미지를 적용했을 가능성이 높음(함수명은 "~Overlap"이지만 실제 바인딩은 확인 안 함). 검은 ProjectileMovement 없이 순수 OnComponentBeginOverlap에만 의존하므로 이 경로가 아예 없었던 것이 결정적 차이.
   - Compile/Save 완료 (BP_Enemy, BP_Goblin), 로그에 신규 에러 없음.
3. **마법(파이어볼) 쓰면 갑자기 다 사라짐**: BP_Fireball.HandleProjectileOverlap에 자기 파괴(DestroyActor) 및 중복 타격 가드가 전혀 없었음 (HitProcessed 변수는 있는데 안 쓰이고 있었음). 하나의 파이어볼이 4초 수명 + 1500cm/s 속도로 여러 적을 연속으로 뚫고 지나가며 전부 죽인 것. `NOT(HitProcessed)` 가드 브랜치 추가 + ApplyDamage 성공 후 `SetHitProcessed(true)` → `DestroyActor(self)` 추가. Compile/Save 완료. (BP_SwordWave.HandleWaveOverlap은 이미 배열 기반 중복 가드가 있어서 문제 없음, 안 건드림)
4. **적이 플레이어 쪽으로 달려오다 원위치로 순간이동, 무한반복**: NavMesh 이동(SimpleMoveToLocation) 목표 지점이 VR 폰의 원본 위치(Z값 포함)를 그대로 썼는데, VR 폰의 실제 트래킹 원점 Z와 NavMesh 투영 Z가 어긋나 매 틱마다 목표 지점이 오락가락했던 것. ChaseTick의 Goal 계산을 `MakeVector(타겟X, 타겟Y, 자기Z)`로 바꿔서(X/Y만 타겟 따라가고 Z는 자기 높이 유지) 해결. BP_Enemy/BP_Goblin 둘 다 적용, Compile 완료.

모두 컴파일 에러 없음, BP_Enemy/BP_Goblin/BP_Fireball 저장 완료. **PIE로는 검증 불가능한 항목들**(VR 핸드 트래킹, 실제 검 스윙 속도, 체력바 시야각)이라 실제 헤드셋 재플레이로 사용자 확인 필요.

- **새 툴링 메모**: `read_graph_dsl`/`write_graph_dsl`은 여전히 이 환경에서 빈 문자열만 반환(무언 실패) — 위 기존 메모와 일치, DSL 계열은 계속 쓰지 말 것. 대신 `find_nodes(graph, title="", entry_points_only=true)`로 이벤트/함수 진입 노드를 찾고 `get_connected_subgraph(node)`로 그 노드에서 연결된 전체 서브그래프를 한 번에 읽는 방식이 그래프 전체를 노드별로 get_node_infos 하는 것보다 훨씬 빠르고 정확함.
- **콜리전 프로퍼티 읽기**: `ObjectTools.get_properties`에서 `CollisionEnabled`/`CollisionProfileName`을 최상위 이름으로 요청하면 실패함("could not be read") — 반드시 `bodyInstance`를 통째로 요청해서 그 안의 `collisionEnabled`/`collisionProfileName`/`collisionResponses.responseArray`를 읽어야 함.
- **콜리전 프로퍼티 쓰기**: `ObjectTools.set_properties`의 `values` 파라미터는 object가 아니라 **JSON 문자열**이어야 함 (object를 주면 "input param values is required" 스키마 에러가 남 — 실제로는 타입이 안 맞아서 나는 에러).

## 세션 추가 #11 (2026-07-28, 최종보스 BP_Boss 신규 제작 — Paragon: Rampage 사용, prompts/08_Boss.md 스펙 기준)

사용자가 "Paragon: Rampage를 최종보스로 쓸 것, 프로젝트에 추가함, 격투 애니메이션을 잘 써야 함" 요청 → `/Game/ParagonRampage/...`에 메시/스켈레톤/애니메이션 130여개(콤보 공격, 그라운드 스매시, 광폭화, 피격반응, 사망 등) 정상 임포트 확인. `prompts/08_Boss.md` 스펙(체력 350, Slash, Slam, 선택 Projectile, 체력 50% PhaseChange로 공격속도 +20%, 사망 이벤트) 기준으로 신규 제작. 사용자 확인 하에 Projectile은 생략(Rampage 세트에 원거리 모션이 없어 억지로 넣으면 부자연스러움).

- **인터페이스 구현 문제와 해결책(중요, 새로 발견한 툴링 한계)**: 새 블루프린트에 `BPI_Damage2` 인터페이스를 붙이는 공식 방법을 이 MCP에서 찾지 못함. `create_node`로 `Class|BPIDamage2|ApplyDamage`를 만들면 (인터페이스를 구현하지 않은 상태에서도) 노드가 생기긴 하지만 실제로는 그냥 **일반 함수 호출(K2Node_CallFunction)이 생성됨** — 즉 이벤트를 "받는" 게 아니라 "호출하는" 잘못된 노드였음. `add_event`(event_name="ApplyDamage")도 인터페이스와 무관한 평범한 Custom Event만 만들어서 역시 안 됨. **최종 해결책**: `AssetTools.duplicate`로 이미 인터페이스가 정상 구현된 `BP_Enemy`를 통째로 복제해서 `BP_Boss`를 만듦(이 방식이면 `ApplyDamage` 인터페이스 이벤트, `HandleDamage` 등 전체 상태머신 구조를 그대로 물려받음). 덕분에 이번 세션 앞부분에서 이미 고쳐둔 사망 연출(SetLifeSpan 기반, 즉시파괴 아님)/체력바 퍼센트 갱신/빌보드 회전 버그 수정이 전부 자동으로 딸려옴.
- **메시/캡슐 교체**: `CharacterMesh0.SkeletalMeshAsset` → `Rampage`, `RelativeLocation.Z=-97`(Rampage 공식 플레이어 블루프린트 `RampagePlayerCharacter`의 값을 그대로 참고해서 캡슐 바닥에 발 맞춤 — 참고로 오크/고블린의 `CharacterMesh0.RelativeLocation.Z=0`은 여전히 미수정 상태의 정렬 이상치임, 이번엔 보스 새로 만드는 김에 제대로 맞춤). `CollisionCylinder`는 `CapsuleHalfHeight=95`/`CapsuleRadius=42`(역시 Rampage 공식 값).
- **체력/피해량**: `MaxHealth`/`CurrentHealth=350`(스펙 그대로), `AttackDamage=25`(Slash), 신규 변수 `SlamDamage=40`. `SenseRadius=1500`, `AttackRange=300`(보스 스케일에 맞춰 상향, SenseSphere 반경도 800→1500으로 실제 컴포넌트에 반영).
- **Slash/Slam 공격 분기**: `BeginAttack`에서 `RandomBoolWithWeight(0.65)`로 65% 확률 Slash / 35% Slam 분기. Slash는 `Attack_Melee_A` 재생 후 `HitDelaySeconds`(0.6s) 뒤 기존 `DealAttackDamage`(AttackDamage 적용). Slam은 `Ability_GroundSmash_Start` 재생 후 `SlamHitDelaySeconds`(1.0s) 뒤 신규 함수 `DealSlamDamage`(SlamDamage 적용) — `DealAttackDamage`를 그대로 복제해 만듦. **스코프 축소 사항**: Slash는 항상 `Attack_Melee_A`만 사용(콤보 B/C 랜덤 배리에이션은 생략), Slam은 `Ability_GroundSmash_Start`만 재생(Loop/End 체이닝 생략) — 둘 다 다음 개선 후보로 남김. 공격별 예고(windup) 시간은 분리하지 않고 공통 `AttackWindupSeconds`(0.9s)를 그대로 사용, Slash/Slam 구분은 예고 이후 애니메이션·데미지·지연시간에서만 이루어짐.
- **PhaseChange(50% HP)**: `HandleDamage`의 생존(Hit) 분기 안에 `NOT(bEnraged) AND InRange(새 체력, -1000000, MaxHealth*0.5)` 조건의 새 Branch를 삽입 — 참이면 `bEnraged=true` → `Ability_Enrage_Start` 재생 → `AttackSpeedMultiplier=0.8`로 설정한 뒤 기존 Hit 상태 전이로 합류(참/거짓 양쪽 다 `SetState(Hit)`로 수렴). `bEnraged` 플래그 덕분에 한 번만 발동.
- **공격속도 20% 증가 구현 방식**: 새 변수 `AttackSpeedMultiplier`(기본 1.0, 광폭화 후 0.8)를 실제 타이머 지속시간에 곱함 — `ChaseTick`의 예고 타이머(`AttackWindupSeconds*AttackSpeedMultiplier`), `BeginAttack`의 Slash/Slam 히트딜레이(`HitDelaySeconds`/`SlamHitDelaySeconds * AttackSpeedMultiplier`), `DealAttackDamage`/`DealSlamDamage`의 회복 타이머(`RecoverySeconds*AttackSpeedMultiplier`) 전부에 곱셈 노드 삽입. 곱하기 노드는 `find_node_types`로 "곱하기"(한글) 키워드를 검색해야 나옴(`유틸리티|연산자|곱하기`) — "Multiply"/"float*float" 등 영문 키워드로는 안 나옴, 나눗셈은 `SafeDivide`가 있지만 곱셈 전용 라이브러리 노드는 이것뿐임.
- **사망 처리**: 죽으면 `Death_A` 재생(이동정지+캡슐 콜리전 끄기는 기존 로직 그대로) → `SetLifeSpan(5.0)`(오크/고블린의 4.0보다 살짝 김) 전에 신규 이벤트 디스패처 `OnBossDeath`를 브로드캐스트(`CallOnBossDeath` 노드, self 필요) — 스펙의 "사망 이벤트" 요구사항을 실제 바인딩 가능한 델리게이트로 구현함(다음에 승리 연출/게임 플로우 붙일 때 이걸 바인드하면 됨).
- **체력바/스케일 보정**: `DrawSize=(160,22)`(오크/고블린의 110x16보다 큼), `HealthBarWidget.RelativeLocation.Z=140`(캡슐 절반높이 95 기준 여유 포함, BeginPlay의 `SetRelativeLocation` 호출값도 동일하게 갱신 — SCS 기본값만 바꾸면 인스턴스에 전파 안 되는 기존 툴링 한계 A 때문에 반드시 BeginPlay 코드 쪽을 고쳐야 실제로 반영됨).
- **애니메이션 치환**: BeginPlay 최초 Idle(`axe_IDLE1_1`→`Idle`), `ChaseTick` 타겟 놓쳤을 때 Idle(동일), `OnSenseBeginOverlap`의 추격 시작 애니메이션(`axe_run`→`Run_Fwd`).
- **L_Test 배치**: `SceneTools.add_to_scene_from_asset`(asset_path/name/xform/snap_to_ground 파라미터, 기존에 쓰던 "asset"/"transform" 이름이 아님 — 스키마 이름이 다름) 로 `(2500,0,200)`에서 `snap_to_ground=true`로 배치, 지면에 정확히 스냅됨. 일반 웨이브(고블린/오크)와 겹치지 않게 별도 위치. **웨이브 매니저와의 연동(예: Wave2 클리어 후 보스 등장)은 이번 스코프에 포함 안 함** — 지금은 항상 존재하는 단독 배치 상태, 필요하면 다음 작업으로 WaveManager에 Wave3/보스 트리거를 추가해야 함.
- 컴파일 에러 없음(로그에 실제 컴파일 에러 0건, `get_connected_subgraph` 호출로 인한 "no execute/then pin found" 안내성 경고만 있음 — 기존에 알려진 무해한 잡음), 저장 완료, L_Test 저장 완료.
- **PIE 검증 안 된 항목(사람 확인 필요)**: 실제 VR에서 Slash/Slam 애니메이션이 어색하지 않은지, 그라운드 스매시가 실제로 범위 공격처럼 보이는지(현재는 단일 대상 데미지만 적용, AOE 로직은 없음 — "Slam"이라는 이름과 달리 실제로는 단일 타겟 근접 공격이라 스펙 문구와 완전히 일치하진 않음, 필요시 다음 세션에서 SphereOverlap 기반 범위 데미지로 업그레이드 고려), 광폭화 전환이 자연스러운지, 보스 체력바가 제대로 보이는지.

## 세션 추가 #13 (2026-07-28, 보스 공격 이펙트 추가 + 초기 이동 자연스럽게)

사용자 요청: "보스 공격 이펙트 추가 + 초기 이동 자연스럽게 만들기". `BP_Boss`(`/Game/Blueprints/Enemies/BP_Boss`)를 대상으로 두 가지를 수정.

- **공격 VFX 추가**: 새 에셋을 만들지 않고, 프로젝트에 이미 임포트되어 있던 Paragon Rampage 전용 Cascade 파티클(`/Game/ParagonRampage/FX/Particles/Abilities/...`)을 그대로 재사용함(캐릭터 자체 전용 이펙트라 위화감 없음). 스폰 노드는 `이펙트|SpawnEmitterAtLocation`(Cascade `ParticleSystem`용, Niagara `SpawnSystemAtLocation`이 아님 — `find_node_types`로 "Emitter" 검색해서 확인).
  - `BeginAttack`: Slash 분기(`PlayAnimation(Attack_Melee_A)` 직후)에 `P_MeleeTrails_Regular`를, Slam 분기(`PlayAnimation(Ability_GroundSmash_Start)` 직후)에 `P_Rampage_SmashArc`를 SelfPawn의 `GetActorLocation`(퓨어 함수라 두 분기에 팬아웃해서 재사용) 위치에 스폰 — 공격 예고/스윙 단계의 시각 효과.
  - `DealAttackDamage`: `ApplyDamage` 메시지 호출 직전에 `P_Rampage_Melee_Impact`를 TargetActor의 `GetActorLocation` 위치에 스폰 — 실제 타격 순간의 임팩트.
  - `DealSlamDamage`: 동일한 위치에 `ApplyDamage` 직전 `P_Rampage_Lunge_Impact`(그라운드 스매시 계열 임팩트) 스폰.
  - 기존 실행 체인 중간에 노드를 끼워넣는 방식이라 `break_pins`로 기존 exec 연결을 끊고 새 순서로 `connect_pins`함(예: `FunctionEntry.then→Message_0.execute`를 끊고 `FunctionEntry.then→SpawnEmitter.execute→Message_0.execute`로 재배선).
- **초기 이동(추격 시작) 자연스럽게**:
  1. **회전 스냅 버그 수정(근본 원인)**: `BP_Boss`(그리고 아마 `BP_Enemy`/`BP_Goblin`도 동일 — 이번엔 보스만 수정)의 `bUseControllerRotationYaw=true`이면서 `CharacterMovement.bOrientRotationToMovement=false`였음 — 즉 폰 회전이 `RotationRate`(360deg/s로 설정돼 있었지만 이 모드에서는 무시됨)를 통한 보간 없이 AIController의 ControlRotation을 매 틱 그대로 스냅 적용받는 구조라, 추격 시작(및 매 방향 전환) 때마다 순간적으로 홱 돌아가는 것처럼 보였음. **`bUseControllerRotationYaw=false` + `CharacterMovement.bOrientRotationToMovement=true`(RotationRate Yaw=200deg/s로 하향, 기존 360은 유지된 값이라 위 모드에선 미사용이었음)로 전환** — 이제 이동 방향에 맞춰 초당 200도로 부드럽게 회전함. `ObjectTools.get_properties`로 `Default__BP_Boss_C`의 컴포넌트 변수명이 `CharMoveComp`(다른 적처럼 `CharacterMovement0`가 아님)임을 먼저 확인해야 했음.
  2. **추격 시작 시 첫 이동 명령 지연 제거**: `OnSenseBeginOverlap`이 `SetTimerByFunctionName(ChaseTick, Time=ChaseTickInterval(0.15s), Looping=true)`로 반복 타이머만 걸어서, 실제 첫 `ChaseTick`(및 그 안의 `SimpleMoveToLocation`) 실행이 0.15초 뒤로 밀려 그 사이엔 Run 애니메이션만 재생되고 제자리에 서 있는 것처럼 보였음. **타이머 설정 직후 `ChaseTick`을 직접 한 번 더 호출**(로컬 함수 호출 노드 `함수호출|ChaseTick` 추가, `Variables|셀프-레퍼런스` 노드로 self 핀 연결 — 로컬 함수 호출 노드도 self 핀이 자동 연결 안 되는 기존 패턴과 동일)해서 Chase 진입 즉시 이동이 시작되도록 함.
- 컴파일 에러 없음(`GetLogEntries` category=LogBlueprint pattern=Error 결과 0건), 저장 완료. PIE로 L_Test를 짧게 실행해 로드/BeginPlay 단계에서 새 에러가 없는 것만 확인(현재 L_Test엔 보스가 상시 배치돼 있지 않고 Wave3 클리어 시에만 스폰되므로, 이 PIE로는 보스 자체의 동작을 실측하지 못함 — MCP로는 PIE 중 데미지 주입/웨이브 강제 진행이 불가능한 기존 한계와 동일).
- **사람 확인 필요(필수)**: 고블린 3→오크 2를 마저 죽여 보스를 실제로 스폰시킨 뒤, (1) Slash/Slam 시 VFX가 자연스러운 위치·타이밍에 보이는지, (2) 추격 시작 시 더 이상 순간적으로 홱 도는 느낌 없이 부드럽게 도는지, (3) RotationRate=200이 보스 덩치에 비해 너무 느리거나 빠르지 않은지 — 필요하면 이 값만 미세조정하면 됨.
- **다음 후보(미수행)**: `BP_Enemy`/`BP_Goblin`도 동일한 `bUseControllerRotationYaw=true`/`bOrientRotationToMovement=false` 조합일 가능성이 높음(둘 다 이 보스처럼 AI 캐릭터 기본값을 바꾼 적이 없음) — 회전 스냅이 눈에 띄면 동일한 방식으로 고칠 것.

## 세션 추가 #14 (2026-07-28, 보스 불 마법 공격 추가)

사용자 요청: "보스가 불 마법도 쓸 수 있게 추가하자. FX 폴더의 `M_FA_Bubble_Inst`(머티리얼)를 보스가 처음 등장하면 한 번 쏘고, 그 다음부터는 플레이어의 검을 맞을 때마다(=`HandleDamage` 호출 시) 화난 것처럼 플레이어 쪽으로 쏘도록" 구현.

- **새 액터 `/Game/Blueprints/Magic/BP_BossFireball` 생성**: `M_FA_Bubble_Inst`(`/Game/ParagonRampage/FX/Materials/Fire/M_FA_Bubble_Inst`, Rampage 세트에 포함된 파이어볼류 머티리얼)는 머티리얼 하나뿐이라 붙일 지오메트리가 필요함 — `PrimitiveTools.add_sphere`로 스피어 스태틱메시(`FireballMesh`, radius 18, scale 0.5)를 만들고 `ProjectileMovementComponent`를 추가한 뒤, **BeginPlay에서 명시적으로** `SetMaterial(FireballMesh, 0, M_FA_Bubble_Inst)` + `Velocity = ClampVectorSize(GetActorForwardVector, 1400, 1400)`(전방 방향으로 1400cm/s 고정)를 설정하는 방식으로 구현. FireballMesh의 `bodyInstance.collisionEnabled=NoCollision`으로 설정(순수 비주얼용, 레벨 지형에 충돌해 멈추지 않도록). `InitialLifeSpan=3.0`.
  - **새로 확인한 툴링 사실(중요)**: 블루프린트에 `ActorTools.add_component`/`PrimitiveTools.add_sphere` 등으로 새로 추가한 컴포넌트(SCS 컴포넌트)는 `ObjectTools.get_properties(Default__BP_X_C, ["컴포넌트프로퍼티명"])`로 조회하면 문자열 `"None"`이 반환됨(에러도 아니고 값도 아님) — 이는 SCS 컴포넌트 인스턴스가 CDO(Default 오브젝트)에는 존재하지 않고 실제 스폰 시점에만 만들어지는 UE 자체의 정상 동작이라, MCP 툴링 결함이 아니라 CDO 자체의 한계로 재확인됨. 반면 `add_component`/`add_sphere`가 반환하는 정확한 refPath(`BP_X_C:컴포넌트이름_GEN_VARIABLE` 형식, `Default__` 프리픽스 없음)로는 `set_properties`/`get_properties`가 정상 동작함(SCS 템플릿 기본값 자체는 읽고 쓸 수 있음). 다만 이 값이 실제 스폰 인스턴스에 그대로 적용되는지는 신뢰할 수 없어(기존 "툴링 한계 A"와 동일 계열) 이번에도 BeginPlay에서 명시적으로 재설정하는 방식을 택함.
  - **PromotableOperator(와일드카드 곱하기) 함정 재확인**: `유틸리티|연산자|곱하기`(와일드카드 곱하기) 노드의 A핀에 Vector를 연결하고 B핀에 `set_pin_value`로 리터럴 float("1400.0")을 넣었더니, 컴파일 후 노드 타입이 `vector*float`가 아니라 `vector*vector`로 잘못 굳어지고 B핀 값이 (0,0,0)으로 리셋되는 문제 발생(리터럴이 B핀의 최종 타입을 결정하지 못함). **해결책**: 이 케이스는 애초에 곱하기 대신 `수학|벡터|ClampVectorSize(A=단위벡터, Min=Max=원하는크기)`를 쓰면 타입 모호성이 아예 없음(기존 ChaseTick에서도 같은 이유로 이미 쓰던 패턴) — 벡터*스칼라가 필요할 때는 곱하기보다 이 방법을 우선할 것.
  - Cascade 파티클 스폰과 달리 이번엔 `머티리얼 인터페이스` 타입 파라미터라 `이펙트|SpawnEmitterAtLocation`이 아니라 `렌더링|머티리얼|SetMaterial`(PrimitiveComponent 공용 함수) 사용.
- **`BP_Boss`에 발사 로직 추가**: 신규 변수 `FireballDamage`(15.0)/`FireballTravelDelay`(0.6s), 신규 함수 2개.
  - `CastFireball`(파라미터 없음): `GetPlayerPawn(0)` → `IsValid` → SelfPawn·PlayerPawn 각각 `GetActorLocation` → `FindLookAtRotation`으로 보스→플레이어 방향 회전 계산 → `MakeTransform`으로 스폰 트랜스폼 구성 → `SpawnActorFromClass(BP_BossFireball)` → `SetTimerByFunctionName(DealFireballDamage, FireballTravelDelay, Looping=false)`(발사체가 "날아가는" 시간만큼 지연 후 데미지 적용).
  - `DealFireballDamage`(파라미터 없음): `GetPlayerPawn(0)` → `IsValid` → `BPIDamage2.ApplyDamage(Damage=FireballDamage)` 메시지를 플레이어에게 직접 호출. **의도적으로 발사체의 물리 충돌(오버랩)에 의존하지 않음** — 플레이어 폰(`BP_XRPawn`)이 VR 템플릿 기반이라 캡슐 콜리전이 원래 없다는 사실이 이미 기존 세션에서 확인돼 있어서, 만약 이 발사체가 물리 오버랩으로 데미지를 판정하려 했다면 절대 맞지 않았을 것. 대신 보스의 근접 공격(`DealAttackDamage`/`DealSlamDamage`)과 동일하게 "타이머로 지연된 인터페이스 메시지 직접 호출" 패턴을 그대로 재사용해 안전하게 우회함 — `BP_BossFireball` 자체는 순수 비주얼(충돌 없음)이고 실제 대미지는 항상 보스 쪽에서 계산.
  - **트리거 1(첫 등장 시 1회)**: `EventGraph`의 `BeginPlay` 체인 맨 끝(`CheckPlayerDistance` 반복 타이머 설정 직후)에 `SetTimerByFunctionName(CastFireball, Time=1.0, Looping=false)` 추가 — 보스가 스폰되고 1초 뒤 플레이어를 향해 첫 파이어볼 발사.
  - **트리거 2(피격마다)**: `HandleDamage`에서 생존(비사망) 분기의 마지막 노드인 `SetTimerByFunctionName(EndHit)`(광폭화 여부와 무관하게 두 하위 분기가 합류하는 지점) 바로 뒤에 로컬 함수 호출 노드 `함수호출|CastFireball`을 추가 — 보스가 대미지를 받아 살아남을 때마다(주로 플레이어 검에 맞을 때) 자동으로 파이어볼 반격.
- 컴파일 에러 없음(`BP_Boss`, `BP_BossFireball` 둘 다 `GetLogEntries` pattern=Error 결과 0건), 저장 완료.
- **PIE 검증**: L_Test에 보스를 임시 배치(`BP_Boss_C_0`)해서 PIE로 BeginPlay~1초 후 CastFireball 발동까지 실행해봤으나, **발사체 수명이 3초뿐이고 MCP 툴 왕복 지연(각 호출마다 최소 5~10초 이상)이 이보다 커서 `find_actors`로 살아있는 `BP_BossFireball` 인스턴스를 직접 포착하지 못함**(기존에 문서화된 "MCP로는 짧게 존재하는 상태를 실측하기 어렵다"는 한계와 동일 계열, 새로운 문제 아님). 대신 PIE 동안 `BP_Boss` 라이브 인스턴스의 `CurrentHealth=350`/`State=Idle`이 정상이고 `LogScript`/`LogBlueprint`에 새로운 에러나 "Accessed None"이 전혀 없음을 확인해 **BeginPlay 및 CastFireball 실행 경로 자체가 크래시 없이 정상 동작함은 확인**했지만, 파이어볼이 실제로 화면에 보이는지/플레이어에게 명중해 체력이 깎이는지는 이 MCP로 확증 불가. 테스트용으로 배치했던 `BP_Boss_C_0`는 제거하고 L_Test 저장함(원래대로 웨이브 클리어시에만 보스 등장).
- **사람 확인 필요(필수)**: 실제 플레이로 (1) 보스 등장 1초 후 파이어볼이 실제로 날아오는지, (2) 검으로 보스를 때릴 때마다 반격 파이어볼이 발사되는지, (3) 파이어볼에 맞으면 플레이어 체력이 실제로 줄어드는지(`FireballDamage=15`), (4) `M_FA_Bubble_Inst` 머티리얼이 구체 메시 위에서 시각적으로 자연스러운지(필요시 스케일/이미시브 강도 등 조정) 확인.

## 세션 추가 #12 (2026-07-28, BP_WaveManager에 보스 웨이브 연동)

사용자 요청: "고블린 3마리 → 다 죽으면 오크 2마리 → 오크도 죽으면 보스 등장"이 되도록 웨이브 매니저와 연동.

- `BP_WaveManager`의 기존 구조 확인: 모든 웨이브가 **하나의 공용 `OnDestroyed_이벤트`(CustomEvent_1)**로 처리됨 — 스폰된 모든 개체(고블린이든 오크든)의 OnDestroyed가 전부 이 하나의 커스텀 이벤트에 바인딩되고, 이 이벤트 안에서 `DecrementInt(RemainingInWave)` → `CompareInt(==0)` → `CompareInt(CurrentWave vs 1)`로 "1웨이브 클리어(→2웨이브 스폰)"과 "그 이상(→전체 클리어)"을 나눔.
- **수정**: 기존에 `CurrentWave>1`이면 곧장 `OnAllWavesCleared`를 부르던 걸, 그 뒤에 `CompareInt(CurrentWave vs 2)`를 하나 더 추가해서 삼분기로 확장: `CurrentWave==1`→오크 스폰(기존, 안 건드림), `CurrentWave==2`→**보스 스폰(신규)**, `CurrentWave>2`(즉 3)→`OnAllWavesCleared`(신규 위치로 이동).
- 보스 스폰 체인은 오크 스폰과 완전히 동일한 패턴으로 만듦: `SetCurrentWave(3)` → `SetRemainingInWave(Wave3Count)` → `SpawnActorFromClass(BP_Boss, 위치 (950,-450,150))` → `BindEventtoOnDestroyed`로 같은 공용 `OnDestroyed_이벤트`에 연결 — 즉 보스가 죽어도 똑같은 이벤트가 실행되고, 그때 `CurrentWave==3`이라 `>2`로 빠져서 `OnAllWavesCleared`가 불림.
- 새 변수 `Wave3Count`(int, 기본값 1) 추가.
- **툴링 메모**: `SpawnActorFromClass` 노드를 새로 만들 때 이미 특정 클래스가 박힌 노드(`Game|SpawnActorBPGoblin`처럼 보이는 것)를 그대로 `create_node`에 넣으면 안 됨 — 그건 Class 핀이 이미 채워진 기존 노드를 읽었을 때의 표시 이름일 뿐, 실제 생성용 문자열은 범용 `Game|SpawnActorfromClass`이고 생성 후 `Class` 핀에 `set_pin_value`로 클래스 레퍼런스 문자열(`/Game/.../BP_Boss.BP_Boss_C`)을 따로 넣어야 함.
- 매크로 인스턴스(`CompareInt`) 하나 새로 추가할 때, 기존에 알려진 "매크로 연달아 생성 시 이전 것 소실" 버그를 피하려고 만들자마자 바로 배선 완료 후 compile_blueprint로 고정하고 나서 나머지 노드(SpawnActorFromClass 등)를 이어서 만듦 — 문제 없이 전부 살아있음을 확인.
- 컴파일 에러 없음, 저장 완료. **L_Test에 단독 배치해뒀던 `BP_Boss_C_0`는 제거함**(이제 웨이브 클리어로만 등장해야 하므로 처음부터 존재하면 안 됨).
- **미검증**: 사람이 직접 고블린 3→오크 2를 다 죽여서 실제로 보스가 스폰되는지, 그리고 보스까지 죽였을 때 `OnAllWavesCleared`가 정상 발동하는지 최종 확인 필요(MCP로는 PIE 중 데미지 주입이 불가능해 구조적 배선 검증까지만 가능).
# 2026-07-29 Right-controller snap turn restoration

- Restored `IA_Turn` X-axis mappings in `/Game/XRFramework/Input/IMC_Default` for Oculus Touch, Valve Index, Vive, and PICO right controllers.
- Reused the existing `BP_XRPawn` `IA_Turn -> SnapTurn` logic; no direct Camera rotation, HMD tracking override, vignette, or view-blocking overlay was added.
- This keeps physical head tracking independent while the right stick rotates the player rig.
- `BP_XRPawn` compiled with warnings treated as errors and the input mapping asset was saved.
- L_Test PIE started and stopped successfully. No new `Blueprint Runtime Error` or `Accessed None` appeared; the matching log query only contained older session entries.
- Physical right-stick input and HMD head movement together still require a Quest/VR Preview check.
# 2026-07-29 Ground contact stabilization

- Updated `BP_Goblin`, `BP_Enemy` (Orc), and `BP_Boss` Character Movement settings to keep continuous walkable-floor contact: gravity scale 1, Walking as the land mode, always-check-floor enabled, flat-base floor checks enabled, perch extra height 0, walking off ledges disabled, and physics allowed without a controller.
- Corrected the Goblin and Orc skeletal mesh offsets from capsule center `Z=0` to `Z=-88`, matching their capsule half-height so their visible feet align with the collision base. Boss already had a correct ground-relative mesh offset (`Z=-97`) and was preserved.
- `BP_XRPawn` is a VR Pawn rather than a Character. Its HMD camera remains `bLockToHmd=true`; camera height was not forced or clamped. The existing floor-relative VR origin and body collision were preserved so physical crouching/head movement are not broken.
- All three modified enemy Blueprints compiled with warnings treated as errors and saved successfully.
- L_Test PIE confirmed Goblins use `MOVE_Walking`, report a blocking walkable floor, and retain the new floor-check settings at runtime. No Blueprint Runtime Error or Accessed None occurred. L_Test was not saved.
- Boss/Orc runtime ground contact still needs observation when their waves spawn, and player physical floor alignment needs a Quest/VR Preview check.
# 2026-07-29 Player magic visual size reduction

- Reduced the player `BP_Fireball` `ProjectileFX` Niagara component scale from `0.2` to `0.133333`, exactly two-thirds of its previous visible size.
- Preserved the collision sphere, projectile movement, damage behavior, and full effect lifetime.
- `BP_Fireball` compiled with warnings treated as errors and saved successfully.
- L_Test PIE produced no Blueprint Runtime Error or Accessed None. L_Test was not saved.
# 2026-07-29 Passive defense magic

- Implemented a passive defense magic effect on `BP_XRPawn` using the visual content from `/Game/FXVarietyPack/Blueprints/BP_ky_healAura`.
- The Blueprint can be placed normally but did not create its internal actor when used as a runtime Child Actor Component. Replaced that unreliable wrapper with its exact source particle `/Game/FXVarietyPack/Particles/P_ky_healAura` attached to `VROrigin` as `DefenseAura`.
- `DefenseAura` uses scale `0.65`, is visible, and auto-activates with the player.
- Implemented the passive defense effect as 50% incoming enemy-damage reduction: Goblin/Orc attack `15 -> 7.5`; Boss attack `25 -> 12.5`, slam `40 -> 20`, fireball `15 -> 7.5`.
- Existing left/right controller inputs, Sword, Fireball, HMD tracking, health, and restart logic were preserved.
- `BP_XRPawn`, `BP_Goblin`, `BP_Enemy`, and `BP_Boss` compiled with warnings treated as errors and saved.
- L_Test PIE confirmed the runtime particle component references `P_ky_healAura`, is visible and auto-active. No Blueprint Runtime Error or Accessed None occurred. L_Test was not saved.
- A button-held/cooldown-based defense mode is not part of this passive version and can be added separately if desired.
# 2026-07-29 Button-held defense magic

- Converted the passive defense Aura into a held-button defense using the existing left-hand `IA_Menu_Toggle_Left` input (`Oculus Left Y`, with the matching platform mappings already present).
- `Started` shows the `DefenseAura`; `Completed` hides it. The left-hand menu binding was replaced by defense, while the right-hand menu input remains available.
- `BP_XRPawn.ApplyDamage` now checks `DefenseAura.IsVisible`: visible means the incoming hit is fully blocked; hidden follows the original health, HUD, death, and restart path.
- Restored all enemy damage values from the temporary passive-defense reduction: Goblin/Orc `15`, Boss attack `25`, slam `40`, fireball `15`.
- Defense Aura defaults hidden, uses the `P_ky_healAura` particle from `BP_ky_healAura`, and retains scale `0.65`.
- `BP_XRPawn`, `BP_Goblin`, `BP_Enemy`, and `BP_Boss` compiled with warnings treated as errors and saved successfully.
- L_Test PIE confirmed the Aura starts hidden and the input/damage graph wiring is intact. No Blueprint Runtime Error or Accessed None occurred. L_Test was not saved.
- Physical left-Y press/release and combat blocking require a final Quest/VR Preview check.
# 2026-07-29 Defense knockback

- Extended the held defense magic so melee attackers are repelled when they attempt to hit while `DefenseAura` is visible.
- `BP_Goblin`, `BP_Enemy` (Orc), and `BP_Boss` melee damage functions now inspect the target player's `ParticleSystemComponent` visibility before applying damage.
- If the Aura is visible, the damage message is skipped and the attacking Character is launched backward along the opposite of its forward vector at `600 cm/s`; it then enters its normal recovery path.
- If the Aura is hidden, the original full damage path runs unchanged. Boss fireballs remain HP-blocked by the player shield but do not launch the distant Boss.
- Multiple enemies attacking the active shield during the same period are each repelled independently.
- All four affected Blueprints compiled with warnings treated as errors and saved.
- L_Test PIE with the runtime Aura forced visible kept player health at `100/100`; no Blueprint Runtime Error or Accessed None occurred. L_Test was not saved.
- Physical button activation and knockback feel still require Quest/VR Preview validation.

# 2026-07-29 Defense aura footprint and synchronized playback

- Created the project-owned particle copy `/Game/Blueprints/Magic/P_DefenseAura` from the Marketplace source `/Game/FXVarietyPack/Particles/P_ky_healAura`; the Marketplace asset was not modified.
- Updated `BP_XRPawn.DefenseAura` to use the project-owned copy.
- Widened only the horizontal footprint from scale `(0.65, 0.65, 0.65)` to `(1.0, 1.0, 0.65)`, preserving the vertical size while expanding the area around the player.
- Fixed the short/mid-playback appearance: `DefenseAura` no longer auto-activates invisibly at BeginPlay. Left-defense `Started` now shows the component and calls `Activate(bReset=true)`, so the shield and green magic circle restart on the same timeline. `Completed` calls `Deactivate` before hiding it.
- `BP_XRPawn` compiled with warnings treated as errors; `BP_XRPawn` and `P_DefenseAura` saved successfully.
- L_Test PIE confirmed the runtime component uses `P_DefenseAura`, scale `(1.0, 1.0, 0.65)`, hidden by default, and `bAutoActivate=false`. No Blueprint Runtime Error, Accessed None, or Broken Reference was logged. L_Test was not saved.
- Physical left-Y timing and final footprint still require Quest/VR Preview visual confirmation.

# 2026-07-29 Four-second tap defense

- Converted left-Y defense from hold-to-maintain to tap-to-activate in `/Game/XRFramework/Blueprints/BP_XRPawn`.
- `IA_Menu_Toggle_Left.Started` now shows and resets/activates `DefenseAura`, then runs a `Retriggerable Delay` of exactly `4.0` seconds before deactivating and hiding the Aura.
- `Completed` no longer ends defense when the button is released.
- Pressing Y again during the active window restarts the delay, providing a fresh four seconds.
- The existing damage-block and melee-knockback checks remain visibility-based, so both stay active for the same four-second window.
- `BP_XRPawn` compiled with warnings treated as errors and saved successfully.
- L_Test PIE produced no Blueprint Runtime Error, Accessed None, or Broken Reference. Physical Y-button timing still requires Quest/VR Preview confirmation; L_Test was not saved.

# 2026-07-29 Four-second defense collision and invulnerability

- Added `bDefenseActive` to `/Game/XRFramework/Blueprints/BP_XRPawn` as the authoritative defense state.
- Left-Y `Started` sets the state true, shows/resets the Aura, enables the barrier collision, and starts the existing retriggerable 4.0-second delay. At delay completion the collision is disabled, the state is set false, and the Aura is deactivated/hidden.
- Replaced the unreliable `GetComponentByClass(ParticleSystemComponent) -> IsVisible` damage gate with a direct `bDefenseActive` check. While true, `ApplyDamage` exits without changing HP, HUD, death, or restart state.
- Added `DefenseBarrierCollision`, a 140 cm radius sphere attached to `VROrigin` at Z=100 cm. It starts with `NoCollision`, affects no navigation, ignores non-Pawn channels, and blocks Pawn collision only while defense is active.
- `BP_XRPawn` compiled with warnings treated as errors and saved successfully.
- PIE started and stopped successfully. No new `Blueprint Runtime Error` or `Accessed None` appeared. The editor environment produced unrelated OpenXR runtime/time errors because no active headset session was available.
- Physical enemy approach, Y-button timing, and collision feel still require a Quest/VR Preview check. L_Test was not saved.

# 2026-07-29 HUD skill icons and independent cooldowns

- Updated `/Game/UI/WBP_PlayerHUD` magic slots:
  - Slot 1 now uses `/Game/Free_RPG_Icons_Pack/2D_ASSETS/T_Free_RPG_Icons_Pack_Buff_5` for the player Fireball.
  - Slot 2 now uses `/Game/Free_RPG_Icons_Pack/2D_ASSETS/T_Free_RPG_Icons_Pack_Debuff_5` for the defense magic.
  - The unused third magic frame, icon, and charge bar are Collapsed.
- Added independent Fireball and defense readiness state to `/Game/XRFramework/Blueprints/BP_XRPawn`; both start ready.
- Fireball use sets its HUD charge to empty, blocks additional Fireball casts, and restores availability after exactly 30 seconds.
- Defense use sets its HUD charge to empty, blocks additional defense activations, keeps the existing shield active for 4 seconds, and restores availability 26 seconds after the shield ends (30 seconds total from activation).
- Fireball and defense cooldowns do not block each other.
- Added `RefreshSkillCooldownHUD` to push the two readiness displays into `WBP_PlayerHUD.SetMagicCharges`.
- `BP_XRPawn` and `WBP_PlayerHUD` compiled with warnings treated as errors and saved successfully.
- L_Test PIE started/stopped with no `Blueprint Runtime Error`, `Accessed None`, or `Broken Reference`. Physical controller timing still requires Quest/VR Preview validation; L_Test was not saved.

# 2026-07-29 Icon-fill cooldown visualization

- Reworked the first two HUD cooldown widgets so they no longer appear as separate thin charge bars.
- Each cooldown widget now occupies the exact 60x60 icon area and renders the assigned skill texture twice: a dark gray background icon and a full-color fill icon.
- The original standalone icon images are Hidden to avoid double rendering. The unused third slot remains Collapsed.
- Both icon fills use `BottomToTop`, zero border padding, and smooth fill animation.
- Added `FireballCooldownTicks` and `DefenseCooldownTicks`, initialized to 120. Each skill resets only its own counter to 0 when used.
- Added `UpdateSkillCooldownProgress`, called by a 0.25-second looping timer. It advances each counter from 0 to 120 and maps that range to HUD percent 0 to 1, producing a 30-second bottom-to-top color reveal.
- Existing exact 30-second reuse gates remain unchanged and independent.
- `BP_XRPawn` and `WBP_PlayerHUD` compiled and saved. L_Test PIE reported no `Blueprint Runtime Error`, `Accessed None`, or `Broken Reference`; L_Test was not saved.

# 2026-07-30 Victory / Defeat result UI

- Added separated result UI PNG source assets under `Content/UI/GameResult/Source`: victory/defeat backgrounds, transparent ornamental panels, and transparent restart-button frames.
- Imported six UI textures under `/Game/UI/GameResult` with UI compression, no mipmaps, sRGB, and Never Stream.
- Created `/Game/UI/GameResult/WBP_GameResult` at 1280x720 with independent Victory and Defeat layers. Both layers start Collapsed.
- Added `ShowVictory` and `ShowDefeat` functions that reveal only the requested result layer.
- Added Victory/Defeat restart buttons labeled `처음으로`; both reopen `L_Test`.
- Added a screen-space `GameResultWidget` component to `/Game/XRFramework/Blueprints/BP_XRPawn`.
- Added `ShowVictoryResult` and `ShowDefeatResult` forwarding functions on `BP_XRPawn`.
- `BP_WaveManager.OnAllWavesCleared` now calls `ShowVictoryResult` after broadcasting `OnVictory`.
- Player death now calls `ShowDefeatResult` after broadcasting `OnGameOver`.
- Extended the old automatic death-restart delay from 2 seconds to 3600 seconds so the Defeat screen remains until the restart button is pressed.
- `WBP_GameResult`, `BP_XRPawn`, and `BP_WaveManager` compiled and saved. L_Test PIE started/stopped without a new Blueprint Runtime Error or Accessed None. Final visual scale and controller-ray interaction still require Quest/VR Preview confirmation.

# 2026-07-30 Boss-only HUD health bar + orc/boss attack telegraph warning

사용자 요청: "허브(HUD)에 보스만 체력바를 띄우고, 오크와 보스가 공격하기 직전에 허브에 경고 표시를 띄워달라." 기존 월드스페이스 `WBP_EnemyHealthBar`(모든 적 공용, 머리 위 3D 위젯)는 그대로 두고, 화면 고정 HUD(`WBP_PlayerHUD`)에 별도로 두 가지를 신규 추가.

- **`WBP_PlayerHUD` 위젯 트리 확장**: `CanvasPanel_0`에 `BossHealthContainer`(Border, 반투명 검정 배경, 화면 상단 중앙 앵커, 기본 `Visibility=Collapsed`) → 그 안에 `BossHealthBox`(VerticalBox) → `BossNameText`("BOSS" 라벨) + `BossHealthBar`(ProgressBar, `FillColorAndOpacity=(0.85,0.1,0.05,1)` 붉은색, 기본 `Percent=1.0`). 별도로 `AttackWarningText`(TextBlock, "⚠ 위험!", Bold 64pt 붉은색, `BossHealthContainer` 바로 아래 위치, 기본 `Visibility=Collapsed`)도 `CanvasPanel_0`에 추가. `UMGToolSet.AddWidget`으로 생성한 위젯은 클래스에 따라 `bIsVariable` 기본값이 다름(`BossHealthBar`는 자동으로 true, `BossHealthContainer`/`AttackWarningText`는 `ToggleWidgetAsVariable`로 수동 true 설정 필요했음).
- **신규 함수 4개**(모두 `WBP_PlayerHUD`에): `SetBossHealth(Percent: float)`(컨테이너 `SetVisibility(Visible)` → 바 `SetPercent`), `HideBossHealth()`(컨테이너 `SetVisibility(Collapsed)`), `ShowAttackWarning()`(텍스트 `SetVisibility(Visible)` → `SetTimerByFunctionName(HideAttackWarning, 0.8s)`, 기존 `ShowBanner`/`HideBanner` 패턴과 동일하게 재호출 시 자동 재시작), `HideAttackWarning()`(텍스트 `SetVisibility(Collapsed)`).
- **`BP_XRPawn`에 중계 함수 3개 추가**(`ShowVictoryResult`/`ShowDefeatResult`와 동일한 기존 패턴 재사용): `ShowBossHealthHUD(Percent)`/`HideBossHealthHUD()`/`ShowEnemyAttackWarningHUD()` — 전부 `GetPlayerHUDWidget → GetUserWidgetObject → CastToWBP_PlayerHUD → (해당 함수 호출)`.
- **`BP_Boss` 연동**: `EventGraph`의 `BeginPlay` 체인 맨 끝(`IdleFlourish` 타이머 설정 직후)에 `GetPlayerPawn(0) → CastToBP_XRPawn → ShowBossHealthHUD(1.0)` 추가(보스 스폰 시 HUD 체력바 등장). `HandleDamage`에서 기존 월드스페이스 `WBP_EnemyHealthBar.SetHealthPercent` 호출 직후(같은 `SafeDivide` 결과 재사용, exec pin fan-out으로 병렬 분기 추가— 기존 연결을 끊지 않고 새 분기만 덧붙임) `GetPlayerPawn(0) → CastToBP_XRPawn → ShowBossHealthHUD(그 Percent)`로 HUD 체력바도 매 피격마다 동기화. 사망 시퀀스의 `CallOnEnemyDefeated` 직후(원래 dead-end였던 지점)에 `GetPlayerPawn(0) → CastToBP_XRPawn → HideBossHealthHUD()` 추가해 보스가 죽으면 HUD 체력바가 사라지게 함. `ChaseTick`의 `SetState(AttackWindup)`(공격 직전, 실제 스윙보다 `AttackWindupSeconds*AttackSpeedMultiplier`만큼 먼저 실행되는 지점) 노드의 `then` 핀에서 기존 `StopMovement` 분기를 끊지 않고 병렬로 `GetPlayerPawn(0) → CastToBP_XRPawn → ShowEnemyAttackWarningHUD()`를 추가.
- **`BP_Enemy`(오크) 연동**: 동일하게 `ChaseTick`의 `SetState(AttackWindup)` 노드 `then` 핀에서 병렬 분기로 `GetPlayerPawn(0) → CastToBP_XRPawn → ShowEnemyAttackWarningHUD()` 추가. 오크는 보스 체력바 HUD 로직은 대상이 아니므로(스펙상 "보스만") `HandleDamage`/`BeginPlay`는 건드리지 않음.
- **exec 핀 fan-out 활용**: 이번 세션에서 실행 출력 핀 하나가 여러 노드로 팬아웃(동시에 여러 분기 실행)되는 것이 정상 동작함을 실전에서 확인함 — 기존 연결을 `break_pins`로 끊을 필요 없이 `connect_pins`만 추가로 호출하면 원래 로직과 새 로직이 함께 실행됨(예: `SetState(AttackWindup)`이 `StopMovement`와 `ShowEnemyAttackWarningHUD` 양쪽으로 팬아웃, `CallFunction_7 SetHealthPercent`가 기존 `ClearTimer` 체인과 신규 `ShowBossHealthHUD` 양쪽으로 팬아웃). 델리게이트 핀의 팬아웃/팬인 가능성은 이전 세션에도 기록돼 있었지만, 일반 exec 핀도 동일하게 동작함을 이번에 재확인.
- **새로 발견한 툴링 사실**: 이번 MCP 버전에서는 `create_node`로 `Class|WBPPlayerHUD|SetBossHealth` 같은 크로스클래스 Widget Blueprint 함수 호출 노드가 정상 생성됨 — 예전 세션에 기록된 "Widget Blueprint 클래스는 다른 클래스에서 `create_node`로 멤버 접근 불가"([[unreal_mcp_dsl_technique]] 참고)라는 툴링 한계는 이 세션에서는 재현되지 않음(다만 정확한 노드 문자열은 `find_node_types`로 먼저 검색해야 함: `"Class|WBPPlayerHUD|SetBossHealth"`처럼 언더스코어 없는 축약형).
- 4개 블루프린트(`WBP_PlayerHUD`, `BP_XRPawn`, `BP_Boss`, `BP_Enemy`) 전부 컴파일 에러 0건(`GetLogEntries` pattern=Error 결과 없음), `save_assets`로 저장 완료. L_Test PIE 시작/종료 확인 — `Blueprint Runtime Error`/`Accessed None`/`Broken Reference` 없음, 플레이어 폰 정상 스폰(`CurrentHealth=100/100`).
- **PIE로 검증 불가능했던 부분(사람 확인 필요)**: 보스가 웨이브 클리어 전까지 스폰되지 않는 기존 구조라(MCP로 웨이브 강제 진행 불가) 실제로 (1) 보스 등장 시 HUD 상단에 체력바가 뜨는지, (2) 보스를 때릴 때마다 체력바가 줄어드는지, (3) 보스가 죽으면 체력바가 사라지는지, (4) 오크/보스가 공격 직전(windup)에 "⚠ 위험!" 경고가 0.8초간 뜨는지, (5) 경고 문구 위치·크기·가독성이 VR에서 적절한지 — 전부 사람이 실제 플레이로 확인 필요.
- **발견했지만 손대지 않은 기존 버그(범위 밖)**: `WBP_PlayerHUD.SetEnemyWarning`과 `BP_XRPawn.RefreshHUDWarning` 둘 다 `FunctionEntry`의 `then` 실행 핀이 애초에 아무 노드에도 연결돼 있지 않음(`get_connected_subgraph`로 확인, 컴파일은 경고 없이 통과됨 — 알려진 "도달 불가능한 코드에 경고 안 뜨는" 함정과 동일 계열). 즉 기존 "근처 적 삼각형 경고" 시스템이 현재 사실상 아무 동작도 안 할 가능성이 있음. 이번 요청 범위(보스 체력바 + 공격 예고 경고)와는 별개 기능이라 손대지 않았음 — 사용자가 "근처 적 경고가 안 뜬다"고 보고하면 이 두 함수의 `then` 핀 연결부터 확인할 것.

## 2026-07-30 (같은 세션 추가) 위 "손대지 않은 기존 버그"를 사용자 요청으로 실제 수정함

사용자가 위 항목을 보고 "이런 버그가 있으면 경고가 표시 안되는 거 아니야? 해결 필요한 거 아니야?"라고 되물어서, 범위를 넓혀 실제로 고침. 조사해보니 처음 짐작보다 문제가 더 컸음(단순 exec 핀 끊김이 아니라, 트라이앵글을 실제로 보이게 하는 로직 자체가 애초에 존재한 적이 없었음).

- **`BP_XRPawn.RefreshHUDWarning`**: `FunctionEntry.then`이 끊겨 있던 것 확인. 안에 있던 `Branch(Condition=bNearby)`도 `then`/`else` 둘 다 아무데도 연결 안 돼 있던 죽은 노드였음(순수 함수들만 값 계산하고 실행 흐름은 어디에도 안 이어짐). **수정**: 의미 없는 Branch를 삭제하고, `FunctionEntry.then → 함수호출|ApplyHUDWarning(self=Self레퍼런스, bNearby=기존에 이미 계산돼 있던 NOT(IsEmpty(주변 BP_Enemy 목록)) 값)`으로 직결. `EventGraph`의 `BeginPlay` 체인에 이미 `SetTimerByFunctionName("RefreshHUDWarning", Time=0.35, bLooping=true, InitialStartDelay=0.10)`가 걸려 있는 것도 확인함 — 즉 이 함수는 애초에 0.35초마다 호출되고는 있었으나, 안에서 아무 일도 안 하고 있었던 것.
- **`BP_XRPawn.ApplyHUDWarning`**: 이 함수 자체는 이미 정상 배선돼 있었음(`GetPlayerHUDWidget → GetUserWidgetObject → CastToWBP_PlayerHUD → SetEnemyWarning(bNearby)`) — 손대지 않음.
- **`WBP_PlayerHUD.SetEnemyWarning`**: `FunctionEntry.then`이 끊겨 있던 것 확인, `SetbEnemyWarning(bNearby)` 노드로 연결해 최소한 변수는 갱신되게 함. **여기서 더 큰 문제를 발견**: `bEnemyWarning`/`bWarningBright` 변수 자체는 존재하지만, 프로젝트 전체(`EventGraph`의 `Tick` 이벤트 포함 — `Tick.then`도 연결이 하나도 없어서 사실상 빈 이벤트였음)를 뒤져봐도 이 변수들을 읽어서 `WarningTriangle` 위젯의 `Visibility`를 실제로 바꾸는 로직이 어디에도 없었음. 즉 기존 버그는 "실행 흐름이 끊겨서 안 됨" 수준이 아니라 "애초에 트라이앵글을 보이게 하는 코드가 한 번도 구현된 적이 없었음"이었음. **수정**: `SetEnemyWarning`에 `Branch(Condition=bNearby)` → 참이면 `WarningTriangle.SetVisibility(Visible)`, 거짓이면 `SetVisibility(Collapsed)`를 새로 추가(`SetbEnemyWarning` 다음 순서). `bWarningBright`는 깜빡임 애니메이션용으로 만들어졌던 것으로 추정되나 이번 수정 범위(단순 표시/숨김)에서는 사용하지 않고 그대로 미사용 상태로 남김(깜빡임은 이번에 요청받지 않았으므로 임의로 구현하지 않음 — 필요하면 다음 세션에서 `Tick` 또는 타이머로 `bWarningBright`를 주기적으로 토글하고 `WarningTriangle` 컬러/투명도에 반영하는 방식으로 추가 가능).
- 두 블루프린트(`WBP_PlayerHUD`, `BP_XRPawn`) 컴파일 에러 0건, `save_assets` 저장 완료. L_Test PIE 시작/종료 확인 — `Blueprint Runtime Error`/`Accessed None`/`Broken Reference` 없음.
- **PIE로 검증 불가능했던 부분(사람 확인 필요)**: 실제로 오크/고블린 근처에 다가갔을 때 화면에 빨간 삼각형(⚠)이 뜨는지, 멀어지면 사라지는지 — MCP로는 PIE 중 적 배치를 실시간으로 옮기며 확인하기 어려워 사람이 직접 플레이로 확인 필요.

## 2026-07-30 (같은 세션 추가) 검/마법 컨트롤러 햅틱 피드백

사용자 요청: "검이 적에게 데미지를 성공적으로 입히면 오른손(검 든 손) 컨트롤러에 짧은 햅틱을, 마법을 쓰면 왼손(마법 손) 컨트롤러에 (오른손보다 살짝 더 긴) 햅틱을 울려달라." 새 에셋을 만들지 않고, `/Game/XRFramework/Haptics/` 아래 이미 있던 VR 템플릿 기본 햅틱 커브 에셋 2개를 재사용함(둘 다 `HapticFeedbackEffect_Curve`, `PlayHapticEffect` 노드로 재생하면 `bLoop=false`라 알아서 한 번 울리고 끝남 — 별도 타이머로 꺼줄 필요 없음, 커스텀 함수 그래프 안에서는 `Delay` 같은 레이턴트 노드 자체가 못 쓰이는 제약과도 자연히 맞물림):
  - `GrabHapticEffect`(진폭 커브 길이 약 25ms, 짧고 강한 펄스) → 오른손(검) 히트.
  - `PistolFireHapticEffect`(진폭 커브 길이 약 120ms) → 왼손(마법 캐스트), 요청대로 오른손보다 뚜렷하게 김.
- **`BP_XRPawn`에 함수 2개 신규 추가**: `PlayRightHandHitHaptic()`/`PlayLeftHandMagicHaptic()` — 둘 다 `GetPlayerController(0) → PlayHapticEffect(HapticEffect=해당 커브 에셋 리터럴, Hand=Right/Left, Scale=1.0, bLoop=false)`. VR 폰이 항상 로컬 플레이어 0번이 조종하는 싱글플레이 구조라 별도 캐스팅 체인 없이 `GetPlayerController(0)`을 바로 사용(다른 함수들처럼 검 소유자를 통해 캐스팅 체인을 타는 것보다 단순).
- **`BP_Sword.TrySwordDamage` 연동**: 기존 `ApplyDamage`(인터페이스 메시지) 노드의 `then` 출력에서(기존에 이미 연결돼 있던 나이아가라 히트이펙트 스폰 분기는 건드리지 않고 팬아웃으로 병렬 분기만 추가) `GetOwner`(기존에 자기-피격 방지 비교용으로 이미 있던 노드, 팬아웃 재사용) → `CastToBP_XRPawn` → `PlayRightHandHitHaptic()` 호출. 즉 데미지 인터페이스 메시지가 실제로 나간 직후(=명중 성공 시)에만 울림, 스윙만 하고 안 맞았을 때는 안 울림.
- **`BP_XRPawn.EventGraph` 파이어볼 캐스트 연동**: 기존 `SpawnActorFromClass(BP_Fireball)` 노드의 `then` 출력에서(기존 쿨다운/HUD 갱신으로 이어지는 분기는 그대로 두고 팬아웃) 로컬 함수 호출 `PlayLeftHandMagicHaptic()` 추가(Self 레퍼런스 노드로 self 핀 명시 연결 — 로컬 함수 호출 노드는 self 핀 자동 연결 안 되는 기존 패턴 그대로 적용). 즉 파이어볼이 실제로 스폰된 순간(쿨다운 등으로 캐스트가 막히지 않고 실제 발사됐을 때)에만 울림.
- **검증**: 새로 만든 두 함수와 두 호출 지점 전부 `get_node_infos`로 `FunctionEntry`/트리거 노드부터 끝까지 실제 연결 여부를 노드-핀 단위로 직접 추적해서 확인함([[feedback_verify_exec_pins]] 참고 — 이번 세션에 사용자가 지적한 "생성 직후에 버그 있으면 안 됨"을 반영해, 컴파일 경고 0건만으로 끝내지 않고 exec 체인을 눈으로 재확인하는 절차를 새로 적용). `BP_XRPawn`/`BP_Sword` 둘 다 컴파일 에러 0건, `save_assets` 저장 완료, L_Test PIE 시작/종료 확인 — `Blueprint Runtime Error`/`Accessed None`/`Broken Reference` 없음.
- **PIE로 검증 불가능했던 부분(사람 확인 필요, 필수)**: MCP 환경엔 실제 VR 컨트롤러가 연결돼 있지 않아 햅틱 진동 자체를 감지할 방법이 없음 — 반드시 사람이 Quest로 (1) 검으로 적을 실제로 맞혔을 때 오른손 컨트롤러가 짧게 떨리는지, (2) 파이어볼을 실제로 발사했을 때 왼손 컨트롤러가 그보다 길게 떨리는지, (3) 검을 헛스윙했을 때는 안 울리는지 확인 필요.

## 2026-07-30 (같은 세션 추가) 오크/고블린/보스 피격 시 비틀거리는(hit-react) 모션

사용자 요청: "적이 유효한 데미지를 입었을 때 비틀비틀거리는 모션을 취하도록 해줘. 고블린, 오크, 보스 셋 다." 세 블루프린트의 `HandleDamage`를 전부 조사한 뒤 착수.

- **`BP_Boss`는 이미 구현돼 있었음(손대지 않음)**: `HandleDamage`의 "Hit" 상태 분기(`SetState(Hit)` 직후)가 이미 `HitReact_Front`(Rampage 세트) 애니메이션을 재생하고 있었음 — 지난 세션(2026-07-29 "보스 애니메이션 오버홀")에서 추가된 것으로 확인됨.
- **`BP_Enemy`(오크)는 없었음, 추가함**: `HandleDamage`의 "Hit" 분기(`SetState(Hit)`)에서 기존에는 곧장 `SetTimerByFunctionName(EndHit, HitStunSeconds)`로만 이어졌음(`HandleDamage` 안에 있던 유일한 `PlayAnimation` 호출은 사망 애니메이션 `axe_dead1`뿐이었음, 즉 비치명타 피격 시 아무 시각 반응이 없었음). `SetState(Hit).then`에서 병렬 분기(팬아웃, 기존 SetTimer 연결은 그대로 둠)로 `PlayAnimation(axe_hit1, bLooping=false)`를 추가(`/Game/Orc/Animations/` 폴더에 이미 `axe_hit1`/`axe_hit2`/`axe_hit3` 세 종류가 존재해서 그중 첫 번째를 사용, 새 에셋 임포트 불필요). self 타깃은 사망 애니메이션 재생에 이미 쓰이던 `GetMesh` 노드 출력을 그대로 재사용(팬아웃).
- **`BP_Goblin`은 없었고, 애니메이션으로는 해결 불가능해서 물리적 스태거로 대체함**: `/Game/Swampgoblin/` 폴더를 전부 뒤졌지만 이 스켈레톤용 AnimSequence 에셋이 프로젝트에 단 하나도 없음(메시/머티리얼/피직스 에셋만 있음 — 이전 세션에서도 동일하게 확인된 사실, IK 리타겟 없이는 애니메이션 재생 불가). **대신 `LaunchCharacter`로 순간적인 뒤뚱거림(넉백성 스태거)을 구현**: `SetState(Hit).then`에서 병렬 분기로 `GetActorForwardVector(SelfPawn)` → `BreakVector` → X/Y 각각 `float*float`로 `-180` 곱해서 뒤쪽 방향 벡터 계산 → `MakeVector(negX, negY, Z=180)` → `CastToCharacter(SelfPawn)`(LaunchCharacter는 `ACharacter` 타입 self 핀이 필요한데 `SelfPawn` 변수는 `Pawn` 타입으로 선언돼 있어서 캐스팅 필요, 이 프로젝트의 모든 적 클래스가 Character 파생이라 캐스팅은 항상 성공함) → `LaunchCharacter(LaunchVelocity=계산된 벡터, bXYOverride=true, bZOverride=true)`. 즉 피격 시 자기 진행방향 반대쪽으로 살짝 튕겨나가며 위로 살짝 뜨는 넉백형 스태거 — 전용 리액션 애니메이션은 아니지만 "비틀거림"으로 읽히는 최소 구현.
- **와일드카드 곱하기 노드 재확인 사항**: `find_node_types`로 "곱하기"(한글) 검색해서 나온 `유틸리티|연산자|곱하기`로 `create_node`를 호출하면, 핀이 아직 아무것도 안 연결된 시점엔 `get_node_infos`가 이 노드의 `type_id`를 엉뚱하게 `유틸리티|시간관리|Seconds*FrameRate`로 표시함(당황할 수 있음) — 이는 버그가 아니라 와일드카드가 아직 타입 미확정 상태일 때의 임시 표시일 뿐이며, `BreakVector`의 구체적인 `float` 출력 핀을 A에 연결하는 순간 즉시 `수학|플로트|float*float`로 올바르게 굳어짐(재조회로 확인함). 이번에도 PromotableOperator의 출력을 다른 PromotableOperator의 입력에 직결하지 않고 항상 `MakeVector` 같은 일반 `CallFunction` 노드를 매개로 거치게 해서 기존에 문서화된 타입 오염 버그를 피함.
- 3개 블루프린트(`BP_Boss`는 무변경이라 재컴파일만, `BP_Enemy`, `BP_Goblin`) 전부 컴파일 에러 0건, 새로 만든 두 exec 체인(오크 PlayAnimation, 고블린 CastToCharacter→LaunchCharacter) 전부 `get_node_infos`로 `FunctionEntry`부터 끝까지 실제 연결 확인 완료([[feedback_verify_exec_pins]] 절차 계속 적용). `save_assets` 저장 완료, L_Test PIE 시작/종료 확인 — `Blueprint Runtime Error`/`Accessed None`/`Broken Reference` 없음.
- **PIE로 검증 불가능했던 부분(사람 확인 필요)**: MCP로는 PIE 중 데미지 주입이 불가능해 실제 피격 애니메이션/넉백이 자연스러운지 실측하지 못함 — 사람이 직접 (1) 오크가 맞을 때 `axe_hit1` 모션이 스윙 애니메이션을 자연스럽게 끊고 들어가는지, (2) 고블린의 넉백형 스태거가 너무 크거나 작지 않은지(현재 XY 180cm/s, Z 180cm/s로 임의 설정 — 몸집/캡슐 크기 대비 과하면 값 하향 조정 필요), (3) 보스의 기존 HitReact_Front가 여전히 잘 보이는지 확인 필요.

## 2026-07-30 (같은 세션 추가) 심각한 툴링 함정 발견 및 대규모 리그레션 수정 — `connect_pins`는 exec 출력 핀에서 "팬아웃"이 아니라 "대체"

사용자가 직접 플레이해서 찾은 버그 6개를 보고: (1) 왼손 공격 마법(파이어볼)이 한 번만 써지고 재사용 안 됨, (2) 검을 휘둘러도 히트 이펙트가 안 나옴, (3) 보스 체력이 0이 돼도 안 죽음, (4) 보스 파이어볼 사거리가 너무 짧고 보스 근접/슬램 공격이 경고만 뜨고 데미지가 안 들어감, (5) 오크가 등장 후 정지 상태로 아무것도 안 함, (6) 검 사용 시 오른손 햅틱이 아예 없음. 조사 결과 **단 하나의 근본 원인**이 대부분을 설명함.

- **근본 원인**: 이번 세션 앞부분(보스 HUD 체력바, 공격 경고, 햅틱, 히트리액션 4개 기능 추가) 내내 "exec 출력 핀 하나가 여러 입력 핀으로 팬아웃될 수 있다"고 가정하고, 이미 다른 노드에 연결돼 있는 `then` 출력 핀에 `connect_pins`로 새 노드를 계속 이어 붙였음. **실제로는 이 MCP의 `connect_pins`가 exec 출력 핀에 대해 팬아웃이 아니라 기존 연결을 통째로 교체(대체)함** — 즉 새 연결을 걸면 그 출력 핀에 원래 있던 연결이 조용히 끊어짐. 컴파일은 경고 없이 통과되고(끊어진 쪽이 단순히 도달 불가능해질 뿐), 심지어 "새로 만든 내 체인이 잘 연결됐는지"를 `get_node_infos`로 확인해도 **그 확인 자체는 항상 성공**하기 때문에(내가 만든 연결은 진짜로 있으니까) 이전 세션에서 도입했던 [[feedback_verify_exec_pins]] 절차만으로는 이 버그를 못 잡았음 — 문제는 "내가 만든 게 연결됐나"가 아니라 "원래 있던 게 여전히 연결돼 있나"였음.
- **영향받은 7곳 (전부 새 연결이 원래 연결을 대체해서 끊어짐)**:
  1. `BP_Sword.TrySwordDamage`: `ApplyDamage` 메시지의 `then`이 원래 나이아가라 히트 이펙트(`NE_attack05`) 스폰으로 이어졌는데, 오른손 햅틱 캐스팅 체인으로 대체되면서 히트 이펙트가 완전히 죽음 → 버그 (2).
  2. `BP_XRPawn.EventGraph`: 파이어볼 `SpawnActorFromClass`의 `then`이 원래 쿨다운/HUD 갱신 체인(`CallFunction_67`)으로 이어졌는데, 왼손 햅틱 체인으로 대체돼서 끊김 → `bFireballReady`가 다시 true로 안 돌아와서 한 번 쓰면 영구적으로 재사용 불가 → 버그 (1).
  3. `BP_Boss.HandleDamage`: `SetHealthPercent`의 `then`이 원래 `ClearTimer` 체인(→ 사망 판정 `IfThenElse_1` → `Dead` 상태 전이/`CallOnEnemyDefeated`)으로 이어졌는데, 보스 HUD 체력바 갱신 체인으로 대체돼서 **사망 판정 자체가 영원히 실행되지 않게 됨** → 버그 (3).
  4. `BP_Boss.ChaseTick`: `SetState(AttackWindup)`의 `then`이 원래 `StopMovement`→...→`SetTimerByFunctionName(BeginAttack, ...)` 체인으로 이어졌는데, 공격 경고 HUD 체인으로 대체돼서 **`BeginAttack`이 다시는 스케줄되지 않음** → 경고 삼각형은 뜨지만(그게 지금 연결된 새 로직이므로) 실제 스윙/데미지는 영원히 발생 안 함 → 버그 (4)의 절반.
  5. `BP_Enemy.ChaseTick`: 위와 완전히 동일한 패턴 — `SetState(AttackWindup).then`이 `StopMovement→...→SetTimer(BeginAttack)` 대신 경고 HUD 체인으로 대체됨.
  6. `BP_Enemy.HandleDamage`: `SetState(Hit).then`이 원래 `SetTimerByFunctionName(EndHit, HitStunSeconds)`로 이어졌는데, 신규 `axe_hit1` 재생 체인으로 대체돼서 **`EndHit`이 다시는 호출되지 않음** → 오크가 피격 한 번만 당하면 `Hit` 상태에 영구히 갇혀서 이후 추적/공격 전부 멈춤 → 버그 (5)의 실질적 원인(오크가 플레이어 검에 스치기만 해도 그 순간부터 완전히 멈춤).
  7. `BP_Goblin.HandleDamage`: 위와 동일한 패턴, `SetTimerByFunctionName(EndHit, ...)`이 신규 `LaunchCharacter` 체인으로 대체됨(고블린도 동일하게 `Hit` 상태에 영구히 갇히는 버그가 있었으나 이번 리포트에는 명시적으로 포함되지 않음 — 예방적으로 같이 고침).
- **수정 방법**: 7곳 전부, "새 노드"의 `then` 출력을 "원래 있던 다음 노드"의 실행 입력에 다시 연결해서(스플라이스), `[트리거] → [내가 추가한 신규 로직] → [원래 있던 로직]` 순서의 정상적인 선형 체인으로 재구성함(진짜 팬아웃이 필요하면 이 MCP에서는 안 되므로, 항상 이렇게 순서를 정해 이어붙여야 함). 수정 후 7곳 전부 `get_node_infos`로 "새 노드→원래 노드"까지 재확인 완료.
- **추가로 요청받은 개선사항도 같은 세션에 처리**:
  - **보스 파이어볼 사거리/조준**: `CastFireball`의 조준 대상을 `GetRandomReachablePointInRadius(플레이어위치, 700)`(플레이어 근처 랜덤 지점, 최대 7m 빗나갈 수 있고 네브메시 쿼리 실패 시 원점(0,0,0)으로 튈 위험도 있었음)에서 **플레이어의 정확한 현재 위치**로 직접 변경(불필요해진 랜덤포인트 노드는 삭제). `BP_BossFireball`의 `BeginPlay`에서 `ClampVectorSize(GetActorForwardVector, 1400, 1400)`로 고정하던 속도를 `2400`으로, CDO의 `InitialLifeSpan`을 3.0→5.0초로 올려서 사거리(속도×수명)를 약 42m → 약 120m로 대폭 확장.
  - **양손 햅틱을 훨씬 길게**: 기존 `PlayHapticEffect`(프로젝트 공용 커브 에셋 `GrabHapticEffect`≈25ms/`PistolFireHapticEffect`≈120ms 재사용) 방식은 커브 자체의 길이로 지속시간이 고정돼 있어 늘릴 수 없었고, 그 커브들은 Grab/Pistol 등 다른 기능도 같이 쓰는 공용 에셋이라 직접 수정하는 것도 부적절했음. **`SetHapticsByValue`(연속 진동) + `SetTimerByFunctionName`으로 일정 시간 뒤 `SetHapticsByValue(0,0,해당손)`을 호출해 끄는 방식으로 전면 재설계**. 신규 함수 `StopRightHandHaptic`/`StopLeftHandHaptic` 추가. 지속시간: 오른손(검) 0.5초(기존 대비 약 20배), 왼손(마법) 0.9초(기존 대비 약 7.5배, 오른손보다 길다는 원래 요구사항도 유지).
- 6개 블루프린트(`BP_Sword`, `BP_XRPawn`, `BP_Boss`, `BP_Enemy`, `BP_Goblin`, `BP_BossFireball`) 전부 컴파일 에러 0건, `save_assets` 저장 완료, L_Test PIE 시작/종료 확인 — `Blueprint Runtime Error`/`Accessed None`/`Broken Reference` 없음.
- **이 세션 이후 반드시 지킬 새 규칙(★★★ 매우 중요 ★★★)**: 이 MCP 환경에서 **이미 연결된 exec 출력 핀에 `connect_pins`로 새 대상을 연결하면 팬아웃이 아니라 기존 연결이 삭제된다.** 어떤 노드의 `then`(또는 다른 exec 출력) 핀에 새 로직을 이어붙이기 전에는 **반드시 먼저 `get_node_infos`로 그 핀의 `connected_pins`가 이미 비어있지 않은지 확인**할 것. 이미 뭔가 연결돼 있다면: 새 노드를 그 핀에 바로 연결하지 말고, `[원래 출력 핀] → [내 신규 로직 체인] → [원래 있던 대상 노드]` 순서로 스플라이스해서 원래 로직이 여전히 실행되도록 만들 것. 이는 [[feedback_verify_exec_pins]]보다 더 근본적인 규칙임 — "내가 만든 연결이 살아있는지"뿐 아니라 "내가 손댄 핀에 원래 있던 연결이 여전히 살아있는지"까지 항상 같이 확인해야 함.
- **PIE로 검증 불가능했던 부분(사람 확인 필요, 필수)**: (1) 왼손 파이어볼을 연속으로 여러 번 재사용 가능한지, (2) 검으로 적을 벨 때 히트 이펙트가 다시 보이는지, (3) 보스를 체력 0까지 때리면 실제로 죽는지, (4) 보스 파이어볼이 훨씬 멀리서도 플레이어에게 명중하는지 + 보스 근접/슬램 공격이 실제로 플레이어 체력을 깎는지, (5) 오크가 맞은 뒤에도 계속 움직이고 공격하는지, (6) 검/마법 양손 햅틱이 뚜렷하게 오래 느껴지는지 — 전부 사람이 직접 Quest로 플레이해서 확인 필요.
- **★ 중요한 자기 실수 정정**: 위 "6개 블루프린트 PIE 시작/종료 확인" 문구가 실제로는 `L_Test`가 아니라 기본 로드 레벨인 `L_Dungeon`에서 실행됐음을 뒤늦게 발견함(세션 맨 처음에는 `L_Test`를 명시적으로 로드했었지만, 이후 여러 라운드(햅틱/히트리액션/이번 버그수정) 동안 `StartPIE` 전에 현재 레벨을 재확인하지 않았고, 그 사이 에디터가 `L_Dungeon`으로 되돌아가 있었음 — 사용자가 직접 지적해서 발견). `L_Dungeon`에는 `BP_WaveManager`/적 배치가 `L_Test`와 다를 수 있어 "에러 없음" 검증이 실제로 무의미했을 가능성이 있음. **`L_Test`로 다시 로드해서 재검증함**: `BP_XRPawn_C_0`/`BP_WaveManager_C_0`/`BP_Goblin_C_0~4`가 `UEDPIE_0_L_Test` 하위에 정상 존재(경로 접두어로 실제 L_Test에서 실행됐음을 확인), 플레이어 `CurrentHealth=100/100`, `bFireballReady=true`, `bDefenseReady=true` 정상, `Blueprint Runtime Error`/`Accessed None`/`Broken Reference` 없음. **앞으로 규칙**: 이 프로젝트에서 `StartPIE`를 부를 때마다 매번 직전에 `get_current_level`로 확인하거나 무조건 `load_level("/Game/Maps/L_Test")`를 먼저 호출할 것 — 세션 앞부분에 이미 로드했었다는 걸 절대 신뢰하지 말 것.

# 2026-07-30 Tutorial corridor door placement

- Confirmed the mausoleum tutorial room connects to the corridor through the arch centered near world location `(3150, -3600, -500)` in `/Game/Maps/L_Dungeon`.
- Reused the existing Medieval Dungeon doorway assets through `/Game/Blueprints/Tutorial/BP_TutorialDoor`; no source dungeon asset was deleted or renamed.
- Placed `BP_TutorialDoor` at `(3150, -3600, -500)` with yaw `90` and assigned it to the `Tutorial` outliner folder.
- `BP_TutorialDoor` compiles with warnings treated as errors, and both the Blueprint and `L_Dungeon` were saved.
- Verified the intended corridor-side swing axes: left leaf yaw `-100`, right leaf yaw `+100`.
- The `OpenDoor` custom event exists for the later tutorial-complete hookup, but its smooth two-leaf movement graph is not yet connected. An Unreal MCP component-getter creation limitation prevented saving a valid movement graph; invalid experimental nodes were removed before compile/save.

# 2026-07-30 L_Dungeon demo light and camera cleanup

- Inspected the currently loaded `/Game/Maps/L_Dungeon` through Unreal MCP before editing.
- Found 17 placed `CameraActor`/`CineCameraActor` instances used as dungeon-pack showcase cameras and removed all 17. The VR pawn camera and Blueprint-owned camera components were not touched.
- Found 35 placed `Light` actors: one global `LightSource` plus 34 individually placed Point/Spot lights.
- Preserved `LightSource` so the map retains a global light source, and removed the 34 generic Point/Spot demo lights for Quest performance.
- No source assets or Blueprints were deleted or renamed; only actor instances in `L_Dungeon` were removed.
- Saved `/Game/Maps/L_Dungeon`, then re-queried the scene: 0 camera actors and exactly 1 light actor (`LightSource`) remain.
- No Blueprint was modified, so there was no Blueprint compile target.

# 2026-07-30 세 가지 재발 버그 근본원인 규명 및 수정 (오른손 검 햅틱 / 오크 공격 방향 / 보스 파이어볼 사거리)

사용자가 이전 라운드에서 "고쳤다"고 보고된 세 가지 버그가 재플레이 후에도 여전히 재현된다고 지적. 이번엔 표면적 증상만 만지지 않고 각 그래프를 `get_connected_subgraph`로 끝까지 추적해 실제 원인을 특정함.

- **오른손 검 햅틱 미작동**: `BP_Sword.TrySwordDamage`가 `ApplyDamage` 이후 `PlayRightHandHitHaptic`을 호출하기 위해 플레이어 폰을 얻는 방법이 `GetOwner()→CastToBP_XRPawn`이었음. `EquippedSword`는 `BP_XRPawn`의 **자손 액터 컴포넌트(Child Actor Component)**라서 이론상 `Owner`가 자동으로 채워져야 하지만, 실제로는 이 경로가 신뢰성 있게 동작하지 않았던 것으로 보임(왼손 마법 햅틱은 `BP_XRPawn` EventGraph 내부에서 `Self`로 직접 호출되어 항상 성공했던 것과 대조적). `GetOwner()` 의존을 제거하고 `GetPlayerPawn(0)→CastToBP_XRPawn`으로 교체(왼손 마법 경로와 동일한, 이미 검증된 패턴). 추가로 사용자 요청대로 양손 햅틱을 훨씬 강하고 길게 조정: 오른손 Amplitude 0.8→1.0, 지속시간 0.5s→1.2s / 왼손 Amplitude 0.9→1.0, 지속시간 0.9s→1.6s.
- **오크(`BP_Enemy`) 공격 방향이 엉뚱함**: `ChaseTick`에서 공격 사거리 안에 들어오면 `StopMovement`만 호출하고 끝 — 플레이어를 바라보도록 회전시키는 로직이 아예 없었음. 즉 오크는 추격 중 마지막으로 향하고 있던 이동 방향(내비게이션 경로상의 각도)을 그대로 유지한 채 공격 애니메이션을 재생했고, 이는 플레이어 위치와 무관한 방향일 수 있었음. `StopMovement.then`과 기존 `ClearTimerByFunctionName("ChaseTick")` 사이에 `FindLookAtRotation(내 위치→타겟 위치)→SetActorRotation`을 스플라이스로 삽입해 공격 준비(AttackWindup) 진입 시점에 플레이어를 정면으로 바라보도록 수정. `BP_Goblin.ChaseTick`은 구조가 달라(`StopMovement` 노드 자체가 없음) 동일 버그가 없음을 확인, 손대지 않음.
- **보스 파이어볼이 "멀리 던지지" 않고 자기 자리 근처에서 터짐**: 두 가지 원인이 겹쳐 있었음. (1) `BP_Boss.CastFireball`의 스폰 위치가 보스 자신의 `GetActorLocation()` 그대로였음(오프셋 없음) — `BP_BossFireball.FireballMesh`는 `QueryAndPhysics`/`BlockAllDynamic` 콜리전(플레이어용 `BP_Fireball`의 `QueryOnly`/`OverlapAllDynamic`과 다름)이라, 스폰되자마자 보스 자신의 캡슐과 즉시 블로킹 충돌해 `OnProjectileStop`이 거의 0거리에서 발동했음. (2) `ProjectileMovement.ProjectileGravityScale=1`이라 설사 자기충돌을 피해도 중력으로 금방 떨어졌음. 수정: `CastFireball`에 `FindLookAtRotation` 결과를 `GetRotationXVector`로 방향 벡터화한 뒤 `보스위치 + 방향*300`을 새 스폰 위치로 계산해 보스 캡슐 밖에서 스폰되도록 함(`MakeTransform.Location` 재배선). `ProjectileGravityScale`은 1→0으로 변경(플레이어용 파이어볼과 동일하게 무중력 직선 비행). 기존 `OnProjectileStop` 기반 데미지/파괴 로직(Block 콜리전 의존)은 그대로 보존 — Overlap 방식으로 바꾸지 않음.
- 세 블루프린트(`BP_Sword`, `BP_XRPawn`, `BP_Boss`, `BP_BossFireball`) 모두 exec 체인을 `get_node_infos`로 전후 재확인 후 컴파일 클린, 저장 완료.
- `L_Test`를 명시적으로 재로드하고 PIE 실행(`UEDPIE_0_L_Test` 경로 접두어 확인) — 로그에 새로운 Blueprint Runtime Error/Accessed None 없음을 확인. 다만 PIE 중에는 신규 액터 스폰이 금지되어(`Cannot create actors while PIE is active`) 오크를 직접 배치해 회전을 육안으로 관찰하는 실측 테스트는 못 했음; 그래프 구조/데이터 배선 검증과 로그 클린 확인까지가 이번 세션에서 가능했던 검증의 한계였고, 실제 체감(특히 햅틱 강도, 전투 중 회전 타이밍)은 사용자의 실제 플레이 확인이 필요함.

# 2026-07-30 검 타격음 추가 + 보스 인트로에서 즉시 파이어볼→전투 진입하도록 변경

- **검 타격 사운드**: `BP_Sword.TrySwordDamage`에서 데미지 적용 성공 경로 마지막(`SpawnSystemAtLocation`(칼 이펙트) 뒤, 기존에 unconnected였던 `then` 핀)에 `PlaySoundAtLocation`을 스플라이스로 추가. 사운드는 `/Game/Game_Item/Equip/Cue/Cue_ToolUseHammerAx14_Cue_Cue`, 위치는 기존 VFX와 동일한 `GetActorLocation(OtherActor)` 재사용. 검이 적을 때릴 때마다(=`ApplyDamage` 성공 시) 재생됨.
- **보스가 멀리 있는 플레이어를 인식 못 해 공격을 안 하던 문제**: 서브에이전트로 `BP_Boss.EventGraph`를 전수 조사해서 원인 확정.
  - `BeginPlay`는 포효(`SelectScreen_Emote`, 4.47초) → `Idle` 루프 애니메이션 → UI/무브먼트 세팅 → 세 개의 반복 타이머를 무조건 예약: `CheckPlayerDistance`(0.2초 간격), `CastFireball`(20초 간격, 상태/거리 무관하게 항상 발사), `IdleFlourish`(1회성, 랜덤 딜레이).
  - `CheckPlayerDistance`(커스텀 이벤트)는 매 0.2초마다 `State=="Idle" AND DistanceTo(Player)<=SenseRadius(1500)`일 때만 `OnSenseBeginOverlap`을 호출해서 전투(Chase)로 전환시킴 — 즉 플레이어가 스폰 시점에 1500유닛보다 멀리 있으면 그 범위 안으로 들어올 때까지 영원히 `Idle` 상태로 남아 근접 공격을 전혀 안 함(파이어볼만 20초마다 상태와 무관하게 날아옴).
  - 물리적 `OnComponentBeginOverlap(SenseSphere)` 이벤트 노드는 모든 출력 핀이 완전히 미연결 상태(죽은 코드)로 확인됨 — 실제 감지는 전부 폴링(`CheckPlayerDistance`) 방식으로만 동작 중이었음.
  - 사용자 요청("어차피 던전에서는 한정된 공간에 배치할 거라 입장하면 바로 보여야 함")에 따라, `BeginPlay`의 타이머 예약 체인 끝(`IdleFlourish` 타이머 설정 직후, 기존에 이어지던 헬스바 위젯 관련 `CastToBP_XRPawn` 앞)에 `CastFireball()` 직접 호출 → `OnSenseBeginOverlap(self, GetPlayerPawn(0))` 직접 호출을 스플라이스로 추가. 이제 포효 인트로가 끝나자마자 거리 상관없이 파이어볼 1발을 던지고 바로 `State=Chase`로 전환되어 추격/공격을 시작함. `CheckPlayerDistance`의 0.2초 폴링은 그대로 남겨뒀지만 이 시점 이후 `State`가 더 이상 `"Idle"`이 아니므로 자연히 아무것도 하지 않는 무해한 상태가 됨(제거 불필요).
  - `BP_Sword`, `BP_Boss` 모두 컴파일 클린, 저장 완료. `L_Test` 재로드 후 PIE 재확인 — 새 런타임 에러 없음. 다만 이번에도 `L_Test`에는 배치된 `BP_Boss` 인스턴스가 없어서(고블린만 존재) 실제 인트로 연출 육안 검증은 못 했음 — 사용자의 던전 맵 실플레이 확인 필요.

# 2026-07-30 방어막 파이어볼 방어 + 오크 3인화(색상별) + 파이어볼 간격/검 사거리/피격음/포션음

- **보스 파이어볼 간격**: `BP_Boss.EventGraph`의 `CastFireball` 반복 타이머 20초→10초.
- **검 사거리 확장**: `BP_Sword.SwordCollision` 박스, 손잡이 쪽 경계는 유지한 채 칼끝 방향 리치만 약 30cm 증가(반경 70→85, 위치 보정).
- **방어막이 보스 파이어볼을 막지 못하던 문제 수정**: 원인 2가지를 모두 고침.
  1. `BP_XRPawn.DefenseBarrierCollision`(스피어)의 콜리전 응답이 `Pawn` 채널만 Block이고 `WorldDynamic`은 Ignore라서, WorldDynamic 오브젝트 타입인 보스 파이어볼이 그냥 통과했음 → `WorldDynamic` 응답을 `ECR_Block`으로 변경.
  2. 막혀도 `BP_BossFireball.OnProjectileStop`이 무조건 `ApplyDamage`를 호출하던 구조라 방어막에 맞아도 데미지가 들어갔음 → `BreakHitResult.HitComponent`가 `AsBP_XRPawn.DefenseBarrierCollision`과 같은지 비교하는 분기를 추가해서, 방어막에 맞은 경우엔 데미지 없이 VFX만 재생하고 파괴하도록 분리(몸통에 맞은 경우는 기존 로직 그대로 보존).
- **오크 3마리 + 색상 구분**: `BP_WaveManager`의 오크 스폰 체인(`BindEventtoOnDestroyed`로 순차 연결된 델리게이트 체인, 기존 2마리)에 동일 패턴으로 3번째 스폰 노드를 추가(위치 X=1100, 기존 800/950과 동일선상). 색상은 이 에셋팩에 이미 `SK_Orc_green`/`SK_Orc_brown`/`SK_Orc_red`(전용 머티리얼·텍스처 포함, 같은 스켈레톤이라 `axe_*` 애니메이션 공유 가능)가 준비되어 있어서, 각 스폰 직후 `GetComponentByClass(SkeletalMeshComponent)` → `SetSkeletalMeshAsset`으로 메시만 교체(1번=green, 2번=brown, 3번=red). 동적 머티리얼 파라미터 방식 대신 기성 메시 스왑을 사용해 리스크를 낮춤.
- **보스 파이어볼 발사음**: `BP_Boss.CastFireball`에서 파이어볼 스폰 직후(`SpawnActorFromClass.then`, 기존 미연결) `Cue_ExplosiveBarrelC15`를 보스 위치에서 `PlaySoundAtLocation`.
- **마법 포션(파이어볼/방어 쿨다운 회복) 획득음**: `BP_XRPawn.EventGraph`에서 쿨다운 종료 시 `bFireballReady=true`/`bDefenseReady=true`를 세팅하는 두 지점 직후에 각각 `Cue_Ashorttactilesou1`을 `PlaySound2D`로 재생(요청하신 "총 2번" = 파이어볼 쿨다운 회복 1회 + 방어막 쿨다운 회복 1회).
- 수정한 5개 블루프린트(`BP_Boss`, `BP_Sword`, `BP_XRPawn`, `BP_WaveManager`, `BP_BossFireball`) 전부 컴파일 클린, 저장 완료. `L_Test` 재로드 후 PIE로 로그 확인 — 새 런타임 에러 없음. 다만 오크 3인 스폰은 웨이브 매니저의 웨이브 진행 조건(이전 웨이브 클리어) 뒤에 걸려 있어서 PIE 시작 직후 5초 안에는 등장하지 않았음 — 실제 웨이브를 진행시켜야 스폰/색상까지 육안 확인 가능, 사용자 플레이 테스트 필요.
- Ran `/Game/Maps/L_Test` in PIE and found no `Blueprint Runtime Error`, `Accessed None`, or `Broken Reference`; stopped PIE and restored `/Game/Maps/L_Dungeon` as the loaded editor level.

# 2026-07-30 플레이어 발자국 소리 추가

- `BP_XRPawn.ApplySmoothLocomotion`(조이스틱 스무스 이동 함수, 매 프레임 가까이 호출됨)에 거리 누적 기반 발자국 시스템 추가.
- 신규 변수 `FootstepDistanceAccum`(float): 매 호출마다 그 프레임의 이동 벡터 길이(`VectorLength(DeltaLocation)`)를 누적하다가, 200(cm)을 넘으면 발자국 사운드를 재생하고 누적값에서 200을 빼서 이월(carry-over) — 실제 보폭 간격과 비슷하게 이동 거리 기준으로 트리거되며, 프레임레이트나 이동 속도와 무관하게 일정한 간격으로 재생됨.
- 사운드는 `/Game/Footsteps_Volume_02/Cues/Footstep_Boots_0{1,3,5,7}_Cue` 4종 중 `RandomIntegerInRange(0,3)`으로 랜덤 선택(중첩 Branch로 분기, `PlaySoundAtLocation`을 플레이어 위치에서 재생) — 매번 똑같은 소리가 반복되지 않도록 변주.
  - 툴링 메모: `K2Node_Select`(선택 노드)의 옵션 핀은 와일드카드 상태에서 `set_pin_value`로 리터럴 애셋 경로를 바로 넣을 수 없었음(에러 발생) — 와일드카드는 실제 와이어 연결로만 타입이 확정되는 것으로 보임. 그래서 Select 대신 `Equal(Int)`+`Branch` 체인(각 브랜치가 이미 타입이 확정된 `PlaySoundAtLocation.Sound` 핀에 직접 리터럴 값을 넣는 방식)으로 우회.
- `BP_XRPawn` 컴파일 클린, 저장 완료. `L_Test` PIE 로그 확인 — 새 런타임 에러 없음. 다만 자동화 도구로 실제 이동시켜 발소리가 들리는지 육안/청각 확인은 못 했음 — 사용자 플레이 테스트 필요.
# 2026-07-30 Victory / Defeat result UI not appearing — diagnosis

- Confirmed `/Game/UI/GameResult/WBP_GameResult` and all six imported result textures still exist.
- Confirmed `BP_XRPawn.GameResultWidget` still uses `WBP_GameResult`, Screen space, 1280x720, visible component state.
- Confirmed the upstream calls are intact: `BP_WaveManager` calls `ShowVictoryResult` after its victory delegate, and the player death chain calls `ShowDefeatResult` after the game-over delegate.
- Confirmed both `BP_XRPawn` forwarding functions are fully connected through `GameResultWidget -> GetUserWidgetObject -> CastToWBP_GameResult -> ShowVictory/ShowDefeat`.
- **Actual root cause:** inside `WBP_GameResult`, both `ShowVictory` and `ShowDefeat` have their intended `SetVisibility` nodes and correct Visible/Collapsed values, but every execution pin is disconnected. Each `FunctionEntry.then` has zero connections, and neither `SetVisibility` node has an exec input connection. Since both result layers default to `Collapsed`, neither can ever become visible.
- `WBP_GameResult`, `BP_XRPawn`, and `BP_WaveManager` compile without errors/warnings, because disconnected nodes are structurally legal Blueprint graphs; compile success does not detect this logic break.
- Diagnosis only in this task; no Blueprint graph was modified.

# 2026-07-30 Victory / Defeat result UI execution-chain repair

- Reconnected `WBP_GameResult.ShowVictory` as `FunctionEntry -> Set VictoryLayer Visible -> Set DefeatLayer Collapsed`.
- Reconnected `WBP_GameResult.ShowDefeat` as `FunctionEntry -> Set VictoryLayer Collapsed -> Set DefeatLayer Visible`.
- Re-read all six affected nodes and verified every execution input/output connection and visibility literal.
- Compiled `WBP_GameResult` with warnings treated as errors and saved it successfully.
- Ran `/Game/Maps/L_Test` in PIE; no `Blueprint Runtime Error`, `Accessed None`, `Broken Reference`, or `Compile Error` was logged.
- Restored `/Game/Maps/L_Dungeon` as the loaded editor level afterward.
- Full victory/defeat triggering still requires playing through the corresponding game conditions in VR, but the proven no-op inside both display functions is repaired.

# 2026-07-30 Transparent game-result lettering textures

- Generated and chroma-keyed three transparent PNG UI assets matching the supplied fantasy result-screen reference:
  - `T_DefeatTitle.png` — red/silver metallic `DEFEAT`, 1532x306.
  - `T_VictoryTitle.png` — gold metallic `VICTORY`, 1657x328.
  - `T_RestartText.png` — ivory/gold Korean `처음으로`, 1417x380.
- Preserved final source PNGs under `Content/UI/GameResult/SourceArt/`.
- Imported and saved Unreal `Texture2D` assets at:
  - `/Game/UI/GameResult/T_DefeatTitle`
  - `/Game/UI/GameResult/T_VictoryTitle`
  - `/Game/UI/GameResult/T_RestartText`
- Configured all three for UI use: `TEXTUREGROUP_UI`, `TC_EditorIcon` (UI RGBA), no mipmaps, clamp X/Y, alpha preserved, sRGB enabled, never stream.
- No Blueprint or widget hierarchy was modified in this task, so no Blueprint compile was required. The existing TextBlock widgets have not yet been replaced with Image widgets.

# 2026-07-30 Cinzel font removal

- User explicitly requested deletion of the added font.
- Audited `/Game/Cinzel`: six saved composite font assets reported no external referencers.
- Unreal MCP asset deletion returned `false` for the loaded font/composite-font packages, so it could not perform a clean in-editor delete.
- Removed the complete physical `Content/Cinzel` folder after validating the exact absolute target. This deleted the imported `.uasset` files, `.ttf` sources, README, and OFL license copy from Content.
- The older repository-root `Cinzel/` source files were already marked deleted in the worktree and remain deleted.
- No Blueprint was changed. Unreal Editor may retain stale Asset Registry entries for the loaded font packages until the editor or project is restarted.

# 2026-07-30 오크 검 데미지 미적용 버그 근본 원인 발견 + 발자국 간격 단축

- **오크가 검에 안 맞는 문제, 진짜 원인**: `BP_Enemy`(오크)와 `BP_Goblin`의 `TrySwordDamage`/`HandleDamage`/`ApplyDamage` 로직, 콜리전 프로파일, 인터페이스 구현은 전부 100% 동일(이번 세션에서 그래프 단위로 재검증 완료, 차이 없음). 진짜 원인은 코드가 아니라 **콜리전 캡슐 크기와 실제 메시(모델) 크기의 불일치**였음.
  - `BP_Enemy.CollisionCylinder`(루트 캡슐)는 `RelativeScale3D=1.6`, 기본 `CapsuleHalfHeight=88`이라 월드 스페이스 캡슐 높이는 `88*1.6*2=281.6cm`. 하지만 `SK_Orc_brown` 메시는 캡슐과 같은 부모 스케일(1.6배)을 그대로 상속받아 실제 렌더링 높이가 `약 206cm(레퍼런스 포즈 원본 높이) * 1.6 ≈ 330cm`까지 커짐 — **시각적 모델이 캡슐보다 약 48cm 더 큼**. 즉 오크의 상체/머리 윗부분은 눈에는 보이지만 실제 충돌(캡슐) 밖에 있어서, 사람이 본능적으로 오크의 몸통·머리를 노리고 검을 휘두르면 스윙이 캡슐을 완전히 빗나가 데미지 판정이 아예 발생하지 않았음.
  - 반대로 `BP_Goblin`은 같은 계산을 해보면 캡슐(스케일 1.3 적용 시 228.8cm)이 실제 메시 시각 크기(약 209cm)보다 오히려 더 커서 콜리전이 매우 관대함 → 아무 데나 휘둘러도 잘 맞는 것처럼 느껴졌던 것.
  - 이 불일치는 `RelativeScale3D`(1.6 vs 1.3) 값 자체가 문제가 아니라(균일 스케일은 캡슐/메시 비율을 보존함), 애초에 **캡슐의 기본(비스케일) 치수(`CapsuleHalfHeight=88`)가 오크 메시의 실제 레퍼런스 포즈 크기(약 206cm, 절반 약 103)보다 작게 설정**되어 있었기 때문 — 오크와 고블린 둘 다 캐릭터 기본값(88)을 커스터마이즈하지 않고 스케일만 조절해서 생긴 문제.
  - **수정**: `BP_Enemy.CollisionCylinder`의 `CapsuleHalfHeight`를 `88→118`, `CapsuleRadius`를 `34→40`으로 상향(스케일 1.6은 그대로 유지) — 여유분 포함해서 오크 메시 전체를 캡슐이 확실히 감싸도록 조정. 고블린은 원래도 캡슐이 메시보다 커서 그대로 둠.
  - 검증: `SkeletalMeshTools.get_bounds`로 두 메시의 레퍼런스 포즈 바운딩 박스를 직접 수치로 비교해서 확인(오크 박스 익스텐트 Z≈103.27 vs 고블린 Z≈81.02, 반면 캡슐 기본 하프하이트는 둘 다 88로 동일했었다는 게 결정적 단서). 실제 VR 플레이로 "검이 이제 오크한테 잘 맞는지"까지는 자동화 도구로 검증 불가 — 사용자 플레이 테스트 필요.
- **발자국 소리 간격 단축**: `BP_XRPawn.ApplySmoothLocomotion`의 누적 거리 임계값 2곳(`K2Node_PromotableOperator_9`/`_10`의 B핀, 기존 200.0)을 `100.0`으로 축소 — 발자국이 기존 대비 2배 더 자주 재생됨.
- `BP_Enemy`, `BP_XRPawn` 컴파일 클린, 저장 완료. `L_Test` 재로드 후 PIE 로그 확인 — 새 런타임 에러 없음(기존부터 있던 `LogCrowdFollowing: RecastNavMesh 없음` 경고는 무관, 오크 관련 에러 없음). 웨이브 매니저의 오크 스폰이 이전 웨이브 클리어 후에 걸려 있어서 PIE 시작 직후에는 오크가 등장하지 않아 실제 스폰 상태 육안 확인은 못 함.
# 2026-07-30 Game-result lettering composition

- Updated `/Game/UI/GameResult/WBP_GameResult` to use the previously generated transparent lettering textures.
- Replaced four non-variable TextBlocks with Image widgets while preserving their names, parents, and slots:
  - `VictoryTitle` -> `T_VictoryTitle`
  - `DefeatTitle` -> `T_DefeatTitle`
  - `VictoryButtonText` and `DefeatButtonText` -> shared `T_RestartText`
- Kept both subtitles, restart Button widgets, click bindings, backgrounds, panels, and result visibility functions unchanged.
- Centered both title images at `(365,155)` with size `550x110`.
- Centered the restart label inside both `550x105` buttons using a `300x80` brush and centered ButtonSlot alignment to avoid stretching the Korean texture across the full button.
- Re-read the widget tree and confirmed all four widgets are Image class with the intended texture references and sizes.
- `WBP_GameResult` compiled with warnings treated as errors and saved successfully.
- Ran `/Game/Maps/L_Test` in PIE and found no new `Blueprint Runtime Error`, `Accessed None`, `Broken Reference`, or compile error. Existing unrelated `ABP_Goblin` unconnected state-machine warnings remain.
- Restored `/Game/Maps/L_Dungeon` after testing.
- Unreal MCP does not support `CaptureAssetImage` for WidgetBlueprint assets, so structural/compile/PIE verification was completed but no automated rendered WBP preview was available.

# 2026-07-30 보스 전투 상황별 보이스 사운드 4종 추가

- `Rampage_Effort_Cheer`(보스가 플레이어에게 데미지를 입힐 때): 실제로 플레이어에게 데미지가 들어가는 3개 함수 각각에서 `ApplyDamage(Message_0).then` 직후로 스플라이스해서 `PlaySound2D` 추가.
  - `BP_Boss.DealAttackDamage`: `Message_0.then → PlaySound2D(Cheer) → CastToCharacter(기존 넉백 체인)`.
  - `BP_Boss.DealSlamDamage`: `Message_0.then → PlaySound2D(Cheer) → SetState(Recovery)`.
  - `BP_Boss.DealFireballDamage`: `Message_0.then`이 원래 미연결(dead end)이라 스플라이스 없이 바로 연결.
- `Rampage_Effort_Death`(보스가 데미지를 입었을 때): `BP_Boss.HandleDamage`의 피격(생존) 분기 — `PlayAnimation(HitReact_Front) → LaunchCharacter(넉백)` 다음, `SetTimer(EndHit)` 이전에 `PlaySoundAtLocation`으로 스플라이스. Location은 이미 넉백 계산에 쓰이던 `GetActorLocation(SelfPawn)` 결과를 데이터 핀 팬아웃으로 재사용(죽음 분기가 아니라 "데미지를 입었을 때" 매번 재생되는 지점이라 이름은 Death지만 실제로는 피격 사운드로 사용).
- `Rampage_Effort_Ability_Ultimate_Shrink`(보스가 파이어볼 던질 때): `BP_Boss.CastFireball`에서 기존 `Cue_ExplosiveBarrelC15`(폭발 사운드) `PlaySoundAtLocation.then`이 미연결 상태였던 걸 이어서 새 `PlaySoundAtLocation` 추가 — 두 사운드가 순서대로 함께 재생됨. Location은 보스 자기 위치(`GetActorLocation`, 기존 3곳에서 이미 재사용 중이던 값)를 팬아웃으로 재사용.
- `Rampage_Effort_Ability_Ultimate_Grow`(그냥 중간중간 한 번씩): `BP_Boss.IdleFlourish`는 원래 `State=="Idle"`일 때 6~12초 랜덤 간격으로 재귀 타이머를 걸며 랜덤 유휴 애니메이션을 재생하는 함수라, 이 "가끔 한 번씩" 트리거에 정확히 들어맞음 — `PlayAnimation.then → PlaySound2D(Grow) → SetTimer(재귀 재예약)`으로 스플라이스.
- 6곳 모두 스플라이스 후 `get_node_infos`로 재조회해서 exec 체인이 의도대로 끊기지 않고 이어졌는지 전부 확인(fan-out 덮어쓰기 버그 방지용 검증).
- `BP_Boss` 컴파일 클린, 저장 완료. `L_Test` 재로드 후 PIE 로그에 `Accessed None`/`Blueprint Runtime Error`/`Broken Reference` 없음. 실제 전투 중 사운드 타이밍이 어울리는지는 사용자 플레이 테스트 필요.
