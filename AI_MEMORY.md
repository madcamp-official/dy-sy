# AI Memory

## 현재 단계
프로젝트 기본 구조와 테스트/아레나 레벨 설정 완료.

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

## 진행 중
- Unreal Editor에서 prompts/01_ProjectSetup.md 결과 사람 검증 대기

## 다음 작업
- Unreal Editor에서 /Game/Maps/L_Test의 바닥, 조명, VR 시작 위치와 HMD 동작 확인
- 사람 검증 성공 후 Git Commit 생성
- 이후 다음 프롬프트를 순서대로 실행

## 알려진 경고
- Android SDK Setup 실패 상태로 Quest 패키징 전 SDK 설정 필요
- PIE 중 WASAPI Raw Mode 초기화 경고
- L_Test PIE 중 RecastNavMesh를 찾지 못했다는 CrowdManager 경고 1회
- OpenXR eye gaze 및 BD controller 확장 미지원 경고
