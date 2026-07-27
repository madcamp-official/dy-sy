# AI Memory

## 현재 단계
prompts/06_Enemy.md (BP_Enemy 상태 머신) 구현 완료. 사람 검증 대기 중.

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

## 진행 중
- Unreal Editor에서 prompts/06_Enemy.md (BP_Enemy) 결과 사람 검증 대기
  - 특히 PIE에서 플레이어가 실제로 이동하여 SenseSphere에 진입할 때 Chase → AttackWindup → Attack → Recovery 전이가 의도대로 되는지 확인 필요 (자동화된 MCP 테스트로는 정지 상태 BeginPlay 초기값만 확인함: State=Idle, CurrentHealth=40 정상)

## 다음 작업
- 사람이 PIE에서 BP_Enemy의 추격/공격/피격/사망 전체 사이클을 직접 조작해 검증
- 사람 검증 성공 후 Git Commit 생성
- 이후 다음 프롬프트를 순서대로 실행
- (선택) BP_Enemy의 시각적 스케일/정렬(SK_Orc_brown 피벗, 회전)은 육안 확인 후 필요시 미세 조정

## 알려진 경고
- Android SDK Setup 실패 상태로 Quest 패키징 전 SDK 설정 필요
- PIE 중 WASAPI Raw Mode 초기화 경고
- L_Test PIE 중 RecastNavMesh를 찾지 못했다는 CrowdManager 경고 1회
- OpenXR eye gaze 및 BD controller 확장 미지원 경고
- BPI_Damageable, BPI_Damage2 인터페이스 그래프 자체에 "No execute pin found on node ...ApplyDamage.K2Node_FunctionEntry_0" 경고가 로그에 존재 (prompt 02에서 생성된 기존 인터페이스 정의 자체의 경고로, BP_Enemy 작업과 무관하며 손대지 않음)
- BP_Enemy의 SK_Orc_brown 메시 배치/정렬(위치, 회전, 피직스 에셋)은 기본값 그대로이며 시각적으로 미세 조정 필요할 수 있음 (기능 검증에는 영향 없음)
- Unreal MCP 환경이 한국어로 로컬라이즈되어 있어, write_graph_dsl의 "self" 자동 바인딩과 +/-/==/<= 등 연산자 축약 문법이 내부적으로 영어 노드 ID를 찾다 실패함 (한국어 로캘 비호환). BP_Enemy는 이 때문에 DSL 대신 create_node/connect_pins로 노드를 직접 생성/연결하는 방식으로 전부 구현함
