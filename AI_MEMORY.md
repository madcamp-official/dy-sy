# AI Memory

## 현재 단계
prompts/06_Enemy.md (BP_Enemy 상태 머신) 구현 및 감지/공격 버그 수정 완료. PIE 자동 검증으로 전체 사이클(Idle→Chase→AttackWindup→Attack→플레이어 피해) 확인됨. 사람의 최종 확인 및 Git Commit 대기 중.

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

## 진행 중
- 사람이 실제 Unreal Editor 화면에서 위 자동 검증 결과를 육안으로 재확인 (선택 사항 — MCP로 State/TargetActor/체력 변화를 직접 확인했으므로 핵심 동작은 검증됨)

## 다음 작업
- 사람 확인 후 원하면 Git Commit 생성 (아직 생성 안 함)
- 이후 다음 프롬프트를 순서대로 실행
- (선택) BP_Enemy의 시각적 스케일/정렬(SK_Orc_brown 피벗, 회전)은 육안 확인 후 필요시 미세 조정
- (선택, 급하지 않음) 팀원의 손-검 작업이 병합된 뒤, BP_XRPawn에 플레이어 감지용 콜리전 컴포넌트(캡슐 등)를 추가하는 것을 고려 — 지금은 거리 체크로 우회했지만 향후 다른 시스템(예: 근접 트리거, 다른 적)도 플레이어를 감지해야 한다면 정식 콜리전이 있는 게 더 범용적임

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
