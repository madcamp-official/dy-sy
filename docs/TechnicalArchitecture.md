# Technical Architecture

- BP_VRPlayer
- BP_Sword
- BP_ProjectileBase
- BP_Fireball
- BPI_Damageable
- BP_Enemy
- BP_Boss
- BP_WaveManager
- BP_GameFlowManager
- WBP_WristHUD
- WBP_BossHUD
- WBP_GameResult

피해는 BPI_Damageable로 통일한다.
UI와 게임 흐름은 Event Dispatcher를 우선한다.
