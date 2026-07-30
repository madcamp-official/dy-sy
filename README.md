# Dungeon VR

Meta Quest 3 스탠드얼론 VR 판타지 액션 데모. 검과 화염구로 웨이브를 돌파하고 보스를 쓰러뜨린다.

목표 플레이타임 3~5분

## Requirements

- Unreal Engine 5.8
- OpenXR 런타임 (Oculus/Meta, Quest Link 또는 Air Link)
- Meta Quest 3 (또는 PC VR Preview로 대체 실행)

## Quick Start

```
1. dy.uproject를 Unreal Engine 5.8로 연다
2. Content Browser에서 /Game/Maps/L_Dungeon(정식 플레이) 또는 /Game/Maps/L_Test(빠른 검증)를 연다
3. VR Preview로 실행한다 — 헤드셋 없이 확인할 땐 일반 PIE로 실행 가능
```

## Controls

| 입력 | 동작 |
|---|---|
| 왼쪽 스틱 | 이동 (HMD 정면 기준) |
| 오른손 검 물리 스윙 | 근접 공격 (스윙 속도가 임계값 이상일 때만 피해 판정) |
| 왼손 그립 | 화염구 발사 |
| 왼손 Y 버튼 (누르고 있기) | 방어막(Aura) 전개 — 유지 중 피격 무효화, 근접 공격자는 넉백 |
| 오른손 메뉴 버튼 | 미니맵 토글 |

## Gameplay Loop

고블린 웨이브 → 오크 웨이브 → 보스전(50% 체력에서 페이즈 전환) → 승리/패배 결과창 → 버튼으로 재시작

## Tech Stack

Unreal Engine 5 · OpenXR · 블루프린트 전용(C++ 없음) · Niagara VFX · UMG(Widget Component 부착 방식 HUD) · Git + Git LFS

## Project Structure

```
Content/
  XRFramework/       VR 폰, 인풋 매핑 (BP_XRPawn, IMC_Default)
  Blueprints/
    Weapons/          BP_Sword, BP_SwordWave
    Magic/            BP_Fireball
    Enemies/          BP_Enemy(오크), BP_Goblin, BP_Boss
    Systems/          BP_WaveManager
    Interfaces/       BPI_Damage2 — 모든 피해 판정 공용 인터페이스
  UI/                 WBP_PlayerHUD, WBP_EnemyHealthBar, WBP_GameResult
  Maps/               L_Test(검증), L_Dungeon(정식 레벨)
```

## Development Status

| 시스템 | 상태 |
|---|---|
| 이동/조작 | 완료 |
| 검 전투 | 완료, 실기기 재검증 예정 |
| 화염구 | 완료 |
| 일반 적 AI | 완료 |
| 보스 | 완료, 실기기 재검증 예정 |
| 웨이브 시스템 | 완료 |
| 던전 맵(벽 충돌/계단) | 완료 |
| HUD | 완료 |
| 승리/패배 결과창 | 완료 |
| Quest 패키징/실기기 테스트 | 미착수 |

세부 항목은 [AI/PROJECT_STATUS.md](AI/PROJECT_STATUS.md) 참고.

## Docs

- [AGENTS.md](AGENTS.md) — AI 작업 공통 규칙
- [AI_MEMORY.md](AI_MEMORY.md) — 세션별 작업 이력/근본 원인 분석
- [docs/GameDesign.md](docs/GameDesign.md) — 기획 범위, 기능 삭제 우선순위
- [docs/TechnicalArchitecture.md](docs/TechnicalArchitecture.md) — 블루프린트 구조 규칙
- `prompts/00_ProjectAudit.md` ~ `13_FinalValidation.md` — 단계별 개발 프롬프트
