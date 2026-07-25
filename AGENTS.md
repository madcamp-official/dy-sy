# ArcaneTrialVR — 공통 AI 작업 규칙

## 환경
- Unreal Engine 5.8
- Meta Quest
- OpenXR
- Blueprint 중심
- Codex + Unreal MCP
- 2명 / 7일 / 3~5분 플레이

## 필수 결과물
- Quest 실행
- 오른손 검
- 왼손 화염구
- 일반 적 1종
- 보스 1종
- 보스 패턴 최소 2개
- 플레이어 체력
- 보스 체력 UI
- 승리, 패배, 재시작

## 절대 규칙
1. 작업 전 현재 프로젝트와 Unreal MCP 도구를 조사한다.
2. 실제 존재하는 에셋 경로만 사용한다.
3. 기존 에셋 삭제·이름 변경 금지.
4. 한 번에 하나의 작은 기능만 구현한다.
5. 수정한 Blueprint는 반드시 Compile한다.
6. Compile Error, Warning, Broken Reference를 확인한다.
7. 새 플러그인을 임의로 추가하지 않는다.
8. 핵심 게임 로직을 Level Blueprint에 넣지 않는다.
9. Tick보다 Event, Timer, Collision Event, Event Dispatcher를 우선한다.
10. Quest 성능을 그래픽 품질보다 우선한다.
11. 지원되지 않는 MCP 작업은 솔직히 보고한다.
12. 작업 후 AI_MEMORY.md를 갱신한다.

## 표준 작업 순서
1. AI_MEMORY.md 읽기
2. 관련 문서 읽기
3. 현재 상태 조사
4. 변경 계획 보고
5. 최소 단위 구현
6. Compile
7. L_Test에서 검증
8. 변경사항 보고
9. AI_MEMORY.md 갱신
