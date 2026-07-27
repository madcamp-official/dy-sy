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
- `BP_DamageDummy`는 `BPI_Damageable`이 아니라 `BPI_Damage2`를 구현하며, 요청된 `BPI_Damageable` 에셋은 현재 로드되지 않음
- `BP_Sword.TrySwordDamage`의 Entry 실행선이 속도 Branch에 연결되지 않았고 Branch True 뒤에 `BPI_Damage2.ApplyDamage` 호출이 없어 실제 피해 실행 경로가 없음
- L_Test PIE에서 느린 접촉 무피해는 확인했으나 7회 빠른 재진입 후에도 Dummy Health가 100으로 유지됨
- L_Test PlayerStart를 `[-300, 0, 110]`으로 복구하고 `BP_XRPawn.SpawnCollisionHandlingMethod=AlwaysSpawn`으로 보강; 기존 충돌 위치와 기본 PlayerStart 양쪽 PIE에서 Pawn과 EquippedSword 정상 생성 확인
- Sword Combat 완료 전 `TrySwordDamage` 실행/피해 경로 수정, Compile, PIE `100 → 85`, continuous-overlap 중복 방지, 반복 타격 Destroy 재검증 필요

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
