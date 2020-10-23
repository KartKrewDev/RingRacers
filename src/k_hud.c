// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_hud.c
/// \brief HUD drawing functions exclusive to Kart

#include "k_hud.h"
#include "k_kart.h"
#include "k_battle.h"
#include "k_color.h"
#include "screen.h"
#include "doomtype.h"
#include "doomdef.h"
#include "hu_stuff.h"
#include "d_netcmd.h"
#include "v_video.h"
#include "r_draw.h"
#include "st_stuff.h"
#include "lua_hud.h"
#include "doomstat.h"
#include "d_clisrv.h"
#include "g_game.h"
#include "p_local.h"
#include "z_zone.h"
#include "m_cond.h"
#include "r_main.h"
#include "s_sound.h"
#include "r_things.h"

#define NUMPOSNUMS 10
#define NUMPOSFRAMES 7 // White, three blues, three reds
#define NUMWINFRAMES 6 // Red, yellow, green, cyan, blue, purple

//{ 	Patch Definitions
static patch_t *kp_nodraw;

static patch_t *kp_timesticker;
static patch_t *kp_timestickerwide;
static patch_t *kp_lapsticker;
static patch_t *kp_lapstickerwide;
static patch_t *kp_lapstickernarrow;
static patch_t *kp_splitlapflag;
static patch_t *kp_bumpersticker;
static patch_t *kp_bumperstickerwide;
static patch_t *kp_capsulesticker;
static patch_t *kp_capsulestickerwide;
static patch_t *kp_karmasticker;
static patch_t *kp_spheresticker;
static patch_t *kp_splitkarmabomb;
static patch_t *kp_timeoutsticker;

static patch_t *kp_prestartbulb[15];
static patch_t *kp_prestartletters[7];

static patch_t *kp_prestartbulb_split[15];
static patch_t *kp_prestartletters_split[7];

static patch_t *kp_startcountdown[20];
static patch_t *kp_racefault[6];
static patch_t *kp_racefinish[6];

static patch_t *kp_positionnum[NUMPOSNUMS][NUMPOSFRAMES];
static patch_t *kp_winnernum[NUMPOSFRAMES];

static patch_t *kp_facenum[MAXPLAYERS+1];
static patch_t *kp_facehighlight[8];

static patch_t *kp_spbminimap;

static patch_t *kp_ringsticker[2];
static patch_t *kp_ringstickersplit[4];
static patch_t *kp_ring[6];
static patch_t *kp_smallring[6];
static patch_t *kp_ringdebtminus;
static patch_t *kp_ringdebtminussmall;
static patch_t *kp_ringspblock[16];
static patch_t *kp_ringspblocksmall[16];

static patch_t *kp_speedometersticker;
static patch_t *kp_speedometerlabel[4];

static patch_t *kp_rankbumper;
static patch_t *kp_tinybumper[2];
static patch_t *kp_ranknobumpers;
static patch_t *kp_rankcapsule;

static patch_t *kp_battlewin;
static patch_t *kp_battlecool;
static patch_t *kp_battlelose;
static patch_t *kp_battlewait;
static patch_t *kp_battleinfo;
static patch_t *kp_wanted;
static patch_t *kp_wantedsplit;
static patch_t *kp_wantedreticle;

static patch_t *kp_itembg[4];
static patch_t *kp_itemtimer[2];
static patch_t *kp_itemmulsticker[2];
static patch_t *kp_itemx;

static patch_t *kp_superring[2];
static patch_t *kp_sneaker[2];
static patch_t *kp_rocketsneaker[2];
static patch_t *kp_invincibility[13];
static patch_t *kp_banana[2];
static patch_t *kp_eggman[2];
static patch_t *kp_orbinaut[5];
static patch_t *kp_jawz[2];
static patch_t *kp_mine[2];
static patch_t *kp_ballhog[2];
static patch_t *kp_selfpropelledbomb[2];
static patch_t *kp_grow[2];
static patch_t *kp_shrink[2];
static patch_t *kp_thundershield[2];
static patch_t *kp_bubbleshield[2];
static patch_t *kp_flameshield[2];
static patch_t *kp_hyudoro[2];
static patch_t *kp_pogospring[2];
static patch_t *kp_kitchensink[2];
static patch_t *kp_sadface[2];

static patch_t *kp_check[6];

static patch_t *kp_rival[2];
static patch_t *kp_localtag[4][2];

static patch_t *kp_eggnum[4];

static patch_t *kp_flameshieldmeter[104][2];
static patch_t *kp_flameshieldmeter_bg[16][2];

static patch_t *kp_fpview[3];
static patch_t *kp_inputwheel[5];

static patch_t *kp_challenger[25];

static patch_t *kp_lapanim_lap[7];
static patch_t *kp_lapanim_final[11];
static patch_t *kp_lapanim_number[10][3];
static patch_t *kp_lapanim_emblem[2];
static patch_t *kp_lapanim_hand[3];

static patch_t *kp_yougotem;
static patch_t *kp_itemminimap;

static patch_t *kp_alagles[10];
static patch_t *kp_blagles[6];

static patch_t *kp_cpu;

static patch_t *kp_nametagstem;

void K_LoadKartHUDGraphics(void)
{
	INT32 i, j;
	char buffer[9];

	// Null Stuff
	kp_nodraw = 				W_CachePatchName("K_TRNULL", PU_HUDGFX);

	// Stickers
	kp_timesticker = 			W_CachePatchName("K_STTIME", PU_HUDGFX);
	kp_timestickerwide = 		W_CachePatchName("K_STTIMW", PU_HUDGFX);
	kp_lapsticker = 			W_CachePatchName("K_STLAPS", PU_HUDGFX);
	kp_lapstickerwide = 		W_CachePatchName("K_STLAPW", PU_HUDGFX);
	kp_lapstickernarrow = 		W_CachePatchName("K_STLAPN", PU_HUDGFX);
	kp_splitlapflag = 			W_CachePatchName("K_SPTLAP", PU_HUDGFX);
	kp_bumpersticker = 			W_CachePatchName("K_STBALN", PU_HUDGFX);
	kp_bumperstickerwide = 		W_CachePatchName("K_STBALW", PU_HUDGFX);
	kp_capsulesticker = 		W_CachePatchName("K_STCAPN", PU_HUDGFX);
	kp_capsulestickerwide = 	W_CachePatchName("K_STCAPW", PU_HUDGFX);
	kp_karmasticker = 			W_CachePatchName("K_STKARM", PU_HUDGFX);
	kp_spheresticker = 			W_CachePatchName("K_STBSMT", PU_HUDGFX);
	kp_splitkarmabomb = 		W_CachePatchName("K_SPTKRM", PU_HUDGFX);
	kp_timeoutsticker = 		W_CachePatchName("K_STTOUT", PU_HUDGFX);

	// Pre-start countdown bulbs
	sprintf(buffer, "K_BULBxx");
	for (i = 0; i < 15; i++)
	{
		buffer[6] = '0'+((i+1)/10);
		buffer[7] = '0'+((i+1)%10);
		kp_prestartbulb[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	sprintf(buffer, "K_SBLBxx");
	for (i = 0; i < 15; i++)
	{
		buffer[6] = '0'+((i+1)/10);
		buffer[7] = '0'+((i+1)%10);
		kp_prestartbulb_split[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// Pre-start position letters
	kp_prestartletters[0] =		W_CachePatchName("K_PL_P", PU_HUDGFX);
	kp_prestartletters[1] =		W_CachePatchName("K_PL_O", PU_HUDGFX);
	kp_prestartletters[2] =		W_CachePatchName("K_PL_S", PU_HUDGFX);
	kp_prestartletters[3] =		W_CachePatchName("K_PL_I", PU_HUDGFX);
	kp_prestartletters[4] =		W_CachePatchName("K_PL_T", PU_HUDGFX);
	kp_prestartletters[5] =		W_CachePatchName("K_PL_N", PU_HUDGFX);
	kp_prestartletters[6] =		W_CachePatchName("K_PL_EX", PU_HUDGFX);

	kp_prestartletters_split[0] =		W_CachePatchName("K_SPL_P", PU_HUDGFX);
	kp_prestartletters_split[1] =		W_CachePatchName("K_SPL_O", PU_HUDGFX);
	kp_prestartletters_split[2] =		W_CachePatchName("K_SPL_S", PU_HUDGFX);
	kp_prestartletters_split[3] =		W_CachePatchName("K_SPL_I", PU_HUDGFX);
	kp_prestartletters_split[4] =		W_CachePatchName("K_SPL_T", PU_HUDGFX);
	kp_prestartletters_split[5] =		W_CachePatchName("K_SPL_N", PU_HUDGFX);
	kp_prestartletters_split[6] =		W_CachePatchName("K_SPL_EX", PU_HUDGFX);

	// Starting countdown
	kp_startcountdown[0] = 		W_CachePatchName("K_CNT3A", PU_HUDGFX);
	kp_startcountdown[1] = 		W_CachePatchName("K_CNT2A", PU_HUDGFX);
	kp_startcountdown[2] = 		W_CachePatchName("K_CNT1A", PU_HUDGFX);
	kp_startcountdown[3] = 		W_CachePatchName("K_CNTGOA", PU_HUDGFX);
	kp_startcountdown[4] = 		W_CachePatchName("K_DUEL1", PU_HUDGFX);
	kp_startcountdown[5] = 		W_CachePatchName("K_CNT3B", PU_HUDGFX);
	kp_startcountdown[6] = 		W_CachePatchName("K_CNT2B", PU_HUDGFX);
	kp_startcountdown[7] = 		W_CachePatchName("K_CNT1B", PU_HUDGFX);
	kp_startcountdown[8] = 		W_CachePatchName("K_CNTGOB", PU_HUDGFX);
	kp_startcountdown[9] = 		W_CachePatchName("K_DUEL2", PU_HUDGFX);
	// Splitscreen
	kp_startcountdown[10] = 	W_CachePatchName("K_SMC3A", PU_HUDGFX);
	kp_startcountdown[11] = 	W_CachePatchName("K_SMC2A", PU_HUDGFX);
	kp_startcountdown[12] = 	W_CachePatchName("K_SMC1A", PU_HUDGFX);
	kp_startcountdown[13] = 	W_CachePatchName("K_SMCGOA", PU_HUDGFX);
	kp_startcountdown[14] = 	W_CachePatchName("K_SDUEL1", PU_HUDGFX);
	kp_startcountdown[15] = 	W_CachePatchName("K_SMC3B", PU_HUDGFX);
	kp_startcountdown[16] = 	W_CachePatchName("K_SMC2B", PU_HUDGFX);
	kp_startcountdown[17] = 	W_CachePatchName("K_SMC1B", PU_HUDGFX);
	kp_startcountdown[18] = 	W_CachePatchName("K_SMCGOB", PU_HUDGFX);
	kp_startcountdown[19] = 	W_CachePatchName("K_SDUEL2", PU_HUDGFX);

	// Fault
	kp_racefault[0] = 			W_CachePatchName("K_FAULTA", PU_HUDGFX);
	kp_racefault[1] = 			W_CachePatchName("K_FAULTB", PU_HUDGFX);
	// Splitscreen
	kp_racefault[2] = 			W_CachePatchName("K_SMFLTA", PU_HUDGFX);
	kp_racefault[3] = 			W_CachePatchName("K_SMFLTB", PU_HUDGFX);
	// 2P splitscreen
	kp_racefault[4] = 			W_CachePatchName("K_2PFLTA", PU_HUDGFX);
	kp_racefault[5] = 			W_CachePatchName("K_2PFLTB", PU_HUDGFX);

	// Finish
	kp_racefinish[0] = 			W_CachePatchName("K_FINA", PU_HUDGFX);
	kp_racefinish[1] = 			W_CachePatchName("K_FINB", PU_HUDGFX);
	// Splitscreen
	kp_racefinish[2] = 			W_CachePatchName("K_SMFINA", PU_HUDGFX);
	kp_racefinish[3] = 			W_CachePatchName("K_SMFINB", PU_HUDGFX);
	// 2P splitscreen
	kp_racefinish[4] = 			W_CachePatchName("K_2PFINA", PU_HUDGFX);
	kp_racefinish[5] = 			W_CachePatchName("K_2PFINB", PU_HUDGFX);

	// Position numbers
	sprintf(buffer, "K_POSNxx");
	for (i = 0; i < NUMPOSNUMS; i++)
	{
		buffer[6] = '0'+i;
		for (j = 0; j < NUMPOSFRAMES; j++)
		{
			//sprintf(buffer, "K_POSN%d%d", i, j);
			buffer[7] = '0'+j;
			kp_positionnum[i][j] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
		}
	}

	sprintf(buffer, "K_POSNWx");
	for (i = 0; i < NUMWINFRAMES; i++)
	{
		buffer[7] = '0'+i;
		kp_winnernum[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	sprintf(buffer, "OPPRNKxx");
	for (i = 0; i <= MAXPLAYERS; i++)
	{
		buffer[6] = '0'+(i/10);
		buffer[7] = '0'+(i%10);
		kp_facenum[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	sprintf(buffer, "K_CHILIx");
	for (i = 0; i < 8; i++)
	{
		buffer[7] = '0'+(i+1);
		kp_facehighlight[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	kp_spbminimap =				W_CachePatchName("SPBMMAP", PU_HUDGFX);

	// Rings & Lives
	kp_ringsticker[0] =			W_CachePatchName("RNGBACKA", PU_HUDGFX);
	kp_ringsticker[1] =			W_CachePatchName("RNGBACKB", PU_HUDGFX);

	sprintf(buffer, "K_RINGx");
	for (i = 0; i < 6; i++)
	{
		buffer[6] = '0'+(i+1);
		kp_ring[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	kp_ringdebtminus =			W_CachePatchName("RDEBTMIN", PU_HUDGFX);

	sprintf(buffer, "SPBRNGxx");
	for (i = 0; i < 16; i++)
	{
		buffer[6] = '0'+((i+1) / 10);
		buffer[7] = '0'+((i+1) % 10);
		kp_ringspblock[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	kp_ringstickersplit[0] =	W_CachePatchName("SMRNGBGA", PU_HUDGFX);
	kp_ringstickersplit[1] =	W_CachePatchName("SMRNGBGB", PU_HUDGFX);

	sprintf(buffer, "K_SRINGx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '0'+(i+1);
		kp_smallring[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	kp_ringdebtminussmall =		W_CachePatchName("SRDEBTMN", PU_HUDGFX);

	sprintf(buffer, "SPBRGSxx");
	for (i = 0; i < 16; i++)
	{
		buffer[6] = '0'+((i+1) / 10);
		buffer[7] = '0'+((i+1) % 10);
		kp_ringspblocksmall[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// Speedometer
	kp_speedometersticker =		W_CachePatchName("K_SPDMBG", PU_HUDGFX);

	sprintf(buffer, "K_SPDMLx");
	for (i = 0; i < 4; i++)
	{
		buffer[7] = '0'+(i+1);
		kp_speedometerlabel[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// Extra ranking icons
	kp_rankbumper =				W_CachePatchName("K_BLNICO", PU_HUDGFX);
	kp_tinybumper[0] =			W_CachePatchName("K_BLNA", PU_HUDGFX);
	kp_tinybumper[1] =			W_CachePatchName("K_BLNB", PU_HUDGFX);
	kp_ranknobumpers =			W_CachePatchName("K_NOBLNS", PU_HUDGFX);
	kp_rankcapsule =			W_CachePatchName("K_CAPICO", PU_HUDGFX);

	// Battle graphics
	kp_battlewin = 				W_CachePatchName("K_BWIN", PU_HUDGFX);
	kp_battlecool = 			W_CachePatchName("K_BCOOL", PU_HUDGFX);
	kp_battlelose = 			W_CachePatchName("K_BLOSE", PU_HUDGFX);
	kp_battlewait = 			W_CachePatchName("K_BWAIT", PU_HUDGFX);
	kp_battleinfo = 			W_CachePatchName("K_BINFO", PU_HUDGFX);
	kp_wanted = 				W_CachePatchName("K_WANTED", PU_HUDGFX);
	kp_wantedsplit = 			W_CachePatchName("4PWANTED", PU_HUDGFX);
	kp_wantedreticle =			W_CachePatchName("MMAPWANT", PU_HUDGFX);

	// Kart Item Windows
	kp_itembg[0] = 				W_CachePatchName("K_ITBG", PU_HUDGFX);
	kp_itembg[1] = 				W_CachePatchName("K_ITBGD", PU_HUDGFX);
	kp_itemtimer[0] = 			W_CachePatchName("K_ITIMER", PU_HUDGFX);
	kp_itemmulsticker[0] = 		W_CachePatchName("K_ITMUL", PU_HUDGFX);
	kp_itemx = 					W_CachePatchName("K_ITX", PU_HUDGFX);

	kp_superring[0] =			W_CachePatchName("K_ITRING", PU_HUDGFX);
	kp_sneaker[0] =				W_CachePatchName("K_ITSHOE", PU_HUDGFX);
	kp_rocketsneaker[0] =		W_CachePatchName("K_ITRSHE", PU_HUDGFX);

	sprintf(buffer, "K_ITINVx");
	for (i = 0; i < 7; i++)
	{
		buffer[7] = '1'+i;
		kp_invincibility[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}
	kp_banana[0] =				W_CachePatchName("K_ITBANA", PU_HUDGFX);
	kp_eggman[0] =				W_CachePatchName("K_ITEGGM", PU_HUDGFX);
	sprintf(buffer, "K_ITORBx");
	for (i = 0; i < 4; i++)
	{
		buffer[7] = '1'+i;
		kp_orbinaut[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}
	kp_jawz[0] =				W_CachePatchName("K_ITJAWZ", PU_HUDGFX);
	kp_mine[0] =				W_CachePatchName("K_ITMINE", PU_HUDGFX);
	kp_ballhog[0] =				W_CachePatchName("K_ITBHOG", PU_HUDGFX);
	kp_selfpropelledbomb[0] =	W_CachePatchName("K_ITSPB", PU_HUDGFX);
	kp_grow[0] =				W_CachePatchName("K_ITGROW", PU_HUDGFX);
	kp_shrink[0] =				W_CachePatchName("K_ITSHRK", PU_HUDGFX);
	kp_thundershield[0] =		W_CachePatchName("K_ITTHNS", PU_HUDGFX);
	kp_bubbleshield[0] =		W_CachePatchName("K_ITBUBS", PU_HUDGFX);
	kp_flameshield[0] =			W_CachePatchName("K_ITFLMS", PU_HUDGFX);
	kp_hyudoro[0] = 			W_CachePatchName("K_ITHYUD", PU_HUDGFX);
	kp_pogospring[0] = 			W_CachePatchName("K_ITPOGO", PU_HUDGFX);
	kp_kitchensink[0] = 		W_CachePatchName("K_ITSINK", PU_HUDGFX);
	kp_sadface[0] = 			W_CachePatchName("K_ITSAD", PU_HUDGFX);

	sprintf(buffer, "FSMFGxxx");
	for (i = 0; i < 104; i++)
	{
		buffer[5] = '0'+((i+1)/100);
		buffer[6] = '0'+(((i+1)/10)%10);
		buffer[7] = '0'+((i+1)%10);
		kp_flameshieldmeter[i][0] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	sprintf(buffer, "FSMBG0xx");
	for (i = 0; i < 16; i++)
	{
		buffer[6] = '0'+((i+1)/10);
		buffer[7] = '0'+((i+1)%10);
		kp_flameshieldmeter_bg[i][0] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// Splitscreen
	kp_itembg[2] = 				W_CachePatchName("K_ISBG", PU_HUDGFX);
	kp_itembg[3] = 				W_CachePatchName("K_ISBGD", PU_HUDGFX);
	kp_itemtimer[1] = 			W_CachePatchName("K_ISIMER", PU_HUDGFX);
	kp_itemmulsticker[1] = 		W_CachePatchName("K_ISMUL", PU_HUDGFX);

	kp_superring[1] =			W_CachePatchName("K_ISRING", PU_HUDGFX);
	kp_sneaker[1] =				W_CachePatchName("K_ISSHOE", PU_HUDGFX);
	kp_rocketsneaker[1] =		W_CachePatchName("K_ISRSHE", PU_HUDGFX);
	sprintf(buffer, "K_ISINVx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '1'+i;
		kp_invincibility[i+7] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}
	kp_banana[1] =				W_CachePatchName("K_ISBANA", PU_HUDGFX);
	kp_eggman[1] =				W_CachePatchName("K_ISEGGM", PU_HUDGFX);
	kp_orbinaut[4] =			W_CachePatchName("K_ISORBN", PU_HUDGFX);
	kp_jawz[1] =				W_CachePatchName("K_ISJAWZ", PU_HUDGFX);
	kp_mine[1] =				W_CachePatchName("K_ISMINE", PU_HUDGFX);
	kp_ballhog[1] =				W_CachePatchName("K_ISBHOG", PU_HUDGFX);
	kp_selfpropelledbomb[1] =	W_CachePatchName("K_ISSPB", PU_HUDGFX);
	kp_grow[1] =				W_CachePatchName("K_ISGROW", PU_HUDGFX);
	kp_shrink[1] =				W_CachePatchName("K_ISSHRK", PU_HUDGFX);
	kp_thundershield[1] =		W_CachePatchName("K_ISTHNS", PU_HUDGFX);
	kp_bubbleshield[1] =		W_CachePatchName("K_ISBUBS", PU_HUDGFX);
	kp_flameshield[1] =			W_CachePatchName("K_ISFLMS", PU_HUDGFX);
	kp_hyudoro[1] = 			W_CachePatchName("K_ISHYUD", PU_HUDGFX);
	kp_pogospring[1] = 			W_CachePatchName("K_ISPOGO", PU_HUDGFX);
	kp_kitchensink[1] = 		W_CachePatchName("K_ISSINK", PU_HUDGFX);
	kp_sadface[1] = 			W_CachePatchName("K_ISSAD", PU_HUDGFX);

	sprintf(buffer, "FSMFSxxx");
	for (i = 0; i < 104; i++)
	{
		buffer[5] = '0'+((i+1)/100);
		buffer[6] = '0'+(((i+1)/10)%10);
		buffer[7] = '0'+((i+1)%10);
		kp_flameshieldmeter[i][1] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	sprintf(buffer, "FSMBS0xx");
	for (i = 0; i < 16; i++)
	{
		buffer[6] = '0'+((i+1)/10);
		buffer[7] = '0'+((i+1)%10);
		kp_flameshieldmeter_bg[i][1] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// CHECK indicators
	sprintf(buffer, "K_CHECKx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '1'+i;
		kp_check[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// Rival indicators
	sprintf(buffer, "K_RIVALx");
	for (i = 0; i < 2; i++)
	{
		buffer[7] = '1'+i;
		kp_rival[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// Rival indicators
	sprintf(buffer, "K_SSPLxx");
	for (i = 0; i < 4; i++)
	{
		buffer[6] = 'A'+i;
		for (j = 0; j < 2; j++)
		{
			buffer[7] = '1'+j;
			kp_localtag[i][j] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
		}
	}

	// Eggman warning numbers
	sprintf(buffer, "K_EGGNx");
	for (i = 0; i < 4; i++)
	{
		buffer[6] = '0'+i;
		kp_eggnum[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// First person mode
	kp_fpview[0] = 				W_CachePatchName("VIEWA0", PU_HUDGFX);
	kp_fpview[1] =				W_CachePatchName("VIEWB0D0", PU_HUDGFX);
	kp_fpview[2] = 				W_CachePatchName("VIEWC0E0", PU_HUDGFX);

	// Input UI Wheel
	sprintf(buffer, "K_WHEELx");
	for (i = 0; i < 5; i++)
	{
		buffer[7] = '0'+i;
		kp_inputwheel[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// HERE COMES A NEW CHALLENGER
	sprintf(buffer, "K_CHALxx");
	for (i = 0; i < 25; i++)
	{
		buffer[6] = '0'+((i+1)/10);
		buffer[7] = '0'+((i+1)%10);
		kp_challenger[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// Lap start animation
	sprintf(buffer, "K_LAP0x");
	for (i = 0; i < 7; i++)
	{
		buffer[6] = '0'+(i+1);
		kp_lapanim_lap[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	sprintf(buffer, "K_LAPFxx");
	for (i = 0; i < 11; i++)
	{
		buffer[6] = '0'+((i+1)/10);
		buffer[7] = '0'+((i+1)%10);
		kp_lapanim_final[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	sprintf(buffer, "K_LAPNxx");
	for (i = 0; i < 10; i++)
	{
		buffer[6] = '0'+i;
		for (j = 0; j < 3; j++)
		{
			buffer[7] = '0'+(j+1);
			kp_lapanim_number[i][j] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
		}
	}

	sprintf(buffer, "K_LAPE0x");
	for (i = 0; i < 2; i++)
	{
		buffer[7] = '0'+(i+1);
		kp_lapanim_emblem[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	sprintf(buffer, "K_LAPH0x");
	for (i = 0; i < 3; i++)
	{
		buffer[7] = '0'+(i+1);
		kp_lapanim_hand[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	kp_yougotem = (patch_t *) W_CachePatchName("YOUGOTEM", PU_HUDGFX);
	kp_itemminimap = (patch_t *) W_CachePatchName("MMAPITEM", PU_HUDGFX);

	sprintf(buffer, "ALAGLESx");
	for (i = 0; i < 10; ++i)
	{
		buffer[7] = '0'+i;
		kp_alagles[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	sprintf(buffer, "BLAGLESx");
	for (i = 0; i < 6; ++i)
	{
		buffer[7] = '0'+i;
		kp_blagles[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	kp_cpu = (patch_t *) W_CachePatchName("K_CPU", PU_HUDGFX);

	kp_nametagstem = (patch_t *) W_CachePatchName("K_NAMEST", PU_HUDGFX);
}

// For the item toggle menu
const char *K_GetItemPatch(UINT8 item, boolean tiny)
{
	switch (item)
	{
		case KITEM_SNEAKER:
		case KRITEM_DUALSNEAKER:
		case KRITEM_TRIPLESNEAKER:
			return (tiny ? "K_ISSHOE" : "K_ITSHOE");
		case KITEM_ROCKETSNEAKER:
			return (tiny ? "K_ISRSHE" : "K_ITRSHE");
		case KITEM_INVINCIBILITY:
			return (tiny ? "K_ISINV1" : "K_ITINV1");
		case KITEM_BANANA:
		case KRITEM_TRIPLEBANANA:
		case KRITEM_TENFOLDBANANA:
			return (tiny ? "K_ISBANA" : "K_ITBANA");
		case KITEM_EGGMAN:
			return (tiny ? "K_ISEGGM" : "K_ITEGGM");
		case KITEM_ORBINAUT:
			return (tiny ? "K_ISORBN" : "K_ITORB1");
		case KITEM_JAWZ:
		case KRITEM_DUALJAWZ:
			return (tiny ? "K_ISJAWZ" : "K_ITJAWZ");
		case KITEM_MINE:
			return (tiny ? "K_ISMINE" : "K_ITMINE");
		case KITEM_BALLHOG:
			return (tiny ? "K_ISBHOG" : "K_ITBHOG");
		case KITEM_SPB:
			return (tiny ? "K_ISSPB" : "K_ITSPB");
		case KITEM_GROW:
			return (tiny ? "K_ISGROW" : "K_ITGROW");
		case KITEM_SHRINK:
			return (tiny ? "K_ISSHRK" : "K_ITSHRK");
		case KITEM_THUNDERSHIELD:
			return (tiny ? "K_ISTHNS" : "K_ITTHNS");
		case KITEM_BUBBLESHIELD:
			return (tiny ? "K_ISBUBS" : "K_ITBUBS");
		case KITEM_FLAMESHIELD:
			return (tiny ? "K_ISFLMS" : "K_ITFLMS");
		case KITEM_HYUDORO:
			return (tiny ? "K_ISHYUD" : "K_ITHYUD");
		case KITEM_POGOSPRING:
			return (tiny ? "K_ISPOGO" : "K_ITPOGO");
		case KITEM_SUPERRING:
			return (tiny ? "K_ISRING" : "K_ITRING");
		case KITEM_KITCHENSINK:
			return (tiny ? "K_ISSINK" : "K_ITSINK");
		case KRITEM_TRIPLEORBINAUT:
			return (tiny ? "K_ISORBN" : "K_ITORB3");
		case KRITEM_QUADORBINAUT:
			return (tiny ? "K_ISORBN" : "K_ITORB4");
		default:
			return (tiny ? "K_ISSAD" : "K_ITSAD");
	}
}

//}

INT32 ITEM_X, ITEM_Y;	// Item Window
INT32 TIME_X, TIME_Y;	// Time Sticker
INT32 LAPS_X, LAPS_Y;	// Lap Sticker
INT32 POSI_X, POSI_Y;	// Position Number
INT32 FACE_X, FACE_Y;	// Top-four Faces
INT32 STCD_X, STCD_Y;	// Starting countdown
INT32 CHEK_Y;			// CHECK graphic
INT32 MINI_X, MINI_Y;	// Minimap
INT32 WANT_X, WANT_Y;	// Battle WANTED poster

// This is for the P2 and P4 side of splitscreen. Then we'll flip P1's and P2's to the bottom with V_SPLITSCREEN.
INT32 ITEM2_X, ITEM2_Y;
INT32 LAPS2_X, LAPS2_Y;
INT32 POSI2_X, POSI2_Y;


void K_AdjustXYWithSnap(INT32 *x, INT32 *y, UINT32 options, INT32 dupx, INT32 dupy)
{
	// dupx adjustments pretend that screen width is BASEVIDWIDTH * dupx
	INT32 screenwidth = vid.width;
	INT32 screenheight = vid.height;
	INT32 basewidth = BASEVIDWIDTH * dupx;
	INT32 baseheight = BASEVIDHEIGHT * dupy;
	SINT8 player = -1;
	UINT8 i;

	if (options & V_SPLITSCREEN)
	{
		if (r_splitscreen > 0)
		{
			screenheight /= 2;
			baseheight /= 2;
		}

		if (r_splitscreen > 1)
		{
			screenwidth /= 2;
			basewidth /= 2;
		}
	}

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (stplyr == &players[displayplayers[i]])
		{
			player = i;
			break;
		}
	}

	if (vid.width != (BASEVIDWIDTH * dupx))
	{
		if (options & V_SNAPTORIGHT)
			*x += (screenwidth - basewidth);
		else if (!(options & V_SNAPTOLEFT))
			*x += (screenwidth - basewidth) / 2;
	}

	if (vid.height != (BASEVIDHEIGHT * dupy))
	{
		if (options & V_SNAPTOBOTTOM)
			*y += (screenheight - baseheight);
		else if (!(options & V_SNAPTOTOP))
			*y += (screenheight - baseheight) / 2;
	}

	if (options & V_SPLITSCREEN)
	{
		if (r_splitscreen == 1)
		{
			if (player == 1)
				*y += screenheight;
		}
		else if (r_splitscreen > 1)
		{
			if (player == 1 || player == 3)
				*x += screenwidth;

			if (player == 2 || player == 3)
				*y += screenheight;
		}
	}

	if (options & V_SLIDEIN)
	{
		tic_t length = TICRATE/2;

		if (leveltime < introtime + length)
		{
			INT32 offset = screenwidth - (((leveltime - introtime) * screenwidth) / length);
			boolean slidefromright = false;

			if (r_splitscreen > 1)
			{
				if (stplyr == &players[displayplayers[1]] || stplyr == &players[displayplayers[3]])
					slidefromright = true;
			}

			if (options & V_SNAPTORIGHT)
				slidefromright = true;
			else if (options & V_SNAPTOLEFT)
				slidefromright = false;

			if (slidefromright == true)
			{
				offset = -offset;
			}

			*x -= offset;
		}
	}
}

static void K_initKartHUD(void)
{
	/*
		BASEVIDWIDTH  = 320
		BASEVIDHEIGHT = 200

		Item window graphic is 41 x 33

		Time Sticker graphic is 116 x 11
		Time Font is a solid block of (8 x [12) x 14], equal to 96 x 14
		Therefore, timestamp is 116 x 14 altogether

		Lap Sticker is 80 x 11
		Lap flag is 22 x 20
		Lap Font is a solid block of (3 x [12) x 14], equal to 36 x 14
		Therefore, lapstamp is 80 x 20 altogether

		Position numbers are 43 x 53

		Faces are 32 x 32
		Faces draw downscaled at 16 x 16
		Therefore, the allocated space for them is 16 x 67 altogether

		----

		ORIGINAL CZ64 SPLITSCREEN:

		Item window:
		if (!splitscreen) 	{ ICONX = 139; 				ICONY = 20; }
		else 				{ ICONX = BASEVIDWIDTH-315; ICONY = 60; }

		Time: 			   236, STRINGY(			   12)
		Lap:  BASEVIDWIDTH-304, STRINGY(BASEVIDHEIGHT-189)

	*/

	// Single Screen (defaults)
	// Item Window
	ITEM_X = 5;						//   5
	ITEM_Y = 5;						//   5
	// Level Timer
	TIME_X = BASEVIDWIDTH - 148;	// 172
	TIME_Y = 9;						//   9
	// Level Laps
	LAPS_X = 9;						//   9
	LAPS_Y = BASEVIDHEIGHT - 29;	// 171
	// Position Number
	POSI_X = BASEVIDWIDTH  - 9;		// 268
	POSI_Y = BASEVIDHEIGHT - 9;		// 138
	// Top-Four Faces
	FACE_X = 9;						//   9
	FACE_Y = 92;					//  92
	// Starting countdown
	STCD_X = BASEVIDWIDTH/2;		//   9
	STCD_Y = BASEVIDHEIGHT/2;		//  92
	// CHECK graphic
	CHEK_Y = BASEVIDHEIGHT;			// 200
	// Minimap
	MINI_X = BASEVIDWIDTH - 50;		// 270
	MINI_Y = (BASEVIDHEIGHT/2)-16; //  84
	// Battle WANTED poster
	WANT_X = BASEVIDWIDTH - 55;		// 270
	WANT_Y = BASEVIDHEIGHT- 71;		// 176

	if (r_splitscreen)	// Splitscreen
	{
		ITEM_X = 5;
		ITEM_Y = 3;

		LAPS_Y = (BASEVIDHEIGHT/2)-24;

		POSI_Y = (BASEVIDHEIGHT/2)- 2;

		STCD_Y = BASEVIDHEIGHT/4;

		MINI_Y = (BASEVIDHEIGHT/2);

		if (r_splitscreen > 1)	// 3P/4P Small Splitscreen
		{
			// 1P (top left)
			ITEM_X = -9;
			ITEM_Y = -8;

			LAPS_X = 3;
			LAPS_Y = (BASEVIDHEIGHT/2)-12;

			POSI_X = 24;
			POSI_Y = (BASEVIDHEIGHT/2)-26;

			// 2P (top right)
			ITEM2_X = (BASEVIDWIDTH/2)-39;
			ITEM2_Y = -8;

			LAPS2_X = (BASEVIDWIDTH/2)-43;
			LAPS2_Y = (BASEVIDHEIGHT/2)-12;

			POSI2_X = (BASEVIDWIDTH/2)-4;
			POSI2_Y = (BASEVIDHEIGHT/2)-26;

			// Reminder that 3P and 4P are just 1P and 2P splitscreen'd to the bottom.

			STCD_X = BASEVIDWIDTH/4;

			MINI_X = (3*BASEVIDWIDTH/4);
			MINI_Y = (3*BASEVIDHEIGHT/4);

			if (r_splitscreen > 2) // 4P-only
			{
				MINI_X = (BASEVIDWIDTH/2);
				MINI_Y = (BASEVIDHEIGHT/2);
			}
		}
	}
}

static void K_drawKartItem(void)
{
	// ITEM_X = BASEVIDWIDTH-50;	// 270
	// ITEM_Y = 24;					//  24

	// Why write V_DrawScaledPatch calls over and over when they're all the same?
	// Set to 'no item' just in case.
	const UINT8 offset = ((r_splitscreen > 1) ? 1 : 0);
	patch_t *localpatch = kp_nodraw;
	patch_t *localbg = ((offset) ? kp_itembg[2] : kp_itembg[0]);
	patch_t *localinv = ((offset) ? kp_invincibility[((leveltime % (6*3)) / 3) + 7] : kp_invincibility[(leveltime % (7*3)) / 3]);
	INT32 fx = 0, fy = 0, fflags = 0;	// final coords for hud and flags...
	const INT32 numberdisplaymin = ((!offset && stplyr->kartstuff[k_itemtype] == KITEM_ORBINAUT) ? 5 : 2);
	INT32 itembar = 0;
	INT32 maxl = 0; // itembar's normal highest value
	const INT32 barlength = (r_splitscreen > 1 ? 12 : 26);
	UINT16 localcolor = SKINCOLOR_NONE;
	SINT8 colormode = TC_RAINBOW;
	UINT8 *colmap = NULL;
	boolean flipamount = false;	// Used for 3P/4P splitscreen to flip item amount stuff

	if (stplyr->kartstuff[k_itemroulette])
	{
		if (stplyr->skincolor)
			localcolor = stplyr->skincolor;

		switch((stplyr->kartstuff[k_itemroulette] % (15*3)) / 3)
		{
			// Each case is handled in threes, to give three frames of in-game time to see the item on the roulette
			case 0: // Sneaker
				localpatch = kp_sneaker[offset];
				//localcolor = SKINCOLOR_RASPBERRY;
				break;
			case 1: // Banana
				localpatch = kp_banana[offset];
				//localcolor = SKINCOLOR_YELLOW;
				break;
			case 2: // Orbinaut
				localpatch = kp_orbinaut[3+offset];
				//localcolor = SKINCOLOR_STEEL;
				break;
			case 3: // Mine
				localpatch = kp_mine[offset];
				//localcolor = SKINCOLOR_JET;
				break;
			case 4: // Grow
				localpatch = kp_grow[offset];
				//localcolor = SKINCOLOR_TEAL;
				break;
			case 5: // Hyudoro
				localpatch = kp_hyudoro[offset];
				//localcolor = SKINCOLOR_STEEL;
				break;
			case 6: // Rocket Sneaker
				localpatch = kp_rocketsneaker[offset];
				//localcolor = SKINCOLOR_TANGERINE;
				break;
			case 7: // Jawz
				localpatch = kp_jawz[offset];
				//localcolor = SKINCOLOR_JAWZ;
				break;
			case 8: // Self-Propelled Bomb
				localpatch = kp_selfpropelledbomb[offset];
				//localcolor = SKINCOLOR_JET;
				break;
			case 9: // Shrink
				localpatch = kp_shrink[offset];
				//localcolor = SKINCOLOR_ORANGE;
				break;
			case 10: // Invincibility
				localpatch = localinv;
				//localcolor = SKINCOLOR_GREY;
				break;
			case 11: // Eggman Monitor
				localpatch = kp_eggman[offset];
				//localcolor = SKINCOLOR_ROSE;
				break;
			case 12: // Ballhog
				localpatch = kp_ballhog[offset];
				//localcolor = SKINCOLOR_LILAC;
				break;
			case 13: // Thunder Shield
				localpatch = kp_thundershield[offset];
				//localcolor = SKINCOLOR_CYAN;
				break;
			case 14: // Super Ring
				localpatch = kp_superring[offset];
				//localcolor = SKINCOLOR_GOLD;
				break;
			/*case 15: // Pogo Spring
				localpatch = kp_pogospring[offset];
				localcolor = SKINCOLOR_TANGERINE;
				break;
			case 16: // Kitchen Sink
				localpatch = kp_kitchensink[offset];
				localcolor = SKINCOLOR_STEEL;
				break;*/
			default:
				break;
		}
	}
	else
	{
		// I'm doing this a little weird and drawing mostly in reverse order
		// The only actual reason is to make sneakers line up this way in the code below
		// This shouldn't have any actual baring over how it functions
		// Hyudoro is first, because we're drawing it on top of the player's current item
		if (stplyr->kartstuff[k_stolentimer] > 0)
		{
			if (leveltime & 2)
				localpatch = kp_hyudoro[offset];
			else
				localpatch = kp_nodraw;
		}
		else if ((stplyr->kartstuff[k_stealingtimer] > 0) && (leveltime & 2))
		{
			localpatch = kp_hyudoro[offset];
		}
		else if (stplyr->kartstuff[k_eggmanexplode] > 1)
		{
			if (leveltime & 1)
				localpatch = kp_eggman[offset];
			else
				localpatch = kp_nodraw;
		}
		else if (stplyr->kartstuff[k_rocketsneakertimer] > 1)
		{
			itembar = stplyr->kartstuff[k_rocketsneakertimer];
			maxl = (itemtime*3) - barlength;

			if (leveltime & 1)
				localpatch = kp_rocketsneaker[offset];
			else
				localpatch = kp_nodraw;
		}
		else if (stplyr->kartstuff[k_sadtimer] > 0)
		{
			if (leveltime & 2)
				localpatch = kp_sadface[offset];
			else
				localpatch = kp_nodraw;
		}
		else
		{
			if (stplyr->kartstuff[k_itemamount] <= 0)
				return;

			switch(stplyr->kartstuff[k_itemtype])
			{
				case KITEM_SNEAKER:
					localpatch = kp_sneaker[offset];
					break;
				case KITEM_ROCKETSNEAKER:
					localpatch = kp_rocketsneaker[offset];
					break;
				case KITEM_INVINCIBILITY:
					localpatch = localinv;
					localbg = kp_itembg[offset+1];
					break;
				case KITEM_BANANA:
					localpatch = kp_banana[offset];
					break;
				case KITEM_EGGMAN:
					localpatch = kp_eggman[offset];
					break;
				case KITEM_ORBINAUT:
					localpatch = kp_orbinaut[(offset ? 4 : min(stplyr->kartstuff[k_itemamount]-1, 3))];
					break;
				case KITEM_JAWZ:
					localpatch = kp_jawz[offset];
					break;
				case KITEM_MINE:
					localpatch = kp_mine[offset];
					break;
				case KITEM_BALLHOG:
					localpatch = kp_ballhog[offset];
					break;
				case KITEM_SPB:
					localpatch = kp_selfpropelledbomb[offset];
					localbg = kp_itembg[offset+1];
					break;
				case KITEM_GROW:
					localpatch = kp_grow[offset];
					break;
				case KITEM_SHRINK:
					localpatch = kp_shrink[offset];
					break;
				case KITEM_THUNDERSHIELD:
					localpatch = kp_thundershield[offset];
					localbg = kp_itembg[offset+1];
					break;
				case KITEM_BUBBLESHIELD:
					localpatch = kp_bubbleshield[offset];
					localbg = kp_itembg[offset+1];
					break;
				case KITEM_FLAMESHIELD:
					localpatch = kp_flameshield[offset];
					localbg = kp_itembg[offset+1];
					break;
				case KITEM_HYUDORO:
					localpatch = kp_hyudoro[offset];
					break;
				case KITEM_POGOSPRING:
					localpatch = kp_pogospring[offset];
					break;
				case KITEM_SUPERRING:
					localpatch = kp_superring[offset];
					break;
				case KITEM_KITCHENSINK:
					localpatch = kp_kitchensink[offset];
					break;
				case KITEM_SAD:
					localpatch = kp_sadface[offset];
					break;
				default:
					return;
			}

			if (stplyr->kartstuff[k_itemheld] && !(leveltime & 1))
				localpatch = kp_nodraw;
		}

		if (stplyr->karthud[khud_itemblink] && (leveltime & 1))
		{
			colormode = TC_BLINK;

			switch (stplyr->karthud[khud_itemblinkmode])
			{
				case 2:
					localcolor = K_RainbowColor(leveltime);
					break;
				case 1:
					localcolor = SKINCOLOR_RED;
					break;
				default:
					localcolor = SKINCOLOR_WHITE;
					break;
			}
		}
	}

	// pain and suffering defined below
	if (r_splitscreen < 2) // don't change shit for THIS splitscreen.
	{
		fx = ITEM_X;
		fy = ITEM_Y;
		fflags = V_SNAPTOTOP|V_SNAPTOLEFT|V_SPLITSCREEN;
	}
	else // now we're having a fun game.
	{
		if (stplyr == &players[displayplayers[0]] || stplyr == &players[displayplayers[2]]) // If we are P1 or P3...
		{
			fx = ITEM_X;
			fy = ITEM_Y;
			fflags = V_SNAPTOLEFT|V_SNAPTOTOP|V_SPLITSCREEN;
		}
		else // else, that means we're P2 or P4.
		{
			fx = ITEM2_X;
			fy = ITEM2_Y;
			fflags = V_SNAPTORIGHT|V_SNAPTOTOP|V_SPLITSCREEN;
			flipamount = true;
		}
	}

	if (localcolor != SKINCOLOR_NONE)
		colmap = R_GetTranslationColormap(colormode, localcolor, GTC_CACHE);

	V_DrawScaledPatch(fx, fy, V_HUDTRANS|V_SLIDEIN|fflags, localbg);

	// Then, the numbers:
	if (stplyr->kartstuff[k_itemamount] >= numberdisplaymin && !stplyr->kartstuff[k_itemroulette])
	{
		V_DrawScaledPatch(fx + (flipamount ? 48 : 0), fy, V_HUDTRANS|V_SLIDEIN|fflags|(flipamount ? V_FLIP : 0), kp_itemmulsticker[offset]); // flip this graphic for p2 and p4 in split and shift it.
		V_DrawFixedPatch(fx<<FRACBITS, fy<<FRACBITS, FRACUNIT, V_HUDTRANS|V_SLIDEIN|fflags, localpatch, colmap);
		if (offset)
			if (flipamount) // reminder that this is for 3/4p's right end of the screen.
				V_DrawString(fx+2, fy+31, V_ALLOWLOWERCASE|V_HUDTRANS|V_SLIDEIN|fflags, va("x%d", stplyr->kartstuff[k_itemamount]));
			else
				V_DrawString(fx+24, fy+31, V_ALLOWLOWERCASE|V_HUDTRANS|V_SLIDEIN|fflags, va("x%d", stplyr->kartstuff[k_itemamount]));
		else
		{
			V_DrawScaledPatch(fy+28, fy+41, V_HUDTRANS|V_SLIDEIN|fflags, kp_itemx);
			V_DrawKartString(fx+38, fy+36, V_HUDTRANS|V_SLIDEIN|fflags, va("%d", stplyr->kartstuff[k_itemamount]));
		}
	}
	else
		V_DrawFixedPatch(fx<<FRACBITS, fy<<FRACBITS, FRACUNIT, V_HUDTRANS|V_SLIDEIN|fflags, localpatch, colmap);

	// Extensible meter, currently only used for rocket sneaker...
	if (itembar)
	{
		const INT32 fill = ((itembar*barlength)/maxl);
		const INT32 length = min(barlength, fill);
		const INT32 height = (offset ? 1 : 2);
		const INT32 x = (offset ? 17 : 11), y = (offset ? 27 : 35);

		V_DrawScaledPatch(fx+x, fy+y, V_HUDTRANS|V_SLIDEIN|fflags, kp_itemtimer[offset]);
		// The left dark "AA" edge
		V_DrawFill(fx+x+1, fy+y+1, (length == 2 ? 2 : 1), height, 12|fflags);
		// The bar itself
		if (length > 2)
		{
			V_DrawFill(fx+x+length, fy+y+1, 1, height, 12|fflags); // the right one
			if (height == 2)
				V_DrawFill(fx+x+2, fy+y+2, length-2, 1, 8|fflags); // the dulled underside
			V_DrawFill(fx+x+2, fy+y+1, length-2, 1, 0|fflags); // the shine
		}
	}

	// Quick Eggman numbers
	if (stplyr->kartstuff[k_eggmanexplode] > 1 /*&& stplyr->kartstuff[k_eggmanexplode] <= 3*TICRATE*/)
		V_DrawScaledPatch(fx+17, fy+13-offset, V_HUDTRANS|V_SLIDEIN|fflags, kp_eggnum[min(3, G_TicsToSeconds(stplyr->kartstuff[k_eggmanexplode]))]);

	if (stplyr->kartstuff[k_itemtype] == KITEM_FLAMESHIELD && stplyr->kartstuff[k_flamelength] > 0)
	{
		INT32 numframes = 104;
		INT32 absolutemax = 16 * flameseg;
		INT32 flamemax = stplyr->kartstuff[k_flamelength] * flameseg;
		INT32 flamemeter = min(stplyr->kartstuff[k_flamemeter], flamemax);

		INT32 bf = 16 - stplyr->kartstuff[k_flamelength];
		INT32 ff = numframes - ((flamemeter * numframes) / absolutemax);
		INT32 fmin = (8 * (bf-1));

		INT32 xo = 6, yo = 4;
		INT32 flip = 0;

		if (offset)
		{
			xo++;

			if (stplyr == &players[displayplayers[0]] || stplyr == &players[displayplayers[2]]) // Flip for P1 and P3 (yes, that's correct)
			{
				xo -= 62;
				flip = V_FLIP;
			}
		}

		if (ff < fmin)
			ff = fmin;

		if (bf >= 0 && bf < 16)
			V_DrawScaledPatch(fx-xo, fy-yo, V_HUDTRANS|V_SLIDEIN|fflags|flip, kp_flameshieldmeter_bg[bf][offset]);

		if (ff >= 0 && ff < numframes && stplyr->kartstuff[k_flamemeter] > 0)
		{
			if ((stplyr->kartstuff[k_flamemeter] > flamemax) && (leveltime & 1))
			{
				UINT8 *fsflash = R_GetTranslationColormap(TC_BLINK, SKINCOLOR_WHITE, GTC_CACHE);
				V_DrawMappedPatch(fx-xo, fy-yo, V_HUDTRANS|V_SLIDEIN|fflags|flip, kp_flameshieldmeter[ff][offset], fsflash);
			}
			else
			{
				V_DrawScaledPatch(fx-xo, fy-yo, V_HUDTRANS|V_SLIDEIN|fflags|flip, kp_flameshieldmeter[ff][offset]);
			}
		}
	}
}

void K_drawKartTimestamp(tic_t drawtime, INT32 TX, INT32 TY, INT16 emblemmap, UINT8 mode)
{
	// TIME_X = BASEVIDWIDTH-124;	// 196
	// TIME_Y = 6;					//   6

	tic_t worktime;

	INT32 splitflags = 0;
	if (!mode)
	{
		splitflags = V_HUDTRANS|V_SLIDEIN|V_SNAPTOTOP|V_SNAPTORIGHT|V_SPLITSCREEN;
		if (cv_timelimit.value && timelimitintics > 0)
		{
			if (drawtime >= timelimitintics)
				drawtime = 0;
			else
				drawtime = timelimitintics - drawtime;
		}
	}

	V_DrawScaledPatch(TX, TY, splitflags, ((mode == 2) ? kp_lapstickerwide : kp_timestickerwide));

	TX += 33;

	worktime = drawtime/(60*TICRATE);

	if (mode && !drawtime)
		V_DrawKartString(TX, TY+3, splitflags, va("--'--\"--"));
	else if (worktime < 100) // 99:99:99 only
	{
		// zero minute
		if (worktime < 10)
		{
			V_DrawKartString(TX, TY+3, splitflags, va("0"));
			// minutes time       0 __ __
			V_DrawKartString(TX+12, TY+3, splitflags, va("%d", worktime));
		}
		// minutes time       0 __ __
		else
			V_DrawKartString(TX, TY+3, splitflags, va("%d", worktime));

		// apostrophe location     _'__ __
		V_DrawKartString(TX+24, TY+3, splitflags, va("'"));

		worktime = (drawtime/TICRATE % 60);

		// zero second        _ 0_ __
		if (worktime < 10)
		{
			V_DrawKartString(TX+36, TY+3, splitflags, va("0"));
		// seconds time       _ _0 __
			V_DrawKartString(TX+48, TY+3, splitflags, va("%d", worktime));
		}
		// zero second        _ 00 __
		else
			V_DrawKartString(TX+36, TY+3, splitflags, va("%d", worktime));

		// quotation mark location    _ __"__
		V_DrawKartString(TX+60, TY+3, splitflags, va("\""));

		worktime = G_TicsToCentiseconds(drawtime);

		// zero tick          _ __ 0_
		if (worktime < 10)
		{
			V_DrawKartString(TX+72, TY+3, splitflags, va("0"));
		// tics               _ __ _0
			V_DrawKartString(TX+84, TY+3, splitflags, va("%d", worktime));
		}
		// zero tick          _ __ 00
		else
			V_DrawKartString(TX+72, TY+3, splitflags, va("%d", worktime));
	}
	else if ((drawtime/TICRATE) & 1)
		V_DrawKartString(TX, TY+3, splitflags, va("99'59\"99"));

	if (emblemmap && (modeattacking || (mode == 1)) && !demo.playback) // emblem time!
	{
		INT32 workx = TX + 96, worky = TY+18;
		SINT8 curemb = 0;
		patch_t *emblempic[3] = {NULL, NULL, NULL};
		UINT8 *emblemcol[3] = {NULL, NULL, NULL};

		emblem_t *emblem = M_GetLevelEmblems(emblemmap);
		while (emblem)
		{
			char targettext[9];

			switch (emblem->type)
			{
				case ET_TIME:
					{
						static boolean canplaysound = true;
						tic_t timetoreach = emblem->var;

						if (emblem->collected)
						{
							emblempic[curemb] = W_CachePatchName(M_GetEmblemPatch(emblem, false), PU_CACHE);
							emblemcol[curemb] = R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_CACHE);
							if (++curemb == 3)
								break;
							goto bademblem;
						}

						snprintf(targettext, 9, "%i'%02i\"%02i",
							G_TicsToMinutes(timetoreach, false),
							G_TicsToSeconds(timetoreach),
							G_TicsToCentiseconds(timetoreach));

						if (!mode)
						{
							if (stplyr->realtime > timetoreach)
							{
								splitflags = (splitflags &~ V_HUDTRANS)|V_HUDTRANSHALF;
								if (canplaysound)
								{
									S_StartSound(NULL, sfx_s3k72); //sfx_s26d); -- you STOLE fizzy lifting drinks
									canplaysound = false;
								}
							}
							else if (!canplaysound)
								canplaysound = true;
						}

						targettext[8] = 0;
					}
					break;
				default:
					goto bademblem;
			}

			V_DrawRightAlignedString(workx, worky, splitflags, targettext);
			workx -= 67;
			V_DrawSmallScaledPatch(workx + 4, worky, splitflags, W_CachePatchName("NEEDIT", PU_CACHE));

			break;

			bademblem:
			emblem = M_GetLevelEmblems(-1);
		}

		if (!mode)
			splitflags = (splitflags &~ V_HUDTRANSHALF)|V_HUDTRANS;
		while (curemb--)
		{
			workx -= 12;
			V_DrawSmallMappedPatch(workx + 4, worky, splitflags, emblempic[curemb], emblemcol[curemb]);
		}
	}
}

static void K_DrawKartPositionNum(INT32 num)
{
	// POSI_X = BASEVIDWIDTH - 51;	// 269
	// POSI_Y = BASEVIDHEIGHT- 64;	// 136

	boolean win = (stplyr->exiting && num == 1);
	//INT32 X = POSI_X;
	INT32 W = SHORT(kp_positionnum[0][0]->width);
	fixed_t scale = FRACUNIT;
	patch_t *localpatch = kp_positionnum[0][0];
	INT32 fx = 0, fy = 0, fflags = 0;
	boolean flipdraw = false;	// flip the order we draw it in for MORE splitscreen bs. fun.
	boolean flipvdraw = false;	// used only for 2p splitscreen so overtaking doesn't make 1P's position fly off the screen.
	boolean overtake = false;

	if (stplyr->kartstuff[k_positiondelay] || stplyr->exiting)
	{
		scale *= 2;
		overtake = true;	// this is used for splitscreen stuff in conjunction with flipdraw.
	}
	if (r_splitscreen)
		scale /= 2;

	W = FixedMul(W<<FRACBITS, scale)>>FRACBITS;

	// pain and suffering defined below
	if (!r_splitscreen)
	{
		fx = POSI_X;
		fy = BASEVIDHEIGHT - 8;
		fflags = V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN;
	}
	else if (r_splitscreen == 1)	// for this splitscreen, we'll use case by case because it's a bit different.
	{
		fx = POSI_X;
		if (stplyr == &players[displayplayers[0]])	// for player 1: display this at the top right, above the minimap.
		{
			fy = 30;
			fflags = V_SNAPTOTOP|V_SNAPTORIGHT|V_SPLITSCREEN;
			if (overtake)
				flipvdraw = true;	// make sure overtaking doesn't explode us
		}
		else	// if we're not p1, that means we're p2. display this at the bottom right, below the minimap.
		{
			fy = (BASEVIDHEIGHT/2) - 8;
			fflags = V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN;
		}
	}
	else
	{
		if (stplyr == &players[displayplayers[0]] || stplyr == &players[displayplayers[2]])	// If we are P1 or P3...
		{
			fx = POSI_X;
			fy = POSI_Y;
			fflags = V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_SPLITSCREEN;
			flipdraw = true;
			if (num && num >= 10)
				fx += W;	// this seems dumb, but we need to do this in order for positions above 10 going off screen.
		}
		else // else, that means we're P2 or P4.
		{
			fx = POSI2_X;
			fy = POSI2_Y;
			fflags = V_SNAPTORIGHT|V_SNAPTOBOTTOM|V_SPLITSCREEN;
		}
	}

	// Special case for 0
	if (!num)
	{
		V_DrawFixedPatch(fx<<FRACBITS, fy<<FRACBITS, scale, V_HUDTRANSHALF|V_SLIDEIN|fflags, kp_positionnum[0][0], NULL);
		return;
	}

	I_Assert(num >= 0); // This function does not draw negative numbers

	// Draw the number
	while (num)
	{
		if (win) // 1st place winner? You get rainbows!!
			localpatch = kp_winnernum[(leveltime % (NUMWINFRAMES*3)) / 3];
		else if (stplyr->laps >= cv_numlaps.value || stplyr->exiting) // Check for the final lap, or won
		{
			// Alternate frame every three frames
			switch (leveltime % 9)
			{
				case 1: case 2: case 3:
					if (K_IsPlayerLosing(stplyr))
						localpatch = kp_positionnum[num % 10][4];
					else
						localpatch = kp_positionnum[num % 10][1];
					break;
				case 4: case 5: case 6:
					if (K_IsPlayerLosing(stplyr))
						localpatch = kp_positionnum[num % 10][5];
					else
						localpatch = kp_positionnum[num % 10][2];
					break;
				case 7: case 8: case 9:
					if (K_IsPlayerLosing(stplyr))
						localpatch = kp_positionnum[num % 10][6];
					else
						localpatch = kp_positionnum[num % 10][3];
					break;
				default:
					localpatch = kp_positionnum[num % 10][0];
					break;
			}
		}
		else
			localpatch = kp_positionnum[num % 10][0];

		V_DrawFixedPatch((fx<<FRACBITS) + ((overtake && flipdraw) ? (SHORT(localpatch->width)*scale/2) : 0), (fy<<FRACBITS) + ((overtake && flipvdraw) ? (SHORT(localpatch->height)*scale/2) : 0), scale, V_HUDTRANSHALF|V_SLIDEIN|fflags, localpatch, NULL);
		// ^ if we overtake as p1 or p3 in splitscren, we shift it so that it doesn't go off screen.
		// ^ if we overtake as p1 in 2p splits, shift vertically so that this doesn't happen either.

		fx -= W;
		num /= 10;
	}
}

static boolean K_drawKartPositionFaces(void)
{
	// FACE_X = 15;				//  15
	// FACE_Y = 72;				//  72

	INT32 Y = FACE_Y+9; // +9 to offset where it's being drawn if there are more than one
	INT32 i, j, ranklines, strank = -1;
	boolean completed[MAXPLAYERS];
	INT32 rankplayer[MAXPLAYERS];
	INT32 bumperx, numplayersingame = 0;
	UINT8 *colormap;

	ranklines = 0;
	memset(completed, 0, sizeof (completed));
	memset(rankplayer, 0, sizeof (rankplayer));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		rankplayer[i] = -1;

		if (!playeringame[i] || players[i].spectator || !players[i].mo)
			continue;

		numplayersingame++;
	}

	if (numplayersingame <= 1)
		return true;

	if (!LUA_HudEnabled(hud_minirankings))
		return false;	// Don't proceed but still return true for free play above if HUD is disabled.

	for (j = 0; j < numplayersingame; j++)
	{
		UINT8 lowestposition = MAXPLAYERS+1;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (completed[i] || !playeringame[i] || players[i].spectator || !players[i].mo)
				continue;

			if (players[i].kartstuff[k_position] >= lowestposition)
				continue;

			rankplayer[ranklines] = i;
			lowestposition = players[i].kartstuff[k_position];
		}

		i = rankplayer[ranklines];

		completed[i] = true;

		if (players+i == stplyr)
			strank = ranklines;

		//if (ranklines == 5)
			//break; // Only draw the top 5 players -- we do this a different way now...

		ranklines++;
	}

	if (ranklines < 5)
		Y -= (9*ranklines);
	else
		Y -= (9*5);

	if (gametype == GT_BATTLE || strank <= 2) // too close to the top, or playing battle, or a spectator? would have had (strank == -1) called out, but already caught by (strank <= 2)
	{
		i = 0;
		if (ranklines > 5) // could be both...
			ranklines = 5;
	}
	else if (strank+3 > ranklines) // too close to the bottom?
	{
		i = ranklines - 5;
		if (i < 0)
			i = 0;
	}
	else
	{
		i = strank-2;
		ranklines = strank+3;
	}

	for (; i < ranklines; i++)
	{
		if (!playeringame[rankplayer[i]]) continue;
		if (players[rankplayer[i]].spectator) continue;
		if (!players[rankplayer[i]].mo) continue;

		bumperx = FACE_X+19;

		if (players[rankplayer[i]].mo->color)
		{
			colormap = R_GetTranslationColormap(players[rankplayer[i]].skin, players[rankplayer[i]].mo->color, GTC_CACHE);
			if (players[rankplayer[i]].mo->colorized)
				colormap = R_GetTranslationColormap(TC_RAINBOW, players[rankplayer[i]].mo->color, GTC_CACHE);
			else
				colormap = R_GetTranslationColormap(players[rankplayer[i]].skin, players[rankplayer[i]].mo->color, GTC_CACHE);

			V_DrawMappedPatch(FACE_X, Y, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, faceprefix[players[rankplayer[i]].skin][FACE_RANK], colormap);

			if (LUA_HudEnabled(hud_battlebumpers))
			{
				if (gametype == GT_BATTLE && players[rankplayer[i]].kartstuff[k_bumper] > 0)
				{
					V_DrawMappedPatch(bumperx-2, Y, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_tinybumper[0], colormap);
					for (j = 1; j < players[rankplayer[i]].kartstuff[k_bumper]; j++)
					{
						bumperx += 5;
						V_DrawMappedPatch(bumperx, Y, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_tinybumper[1], colormap);
					}
				}
			}	// A new level of stupidity: checking if lua is enabled to close a bracket. :Fascinating:
		}

		if (i == strank)
			V_DrawScaledPatch(FACE_X, Y, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_facehighlight[(leveltime / 4) % 8]);

		if (gametype == GT_BATTLE && players[rankplayer[i]].kartstuff[k_bumper] <= 0)
			V_DrawScaledPatch(FACE_X-4, Y-3, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_ranknobumpers);
		else
		{
			INT32 pos = players[rankplayer[i]].kartstuff[k_position];
			if (pos < 0 || pos > MAXPLAYERS)
				pos = 0;
			// Draws the little number over the face
			V_DrawScaledPatch(FACE_X-5, Y+10, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_facenum[pos]);
		}

		Y += 18;
	}

	return false;
}

//
// HU_DrawTabRankings -- moved here to take advantage of kart stuff!
//
void K_DrawTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer, INT32 hilicol)
{
	static tic_t alagles_timer = 9;
	INT32 i, rightoffset = 240;
	const UINT8 *colormap;
	INT32 dupadjust = (vid.width/vid.dupx), duptweak = (dupadjust - BASEVIDWIDTH)/2;
	int y2;

	//this function is designed for 9 or less score lines only
	//I_Assert(scorelines <= 9); -- not today bitch, kart fixed it up

	V_DrawFill(1-duptweak, 26, dupadjust-2, 1, 0); // Draw a horizontal line because it looks nice!
	if (scorelines > 8)
	{
		V_DrawFill(160, 26, 1, 147, 0); // Draw a vertical line to separate the two sides.
		V_DrawFill(1-duptweak, 173, dupadjust-2, 1, 0); // And a horizontal line near the bottom.
		rightoffset = (BASEVIDWIDTH/2) - 4 - x;
	}

	for (i = 0; i < scorelines; i++)
	{
		char strtime[MAXPLAYERNAME+1];

		if (players[tab[i].num].spectator || !players[tab[i].num].mo)
			continue; //ignore them.

		if (netgame) // don't draw ping offline
		{
			if (players[tab[i].num].bot)
			{
				V_DrawScaledPatch(x + ((i < 8) ? -25 : rightoffset + 3), y-2, 0, kp_cpu);
			}
			else if (tab[i].num != serverplayer || !server_lagless)
			{
				HU_drawPing(x + ((i < 8) ? -17 : rightoffset + 11), y-4, playerpingtable[tab[i].num], 0);
			}
		}

		STRBUFCPY(strtime, tab[i].name);

		y2 = y;

		if (netgame && playerconsole[tab[i].num] == 0 && server_lagless && !players[tab[i].num].bot)
		{
			y2 = ( y - 4 );

			V_DrawScaledPatch(x + 20, y2, 0, kp_blagles[(leveltime / 3) % 6]);
			// every 70 tics
			if (( leveltime % 70 ) == 0)
			{
				alagles_timer = 9;
			}
			if (alagles_timer > 0)
			{
				V_DrawScaledPatch(x + 20, y2, 0, kp_alagles[alagles_timer]);
				if (( leveltime % 2 ) == 0)
					alagles_timer--;
			}
			else
				V_DrawScaledPatch(x + 20, y2, 0, kp_alagles[0]);

			y2 += SHORT (kp_alagles[0]->height) + 1;
		}

		if (scorelines > 8)
			V_DrawThinString(x + 20, y2, ((tab[i].num == whiteplayer) ? hilicol : 0)|V_ALLOWLOWERCASE|V_6WIDTHSPACE, strtime);
		else
			V_DrawString(x + 20, y2, ((tab[i].num == whiteplayer) ? hilicol : 0)|V_ALLOWLOWERCASE, strtime);

		if (players[tab[i].num].mo->color)
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo->color, GTC_CACHE);
			if (players[tab[i].num].mo->colorized)
				colormap = R_GetTranslationColormap(TC_RAINBOW, players[tab[i].num].mo->color, GTC_CACHE);
			else
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo->color, GTC_CACHE);

			V_DrawMappedPatch(x, y-4, 0, faceprefix[players[tab[i].num].skin][FACE_RANK], colormap);
			/*if (gametype == GT_BATTLE && players[tab[i].num].kartstuff[k_bumper] > 0) -- not enough space for this
			{
				INT32 bumperx = x+19;
				V_DrawMappedPatch(bumperx-2, y-4, 0, kp_tinybumper[0], colormap);
				for (j = 1; j < players[tab[i].num].kartstuff[k_bumper]; j++)
				{
					bumperx += 5;
					V_DrawMappedPatch(bumperx, y-4, 0, kp_tinybumper[1], colormap);
				}
			}*/
		}

		if (tab[i].num == whiteplayer)
			V_DrawScaledPatch(x, y-4, 0, kp_facehighlight[(leveltime / 4) % 8]);

		if (gametype == GT_BATTLE && players[tab[i].num].kartstuff[k_bumper] <= 0)
			V_DrawScaledPatch(x-4, y-7, 0, kp_ranknobumpers);
		else
		{
			INT32 pos = players[tab[i].num].kartstuff[k_position];
			if (pos < 0 || pos > MAXPLAYERS)
				pos = 0;
			// Draws the little number over the face
			V_DrawScaledPatch(x-5, y+6, 0, kp_facenum[pos]);
		}

		if (gametype == GT_RACE)
		{
#define timestring(time) va("%i'%02i\"%02i", G_TicsToMinutes(time, true), G_TicsToSeconds(time), G_TicsToCentiseconds(time))
			if (scorelines > 8)
			{
				if (players[tab[i].num].exiting)
					V_DrawRightAlignedThinString(x+rightoffset, y-1, hilicol|V_6WIDTHSPACE, timestring(players[tab[i].num].realtime));
				else if (players[tab[i].num].pflags & PF_GAMETYPEOVER)
					V_DrawRightAlignedThinString(x+rightoffset, y-1, V_6WIDTHSPACE, "NO CONTEST.");
				else if (circuitmap)
					V_DrawRightAlignedThinString(x+rightoffset, y-1, V_6WIDTHSPACE, va("Lap %d", tab[i].count));
			}
			else
			{
				if (players[tab[i].num].exiting)
					V_DrawRightAlignedString(x+rightoffset, y, hilicol, timestring(players[tab[i].num].realtime));
				else if (players[tab[i].num].pflags & PF_GAMETYPEOVER)
					V_DrawRightAlignedThinString(x+rightoffset, y-1, 0, "NO CONTEST.");
				else if (circuitmap)
					V_DrawRightAlignedString(x+rightoffset, y, 0, va("Lap %d", tab[i].count));
			}
#undef timestring
		}
		else
			V_DrawRightAlignedString(x+rightoffset, y, 0, va("%u", tab[i].count));

		y += 18;
		if (i == 7)
		{
			y = 33;
			x = (BASEVIDWIDTH/2) + 4;
		}
	}
}

#define RINGANIM_FLIPFRAME (RINGANIM_NUMFRAMES/2)

static void K_drawKartLapsAndRings(void)
{
	const boolean uselives = G_GametypeUsesLives();
	SINT8 ringanim_realframe = stplyr->karthud[khud_ringframe];
	INT32 splitflags = V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_SPLITSCREEN;
	UINT8 rn[2];
	INT32 ringflip = 0;
	UINT8 *ringmap = NULL;
	boolean colorring = false;
	INT32 ringx = 0;

	rn[0] = ((abs(stplyr->rings) / 10) % 10);
	rn[1] = (abs(stplyr->rings) % 10);

	if (stplyr->rings <= 0 && (leveltime/5 & 1)) // In debt
	{
		ringmap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_CRIMSON, GTC_CACHE);
		colorring = true;
	}
	else if (stplyr->rings >= 20) // Maxed out
		ringmap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_YELLOW, GTC_CACHE);

	if (stplyr->karthud[khud_ringframe] > RINGANIM_FLIPFRAME)
	{
		ringflip = V_FLIP;
		ringanim_realframe = RINGANIM_NUMFRAMES-stplyr->karthud[khud_ringframe];
		ringx += SHORT((r_splitscreen > 1) ? kp_smallring[ringanim_realframe]->width : kp_ring[ringanim_realframe]->width);
	}

	if (r_splitscreen > 1)
	{
		INT32 fx = 0, fy = 0, fr = 0;
		INT32 flipflag = 0;

		// pain and suffering defined below
		if (r_splitscreen < 2)	// don't change shit for THIS splitscreen.
		{
			fx = LAPS_X;
			fy = LAPS_Y;
		}
		else
		{
			if (stplyr == &players[displayplayers[0]] || stplyr == &players[displayplayers[2]])	// If we are P1 or P3...
			{
				fx = LAPS_X;
				fy = LAPS_Y;
				splitflags = V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_SPLITSCREEN;
			}
			else // else, that means we're P2 or P4.
			{
				fx = LAPS2_X;
				fy = LAPS2_Y;
				splitflags = V_SNAPTORIGHT|V_SNAPTOBOTTOM|V_SPLITSCREEN;
				flipflag = V_FLIP; // make the string right aligned and other shit
			}
		}

		fr = fx;

		// Laps
		V_DrawScaledPatch(fx-2 + (flipflag ? (SHORT(kp_ringstickersplit[1]->width) - 3) : 0), fy, V_HUDTRANS|V_SLIDEIN|splitflags|flipflag, kp_ringstickersplit[0]);

		V_DrawScaledPatch(fx, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_splitlapflag);
		V_DrawScaledPatch(fx+22, fy, V_HUDTRANS|V_SLIDEIN|splitflags, frameslash);

		if (cv_numlaps.value >= 10)
		{
			UINT8 ln[2];
			ln[0] = ((stplyr->laps / 10) % 10);
			ln[1] = (stplyr->laps % 10);

			V_DrawScaledPatch(fx+13, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[0]]);
			V_DrawScaledPatch(fx+17, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[1]]);

			ln[0] = ((abs(cv_numlaps.value) / 10) % 10);
			ln[1] = (abs(cv_numlaps.value) % 10);

			V_DrawScaledPatch(fx+27, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[0]]);
			V_DrawScaledPatch(fx+31, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[1]]);
		}
		else
		{
			V_DrawScaledPatch(fx+13, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[(stplyr->laps) % 10]);
			V_DrawScaledPatch(fx+27, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[(cv_numlaps.value) % 10]);
		}

		// Rings
		if (!uselives)
		{
			V_DrawScaledPatch(fx-2 + (flipflag ? (SHORT(kp_ringstickersplit[1]->width) - 3) : 0), fy-10, V_HUDTRANS|V_SLIDEIN|splitflags|flipflag, kp_ringstickersplit[1]);
			if (flipflag)
				fr += 15;
		}
		else
			V_DrawScaledPatch(fx-2 + (flipflag ? (SHORT(kp_ringstickersplit[0]->width) - 3) : 0), fy-10, V_HUDTRANS|V_SLIDEIN|splitflags|flipflag, kp_ringstickersplit[0]);

		V_DrawMappedPatch(fr+ringx, fy-13, V_HUDTRANS|V_SLIDEIN|splitflags|ringflip, kp_smallring[ringanim_realframe], (colorring ? ringmap : NULL));

		if (stplyr->rings < 0) // Draw the minus for ring debt
			V_DrawMappedPatch(fr+7, fy-10, V_HUDTRANS|V_SLIDEIN|splitflags, kp_ringdebtminussmall, ringmap);

		V_DrawMappedPatch(fr+11, fy-10, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[rn[0]], ringmap);
		V_DrawMappedPatch(fr+15, fy-10, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[rn[1]], ringmap);

		// SPB ring lock
		if (stplyr->kartstuff[k_ringlock])
			V_DrawScaledPatch(fr-12, fy-23, V_HUDTRANS|V_SLIDEIN|splitflags, kp_ringspblocksmall[stplyr->karthud[khud_ringspblock]]);

		// Lives
		if (uselives)
		{
			UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, stplyr->skincolor, GTC_CACHE);
			V_DrawMappedPatch(fr+21, fy-13, V_HUDTRANS|V_SLIDEIN|splitflags, faceprefix[stplyr->skin][FACE_MINIMAP], colormap);
			V_DrawScaledPatch(fr+34, fy-10, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[(stplyr->lives % 10)]); // make sure this doesn't overflow
		}
	}
	else
	{
		// Laps
		V_DrawScaledPatch(LAPS_X, LAPS_Y, V_HUDTRANS|V_SLIDEIN|splitflags, kp_lapsticker);

		if (stplyr->exiting)
			V_DrawKartString(LAPS_X+33, LAPS_Y+3, V_HUDTRANS|V_SLIDEIN|splitflags, "FIN");
		else
			V_DrawKartString(LAPS_X+33, LAPS_Y+3, V_HUDTRANS|V_SLIDEIN|splitflags, va("%d/%d", stplyr->laps, cv_numlaps.value));

		// Rings
		if (!uselives)
			V_DrawScaledPatch(LAPS_X, LAPS_Y-11, V_HUDTRANS|V_SLIDEIN|splitflags, kp_ringsticker[1]);
		else
			V_DrawScaledPatch(LAPS_X, LAPS_Y-11, V_HUDTRANS|V_SLIDEIN|splitflags, kp_ringsticker[0]);

		V_DrawMappedPatch(LAPS_X+ringx+7, LAPS_Y-16, V_HUDTRANS|V_SLIDEIN|splitflags|ringflip, kp_ring[ringanim_realframe], (colorring ? ringmap : NULL));

		if (stplyr->rings < 0) // Draw the minus for ring debt
		{
			V_DrawMappedPatch(LAPS_X+23, LAPS_Y-11, V_HUDTRANS|V_SLIDEIN|splitflags, kp_ringdebtminus, ringmap);
			V_DrawMappedPatch(LAPS_X+29, LAPS_Y-11, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[rn[0]], ringmap);
			V_DrawMappedPatch(LAPS_X+35, LAPS_Y-11, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[rn[1]], ringmap);
		}
		else
		{
			V_DrawMappedPatch(LAPS_X+23, LAPS_Y-11, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[rn[0]], ringmap);
			V_DrawMappedPatch(LAPS_X+29, LAPS_Y-11, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[rn[1]], ringmap);
		}

		// SPB ring lock
		if (stplyr->kartstuff[k_ringlock])
			V_DrawScaledPatch(LAPS_X-5, LAPS_Y-28, V_HUDTRANS|V_SLIDEIN|splitflags, kp_ringspblock[stplyr->karthud[khud_ringspblock]]);

		// Lives
		if (uselives)
		{
			UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, stplyr->skincolor, GTC_CACHE);
			V_DrawMappedPatch(LAPS_X+46, LAPS_Y-16, V_HUDTRANS|V_SLIDEIN|splitflags, faceprefix[stplyr->skin][FACE_RANK], colormap);
			V_DrawScaledPatch(LAPS_X+63, LAPS_Y-11, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[(stplyr->lives % 10)]); // make sure this doesn't overflow
		}
	}
}

#undef RINGANIM_FLIPFRAME

static void K_drawKartSpeedometer(void)
{
	static fixed_t convSpeed;
	UINT8 labeln = 0;
	UINT8 numbers[3];
	INT32 splitflags = V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_SPLITSCREEN;
	INT32 battleoffset = 0;

	if (!stplyr->exiting) // Keep the same speed value as when you crossed the finish line!
	{
		switch (cv_kartspeedometer.value)
		{
			case 1: // Sonic Drift 2 style percentage
			default:
				convSpeed = (((25*stplyr->speed)/24) * 100) / K_GetKartSpeed(stplyr, false); // Based on top speed! (cheats with the numbers due to some weird discrepancy)
				labeln = 0;
				break;
			case 2: // Kilometers
				convSpeed = FixedDiv(FixedMul(stplyr->speed, 142371), mapobjectscale)/FRACUNIT; // 2.172409058
				labeln = 1;
				break;
			case 3: // Miles
				convSpeed = FixedDiv(FixedMul(stplyr->speed, 88465), mapobjectscale)/FRACUNIT; // 1.349868774
				labeln = 2;
				break;
			case 4: // Fracunits
				convSpeed = FixedDiv(stplyr->speed, mapobjectscale)/FRACUNIT; // 1.0. duh.
				labeln = 3;
				break;
		}
	}

	// Don't overflow
	if (convSpeed > 999)
		convSpeed = 999;

	numbers[0] = ((convSpeed / 100) % 10);
	numbers[1] = ((convSpeed / 10) % 10);
	numbers[2] = (convSpeed % 10);

	if (gametype == GT_BATTLE)
		battleoffset = -4;

	V_DrawScaledPatch(LAPS_X, LAPS_Y-25 + battleoffset, V_HUDTRANS|V_SLIDEIN|splitflags, kp_speedometersticker);
	V_DrawScaledPatch(LAPS_X+7, LAPS_Y-25 + battleoffset, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[numbers[0]]);
	V_DrawScaledPatch(LAPS_X+13, LAPS_Y-25 + battleoffset, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[numbers[1]]);
	V_DrawScaledPatch(LAPS_X+19, LAPS_Y-25 + battleoffset, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[numbers[2]]);
	V_DrawScaledPatch(LAPS_X+29, LAPS_Y-25 + battleoffset, V_HUDTRANS|V_SLIDEIN|splitflags, kp_speedometerlabel[labeln]);
}

static void K_drawBlueSphereMeter(void)
{
	const UINT8 maxBars = 4;
	const UINT8 segColors[] = {73, 64, 52, 54, 55, 35, 34, 33, 202, 180, 181, 182, 164, 165, 166, 153, 152};
	const UINT8 sphere = max(min(stplyr->spheres, 40), 0);

	UINT8 numBars = min((sphere / 10), maxBars);
	UINT8 color = segColors[(sphere * sizeof(segColors)) / (40 + 1)];
	INT32 x = LAPS_X + 25;
	UINT8 i;

	V_DrawScaledPatch(LAPS_X, LAPS_Y - 22, V_HUDTRANS|V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_SPLITSCREEN, kp_spheresticker);

	for (i = 0; i <= numBars; i++)
	{
		UINT8 segLen = 10;

		if (i == numBars)
		{
			segLen = (sphere % 10);
		}

		V_DrawFill(x, LAPS_Y - 16, segLen, 6, color | V_HUDTRANS|V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_SPLITSCREEN);

		x += 15;
	}
}

static void K_drawKartBumpersOrKarma(void)
{
	UINT8 *colormap = R_GetTranslationColormap(TC_DEFAULT, stplyr->skincolor, GTC_CACHE);
	INT32 splitflags = V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_SPLITSCREEN;

	if (r_splitscreen > 1)
	{
		INT32 fx = 0, fy = 0;
		INT32 flipflag = 0;

		// pain and suffering defined below
		if (r_splitscreen < 2)	// don't change shit for THIS splitscreen.
		{
			fx = LAPS_X;
			fy = LAPS_Y;
		}
		else
		{
			if (stplyr == &players[displayplayers[0]] || stplyr == &players[displayplayers[2]])	// If we are P1 or P3...
			{
				fx = LAPS_X;
				fy = LAPS_Y;
				splitflags = V_SNAPTOLEFT|((stplyr == &players[displayplayers[2]]) ? V_SPLITSCREEN|V_SNAPTOBOTTOM : 0); // flip P3 to the bottom.
			}
			else // else, that means we're P2 or P4.
			{
				fx = LAPS2_X;
				fy = LAPS2_Y;
				splitflags = V_SNAPTORIGHT|((stplyr == &players[displayplayers[3]]) ? V_SPLITSCREEN|V_SNAPTOBOTTOM : 0); // flip P4 to the bottom
				flipflag = V_FLIP; // make the string right aligned and other shit
			}
		}

		V_DrawScaledPatch(fx-2 + (flipflag ? (SHORT(kp_ringstickersplit[1]->width) - 3) : 0), fy, V_HUDTRANS|V_SLIDEIN|splitflags|flipflag, kp_ringstickersplit[0]);
		V_DrawScaledPatch(fx+22, fy, V_HUDTRANS|V_SLIDEIN|splitflags, frameslash);

		if (battlecapsules)
		{
			V_DrawMappedPatch(fx+1, fy-2, V_HUDTRANS|V_SLIDEIN|splitflags, kp_rankcapsule, NULL);

			if (numtargets > 9 || maptargets > 9)
			{
				UINT8 ln[2];
				ln[0] = ((numtargets / 10) % 10);
				ln[1] = (numtargets % 10);

				V_DrawScaledPatch(fx+13, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[0]]);
				V_DrawScaledPatch(fx+17, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[1]]);

				ln[0] = ((maptargets / 10) % 10);
				ln[1] = (maptargets % 10);

				V_DrawScaledPatch(fx+27, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[0]]);
				V_DrawScaledPatch(fx+31, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[1]]);
			}
			else
			{
				V_DrawScaledPatch(fx+13, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[numtargets % 10]);
				V_DrawScaledPatch(fx+27, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[maptargets % 10]);
			}
		}
		else
		{
			if (stplyr->kartstuff[k_bumper] <= 0)
			{
				V_DrawMappedPatch(fx+1, fy-2, V_HUDTRANS|V_SLIDEIN|splitflags, kp_splitkarmabomb, colormap);
				V_DrawScaledPatch(fx+13, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[(stplyr->kartstuff[k_comebackpoints]) % 10]);
				V_DrawScaledPatch(fx+27, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[2]);
			}
			else
			{
				INT32 maxbumper = K_StartingBumperCount();
				V_DrawMappedPatch(fx+1, fy-2, V_HUDTRANS|V_SLIDEIN|splitflags, kp_rankbumper, colormap);

				if (stplyr->kartstuff[k_bumper] > 9 || maxbumper > 9)
				{
					UINT8 ln[2];
					ln[0] = ((abs(stplyr->kartstuff[k_bumper]) / 10) % 10);
					ln[1] = (abs(stplyr->kartstuff[k_bumper]) % 10);

					V_DrawScaledPatch(fx+13, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[0]]);
					V_DrawScaledPatch(fx+17, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[1]]);

					ln[0] = ((abs(maxbumper) / 10) % 10);
					ln[1] = (abs(maxbumper) % 10);

					V_DrawScaledPatch(fx+27, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[0]]);
					V_DrawScaledPatch(fx+31, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[1]]);
				}
				else
				{
					V_DrawScaledPatch(fx+13, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[(stplyr->kartstuff[k_bumper]) % 10]);
					V_DrawScaledPatch(fx+27, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[(maxbumper) % 10]);
				}
			}
		}
	}
	else
	{
		if (battlecapsules)
		{
			if (numtargets > 9 && maptargets > 9)
				V_DrawMappedPatch(LAPS_X, LAPS_Y, V_HUDTRANS|V_SLIDEIN|splitflags, kp_capsulestickerwide, NULL);
			else
				V_DrawMappedPatch(LAPS_X, LAPS_Y, V_HUDTRANS|V_SLIDEIN|splitflags, kp_capsulesticker, NULL);
			V_DrawKartString(LAPS_X+47, LAPS_Y+3, V_HUDTRANS|V_SLIDEIN|splitflags, va("%d/%d", numtargets, maptargets));
		}
		else
		{
			if (stplyr->kartstuff[k_bumper] <= 0)
			{
				V_DrawMappedPatch(LAPS_X, LAPS_Y, V_HUDTRANS|V_SLIDEIN|splitflags, kp_karmasticker, colormap);
				V_DrawKartString(LAPS_X+47, LAPS_Y+3, V_HUDTRANS|V_SLIDEIN|splitflags, va("%d/2", stplyr->kartstuff[k_comebackpoints]));
			}
			else
			{
				INT32 maxbumper = K_StartingBumperCount();

				if (stplyr->kartstuff[k_bumper] > 9 && maxbumper > 9)
					V_DrawMappedPatch(LAPS_X, LAPS_Y, V_HUDTRANS|V_SLIDEIN|splitflags, kp_bumperstickerwide, colormap);
				else
					V_DrawMappedPatch(LAPS_X, LAPS_Y, V_HUDTRANS|V_SLIDEIN|splitflags, kp_bumpersticker, colormap);

				V_DrawKartString(LAPS_X+47, LAPS_Y+3, V_HUDTRANS|V_SLIDEIN|splitflags, va("%d/%d", stplyr->kartstuff[k_bumper], maxbumper));
			}
		}
	}
}

static void K_drawKartWanted(void)
{
	UINT8 i, numwanted = 0;
	UINT8 *colormap = NULL;
	INT32 basex = 0, basey = 0;

	if (stplyr != &players[displayplayers[0]])
		return;

	for (i = 0; i < 4; i++)
	{
		if (battlewanted[i] == -1)
			break;
		numwanted++;
	}

	if (numwanted <= 0)
		return;

	// set X/Y coords depending on splitscreen.
	if (r_splitscreen < 3) // 1P and 2P use the same code.
	{
		basex = WANT_X;
		basey = WANT_Y;
		if (r_splitscreen == 2)
		{
			basey += 16; // slight adjust for 3P
			basex -= 6;
		}
	}
	else if (r_splitscreen == 3) // 4P splitscreen...
	{
		basex = BASEVIDWIDTH/2 - (SHORT(kp_wantedsplit->width)/2);	// center on screen
		basey = BASEVIDHEIGHT - 55;
		//basey2 = 4;
	}

	if (battlewanted[0] != -1)
		colormap = R_GetTranslationColormap(0, players[battlewanted[0]].skincolor, GTC_CACHE);
	V_DrawFixedPatch(basex<<FRACBITS, basey<<FRACBITS, FRACUNIT, V_HUDTRANS|V_SLIDEIN|(r_splitscreen < 3 ? V_SNAPTORIGHT : 0)|V_SNAPTOBOTTOM, (r_splitscreen > 1 ? kp_wantedsplit : kp_wanted), colormap);
	/*if (basey2)
		V_DrawFixedPatch(basex<<FRACBITS, basey2<<FRACBITS, FRACUNIT, V_HUDTRANS|V_SLIDEIN|V_SNAPTOTOP, (splitscreen == 3 ? kp_wantedsplit : kp_wanted), colormap);	// < used for 4p splits.*/

	for (i = 0; i < numwanted; i++)
	{
		INT32 x = basex+(r_splitscreen > 1 ? 13 : 8), y = basey+(r_splitscreen > 1 ? 16 : 21);
		fixed_t scale = FRACUNIT/2;
		player_t *p = &players[battlewanted[i]];

		if (battlewanted[i] == -1)
			break;

		if (numwanted == 1)
			scale = FRACUNIT;
		else
		{
			if (i & 1)
				x += 16;
			if (i > 1)
				y += 16;
		}

		if (players[battlewanted[i]].skincolor)
		{
			colormap = R_GetTranslationColormap(TC_RAINBOW, p->skincolor, GTC_CACHE);
			V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT, V_HUDTRANS|V_SLIDEIN|(r_splitscreen < 3 ? V_SNAPTORIGHT : 0)|V_SNAPTOBOTTOM, (scale == FRACUNIT ? faceprefix[p->skin][FACE_WANTED] : faceprefix[p->skin][FACE_RANK]), colormap);
			/*if (basey2)	// again with 4p stuff
				V_DrawFixedPatch(x<<FRACBITS, (y - (basey-basey2))<<FRACBITS, FRACUNIT, V_HUDTRANS|V_SLIDEIN|V_SNAPTOTOP, (scale == FRACUNIT ? faceprefix[p->skin][FACE_WANTED] : faceprefix[p->skin][FACE_RANK]), colormap);*/
		}
	}
}

static void K_ObjectTracking(fixed_t *hud_x, fixed_t *hud_y, vector3_t *campos, angle_t camang, angle_t camaim, vector3_t *point, UINT8 cnum)
{
	const INT32 swhalf = (BASEVIDWIDTH / 2);
	const fixed_t swhalffixed = swhalf * FRACUNIT;

	const INT32 shhalf = (BASEVIDHEIGHT / 2);
	const fixed_t shhalffixed = shhalf * FRACUNIT;


	const UINT8 precisionloss = 4;

	fixed_t anglediff = (AngleFixed(camang) / precisionloss) - (AngleFixed(R_PointToAngle2(campos->x, campos->y, point->x, point->y)) / precisionloss);

	fixed_t distance = R_PointToDist2(campos->x, campos->y, point->x, point->y);
	fixed_t factor = INT32_MAX;

	const fixed_t fov = cv_fov[cnum].value;
	fixed_t intendedfov = 90*FRACUNIT;
	fixed_t fovmul = FRACUNIT;

	if (r_splitscreen == 1) // Splitscreen FOV should be adjusted to maintain expected vertical view
	{
		intendedfov = 17 * intendedfov / 10;
	}

	fovmul = FixedDiv(fov, intendedfov);

	anglediff = FixedMul(anglediff, fovmul) * precisionloss;

	if (abs(anglediff) > 90*FRACUNIT)
	{
		if (hud_x != NULL)
		{
			*hud_x = -1000 * FRACUNIT;
		}

		if (hud_y != NULL)
		{
			*hud_y = -1000 * FRACUNIT;
		}

		//*hud_scale = 0;
		return;
	}

	anglediff = FixedAngle(anglediff);

	factor = max(1, FINECOSINE(anglediff >> ANGLETOFINESHIFT));

#define NEWTAN(n) FINETANGENT(((n + ANGLE_90) >> ANGLETOFINESHIFT) & 4095)

	if (hud_x != NULL)
	{
		*hud_x = FixedMul(NEWTAN(anglediff), swhalffixed) + swhalffixed;

		if (*hud_x < 0 || *hud_x > BASEVIDWIDTH * FRACUNIT)
		{
			*hud_x = -1000 * FRACUNIT;
		}
		else
		{
			if (encoremode)
			{
				*hud_x = (BASEVIDWIDTH * FRACUNIT) - *hud_x;
			}

			if (r_splitscreen >= 2)
			{
				*hud_x /= 2;
			}
		}
	}

	if (hud_y != NULL)
	{
		*hud_y = campos->z - point->z;
		*hud_y = FixedDiv(*hud_y, FixedMul(factor, distance));
		*hud_y = (*hud_y * swhalf) + shhalffixed;
		*hud_y = *hud_y + NEWTAN(camaim) * swhalf;

		if (*hud_y < 0 || *hud_y > BASEVIDHEIGHT * FRACUNIT)
		{
			*hud_y = -1000 * FRACUNIT;
		}
		else
		{
			if (r_splitscreen >= 1)
			{
				*hud_y /= 2;
			}
		}
	}

	//*hud_scale = FixedDiv(swhalffixed, FixedMul(factor, distance));

#undef NEWTAN
}

static void K_drawKartPlayerCheck(void)
{
	const fixed_t maxdistance = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	camera_t *thiscam;
	vector3_t c;
	UINT8 cnum = 0;
	UINT8 i;
	INT32 splitflags = V_SNAPTOBOTTOM|V_SPLITSCREEN;

	if (stplyr == NULL || stplyr->mo == NULL || P_MobjWasRemoved(stplyr->mo))
	{
		return;
	}

	if (stplyr->spectator || stplyr->awayviewtics)
	{
		return;
	}

	if (stplyr->cmd.buttons & BT_LOOKBACK)
	{
		return;
	}

	if (r_splitscreen)
	{
		for (i = 1; i <= r_splitscreen; i++)
		{
			if (stplyr == &players[displayplayers[i]])
			{
				cnum = i;
				break;
			}
		}
	}

	thiscam = &camera[cnum];

	c.x = stplyr->mo->x;
	c.y = stplyr->mo->y;
	c.z = stplyr->mo->z;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *checkplayer = &players[i];
		fixed_t distance = maxdistance+1;
		UINT8 *colormap = NULL;
		UINT8 pnum = 0;
		fixed_t x = 0;
		vector3_t v;

		if (!playeringame[i] || checkplayer->spectator)
		{
			// Not in-game
			continue;
		}

		if (checkplayer->mo == NULL || P_MobjWasRemoved(checkplayer->mo))
		{
			// No object
			continue;
		}

		if (checkplayer == stplyr)
		{
			// This is you!
			continue;
		}

		v.x = checkplayer->mo->x;
		v.y = checkplayer->mo->y;
		v.z = checkplayer->mo->z;

		distance = R_PointToDist2(c.x, c.y, v.x, v.y);

		if (distance > maxdistance)
		{
			// Too far away
			continue;
		}

		if ((checkplayer->kartstuff[k_invincibilitytimer] <= 0) && (leveltime & 2))
		{
			pnum++; // white frames
		}

		if (checkplayer->kartstuff[k_itemtype] == KITEM_GROW || checkplayer->kartstuff[k_growshrinktimer] > 0)
		{
			pnum += 4;
		}
		else if (checkplayer->kartstuff[k_itemtype] == KITEM_INVINCIBILITY || checkplayer->kartstuff[k_invincibilitytimer])
		{
			pnum += 2;
		}

		K_ObjectTracking(&x, NULL, &c, thiscam->angle + ANGLE_180, 0, &v, cnum);

		colormap = R_GetTranslationColormap(TC_DEFAULT, checkplayer->mo->color, GTC_CACHE);
		V_DrawFixedPatch(x, CHEK_Y * FRACUNIT, FRACUNIT, V_HUDTRANS|V_SPLITSCREEN|splitflags, kp_check[pnum], colormap);
	}
}

static boolean K_ShowPlayerNametag(player_t *p)
{
	if (demo.playback == true && demo.freecam == true)
	{
		return true;
	}

	if (stplyr == p)
	{
		return false;
	}

	if (gametype == GT_RACE)
	{
		if ((p->kartstuff[k_position] < stplyr->kartstuff[k_position]-2)
		|| (p->kartstuff[k_position] > stplyr->kartstuff[k_position]+2))
		{
			return false;
		}
	}

	return true;
}

static void K_DrawLocalTagForPlayer(fixed_t x, fixed_t y, player_t *p, UINT8 id)
{
	UINT8 blink = ((leveltime / 7) & 1);
	UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, p->skincolor, GTC_CACHE);
	V_DrawFixedPatch(x, y, FRACUNIT, V_HUDTRANS|V_SPLITSCREEN, kp_localtag[id][blink], colormap);
}

static void K_DrawRivalTagForPlayer(fixed_t x, fixed_t y)
{
	UINT8 blink = ((leveltime / 7) & 1);
	V_DrawFixedPatch(x, y, FRACUNIT, V_HUDTRANS|V_SPLITSCREEN, kp_rival[blink], NULL);
}

static void K_DrawNameTagForPlayer(fixed_t x, fixed_t y, player_t *p, UINT8 cnum)
{
	const INT32 clr = skincolors[p->skincolor].chatcolor;
	const INT32 namelen = V_ThinStringWidth(player_names[p - players], V_6WIDTHSPACE|V_ALLOWLOWERCASE);

	UINT8 *colormap = V_GetStringColormap(clr);
	INT32 barx = 0, bary = 0, barw = 0;

	// Since there's no "V_DrawFixedFill", and I don't feel like making it,
	// fuck it, we're gonna just V_NOSCALESTART hack it
	if (cnum & 1)
	{
		x += (BASEVIDWIDTH/2) * FRACUNIT;
	}

	if ((r_splitscreen == 1 && cnum == 1)
	|| (r_splitscreen > 1 && cnum > 1))
	{
		y += (BASEVIDHEIGHT/2) * FRACUNIT;
	}

	barw = (namelen * vid.dupx);

	barx = (x * vid.dupx) / FRACUNIT;
	bary = (y * vid.dupy) / FRACUNIT;

	barx += (6 * vid.dupx);
	bary -= (16 * vid.dupx);

	// Center it if necessary
	if (vid.width != BASEVIDWIDTH * vid.dupx)
	{
		barx += (vid.width - (BASEVIDWIDTH * vid.dupx)) / 2;
	}

	if (vid.height != BASEVIDHEIGHT * vid.dupy)
	{
		bary += (vid.height - (BASEVIDHEIGHT * vid.dupy)) / 2;
	}

	// Lat: 10/06/2020: colormap can be NULL on the frame you join a game, just arbitrarily use palette indexes 31 and 0 instead of whatever the colormap would give us instead to avoid crashes.
	V_DrawFill(barx, bary, barw, (3 * vid.dupy), (colormap ? colormap[31] : 31)|V_NOSCALESTART);
	V_DrawFill(barx, bary + vid.dupy, barw, vid.dupy, (colormap ? colormap[0] : 0)|V_NOSCALESTART);
	// END DRAWFILL DUMBNESS

	// Draw the stem
	V_DrawFixedPatch(x, y, FRACUNIT, 0, kp_nametagstem, colormap);

	// Draw the name itself
	V_DrawThinStringAtFixed(x + (5*FRACUNIT), y - (26*FRACUNIT), V_6WIDTHSPACE|V_ALLOWLOWERCASE|clr, player_names[p - players]);
}

static void K_drawKartNameTags(void)
{
	const fixed_t maxdistance = 8192*mapobjectscale;
	camera_t *thiscam;
	vector3_t c;
	UINT8 cnum = 0;
	UINT8 tobesorted[MAXPLAYERS];
	fixed_t sortdist[MAXPLAYERS];
	UINT8 sortlen = 0;
	UINT8 i, j;

	if (stplyr == NULL || stplyr->mo == NULL || P_MobjWasRemoved(stplyr->mo))
	{
		return;
	}

	if (stplyr->awayviewtics)
	{
		return;
	}

	if (r_splitscreen)
	{
		for (i = 1; i <= r_splitscreen; i++)
		{
			if (stplyr == &players[displayplayers[i]])
			{
				cnum = i;
				break;
			}
		}
	}

	thiscam = &camera[cnum];

	c.x = thiscam->x;
	c.y = thiscam->y;
	c.z = thiscam->z;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *ntplayer = &players[i];
		fixed_t distance = maxdistance+1;
		vector3_t v;

		if (!playeringame[i] || ntplayer->spectator)
		{
			// Not in-game
			continue;
		}

		if (ntplayer->mo == NULL || P_MobjWasRemoved(ntplayer->mo))
		{
			// No object
			continue;
		}

		if (!P_CheckSight(stplyr->mo, ntplayer->mo))
		{
			// Can't see
			continue;
		}

		v.x = ntplayer->mo->x;
		v.y = ntplayer->mo->y;
		v.z = ntplayer->mo->z;

		if (!(ntplayer->mo->eflags & MFE_VERTICALFLIP))
		{
			v.z += ntplayer->mo->height;
		}

		distance = R_PointToDist2(c.x, c.y, v.x, v.y);

		if (distance > maxdistance)
		{
			// Too far away
			continue;
		}

		tobesorted[sortlen] = ntplayer - players;
		sortdist[sortlen] = distance;
		sortlen++;
	}

	if (sortlen > 0)
	{
		UINT8 sortedplayers[sortlen];

		for (i = 0; i < sortlen; i++)
		{
			UINT8 pos = 0;

			for (j = 0; j < sortlen; j++)
			{
				if (j == i)
				{
					continue;
				}

				if (sortdist[i] < sortdist[j]
				|| (sortdist[i] == sortdist[j] && i > j))
				{
					pos++;
				}
			}

			sortedplayers[pos] = tobesorted[i];
		}

		for (i = 0; i < sortlen; i++)
		{
			player_t *ntplayer = &players[sortedplayers[i]];

			fixed_t x = -BASEVIDWIDTH * FRACUNIT;
			fixed_t y = -BASEVIDWIDTH * FRACUNIT;

			SINT8 localindicator = -1;
			vector3_t v;

			v.x = ntplayer->mo->x;
			v.y = ntplayer->mo->y;
			v.z = ntplayer->mo->z;

			if (!(ntplayer->mo->eflags & MFE_VERTICALFLIP))
			{
				v.z += ntplayer->mo->height;
			}

			K_ObjectTracking(&x, &y, &c, thiscam->angle, thiscam->aiming, &v, cnum);

			/*
			if ((x < 0 || x > BASEVIDWIDTH * FRACUNIT)
			|| (y < 0 || y > BASEVIDHEIGHT * FRACUNIT))
			{
				// Off-screen
				continue;
			}
			*/

			if (!(demo.playback == true && demo.freecam == true))
			{
				for (j = 0; j <= r_splitscreen; j++)
				{
					if (ntplayer == &players[displayplayers[j]])
					{
						break;
					}
				}

				if (j <= r_splitscreen && j != cnum)
				{
					localindicator = j;
				}
			}

			if (localindicator >= 0)
			{
				K_DrawLocalTagForPlayer(x, y, ntplayer, localindicator);
			}
			else if (ntplayer->bot)
			{
				if (ntplayer->botvars.rival == true)
				{
					K_DrawRivalTagForPlayer(x, y);
				}
			}
			else if (netgame || demo.playback)
			{
				if (K_ShowPlayerNametag(ntplayer) == true)
				{
					K_DrawNameTagForPlayer(x, y, ntplayer, cnum);
				}
			}
		}
	}
}

static void K_drawKartMinimapIcon(fixed_t objx, fixed_t objy, INT32 hudx, INT32 hudy, INT32 flags, patch_t *icon, UINT8 *colormap, patch_t *AutomapPic)
{
	// amnum xpos & ypos are the icon's speed around the HUD.
	// The number being divided by is for how fast it moves.
	// The higher the number, the slower it moves.

	// am xpos & ypos are the icon's starting position. Withouht
	// it, they wouldn't 'spawn' on the top-right side of the HUD.

	fixed_t amnumxpos, amnumypos;
	INT32 amxpos, amypos;

	node_t *bsp = &nodes[numnodes-1];
	fixed_t maxx, minx, maxy, miny;

	fixed_t mapwidth, mapheight;
	fixed_t xoffset, yoffset;
	fixed_t xscale, yscale, zoom;

	maxx = maxy = INT32_MAX;
	minx = miny = INT32_MIN;
	minx = bsp->bbox[0][BOXLEFT];
	maxx = bsp->bbox[0][BOXRIGHT];
	miny = bsp->bbox[0][BOXBOTTOM];
	maxy = bsp->bbox[0][BOXTOP];

	if (bsp->bbox[1][BOXLEFT] < minx)
		minx = bsp->bbox[1][BOXLEFT];
	if (bsp->bbox[1][BOXRIGHT] > maxx)
		maxx = bsp->bbox[1][BOXRIGHT];
	if (bsp->bbox[1][BOXBOTTOM] < miny)
		miny = bsp->bbox[1][BOXBOTTOM];
	if (bsp->bbox[1][BOXTOP] > maxy)
		maxy = bsp->bbox[1][BOXTOP];

	// You might be wondering why these are being bitshift here
	// it's because mapwidth and height would otherwise overflow for maps larger than half the size possible...
	// map boundaries and sizes will ALWAYS be whole numbers thankfully
	// later calculations take into consideration that these are actually not in terms of FRACUNIT though
	minx >>= FRACBITS;
	maxx >>= FRACBITS;
	miny >>= FRACBITS;
	maxy >>= FRACBITS;

	mapwidth = maxx - minx;
	mapheight = maxy - miny;

	// These should always be small enough to be bitshift back right now
	xoffset = (minx + mapwidth/2)<<FRACBITS;
	yoffset = (miny + mapheight/2)<<FRACBITS;

	xscale = FixedDiv(AutomapPic->width, mapwidth);
	yscale = FixedDiv(AutomapPic->height, mapheight);
	zoom = FixedMul(min(xscale, yscale), FRACUNIT-FRACUNIT/20);

	amnumxpos = (FixedMul(objx, zoom) - FixedMul(xoffset, zoom));
	amnumypos = -(FixedMul(objy, zoom) - FixedMul(yoffset, zoom));

	if (encoremode)
		amnumxpos = -amnumxpos;

	amxpos = amnumxpos + ((hudx + AutomapPic->width/2 - (icon->width/2))<<FRACBITS);
	amypos = amnumypos + ((hudy + AutomapPic->height/2 - (icon->height/2))<<FRACBITS);

	// do we want this? it feels unnecessary. easier to just modify the amnumxpos?
	/*if (encoremode)
	{
		flags |= V_FLIP;
		amxpos = -amnumxpos + ((hudx + AutomapPic->width/2 + (icon->width/2))<<FRACBITS);
	}*/

	V_DrawFixedPatch(amxpos, amypos, FRACUNIT, flags, icon, colormap);
}

static void K_drawKartMinimap(void)
{
	INT32 lumpnum;
	patch_t *AutomapPic;
	INT32 i = 0;
	INT32 x, y;
	INT32 minimaptrans, splitflags = (r_splitscreen == 3 ? 0 : (V_SLIDEIN|V_SNAPTORIGHT)); // flags should only be 0 when it's centered (4p split)
	UINT8 skin = 0;
	UINT8 *colormap = NULL;
	SINT8 localplayers[4];
	SINT8 numlocalplayers = 0;
	INT32 hyu = hyudorotime;
	mobj_t *mobj, *next;	// for SPB drawing (or any other item(s) we may wanna draw, I dunno!)

	// Draw the HUD only when playing in a level.
	// hu_stuff needs this, unlike st_stuff.
	if (gamestate != GS_LEVEL)
		return;

	// Only draw for the first player
	// Maybe move this somewhere else where this won't be a concern?
	if (stplyr != &players[displayplayers[0]])
		return;

	lumpnum = W_CheckNumForName(va("%sR", G_BuildMapName(gamemap)));

	if (lumpnum != -1)
		AutomapPic = W_CachePatchName(va("%sR", G_BuildMapName(gamemap)), PU_HUDGFX);
	else
		return; // no pic, just get outta here

	x = MINI_X - (AutomapPic->width/2);
	y = MINI_Y - (AutomapPic->height/2);

	if (timeinmap > 105)
	{
		minimaptrans = cv_kartminimap.value;
		if (timeinmap <= 113)
			minimaptrans = ((((INT32)timeinmap) - 105)*minimaptrans)/(113-105);
		if (!minimaptrans)
			return;
	}
	else
		return;

	minimaptrans = ((10-minimaptrans)<<FF_TRANSSHIFT);
	splitflags |= minimaptrans;

	if (encoremode)
		V_DrawScaledPatch(x+(AutomapPic->width), y, splitflags|V_FLIP, AutomapPic);
	else
		V_DrawScaledPatch(x, y, splitflags, AutomapPic);

	if (r_splitscreen != 2)
	{
		splitflags &= ~minimaptrans;
		splitflags |= V_HUDTRANSHALF;
	}

	// let offsets transfer to the heads, too!
	if (encoremode)
		x += SHORT(AutomapPic->leftoffset);
	else
		x -= SHORT(AutomapPic->leftoffset);
	y -= SHORT(AutomapPic->topoffset);

	// Draw the super item in Battle
	if (gametype == GT_BATTLE && battleovertime.enabled)
	{
		if (battleovertime.enabled >= 10*TICRATE || (battleovertime.enabled & 1))
		{
			const INT32 prevsplitflags = splitflags;
			splitflags &= ~V_HUDTRANSHALF;
			splitflags |= V_HUDTRANS;
			colormap = R_GetTranslationColormap(TC_RAINBOW, K_RainbowColor(leveltime), GTC_CACHE);
			K_drawKartMinimapIcon(battleovertime.x, battleovertime.y, x, y, splitflags, kp_itemminimap, colormap, AutomapPic);
			splitflags = prevsplitflags;
		}
	}

	// initialize
	for (i = 0; i < 4; i++)
		localplayers[i] = -1;

	if (gametype == GT_RACE)
		hyu *= 2; // double in race

	// Player's tiny icons on the Automap. (drawn opposite direction so player 1 is drawn last in splitscreen)
	if (ghosts)
	{
		demoghost *g = ghosts;
		while (g)
		{
			if (g->mo->skin)
				skin = ((skin_t*)g->mo->skin)-skins;
			else
				skin = 0;
			if (g->mo->color)
			{
				if (g->mo->colorized)
					colormap = R_GetTranslationColormap(TC_RAINBOW, g->mo->color, GTC_CACHE);
				else
					colormap = R_GetTranslationColormap(skin, g->mo->color, GTC_CACHE);
			}
			else
				colormap = NULL;
			K_drawKartMinimapIcon(g->mo->x, g->mo->y, x, y, splitflags, faceprefix[skin][FACE_MINIMAP], colormap, AutomapPic);
			g = g->next;
		}

		if (!stplyr->mo || stplyr->spectator || stplyr->exiting)
			return;

		localplayers[numlocalplayers] = stplyr-players;
		numlocalplayers++;
	}
	else
	{
		for (i = MAXPLAYERS-1; i >= 0; i--)
		{
			if (!playeringame[i])
				continue;
			if (!players[i].mo || players[i].spectator || players[i].exiting)
				continue;

			if (i != displayplayers[0] || r_splitscreen)
			{
				if (gametype == GT_BATTLE && players[i].kartstuff[k_bumper] <= 0)
					continue;

				if (players[i].kartstuff[k_hyudorotimer] > 0)
				{
					if (!((players[i].kartstuff[k_hyudorotimer] < TICRATE/2
						|| players[i].kartstuff[k_hyudorotimer] > hyu-(TICRATE/2))
						&& !(leveltime & 1)))
						continue;
				}
			}

			if (i == displayplayers[0] || i == displayplayers[1] || i == displayplayers[2] || i == displayplayers[3])
			{
				// Draw display players on top of everything else
				localplayers[numlocalplayers] = i;
				numlocalplayers++;
				continue;
			}

			if (players[i].mo->skin)
				skin = ((skin_t*)players[i].mo->skin)-skins;
			else
				skin = 0;

			if (players[i].mo->color)
			{
				if (players[i].mo->colorized)
					colormap = R_GetTranslationColormap(TC_RAINBOW, players[i].mo->color, GTC_CACHE);
				else
					colormap = R_GetTranslationColormap(skin, players[i].mo->color, GTC_CACHE);
			}
			else
				colormap = NULL;

			K_drawKartMinimapIcon(players[i].mo->x, players[i].mo->y, x, y, splitflags, faceprefix[skin][FACE_MINIMAP], colormap, AutomapPic);
			// Target reticule
			if ((gametype == GT_RACE && players[i].kartstuff[k_position] == spbplace)
			|| (gametype == GT_BATTLE && K_IsPlayerWanted(&players[i])))
				K_drawKartMinimapIcon(players[i].mo->x, players[i].mo->y, x, y, splitflags, kp_wantedreticle, NULL, AutomapPic);
		}
	}

	// draw SPB(s?)
	for (mobj = kitemcap; mobj; mobj = next)
	{
		next = mobj->itnext;
		if (mobj->type == MT_SPB)
		{
			colormap = NULL;

			if (mobj->target && !P_MobjWasRemoved(mobj->target))
			{
				if (mobj->player && mobj->player->skincolor)
					colormap = R_GetTranslationColormap(TC_RAINBOW, mobj->player->skincolor, GTC_CACHE);
				else if (mobj->color)
					colormap = R_GetTranslationColormap(TC_RAINBOW, mobj->color, GTC_CACHE);
			}

			K_drawKartMinimapIcon(mobj->x, mobj->y, x, y, splitflags, kp_spbminimap, colormap, AutomapPic);
		}
	}

	// draw our local players here, opaque.
	splitflags &= ~V_HUDTRANSHALF;
	splitflags |= V_HUDTRANS;

	for (i = 0; i < numlocalplayers; i++)
	{
		if (i == -1)
			continue; // this doesn't interest us

		if (players[localplayers[i]].mo->skin)
			skin = ((skin_t*)players[localplayers[i]].mo->skin)-skins;
		else
			skin = 0;

		if (players[localplayers[i]].mo->color)
		{
			if (players[localplayers[i]].mo->colorized)
				colormap = R_GetTranslationColormap(TC_RAINBOW, players[localplayers[i]].mo->color, GTC_CACHE);
			else
				colormap = R_GetTranslationColormap(skin, players[localplayers[i]].mo->color, GTC_CACHE);
		}
		else
			colormap = NULL;

		K_drawKartMinimapIcon(players[localplayers[i]].mo->x, players[localplayers[i]].mo->y, x, y, splitflags, faceprefix[skin][FACE_MINIMAP], colormap, AutomapPic);

		// Target reticule
		if ((gametype == GT_RACE && players[localplayers[i]].kartstuff[k_position] == spbplace)
		|| (gametype == GT_BATTLE && K_IsPlayerWanted(&players[localplayers[i]])))
			K_drawKartMinimapIcon(players[localplayers[i]].mo->x, players[localplayers[i]].mo->y, x, y, splitflags, kp_wantedreticle, NULL, AutomapPic);
	}
}

static void K_drawKartStartBulbs(void)
{
	const UINT8 start_animation[14] = {
		1, 2, 3, 4, 5, 6, 7, 8,
		7, 6,
		9, 10, 11, 12
	};

	const UINT8 loop_animation[4] = {
		12, 13, 12, 14
	};

	const UINT8 chillloop_animation[2] = {
		11, 12
	};

	const UINT8 letters_order[10] = {
		0, 1, 2, 3, 4, 3, 1, 5, 6, 6
	};

	const UINT8 letters_transparency[40] = {
		0, 2, 4, 6, 8,
		10, 10, 10, 10, 10,
		10, 10, 10, 10, 10,
		10, 10, 10, 10, 10,
		10, 10, 10, 10, 10,
		10, 10, 10, 10, 10,
		10, 10, 10, 10, 10,
		10, 8, 6, 4, 2
	};

	fixed_t spacing = 24*FRACUNIT;

	fixed_t startx = (BASEVIDWIDTH/2)*FRACUNIT;
	fixed_t starty = 48*FRACUNIT;
	fixed_t x, y;

	UINT8 numperrow = numbulbs/2;
	UINT8 i;

	if (r_splitscreen >= 1)
	{
		spacing /= 2;
		starty /= 3;

		if (r_splitscreen > 1)
		{
			startx /= 2;
		}
	}

	startx += (spacing/2);

	if (numbulbs <= 10)
	{
		// No second row
		numperrow = numbulbs;
	}
	else
	{
		if (numbulbs & 1)
		{
			numperrow++;
		}

		starty -= (spacing/2);
	}

	startx -= (spacing/2) * numperrow;

	x = startx;
	y = starty;

	for (i = 0; i < numbulbs; i++)
	{
		UINT8 patchnum = 0;
		INT32 bulbtic = (leveltime - introtime - TICRATE) - (bulbtime * i);

		if (i == numperrow)
		{
			y += spacing;
			x = startx + (spacing/2);
		}

		if (bulbtic > 0)
		{
			if (bulbtic < 14)
			{
				patchnum = start_animation[bulbtic];
			}
			else
			{
				const INT32 length = (bulbtime * 3);

				bulbtic -= 14;

				if (bulbtic > length)
				{
					bulbtic -= length;
					patchnum = chillloop_animation[bulbtic % 2];
				}
				else
				{
					patchnum = loop_animation[bulbtic % 4];
				}
			}
		}

		V_DrawFixedPatch(x, y, FRACUNIT, V_SNAPTOTOP|V_SPLITSCREEN,
			(r_splitscreen ? kp_prestartbulb_split[patchnum] : kp_prestartbulb[patchnum]), NULL);
		x += spacing;
	}

	x = 70*FRACUNIT;
	y = starty;

	if (r_splitscreen == 1)
	{
		x = 106*FRACUNIT;
	}
	else if (r_splitscreen > 1)
	{
		x = 28*FRACUNIT;
	}

	if (timeinmap < 16)
		return; // temporary for current map start behaviour

	for (i = 0; i < 10; i++)
	{
		UINT8 patchnum = letters_order[i];
		INT32 transflag = letters_transparency[(leveltime - i) % 40];
		patch_t *patch = (r_splitscreen ? kp_prestartletters_split[patchnum] : kp_prestartletters[patchnum]);

		if (transflag >= 10)
			;
		else
		{
			if (transflag != 0)
				transflag = transflag << FF_TRANSSHIFT;

			V_DrawFixedPatch(x, y, FRACUNIT, V_SNAPTOTOP|V_SPLITSCREEN|transflag, patch, NULL);
		}

		if (i < 9)
		{
			x += (SHORT(patch->width)) * FRACUNIT/2;

			patchnum = letters_order[i+1];
			patch = (r_splitscreen ? kp_prestartletters_split[patchnum] : kp_prestartletters[patchnum]);
			x += (SHORT(patch->width)) * FRACUNIT/2;

			if (r_splitscreen)
				x -= FRACUNIT;
		}
	}
}

static void K_drawKartStartCountdown(void)
{
	INT32 pnum = 0;

	if (stplyr->karthud[khud_fault] != 0)
	{
		INT32 x, xval;

		if (r_splitscreen > 1) // 3/4p, stationary FIN
		{
			pnum += 2;
		}
		else if (r_splitscreen == 1) // wide splitscreen
		{
			pnum += 4;
		}

		if ((leveltime % (2*5)) / 5) // blink
			pnum += 1;

		if (r_splitscreen == 0)
		{
			x = ((vid.width<<FRACBITS)/vid.dupx);
			xval = (SHORT(kp_racefault[pnum]->width)<<FRACBITS);
			x = ((TICRATE - stplyr->karthud[khud_fault])*(xval > x ? xval : x))/TICRATE;

			V_DrawFixedPatch(x + (STCD_X<<FRACBITS) - (xval>>1),
				(STCD_Y<<FRACBITS) - (SHORT(kp_racefault[pnum]->height)<<(FRACBITS-1)),
				FRACUNIT,
				V_SPLITSCREEN, kp_racefault[pnum], NULL);
		}
		else
		{
			V_DrawScaledPatch(STCD_X - (SHORT(kp_racefault[pnum]->width)/2), STCD_Y - (SHORT(kp_racefault[pnum]->height)/2), V_SPLITSCREEN, kp_racefault[pnum]);
		}
	}
	else if (leveltime >= introtime && leveltime < starttime-(3*TICRATE))
	{
		K_drawKartStartBulbs();
	}
	else
	{

		if (leveltime >= starttime-(2*TICRATE)) // 2
			pnum++;
		if (leveltime >= starttime-TICRATE) // 1
			pnum++;

		if (leveltime >= starttime) // GO!
		{
			UINT8 i;
			UINT8 numplayers = 0;

			pnum++;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && !players[i].spectator)
					numplayers++;

				if (numplayers > 2)
					break;
			}

			if (numplayers == 2)
			{
				pnum++; // DUEL
			}
		}

		if ((leveltime % (2*5)) / 5) // blink
			pnum += 5;
		if (r_splitscreen) // splitscreen
			pnum += 10;

		V_DrawScaledPatch(STCD_X - (SHORT(kp_startcountdown[pnum]->width)/2), STCD_Y - (SHORT(kp_startcountdown[pnum]->height)/2), V_SPLITSCREEN, kp_startcountdown[pnum]);
	}
}

static void K_drawKartFinish(void)
{
	INT32 pnum = 0, splitflags = V_SPLITSCREEN;

	if (!stplyr->karthud[khud_cardanimation] || stplyr->karthud[khud_cardanimation] >= 2*TICRATE)
		return;

	if ((stplyr->karthud[khud_cardanimation] % (2*5)) / 5) // blink
		pnum = 1;

	if (r_splitscreen > 1) // 3/4p, stationary FIN
	{
		pnum += 2;
		V_DrawScaledPatch(STCD_X - (SHORT(kp_racefinish[pnum]->width)/2), STCD_Y - (SHORT(kp_racefinish[pnum]->height)/2), splitflags, kp_racefinish[pnum]);
		return;
	}

	//else -- 1/2p, scrolling FINISH
	{
		INT32 x, xval;

		if (r_splitscreen) // wide splitscreen
			pnum += 4;

		x = ((vid.width<<FRACBITS)/vid.dupx);
		xval = (SHORT(kp_racefinish[pnum]->width)<<FRACBITS);
		x = ((TICRATE - stplyr->karthud[khud_cardanimation])*(xval > x ? xval : x))/TICRATE;

		if (r_splitscreen && stplyr == &players[displayplayers[1]])
			x = -x;

		V_DrawFixedPatch(x + (STCD_X<<FRACBITS) - (xval>>1),
			(STCD_Y<<FRACBITS) - (SHORT(kp_racefinish[pnum]->height)<<(FRACBITS-1)),
			FRACUNIT,
			splitflags, kp_racefinish[pnum], NULL);
	}
}

static void K_drawBattleFullscreen(void)
{
	INT32 x = BASEVIDWIDTH/2;
	INT32 y = -64+(stplyr->karthud[khud_cardanimation]); // card animation goes from 0 to 164, 164 is the middle of the screen
	INT32 splitflags = V_SNAPTOTOP; // I don't feel like properly supporting non-green resolutions, so you can have a misuse of SNAPTO instead
	fixed_t scale = FRACUNIT;
	boolean drawcomebacktimer = true;	// lazy hack because it's cleaner in the long run.

	if (!LUA_HudEnabled(hud_battlecomebacktimer))
		drawcomebacktimer = false;

	if (r_splitscreen)
	{
		if ((r_splitscreen == 1 && stplyr == &players[displayplayers[1]])
			|| (r_splitscreen > 1 && (stplyr == &players[displayplayers[2]]
			|| (stplyr == &players[displayplayers[3]] && r_splitscreen > 2))))
		{
			y = 232-(stplyr->karthud[khud_cardanimation]/2);
			splitflags = V_SNAPTOBOTTOM;
		}
		else
			y = -32+(stplyr->karthud[khud_cardanimation]/2);

		if (r_splitscreen > 1)
		{
			scale /= 2;

			if (stplyr == &players[displayplayers[1]]
				|| (stplyr == &players[displayplayers[3]] && r_splitscreen > 2))
				x = 3*BASEVIDWIDTH/4;
			else
				x = BASEVIDWIDTH/4;
		}
		else
		{
			if (stplyr->exiting)
			{
				if (stplyr == &players[displayplayers[1]])
					x = BASEVIDWIDTH-96;
				else
					x = 96;
			}
			else
				scale /= 2;
		}
	}

	if (stplyr->exiting)
	{
		if (stplyr == &players[displayplayers[0]])
			V_DrawFadeScreen(0xFF00, 16);
		if (stplyr->exiting < 6*TICRATE && !stplyr->spectator)
		{
			patch_t *p = kp_battlecool;

			if (K_IsPlayerLosing(stplyr))
				p = kp_battlelose;
			else if (stplyr->kartstuff[k_position] == 1)
				p = kp_battlewin;

			V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, scale, splitflags, p, NULL);
		}
		else
			K_drawKartFinish();
	}
	else if (stplyr->kartstuff[k_bumper] <= 0 && stplyr->kartstuff[k_comebacktimer] && comeback && !stplyr->spectator && drawcomebacktimer)
	{
		UINT16 t = stplyr->kartstuff[k_comebacktimer]/(10*TICRATE);
		INT32 txoff, adjust = (r_splitscreen > 1) ? 4 : 6; // normal string is 8, kart string is 12, half of that for ease
		INT32 ty = (BASEVIDHEIGHT/2)+66;

		txoff = adjust;

		while (t)
		{
			txoff += adjust;
			t /= 10;
		}

		if (r_splitscreen)
		{
			if (r_splitscreen > 1)
				ty = (BASEVIDHEIGHT/4)+33;
			if ((r_splitscreen == 1 && stplyr == &players[displayplayers[1]])
				|| (stplyr == &players[displayplayers[2]] && r_splitscreen > 1)
				|| (stplyr == &players[displayplayers[3]] && r_splitscreen > 2))
				ty += (BASEVIDHEIGHT/2);
		}
		else
			V_DrawFadeScreen(0xFF00, 16);

		if (!comebackshowninfo)
			V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, scale, splitflags, kp_battleinfo, NULL);
		else
			V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, scale, splitflags, kp_battlewait, NULL);

		if (r_splitscreen > 1)
			V_DrawString(x-txoff, ty, 0, va("%d", stplyr->kartstuff[k_comebacktimer]/TICRATE));
		else
		{
			V_DrawFixedPatch(x<<FRACBITS, ty<<FRACBITS, scale, 0, kp_timeoutsticker, NULL);
			V_DrawKartString(x-txoff, ty, 0, va("%d", stplyr->kartstuff[k_comebacktimer]/TICRATE));
		}
	}

	if (netgame && !stplyr->spectator && timeinmap > 113) // FREE PLAY?
	{
		UINT8 i;

		// check to see if there's anyone else at all
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (i == displayplayers[0])
				continue;
			if (playeringame[i] && !stplyr->spectator)
				return;
		}

		if (LUA_HudEnabled(hud_freeplay))
			K_drawKartFreePlay(leveltime);
	}
}

static void K_drawKartFirstPerson(void)
{
	static INT32 pnum[4], turn[4], drift[4];
	INT32 pn = 0, tn = 0, dr = 0;
	INT32 target = 0, splitflags = V_SNAPTOBOTTOM|V_SPLITSCREEN;
	INT32 x = BASEVIDWIDTH/2, y = BASEVIDHEIGHT;
	fixed_t scale;
	UINT8 *colmap = NULL;
	ticcmd_t *cmd = &stplyr->cmd;

	if (stplyr->spectator || !stplyr->mo || (stplyr->mo->drawflags & MFD_DONTDRAW))
		return;

	if (stplyr == &players[displayplayers[1]] && r_splitscreen)
		{ pn = pnum[1]; tn = turn[1]; dr = drift[1]; }
	else if (stplyr == &players[displayplayers[2]] && r_splitscreen > 1)
		{ pn = pnum[2]; tn = turn[2]; dr = drift[2]; }
	else if (stplyr == &players[displayplayers[3]] && r_splitscreen > 2)
		{ pn = pnum[3]; tn = turn[3]; dr = drift[3]; }
	else
		{ pn = pnum[0]; tn = turn[0]; dr = drift[0]; }

	if (r_splitscreen)
	{
		y >>= 1;
		if (r_splitscreen > 1)
			x >>= 1;
	}

	{
		if (stplyr->speed < (20*stplyr->mo->scale) && (leveltime & 1) && !r_splitscreen)
			y++;

		if (stplyr->mo->drawflags & MFD_TRANSMASK)
			splitflags |= ((stplyr->mo->drawflags & MFD_TRANSMASK) >> MFD_TRANSSHIFT) << FF_TRANSSHIFT;
		else if (stplyr->mo->frame & FF_TRANSMASK)
			splitflags |= (stplyr->mo->frame & FF_TRANSMASK);
	}

	if (cmd->turning > 400) // strong left turn
		target = 2;
	else if (cmd->turning < -400) // strong right turn
		target = -2;
	else if (cmd->turning > 0) // weak left turn
		target = 1;
	else if (cmd->turning < 0) // weak right turn
		target = -1;
	else // forward
		target = 0;

	if (encoremode)
		target = -target;

	if (pn < target)
		pn++;
	else if (pn > target)
		pn--;

	if (pn < 0)
		splitflags |= V_FLIP; // right turn

	target = abs(pn);
	if (target > 2)
		target = 2;

	x <<= FRACBITS;
	y <<= FRACBITS;

	if (tn != cmd->turning/50)
		tn -= (tn - (cmd->turning/50))/8;

	if (dr != stplyr->kartstuff[k_drift]*16)
		dr -= (dr - (stplyr->kartstuff[k_drift]*16))/8;

	if (r_splitscreen == 1)
	{
		scale = (2*FRACUNIT)/3;
		y += FRACUNIT/(vid.dupx < vid.dupy ? vid.dupx : vid.dupy); // correct a one-pixel gap on the screen view (not the basevid view)
	}
	else if (r_splitscreen)
		scale = FRACUNIT/2;
	else
		scale = FRACUNIT;

	if (stplyr->mo)
	{
		UINT8 driftcolor = K_DriftSparkColor(stplyr, stplyr->kartstuff[k_driftcharge]);
		const angle_t ang = R_PointToAngle2(0, 0, stplyr->rmomx, stplyr->rmomy) - stplyr->drawangle;
		// yes, the following is correct. no, you do not need to swap the x and y.
		fixed_t xoffs = -P_ReturnThrustY(stplyr->mo, ang, (BASEVIDWIDTH<<(FRACBITS-2))/2);
		fixed_t yoffs = -(P_ReturnThrustX(stplyr->mo, ang, 4*FRACUNIT) - 4*FRACUNIT);

		if (r_splitscreen)
			xoffs = FixedMul(xoffs, scale);

		xoffs -= (tn)*scale;
		xoffs -= (dr)*scale;

		if (stplyr->drawangle == stplyr->mo->angle)
		{
			const fixed_t mag = FixedDiv(stplyr->speed, 10*stplyr->mo->scale);

			if (mag < FRACUNIT)
			{
				xoffs = FixedMul(xoffs, mag);
				if (!r_splitscreen)
					yoffs = FixedMul(yoffs, mag);
			}
		}

		if (stplyr->mo->momz > 0) // TO-DO: Draw more of the kart so we can remove this if!
			yoffs += stplyr->mo->momz/3;

		if (encoremode)
			x -= xoffs;
		else
			x += xoffs;
		if (!r_splitscreen)
			y += yoffs;


		if ((leveltime & 1) && (driftcolor != SKINCOLOR_NONE)) // drift sparks!
			colmap = R_GetTranslationColormap(TC_RAINBOW, driftcolor, GTC_CACHE);
		else if (stplyr->mo->colorized && stplyr->mo->color) // invincibility/grow/shrink!
			colmap = R_GetTranslationColormap(TC_RAINBOW, stplyr->mo->color, GTC_CACHE);
	}

	V_DrawFixedPatch(x, y, scale, splitflags, kp_fpview[target], colmap);

	if (stplyr == &players[displayplayers[1]] && r_splitscreen)
		{ pnum[1] = pn; turn[1] = tn; drift[1] = dr; }
	else if (stplyr == &players[displayplayers[2]] && r_splitscreen > 1)
		{ pnum[2] = pn; turn[2] = tn; drift[2] = dr; }
	else if (stplyr == &players[displayplayers[3]] && r_splitscreen > 2)
		{ pnum[3] = pn; turn[3] = tn; drift[3] = dr; }
	else
		{ pnum[0] = pn; turn[0] = tn; drift[0] = dr; }
}

// doesn't need to ever support 4p
static void K_drawInput(void)
{
	static INT32 pn = 0;
	INT32 target = 0, splitflags = (V_SNAPTOBOTTOM|V_SNAPTORIGHT);
	INT32 x = BASEVIDWIDTH - 32, y = BASEVIDHEIGHT-24, offs, col;
	const INT32 accent1 = splitflags | skincolors[stplyr->skincolor].ramp[5];
	const INT32 accent2 = splitflags | skincolors[stplyr->skincolor].ramp[9];
	ticcmd_t *cmd = &stplyr->cmd;

	if (timeinmap <= 105)
		return;

	if (timeinmap < 113)
	{
		INT32 count = ((INT32)(timeinmap) - 105);
		offs = 64;
		while (count-- > 0)
			offs >>= 1;
		x += offs;
	}

#define BUTTW 8
#define BUTTH 11

#define drawbutt(xoffs, butt, symb)\
	if (stplyr->cmd.buttons & butt)\
	{\
		offs = 2;\
		col = accent1;\
	}\
	else\
	{\
		offs = 0;\
		col = accent2;\
		V_DrawFill(x+(xoffs), y+BUTTH, BUTTW-1, 2, splitflags|31);\
	}\
	V_DrawFill(x+(xoffs), y+offs, BUTTW-1, BUTTH, col);\
	V_DrawFixedPatch((x+1+(xoffs))<<FRACBITS, (y+offs+1)<<FRACBITS, FRACUNIT, splitflags, fontv[TINY_FONT].font[symb-HU_FONTSTART], NULL)

	drawbutt(-2*BUTTW, BT_ACCELERATE, 'A');
	drawbutt(  -BUTTW, BT_BRAKE,      'B');
	drawbutt(       0, BT_DRIFT,      'D');
	drawbutt(   BUTTW, BT_ATTACK,     'I');

#undef drawbutt

#undef BUTTW
#undef BUTTH

	y -= 1;

	if (!cmd->turning) // no turn
		target = 0;
	else // turning of multiple strengths!
	{
		target = ((abs(cmd->turning) - 1)/125)+1;
		if (target > 4)
			target = 4;
		if (cmd->turning < 0)
			target = -target;
	}

	if (pn != target)
	{
		if (abs(pn - target) == 1)
			pn = target;
		else if (pn < target)
			pn += 2;
		else //if (pn > target)
			pn -= 2;
	}

	if (pn < 0)
	{
		splitflags |= V_FLIP; // right turn
		x--;
	}

	target = abs(pn);
	if (target > 4)
		target = 4;

	if (!stplyr->skincolor)
		V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT, splitflags, kp_inputwheel[target], NULL);
	else
	{
		UINT8 *colormap;
		colormap = R_GetTranslationColormap(0, stplyr->skincolor, GTC_CACHE);
		V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT, splitflags, kp_inputwheel[target], colormap);
	}
}

static void K_drawChallengerScreen(void)
{
	// This is an insanely complicated animation.
	static UINT8 anim[52] = {
		0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13, // frame 1-14, 2 tics: HERE COMES A NEW slides in
		14,14,14,14,14,14, // frame 15, 6 tics: pause on the W
		15,16,17,18, // frame 16-19, 1 tic: CHALLENGER approaches screen
		19,20,19,20,19,20,19,20,19,20, // frame 20-21, 1 tic, 5 alternating: all text vibrates from impact
		21,22,23,24 // frame 22-25, 1 tic: CHALLENGER turns gold
	};
	const UINT8 offset = min(52-1, (3*TICRATE)-mapreset);

	V_DrawFadeScreen(0xFF00, 16); // Fade out
	V_DrawScaledPatch(0, 0, 0, kp_challenger[anim[offset]]);
}

static void K_drawLapStartAnim(void)
{
	// This is an EVEN MORE insanely complicated animation.
	const UINT8 progress = 80-stplyr->karthud[khud_lapanimation];
	UINT8 *colormap = R_GetTranslationColormap(TC_DEFAULT, stplyr->skincolor, GTC_CACHE);

	V_DrawFixedPatch((BASEVIDWIDTH/2 + (32*max(0, stplyr->karthud[khud_lapanimation]-76)))*FRACUNIT,
		(48 - (32*max(0, progress-76)))*FRACUNIT,
		FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
		(modeattacking ? kp_lapanim_emblem[1] : kp_lapanim_emblem[0]), colormap);

	if (stplyr->karthud[khud_laphand] >= 1 && stplyr->karthud[khud_laphand] <= 3)
	{
		V_DrawFixedPatch((BASEVIDWIDTH/2 + (32*max(0, stplyr->karthud[khud_lapanimation]-76)))*FRACUNIT,
			(48 - (32*max(0, progress-76))
				+ 4 - abs((signed)((leveltime % 8) - 4)))*FRACUNIT,
			FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
			kp_lapanim_hand[stplyr->karthud[khud_laphand]-1], NULL);
	}

	if (stplyr->laps == (UINT8)(cv_numlaps.value))
	{
		V_DrawFixedPatch((62 - (32*max(0, progress-76)))*FRACUNIT, // 27
			30*FRACUNIT, // 24
			FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
			kp_lapanim_final[min(progress/2, 10)], NULL);

		if (progress/2-12 >= 0)
		{
			V_DrawFixedPatch((188 + (32*max(0, progress-76)))*FRACUNIT, // 194
				30*FRACUNIT, // 24
				FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
				kp_lapanim_lap[min(progress/2-12, 6)], NULL);
		}
	}
	else
	{
		V_DrawFixedPatch((82 - (32*max(0, progress-76)))*FRACUNIT, // 61
			30*FRACUNIT, // 24
			FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
			kp_lapanim_lap[min(progress/2, 6)], NULL);

		if (progress/2-8 >= 0)
		{
			V_DrawFixedPatch((188 + (32*max(0, progress-76)))*FRACUNIT, // 194
				30*FRACUNIT, // 24
				FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
				kp_lapanim_number[(((UINT32)stplyr->laps) / 10)][min(progress/2-8, 2)], NULL);

			if (progress/2-10 >= 0)
			{
				V_DrawFixedPatch((208 + (32*max(0, progress-76)))*FRACUNIT, // 221
					30*FRACUNIT, // 24
					FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
					kp_lapanim_number[(((UINT32)stplyr->laps) % 10)][min(progress/2-10, 2)], NULL);
			}
		}
	}
}

void K_drawKartFreePlay(UINT32 flashtime)
{
	// no splitscreen support because it's not FREE PLAY if you have more than one player in-game
	// (you fool, you can take splitscreen online. :V)

	if ((flashtime % TICRATE) < TICRATE/2)
		return;

	V_DrawKartString((BASEVIDWIDTH - (LAPS_X+1)) - (12*9), // mirror the laps thingy
		LAPS_Y+3, V_HUDTRANS|V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTORIGHT, "FREE PLAY");
}

static void
Draw_party_ping (int ss, INT32 snap)
{
	HU_drawMiniPing(0, 0, playerpingtable[displayplayers[ss]], V_HUDTRANS|V_SPLITSCREEN|V_SNAPTOTOP|snap);
}

static void
K_drawMiniPing (void)
{
	UINT32 f = V_SNAPTORIGHT;
	UINT8 i;

	if (!cv_showping.value)
	{
		return;
	}

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (stplyr == &players[displayplayers[i]])
		{
			if (r_splitscreen > 1 && !(i & 1))
			{
				f = V_SNAPTOLEFT;
			}

			Draw_party_ping(i, f);
			break;
		}
	}
}

static void K_drawDistributionDebugger(void)
{
	patch_t *items[NUMKARTRESULTS] = {
		kp_sadface[1],
		kp_sneaker[1],
		kp_rocketsneaker[1],
		kp_invincibility[7],
		kp_banana[1],
		kp_eggman[1],
		kp_orbinaut[4],
		kp_jawz[1],
		kp_mine[1],
		kp_ballhog[1],
		kp_selfpropelledbomb[1],
		kp_grow[1],
		kp_shrink[1],
		kp_thundershield[1],
		kp_bubbleshield[1],
		kp_flameshield[1],
		kp_hyudoro[1],
		kp_pogospring[1],
		kp_superring[1],
		kp_kitchensink[1],

		kp_sneaker[1],
		kp_banana[1],
		kp_banana[1],
		kp_orbinaut[4],
		kp_orbinaut[4],
		kp_jawz[1]
	};
	UINT8 useodds = 0;
	UINT8 pingame = 0, bestbumper = 0;
	UINT32 pdis = 0;
	INT32 i;
	INT32 x = -9, y = -9;
	boolean spbrush = false;

	if (stplyr != &players[displayplayers[0]]) // only for p1
		return;

	// The only code duplication from the Kart, just to avoid the actual item function from calculating pingame twice
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;
		pingame++;
		if (players[i].kartstuff[k_bumper] > bestbumper)
			bestbumper = players[i].kartstuff[k_bumper];
	}

	// lovely double loop......
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator
			&& players[i].kartstuff[k_position] == 1)
		{
			// This player is first! Yay!
			pdis = stplyr->distancetofinish - players[i].distancetofinish;
			break;
		}
	}

	if (franticitems) // Frantic items make the distances between everyone artifically higher, for crazier items
		pdis = (15 * pdis) / 14;

	if (spbplace != -1 && stplyr->kartstuff[k_position] == spbplace+1) // SPB Rush Mode: It's 2nd place's job to catch-up items and make 1st place's job hell
	{
		pdis = (3 * pdis) / 2;
		spbrush = true;
	}

	if (stplyr->bot && stplyr->botvars.rival)
	{
		// Rival has better odds :)
		pdis = (15 * pdis) / 14;
	}

	pdis = ((28 + (8-pingame)) * pdis) / 28; // scale with player count

	useodds = K_FindUseodds(stplyr, 0, pdis, bestbumper, spbrush);

	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		const INT32 itemodds = K_KartGetItemOdds(useodds, i, 0, spbrush, stplyr->bot, (stplyr->bot && stplyr->botvars.rival));
		if (itemodds <= 0)
			continue;

		V_DrawScaledPatch(x, y, V_HUDTRANS|V_SLIDEIN|V_SNAPTOTOP, items[i]);
		V_DrawThinString(x+11, y+31, V_HUDTRANS|V_SLIDEIN|V_SNAPTOTOP, va("%d", itemodds));

		// Display amount for multi-items
		if (i >= NUMKARTITEMS)
		{
			INT32 amount;
			switch (i)
			{
				case KRITEM_TENFOLDBANANA:
					amount = 10;
					break;
				case KRITEM_QUADORBINAUT:
					amount = 4;
					break;
				case KRITEM_DUALJAWZ:
					amount = 2;
					break;
				default:
					amount = 3;
					break;
			}
			V_DrawString(x+24, y+31, V_ALLOWLOWERCASE|V_HUDTRANS|V_SLIDEIN|V_SNAPTOTOP, va("x%d", amount));
		}

		x += 32;
		if (x >= 297)
		{
			x = -9;
			y += 32;
		}
	}

	V_DrawString(0, 0, V_HUDTRANS|V_SLIDEIN|V_SNAPTOTOP, va("USEODDS %d", useodds));
}

static void K_drawCheckpointDebugger(void)
{
	if (stplyr != &players[displayplayers[0]]) // only for p1
		return;

	if (stplyr->starpostnum == numstarposts)
		V_DrawString(8, 184, 0, va("Checkpoint: %d / %d (Can finish)", stplyr->starpostnum, numstarposts));
	else
		V_DrawString(8, 184, 0, va("Checkpoint: %d / %d", stplyr->starpostnum, numstarposts));
}

static void K_DrawWaypointDebugger(void)
{
	if ((cv_kartdebugwaypoints.value != 0) && (stplyr == &players[displayplayers[0]]))
	{
		V_DrawString(8, 166, 0, va("'Best' Waypoint ID: %d", K_GetWaypointID(stplyr->nextwaypoint)));
		V_DrawString(8, 176, 0, va("Finishline Distance: %d", stplyr->distancetofinish));
	}
}

void K_drawKartHUD(void)
{
	boolean isfreeplay = false;
	boolean battlefullscreen = false;
	boolean freecam = demo.freecam;	//disable some hud elements w/ freecam
	UINT8 i;

	// Define the X and Y for each drawn object
	// This is handled by console/menu values
	K_initKartHUD();

	// Draw that fun first person HUD! Drawn ASAP so it looks more "real".
	for (i = 0; i <= r_splitscreen; i++)
	{
		if (stplyr == &players[displayplayers[i]] && !camera[i].chase && !freecam)
			K_drawKartFirstPerson();
	}

	// Draw full screen stuff that turns off the rest of the HUD
	if (mapreset && stplyr == &players[displayplayers[0]])
	{
		K_drawChallengerScreen();
		return;
	}

	battlefullscreen = ((gametype == GT_BATTLE)
		&& (stplyr->exiting
		|| (stplyr->kartstuff[k_bumper] <= 0
		&& stplyr->kartstuff[k_comebacktimer]
		&& comeback
		&& stplyr->playerstate == PST_LIVE)));

	if (!demo.title && (!battlefullscreen || r_splitscreen))
	{
		// Draw the CHECK indicator before the other items, so it's overlapped by everything else
		if (LUA_HudEnabled(hud_check))	// delete lua when?
			if (cv_kartcheck.value && !splitscreen && !players[displayplayers[0]].exiting && !freecam)
				K_drawKartPlayerCheck();

		// nametags
		if (LUA_HudEnabled(hud_names))
			K_drawKartNameTags();

		// Draw WANTED status
		if (gametype == GT_BATTLE)
		{
			if (LUA_HudEnabled(hud_wanted))
				K_drawKartWanted();
		}

		if (cv_kartminimap.value)
		{
			if (LUA_HudEnabled(hud_minimap))
				K_drawKartMinimap();
		}
	}

	if (battlefullscreen && !freecam)
	{
		if (LUA_HudEnabled(hud_battlefullscreen))
			K_drawBattleFullscreen();
		return;
	}

	// Draw the item window
	if (LUA_HudEnabled(hud_item) && !freecam)
		K_drawKartItem();

	// If not splitscreen, draw...
	if (!r_splitscreen && !demo.title)
	{
		// Draw the timestamp
		if (LUA_HudEnabled(hud_time))
			K_drawKartTimestamp(stplyr->realtime, TIME_X, TIME_Y, gamemap, 0);

		if (!modeattacking)
		{
			// The top-four faces on the left
			//if (LUA_HudEnabled(hud_minirankings))
				isfreeplay = K_drawKartPositionFaces();
		}
	}

	if (!stplyr->spectator && !demo.freecam) // Bottom of the screen elements, don't need in spectate mode
	{
		// Draw the speedometer
		if (cv_kartspeedometer.value && !r_splitscreen)
		{
			if (LUA_HudEnabled(hud_speedometer))
				K_drawKartSpeedometer();
		}

		if (demo.title) // Draw title logo instead in demo.titles
		{
			INT32 x = BASEVIDWIDTH - 32, y = 128, offs;

			if (r_splitscreen == 3)
			{
				x = BASEVIDWIDTH/2 + 10;
				y = BASEVIDHEIGHT/2 - 30;
			}

			if (timeinmap < 113)
			{
				INT32 count = ((INT32)(timeinmap) - 104);
				offs = 256;
				while (count-- > 0)
					offs >>= 1;
				x += offs;
			}

			V_DrawTinyScaledPatch(x-54, y, 0, W_CachePatchName("TTKBANNR", PU_CACHE));
			V_DrawTinyScaledPatch(x-54, y+25, 0, W_CachePatchName("TTKART", PU_CACHE));
		}
		else if (gametype == GT_RACE) // Race-only elements
		{
			// Draw the lap counter
			if (LUA_HudEnabled(hud_gametypeinfo))
				K_drawKartLapsAndRings();

			if (isfreeplay)
				;
			else if (!modeattacking)
			{
				// Draw the numerical position
				if (LUA_HudEnabled(hud_position))
					K_DrawKartPositionNum(stplyr->kartstuff[k_position]);
			}
			else //if (!(demo.playback && hu_showscores))
			{
				// Draw the input UI
				if (LUA_HudEnabled(hud_position))
					K_drawInput();
			}
		}
		else if (gametype == GT_BATTLE) // Battle-only
		{
			// Draw the hits left!
			if (LUA_HudEnabled(hud_gametypeinfo))
				K_drawKartBumpersOrKarma();
		}

		if (gametyperules & GTR_SPHERES)
		{
			K_drawBlueSphereMeter();
		}
	}

	// Draw the countdowns after everything else.
	if (leveltime >= introtime
	&& leveltime < starttime+TICRATE)
	{
		K_drawKartStartCountdown();
	}
	else if (racecountdown && (!r_splitscreen || !stplyr->exiting))
	{
		char *countstr = va("%d", racecountdown/TICRATE);

		if (r_splitscreen > 1)
			V_DrawCenteredString(BASEVIDWIDTH/4, LAPS_Y+1, V_SPLITSCREEN, countstr);
		else
		{
			INT32 karlen = strlen(countstr)*6; // half of 12
			V_DrawKartString((BASEVIDWIDTH/2)-karlen, LAPS_Y+3, V_SPLITSCREEN, countstr);
		}
	}

	// Race overlays
	if (gametype == GT_RACE && !freecam)
	{
		if (stplyr->exiting)
			K_drawKartFinish();
		else if (stplyr->karthud[khud_lapanimation] && !r_splitscreen)
			K_drawLapStartAnim();
	}

	if (modeattacking || freecam) // everything after here is MP and debug only
		return;

	if (gametype == GT_BATTLE && !r_splitscreen && (stplyr->karthud[khud_yougotem] % 2)) // * YOU GOT EM *
		V_DrawScaledPatch(BASEVIDWIDTH/2 - (SHORT(kp_yougotem->width)/2), 32, V_HUDTRANS, kp_yougotem);

	// Draw FREE PLAY.
	if (isfreeplay && !stplyr->spectator && timeinmap > 113)
	{
		if (LUA_HudEnabled(hud_freeplay))
			K_drawKartFreePlay(leveltime);
	}

	if (r_splitscreen == 0 && stplyr->kartstuff[k_wrongway] && ((leveltime / 8) & 1))
	{
		V_DrawCenteredString(BASEVIDWIDTH>>1, 176, V_REDMAP|V_SNAPTOBOTTOM, "WRONG WAY");
	}

	if (netgame && r_splitscreen)
	{
		K_drawMiniPing();
	}

	if (cv_kartdebugdistribution.value)
		K_drawDistributionDebugger();

	if (cv_kartdebugcheckpoint.value)
		K_drawCheckpointDebugger();

	if (cv_kartdebugnodes.value)
	{
		UINT8 p;
		for (p = 0; p < MAXPLAYERS; p++)
			V_DrawString(8, 64+(8*p), V_YELLOWMAP, va("%d - %d (%dl)", p, playernode[p], players[p].cmd.latency));
	}

	if (cv_kartdebugcolorize.value && stplyr->mo && stplyr->mo->skin)
	{
		INT32 x = 0, y = 0;
		UINT16 c;

		for (c = 0; c < numskincolors; c++)
		{
			if (skincolors[c].accessible)
			{
				UINT8 *cm = R_GetTranslationColormap(TC_RAINBOW, c, GTC_CACHE);
				V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT>>1, 0, faceprefix[stplyr->skin][FACE_WANTED], cm);

				x += 16;
				if (x > BASEVIDWIDTH-16)
				{
					x = 0;
					y += 16;
				}
			}
		}
	}

	K_DrawWaypointDebugger();
}
