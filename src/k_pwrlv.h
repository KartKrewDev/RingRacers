#ifndef __K_PWRLV__
#define __K_PWRLV__

#include "doomtype.h"
#include "doomdef.h"

#define PWRLV_DISABLED -1
#define PWRLV_RACE 0
#define PWRLV_BATTLE 1
#define PWRLV_NUMTYPES 2

#define PWRLVRECORD_START 5000 //1000
#define PWRLVRECORD_DEF 5000
#define PWRLVRECORD_MIN 1
#define PWRLVRECORD_MAX 9999

extern SINT8 speedscramble;
extern SINT8 encorescramble;

extern UINT16 vspowerlevel[PWRLV_NUMTYPES];
extern UINT16 clientpowerlevels[MAXPLAYERS][PWRLV_NUMTYPES];
extern INT16 nospectategrief[MAXPLAYERS];

void K_ClearClientPowerLevels(void);
INT16 K_CalculatePowerLevelInc(INT16 diff);
INT16 K_CalculatePowerLevelAvg(void);
//void K_UpdatePowerLevels(void);
void K_SetPowerLevelScrambles(SINT8 powertype);
void K_PlayerForfeit(UINT8 playernum, boolean nopointloss);

#endif
