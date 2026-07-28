# Work Queue

Last synchronized: 2026-07-28 live audit

## Verified complete

- [x] Damage System / `BPI_Damage2`
- [x] Player health
- [x] XR pawn spawn
- [x] Left-stick locomotion
- [x] Right-stick turning disabled
- [x] Sword attachment
- [x] Automatic level restart on player death
- [x] Minimal player HUD

## P0 — current gameplay blocker

- [ ] CURRENT: Player Magic and Sword VFX Package

## P1 — required prototype work

- [ ] Finish magic Aura, Sword Trail, and Sword Wave launch integration
- [ ] Victory and defeat flow

## P1 — teammate-owned / coordination required

- [ ] Enemy damage, death, HP-bar, collision, AI, and animation validation
- [ ] Goblin AnimBP warning cleanup
- [ ] Wave Manager behavior
- [ ] Boss actor and pattern 1
- [ ] Boss pattern 2
- [ ] Boss health UI
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
