# AI Operating System

## 목적
현재 Unreal 프로젝트의 상태, Git 상태, MCP 상태, 에셋 상태와
다음 작업을 한 곳에서 관리합니다.

## 항상 읽을 파일
1. `AI/README.md`
2. `AI/PROJECT_STATUS.md`
3. `AI/CURRENT_TASK.md`
4. `AI/MEMORY.md`
5. 필요 시 관련 Checklist/Report

## 표준 실행 순서
1. Git 상태 확인
2. Unreal MCP 연결 확인
3. 현재 에셋과 맵 조사
4. 작업 계획 보고
5. 최소 단위 구현
6. Compile
7. Save
8. Output Log 확인
9. PIE 실행
10. 요청 기능 검증
11. 회귀 테스트
12. MEMORY와 PROJECT_STATUS 갱신
13. Git 커밋 메시지 제안
14. 중단

## 금지
- 기존 Blueprint 재생성
- 존재하지 않는 경로 추측
- 관련 없는 에셋 수정
- 새 플러그인 임의 추가
- PIE 없이 완료 선언
- 다음 작업 자동 진행
- Marketplace 원본 직접 수정
