# Work Queue

Last synchronized: 2026-07-28 live audit

## Verified complete

- [x] Damage System / `BPI_Damage2`
- [x] Player health
- [x] XR pawn spawn
- [x] Left-stick locomotion
- [x] Right-stick turning disabled
- [x] Sword attachment

## P0 — current gameplay blocker

- [ ] CURRENT: End-to-end Sword or Fireball → Enemy damage → HP-bar PIE validation
- [ ] Enemy damage-to-zero, death, and widget-removal validation

## P1 — required prototype work

- [ ] Player death and restart
- [ ] Goblin AnimBP warning cleanup
- [ ] Finish magic Aura, Sword Trail, and Sword Wave launch integration
- [ ] Verify Goblin combat/death
- [ ] Verify Wave Manager behavior
- [ ] Boss actor and pattern 1
- [ ] Boss pattern 2
- [ ] Boss health UI / HUD
- [ ] Victory and defeat flow
- [ ] L_Test navigation decision and NavMesh validation if required

## P1 — external/device dependent

- [ ] Android SDK and Quest packaging preflight
- [ ] Quest build
- [ ] Physical HMD/controller tracking and input test
- [ ] Quest combat, UI, restart, victory, and defeat regression

## P2 — repository and polish

- [ ] Reconcile modified `Content/Maps/L_Test.umap` without discarding teammate work
- [ ] Resolve untracked nested `dy-sy/` repository and missing `.gitmodules` policy
- [ ] Complete Git LFS integrity check
- [ ] Quest performance pass, VFX/audio polish, final freeze
