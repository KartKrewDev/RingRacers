// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
/// \file  k_director.h
/// \brief SRB2kart automatic spectator camera.

#ifndef __K_DIRECTOR_H__
#define __K_DIRECTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

void K_InitDirector(void);
void K_UpdateDirector(void);
void K_DrawDirectorDebugger(void);
void K_DirectorFollowAttack(player_t *player, mobj_t *inflictor, mobj_t *source);
void K_ToggleDirector(UINT8 viewnum, boolean active);
boolean K_DirectorIsEnabled(UINT8 viewnum);
boolean K_DirectorIsAvailable(UINT8 viewnum);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_DIRECTOR_H__
