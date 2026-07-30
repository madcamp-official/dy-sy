# ArcaneTrialVR

Meta Quest 3용 VR 판타지 전투 데모입니다. 오른손 검과 왼손 화염구로 적과 보스를 쓰러뜨리는 짧고 몰입감 있는 전투 루프를 목표로 합니다.

- 매드캠프 팀 프로젝트 (2인 / 7일 개발)
- 목표 플레이 타임: 3~5분

## 🎯 기획 의도

최소 기능으로 완결된 전투 경험을 우선한다. "검 + 화염구 + 일반 적 + 보스 + 승패"라는 핵심 루프가 확실히 재미있게 동작하는 것이 화려한 콘텐츠 양보다 중요하다. 리소스가 부족해질 경우 다음 순서로 기능을 덜어낸다.

점수 → 음성 → 손목 HUD → 투사체 반사 → 충격파 → 검기 → 궁극기 → 두 번째 웨이브

## ✨ 주요 기능

- **오른손 검 전투** — 스윙 속도 기반 피격 판정, 히트 이펙트(Niagara)
- **왼손 화염구 마법** — 투사체 충돌 기반 피해, 낙하 지점 화염 잔여 효과
- **일반 적 2종** — 고블린 · 오크, 감지-추격-공격 FSM과 유휴 배회 로직
- **보스** — 근접 슬래시 / 강타 랜덤 패턴, 체력 50%에서 페이즈 전환, 주기적 원거리 화염구
- **웨이브 시스템** — 고블린 → 오크 → 보스 순으로 진행
- **던전 맵** — 층간 이동(계단)과 벽 충돌이 적용된 탐험형 레벨
- **HUD** — 손목형 체력바 · 마법 충전 게이지 · 미니맵 토글, 적 머리 위 체력바
- **승리/패배 결과창** — 전투 종료 시 결과 UI 표시 후 재시작

## 🏗️ 시스템 아키텍처

| 항목 | 내용 |
|---|---|
| 엔진 | Unreal Engine 5.8 |
| VR 기기 | Meta Quest 3 |
| VR 기반 | OpenXR |
| 개발 방식 | 블루프린트 중심 |
| 이펙트 | Niagara |
| HUD | UMG Widget + Widget Component |
| 형상관리 | GitHub + Git LFS |

**핵심 블루프린트**

- `BP_XRPawn` — VR 플레이어
- `BP_Sword`, `BP_Fireball` (`BP_ProjectileBase` 기반)
- `BPI_Damage2` — 모든 피해 판정이 거치는 공용 인터페이스
- `BP_Enemy`(오크), `BP_Goblin`, `BP_Boss`
- `BP_WaveManager` — 웨이브 진행 관리
- `WBP_PlayerHUD`, `WBP_EnemyHealthBar`, `WBP_GameResult`

피해 판정은 `BPI_Damage2` 인터페이스로 통일하고, UI·게임 흐름 전환은 Event Dispatcher를 우선 사용한다.

## ▶️ 실행 방법

1. Unreal Engine 5.8로 `dy.uproject`를 연다.
2. `/Game/Maps/L_Test` 또는 `/Game/Maps/L_Dungeon`을 로드한다.
3. VR Preview 또는 PIE로 실행한다. (Quest는 Link/Air Link 또는 패키지 빌드로 실행)

## 🤖 AI 협업 워크플로

이 프로젝트는 Codex/Claude + Unreal MCP를 이용해 AI가 블루프린트를 직접 조작하며 개발한다. 세부 규칙과 진행 기록은 다음 문서를 참고한다.

- [AGENTS.md](AGENTS.md) — AI 작업 공통 규칙
- [AI_MEMORY.md](AI_MEMORY.md) — 세션별 작업 이력과 원인 분석
- [AI/PROJECT_STATUS.md](AI/PROJECT_STATUS.md) — 시스템별 완료 상태
- [docs/GameDesign.md](docs/GameDesign.md), [docs/TechnicalArchitecture.md](docs/TechnicalArchitecture.md)
- `prompts/00_ProjectAudit.md` ~ `13_FinalValidation.md` — 단계별 개발 프롬프트
