# AI Memory

## 현재 단계
prompts/07_WaveSystem.md (BP_WaveManager) 구현 완료: Wave1 = BP_Goblin 3마리, Wave2 = BP_Enemy 5마리, 전멸 시 OnAllWavesCleared 디스패처. Compile 정상, PIE로 스폰/변수 확인됨. 실제 전투로 죽여서 Wave 전환이 실사격되는지는 사람 확인 필요(사유는 아래 "다음 작업" 참고). Git Commit 대기 중.

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

## 진행 중
- 사람이 실제 Unreal Editor 화면에서 06_Enemy.md 자동 검증 결과를 육안으로 재확인 (선택 사항 — MCP로 State/TargetActor/체력 변화를 직접 확인했으므로 핵심 동작은 검증됨)
- 사람이 실제 플레이로 BP_WaveManager의 Wave1→Wave2→OnAllWavesCleared 전환을 확인 (필수 — 위 "검증 미완료 부분" 참고)

## 다음 작업
- 사람 확인 후 원하면 Git Commit 생성 (아직 생성 안 함, 06/07 둘 다 대기 중)
- 이후 다음 프롬프트(08_Boss.md)를 순서대로 실행
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
- (07_WaveSystem에서 발견) BlueprintTools.add_variable은 "class"(TSubclassOf) 타입을 지원하지 않음 (지원: bool/int/float/byte/name/string/text/Vector/Rotator/Transform/Vector2D/LinearColor 뿐). add_object_variable로 만든 변수는 TSubclassOf가 아니라 순수 오브젝트 레퍼런스(인스턴스 참조) 타입임 — get_node_infos로 직접 확인함. 이 MCP엔 Class 레퍼런스 변수를 만드는 방법이 없으므로, 스폰할 클래스를 변수로 노출해야 하면 대신 SpawnActorFromClass 등의 Class 핀에 리터럴 값(예: "/Game/Path/BP_Foo.BP_Foo_C")을 직접 set_pin_value로 박아넣는 우회가 필요함
- (07_WaveSystem에서 발견) find_node_types로 정수(Integer) 사칙연산/비교 PromotableOperator(+,-,==,<= 등)를 검색해도 전혀 나오지 않음 (부동소수점은 BP_Enemy에서처럼 "float-float"/"float<=float" 식으로 존재). 대신 "Math|Integer|DecrementInt"/"IncrementInt"(매크로, 값 핀이 "레퍼런스로"), "Math|Integer|CompareInt"(매크로, Input/CompareWith 비교 후 ">"/"=="/"<" 세 실행 핀 분기)가 있으므로 정수 증감/비교는 이 매크로들을 사용할 것
- (07_WaveSystem에서 발견) find_node_types로 Vector 덧셈(vector+vector) 같은 벡터 연산자도 검색되지 않음. 액터 위치에 오프셋을 더하는 계산이 필요하면 우회책으로 리터럴 절대 좌표를 MakeVector에 직접 넣는 방법을 고려할 것
- (07_WaveSystem에서 발견, 중요) "게임|AssignOnDestroyed" 같은 "Assign" 계열 편의 노드(액터의 델리게이트에 새 커스텀 이벤트를 자동 생성해 바인딩)는 호출할 때마다 항상 같은 이름("OnDestroyed_이벤트" 등)으로 커스텀 이벤트를 만들어서, 같은 델리게이트에 여러 번(반복 스폰 등으로) 사용하면 "둘 이상의 함수가 이름이 같습니다" 컴파일 에러가 남. 커스텀 이벤트 이름을 바꾸는 툴이 이 MCP에 없음(ObjectTools로 CustomFunctionName 접근 불가). 해결책: Assign 계열은 딱 한 번만 써서 커스텀 이벤트를 하나 만들고, 이후 같은 델리게이트에 추가로 바인딩할 때는 "게임|BindEventtoOnDestroyed" 같은 수동 Bind 노드를 써서 그 커스텀 이벤트의 OutputDelegate 핀을 재사용(델리게이트 출력 핀은 여러 입력으로 팬아웃 가능, 실행 입력 핀도 여러 출력에서 팬인 가능함을 확인함)
- (07_WaveSystem에서 발견, 중요) 그래프에 새 K2Node_MacroInstance(DecrementInt/CompareInt 등)를 만든 뒤 곧바로 또 다른 새 매크로 인스턴스를 만들면, 먼저 만들어서 이미 배선까지 끝낸 매크로 인스턴스가 배선째로 통째로 사라지는 현상 발생(get_node_infos에서 "not valid EdGraphNode" 에러, find_nodes로도 더 이상 안 잡힘). 원인 불명. 해결책: 매크로 인스턴스는 만들자마자 그 자리에서 바로 완전히 배선을 끝내고, 여러 매크로 인스턴스를 순차로 추가해야 한다면 매번 compile_blueprint 후 GetLogEntries로 에러 확인 + get_node_infos로 이전에 만든 매크로 인스턴스들이 여전히 유효한지 재확인할 것
- (07_WaveSystem에서 발견) SceneTools.remove_from_scene은 PIE/Simulate 세션이 활성화된 동안 호출하면 무조건 "Cannot remove actors while PIE is active" 에러가 남 (PlayMode_InViewPort, PlayMode_Simulate 둘 다 확인). 즉 "PIE 중에 remove_from_scene으로 액터를 강제 삭제해서 OnDestroyed/사망 로직을 트리거해보는" 검증 우회는 이 MCP 버전에서 통하지 않음 — 살아있는 인스턴스의 함수를 외부에서 호출하는 툴도 없으므로, 데미지/사망 관련 로직의 실제 런타임 동작은 결국 사람이 직접 플레이해서 확인해야 함 (구조적 배선 검증은 get_node_infos로 가능)
