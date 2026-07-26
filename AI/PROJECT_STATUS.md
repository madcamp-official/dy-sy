# Project Status

## 현재 단계
P0 Foundation

## 작업 포커스
- 완료: Damage System
- 완료: VR Player Health
- CURRENT_TASK: Sword Combat
- NEXT_TASK: Fireball Combat

## 현재 차단 사항
- Damage System 차단 해제: `BPI_Damage2` 수동 구현과 PIE `100 → 50 → Destroy` 검증 완료
- 현재 Damage System 관련 차단 사항 없음
- Sword Combat 구현은 저장되었으나 MCP in-viewport PIE에서 tracked swing을 재현할 수 없어 성공 damage evidence가 없음
- Sword Combat 완료 전 VR Preview 또는 실제 헤드셋에서 `100 → 85` 및 continuous-overlap 중복 방지 검증 필요

## P0
- [ ] Git `dy-sy` 구성 의도 확인
- [x] Unreal MCP 라이브 연결 확인
- [x] Damage System 완료
- [x] Player HP
- [ ] Player Death / Restart
- [ ] Sword Combat
- [ ] Fireball Combat
- [ ] Enemy Combat
- [ ] Boss Slash
- [ ] Boss Slam
- [ ] Player HUD
- [ ] Boss HUD
- [ ] Win / Lose / Restart
- [ ] Quest Build
- [ ] Quest Device Test

## P1
- [ ] Wave Manager
- [ ] Parry
- [ ] Boss Phase
- [ ] VFX
- [ ] Audio
- [ ] Haptics

## P2
- [ ] Ultimate
- [ ] Score
- [ ] Voice
- [ ] Extra Enemy Variant

## 완료 기준
각 항목은 Compile, Save, PIE 검증, 회귀 테스트가 끝나야 체크합니다.
