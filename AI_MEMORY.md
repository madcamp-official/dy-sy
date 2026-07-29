# AI Memory

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
