# Project Memory

## 프로젝트
- Unreal Engine 5.8
- Meta Quest / OpenXR
- Blueprint 중심
- Codex + Unreal MCP
- 2명 / 7일

## 마지막 확인된 Git 상태
- branch: main
- upstream: origin/main
- ahead: 0
- behind: 0
- last audited commit: a2bc502ac55973fc6a08620db98068b64de16315
- message: assets add
- warning: `dy-sy`가 mode 160000이지만 `.gitmodules` 매핑이 없음

## 주요 맵
- `/Game/Maps/L_Test`
- `/Game/Maps/L_Arena`
- `/Game/Maps/L_Dungeon`
- `/Game/Maps/Sublevels/L_Dungeon_Gameplay`

## 주요 Blueprint
- `/Game/Blueprints/Interfaces/BPI_Damageable`
- `/Game/Blueprints/Interfaces/BPI_Damage2`
- `/Game/Blueprints/Enemies/BP_DamageDummy`
- `/Game/XRFramework/Blueprints/BP_XRGameMode`
- `BP_XRPawn`

## Import된 에셋
- MedievalDungeon
- ParagonGreystone
- ParagonRampage
- Skeleton_Necromancer
- Swampgoblin
- Orc
- Free_Magic
- FANTASY_LONGSWORD_KRYVEN_BLADE
- XRFramework
- XRMannequins

## 현재 미완성
- Player Death / Restart
- Sword
- Fireball
- Enemy Combat
- Boss Patterns
- HUD
- Win/Lose/Restart
- Quest 패키징

## 알려진 리스크
- Unreal MCP가 세션에 따라 도구를 노출하지 않을 수 있음
- Android SDK 검증 필요
- PICOController 플러그인 필요성 검토
- ParagonRampage BuiltData resave 경고
- GameFeatureData Asset Manager 경고

## 2026-07-26 Damage System 재개 점검
- Unreal Editor 프로세스가 실행 중이지 않음
- `127.0.0.1:8000` Unreal MCP 서버 TCP 연결 실패
- Codex 세션에 Unreal MCP 도구가 노출되지 않음
- 저장된 Blueprint 상태, Compile 상태, Save 상태, Output Log 및 PIE 동작을 MCP로 재검증하지 못함
- 기존 Blueprint와 맵은 수정하지 않음
- Damage System은 완료로 판정하지 않으며 MCP 연결 복구 후 현재 저장 상태부터 다시 조사해야 함

## 2026-07-26 Damage System 저장 상태 재점검
- Unreal MCP 라이브 연결 정상
- `/Game/Blueprints/Enemies/BP_DamageDummy`의 저장 상태 확인:
  - `CurrentHealth` 기본값 100
  - `MaxHealth` 변수를 추가하고 기본값 100으로 저장
  - 기존 `ApplyDamage` 그래프는 `CurrentHealth - Damage` → 저장 → `CurrentHealth <= 0` → `DestroyActor`로 연결됨
  - 이름이 같은 `ApplyDamage` 그래프는 있으나 `BP_DamageDummy`가 `BPI_Damageable`을 실제 구현하지 않음
- 인터페이스 호출 테스트 노드 컴파일 시 `Self는 BPI_Damageable_C가 아니므로 Target 연결 필요` 오류로 실제 미구현 확인
- Unreal MCP BlueprintTools에는 Blueprint Interface 추가 도구가 없어 자동 진행 중단
- 임시 테스트 노드는 모두 제거했고 `BP_DamageDummy`는 다시 Compile 및 Save 성공
- `/Game/Maps/L_Test`에 `BP_DamageDummy_C_0` 1개 배치 및 Save 성공
- 인터페이스 추가 전이므로 PIE 100 → 50 → Destroy 검증은 실행하지 않았고 Damage System은 미완료

## 2026-07-26 Damage System 완료 검증
- 수동 구현된 `/Game/Blueprints/Interfaces/BPI_Damage2`와 `BP_DamageDummy` 저장 상태를 Unreal MCP로 재검사
- `BPI_Damage2.ApplyDamage`:
  - `Damage` 입력은 Blueprint Float이며 엔진 핀 표기는 `플로트(배정밀도)`
- `BP_DamageDummy`:
  - Event `ApplyDamage.Damage`가 `HandleDamage.Damage`에 직접 연결됨
  - `HandleDamage`가 `CurrentHealth - Damage` 결과를 `CurrentHealth`에 저장
  - `CurrentHealth <= 0`의 True 경로가 `DestroyActor`에 연결됨
  - `MaxHealth=100`, `CurrentHealth=100`
- `/Game/Maps/L_Test`에 `BP_DamageDummy_C_0` 1개 존재
- `BPI_Damage2`와 `BP_DamageDummy` Compile 및 Save 성공
- L_Test PIE에서 인터페이스 호출 50을 두 번 실행하고 런타임 상태를 직접 확인:
  - 시작 `CurrentHealth=100`
  - 첫 호출 후 `CurrentHealth=50`
  - 두 번째 호출 후 `BP_DamageDummy` 월드 조회 결과 없음(Destroy 확인)
- PIE 종료 후 임시 Delay/인터페이스 호출 노드 4개를 삭제
- 임시 노드 제거 후 두 Blueprint 재Compile 및 Save 성공
- Damage System 완료

## 2026-07-26 VR Player Health 완료 검증
- `/Game/XRFramework/Blueprints/BP_XRPawn`이 `BPI_Damage2`를 실제 구현
- `MaxHealth`와 `CurrentHealth`는 Blueprint Float이며 기본값은 각각 100
- Event `ApplyDamage` 저장 로직:
  - `CurrentHealth - Damage`
  - `Clamp(Float)` Min 0, Max `MaxHealth`
  - 결과를 `CurrentHealth`에 저장
- ApplyDamage 경로에 `DestroyActor` 없음
- `BP_XRPawn` Compile 및 Save 성공
- L_Test PIE에서 인터페이스 호출 50을 두 번 실행하고 런타임 상태를 직접 확인:
  - 시작 `CurrentHealth=100`
  - 첫 호출 후 `CurrentHealth=50`
  - 두 번째 호출 후 `CurrentHealth=0`
  - `BP_XRPawn_C_0`가 0 Health에서도 PIE 월드에 계속 존재
- PIE 종료 후 임시 Delay/인터페이스 호출 노드 4개 삭제
- 기존 BeginPlay 실행선을 원래 `K2Node_IfThenElse_4`에 재연결
- 임시 로직 제거 후 `BP_XRPawn` 재Compile 및 Save 성공
- VR Preview 또는 실제 헤드셋 검증은 수행하지 않음
- Player Health 완료. Death와 Restart는 아직 구현하지 않음

## 2026-07-26 Task Transition
- VR Player Health는 저장 상태 검사, Compile, Save, L_Test PIE `100 → 50 → 0`, 0 Health에서 `BP_XRPawn_C_0` 생존 확인까지 완료
- 모든 임시 테스트 로직 제거 및 최종 dirty state 없음 확인
- Current Task를 Sword Combat으로 전환
- Next Task를 Fireball Combat으로 전환
- 기존 VR Player Health 검증 증거는 위 완료 검증 섹션에 보존

## 2026-07-26 Sword Combat 구현 및 검증 차단
- 요청된 `assets/AssetManifest.md`는 프로젝트에 존재하지 않았고, 파일명 검색에서도 manifest 문서를 찾지 못함
- 선택한 메시: `/Game/FANTASY_LONGSWORD_KRYVEN_BLADE/Meshes/SM_KRYVEN_BLADE`
  - 확인된 imported sword Static Mesh 중 프로젝트 요구에 맞는 단일 메시
  - LOD0 triangle count: 1,254
- Marketplace 원본은 수정하지 않고 프로젝트 소유 `/Game/Blueprints/Weapons/BP_Sword`를 생성
- `BP_Sword` 구성:
  - 기존 Kryven Blade Static Mesh
  - `SwordCollision` Box Collision, `OverlapAllDynamic`, Query Only
  - `SwordDamage=15`
  - `MinimumDamageSpeed=100` (configurable)
  - `PreviousLocation`, `SwordSpeed`
  - Tick에서 `Distance(CurrentLocation, PreviousLocation) / DeltaSeconds`로 이동 속도 계산
  - `SwordSpeed > MinimumDamageSpeed`일 때만 `BPI_Damage2.ApplyDamage` 메시지 호출
  - `ActorBeginOverlap` 경로를 사용하므로 한 continuous overlap에서 같은 액터에 반복 호출하지 않음
- `BP_XRPawn`에 `EquippedSword` Child Actor Component를 추가하고 기존 `HandRight` 아래에 부착
- `HandRight`는 기존 `MotionControllerRightGrip` 아래 구조를 유지
- `BP_Sword`, `BP_XRPawn` 최종 Compile(warnings as errors) 및 Save 성공
- L_Test in-viewport PIE:
  - `EquippedSword_GEN_VARIABLE_BP_Sword_C_CAT_0` 생성 확인
  - zero-speed overlap에서 `BP_DamageDummy.CurrentHealth=100` 유지 확인
  - MCP 호출 간 프레임 간격 때문에 controller swing speed와 overlap을 같은 프레임 흐름으로 재현하지 못함
  - 임시 forced-speed 검증 로직으로도 신뢰 가능한 damage evidence를 얻지 못해 Sword Combat 완료로 판정하지 않음
- 모든 임시 PIE validation 함수, Delay, DoOnce, 호출 노드 제거
- 최종 PIE 종료, `BP_Sword`, `BP_XRPawn`, `L_Test` dirty state 없음
- 남은 필수 검증: VR Preview 또는 실제 헤드셋에서 오른손 검을 `BP_DamageDummy`에 threshold 이상 속도로 한 번 휘둘러 `CurrentHealth 100 → 85`를 확인하고, 접촉을 유지하는 동안 추가 감소가 없는지 확인
