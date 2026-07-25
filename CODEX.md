# Codex 실행 규칙

AGENTS.md를 최우선으로 따른다.

## Unreal 작업 시작 문장
모든 Unreal 작업은 다음 문장으로 시작한다.

Use the connected unreal_mcp server.

## 실행 원칙
- prompts/00_ProjectAudit.md부터 순서대로 실행한다.
- 실패한 프롬프트가 있으면 다음 단계로 넘어가지 않는다.
- 한 프롬프트 실행 후 사람이 Unreal Editor에서 검증한다.
- 정상 동작할 때마다 Git Commit을 만든다.
- 실제 MCP 호출 없이 파일만 읽고 추측해 답하지 않는다.
