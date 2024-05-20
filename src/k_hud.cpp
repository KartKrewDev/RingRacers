// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_hud.c
/// \brief HUD drawing functions exclusive to Kart

#include <algorithm>
#include <array>
#include <vector>
#include <deque>

#include "v_draw.hpp"

#include "k_hud.h"
#include "k_kart.h"
#include "k_battle.h"
#include "k_grandprix.h"
#include "k_specialstage.h"
#include "k_objects.h"
#include "k_boss.h"
#include "k_color.h"
#include "k_director.h"
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
#include "r_fps.h"
#include "m_random.h"
#include "k_roulette.h"
#include "k_bot.h"
#include "k_rank.h"
#include "g_party.h"
#include "k_hitlag.h"
#include "g_input.h"
#include "k_dialogue.h"
#include "f_finale.h"
#include "m_easing.h"

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
static patch_t *kp_splitspheresticker;
static patch_t *kp_splitkarmabomb;
static patch_t *kp_timeoutsticker;

static patch_t *kp_prestartbulb[15];
static patch_t *kp_prestartletters[7];

static patch_t *kp_prestartbulb_split[15];
static patch_t *kp_prestartletters_split[7];

static patch_t *kp_startcountdown[20];
static patch_t *kp_racefault[6];
static patch_t *kp_racefinish[6];

static patch_t *kp_positionnum[10][2][2]; // number, overlay or underlay, splitscreen

patch_t *kp_facenum[MAXPLAYERS+1];
static patch_t *kp_facehighlight[8];

static patch_t *kp_nocontestminimap;
patch_t *kp_unknownminimap;
static patch_t *kp_spbminimap;
static patch_t *kp_wouldyoustillcatchmeifiwereaworm;
static patch_t *kp_catcherminimap;
static patch_t *kp_emeraldminimap[2];
static patch_t *kp_capsuleminimap[3];
static patch_t *kp_battleufominimap;
static patch_t *kp_superflickyminimap;

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
static patch_t *kp_bigbumper;
static patch_t *kp_tinybumper[2];
static patch_t *kp_ranknobumpers;
static patch_t *kp_rankcapsule;
static patch_t *kp_rankemerald;
static patch_t *kp_rankemeraldflash;
static patch_t *kp_rankemeraldback;
static patch_t *kp_pts[2];

static patch_t *kp_goal[2][2]; // [skull][4p]
static patch_t *kp_goalrod[2]; // [4p]
static patch_t *kp_goaltext1p;

static patch_t *kp_battlewin;
static patch_t *kp_battlecool;
static patch_t *kp_battlelose;
static patch_t *kp_battlewait;
static patch_t *kp_battleinfo;
static patch_t *kp_wanted;
static patch_t *kp_wantedsplit;
static patch_t *kp_wantedreticle;
static patch_t *kp_minimapdot;

static patch_t *kp_itembg[6];
static patch_t *kp_ringbg[4];
static patch_t *kp_itemtimer[2];
static patch_t *kp_itemmulsticker[2];
static patch_t *kp_itemx;

static patch_t *kp_sadface[3];
static patch_t *kp_sneaker[3];
static patch_t *kp_rocketsneaker[3];
static patch_t *kp_invincibility[19];
static patch_t *kp_banana[3];
static patch_t *kp_eggman[3];
static patch_t *kp_orbinaut[6];
static patch_t *kp_jawz[3];
static patch_t *kp_mine[3];
static patch_t *kp_landmine[3];
static patch_t *kp_ballhog[3];
static patch_t *kp_selfpropelledbomb[3];
static patch_t *kp_grow[3];
static patch_t *kp_shrink[3];
static patch_t *kp_lightningshield[3];
static patch_t *kp_bubbleshield[3];
static patch_t *kp_flameshield[3];
static patch_t *kp_hyudoro[3];
static patch_t *kp_pogospring[3];
static patch_t *kp_superring[3];
static patch_t *kp_kitchensink[3];
static patch_t *kp_droptarget[3];
static patch_t *kp_gardentop[3];
static patch_t *kp_gachabom[3];
static patch_t *kp_bar[2];
static patch_t *kp_doublebar[2];
static patch_t *kp_triplebar[2];
static patch_t *kp_slotring[2];
static patch_t *kp_seven[2];
static patch_t *kp_jackpot[2];

static patch_t *kp_check[6];

static patch_t *kp_rival[2];
static patch_t *kp_localtag[4][2];

static patch_t *kp_talk;
static patch_t *kp_typdot;

patch_t *kp_eggnum[6];

static patch_t *kp_flameshieldmeter[FLAMESHIELD_MAX][2];
static patch_t *kp_flameshieldmeter_bg[FLAMESHIELD_MAX][2];

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

static patch_t *kp_bossbar[8];
static patch_t *kp_bossret[4];

static patch_t *kp_trickcool[2];

patch_t *kp_autoroulette;
patch_t *kp_autoring;

patch_t *kp_capsuletarget_arrow[2][2];
patch_t *kp_capsuletarget_icon[2];
patch_t *kp_capsuletarget_far[2][2];
patch_t *kp_capsuletarget_far_text[2];
patch_t *kp_capsuletarget_near[2][8];

patch_t *kp_superflickytarget[2][4];

patch_t *kp_spraycantarget_far[2][6];
patch_t *kp_spraycantarget_near[2][6];

patch_t *kp_button_a[2][2];
patch_t *kp_button_b[2][2];
patch_t *kp_button_c[2][2];
patch_t *kp_button_x[2][2];
patch_t *kp_button_y[2][2];
patch_t *kp_button_z[2][2];
patch_t *kp_button_start[2];
patch_t *kp_button_l[2];
patch_t *kp_button_r[2];
patch_t *kp_button_up[2];
patch_t *kp_button_down[2];
patch_t *kp_button_right[2];
patch_t *kp_button_left[2];
patch_t *kp_button_dpad[2];

static void K_LoadButtonGraphics(patch_t *kp[2], int letter)
{
	HU_UpdatePatch(&kp[0], "TLB_%c", letter);
	HU_UpdatePatch(&kp[1], "TLB_%cB", letter);
}

void K_LoadKartHUDGraphics(void)
{
	INT32 i, j, k;
	char buffer[9];

	// Null Stuff
	HU_UpdatePatch(&kp_nodraw, "K_TRNULL");

	// Stickers
	HU_UpdatePatch(&kp_timesticker, "K_STTIME");
	HU_UpdatePatch(&kp_timestickerwide, "K_STTIMW");
	HU_UpdatePatch(&kp_lapsticker, "K_STLAPS");
	HU_UpdatePatch(&kp_lapstickerwide, "K_STLAPW");
	HU_UpdatePatch(&kp_lapstickernarrow, "K_STLAPN");
	HU_UpdatePatch(&kp_splitlapflag, "K_SPTLAP");
	HU_UpdatePatch(&kp_bumpersticker, "K_STBALN");
	HU_UpdatePatch(&kp_bumperstickerwide, "K_STBALW");
	HU_UpdatePatch(&kp_capsulesticker, "K_STCAPN");
	HU_UpdatePatch(&kp_capsulestickerwide, "K_STCAPW");
	HU_UpdatePatch(&kp_karmasticker, "K_STKARM");
	HU_UpdatePatch(&kp_spheresticker, "K_STBSMT");
	HU_UpdatePatch(&kp_splitspheresticker, "K_SPBSMT");
	HU_UpdatePatch(&kp_splitkarmabomb, "K_SPTKRM");
	HU_UpdatePatch(&kp_timeoutsticker, "K_STTOUT");

	// Pre-start countdown bulbs
	sprintf(buffer, "K_BULBxx");
	for (i = 0; i < 15; i++)
	{
		buffer[6] = '0'+((i+1)/10);
		buffer[7] = '0'+((i+1)%10);
		HU_UpdatePatch(&kp_prestartbulb[i], "%s", buffer);
	}

	sprintf(buffer, "K_SBLBxx");
	for (i = 0; i < 15; i++)
	{
		buffer[6] = '0'+((i+1)/10);
		buffer[7] = '0'+((i+1)%10);
		HU_UpdatePatch(&kp_prestartbulb_split[i], "%s", buffer);
	}

	// Pre-start position letters
	HU_UpdatePatch(&kp_prestartletters[0], "K_PL_P");
	HU_UpdatePatch(&kp_prestartletters[1], "K_PL_O");
	HU_UpdatePatch(&kp_prestartletters[2], "K_PL_S");
	HU_UpdatePatch(&kp_prestartletters[3], "K_PL_I");
	HU_UpdatePatch(&kp_prestartletters[4], "K_PL_T");
	HU_UpdatePatch(&kp_prestartletters[5], "K_PL_N");
	HU_UpdatePatch(&kp_prestartletters[6], "K_PL_EX");

	HU_UpdatePatch(&kp_prestartletters_split[0], "K_SPL_P");
	HU_UpdatePatch(&kp_prestartletters_split[1], "K_SPL_O");
	HU_UpdatePatch(&kp_prestartletters_split[2], "K_SPL_S");
	HU_UpdatePatch(&kp_prestartletters_split[3], "K_SPL_I");
	HU_UpdatePatch(&kp_prestartletters_split[4], "K_SPL_T");
	HU_UpdatePatch(&kp_prestartletters_split[5], "K_SPL_N");
	HU_UpdatePatch(&kp_prestartletters_split[6], "K_SPL_EX");

	// Starting countdown
	HU_UpdatePatch(&kp_startcountdown[0], "K_CNT3A");
	HU_UpdatePatch(&kp_startcountdown[1], "K_CNT2A");
	HU_UpdatePatch(&kp_startcountdown[2], "K_CNT1A");
	HU_UpdatePatch(&kp_startcountdown[3], "K_CNTGOA");
	HU_UpdatePatch(&kp_startcountdown[4], "K_DUEL1");
	HU_UpdatePatch(&kp_startcountdown[5], "K_CNT3B");
	HU_UpdatePatch(&kp_startcountdown[6], "K_CNT2B");
	HU_UpdatePatch(&kp_startcountdown[7], "K_CNT1B");
	HU_UpdatePatch(&kp_startcountdown[8], "K_CNTGOB");
	HU_UpdatePatch(&kp_startcountdown[9], "K_DUEL2");
	// Splitscreen
	HU_UpdatePatch(&kp_startcountdown[10], "K_SMC3A");
	HU_UpdatePatch(&kp_startcountdown[11], "K_SMC2A");
	HU_UpdatePatch(&kp_startcountdown[12], "K_SMC1A");
	HU_UpdatePatch(&kp_startcountdown[13], "K_SMCGOA");
	HU_UpdatePatch(&kp_startcountdown[14], "K_SDUEL1");
	HU_UpdatePatch(&kp_startcountdown[15], "K_SMC3B");
	HU_UpdatePatch(&kp_startcountdown[16], "K_SMC2B");
	HU_UpdatePatch(&kp_startcountdown[17], "K_SMC1B");
	HU_UpdatePatch(&kp_startcountdown[18], "K_SMCGOB");
	HU_UpdatePatch(&kp_startcountdown[19], "K_SDUEL2");

	// Fault
	HU_UpdatePatch(&kp_racefault[0], "K_FAULTA");
	HU_UpdatePatch(&kp_racefault[1], "K_FAULTB");
	// Splitscreen
	HU_UpdatePatch(&kp_racefault[2], "K_SMFLTA");
	HU_UpdatePatch(&kp_racefault[3], "K_SMFLTB");
	// 2P splitscreen
	HU_UpdatePatch(&kp_racefault[4], "K_2PFLTA");
	HU_UpdatePatch(&kp_racefault[5], "K_2PFLTB");

	// Finish
	HU_UpdatePatch(&kp_racefinish[0], "K_FINA");
	HU_UpdatePatch(&kp_racefinish[1], "K_FINB");
	// Splitscreen
	HU_UpdatePatch(&kp_racefinish[2], "K_SMFINA");
	HU_UpdatePatch(&kp_racefinish[3], "K_SMFINB");
	// 2P splitscreen
	HU_UpdatePatch(&kp_racefinish[4], "K_2PFINA");
	HU_UpdatePatch(&kp_racefinish[5], "K_2PFINB");

	// Position numbers
	sprintf(buffer, "KRNKxyz");
	for (i = 0; i < 10; i++)
	{
		buffer[6] = '0'+i;

		for (j = 0; j < 2; j++)
		{
			buffer[5] = 'A'+j;

			for (k = 0; k < 2; k++)
			{
				if (k > 0)
				{
					buffer[4] = 'S';
				}
				else
				{
					buffer[4] = 'B';
				}

				HU_UpdatePatch(&kp_positionnum[i][j][k], "%s", buffer);
			}
		}
	}

	sprintf(buffer, "OPPRNKxx");
	for (i = 0; i <= MAXPLAYERS; i++)
	{
		buffer[6] = '0'+(i/10);
		buffer[7] = '0'+(i%10);
		HU_UpdatePatch(&kp_facenum[i], "%s", buffer);
	}

	sprintf(buffer, "K_CHILIx");
	for (i = 0; i < 8; i++)
	{
		buffer[7] = '0'+(i+1);
		HU_UpdatePatch(&kp_facehighlight[i], "%s", buffer);
	}

	// Special minimap icons
	HU_UpdatePatch(&kp_nocontestminimap, "MINIDEAD");
	HU_UpdatePatch(&kp_unknownminimap, "HUHMAP");
	HU_UpdatePatch(&kp_spbminimap, "SPBMMAP");

	HU_UpdatePatch(&kp_wouldyoustillcatchmeifiwereaworm, "MINIPROG");
	HU_UpdatePatch(&kp_catcherminimap, "UFOMAP");
	HU_UpdatePatch(&kp_emeraldminimap[0], "EMEMAP");
	HU_UpdatePatch(&kp_emeraldminimap[1], "SUPMAP");

	HU_UpdatePatch(&kp_capsuleminimap[0], "MINICAP1");
	HU_UpdatePatch(&kp_capsuleminimap[1], "MINICAP2");
	HU_UpdatePatch(&kp_capsuleminimap[2], "MINICAP3");

	HU_UpdatePatch(&kp_battleufominimap, "MINIBUFO");
	HU_UpdatePatch(&kp_superflickyminimap, "FLKMAPA");

	// Rings & Lives
	HU_UpdatePatch(&kp_ringsticker[0], "RNGBACKA");
	HU_UpdatePatch(&kp_ringsticker[1], "RNGBACKB");

	sprintf(buffer, "K_RINGx");
	for (i = 0; i < 6; i++)
	{
		buffer[6] = '0'+(i+1);
		HU_UpdatePatch(&kp_ring[i], "%s", buffer);
	}

	HU_UpdatePatch(&kp_ringdebtminus, "RDEBTMIN");

	sprintf(buffer, "SPBRNGxx");
	for (i = 0; i < 16; i++)
	{
		buffer[6] = '0'+((i+1) / 10);
		buffer[7] = '0'+((i+1) % 10);
		HU_UpdatePatch(&kp_ringspblock[i], "%s", buffer);
	}

	HU_UpdatePatch(&kp_ringstickersplit[0], "SMRNGBGA");
	HU_UpdatePatch(&kp_ringstickersplit[1], "SMRNGBGB");

	sprintf(buffer, "K_SRINGx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '0'+(i+1);
		HU_UpdatePatch(&kp_smallring[i], "%s", buffer);
	}

	HU_UpdatePatch(&kp_ringdebtminussmall, "SRDEBTMN");

	sprintf(buffer, "SPBRGSxx");
	for (i = 0; i < 16; i++)
	{
		buffer[6] = '0'+((i+1) / 10);
		buffer[7] = '0'+((i+1) % 10);
		HU_UpdatePatch(&kp_ringspblocksmall[i], "%s", buffer);
	}

	// Speedometer
	HU_UpdatePatch(&kp_speedometersticker, "K_SPDMBG");

	sprintf(buffer, "K_SPDMLx");
	for (i = 0; i < 4; i++)
	{
		buffer[7] = '0'+(i+1);
		HU_UpdatePatch(&kp_speedometerlabel[i], "%s", buffer);
	}

	// Extra ranking icons
	HU_UpdatePatch(&kp_rankbumper, "K_BLNICO");
	HU_UpdatePatch(&kp_bigbumper, "K_BLNREG");
	HU_UpdatePatch(&kp_tinybumper[0], "K_BLNA");
	HU_UpdatePatch(&kp_tinybumper[1], "K_BLNB");
	HU_UpdatePatch(&kp_ranknobumpers, "K_NOBLNS");
	HU_UpdatePatch(&kp_rankcapsule, "K_CAPICO");
	HU_UpdatePatch(&kp_rankemerald, "K_EMERC");
	HU_UpdatePatch(&kp_rankemeraldflash, "K_EMERW");
	HU_UpdatePatch(&kp_rankemeraldback, "K_EMERBK");
	HU_UpdatePatch(&kp_pts[0], "K_POINTS");
	HU_UpdatePatch(&kp_pts[1], "K_POINT4");

	// Battle goal
	HU_UpdatePatch(&kp_goal[0][0], "K_ST1GLA");
	HU_UpdatePatch(&kp_goal[1][0], "K_ST1GLB");
	HU_UpdatePatch(&kp_goal[0][1], "K_ST4GLA");
	HU_UpdatePatch(&kp_goal[1][1], "K_ST4GLB");
	HU_UpdatePatch(&kp_goalrod[0], "K_ST1GLD");
	HU_UpdatePatch(&kp_goalrod[1], "K_ST4GLD");
	HU_UpdatePatch(&kp_goaltext1p, "K_ST1GLC");

	// Battle graphics
	HU_UpdatePatch(&kp_battlewin, "K_BWIN");
	HU_UpdatePatch(&kp_battlecool, "K_BCOOL");
	HU_UpdatePatch(&kp_battlelose, "K_BLOSE");
	HU_UpdatePatch(&kp_battlewait, "K_BWAIT");
	HU_UpdatePatch(&kp_battleinfo, "K_BINFO");
	HU_UpdatePatch(&kp_wanted, "K_WANTED");
	HU_UpdatePatch(&kp_wantedsplit, "4PWANTED");
	HU_UpdatePatch(&kp_wantedreticle, "MMAPWANT");
	HU_UpdatePatch(&kp_minimapdot, "MMAPDOT");

	// Kart Item Windows
	HU_UpdatePatch(&kp_itembg[0], "K_ITBG");
	HU_UpdatePatch(&kp_itembg[1], "K_ITBGD");
	HU_UpdatePatch(&kp_itemtimer[0], "K_ITIMER");
	HU_UpdatePatch(&kp_itemmulsticker[0], "K_ITMUL");
	HU_UpdatePatch(&kp_itemx, "K_ITX");

	HU_UpdatePatch(&kp_ringbg[0], "K_RBBG");
	HU_UpdatePatch(&kp_ringbg[1], "K_SBBG");

	HU_UpdatePatch(&kp_sadface[0], "K_ITSAD");
	HU_UpdatePatch(&kp_sneaker[0], "K_ITSHOE");
	HU_UpdatePatch(&kp_rocketsneaker[0], "K_ITRSHE");

	sprintf(buffer, "K_ITINVx");
	for (i = 0; i < 7; i++)
	{
		buffer[7] = '1'+i;
		HU_UpdatePatch(&kp_invincibility[i], "%s", buffer);
	}
	HU_UpdatePatch(&kp_banana[0], "K_ITBANA");
	HU_UpdatePatch(&kp_eggman[0], "K_ITEGGM");
	sprintf(buffer, "K_ITORBx");
	for (i = 0; i < 4; i++)
	{
		buffer[7] = '1'+i;
		HU_UpdatePatch(&kp_orbinaut[i], "%s", buffer);
	}
	HU_UpdatePatch(&kp_jawz[0], "K_ITJAWZ");
	HU_UpdatePatch(&kp_mine[0], "K_ITMINE");
	HU_UpdatePatch(&kp_landmine[0], "K_ITLNDM");
	HU_UpdatePatch(&kp_ballhog[0], "K_ITBHOG");
	HU_UpdatePatch(&kp_selfpropelledbomb[0], "K_ITSPB");
	HU_UpdatePatch(&kp_grow[0], "K_ITGROW");
	HU_UpdatePatch(&kp_shrink[0], "K_ITSHRK");
	HU_UpdatePatch(&kp_lightningshield[0], "K_ITTHNS");
	HU_UpdatePatch(&kp_bubbleshield[0], "K_ITBUBS");
	HU_UpdatePatch(&kp_flameshield[0], "K_ITFLMS");
	HU_UpdatePatch(&kp_hyudoro[0], "K_ITHYUD");
	HU_UpdatePatch(&kp_pogospring[0], "K_ITPOGO");
	HU_UpdatePatch(&kp_superring[0], "K_ITRING");
	HU_UpdatePatch(&kp_kitchensink[0], "K_ITSINK");
	HU_UpdatePatch(&kp_droptarget[0], "K_ITDTRG");
	HU_UpdatePatch(&kp_gardentop[0], "K_ITGTOP");
	HU_UpdatePatch(&kp_gachabom[0], "K_ITGBOM");
	HU_UpdatePatch(&kp_bar[0], "K_RBBAR");
	HU_UpdatePatch(&kp_doublebar[0], "K_RBBAR2");
	HU_UpdatePatch(&kp_triplebar[0], "K_RBBAR3");
	HU_UpdatePatch(&kp_slotring[0], "K_RBRING");
	HU_UpdatePatch(&kp_seven[0], "K_RBSEV");
	HU_UpdatePatch(&kp_jackpot[0], "K_RBJACK");

	sprintf(buffer, "FSMFGxxx");
	for (i = 0; i < FLAMESHIELD_MAX; i++)
	{
		buffer[5] = '0'+((i+1)/100);
		buffer[6] = '0'+(((i+1)/10)%10);
		buffer[7] = '0'+((i+1)%10);
		HU_UpdatePatch(&kp_flameshieldmeter[i][0], "%s", buffer);
	}

	sprintf(buffer, "FSMBGxxx");
	for (i = 0; i < FLAMESHIELD_MAX; i++)
	{
		buffer[5] = '0'+((i+1)/100);
		buffer[6] = '0'+(((i+1)/10)%10);
		buffer[7] = '0'+((i+1)%10);
		HU_UpdatePatch(&kp_flameshieldmeter_bg[i][0], "%s", buffer);
	}

	// Splitscreen
	HU_UpdatePatch(&kp_itembg[2], "K_ISBG");
	HU_UpdatePatch(&kp_itembg[3], "K_ISBGD");
	HU_UpdatePatch(&kp_itemtimer[1], "K_ISIMER");
	HU_UpdatePatch(&kp_itemmulsticker[1], "K_ISMUL");

	HU_UpdatePatch(&kp_sadface[1], "K_ISSAD");
	HU_UpdatePatch(&kp_sneaker[1], "K_ISSHOE");
	HU_UpdatePatch(&kp_rocketsneaker[1], "K_ISRSHE");
	sprintf(buffer, "K_ISINVx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '1'+i;
		HU_UpdatePatch(&kp_invincibility[i+7], "%s", buffer);
	}
	HU_UpdatePatch(&kp_banana[1], "K_ISBANA");
	HU_UpdatePatch(&kp_eggman[1], "K_ISEGGM");
	HU_UpdatePatch(&kp_orbinaut[4], "K_ISORBN");
	HU_UpdatePatch(&kp_jawz[1], "K_ISJAWZ");
	HU_UpdatePatch(&kp_mine[1], "K_ISMINE");
	HU_UpdatePatch(&kp_landmine[1], "K_ISLNDM");
	HU_UpdatePatch(&kp_ballhog[1], "K_ISBHOG");
	HU_UpdatePatch(&kp_selfpropelledbomb[1], "K_ISSPB");
	HU_UpdatePatch(&kp_grow[1], "K_ISGROW");
	HU_UpdatePatch(&kp_shrink[1], "K_ISSHRK");
	HU_UpdatePatch(&kp_lightningshield[1], "K_ISTHNS");
	HU_UpdatePatch(&kp_bubbleshield[1], "K_ISBUBS");
	HU_UpdatePatch(&kp_flameshield[1], "K_ISFLMS");
	HU_UpdatePatch(&kp_hyudoro[1], "K_ISHYUD");
	HU_UpdatePatch(&kp_pogospring[1], "K_ISPOGO");
	HU_UpdatePatch(&kp_superring[1], "K_ISRING");
	HU_UpdatePatch(&kp_kitchensink[1], "K_ISSINK");
	HU_UpdatePatch(&kp_droptarget[1], "K_ISDTRG");
	HU_UpdatePatch(&kp_gardentop[1], "K_ISGTOP");
	HU_UpdatePatch(&kp_gachabom[1], "K_ISGBOM");
	HU_UpdatePatch(&kp_bar[1], "K_SBBAR");
	HU_UpdatePatch(&kp_doublebar[1], "K_SBBAR2");
	HU_UpdatePatch(&kp_triplebar[1], "K_SBBAR3");
	HU_UpdatePatch(&kp_slotring[1], "K_SBRING");
	HU_UpdatePatch(&kp_seven[1], "K_SBSEV");
	HU_UpdatePatch(&kp_jackpot[1], "K_SBJACK");

	sprintf(buffer, "FSMFSxxx");
	for (i = 0; i < 120; i++)
	{
		buffer[5] = '0'+((i+1)/100);
		buffer[6] = '0'+(((i+1)/10)%10);
		buffer[7] = '0'+((i+1)%10);
		HU_UpdatePatch(&kp_flameshieldmeter[i][1], "%s", buffer);
	}

	sprintf(buffer, "FSMBS0xx");
	for (i = 0; i < 120; i++)
	{
		buffer[5] = '0'+((i+1)/100);
		buffer[6] = '0'+(((i+1)/10)%10);
		buffer[7] = '0'+((i+1)%10);
		HU_UpdatePatch(&kp_flameshieldmeter_bg[i][1], "%s", buffer);
	}

	// 4P item spy
	HU_UpdatePatch(&kp_itembg[4], "ISPYBG");
	HU_UpdatePatch(&kp_itembg[5], "ISPYBGD");

	//HU_UpdatePatch(&kp_sadface[2], "ISPYSAD");
	HU_UpdatePatch(&kp_sneaker[2], "ISPYSHOE");
	HU_UpdatePatch(&kp_rocketsneaker[2], "ISPYRSHE");
	sprintf(buffer, "ISPYINVx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '1'+i;
		HU_UpdatePatch(&kp_invincibility[i+13], "%s", buffer);
	}
	HU_UpdatePatch(&kp_banana[2], "ISPYBANA");
	HU_UpdatePatch(&kp_eggman[2], "ISPYEGGM");
	HU_UpdatePatch(&kp_orbinaut[5], "ISPYORBN");
	HU_UpdatePatch(&kp_jawz[2], "ISPYJAWZ");
	HU_UpdatePatch(&kp_mine[2], "ISPYMINE");
	HU_UpdatePatch(&kp_landmine[2], "ISPYLNDM");
	HU_UpdatePatch(&kp_ballhog[2], "ISPYBHOG");
	HU_UpdatePatch(&kp_selfpropelledbomb[2], "ISPYSPB");
	HU_UpdatePatch(&kp_grow[2], "ISPYGROW");
	HU_UpdatePatch(&kp_shrink[2], "ISPYSHRK");
	HU_UpdatePatch(&kp_lightningshield[2], "ISPYTHNS");
	HU_UpdatePatch(&kp_bubbleshield[2], "ISPYBUBS");
	HU_UpdatePatch(&kp_flameshield[2], "ISPYFLMS");
	HU_UpdatePatch(&kp_hyudoro[2], "ISPYHYUD");
	HU_UpdatePatch(&kp_pogospring[2], "ISPYPOGO");
	HU_UpdatePatch(&kp_superring[2], "ISPYRING");
	HU_UpdatePatch(&kp_kitchensink[2], "ISPYSINK");
	HU_UpdatePatch(&kp_droptarget[2], "ISPYDTRG");
	HU_UpdatePatch(&kp_gardentop[2], "ISPYGTOP");
	HU_UpdatePatch(&kp_gachabom[2], "ISPYGBOM");

	// CHECK indicators
	sprintf(buffer, "K_CHECKx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '1'+i;
		HU_UpdatePatch(&kp_check[i], "%s", buffer);
	}

	// Rival indicators
	sprintf(buffer, "K_RIVALx");
	for (i = 0; i < 2; i++)
	{
		buffer[7] = '1'+i;
		HU_UpdatePatch(&kp_rival[i], "%s", buffer);
	}

	// Rival indicators
	sprintf(buffer, "K_SSPLxx");
	for (i = 0; i < 4; i++)
	{
		buffer[6] = 'A'+i;
		for (j = 0; j < 2; j++)
		{
			buffer[7] = '1'+j;
			HU_UpdatePatch(&kp_localtag[i][j], "%s", buffer);
		}
	}

	// Typing indicator
	HU_UpdatePatch(&kp_talk, "K_TALK");
	HU_UpdatePatch(&kp_typdot, "K_TYPDOT");

	// Eggman warning numbers
	sprintf(buffer, "K_EGGNx");
	for (i = 0; i < 6; i++)
	{
		buffer[6] = '0'+i;
		HU_UpdatePatch(&kp_eggnum[i], "%s", buffer);
	}

	// First person mode
	HU_UpdatePatch(&kp_fpview[0], "VIEWA0");
	HU_UpdatePatch(&kp_fpview[1], "VIEWB0D0");
	HU_UpdatePatch(&kp_fpview[2], "VIEWC0E0");

	// Input UI Wheel
	sprintf(buffer, "K_WHEELx");
	for (i = 0; i < 5; i++)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_inputwheel[i], "%s", buffer);
	}

	// HERE COMES A NEW CHALLENGER
	sprintf(buffer, "K_CHALxx");
	for (i = 0; i < 25; i++)
	{
		buffer[6] = '0'+((i+1)/10);
		buffer[7] = '0'+((i+1)%10);
		HU_UpdatePatch(&kp_challenger[i], "%s", buffer);
	}

	// Lap start animation
	sprintf(buffer, "K_LAP0x");
	for (i = 0; i < 7; i++)
	{
		buffer[6] = '0'+(i+1);
		HU_UpdatePatch(&kp_lapanim_lap[i], "%s", buffer);
	}

	sprintf(buffer, "K_LAPFxx");
	for (i = 0; i < 11; i++)
	{
		buffer[6] = '0'+((i+1)/10);
		buffer[7] = '0'+((i+1)%10);
		HU_UpdatePatch(&kp_lapanim_final[i], "%s", buffer);
	}

	sprintf(buffer, "K_LAPNxx");
	for (i = 0; i < 10; i++)
	{
		buffer[6] = '0'+i;
		for (j = 0; j < 3; j++)
		{
			buffer[7] = '0'+(j+1);
			HU_UpdatePatch(&kp_lapanim_number[i][j], "%s", buffer);
		}
	}

	sprintf(buffer, "K_LAPE0x");
	for (i = 0; i < 2; i++)
	{
		buffer[7] = '0'+(i+1);
		HU_UpdatePatch(&kp_lapanim_emblem[i], "%s", buffer);
	}

	sprintf(buffer, "K_LAPH0x");
	for (i = 0; i < 3; i++)
	{
		buffer[7] = '0'+(i+1);
		HU_UpdatePatch(&kp_lapanim_hand[i], "%s", buffer);
	}

	HU_UpdatePatch(&kp_yougotem, "YOUGOTEM");
	HU_UpdatePatch(&kp_itemminimap, "MMAPITEM");

	sprintf(buffer, "ALAGLESx");
	for (i = 0; i < 10; ++i)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_alagles[i], "%s", buffer);
	}

	sprintf(buffer, "BLAGLESx");
	for (i = 0; i < 6; ++i)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_blagles[i], "%s", buffer);
	}

	HU_UpdatePatch(&kp_cpu, "K_CPU");

	HU_UpdatePatch(&kp_nametagstem, "K_NAMEST");

	HU_UpdatePatch(&kp_trickcool[0], "K_COOL1");
	HU_UpdatePatch(&kp_trickcool[1], "K_COOL2");

	HU_UpdatePatch(&kp_autoroulette, "A11YITEM");
	HU_UpdatePatch(&kp_autoring, "A11YRING");

	sprintf(buffer, "K_BOSB0x");
	for (i = 0; i < 8; i++)
	{
		buffer[7] = '0'+((i+1)%10);
		HU_UpdatePatch(&kp_bossbar[i], "%s", buffer);
	}

	sprintf(buffer, "K_BOSR0x");
	for (i = 0; i < 4; i++)
	{
		buffer[7] = '0'+((i+1)%10);
		HU_UpdatePatch(&kp_bossret[i], "%s", buffer);
	}

	sprintf(buffer, "HCAPARxx");
	for (i = 0; i < 2; i++)
	{
		buffer[6] = 'A'+i;

		for (j = 0; j < 2; j++)
		{
			buffer[7] = '0'+j;
			HU_UpdatePatch(&kp_capsuletarget_arrow[i][j], "%s", buffer);
		}
	}

	sprintf(buffer, "HUDCAPDx");
	for (i = 0; i < 2; i++)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_capsuletarget_far_text[i], "%s", buffer);
	}

	sprintf(buffer, "HUDCAPCx");
	for (i = 0; i < 2; i++)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_capsuletarget_icon[i], "%s", buffer);
	}

	sprintf(buffer, "HUDCAPBx");
	for (i = 0; i < 2; i++)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_capsuletarget_far[0][i], "%s", buffer);
	}

	sprintf(buffer, "HUDC4PBx");
	for (i = 0; i < 2; i++)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_capsuletarget_far[1][i], "%s", buffer);
	}

	sprintf(buffer, "HUDCAPAx");
	for (i = 0; i < 8; i++)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_capsuletarget_near[0][i], "%s", buffer);
	}

	sprintf(buffer, "HUDC4PAx");
	for (i = 0; i < 8; i++)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_capsuletarget_near[1][i], "%s", buffer);
	}

	sprintf(buffer, "HUDFLKAx");
	for (i = 0; i < 4; i++)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_superflickytarget[0][i], "%s", buffer);
	}

	sprintf(buffer, "H4PFLKAx");
	for (i = 0; i < 4; i++)
	{
		buffer[7] = '0'+i;
		HU_UpdatePatch(&kp_superflickytarget[1][i], "%s", buffer);
	}

	sprintf(buffer, "SPCNBFAx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '1'+i;
		HU_UpdatePatch(&kp_spraycantarget_far[0][i], "%s", buffer);
	}

	sprintf(buffer, "SPCNSFAx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '1'+i;
		HU_UpdatePatch(&kp_spraycantarget_far[1][i], "%s", buffer);
	}

	sprintf(buffer, "SPCNBCLx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '1'+i;
		HU_UpdatePatch(&kp_spraycantarget_near[0][i], "%s", buffer);
	}

	sprintf(buffer, "SPCNSCLx");
	for (i = 0; i < 6; i++)
	{
		buffer[7] = '1'+i;
		HU_UpdatePatch(&kp_spraycantarget_near[1][i], "%s", buffer);
	}

	K_LoadButtonGraphics(kp_button_a[0], 'A');
	K_LoadButtonGraphics(kp_button_a[1], 'N');
	K_LoadButtonGraphics(kp_button_b[0], 'B');
	K_LoadButtonGraphics(kp_button_b[1], 'O');
	K_LoadButtonGraphics(kp_button_c[0], 'C');
	K_LoadButtonGraphics(kp_button_c[1], 'P');
	K_LoadButtonGraphics(kp_button_x[0], 'D');
	K_LoadButtonGraphics(kp_button_x[1], 'Q');
	K_LoadButtonGraphics(kp_button_y[0], 'E');
	K_LoadButtonGraphics(kp_button_y[1], 'R');
	K_LoadButtonGraphics(kp_button_z[0], 'F');
	K_LoadButtonGraphics(kp_button_z[1], 'S');
	K_LoadButtonGraphics(kp_button_start, 'G');
	K_LoadButtonGraphics(kp_button_l, 'H');
	K_LoadButtonGraphics(kp_button_r, 'I');
	K_LoadButtonGraphics(kp_button_up, 'J');
	K_LoadButtonGraphics(kp_button_down, 'K');
	K_LoadButtonGraphics(kp_button_right, 'L');
	K_LoadButtonGraphics(kp_button_left, 'M');
	K_LoadButtonGraphics(kp_button_dpad, 'T');
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
		case KITEM_LANDMINE:
			return (tiny ? "K_ISLNDM" : "K_ITLNDM");
		case KITEM_BALLHOG:
			return (tiny ? "K_ISBHOG" : "K_ITBHOG");
		case KITEM_SPB:
			return (tiny ? "K_ISSPB" : "K_ITSPB");
		case KITEM_GROW:
			return (tiny ? "K_ISGROW" : "K_ITGROW");
		case KITEM_SHRINK:
			return (tiny ? "K_ISSHRK" : "K_ITSHRK");
		case KITEM_LIGHTNINGSHIELD:
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
		case KITEM_DROPTARGET:
			return (tiny ? "K_ISDTRG" : "K_ITDTRG");
		case KITEM_GARDENTOP:
			return (tiny ? "K_ISGTOP" : "K_ITGTOP");
		case KITEM_GACHABOM:
		case KRITEM_TRIPLEGACHABOM:
			return (tiny ? "K_ISGBOM" : "K_ITGBOM");
		case KRITEM_TRIPLEORBINAUT:
			return (tiny ? "K_ISORBN" : "K_ITORB3");
		case KRITEM_QUADORBINAUT:
			return (tiny ? "K_ISORBN" : "K_ITORB4");
		default:
			return (tiny ? "K_ISSAD" : "K_ITSAD");
	}
}

static patch_t *K_GetCachedItemPatch(INT32 item, UINT8 offset)
{
	patch_t **kp[1 + NUMKARTITEMS] = {
		kp_sadface,
		NULL,
		kp_sneaker,
		kp_rocketsneaker,
		kp_invincibility,
		kp_banana,
		kp_eggman,
		kp_orbinaut,
		kp_jawz,
		kp_mine,
		kp_landmine,
		kp_ballhog,
		kp_selfpropelledbomb,
		kp_grow,
		kp_shrink,
		kp_lightningshield,
		kp_bubbleshield,
		kp_flameshield,
		kp_hyudoro,
		kp_pogospring,
		kp_superring,
		kp_kitchensink,
		kp_droptarget,
		kp_gardentop,
		kp_gachabom,
	};

	if (item == KITEM_SAD || (item > KITEM_NONE && item < NUMKARTITEMS))
		return kp[item - KITEM_SAD][offset];
	else
		return NULL;
}

static patch_t *K_GetSmallStaticCachedItemPatch(kartitems_t item)
{
	UINT8 offset;

	item = static_cast<kartitems_t>(K_ItemResultToType(item));

	switch (item)
	{
		case KITEM_INVINCIBILITY:
			offset = 7;
			break;

		case KITEM_ORBINAUT:
			offset = 4;
			break;

		default:
			offset = 1;
	}

	return K_GetCachedItemPatch(item, offset);
}

static patch_t *K_GetCachedSlotMachinePatch(INT32 item, UINT8 offset)
{
	patch_t **kp[KSM__MAX] = {
		kp_bar,
		kp_doublebar,
		kp_triplebar,
		kp_slotring,
		kp_seven,
		kp_jackpot,
	};

	if (item >= 0 && item < KSM__MAX)
		return kp[item][offset];
	else
		return NULL;
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

// trick "cool"
INT32 TCOOL_X, TCOOL_Y;

// This version of the function was prototyped in Lua by Nev3r ... a HUGE thank you goes out to them!
void K_ObjectTracking(trackingResult_t *result, const vector3_t *point, boolean reverse)
{
#define NEWTAN(x) FINETANGENT(((x + ANGLE_90) >> ANGLETOFINESHIFT) & 4095) // tan function used by Lua
#define NEWCOS(x) FINECOSINE((x >> ANGLETOFINESHIFT) & FINEMASK)

	angle_t viewpointAngle, viewpointAiming, viewpointRoll;

	INT32 screenWidth, screenHeight;
	fixed_t screenHalfW, screenHalfH;

	const fixed_t baseFov = 90*FRACUNIT;
	fixed_t fovDiff, fov, fovTangent, fg;

	fixed_t h;
	INT32 da, dp;

	UINT8 cameraNum = R_GetViewNumber();

	I_Assert(result != NULL);
	I_Assert(point != NULL);

	// Initialize defaults
	result->x = result->y = 0;
	result->scale = FRACUNIT;
	result->onScreen = false;

	// Take the view's properties as necessary.
	if (reverse)
	{
		viewpointAngle = (INT32)(viewangle + ANGLE_180);
		viewpointAiming = (INT32)InvAngle(aimingangle);
		viewpointRoll = (INT32)viewroll;
	}
	else
	{
		viewpointAngle = (INT32)viewangle;
		viewpointAiming = (INT32)aimingangle;
		viewpointRoll = (INT32)InvAngle(viewroll);
	}

	// Calculate screen size adjustments.
	screenWidth = vid.width/vid.dupx;
	screenHeight = vid.height/vid.dupy;

	if (r_splitscreen >= 2)
	{
		// Half-wide screens
		screenWidth >>= 1;
	}

	if (r_splitscreen >= 1)
	{
		// Half-tall screens
		screenHeight >>= 1;
	}

	screenHalfW = (screenWidth >> 1) << FRACBITS;
	screenHalfH = (screenHeight >> 1) << FRACBITS;

	// Calculate FOV adjustments.
	fovDiff = R_FOV(cameraNum) - baseFov;
	fov = ((baseFov - fovDiff) / 2) - (stplyr->fovadd / 2);
	fovTangent = NEWTAN(FixedAngle(fov));

	if (r_splitscreen == 1)
	{
		// Splitscreen FOV is adjusted to maintain expected vertical view
		fovTangent = 10*fovTangent/17;
	}

	fg = (screenWidth >> 1) * fovTangent;

	// Determine viewpoint factors.
	h = R_PointToDist2(point->x, point->y, viewx, viewy);
	da = AngleDeltaSigned(viewpointAngle, R_PointToAngle2(viewx, viewy, point->x, point->y));
	dp = AngleDeltaSigned(viewpointAiming, R_PointToAngle2(0, 0, h, viewz));

	if (reverse)
	{
		da = -(da);
	}

	// Set results relative to top left!
	result->x = FixedMul(NEWTAN(da), fg);
	result->y = FixedMul((NEWTAN(viewpointAiming) - FixedDiv((point->z - viewz), 1 + FixedMul(NEWCOS(da), h))), fg);

	result->angle = da;
	result->pitch = dp;
	result->fov = fg;

	// Rotate for screen roll...
	if (viewpointRoll)
	{
		fixed_t tempx = result->x;
		viewpointRoll >>= ANGLETOFINESHIFT;
		result->x = FixedMul(FINECOSINE(viewpointRoll), tempx) - FixedMul(FINESINE(viewpointRoll), result->y);
		result->y = FixedMul(FINESINE(viewpointRoll), tempx) + FixedMul(FINECOSINE(viewpointRoll), result->y);
	}

	// Flipped screen?
	if (encoremode)
	{
		result->x = -result->x;
	}

	// Center results.
	result->x += screenHalfW;
	result->y += screenHalfH;

	result->scale = FixedDiv(screenHalfW, h+1);

	result->onScreen = !((abs(da) > ANG60) || (abs(AngleDeltaSigned(viewpointAiming, R_PointToAngle2(0, 0, h, (viewz - point->z)))) > ANGLE_45));

	// Cheap dirty hacks for some split-screen related cases
	if (result->x < 0 || result->x > (screenWidth << FRACBITS))
	{
		result->onScreen = false;
	}

	if (result->y < 0 || result->y > (screenHeight << FRACBITS))
	{
		result->onScreen = false;
	}

	// adjust to non-green-resolution screen coordinates
	result->x -= ((vid.width/vid.dupx) - BASEVIDWIDTH)<<(FRACBITS-((r_splitscreen >= 2) ? 2 : 1));
	result->y -= ((vid.height/vid.dupy) - BASEVIDHEIGHT)<<(FRACBITS-((r_splitscreen >= 1) ? 2 : 1));

	return;

#undef NEWTAN
#undef NEWCOS
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

	// trick COOL
	TCOOL_X = (BASEVIDWIDTH)/2;
	TCOOL_Y = (BASEVIDHEIGHT)/2 -10;

	if (r_splitscreen)	// Splitscreen
	{
		ITEM_X = 5;
		ITEM_Y = 3;

		LAPS_Y = (BASEVIDHEIGHT/2)-24;

		POSI_Y = (BASEVIDHEIGHT/2)- 2;

		STCD_Y = BASEVIDHEIGHT/4;

		MINI_X -= 16;
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

			TCOOL_X = (BASEVIDWIDTH)/4;

			if (r_splitscreen > 2) // 4P-only
			{
				MINI_X = (BASEVIDWIDTH/2);
				MINI_Y = (BASEVIDHEIGHT/2);
			}
		}
	}
}

void K_DrawMapThumbnail(fixed_t x, fixed_t y, fixed_t width, UINT32 flags, UINT16 map, const UINT8 *colormap)
{
	patch_t *PictureOfLevel = NULL;

	if (map >= nummapheaders || !mapheaderinfo[map])
	{
		PictureOfLevel = nolvl;
	}
	else if (!mapheaderinfo[map]->thumbnailPic)
	{
		PictureOfLevel = blanklvl;
	}
	else
	{
		PictureOfLevel = static_cast<patch_t*>(mapheaderinfo[map]->thumbnailPic);
	}

	K_DrawLikeMapThumbnail(x, y, width, flags, PictureOfLevel, colormap);
}

void K_DrawLikeMapThumbnail(fixed_t x, fixed_t y, fixed_t width, UINT32 flags, patch_t *patch, const UINT8 *colormap)
{
	if (flags & V_FLIP)
		x += width;

	V_DrawFixedPatch(
		x, y,
		FixedDiv(width, (320 << FRACBITS)),
		flags,
		patch,
		colormap
	);
}

// see also K_DrawNameTagItemSpy
static void K_drawKartItem(void)
{
	// ITEM_X = BASEVIDWIDTH-50;	// 270
	// ITEM_Y = 24;					//  24

	// Why write V_DrawScaledPatch calls over and over when they're all the same?
	// Set to 'no item' just in case.
	const UINT8 offset = ((r_splitscreen > 1) ? 1 : 0);
	patch_t *localpatch[3] = { kp_nodraw, kp_nodraw, kp_nodraw };
	UINT8 localamt[3] = {0, 0, 0};
	patch_t *localbg = ((offset) ? kp_itembg[2] : kp_itembg[0]);
	patch_t *localinv = ((offset) ? kp_invincibility[((leveltime % (6*3)) / 3) + 7] : kp_invincibility[(leveltime % (7*3)) / 3]);
	INT32 fx = 0, fy = 0, fflags = 0;	// final coords for hud and flags...
	const INT32 numberdisplaymin = ((!offset && stplyr->itemtype == KITEM_ORBINAUT) ? 5 : 2);
	INT32 itembar = 0;
	INT32 maxl = 0; // itembar's normal highest value
	const INT32 barlength = (offset ? 12 : 26);
	skincolornum_t localcolor[3] = { static_cast<skincolornum_t>(stplyr->skincolor) };
	SINT8 colormode[3] = { TC_RAINBOW };
	boolean flipamount = false;	// Used for 3P/4P splitscreen to flip item amount stuff

	fixed_t rouletteOffset = 0;
	fixed_t rouletteSpace = ROULETTE_SPACING;
	vector2_t rouletteCrop = {7, 7};
	INT32 i;

	boolean flashOnOne = false;
	boolean flashOnTwo = false;

	if (stplyr->itemRoulette.itemListLen > 0)
	{
		// Init with item roulette stuff.
		for (i = 0; i < 3; i++)
		{
			const SINT8 indexOfs = i-1;
			const size_t index = (stplyr->itemRoulette.itemListLen + (stplyr->itemRoulette.index + indexOfs)) % stplyr->itemRoulette.itemListLen;

			const SINT8 result = stplyr->itemRoulette.itemList[index];
			const SINT8 item = K_ItemResultToType(result);
			const boolean usingDebugItemAmount = cv_kartdebugitem.value != KITEM_NONE && cv_kartdebugitem.value == item && cv_kartdebugamount.value > 1;
			const UINT8 amt = usingDebugItemAmount ? cv_kartdebugamount.value : K_ItemResultToAmount(result);

			switch (item)
			{
				case KITEM_INVINCIBILITY:
					localpatch[i] = localinv;
					localamt[i] = amt;
					break;

				case KITEM_ORBINAUT:
					localpatch[i] = kp_orbinaut[(offset ? 4 : std::min(amt-1, 3))];
					if (amt > 4)
						localamt[i] = amt;
					break;

				default:
					localpatch[i] = K_GetCachedItemPatch(item, offset);
					if (item != KITEM_BALLHOG || amt != 5)
						localamt[i] = amt;
					break;
			}
		}
	}

	if (stplyr->itemRoulette.active == true)
	{
		rouletteOffset = K_GetRouletteOffset(&stplyr->itemRoulette, rendertimefrac, 0);
	}
	else
	{
		// I'm doing this a little weird and drawing mostly in reverse order
		// The only actual reason is to make sneakers line up this way in the code below
		// This shouldn't have any actual baring over how it functions
		// Hyudoro is first, because we're drawing it on top of the player's current item

		localcolor[1] = SKINCOLOR_NONE;
		rouletteOffset = stplyr->karthud[khud_rouletteoffset];

		if (stplyr->stealingtimer < 0)
		{
			localpatch[1] = kp_hyudoro[offset];
			flashOnTwo = true;
		}
		else if ((stplyr->stealingtimer > 0) && (leveltime & 2))
		{
			localpatch[1] = kp_hyudoro[offset];
		}
		else if (stplyr->eggmanexplode > 1)
		{
			localpatch[1] = kp_eggman[offset];
			flashOnOne = true;
		}
		else if (stplyr->ballhogcharge > 0)
		{
			// itembar = stplyr->ballhogcharge;
			// maxl = (((stplyr->itemamount-1) * BALLHOGINCREMENT) + 1);

			itembar = stplyr->ballhogcharge % BALLHOGINCREMENT;
			maxl = BALLHOGINCREMENT;

			localpatch[1] = kp_ballhog[offset];
			flashOnOne = true;
		}
		else if (stplyr->rocketsneakertimer > 1)
		{
			itembar = stplyr->rocketsneakertimer;
			maxl = (itemtime*3) - barlength;

			localpatch[1] = kp_rocketsneaker[offset];
			flashOnOne = true;
		}
		else if (stplyr->sadtimer > 0)
		{
			localpatch[1] = kp_sadface[offset];
			flashOnTwo = true;
		}
		else if (stplyr->itemRoulette.reserved > 0)
		{
			localpatch[1] = kp_nodraw;
		}
		else
		{
			if (stplyr->itemamount <= 0)
				return;

			switch (stplyr->itemtype)
			{
				case KITEM_INVINCIBILITY:
					localpatch[1] = localinv;
					localbg = kp_itembg[offset+1];
					break;

				case KITEM_ORBINAUT:
					localpatch[1] = kp_orbinaut[(offset ? 4 : std::min(stplyr->itemamount-1, 3))];
					break;

				case KITEM_SPB:
				case KITEM_LIGHTNINGSHIELD:
				case KITEM_BUBBLESHIELD:
				case KITEM_FLAMESHIELD:
					localbg = kp_itembg[offset+1];
					/*FALLTHRU*/

				default:
					localpatch[1] = K_GetCachedItemPatch(stplyr->itemtype, offset);

					if (localpatch[1] == NULL)
						localpatch[1] = kp_nodraw; // diagnose underflows
					break;
			}

			if ((stplyr->itemflags & IF_ITEMOUT))
				flashOnOne = true;
		}

		if (!cv_reducevfx.value)
		{
			if (flashOnOne && !(leveltime & 1))
				localpatch[1] = kp_nodraw;
			else if (flashOnTwo && !(leveltime & 2))
				localpatch[1] = kp_nodraw;
		}

		if (stplyr->karthud[khud_itemblink] && (leveltime & 1) && !(cv_reducevfx.value))
		{
			colormode[1] = TC_BLINK;

			switch (stplyr->karthud[khud_itemblinkmode])
			{
				case 2:
					localcolor[1] = static_cast<skincolornum_t>(K_RainbowColor(leveltime));
					break;
				case 1:
					localcolor[1] = SKINCOLOR_RED;
					break;
				default:
					localcolor[1] = SKINCOLOR_WHITE;
					break;
			}
		}
		else
		{
			// Hide the other items.
			// Effectively lets the other roulette items
			// show flicker away after you select.
			localpatch[0] = localpatch[2] = kp_nodraw;
		}
	}

	// pain and suffering defined below
	if (offset)
	{
		if (!(R_GetViewNumber() & 1)) // If we are P1 or P3...
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

		rouletteSpace = ROULETTE_SPACING_SPLITSCREEN;
		rouletteOffset = FixedMul(rouletteOffset, FixedDiv(ROULETTE_SPACING_SPLITSCREEN, ROULETTE_SPACING));
		rouletteCrop.x = 16;
		rouletteCrop.y = 15;
	}
	else
	{
		fx = ITEM_X;
		fy = ITEM_Y;
		fflags = V_SNAPTOTOP|V_SNAPTOLEFT|V_SPLITSCREEN;
	}

	if (r_splitscreen == 1)
	{
		fy -= 5;
	}

	V_DrawScaledPatch(fx, fy, V_HUDTRANS|V_SLIDEIN|fflags, localbg);

	// Need to draw these in a particular order, for sorting.
	V_SetClipRect(
		(fx + rouletteCrop.x) << FRACBITS, (fy + rouletteCrop.y) << FRACBITS,
		rouletteSpace, rouletteSpace,
		V_SLIDEIN|fflags
	);

	auto draw_item = [&](fixed_t y, int i)
	{
		const UINT8 *colormap = (localcolor[i] ? R_GetTranslationColormap(colormode[i], localcolor[i], GTC_CACHE) : NULL);
		V_DrawFixedPatch(
			fx<<FRACBITS, (fy<<FRACBITS) + rouletteOffset + y,
			FRACUNIT, V_HUDTRANS|V_SLIDEIN|fflags,
			localpatch[i], colormap
		);
		if (localamt[i] > 1)
		{
			using srb2::Draw;
			Draw(
				fx + rouletteCrop.x + FixedToFloat(rouletteSpace/2),
				fy + rouletteCrop.y + FixedToFloat(rouletteOffset + y + rouletteSpace) - (r_splitscreen > 1 ? 15 : 33))
				.font(r_splitscreen > 1 ? Draw::Font::kRollingNum4P : Draw::Font::kRollingNum)
				.align(Draw::Align::kCenter)
				.flags(V_HUDTRANS|V_SLIDEIN|fflags)
				.colormap(colormap)
				.text("{}", localamt[i]);
		}
	};

	draw_item(rouletteSpace, 0);
	draw_item(-rouletteSpace, 2);

	if (stplyr->itemRoulette.active == true)
	{
		// Draw the item underneath the box.
		draw_item(0, 1);
		V_ClearClipRect();
	}
	else
	{
		// Draw the item above the box.
		V_ClearClipRect();

		// A little goofy, but helps with ballhog charge conveyance—you're "loading" them.
		UINT8 fakeitemamount = stplyr->itemamount - (stplyr->ballhogcharge / BALLHOGINCREMENT);

		boolean transflag = V_HUDTRANS;

		if (cv_reducevfx.value && (flashOnOne || flashOnTwo))
		{
			transflag = V_HUDTRANSHALF;
		}

		if (fakeitemamount >= numberdisplaymin && stplyr->itemRoulette.active == false)
		{
			// Then, the numbers:
			V_DrawScaledPatch(
				fx + (flipamount ? 48 : 0), fy,
				V_HUDTRANS|V_SLIDEIN|fflags|(flipamount ? V_FLIP : 0),
				kp_itemmulsticker[offset]
			); // flip this graphic for p2 and p4 in split and shift it.

			V_DrawFixedPatch(
				fx<<FRACBITS, (fy<<FRACBITS) + rouletteOffset,
				FRACUNIT, transflag|V_SLIDEIN|fflags,
				localpatch[1], (localcolor[1] ? R_GetTranslationColormap(colormode[1], localcolor[1], GTC_CACHE) : NULL)
			);

			if (offset)
			{
				if (flipamount) // reminder that this is for 3/4p's right end of the screen.
					V_DrawString(fx+2, fy+31, V_HUDTRANS|V_SLIDEIN|fflags, va("x%d", fakeitemamount));
				else
					V_DrawString(fx+24, fy+31, V_HUDTRANS|V_SLIDEIN|fflags, va("x%d", fakeitemamount));
			}
			else
			{
				V_DrawScaledPatch(fy+28, fy+41, V_HUDTRANS|V_SLIDEIN|fflags, kp_itemx);
				V_DrawTimerString(fx+38, fy+36, V_HUDTRANS|V_SLIDEIN|fflags, va("%d", fakeitemamount));
			}
		}
		else
		{
			V_DrawFixedPatch(
				fx<<FRACBITS, (fy<<FRACBITS) + rouletteOffset,
				FRACUNIT, transflag|V_SLIDEIN|fflags,
				localpatch[1], (localcolor[1] ? R_GetTranslationColormap(colormode[1], localcolor[1], GTC_CACHE) : NULL)
			);
		}
	}

	// Extensible meter, currently only used for rocket sneaker...
	if (itembar)
	{
		const INT32 fill = ((itembar*barlength)/maxl);
		const INT32 length = std::min(barlength, fill);
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
	if (stplyr->eggmanexplode > 1)
		V_DrawScaledPatch(fx+17, fy+13-offset, V_HUDTRANS|V_SLIDEIN|fflags, kp_eggnum[std::min<INT32>(5, G_TicsToSeconds(stplyr->eggmanexplode))]);

	if (stplyr->itemtype == KITEM_FLAMESHIELD && stplyr->flamelength > 0)
	{
		INT32 numframes = FLAMESHIELD_MAX;
		INT32 absolutemax = numframes;
		INT32 flamemax = stplyr->flamelength;
		INT32 flamemeter = std::min(static_cast<INT32>(stplyr->flamemeter), flamemax);

		INT32 bf = numframes - stplyr->flamelength;
		INT32 ff = numframes - ((flamemeter * numframes) / absolutemax);

		INT32 xo = 6, yo = 4;
		INT32 flip = 0;

		if (offset)
		{
			xo++;

			if (!(R_GetViewNumber() & 1)) // Flip for P1 and P3 (yes, that's correct)
			{
				xo -= 62;
				flip = V_FLIP;
			}
		}

		/*
		INT32 fmin = (8 * (bf-1));
		if (ff < fmin)
			ff = fmin;
		*/

		if (bf >= 0 && bf < numframes)
			V_DrawScaledPatch(fx-xo, fy-yo, V_HUDTRANS|V_SLIDEIN|fflags|flip, kp_flameshieldmeter_bg[bf][offset]);

		if (ff >= 0 && ff < numframes && stplyr->flamemeter > 0)
		{
			if ((stplyr->flamemeter > flamemax) && (leveltime & 1))
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

static void K_drawKartSlotMachine(void)
{
	// ITEM_X = BASEVIDWIDTH-50;	// 270
	// ITEM_Y = 24;					//  24

	// Why write V_DrawScaledPatch calls over and over when they're all the same?
	// Set to 'no item' just in case.
	const UINT8 offset = ((r_splitscreen > 1) ? 1 : 0);

	patch_t *localpatch[3] = { kp_nodraw, kp_nodraw, kp_nodraw };
	patch_t *localbg = offset ? kp_ringbg[1] : kp_ringbg[0];

	// == SHITGARBAGE UNLIMITED 2: RISE OF MY ASS ==
	// FIVE LAYERS OF BULLSHIT PER-PIXEL SHOVING BECAUSE THE PATCHES HAVE DIFFERENT OFFSETS
	// IF YOU ARE HERE TO ADJUST THE RINGBOX HUD TURN OFF YOUR COMPUTER AND GO TO YOUR LOCAL PARK

	INT32 fx = 0, fy = 0, fflags = 0;	// final coords for hud and flags...
	INT32 boxoffx = 0;
	INT32 boxoffy = -6;
	INT32 vstretch = 0;
	INT32 hstretch = 3;
	INT32 splitbsx = 0, splitbsy = 0;
	skincolornum_t localcolor[3] = { static_cast<skincolornum_t>(stplyr->skincolor) };
	SINT8 colormode[3] = { TC_RAINBOW };

	fixed_t rouletteOffset = 0;
	fixed_t rouletteSpace = SLOT_SPACING;
	vector2_t rouletteCrop = {10, 10};
	INT32 i;

	if (stplyr->itemRoulette.itemListLen > 0)
	{
		// Init with item roulette stuff.
		for (i = 0; i < 3; i++)
		{
			const SINT8 indexOfs = i-1;
			const size_t index = (stplyr->itemRoulette.itemListLen + (stplyr->itemRoulette.index + indexOfs)) % stplyr->itemRoulette.itemListLen;

			const SINT8 result = stplyr->itemRoulette.itemList[index];

			localpatch[i] = K_GetCachedSlotMachinePatch(result, offset);
		}
	}

	if (stplyr->itemRoulette.active == true)
	{
		rouletteOffset = K_GetSlotOffset(&stplyr->itemRoulette, rendertimefrac, 0);
	}
	else
	{
		rouletteOffset = stplyr->karthud[khud_rouletteoffset];

		if (!stplyr->ringboxdelay)
		{
			return;
		}
	}

	if (stplyr->karthud[khud_itemblink] && (leveltime & 1) && !cv_reducevfx.value)
	{
		colormode[1] = TC_BLINK;
		localcolor[1] = SKINCOLOR_WHITE;

		// This looks kinda wild with the white-background patch.
		/*
		switch (stplyr->ringboxaward)
		{
			case 5: // JACKPOT!
				localcolor[1] = K_RainbowColor(leveltime);
				break;
			default:
				localcolor[1] = SKINCOLOR_WHITE;
				break;
		}
		*/
	}

	// pain and suffering defined below
	if (offset)
	{
		boxoffx -= 4;
		if (!(R_GetViewNumber() & 1)) // If we are P1 or P3...
		{
			fx = ITEM_X + 10;
			fy = ITEM_Y + 10;
			fflags = V_SNAPTOLEFT|V_SNAPTOTOP|V_SPLITSCREEN;
		}
		else // else, that means we're P2 or P4.
		{
			fx = ITEM2_X + 7;
			fy = ITEM2_Y + 10;
			fflags = V_SNAPTORIGHT|V_SNAPTOTOP|V_SPLITSCREEN;
		}

		rouletteSpace = SLOT_SPACING_SPLITSCREEN;
		rouletteOffset = FixedMul(rouletteOffset, FixedDiv(SLOT_SPACING_SPLITSCREEN, SLOT_SPACING));
		rouletteCrop.x = 16;
		rouletteCrop.y = 13;
		splitbsx = -6;
		splitbsy = -6;
		boxoffy += 2;
		hstretch = 0;
	}
	else
	{
		fx = ITEM_X;
		fy = ITEM_Y;
		fflags = V_SNAPTOTOP|V_SNAPTOLEFT|V_SPLITSCREEN;
	}

	if (r_splitscreen == 1)
	{
		fy -= 5;
	}

	V_DrawScaledPatch(fx, fy, V_HUDTRANS|V_SLIDEIN|fflags, localbg);

	V_SetClipRect(
		((fx + rouletteCrop.x + boxoffx + splitbsx) << FRACBITS), ((fy + rouletteCrop.y + boxoffy - vstretch + splitbsy) << FRACBITS),
		rouletteSpace + (hstretch<<FRACBITS), rouletteSpace + (vstretch<<FRACBITS),
		V_SLIDEIN|fflags
	);

	// item box has special layering, transparency, different sized patches, other fucked up shit
	// ring box is evenly spaced and easy
	rouletteOffset += rouletteSpace;
	for (i = 0; i < 3; i++)
	{
		V_DrawFixedPatch(
			((fx)<<FRACBITS), ((fy)<<FRACBITS) + rouletteOffset,
			FRACUNIT, V_HUDTRANS|V_SLIDEIN|fflags,
			localpatch[i], (localcolor[i] ? R_GetTranslationColormap(colormode[i], localcolor[i], GTC_CACHE) : NULL)
		);

		rouletteOffset -= rouletteSpace;
	}

	V_ClearClipRect();
}

tic_t K_TranslateTimer(tic_t drawtime, UINT8 mode, INT32 *return_jitter)
{
	INT32 jitter = 0;

	if (!mode && drawtime != UINT32_MAX)
	{
		if (timelimitintics > 0)
		{
			if (drawtime >= timelimitintics)
			{
				jitter = 2;
				if (drawtime & 2)
					jitter = -jitter;
				drawtime = 0;
			}
			else
			{
				drawtime = timelimitintics - drawtime;
				if (secretextratime)
					;
				else if (extratimeintics)
				{
					jitter = 2;
					if (leveltime & 1)
						jitter = -jitter;
				}
				else if (drawtime <= 5*TICRATE)
				{
					jitter = ((drawtime <= 3*TICRATE) && (((drawtime-1) % TICRATE) >= TICRATE-2))
						? 3 : 1;
					if (drawtime & 2)
						jitter = -jitter;
				}
			}
		}
	}

	if (return_jitter)
	{
		*return_jitter = jitter;
	}

	return drawtime;
}

INT32 K_drawKartMicroTime(const char *todrawtext, INT32 workx, INT32 worky, INT32 splitflags)
{
	using srb2::Draw;
	Draw::TextElement text(todrawtext);
	text.flags(splitflags);
	text.font(Draw::Font::kZVote);

	INT32 result = text.width();
	Draw(workx - result, worky).text(text);

	return result;
}

void K_drawKartTimestamp(tic_t drawtime, INT32 TX, INT32 TY, INT32 splitflags, UINT8 mode)
{
	// TIME_X = BASEVIDWIDTH-124;	// 196
	// TIME_Y = 6;					//   6

	INT32 jitter = 0;

	drawtime = K_TranslateTimer(drawtime, mode, &jitter);

	V_DrawScaledPatch(TX, TY, splitflags, ((mode == 2) ? kp_lapstickerwide : kp_timestickerwide));

	TX += 33;

	if (drawtime == UINT32_MAX)
		;
	else if (mode && !drawtime)
	{
		// apostrophe location     _'__ __
		V_DrawTimerString(TX+24, TY+3, splitflags, va("'"));

		// quotation mark location    _ __"__
		V_DrawTimerString(TX+60, TY+3, splitflags, va("\""));
	}
	else
	{
		tic_t worktime = drawtime/(60*TICRATE);

		if (worktime >= 100)
		{
			jitter = (drawtime & 1 ? 1 : -1);
			worktime = 99;
			drawtime = (100*(60*TICRATE))-1;
		}

		// minutes time      00 __ __
		V_DrawTimerString(TX,    TY+3+jitter, splitflags, va("%d", worktime/10));
		V_DrawTimerString(TX+12, TY+3-jitter, splitflags, va("%d", worktime%10));

		// apostrophe location     _'__ __
		V_DrawTimerString(TX+24, TY+3, splitflags, va("'"));

		worktime = (drawtime/TICRATE % 60);

		// seconds time       _ 00 __
		V_DrawTimerString(TX+36, TY+3+jitter, splitflags, va("%d", worktime/10));
		V_DrawTimerString(TX+48, TY+3-jitter, splitflags, va("%d", worktime%10));

		// quotation mark location    _ __"__
		V_DrawTimerString(TX+60, TY+3, splitflags, va("\""));

		worktime = G_TicsToCentiseconds(drawtime);

		// tics               _ __ 00
		V_DrawTimerString(TX+72, TY+3+jitter, splitflags, va("%d", worktime/10));
		V_DrawTimerString(TX+84, TY+3-jitter, splitflags, va("%d", worktime%10));
	}

	// Medal data!
	if ((modeattacking || (mode == 1))
		&& !demo.playback)
	{
		INT32 workx = TX + 96, worky = TY+18;
		UINT8 i = stickermedalinfo.visiblecount;

		if (stickermedalinfo.targettext[0] != '\0')
		{
			if (!mode)
			{
				if (stickermedalinfo.jitter)
				{
					jitter = stickermedalinfo.jitter+3;
					if (jitter & 2)
						workx += jitter/4;
					else
						workx -= jitter/4;
				}

				if (stickermedalinfo.norecord == true)
				{
					splitflags = (splitflags &~ V_HUDTRANS)|V_HUDTRANSHALF;
				}
			}

			workx -= K_drawKartMicroTime(stickermedalinfo.targettext, workx, worky, splitflags);
		}

		workx -= (((1 + i - stickermedalinfo.platinumcount)*6) - 1);

		if (!mode)
			splitflags = (splitflags &~ V_HUDTRANSHALF)|V_HUDTRANS;
		while (i > 0)
		{
			i--;

			if (gamedata->collected[(stickermedalinfo.emblems[i]-emblemlocations)])
			{
				V_DrawMappedPatch(workx, worky, splitflags,
					static_cast<patch_t*>(W_CachePatchName(M_GetEmblemPatch(stickermedalinfo.emblems[i], false), PU_CACHE)),
					R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(stickermedalinfo.emblems[i]), GTC_CACHE)
				);

				workx += 6;
			}
			else if (
				stickermedalinfo.emblems[i]->type != ET_TIME
				|| stickermedalinfo.emblems[i]->tag != AUTOMEDAL_PLATINUM
			)
			{
				V_DrawMappedPatch(workx, worky, splitflags,
					static_cast<patch_t*>(W_CachePatchName("NEEDIT", PU_CACHE)),
					NULL
				);

				workx += 6;
			}
		}
	}

	if (modeattacking & ATTACKING_SPB && stplyr->SPBdistance > 0)
	{
		UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, static_cast<skincolornum_t>(stplyr->skincolor), GTC_CACHE);
		INT32 ybar = 180;
		INT32 widthbar = 120, xbar = 160 - widthbar/2, currentx;
		INT32 barflags = V_SNAPTOBOTTOM|V_SLIDEIN;
		INT32 transflags = ((6)<<FF_TRANSSHIFT);

		V_DrawScaledPatch(xbar, ybar - 2, barflags|transflags, kp_wouldyoustillcatchmeifiwereaworm);

		V_DrawMappedPatch(160 + widthbar/2 - 7, ybar - 7, barflags, faceprefix[stplyr->skin][FACE_MINIMAP], colormap);

		// vibes-based math
		currentx = (stplyr->SPBdistance/mapobjectscale - mobjinfo[MT_SPB].radius/FRACUNIT - mobjinfo[MT_PLAYER].radius/FRACUNIT) * 6;
		if (currentx > 0)
		{
			currentx = sqrt(currentx);
			if (currentx > widthbar)
				currentx = widthbar;
		}
		else
		{
			currentx = 0;
		}
		V_DrawScaledPatch(160 + widthbar/2 - currentx - 5, ybar - 7, barflags, kp_spbminimap);
	}
}

static fixed_t K_DrawKartPositionNumPatch(UINT8 num, UINT8 splitIndex, UINT8 *color, fixed_t x, fixed_t y, fixed_t scale, INT32 flags)
{
	fixed_t w = FRACUNIT;
	fixed_t h = FRACUNIT;
	INT32 overlayFlags[2];
	INT32 i;

	if (num > 9)
	{
		return x; // invalid input
	}

	if ((mapheaderinfo[gamemap - 1]->levelflags & LF_SUBTRACTNUM) == LF_SUBTRACTNUM)
	{
		overlayFlags[0] = V_SUBTRACT;
		overlayFlags[1] = V_ADD;
	}
	else
	{
		overlayFlags[0] = V_ADD;
		overlayFlags[1] = V_SUBTRACT;
	}

	w = SHORT(kp_positionnum[num][0][splitIndex]->width) * scale;
	h = SHORT(kp_positionnum[num][0][splitIndex]->height) * scale;

	x -= w;

	if (flags & V_SNAPTOBOTTOM)
	{
		y -= h;
	}

	for (i = 1; i >= 0; i--)
	{
		V_DrawFixedPatch(
			x, y, scale,
			flags | overlayFlags[i],
			kp_positionnum[num][i][splitIndex],
			color
		);
	}

	return x;
}

void K_DrawKartPositionNumXY(
		UINT8 num,
		UINT8 splitIndex,
		fixed_t fx, fixed_t fy, fixed_t scale, INT32 fflags,
		tic_t counter, boolean subtract,
		boolean exit, boolean lastLap, boolean losing
	)
{
	if (cv_reducevfx.value != 0)
	{
		// Reduce the flashing rate
		counter /= 4;
	}

	counter /= 3; // Alternate colors every three frames

	UINT8 *color = NULL;
	if (exit && num == 1)
	{
		// 1st place winner? You get rainbows!!
		color = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(SKINCOLOR_POSNUM_BEST1 + (counter % 6)), GTC_CACHE);
	}
	else if (exit || lastLap)
	{
		// On the final lap, or already won.
		boolean useRedNums = losing;

		if (subtract)
		{
			// Subtracting RED will look BLUE, and vice versa.
			useRedNums = !useRedNums;
		}

		if (useRedNums == true)
		{
			color = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(SKINCOLOR_POSNUM_LOSE1 + (counter % 3)), GTC_CACHE);
		}
		else
		{
			color = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(SKINCOLOR_POSNUM_WIN1 + (counter % 3)), GTC_CACHE);
		}
	}
	else
	{
		color = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_POSNUM, GTC_CACHE);
	}

	if ((fflags & V_SNAPTORIGHT) == 0)
	{
		UINT8 adjustNum = num;
		do
		{
			fixed_t w = SHORT(kp_positionnum[adjustNum % 10][0][splitIndex]->width) * scale;
			fx += w;
			adjustNum /= 10;
		} while (adjustNum);
	}

	// Draw the number
	do
	{
		fx = K_DrawKartPositionNumPatch(
			(num % 10), splitIndex, color,
			fx, fy, scale, V_SPLITSCREEN|fflags
		);
		num /= 10;
	} while (num);
}

static void K_DrawKartPositionNum(UINT8 num)
{
	UINT8 splitIndex = (r_splitscreen > 0) ? 1 : 0;
	fixed_t scale = FRACUNIT;
	fixed_t fx = 0, fy = 0;
	transnum_t trans = static_cast<transnum_t>(0);
	INT32 fflags = 0;

	if (stplyr->lives <= 0 && stplyr->playerstate == PST_DEAD)
	{
		return;
	}

	if (leveltime < (starttime + NUMTRANSMAPS))
	{
		trans = static_cast<transnum_t>((starttime + NUMTRANSMAPS) - leveltime);
	}

	if (trans >= NUMTRANSMAPS)
	{
		return;
	}

	if (stplyr->positiondelay > 0 || K_PlayerTallyActive(stplyr) == true)
	{
		const UINT8 delay = (stplyr->exiting) ? POS_DELAY_TIME : stplyr->positiondelay;
		const fixed_t add = (scale * 3) >> ((r_splitscreen == 1) ? 1 : 2);
		scale += std::min((add * (delay * delay)) / (POS_DELAY_TIME * POS_DELAY_TIME), add);
	}

	// pain and suffering defined below
	if (!r_splitscreen)
	{
		fx = BASEVIDWIDTH << FRACBITS;
		fy = BASEVIDHEIGHT << FRACBITS;
		fflags = V_SNAPTOBOTTOM|V_SNAPTORIGHT;
	}
	else if (r_splitscreen == 1)	// for this splitscreen, we'll use case by case because it's a bit different.
	{
		fx = BASEVIDWIDTH << FRACBITS;

		if (R_GetViewNumber() == 0)
		{
			// for player 1: display this at the top right, above the minimap.
			fy = 0;
			fflags = V_SNAPTOTOP|V_SNAPTORIGHT;
		}
		else
		{
			// if we're not p1, that means we're p2. display this at the bottom right, below the minimap.
			fy = BASEVIDHEIGHT << FRACBITS;
			fflags = V_SNAPTOBOTTOM|V_SNAPTORIGHT;
		}

		fy >>= 1;
	}
	else
	{
		fy = BASEVIDHEIGHT << FRACBITS;

		if (!(R_GetViewNumber() & 1)) // If we are P1 or P3...
		{
			// If we are P1 or P3...
			fx = 0;
			fflags = V_SNAPTOLEFT|V_SNAPTOBOTTOM;
		}
		else
		{
			// else, that means we're P2 or P4.
			fx = BASEVIDWIDTH << FRACBITS;
			fflags = V_SNAPTORIGHT|V_SNAPTOBOTTOM;
		}

		fx >>= 1;
		fy >>= 1;

		// We're putting it in the same corner as
		// the rest of our HUD, so it needs raised.
		fy -= (21 << FRACBITS);
	}

	if (trans > 0)
	{
		fflags |= (trans << V_ALPHASHIFT);
	}

	K_DrawKartPositionNumXY(
		num,
		splitIndex,
		fx, fy, scale, fflags,
		leveltime,
		((mapheaderinfo[gamemap - 1]->levelflags & LF_SUBTRACTNUM) == LF_SUBTRACTNUM),
		stplyr->exiting,
		(stplyr->laps >= numlaps),
		K_IsPlayerLosing(stplyr)
	);
}

struct PositionFacesInfo
{
	INT32 ranklines = 0;
	INT32 strank = -1;
	INT32 numplayersingame = 0;
	INT32 rankplayer[MAXPLAYERS] = {};

	PositionFacesInfo();
	void draw_1p();
	void draw_4p_battle(int x, int y, INT32 flags);

	player_t* top() const { return &players[rankplayer[0]]; }
	UINT32 top_score() const { return top()->roundscore; }

	bool near_goal() const
	{
		if (g_pointlimit == 0)
			return false;
		constexpr tic_t kThreshold = 5;
		return std::max(kThreshold, g_pointlimit) - kThreshold <= top_score();
	}

	skincolornum_t vomit_color() const
	{
		if (!near_goal())
		{
			return static_cast<skincolornum_t>(top()->skincolor);
		}

		constexpr int kCycleSpeed = 4;
		constexpr std::array<skincolornum_t, 6> kColors = {
			SKINCOLOR_RED,
			SKINCOLOR_VOMIT,
			SKINCOLOR_YELLOW,
			SKINCOLOR_GREEN,
			SKINCOLOR_JET,
			SKINCOLOR_MOONSET,
		};
		return kColors[leveltime / kCycleSpeed % kColors.size()];
	}
};

PositionFacesInfo::PositionFacesInfo()
{
	INT32 i, j;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		rankplayer[i] = -1;

		if (!playeringame[i] || players[i].spectator || !players[i].mo)
			continue;

		numplayersingame++;
	}

	if (numplayersingame <= 1)
		return;

	boolean completed[MAXPLAYERS] = {};

	for (j = 0; j < numplayersingame; j++)
	{
		UINT8 lowestposition = MAXPLAYERS+1;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (completed[i] || !playeringame[i] || players[i].spectator || !players[i].mo)
				continue;

			if (players[i].position >= lowestposition)
				continue;

			rankplayer[ranklines] = i;
			lowestposition = players[i].position;
		}

		i = rankplayer[ranklines];

		completed[i] = true;

		if (players+i == stplyr)
			strank = ranklines;

		//if (ranklines == 5)
			//break; // Only draw the top 5 players -- we do this a different way now...

		ranklines++;
	}
}

void PositionFacesInfo::draw_1p()
{
	// FACE_X = 15;				//  15
	// FACE_Y = 72;				//  72

	INT32 Y = FACE_Y-9; // -9 to offset where it's being drawn if there are more than one
	INT32 i, j;
	INT32 bumperx, emeraldx;
	INT32 xoff, yoff, flipflag = 0;
	UINT8 workingskin;
	UINT8 *colormap;
	UINT32 skinflags;

	if (gametyperules & GTR_POINTLIMIT) // playing battle
	{
		Y += 40;
		if (ranklines < 3)
			Y -= 18;
	}
	else if (ranklines < 5)
		Y += (9*ranklines);
	else
		Y += (9*5);

	ranklines--;
	i = ranklines;

	if (gametyperules & GTR_POINTLIMIT) // playing battle
	{
		// 3 lines max in Battle
		if (i > 2)
			i = 2;
		ranklines = 0;

		// You must appear on the leaderboard, even if you don't rank top 3
		if (strank > i)
		{
			strank = i;
			rankplayer[strank] = stplyr - players;
		}

		// Draw GOAL
		bool skull = g_pointlimit && (g_pointlimit <= stplyr->roundscore);
		INT32 height = i*18;
		INT32 GOAL_Y = Y-height;

		colormap = nullptr;

		if (skincolornum_t vomit = vomit_color())
		{
			colormap = R_GetTranslationColormap(TC_DEFAULT, vomit, GTC_CACHE);
		}

		V_DrawMappedPatch(FACE_X-5, GOAL_Y-32, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_goal[skull][0], colormap);

		// Flashing KO
		if (skull)
		{
			if (leveltime % 16 < 8)
				V_DrawScaledPatch(FACE_X-5, GOAL_Y-32, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_goaltext1p);
		}
		else if (g_pointlimit)
		{
			using srb2::Draw;
			Draw(FACE_X+8.5, GOAL_Y-15)
				.font(Draw::Font::kZVote)
				.align(Draw::Align::kCenter)
				.flags(V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT)
				.text("{:02}", g_pointlimit);
		}

		// Line cutting behind rank faces
		V_DrawScaledPatch(FACE_X+6, GOAL_Y, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_goalrod[0]);
	}
	else if (strank <= 2) // too close to the top, or a spectator? would have had (strank == -1) called out, but already caught by (strank <= 2)
	{
		if (i > 4) // could be both...
			i = 4;
		ranklines = 0;
	}
	else if (strank+2 >= ranklines) // too close to the bottom?
	{
		ranklines -= 4;
		if (ranklines < 0)
			ranklines = 0;
	}
	else
	{
		i = strank+2;
		ranklines = strank-2;
	}

	for (; i >= ranklines; i--)
	{
		if (!playeringame[rankplayer[i]]) continue;
		if (players[rankplayer[i]].spectator) continue;
		if (!players[rankplayer[i]].mo) continue;

		bumperx = FACE_X+19;
		emeraldx = FACE_X+16;

		skinflags = (demo.playback)
			? demo.skinlist[demo.currentskinid[rankplayer[i]]].flags
			: skins[players[rankplayer[i]].skin].flags;

		// Flip SF_IRONMAN portraits, but only if they're transformed
		if (skinflags & SF_IRONMAN
			&& !(players[rankplayer[i]].charflags & SF_IRONMAN) )
		{
			flipflag = V_FLIP|V_VFLIP; // blonic flip
			xoff = yoff = 16;
		} else
		{
			flipflag = 0;
			xoff = yoff = 0;
		}

		if (players[rankplayer[i]].mo->color)
		{
			if ((skin_t*)players[rankplayer[i]].mo->skin)
				workingskin = (skin_t*)players[rankplayer[i]].mo->skin - skins;
			else
				workingskin = players[rankplayer[i]].skin;

			colormap = R_GetTranslationColormap(workingskin, static_cast<skincolornum_t>(players[rankplayer[i]].mo->color), GTC_CACHE);
			if (players[rankplayer[i]].mo->colorized)
				colormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(players[rankplayer[i]].mo->color), GTC_CACHE);
			else
				colormap = R_GetTranslationColormap(workingskin, static_cast<skincolornum_t>(players[rankplayer[i]].mo->color), GTC_CACHE);

			V_DrawMappedPatch(FACE_X + xoff, Y + yoff, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT|flipflag, faceprefix[workingskin][FACE_RANK], colormap);

			if (LUA_HudEnabled(hud_battlebumpers))
			{
				const UINT8 bumpers = K_Bumpers(&players[rankplayer[i]]);

				if (bumpers > 0)
				{
					V_DrawMappedPatch(bumperx-2, Y, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_tinybumper[0], colormap);
					for (j = 1; j < bumpers; j++)
					{
						bumperx += 5;
						V_DrawMappedPatch(bumperx, Y, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_tinybumper[1], colormap);
					}
				}
			}
		}

		for (j = 0; j < 7; j++)
		{
			UINT32 emeraldFlag = (1 << j);
			skincolornum_t emeraldColor = static_cast<skincolornum_t>(SKINCOLOR_CHAOSEMERALD1 + j);

			if (players[rankplayer[i]].emeralds & emeraldFlag)
			{
				colormap = R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE);
				V_DrawMappedPatch(emeraldx, Y+7, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_rankemerald, colormap);
				emeraldx += 7;
			}
		}

		if (i == strank)
			V_DrawScaledPatch(FACE_X, Y, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_facehighlight[(leveltime / 4) % 8]);

		if ((gametyperules & GTR_BUMPERS) && (players[rankplayer[i]].pflags & PF_ELIMINATED))
			V_DrawScaledPatch(FACE_X-4, Y-3, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_ranknobumpers);
		else if (K_Cooperative())
			;
		else if (gametyperules & GTR_CIRCUIT)
		{
			INT32 pos = players[rankplayer[i]].position;
			if (pos < 0 || pos > MAXPLAYERS)
				pos = 0;
			// Draws the little number over the face
			V_DrawScaledPatch(FACE_X-5, Y+10, V_HUDTRANS|V_SLIDEIN|V_SNAPTOLEFT, kp_facenum[pos]);
		}
		else if (gametyperules & GTR_POINTLIMIT)
		{
			INT32 flags = V_HUDTRANS | V_SLIDEIN | V_SNAPTOLEFT;

			colormap = NULL;

			if (g_pointlimit && g_pointlimit <= players[rankplayer[i]].roundscore)
			{
				if (leveltime % 8 < 4)
				{
					colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_TANGERINE, GTC_CACHE);
				}

				flags |= V_STRINGDANCE;
			}

			V_DrawStringScaled(
					(FACE_X - 5) * FRACUNIT,
					(Y + 10) * FRACUNIT,
					FRACUNIT,
					FRACUNIT,
					FRACUNIT,
					flags,
					colormap,
					PINGF_FONT,
					va("%d", players[rankplayer[i]].roundscore)
			);
		}

		Y -= 18;
	}
}

void PositionFacesInfo::draw_4p_battle(int x, int y, INT32 flags)
{
	using srb2::Draw;
	Draw row = Draw(x, y).flags(V_HUDTRANS | V_SLIDEIN | flags).font(Draw::Font::kPing);

	UINT8 skull = []
	{
		if (g_pointlimit == 0)
			return 0;

		int party = G_PartySize(consoleplayer);
		for (int i = 0; i < party; ++i)
		{
			// Is any party member about to win?
			if (g_pointlimit <= players[G_PartyMember(consoleplayer, i)].roundscore)
			{
				return 1;
			}
		}
		return 0;
	}();

	skincolornum_t vomit = vomit_color();
	(vomit ? row.colormap(vomit) : row).patch(kp_goal[skull][1]);

	if (!skull && g_pointlimit)
	{
		row.xy(8.5, 5).align(Draw::Align::kCenter).text("{:02}", g_pointlimit);
	}

	row.xy(7, 18).patch(kp_goalrod[1]);

	auto head = [&](Draw col, int i)
	{
		const player_t& p = players[rankplayer[i]];
		col.colormap(p.skin, static_cast<skincolornum_t>(p.skincolor)).patch(faceprefix[p.skin][FACE_MINIMAP]);

		bool dance = g_pointlimit && (g_pointlimit <= p.roundscore);
		bool flash = dance && leveltime % 8 < 4;
		(
			flash ?
			col.xy(8, 6).colorize(SKINCOLOR_TANGERINE).flags(V_STRINGDANCE) :
			col.xy(8, 6).flags(dance ? V_STRINGDANCE : 0)
		).text("{:02}", p.roundscore);
	};

	// Draw top 2 players
	head(row.xy(2, 31), 1);
	head(row.xy(2, 18), 0);
}

static boolean K_drawKartPositionFaces(void)
{
	PositionFacesInfo state{};

	if (state.numplayersingame <= 1)
		return true;

	if (!LUA_HudEnabled(hud_minirankings))
		return false;	// Don't proceed but still return true for free play above if HUD is disabled.

	switch (r_splitscreen)
	{
	case 0:
		state.draw_1p();
		break;

	case 1:
		state.draw_4p_battle(292, 78, V_SNAPTORIGHT);
		break;

	case 2:
	case 3:
		state.draw_4p_battle(152, 9, V_SNAPTOTOP);
		state.draw_4p_battle(152, 147, V_SNAPTOBOTTOM);
		break;
	}

	return false;
}

static void K_drawBossHealthBar(void)
{
	UINT8 i = 0, barstatus = 1, randlen = 0, darken = 0;
	const INT32 startx = BASEVIDWIDTH - 23;
	INT32 starty = BASEVIDHEIGHT - 25;
	INT32 rolrand = 0, randtemp = 0;
	boolean randsign = false;

	if (bossinfo.barlen <= 1)
		return;

	// Entire bar juddering!
	if (lt_exitticker < (TICRATE/2))
		;
	else if (bossinfo.visualbarimpact)
	{
		INT32 mag = std::min<UINT32>((bossinfo.visualbarimpact/4) + 1, 8u);
		if (bossinfo.visualbarimpact & 1)
			starty -= mag;
		else
			starty += mag;
	}

	if ((lt_ticker >= lt_endtime) && bossinfo.enemyname)
	{
		if (lt_exitticker == 0)
		{
			rolrand = 5;
		}
		else if (lt_exitticker == 1)
		{
			rolrand = 7;
		}
		else
		{
			rolrand = 10;
		}
		V_DrawRightAlignedThinString(startx, starty-rolrand, V_FORCEUPPERCASE|V_HUDTRANS|V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTORIGHT, bossinfo.enemyname);
		rolrand = 0;
	}

	// Used for colour and randomisation.
	if (bossinfo.healthbar <= (bossinfo.visualdiv/FRACUNIT))
	{
		barstatus = 3;
	}
	else if (bossinfo.healthbar <= bossinfo.healthbarpinch)
	{
		barstatus = 2;
	}

	randtemp = bossinfo.visualbar-(bossinfo.visualdiv/(2*FRACUNIT));
	if (randtemp > 0)
		randlen = P_RandomKey(PR_INTERPHUDRANDOM, randtemp)+1;
	randsign = P_RandomChance(PR_INTERPHUDRANDOM, FRACUNIT/2);

	// Right wing.
	V_DrawScaledPatch(startx-1, starty, V_HUDTRANS|V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_FLIP, kp_bossbar[0]);

	// Draw the bar itself...
	while (i < bossinfo.barlen)
	{
		V_DrawScaledPatch(startx-i, starty, V_HUDTRANS|V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTORIGHT, kp_bossbar[1]);
		if (i < bossinfo.visualbar)
		{
			randlen--;
			if (!randlen)
			{
				randtemp = bossinfo.visualbar-(bossinfo.visualdiv/(2*FRACUNIT));
				if (randtemp > 0)
					randlen = P_RandomKey(PR_INTERPHUDRANDOM, randtemp)+1;
				if (barstatus > 1)
				{
					rolrand = P_RandomKey(PR_INTERPHUDRANDOM, barstatus)+1;
				}
				else
				{
					rolrand = 1;
				}
				if (randsign)
				{
					rolrand = -rolrand;
				}
				randsign = !randsign;
			}
			else
			{
				rolrand = 0;
			}
			if (lt_exitticker < (TICRATE/2))
				;
			else if ((bossinfo.visualbar - i) < (INT32)(bossinfo.visualbarimpact/8))
			{
				if (bossinfo.visualbarimpact & 1)
					rolrand += (bossinfo.visualbar - i);
				else
					rolrand -= (bossinfo.visualbar - i);
			}
			if (bossinfo.visualdiv)
			{
				fixed_t work = 0;
				if ((i+1) == bossinfo.visualbar)
					darken = 1;
				else
				{
					darken = 0;
					// a hybrid fixed-int modulo...
					while ((work/FRACUNIT) < bossinfo.visualbar)
					{
						if (work/FRACUNIT != i)
						{
							work += bossinfo.visualdiv;
							continue;
						}
						darken = 1;
						break;
					}
				}
			}
			V_DrawScaledPatch(startx-i, starty+rolrand, V_HUDTRANS|V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTORIGHT, kp_bossbar[(2*barstatus)+darken]);
		}
		i++;
	}

	// Left wing.
	V_DrawScaledPatch(startx-i, starty, V_HUDTRANS|V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTORIGHT, kp_bossbar[0]);
}

static void K_drawKartEmeralds(void)
{
	static const INT32 emeraldOffsets[7][3] = {
		{34, 0, 15},
		{25, 8, 11},
		{43, 8, 19},
		{16, 0,  7},
		{52, 0, 23},
		{ 7, 8,  3},
		{61, 8, 27}
	};

	INT32 splitflags = V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN;
	INT32 startx = BASEVIDWIDTH - 77;
	INT32 starty = BASEVIDHEIGHT - 29;
	INT32 i = 0, xindex = 0;

	{
		if (r_splitscreen)
		{
			starty = (starty/2) - 8;
		}
		starty -= 8;

		if (r_splitscreen < 2)
		{
			startx -= 8;
			if (r_splitscreen == 1 && R_GetViewNumber() == 0)
			{
				starty = 1;
			}
			V_DrawScaledPatch(startx, starty, V_HUDTRANS|splitflags, kp_rankemeraldback);
		}
		else
		{
			xindex = 2;
			starty -= 15;
			if (!(R_GetViewNumber() & 1)) // If we are P1 or P3...
			{
				startx = LAPS_X;
				splitflags = V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_SPLITSCREEN;
			}
			else // else, that means we're P2 or P4.
			{
				startx = LAPS2_X + 1;
				splitflags = V_SNAPTORIGHT|V_SNAPTOBOTTOM|V_SPLITSCREEN;
			}
		}
	}

	for (i = 0; i < 7; i++)
	{
		UINT32 emeraldFlag = (1 << i);
		skincolornum_t emeraldColor = static_cast<skincolornum_t>(SKINCOLOR_CHAOSEMERALD1 + i);

		if (stplyr->emeralds & emeraldFlag)
		{
			boolean whiteFlash = (leveltime & 1);
			UINT8 *colormap;

			if (i & 1)
			{
				whiteFlash = !whiteFlash;
			}

			colormap = R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE);
			V_DrawMappedPatch(
				startx + emeraldOffsets[i][xindex], starty + emeraldOffsets[i][1],
				V_HUDTRANS|splitflags,
				kp_rankemerald, colormap
			);

			if (whiteFlash == true)
			{
				V_DrawScaledPatch(
					startx + emeraldOffsets[i][xindex], starty + emeraldOffsets[i][1],
					V_HUDTRANSHALF|splitflags,
					kp_rankemeraldflash
				);
			}
		}
	}
}

static void K_drawKartLaps(void)
{
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
			if (!(R_GetViewNumber() & 1)) // If we are P1 or P3...
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

		// Laps
		V_DrawScaledPatch(fx-2 + (flipflag ? (SHORT(kp_ringstickersplit[1]->width) - 3) : 0), fy, V_HUDTRANS|V_SLIDEIN|splitflags|flipflag, kp_ringstickersplit[0]);

		V_DrawScaledPatch(fx, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_splitlapflag);
		V_DrawScaledPatch(fx+22, fy, V_HUDTRANS|V_SLIDEIN|splitflags, frameslash);

		if (numlaps >= 10)
		{
			UINT8 ln[2];
			ln[0] = ((stplyr->laps / 10) % 10);
			ln[1] = (stplyr->laps % 10);

			V_DrawScaledPatch(fx+13, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[0]]);
			V_DrawScaledPatch(fx+17, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[1]]);

			ln[0] = ((numlaps / 10) % 10);
			ln[1] = (numlaps % 10);

			V_DrawScaledPatch(fx+27, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[0]]);
			V_DrawScaledPatch(fx+31, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[ln[1]]);
		}
		else
		{
			V_DrawScaledPatch(fx+13, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[(stplyr->laps) % 10]);
			V_DrawScaledPatch(fx+27, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[(numlaps) % 10]);
		}
	}
	else
	{
		// Laps
		V_DrawScaledPatch(LAPS_X, LAPS_Y, V_HUDTRANS|V_SLIDEIN|splitflags, kp_lapsticker);
		V_DrawTimerString(LAPS_X+33, LAPS_Y+3, V_HUDTRANS|V_SLIDEIN|splitflags, va("%d/%d", std::min(stplyr->laps, numlaps), numlaps));
	}
}

#define RINGANIM_FLIPFRAME (RINGANIM_NUMFRAMES/2)

static void K_DrawLivesDigits(INT32 x, INT32 y, INT32 width, INT32 flags, patch_t *font[10])
{
	const SINT8 tens = stplyr->lives / 10;

	if (tens)
	{
		V_DrawScaledPatch(x, y, flags, font[tens % 10]);
		x += width;
	}

	V_DrawScaledPatch(x, y, flags, font[stplyr->lives % 10]);
}

static void K_drawRingCounter(boolean gametypeinfoshown)
{
	const boolean uselives = G_GametypeUsesLives();
	SINT8 ringanim_realframe = stplyr->karthud[khud_ringframe];
	INT32 splitflags = V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_SPLITSCREEN;
	UINT8 rn[2];
	INT32 ringflip = 0;
	UINT8 *ringmap = NULL;
	boolean colorring = false;
	INT32 ringx = 0, fy = 0;

	rn[0] = ((abs(stplyr->hudrings) / 10) % 10);
	rn[1] = (abs(stplyr->hudrings) % 10);

	if (stplyr->hudrings <= 0 && stplyr->ringvisualwarning > 1)
	{
		colorring = true;
		if ((leveltime/2 & 1))
		{
			ringmap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_CRIMSON, GTC_CACHE);
		}
		else
		{
			ringmap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_WHITE, GTC_CACHE);
		}
	}
	else if (stplyr->hudrings <= 0 && (leveltime/5 & 1)) // In debt
	{
		ringmap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_CRIMSON, GTC_CACHE);
		colorring = true;
	}
	else if (stplyr->hudrings >= 20) // Maxed out
		ringmap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_YELLOW, GTC_CACHE);

	if (stplyr->karthud[khud_ringframe] > RINGANIM_FLIPFRAME)
	{
		ringflip = V_FLIP;
		ringanim_realframe = RINGANIM_NUMFRAMES-stplyr->karthud[khud_ringframe];
		ringx += SHORT((r_splitscreen > 1) ? kp_smallring[ringanim_realframe]->width : kp_ring[ringanim_realframe]->width);
	}

	if (r_splitscreen > 1)
	{
		INT32 fx = 0, fr = 0;
		INT32 flipflag = 0;

		// pain and suffering defined below
		if (r_splitscreen < 2)	// don't change shit for THIS splitscreen.
		{
			fx = LAPS_X;
			fy = LAPS_Y;
		}
		else
		{
			if (!(R_GetViewNumber() & 1)) // If we are P1 or P3...
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

		if (gametypeinfoshown)
		{
			fy -= 10;
		}

		// Rings
		if (!uselives)
		{
			V_DrawScaledPatch(fx-2 + (flipflag ? (SHORT(kp_ringstickersplit[1]->width) - 3) : 0), fy, V_HUDTRANS|V_SLIDEIN|splitflags|flipflag, kp_ringstickersplit[1]);
			if (flipflag)
				fr += 15;
		}
		else
			V_DrawScaledPatch(fx-2 + (flipflag ? (SHORT(kp_ringstickersplit[0]->width) - 3) : 0), fy, V_HUDTRANS|V_SLIDEIN|splitflags|flipflag, kp_ringstickersplit[0]);

		V_DrawMappedPatch(fr+ringx, fy-3, V_HUDTRANS|V_SLIDEIN|splitflags|ringflip, kp_smallring[ringanim_realframe], (colorring ? ringmap : NULL));

		if (stplyr->hudrings < 0) // Draw the minus for ring debt
		{
			V_DrawMappedPatch(fr+11, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_ringdebtminussmall, ringmap);
			V_DrawMappedPatch(fr+15, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[rn[0]], ringmap);
			V_DrawMappedPatch(fr+19, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[rn[1]], ringmap);
		}
		else
		{
			V_DrawMappedPatch(fr+11, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[rn[0]], ringmap);
			V_DrawMappedPatch(fr+15, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font[rn[1]], ringmap);
		}

		// SPB ring lock
		if (stplyr->pflags & PF_RINGLOCK)
			V_DrawScaledPatch(fr-12, fy-13, V_HUDTRANS|V_SLIDEIN|splitflags, kp_ringspblocksmall[stplyr->karthud[khud_ringspblock]]);

		// Lives
		if (uselives)
		{
			UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, static_cast<skincolornum_t>(stplyr->skincolor), GTC_CACHE);
			V_DrawMappedPatch(fr+21, fy-3, V_HUDTRANS|V_SLIDEIN|splitflags, faceprefix[stplyr->skin][FACE_MINIMAP], colormap);
			if (stplyr->lives >= 0)
				K_DrawLivesDigits(fr+34, fy, 4, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[PINGNUM_FONT].font);
		}
	}
	else
	{
		fy = LAPS_Y;

		if (gametypeinfoshown)
		{
			fy -= 11;

			if ((gametyperules & (GTR_BUMPERS|GTR_CIRCUIT)) == GTR_BUMPERS)
				fy -= 4;
		}
		else
		{
			fy += 9;
		}

		// Rings
		using srb2::Draw;
		Draw(LAPS_X+7, fy+1)
			.flags(V_HUDTRANS|V_SLIDEIN|splitflags)
			.align(Draw::Align::kCenter)
			.width(uselives ? (stplyr->lives >= 10 ? 70 : 64) : 33)
			.small_sticker();

		V_DrawMappedPatch(LAPS_X+ringx+7, fy-5, V_HUDTRANS|V_SLIDEIN|splitflags|ringflip, kp_ring[ringanim_realframe], (colorring ? ringmap : NULL));

		// "Why fy-4? Why LAPS_X+29+1?"
		// "use magic numbers" - jartha 2024-03-05
		if (stplyr->hudrings < 0) // Draw the minus for ring debt
		{
			V_DrawMappedPatch(LAPS_X+23-1, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_ringdebtminus, ringmap);
			using srb2::Draw;
			Draw row = Draw(LAPS_X+29+0, fy-4).flags(V_HUDTRANS|V_SLIDEIN|splitflags).font(Draw::Font::kThinTimer).colormap(ringmap);
			row.text("{:02}", abs(stplyr->hudrings));
			// V_DrawMappedPatch(LAPS_X+29, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[TALLNUM_FONT].font[rn[0]], ringmap);
			// V_DrawMappedPatch(LAPS_X+35, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[TALLNUM_FONT].font[rn[1]], ringmap);
		}
		else
		{
			using srb2::Draw;
			Draw row = Draw(LAPS_X+23+3, fy-4).flags(V_HUDTRANS|V_SLIDEIN|splitflags).font(Draw::Font::kThinTimer).colormap(ringmap);
			row.text("{:02}", abs(stplyr->hudrings));
			// V_DrawMappedPatch(LAPS_X+23, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[TALLNUM_FONT].font[rn[0]], ringmap);
			// V_DrawMappedPatch(LAPS_X+29, fy, V_HUDTRANS|V_SLIDEIN|splitflags, fontv[TALLNUM_FONT].font[rn[1]], ringmap);
		}

		// SPB ring lock
		if (stplyr->pflags & PF_RINGLOCK)
			V_DrawScaledPatch(LAPS_X-5, fy-17, V_HUDTRANS|V_SLIDEIN|splitflags, kp_ringspblock[stplyr->karthud[khud_ringspblock]]);

		// Lives
		if (uselives)
		{
			UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, static_cast<skincolornum_t>(stplyr->skincolor), GTC_CACHE);
			V_DrawMappedPatch(LAPS_X+46, fy-5, V_HUDTRANS|V_SLIDEIN|splitflags, faceprefix[stplyr->skin][FACE_RANK], colormap);
			SINT8 livescount = 0;
			if (stplyr->lives > 0)
			{
				livescount = stplyr->lives;
				if (livescount > 10)
					livescount = 10;
			}
			using srb2::Draw;
			Draw row = Draw(LAPS_X+65, fy-4).flags(V_HUDTRANS|V_SLIDEIN|splitflags).font(Draw::Font::kThinTimer);
			row.text("{}", livescount);
		}
	}
}

#undef RINGANIM_FLIPFRAME

static void K_drawKartAccessibilityIcons(boolean gametypeinfoshown, INT32 fx)
{
    INT32 fy = LAPS_Y-14;
    INT32 splitflags = V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_SPLITSCREEN;

    boolean mirror = false;

    fx += LAPS_X;

    if (r_splitscreen < 2) // adjust to speedometer height
    {
		if (battleprisons)
		{
			fy -= 2;
		}

        if (gametypeinfoshown)
        {
            fy -= 11;

            if ((gametyperules & (GTR_BUMPERS|GTR_CIRCUIT)) == GTR_BUMPERS)
                fy -= 4;
        }
        else
        {
            fy += 9;
        }
    }
    else
    {
        fx = LAPS_X+44;
        fy = LAPS_Y;
        if (R_GetViewNumber() & 1) // If we are not P1 or P3...
        {
            splitflags ^= (V_SNAPTOLEFT|V_SNAPTORIGHT);
            fx = (BASEVIDWIDTH/2) - fx;
            mirror = true;
        }
    }

    // Kickstart Accel
    if (stplyr->pflags & PF_KICKSTARTACCEL)
    {
        if (mirror)
            fx -= 10;

        SINT8 col = 0, wid, fil, ofs;
        UINT8 i = 7;
        ofs = (stplyr->kickstartaccel == ACCEL_KICKSTART) ? 1 : 0;
        fil = i-(stplyr->kickstartaccel*i)/ACCEL_KICKSTART;

        V_DrawFill(fx+4, fy+ofs-1, 2, 1, 31|V_SLIDEIN|splitflags);
        V_DrawFill(fx, (fy+ofs-1)+8, 10, 1, 31|V_SLIDEIN|splitflags);

        while (i--)
        {
            wid = (i/2)+1;
            V_DrawFill(fx+4-wid, fy+ofs+i, 2+(wid*2), 1, 31|V_SLIDEIN|splitflags);
            if (fil > 0)
            {
                if (i < fil)
                    col = 23;
                else if (i == fil)
                    col = 3;
                else
                    col = 5 + (i-fil)*2;
            }
            else if ((leveltime % 7) == i)
                col = 0;
            else
                col = 3;
            V_DrawFill(fx+5-wid, fy+ofs+i, (wid*2), 1, col|V_SLIDEIN|splitflags);
        }

        if (mirror)
            fx--;
        else
            fx += 10 + 1;
    }

    // Auto Roulette
    if (stplyr->pflags & PF_AUTOROULETTE)
    {
        if (mirror)
            fx -= 12;

        V_DrawScaledPatch(fx, fy-1, V_SLIDEIN|splitflags, kp_autoroulette);

        if (mirror)
            fx--;
        else
            fx += 12 + 1;
    }

	if (stplyr->pflags & PF_AUTORING)
    {
        if (mirror)
            fx -= 14;

        V_DrawScaledPatch(fx, fy-1, V_SLIDEIN|splitflags, kp_autoring);

        if (mirror)
            fx--;
        else
            fx += 14 + 1;
    }
}

static void K_drawKartSpeedometer(boolean gametypeinfoshown)
{
	static fixed_t convSpeed;
	UINT8 labeln = 0;
	UINT8 numbers[3];
	INT32 splitflags = V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_SPLITSCREEN;
	INT32 fy = LAPS_Y-14;

	if (battleprisons)
	{
		fy -= 2;
	}

	if (!stplyr->exiting) // Keep the same speed value as when you crossed the finish line!
	{
		switch (cv_kartspeedometer.value)
		{
			case 1: // Sonic Drift 2 style percentage
			default:
				convSpeed = (stplyr->speed * 100) / K_GetKartSpeed(stplyr, false, true); // Based on top speed!
				labeln = 0;
				break;
			case 2: // Kilometers
				convSpeed = FixedDiv(FixedMul(stplyr->speed, 142371), mapobjectscale) / FRACUNIT; // 2.172409058
				labeln = 1;
				break;
			case 3: // Miles
				convSpeed = FixedDiv(FixedMul(stplyr->speed, 88465), mapobjectscale) / FRACUNIT; // 1.349868774
				labeln = 2;
				break;
			case 4: // Fracunits
				convSpeed = FixedDiv(stplyr->speed, mapobjectscale) / FRACUNIT; // 1.0. duh.
				labeln = 3;
				break;
		}
	}

	// Don't overflow
	// (negative speed IS really high speed :V)
	if (convSpeed > 999 || convSpeed < 0)
		convSpeed = 999;

	numbers[0] = ((convSpeed / 100) % 10);
	numbers[1] = ((convSpeed / 10) % 10);
	numbers[2] = (convSpeed % 10);

	if (gametypeinfoshown)
	{
		fy -= 11;

		if ((gametyperules & (GTR_BUMPERS|GTR_CIRCUIT)) == GTR_BUMPERS)
			fy -= 4;
	}
	else
	{
		fy += 9;
	}

	using srb2::Draw;
	Draw(LAPS_X+7, fy+1).flags(V_HUDTRANS|V_SLIDEIN|splitflags).align(Draw::Align::kCenter).width(42).small_sticker();
	V_DrawScaledPatch(LAPS_X+7, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[numbers[0]]);
	V_DrawScaledPatch(LAPS_X+13, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[numbers[1]]);
	V_DrawScaledPatch(LAPS_X+19, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_facenum[numbers[2]]);
	V_DrawScaledPatch(LAPS_X+29, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_speedometerlabel[labeln]);

	K_drawKartAccessibilityIcons(gametypeinfoshown, 56);
}

static void K_drawBlueSphereMeter(boolean gametypeinfoshown)
{
	const UINT8 maxBars = 4;
	// see also K_DrawNameTagSphereMeter
	const UINT8 segColors[] = {73, 64, 52, 54, 55, 35, 34, 33, 202, 180, 181, 182, 164, 165, 166, 153, 152};
	const UINT8 sphere = std::clamp(static_cast<int>(stplyr->spheres), 0, 40);

	UINT8 numBars = std::min((sphere / 10), +maxBars);
	UINT8 colorIndex = (sphere * sizeof(segColors)) / (40 + 1);
	INT32 fx, fy;
	UINT8 i;
	INT32 splitflags = V_HUDTRANS|V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_SPLITSCREEN;
	INT32 flipflag = 0;
	INT32 xstep = 15;

	// pain and suffering defined below
	if (r_splitscreen < 2)	// don't change shit for THIS splitscreen.
	{
		fx = LAPS_X;
		fy = LAPS_Y-4;

		if (battleprisons)
		{
			if (r_splitscreen == 1)
			{
				fy -= 8;
			}
			else
			{
				fy -= 5;
			}
		}
		else if (r_splitscreen == 1)
		{
			fy -= 5;
		}

		if (gametypeinfoshown)
		{
			fy -= 11 + 4;
		}
		else
		{
			fy += 9;
		}

		V_DrawScaledPatch(fx, fy, splitflags|flipflag, kp_spheresticker);
	}
	else
	{
		xstep = 8;
		if (!(R_GetViewNumber() & 1)) // If we are P1 or P3...
		{
			fx = LAPS_X-2;
			fy = LAPS_Y;
		}
		else // else, that means we're P2 or P4.
		{
			fx = LAPS2_X+(SHORT(kp_splitspheresticker->width) - 10);
			fy = LAPS2_Y;
			splitflags ^= V_SNAPTOLEFT|V_SNAPTORIGHT;
			flipflag = V_FLIP; // make the string right aligned and other shit
			xstep = -xstep;
		}

		if (battleprisons)
		{
			fy -= 5;
		}

		if (gametypeinfoshown)
		{
			fy -= 16;
		}

		V_DrawScaledPatch(fx, fy, splitflags|flipflag, kp_splitspheresticker);
	}

	if (r_splitscreen < 2)
	{
		fx += 25;
	}
	else
	{
		fx += (flipflag) ? -18 : 13;
	}

	for (i = 0; i <= numBars; i++)
	{
		UINT8 segLen = (r_splitscreen < 2) ? 10 : 5;

		if (i == numBars)
		{
			segLen = (sphere % 10);
			if (r_splitscreen < 2)
				;
			else
			{
				segLen = (segLen+1)/2; // offset so nonzero spheres shows up IMMEDIATELY
				if (!segLen)
					break;
				if (flipflag)
					fx += (5-segLen);
			}
		}

		if (r_splitscreen < 2)
		{
			V_DrawFill(fx, fy + 6, segLen, 3, segColors[std::max(colorIndex-1, 0)] | splitflags);
			V_DrawFill(fx, fy + 7, segLen, 1, segColors[std::max(colorIndex-2, 0)] | splitflags);
			V_DrawFill(fx, fy + 9, segLen, 3, segColors[colorIndex] | splitflags);
		}
		else
		{
			V_DrawFill(fx, fy + 5, segLen, 1, segColors[std::max(colorIndex-1, 0)] | splitflags);
			V_DrawFill(fx, fy + 6, segLen, 1, segColors[std::max(colorIndex-2, 0)] | splitflags);
			V_DrawFill(fx, fy + 7, segLen, 2, segColors[colorIndex] | splitflags);
		}

		fx += xstep;
	}
}

static void K_drawKartBumpersOrKarma(void)
{
	UINT8 *colormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(stplyr->skincolor), GTC_CACHE);
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
			if (!(R_GetViewNumber() & 1)) // If we are P1 or P3...
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

		{
			using srb2::Draw;
			int width = 39;
			if (!battleprisons)
			{
				constexpr int kPad = 16;
				if (flipflag)
					fx -= kPad;
				width += kPad;
			}
			Draw(fx-1 + (flipflag ? width + 3 : 0), fy+1)
				.flags(V_HUDTRANS|V_SLIDEIN|splitflags)
				.align(flipflag ? Draw::Align::kRight : Draw::Align::kLeft)
				.width(width)
				.small_sticker();
		}

		fx += 2;

		if (battleprisons)
		{
			V_DrawScaledPatch(fx+22, fy, V_HUDTRANS|V_SLIDEIN|splitflags, frameslash);
			V_DrawMappedPatch(fx-1, fy-2, V_HUDTRANS|V_SLIDEIN|splitflags, kp_rankcapsule, NULL);

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
			const UINT8 bumpers = K_Bumpers(stplyr);
			const bool dance = g_pointlimit && (g_pointlimit <= stplyr->roundscore);

			V_DrawMappedPatch(fx-1, fy-2, V_HUDTRANS|V_SLIDEIN|splitflags, kp_rankbumper, colormap);

			using srb2::Draw;
			Draw row = Draw(fx+12, fy).flags(V_HUDTRANS|V_SLIDEIN|splitflags).font(Draw::Font::kPing);
			row.text("{:02}", bumpers);
			if (dance && leveltime % 8 < 4)
			{
				row = row.colorize(SKINCOLOR_TANGERINE);
			}
			row.xy(10, -2).patch(kp_pts[1]);
			row
				.x(31)
				.flags(dance ? V_STRINGDANCE : 0)
				.text("{:02}", stplyr->roundscore);
		}
	}
	else
	{
		INT32 fy = r_splitscreen == 1 ? LAPS_Y-3 : LAPS_Y;

		if (battleprisons)
		{
			if (numtargets > 9 && maptargets > 9)
				V_DrawMappedPatch(LAPS_X, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_capsulestickerwide, NULL);
			else
				V_DrawMappedPatch(LAPS_X, fy, V_HUDTRANS|V_SLIDEIN|splitflags, kp_capsulesticker, NULL);
			V_DrawTimerString(LAPS_X+47, fy+3, V_HUDTRANS|V_SLIDEIN|splitflags, va("%d/%d", numtargets, maptargets));
		}
		else
		{
			const UINT8 bumpers = K_Bumpers(stplyr);
			const bool dance = g_pointlimit && (g_pointlimit <= stplyr->roundscore);

			if (r_splitscreen == 0)
			{
				fy += 2;
			}

			K_DrawSticker(LAPS_X+12, fy+5, 75, V_HUDTRANS|V_SLIDEIN|splitflags, false);
			V_DrawMappedPatch(LAPS_X+12, fy-2, V_HUDTRANS|V_SLIDEIN|splitflags, kp_bigbumper, colormap);

			using srb2::Draw;
			Draw row = Draw(LAPS_X+12+23+1, fy+3).flags(V_HUDTRANS|V_SLIDEIN|splitflags).font(Draw::Font::kThinTimer);
			row.text("{:02}", bumpers);
			if (dance && leveltime % 8 < 4)
			{
				row = row.colorize(SKINCOLOR_TANGERINE);
			}
			row.xy(12, -2).patch(kp_pts[0]);
			row
				.x(12+27)
				.flags(dance ? V_STRINGDANCE : 0)
				.text("{:02}", stplyr->roundscore);
		}
	}
}

#if 0
static void K_drawKartWanted(void)
{
	UINT8 i, numwanted = 0;
	UINT8 *colormap = NULL;
	INT32 basex = 0, basey = 0;

	if (!splitscreen)
		return;

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
		colormap = R_GetTranslationColormap(TC_DEFAULT, players[battlewanted[0]].skincolor, GTC_CACHE);
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
#endif //if 0

static void K_drawKartPlayerCheck(void)
{
	const fixed_t maxdistance = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	UINT8 i;
	INT32 splitflags = V_SNAPTOBOTTOM|V_SPLITSCREEN;
	fixed_t y = CHEK_Y * FRACUNIT;

	if (stplyr == NULL || stplyr->mo == NULL || P_MobjWasRemoved(stplyr->mo))
	{
		return;
	}

	if (stplyr->spectator || stplyr->awayview.tics)
	{
		return;
	}

	if (stplyr->cmd.buttons & BT_LOOKBACK)
	{
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *checkplayer = &players[i];
		fixed_t distance = maxdistance+1;
		UINT8 *colormap = NULL;
		UINT8 pnum = 0;
		vector3_t v;
		vector3_t pPos;
		trackingResult_t result;

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

		v.x = R_InterpolateFixed(checkplayer->mo->old_x, checkplayer->mo->x);
		v.y = R_InterpolateFixed(checkplayer->mo->old_y, checkplayer->mo->y);
		v.z = R_InterpolateFixed(checkplayer->mo->old_z, checkplayer->mo->z);

		pPos.x = R_InterpolateFixed(stplyr->mo->old_x, stplyr->mo->x);
		pPos.y = R_InterpolateFixed(stplyr->mo->old_y, stplyr->mo->y);
		pPos.z = R_InterpolateFixed(stplyr->mo->old_z, stplyr->mo->z);

		distance = R_PointToDist2(pPos.x, pPos.y, v.x, v.y);

		if (distance > maxdistance)
		{
			// Too far away
			continue;
		}

		if ((checkplayer->invincibilitytimer <= 0) && (leveltime & 2))
		{
			pnum++; // white frames
		}

		if (checkplayer->itemtype == KITEM_GROW || checkplayer->growshrinktimer > 0)
		{
			pnum += 4;
		}
		else if (checkplayer->itemtype == KITEM_INVINCIBILITY || checkplayer->invincibilitytimer)
		{
			pnum += 2;
		}

		K_ObjectTracking(&result, &v, true);

		if (result.onScreen == true)
		{
			colormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(checkplayer->mo->color), GTC_CACHE);
			V_DrawFixedPatch(result.x, y, FRACUNIT, V_HUDTRANS|V_SPLITSCREEN|splitflags, kp_check[pnum], colormap);
		}
	}
}

static boolean K_ShowPlayerNametag(player_t *p)
{
	if (cv_seenames.value == 0)
	{
		return false;
	}

	if (demo.playback == true && camera[R_GetViewNumber()].freecam == true)
	{
		return true;
	}

	if (stplyr == p)
	{
		return false;
	}

	if (gametyperules & GTR_CIRCUIT)
	{
		if ((p->position == 0)
		|| (stplyr->position == 0)
		|| (p->position < stplyr->position-2)
		|| (p->position > stplyr->position+2))
		{
			return false;
		}
	}

	return true;
}

static void K_DrawLocalTagForPlayer(fixed_t x, fixed_t y, player_t *p, UINT8 id)
{
	UINT8 blink = ((leveltime / 7) & 1);
	UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(p->skincolor), GTC_CACHE);
	V_DrawFixedPatch(x, y, FRACUNIT, V_HUDTRANS|V_SPLITSCREEN, kp_localtag[id][blink], colormap);
}

static void K_DrawRivalTagForPlayer(fixed_t x, fixed_t y)
{
	UINT8 blink = ((leveltime / 7) & 1);
	V_DrawFixedPatch(x, y, FRACUNIT, V_HUDTRANS|V_SPLITSCREEN, kp_rival[blink], NULL);
}

static void K_DrawTypingDot(fixed_t x, fixed_t y, UINT8 duration, player_t *p, INT32 flags)
{
	if (p->typing_duration > duration)
	{
		V_DrawFixedPatch(x, y, FRACUNIT, V_HUDTRANS|V_SPLITSCREEN|flags, kp_typdot, NULL);
	}
}

static void K_DrawTypingNotifier(fixed_t x, fixed_t y, player_t *p, INT32 flags)
{
	if (p->cmd.flags & TICCMD_TYPING)
	{
		V_DrawFixedPatch(x, y, FRACUNIT, V_HUDTRANS|V_SPLITSCREEN|flags, kp_talk, NULL);

		/* spacing closer with the last two looks a better most of the time */
		K_DrawTypingDot(x + 3*FRACUNIT,              y, 15, p, flags);
		K_DrawTypingDot(x + 6*FRACUNIT - FRACUNIT/3, y, 31, p, flags);
		K_DrawTypingDot(x + 9*FRACUNIT - FRACUNIT/3, y, 47, p, flags);
	}
}

// see also K_drawKartItem
static void K_DrawNameTagItemSpy(INT32 x, INT32 y, player_t *p, INT32 flags)
{
	using srb2::Draw;
	bool tiny = r_splitscreen > 1;
	Draw bar = Draw(x, y).flags(V_NOSCALESTART|flags);
	Draw box = tiny ? bar.xy(-22 * vid.dupx, -17 * vid.dupy) : bar.xy(-40 * vid.dupx, -26 * vid.dupy);

	box.colorize(p->skincolor).patch(kp_itembg[tiny ? 4 : 2]);

	if (!(p->itemflags & IF_ITEMOUT) || (leveltime & 1))
	{
		switch (p->itemtype)
		{
		case KITEM_INVINCIBILITY:
			box.patch(kp_invincibility[((leveltime % (6*3)) / 3) + (tiny ? 13 : 7)]);
			break;

		case KITEM_ORBINAUT:
			box.patch(kp_orbinaut[4 + tiny]);
			break;

		default:
			if (patch_t *ico = K_GetCachedItemPatch(p->itemtype, 1 + tiny))
			{
				box.patch(ico);
			}
		}
	}

	if (p->itemamount > 1)
	{
		(tiny ?
			bar.xy(-3 * vid.dupx, -4 * vid.dupy).font(Draw::Font::kPing) :
			bar.xy(-4 * vid.dupx, -2 * vid.dupy).font(Draw::Font::kThinTimer)
		)
			.align(Draw::Align::kRight)
			.text("{}", p->itemamount);
	}
}

static void K_DrawNameTagSphereMeter(INT32 x, INT32 y, INT32 width, INT32 spheres, INT32 flags)
{
	using srb2::Draw;
	Draw bar = Draw(x + vid.dupx, y).flags(V_NOSCALESTART).height(vid.dupy);

	// see also K_drawBlueSphereMeter
	const UINT8 segColors[] = {73, 64, 52, 54, 55, 35, 34, 33, 202, 180, 181, 182, 164, 165, 166, 153, 152};

	spheres = std::clamp<INT32>(spheres, 0, 40);
	int colorIndex = (spheres * sizeof segColors) / (40 + 1);

	int px = r_splitscreen > 1 ? 1 : 2;
	int b = 10 * px;
	int m = spheres * px;

	while (m > 0)
	{
		if (b > m)
			b = m;

		Draw seg = bar.width(b);

		seg.fill(segColors[std::max(colorIndex - 1, 0)]);
		seg.y(vid.dupy).fill(segColors[std::max(colorIndex - 2, 0)]);
		seg.y(2 * vid.dupy).height(2 * vid.dupy).fill(segColors[colorIndex]);
		seg.y(4 * vid.dupy).fill(31);

		bar = bar.x(b + vid.dupx);
		m -= b;
	}
}

static void K_DrawNameTagForPlayer(fixed_t x, fixed_t y, player_t *p, INT32 flags)
{
	const INT32 clr = skincolors[p->skincolor].chatcolor;
	const INT32 namelen = V_ThinStringWidth(player_names[p - players], 0);

	UINT8 *colormap = V_GetStringColormap(clr);
	INT32 barx = 0, bary = 0, barw = 0;

	UINT8 cnum = R_GetViewNumber();

	// Since there's no "V_DrawFixedFill", and I don't feel like making it,
	// fuck it, we're gonna just V_NOSCALESTART hack it
	if (r_splitscreen > 1 && cnum & 1)
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

	// see also K_CullTargetList
	if ((gametyperules & GTR_ITEMARROWS) && p->itemtype != KITEM_NONE && p->itemamount != 0)
	{
		K_DrawNameTagItemSpy(barx, bary, p, flags);
	}

	if (gametyperules & GTR_SPHERES)
	{
		K_DrawNameTagSphereMeter(barx, bary + (4 * vid.dupy), barw, p->spheres, flags);
	}

	// Lat: 10/06/2020: colormap can be NULL on the frame you join a game, just arbitrarily use palette indexes 31 and 0 instead of whatever the colormap would give us instead to avoid crashes.
	V_DrawFill(barx, bary, barw, (3 * vid.dupy), (colormap ? colormap[31] : 31)|V_NOSCALESTART|flags);
	V_DrawFill(barx, bary + vid.dupy, barw, vid.dupy, (colormap ? colormap[0] : 0)|V_NOSCALESTART|flags);
	// END DRAWFILL DUMBNESS

	// Draw the stem
	V_DrawFixedPatch(x, y, FRACUNIT, flags, kp_nametagstem, colormap);

	// Draw the name itself
	V_DrawThinStringAtFixed(x + (5*FRACUNIT), y - (26*FRACUNIT), clr|flags, player_names[p - players]);
}

playertagtype_t K_WhichPlayerTag(player_t *p)
{
	UINT8 cnum = R_GetViewNumber();

	if (!(demo.playback == true && camera[cnum].freecam == true) && P_IsDisplayPlayer(p) &&
		p != &players[displayplayers[cnum]])
	{
		return PLAYERTAG_LOCAL;
	}
	else if (p->bot)
	{
		if (p->botvars.rival == true || cv_levelskull.value)
		{
			return PLAYERTAG_RIVAL;
		}
	}
	else if (netgame || demo.playback)
	{
		if (K_ShowPlayerNametag(p) == true)
		{
			return PLAYERTAG_NAME;
		}
	}

	return PLAYERTAG_NONE;
}

void K_DrawPlayerTag(fixed_t x, fixed_t y, player_t *p, playertagtype_t type, INT32 flags)
{
	switch (type)
	{
	case PLAYERTAG_LOCAL:
		K_DrawLocalTagForPlayer(x, y, p, G_PartyPosition(p - players));
		break;

	case PLAYERTAG_RIVAL:
		K_DrawRivalTagForPlayer(x, y);
		break;

	case PLAYERTAG_NAME:
		K_DrawNameTagForPlayer(x, y, p, flags);
		K_DrawTypingNotifier(x, y, p, flags);
		break;

	default:
		break;
	}
}

typedef struct weakspotdraw_t
{
	UINT8 i;
	INT32 x;
	INT32 y;
	boolean candrawtag;
} weakspotdraw_t;

static void K_DrawWeakSpot(weakspotdraw_t *ws)
{
	UINT8 *colormap;
	UINT8 j = (bossinfo.weakspots[ws->i].type == SPOT_BUMP) ? 1 : 0;
	tic_t flashtime = ~1; // arbitrary high even number

	if (bossinfo.weakspots[ws->i].time < TICRATE)
	{
		if (bossinfo.weakspots[ws->i].time & 1)
			return;

		flashtime = bossinfo.weakspots[ws->i].time;
	}
	else if (bossinfo.weakspots[ws->i].time > (WEAKSPOTANIMTIME - TICRATE))
		flashtime = WEAKSPOTANIMTIME - bossinfo.weakspots[ws->i].time;

	if (flashtime & 1)
		colormap = R_GetTranslationColormap(TC_ALLWHITE, SKINCOLOR_NONE, GTC_CACHE);
	else
		colormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(bossinfo.weakspots[ws->i].color), GTC_CACHE);

	V_DrawFixedPatch(ws->x, ws->y, FRACUNIT, 0, kp_bossret[j], colormap);

	if (!ws->candrawtag || flashtime & 1 || flashtime < TICRATE/2)
		return;

	V_DrawFixedPatch(ws->x, ws->y, FRACUNIT, 0, kp_bossret[j+1], colormap);
}

static void K_drawKartNameTags(void)
{
	vector3_t c;
	UINT8 cnum = R_GetViewNumber();
	size_t i, j;

	if (stplyr == NULL || stplyr->mo == NULL || P_MobjWasRemoved(stplyr->mo))
	{
		return;
	}

	if (stplyr->awayview.tics)
	{
		return;
	}

	// Crop within splitscreen bounds
	switch (r_splitscreen)
	{
		case 1:
			V_SetClipRect(
				0,
				cnum == 1 ? (BASEVIDHEIGHT / 2) * FRACUNIT : 0,
				BASEVIDWIDTH * FRACUNIT,
				(BASEVIDHEIGHT / 2) * FRACUNIT,
				0
			);
			break;

		case 2:
		case 3:
			V_SetClipRect(
				cnum & 1 ? (BASEVIDWIDTH / 2) * FRACUNIT : 0,
				cnum > 1 ? (BASEVIDHEIGHT / 2) * FRACUNIT : 0,
				(BASEVIDWIDTH / 2) * FRACUNIT,
				(BASEVIDHEIGHT / 2) * FRACUNIT,
				0
			);
			break;
	}

	c.x = viewx;
	c.y = viewy;
	c.z = viewz;

	// Maybe shouldn't be handling this here... but the camera info is too good.
	if (bossinfo.valid == true)
	{
		weakspotdraw_t weakspotdraw[NUMWEAKSPOTS];
		UINT8 numdraw = 0;
		boolean onleft = false;

		for (i = 0; i < NUMWEAKSPOTS; i++)
		{
			trackingResult_t result;
			vector3_t v;

			if (bossinfo.weakspots[i].spot == NULL || P_MobjWasRemoved(bossinfo.weakspots[i].spot))
			{
				// No object
				continue;
			}

			if (bossinfo.weakspots[i].time == 0 || bossinfo.weakspots[i].type == SPOT_NONE)
			{
				// not visible
				continue;
			}

			v.x = R_InterpolateFixed(bossinfo.weakspots[i].spot->old_x, bossinfo.weakspots[i].spot->x);
			v.y = R_InterpolateFixed(bossinfo.weakspots[i].spot->old_y, bossinfo.weakspots[i].spot->y);
			v.z = R_InterpolateFixed(bossinfo.weakspots[i].spot->old_z, bossinfo.weakspots[i].spot->z);

			v.z += (bossinfo.weakspots[i].spot->height / 2);

			K_ObjectTracking(&result, &v, false);
			if (result.onScreen == false)
			{
				continue;
			}

			weakspotdraw[numdraw].i = i;
			weakspotdraw[numdraw].x = result.x;
			weakspotdraw[numdraw].y = result.y;
			weakspotdraw[numdraw].candrawtag = true;

			for (j = 0; j < numdraw; j++)
			{
				if (abs(weakspotdraw[j].x - weakspotdraw[numdraw].x) > 50*FRACUNIT)
				{
					continue;
				}

				onleft = (weakspotdraw[j].x < weakspotdraw[numdraw].x);

				if (abs((onleft ? -5 : 5)
					+ weakspotdraw[j].y - weakspotdraw[numdraw].y) > 18*FRACUNIT)
				{
					continue;
				}

				if (weakspotdraw[j].x < weakspotdraw[numdraw].x)
				{
					weakspotdraw[j].candrawtag = false;
					break;
				}

				weakspotdraw[numdraw].candrawtag = false;
				break;
			}

			numdraw++;
		}

		for (i = 0; i < numdraw; i++)
		{
			K_DrawWeakSpot(&weakspotdraw[i]);
		}
	}

	K_drawTargetHUD(&c, stplyr);

	V_ClearClipRect();
}

#define PROGRESSION_BAR_WIDTH 120

static INT32 K_getKartProgressionMinimapDistance(UINT32 distancetofinish)
{
	INT32 dist;

	if (specialstageinfo.maxDist == 0U)
	{
		return 0;
	}

	dist = specialstageinfo.maxDist/PROGRESSION_BAR_WIDTH;

	dist = (specialstageinfo.maxDist-distancetofinish)/dist;

	if (dist > PROGRESSION_BAR_WIDTH)
	{
		return PROGRESSION_BAR_WIDTH;
	}

	if (dist < 0)
	{
		return 0;
	}

	return dist;
}

static void K_drawKartProgressionMinimapIcon(UINT32 distancetofinish, INT32 hudx, INT32 hudy, INT32 flags, patch_t *icon, UINT8 *colormap)
{
	if (distancetofinish == UINT32_MAX)
		return;

	hudx += K_getKartProgressionMinimapDistance(distancetofinish);

	hudx = ((hudx - (SHORT(icon->width)/2))<<FRACBITS);
	hudy = ((hudy - (SHORT(icon->height)/2))<<FRACBITS);

	V_DrawFixedPatch(hudx, hudy, FRACUNIT, flags, icon, colormap);
}

static void K_drawKartMinimapIcon(fixed_t objx, fixed_t objy, INT32 hudx, INT32 hudy, INT32 flags, patch_t *icon, UINT8 *colormap)
{
	// amnum xpos & ypos are the icon's speed around the HUD.
	// The number being divided by is for how fast it moves.
	// The higher the number, the slower it moves.

	// am xpos & ypos are the icon's starting position. Withouht
	// it, they wouldn't 'spawn' on the top-right side of the HUD.

	fixed_t amnumxpos, amnumypos;
	INT32 amxpos, amypos;

	amnumxpos = (FixedMul(objx, minimapinfo.zoom) - minimapinfo.offs_x);
	amnumypos = -(FixedMul(objy, minimapinfo.zoom) - minimapinfo.offs_y);

	if (encoremode)
		amnumxpos = -amnumxpos;

	amxpos = amnumxpos + ((hudx - (SHORT(icon->width))/2)<<FRACBITS);
	amypos = amnumypos + ((hudy - (SHORT(icon->height))/2)<<FRACBITS);

	V_DrawFixedPatch(amxpos, amypos, FRACUNIT, flags, icon, colormap);
}

static void K_drawKartMinimapDot(fixed_t objx, fixed_t objy, INT32 hudx, INT32 hudy, INT32 flags, UINT8 color, UINT8 size)
{
	fixed_t amnumxpos, amnumypos;
	INT32 amxpos, amypos;

	amnumxpos = (FixedMul(objx, minimapinfo.zoom) - minimapinfo.offs_x);
	amnumypos = -(FixedMul(objy, minimapinfo.zoom) - minimapinfo.offs_y);

	if (encoremode)
		amnumxpos = -amnumxpos;

	amxpos = (amnumxpos / FRACUNIT);
	amypos = (amnumypos / FRACUNIT);

	if (flags & V_NOSCALESTART)
	{
		amxpos *= vid.dupx;
		amypos *= vid.dupy;
	}

	V_DrawFill((amxpos + hudx) - (size / 2), (amypos + hudy) - (size / 2), size, size, flags | color);
}

static UINT8 K_RankMinimapWaypoint(waypoint_t *wp)
{
	if (wp == stplyr->nextwaypoint)
	{
		return 4;
	}
	else if (wp->numnextwaypoints == 0 || wp->numprevwaypoints == 0)
	{
		return 3;
	}
	else if (!K_GetWaypointIsEnabled(wp)) // disabled
	{
		return 2;
	}
	else if (K_GetWaypointIsShortcut(wp)) // shortcut
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static void K_drawKartMinimapWaypoint(waypoint_t *wp, UINT8 rank, INT32 hudx, INT32 hudy, INT32 flags)
{
	static UINT8 colors[] =
	{
		0x95, // blue (0 - default)
		0x20, // pink (1 - shortcut)
		0x10, // gray (2 - disabled)
		0x40, // yellow (3 - error)
		0x70, // green (4 - player)
	};

	UINT8 pal = colors[rank]; // blue
	UINT8 size = 3;

	if (rank == 4)
	{
		size = 6;
	}

	if (!(flags & V_NOSCALESTART))
	{
		hudx *= vid.dupx;
		hudy *= vid.dupy;
	}

	K_drawKartMinimapDot(wp->mobj->x, wp->mobj->y, hudx, hudy, flags | V_NOSCALESTART, pal, size);
}

#define ICON_DOT_RADIUS (10)

static void K_drawKartMinimap(void)
{
	patch_t *workingPic;

	INT32 i = 0;
	INT32 x, y;

	INT32 minimaptrans = 4;
	INT32 splitflags = 0;

	UINT8 skin = 0;
	UINT8 *colormap = NULL;

	SINT8 localplayers[MAXSPLITSCREENPLAYERS];
	SINT8 numlocalplayers = 0;

	mobj_t *mobj, *next;	// for SPB drawing (or any other item(s) we may wanna draw, I dunno!)

	fixed_t interpx, interpy;

	boolean doprogressionbar = false;
	boolean dofade = false, doencore = false;

	UINT8 minipal;

	// Draw the HUD only when playing in a level.
	// hu_stuff needs this, unlike st_stuff.
	if (gamestate != GS_LEVEL)
		return;

	// Only draw for the first player
	// Maybe move this somewhere else where this won't be a concern?
	if (R_GetViewNumber() != 0)
		return;

	if (specialstageinfo.valid == true)
	{
		// future work: maybe make this a unique gametype rule?
		// I would do this now if it were easier to get the
		// distancetofinish for an arbitrary object. ~toast 070423
		doprogressionbar = true;
	}

	if (doprogressionbar == false)
	{
		if (minimapinfo.minimap_pic == NULL)
		{
			return; // no pic, just get outta here
		}

		else if (r_splitscreen < 1) // 1P right aligned
		{
			splitflags = (V_SLIDEIN|V_SNAPTORIGHT);
		}
		else // 2/4P splits
		{
			if (r_splitscreen == 1)
				splitflags = V_SNAPTORIGHT; // 2P right aligned

			dofade = true;
		}
		// 3P lives in the middle of the bottom right
		// viewport and shouldn't fade in OR slide

		x = MINI_X;
		y = MINI_Y;

		workingPic = minimapinfo.minimap_pic;

		doencore = encoremode;
	}
	else
	{
		x = BASEVIDWIDTH/2;

		if (r_splitscreen > 0)
		{
			y = BASEVIDHEIGHT/2;
			dofade = true;
		}
		else
		{
			y = 180;
			splitflags = (V_SLIDEIN|V_SNAPTOBOTTOM);
		}

		workingPic = kp_wouldyoustillcatchmeifiwereaworm;
	}

	if (dofade)
	{
		minimaptrans = FixedMul(minimaptrans, (st_translucency * FRACUNIT) / 10);

		if (!minimaptrans)
			return;
	}

	minimaptrans = ((10-minimaptrans)<<V_ALPHASHIFT);

	// Really looking forward to never writing this loop again
	UINT8 bestplayer = MAXPLAYERS;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		if (players[i].spectator)
			continue;
		if (players[i].position == 1)
			bestplayer = i;
	}

	if (bestplayer == MAXPLAYERS || leveltime < starttime) // POSITION / no players
		minipal = ((leveltime/10)%2) ? SKINCOLOR_WHITE : SKINCOLOR_BLACK;
	else if (players[bestplayer].laps >= numlaps) // Final lap
		minipal = K_RainbowColor(leveltime);
	else // Standard: color to leader
		minipal = players[bestplayer].skincolor;

	if (doencore)
	{
		V_DrawFixedPatch(
			(x + (SHORT(workingPic->width)/2))*FRACUNIT,
			(y - (SHORT(workingPic->height)/2))*FRACUNIT,
			FRACUNIT,
			splitflags|minimaptrans|V_FLIP,
			workingPic,
			R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(minipal), GTC_CACHE)
		);
	}
	else
	{
		V_DrawFixedPatch(
			(x - (SHORT(workingPic->width)/2))*FRACUNIT,
			(y - (SHORT(workingPic->height)/2))*FRACUNIT,
			FRACUNIT,
			splitflags|minimaptrans,
			workingPic,
			R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(minipal), GTC_CACHE)
		);
	}

	// most icons will be rendered semi-ghostly.
	splitflags |= V_HUDTRANSHALF;

	// let offsets transfer to the heads, too!
	if (doencore)
		x += SHORT(workingPic->leftoffset);
	else
		x -= SHORT(workingPic->leftoffset);
	y -= SHORT(workingPic->topoffset);

	if (doprogressionbar == true)
	{
		x -= PROGRESSION_BAR_WIDTH/2;
	}

	// Draw the super item in Battle
	if (doprogressionbar == false && (gametyperules & GTR_OVERTIME) && battleovertime.enabled)
	{
		if (battleovertime.enabled >= 10*TICRATE || (battleovertime.enabled & 1))
		{
			const INT32 prevsplitflags = splitflags;
			splitflags &= ~V_HUDTRANSHALF;
			splitflags |= V_HUDTRANS;
			colormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(K_RainbowColor(leveltime)), GTC_CACHE);
			K_drawKartMinimapIcon(battleovertime.x, battleovertime.y, x, y, splitflags, kp_itemminimap, colormap);
			splitflags = prevsplitflags;
		}
	}

	// initialize
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
		localplayers[i] = -1;

	// Player's tiny icons on the Automap. (drawn opposite direction so player 1 is drawn last in splitscreen)
	if (ghosts && doprogressionbar == false) // future work: show ghosts on progression bar
	{
		demoghost *g = ghosts;
		while (g)
		{
			if (g->mo && !P_MobjWasRemoved(g->mo) && g->mo->skin)
			{
				skin = ((skin_t*)g->mo->skin)-skins;

				workingPic = R_CanShowSkinInDemo(skin) ? faceprefix[skin][FACE_MINIMAP] : kp_unknownminimap;

				if (g->mo->color)
				{
					if (g->mo->colorized)
						colormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(g->mo->color), GTC_CACHE);
					else
						colormap = R_GetTranslationColormap(skin, static_cast<skincolornum_t>(g->mo->color), GTC_CACHE);
				}
				else
					colormap = NULL;

				interpx = R_InterpolateFixed(g->mo->old_x, g->mo->x);
				interpy = R_InterpolateFixed(g->mo->old_y, g->mo->y);

				K_drawKartMinimapIcon(interpx, interpy, x, y, splitflags, workingPic, colormap);
			}

			g = g->next;
		}
	}

	{
		for (i = MAXPLAYERS-1; i >= 0; i--)
		{
			if (!playeringame[i])
				continue;
			if (!players[i].mo || players[i].spectator || !players[i].mo->skin
			|| (doprogressionbar == false && players[i].exiting))
				continue;

			// This player is out of the game!
			if ((gametyperules & GTR_BUMPERS) && (players[i].pflags & PF_ELIMINATED))
				continue;

			// This gets set for a player who has GAME OVER'd
			if (P_MobjIsReappearing(players[i].mo))
				continue;

			if (i == displayplayers[0] || i == displayplayers[1] || i == displayplayers[2] || i == displayplayers[3])
			{
				// Draw display players on top of everything else
				localplayers[numlocalplayers++] = i;
				continue;
			}

			if (players[i].hyudorotimer > 0)
			{
				if (!((players[i].hyudorotimer < TICRATE/2
					|| players[i].hyudorotimer > hyudorotime-(TICRATE/2))
					&& !(leveltime & 1)))
					continue;
			}

			mobj = players[i].mo;

			if (mobj->health <= 0 && (players[i].pflags & PF_NOCONTEST))
			{
				if (P_MobjWasRemoved(mobj->tracer))
				{
					continue;
				}

				if (mobj->tracer->renderflags & RF_DONTDRAW)
				{
					continue;
				}

				workingPic = kp_nocontestminimap;
				colormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(mobj->color), GTC_CACHE);

				mobj = mobj->tracer;
			}
			else
			{
				skin = ((skin_t*)mobj->skin)-skins;

				workingPic = R_CanShowSkinInDemo(skin) ? faceprefix[skin][FACE_MINIMAP] : kp_unknownminimap;

				if (mobj->color)
				{
					if (mobj->colorized)
						colormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(mobj->color), GTC_CACHE);
					else
						colormap = R_GetTranslationColormap(skin, static_cast<skincolornum_t>(mobj->color), GTC_CACHE);
				}
				else
					colormap = NULL;
			}

			if (doprogressionbar == false)
			{
				interpx = R_InterpolateFixed(mobj->old_x, mobj->x);
				interpy = R_InterpolateFixed(mobj->old_y, mobj->y);

				K_drawKartMinimapIcon(interpx, interpy, x, y, splitflags, workingPic, colormap);

				// Target reticule
				if (((gametyperules & GTR_CIRCUIT) && players[i].position == spbplace)
					|| ((gametyperules & (GTR_BOSS|GTR_POINTLIMIT)) == GTR_POINTLIMIT && K_IsPlayerWanted(&players[i])))
				{
					K_drawKartMinimapIcon(interpx, interpy, x, y, splitflags, kp_wantedreticle, NULL);
				}
			}
			else
			{
				K_drawKartProgressionMinimapIcon(players[i].distancetofinish, x, y, splitflags, workingPic, colormap);
			}
		}
	}

	// draw minimap-pertinent objects
	if (doprogressionbar == true)
	{
		// future work: support these specific objects on this
	}
	else for (mobj = trackercap; mobj; mobj = next)
	{
		next = mobj->itnext;

		workingPic = NULL;
		colormap = NULL;

		if (mobj->health <= 0)
			continue;

		switch (mobj->type)
		{
			case MT_SPB:
				workingPic = kp_spbminimap;
#if 0
				if (mobj->target && !P_MobjWasRemoved(mobj->target) && mobj->target->player && mobj->target->player->skincolor)
				{
					colormap = R_GetTranslationColormap(TC_RAINBOW, mobj->target->player->skincolor, GTC_CACHE);
				}
				else
#endif
				if (mobj->color)
				{
					colormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(mobj->color), GTC_CACHE);
				}

				break;
			case MT_BATTLECAPSULE:
				workingPic = kp_capsuleminimap[(mobj->extravalue1 != 0 ? 1 : 0)];
				break;
			case MT_CDUFO:
				if (battleprisons)
					workingPic = kp_capsuleminimap[2];
				break;
			case MT_BATTLEUFO:
				workingPic = kp_battleufominimap;
				break;
			case MT_SUPER_FLICKY:
				workingPic = kp_superflickyminimap;
				if (mobj_t* owner = Obj_SuperFlickyOwner(mobj); owner && owner->color)
				{
					colormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(owner->color), GTC_CACHE);
				}
				break;
			default:
				break;
		}

		if (!workingPic)
			continue;

		interpx = R_InterpolateFixed(mobj->old_x, mobj->x);
		interpy = R_InterpolateFixed(mobj->old_y, mobj->y);

		K_drawKartMinimapIcon(interpx, interpy, x, y, splitflags, workingPic, colormap);
	}

	// draw our local players here, opaque.
	{
		splitflags &= ~V_HUDTRANSHALF;
		splitflags |= V_HUDTRANS;
	}

	// ...but first, any boss targets.
	if (doprogressionbar == true)
	{
		if (specialstageinfo.valid == true)
		{
			UINT32 distancetofinish = K_GetSpecialUFODistance();
			if (distancetofinish > 0 && specialstageinfo.ufo != NULL && P_MobjWasRemoved(specialstageinfo.ufo) == false)
			{
				colormap = NULL;
				if (specialstageinfo.ufo->health > 1)
				{
					workingPic = kp_catcherminimap;
				}
				else
				{
					UINT8 emid = 0;
					if (specialstageinfo.ufo->cvmem > 7)
						emid = 1;
					workingPic = kp_emeraldminimap[emid];

					if (specialstageinfo.ufo->color)
					{
						colormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(specialstageinfo.ufo->color), GTC_CACHE);
					}
				}

				K_drawKartProgressionMinimapIcon(distancetofinish, x, y, splitflags, workingPic, colormap);
			}
		}

		// future work: support boss minimap icons on the progression bar
	}
	else if (bossinfo.valid == true)
	{
		for (i = 0; i < NUMWEAKSPOTS; i++)
		{
			// exists at all?
			if (bossinfo.weakspots[i].spot == NULL || P_MobjWasRemoved(bossinfo.weakspots[i].spot))
				continue;
			// shows on the minimap?
			if (bossinfo.weakspots[i].minimap == false)
				continue;
			// in the flashing period?
			if ((bossinfo.weakspots[i].time > (WEAKSPOTANIMTIME-(TICRATE/2))) && (bossinfo.weakspots[i].time & 1))
				continue;

			colormap = NULL;

			if (bossinfo.weakspots[i].color)
				colormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(bossinfo.weakspots[i].color), GTC_CACHE);

			interpx = R_InterpolateFixed(bossinfo.weakspots[i].spot->old_x, bossinfo.weakspots[i].spot->x);
			interpy = R_InterpolateFixed(bossinfo.weakspots[i].spot->old_y, bossinfo.weakspots[i].spot->y);

			// temporary graphic?
			K_drawKartMinimapIcon(interpx, interpy, x, y, splitflags, kp_wantedreticle, colormap);
		}
	}

	for (i = 0; i < numlocalplayers; i++)
	{
		boolean nocontest = false;

		if (localplayers[i] == -1)
			continue; // this doesn't interest us

		if ((players[localplayers[i]].hyudorotimer > 0) && (leveltime & 1))
			continue;

		mobj = players[localplayers[i]].mo;

		// This gets set for a player who has GAME OVER'd
		if (P_MobjIsReappearing(mobj))
			continue;

		if (mobj->health <= 0 && (players[localplayers[i]].pflags & PF_NOCONTEST))
		{
			if (P_MobjWasRemoved(mobj->tracer))
			{
				continue;
			}

			if (mobj->tracer->renderflags & RF_DONTDRAW)
			{
				continue;
			}

			workingPic = kp_nocontestminimap;
			colormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(mobj->color), GTC_CACHE);

			mobj = mobj->tracer;

			nocontest = true;
		}
		else
		{
			skin = ((skin_t*)mobj->skin)-skins;

			workingPic = R_CanShowSkinInDemo(skin) ? faceprefix[skin][FACE_MINIMAP] : kp_unknownminimap;

			if (mobj->color)
			{
				if (mobj->colorized)
					colormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(mobj->color), GTC_CACHE);
				else
					colormap = R_GetTranslationColormap(skin, static_cast<skincolornum_t>(mobj->color), GTC_CACHE);
			}
			else
				colormap = NULL;
		}

		if (doprogressionbar == false)
		{
			interpx = R_InterpolateFixed(mobj->old_x, mobj->x);
			interpy = R_InterpolateFixed(mobj->old_y, mobj->y);

			K_drawKartMinimapIcon(interpx, interpy, x, y, splitflags, workingPic, colormap);

			// Target reticule
			if (((gametyperules & GTR_CIRCUIT) && players[localplayers[i]].position == spbplace)
				|| ((gametyperules & (GTR_BOSS|GTR_POINTLIMIT)) == GTR_POINTLIMIT && K_IsPlayerWanted(&players[localplayers[i]])))
			{
				K_drawKartMinimapIcon(interpx, interpy, x, y, splitflags, kp_wantedreticle, NULL);
			}

			if (!nocontest)
			{
				angle_t ang = R_InterpolateAngle(mobj->old_angle, mobj->angle);
				if (encoremode)
					ang = ANGLE_180 - ang;

				K_drawKartMinimapIcon(
						interpx,
						interpy,
						x + FixedMul(FCOS(ang), ICON_DOT_RADIUS),
						y - FixedMul(FSIN(ang), ICON_DOT_RADIUS),
						splitflags,
						kp_minimapdot,
						colormap
				);
			}
		}
		else
		{
			K_drawKartProgressionMinimapIcon(players[localplayers[i]].distancetofinish, x, y, splitflags, workingPic, colormap);
		}
	}

	if (doprogressionbar == false && cv_kartdebugwaypoints.value != 0)
	{
		struct MiniWaypoint
		{
			waypoint_t* waypoint;
			UINT8 rank;

			MiniWaypoint(waypoint_t* wp) : waypoint(wp), rank(K_RankMinimapWaypoint(wp)) {}

			bool operator<(const MiniWaypoint& b) const noexcept { return rank < b.rank; }
		};

		std::vector<MiniWaypoint> waypoints;
		size_t idx;

		waypoints.reserve(K_GetNumWaypoints());

		for (idx = 0; idx < K_GetNumWaypoints(); ++idx)
		{
			waypoint_t *wp = K_GetWaypointFromIndex(idx);

			I_Assert(wp != NULL);

			waypoints.push_back(wp);
		}

		std::sort(waypoints.begin(), waypoints.end());

		for (MiniWaypoint& wp : waypoints)
		{
			K_drawKartMinimapWaypoint(wp.waypoint, wp.rank, x, y, splitflags);
		}
	}
}

#undef PROGRESSION_BAR_WIDTH

static void K_drawKartFinish(boolean finish)
{
	INT32 timer, minsplitstationary, pnum = 0, splitflags = V_SPLITSCREEN;
	patch_t **kptodraw;

	if (finish)
	{
		if (gametyperules & GTR_SPECIALSTART)
			return;

		timer = stplyr->karthud[khud_finish];
		kptodraw = kp_racefinish;
		minsplitstationary = 2;
	}
	else
	{
		timer = stplyr->karthud[khud_fault];
		kptodraw = kp_racefault;
		minsplitstationary = 1;
	}

	if (!timer || timer > 2*TICRATE)
		return;

	if ((timer % (2*5)) / 5) // blink
		pnum = 1;

	if (r_splitscreen > 0)
		pnum += (r_splitscreen > 1) ? 2 : 4;

	if (r_splitscreen >= minsplitstationary) // 3/4p, stationary FIN
	{
		V_DrawScaledPatch(STCD_X - (SHORT(kptodraw[pnum]->width)/2), STCD_Y - (SHORT(kptodraw[pnum]->height)/2), splitflags, kptodraw[pnum]);
		return;
	}

	//else -- 1/2p, scrolling FINISH
	{
		INT32 x, xval, ox, interpx, pwidth;

		x = ((vid.width<<FRACBITS)/vid.dupx);
		xval = (SHORT(kptodraw[pnum]->width)<<FRACBITS);

		pwidth = std::max(xval, x);

		x = ((TICRATE - timer) * pwidth) / TICRATE;
		ox = ((TICRATE - (timer - 1)) * pwidth) / TICRATE;

		interpx = R_InterpolateFixed(ox, x);

		if (r_splitscreen && R_GetViewNumber() == 1)
			interpx = -interpx;

		V_DrawFixedPatch(interpx + (STCD_X<<FRACBITS) - (pwidth / 2),
			(STCD_Y<<FRACBITS) - (SHORT(kptodraw[pnum]->height)<<(FRACBITS-1)),
			FRACUNIT,
			splitflags, kptodraw[pnum], NULL);
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

				// Reduce VFX disables the bulb animation while still presenting this indicator

				if (bulbtic > length)
				{
					bulbtic -= length;

					if (cv_reducevfx.value != 0)
					{
						patchnum = chillloop_animation[0];
					}
					else
					{
						patchnum = chillloop_animation[bulbtic % 2];
					}
				}
				else
				{
					if (cv_reducevfx.value != 0)
					{
						patchnum = loop_animation[0];
					}
					else
					{
						patchnum = loop_animation[bulbtic % 4];
					}
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

	if (leveltime >= introtime && leveltime < starttime-(3*TICRATE))
	{
		if (numbulbs > 1)
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

			if (inDuel == true)
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

static void K_drawKartFirstPerson(void)
{
	static INT32 pnum[4], turn[4], drift[4];
	const INT16 steerThreshold = KART_FULLTURN / 2;
	INT32 pn = 0, tn = 0, dr = 0;
	INT32 target = 0, splitflags = V_SNAPTOBOTTOM|V_SPLITSCREEN;
	INT32 x = BASEVIDWIDTH/2, y = BASEVIDHEIGHT;
	fixed_t scale;
	UINT8 *colmap = NULL;

	if (stplyr->spectator || !stplyr->mo || (stplyr->mo->renderflags & RF_DONTDRAW))
		return;

	{
		UINT8 view = R_GetViewNumber();
		pn = pnum[view];
		tn = turn[view];
		dr = drift[view];
	}

	if (r_splitscreen)
	{
		y >>= 1;
		if (r_splitscreen > 1)
			x >>= 1;
	}

	{
		if (stplyr->speed < (20*stplyr->mo->scale) && (leveltime & 1) && !r_splitscreen)
			y++;

		if (stplyr->mo->renderflags & RF_TRANSMASK)
			splitflags |= ((stplyr->mo->renderflags & RF_TRANSMASK) >> RF_TRANSSHIFT) << FF_TRANSSHIFT;
		else if (stplyr->mo->frame & FF_TRANSMASK)
			splitflags |= (stplyr->mo->frame & FF_TRANSMASK);
	}

	if (stplyr->steering > steerThreshold) // strong left turn
		target = 2;
	else if (stplyr->steering < -steerThreshold) // strong right turn
		target = -2;
	else if (stplyr->steering > 0) // weak left turn
		target = 1;
	else if (stplyr->steering < 0) // weak right turn
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

	if (tn != stplyr->steering/50)
		tn -= (tn - (stplyr->steering/50))/8;

	if (dr != stplyr->drift*16)
		dr -= (dr - (stplyr->drift*16))/8;

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
		UINT8 driftcolor = K_DriftSparkColor(stplyr, stplyr->driftcharge);
		const angle_t ang = R_PointToAngle2(0, 0, stplyr->rmomx, stplyr->rmomy) - stplyr->drawangle;
		// yes, the following is correct. no, you do not need to swap the x and y.
		fixed_t xoffs = -P_ReturnThrustY(stplyr->mo, ang, (BASEVIDWIDTH<<(FRACBITS-2))/2);
		fixed_t yoffs = -P_ReturnThrustX(stplyr->mo, ang, 4*FRACUNIT);

		// hitlag vibrating
		if (stplyr->mo->hitlag > 0 && (stplyr->mo->eflags & MFE_DAMAGEHITLAG))
		{
			fixed_t mul = stplyr->mo->hitlag * HITLAGJITTERS;
			if (r_splitscreen && mul > FRACUNIT)
				mul = FRACUNIT;

			if (leveltime & 1)
			{
				mul = -mul;
			}

			xoffs = FixedMul(xoffs, mul);
			yoffs = FixedMul(yoffs, mul);

		}

		if ((yoffs += 4*FRACUNIT) < 0)
			yoffs = 0;

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
			colmap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(driftcolor), GTC_CACHE);
		else if (stplyr->mo->colorized && stplyr->mo->color) // invincibility/grow/shrink!
			colmap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(stplyr->mo->color), GTC_CACHE);
	}

	V_DrawFixedPatch(x, y, scale, splitflags, kp_fpview[target], colmap);

	{
		UINT8 view = R_GetViewNumber();
		pnum[view] = pn;
		turn[view] = tn;
		drift[view] = dr;
	}
}

static void K_drawInput(void)
{
	UINT8 viewnum = R_GetViewNumber();
	boolean freecam = camera[viewnum].freecam;	//disable some hud elements w/ freecam

	if (!cv_drawinput.value && !modeattacking && gametype != GT_TUTORIAL)
		return;

	if (stplyr->spectator || freecam || demo.attract)
		return;

	INT32 def[4][3] = {
		{247, 156, V_SNAPTOBOTTOM | V_SNAPTORIGHT}, // 1p
		{247, 56, V_SNAPTOBOTTOM | V_SNAPTORIGHT}, // 2p
		{6, 52, V_SNAPTOBOTTOM | V_SNAPTOLEFT}, // 4p left
		{282 - BASEVIDWIDTH/2, 52, V_SNAPTOBOTTOM | V_SNAPTORIGHT}, // 4p right
	};
	INT32 k = r_splitscreen <= 1 ? r_splitscreen : 2 + (viewnum & 1);
	INT32 flags = def[k][2] | V_SPLITSCREEN;
	char mode = ((stplyr->pflags & PF_ANALOGSTICK) ? '4' : '2') + (r_splitscreen > 1);
	bool local = !demo.playback && P_IsMachineLocalPlayer(stplyr);
	fixed_t slide = K_GetDialogueSlide(FRACUNIT);
	INT32 tallySlide = []() -> INT32
	{
		if (r_splitscreen <= 1)
		{
			return 0;
		}
		if (!stplyr->tally.active)
		{
			return 0;
		}
		constexpr INT32 kSlideDown = 22;
		if (stplyr->tally.state == TALLY_ST_GOTTHRU_SLIDEIN ||
			stplyr->tally.state == TALLY_ST_GAMEOVER_SLIDEIN)
		{
			return static_cast<INT32>(Easing_OutQuad(std::min<fixed_t>(stplyr->tally.transition * 2, FRACUNIT), 0, kSlideDown));
		}
		return kSlideDown;
	}();
	if (slide)
		flags &= ~(V_SNAPTORIGHT); // don't draw underneath the dialogue box in non-green resolutions

	// Move above the boss health bar.
	// TODO: boss HUD only works in 1P, so this only works in 1P too.
	if (LUA_HudEnabled(hud_position) && bossinfo.valid)
	{
		constexpr tic_t kDelay = 2u;
		// See K_drawBossHealthBar
		tic_t start = lt_endtime - 1u;
		tic_t t = std::clamp(lt_ticker, start, start + kDelay) - start;
		def[0][1] -= 24 + Easing_Linear(t * FRACUNIT / kDelay, 0, 7);
	}

	K_DrawInputDisplay(
		def[k][0] - FixedToFloat(34 * slide),
		def[k][1] - FixedToFloat(51 * slide) + tallySlide,
		flags,
		mode,
		(local ? G_LocalSplitscreenPartyPosition : G_PartyPosition)(stplyr - players),
		local,
		stplyr->speed > 0
	);
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
	const UINT8 offset = std::min<UINT32>(52-1u, (3*TICRATE)-mapreset);

	V_DrawFadeScreen(0xFF00, 16); // Fade out
	V_DrawScaledPatch(0, 0, 0, kp_challenger[anim[offset]]);
}

static void K_drawLapStartAnim(void)
{
	if (demo.attract == DEMO_ATTRACT_CREDITS)
	{
		return;
	}

	// This is an EVEN MORE insanely complicated animation.
	const UINT8 t = stplyr->karthud[khud_lapanimation];
	const UINT8 progress = 80 - t;

	const UINT8 tOld = t + 1;
	const UINT8 progressOld = 80 - tOld;

	const tic_t leveltimeOld = leveltime - 1;

	UINT8 *colormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(stplyr->skincolor), GTC_CACHE);

	fixed_t interpx, interpy, newval, oldval;

	newval = (BASEVIDWIDTH/2 + (32 * std::max(0, t - 76))) * FRACUNIT;
	oldval = (BASEVIDWIDTH/2 + (32 * std::max(0, tOld - 76))) * FRACUNIT;
	interpx = R_InterpolateFixed(oldval, newval);

	newval = (48 - (32 * std::max(0, progress - 76))) * FRACUNIT;
	oldval = (48 - (32 * std::max(0, progressOld - 76))) * FRACUNIT;
	interpy = R_InterpolateFixed(oldval, newval);

	V_DrawFixedPatch(
		interpx, interpy,
		FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
		(modeattacking ? kp_lapanim_emblem[1] : kp_lapanim_emblem[0]), colormap);

	if (stplyr->karthud[khud_laphand] >= 1 && stplyr->karthud[khud_laphand] <= 3)
	{
		newval = (4 - abs((signed)((leveltime % 8) - 4))) * FRACUNIT;
		oldval = (4 - abs((signed)((leveltimeOld % 8) - 4))) * FRACUNIT;
		interpy += R_InterpolateFixed(oldval, newval);

		V_DrawFixedPatch(
			interpx, interpy,
			FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
			kp_lapanim_hand[stplyr->karthud[khud_laphand]-1], NULL);
	}

	if (stplyr->latestlap == (UINT8)(numlaps))
	{
		newval = (62 - (32 * std::max(0, progress - 76))) * FRACUNIT;
		oldval = (62 - (32 * std::max(0, progressOld - 76))) * FRACUNIT;
		interpx = R_InterpolateFixed(oldval, newval);

		V_DrawFixedPatch(
			interpx, // 27
			30*FRACUNIT, // 24
			FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
			kp_lapanim_final[std::min(progress/2, 10)], NULL);

		if (progress/2-12 >= 0)
		{
			newval = (188 + (32 * std::max(0, progress - 76))) * FRACUNIT;
			oldval = (188 + (32 * std::max(0, progressOld - 76))) * FRACUNIT;
			interpx = R_InterpolateFixed(oldval, newval);

			V_DrawFixedPatch(
				interpx, // 194
				30*FRACUNIT, // 24
				FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
				kp_lapanim_lap[std::min(progress/2-12, 6)], NULL);
		}
	}
	else
	{
		newval = (82 - (32 * std::max(0, progress - 76))) * FRACUNIT;
		oldval = (82 - (32 * std::max(0, progressOld - 76))) * FRACUNIT;
		interpx = R_InterpolateFixed(oldval, newval);

		V_DrawFixedPatch(
			interpx, // 61
			30*FRACUNIT, // 24
			FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
			kp_lapanim_lap[std::min(progress/2, 6)], NULL);

		if (progress/2-8 >= 0)
		{
			newval = (188 + (32 * std::max(0, progress - 76))) * FRACUNIT;
			oldval = (188 + (32 * std::max(0, progressOld - 76))) * FRACUNIT;
			interpx = R_InterpolateFixed(oldval, newval);

			V_DrawFixedPatch(
				interpx, // 194
				30*FRACUNIT, // 24
				FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
				kp_lapanim_number[(((UINT32)stplyr->latestlap) / 10)][std::min(progress/2-8, 2)], NULL);

			if (progress/2-10 >= 0)
			{
				newval = (208 + (32 * std::max(0, progress - 76))) * FRACUNIT;
				oldval = (208 + (32 * std::max(0, progressOld - 76))) * FRACUNIT;
				interpx = R_InterpolateFixed(oldval, newval);

				V_DrawFixedPatch(
					interpx, // 221
					30*FRACUNIT, // 24
					FRACUNIT, V_SNAPTOTOP|V_HUDTRANS,
					kp_lapanim_number[(((UINT32)stplyr->latestlap) % 10)][std::min(progress/2-10, 2)], NULL);
			}
		}
	}
}

// stretch for "COOOOOL" popup.
// I can't be fucked to find out any math behind this so have a table lmao
static fixed_t stretch[6][2] = {
	{FRACUNIT/4, FRACUNIT*4},
	{FRACUNIT/2, FRACUNIT*2},
	{FRACUNIT, FRACUNIT},
	{FRACUNIT*4, FRACUNIT/2},
	{FRACUNIT*8, FRACUNIT/4},
	{FRACUNIT*4, FRACUNIT/2},
};

static void K_drawTrickCool(void)
{

	tic_t timer = TICRATE - stplyr->karthud[khud_trickcool];

	if (timer <= 6)
	{
		V_DrawStretchyFixedPatch(TCOOL_X<<FRACBITS, TCOOL_Y<<FRACBITS, stretch[timer-1][0], stretch[timer-1][1], V_HUDTRANS|V_SPLITSCREEN, kp_trickcool[splitscreen ? 1 : 0], NULL);
	}
	else if (leveltime & 1)
	{
		V_DrawFixedPatch(TCOOL_X<<FRACBITS, (TCOOL_Y<<FRACBITS) - (timer-10)*FRACUNIT/2, FRACUNIT, V_HUDTRANS|V_SPLITSCREEN, kp_trickcool[splitscreen ? 1 : 0], NULL);
	}
}

void K_drawKartFreePlay(void)
{
	if (!LUA_HudEnabled(hud_freeplay))
		return;

	if (stplyr->spectator == true)
		return;

	if (M_NotFreePlay() == true)
		return;

	if (lt_exitticker < TICRATE/2)
		return;

	if (((leveltime-lt_endtime) % TICRATE) < TICRATE/2)
		return;

	INT32 h_snap = r_splitscreen < 2 ? V_SNAPTORIGHT | V_SLIDEIN : V_HUDTRANS;
	fixed_t x = ((r_splitscreen > 1 ? BASEVIDWIDTH/4 : BASEVIDWIDTH - (LAPS_X+6)) * FRACUNIT);
	fixed_t y = ((r_splitscreen ? BASEVIDHEIGHT/2 : BASEVIDHEIGHT) - 20) * FRACUNIT;

	x -= V_StringScaledWidth(
		FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		V_SNAPTOBOTTOM|h_snap|V_SPLITSCREEN,
		KART_FONT,
		"FREE PLAY"
	) / (r_splitscreen > 1 ? 2 : 1);

	V_DrawStringScaled(
		x,
		y,
		FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		V_SNAPTOBOTTOM|h_snap|V_SPLITSCREEN,
		NULL,
		KART_FONT,
		"FREE PLAY"
	);
}

static void
Draw_party_ping (int ss, INT32 snap)
{
	UINT32 ping = playerpingtable[displayplayers[ss]];
	UINT32 mindelay = playerdelaytable[displayplayers[ss]];
	HU_drawMiniPing(0, 0, ping, mindelay, V_SPLITSCREEN|V_SNAPTOTOP|snap);
}

static void
K_drawMiniPing (void)
{
	UINT32 f = V_SNAPTORIGHT;
	UINT8 i = R_GetViewNumber();

	if (r_splitscreen > 1 && !(i & 1))
	{
		f = V_SNAPTOLEFT;
	}

	Draw_party_ping(i, f);
}

void K_drawButton(fixed_t x, fixed_t y, INT32 flags, patch_t *button[2], boolean pressed)
{
	V_DrawFixedPatch(x, y, FRACUNIT, flags, button[(pressed == true) ? 1 : 0], NULL);
}

void K_drawButtonAnim(INT32 x, INT32 y, INT32 flags, patch_t *button[2], tic_t animtic)
{
	const UINT8 anim_duration = 16;
	const boolean anim = ((animtic % (anim_duration * 2)) < anim_duration);
	K_drawButton(x << FRACBITS, y << FRACBITS, flags, button, anim);
}

static void K_drawDistributionDebugger(void)
{
	itemroulette_t rouletteData = {0};

	const fixed_t scale = (FRACUNIT >> 1);
	const fixed_t space = 24 * scale;
	const fixed_t pad = 9 * scale;

	fixed_t x = -pad;
	fixed_t y = -pad;
	size_t i;

	if (R_GetViewNumber() != 0) // only for p1
	{
		return;
	}

	K_FillItemRouletteData(stplyr, &rouletteData, false);

	for (i = 0; i < rouletteData.itemListLen; i++)
	{
		const kartitems_t item = static_cast<kartitems_t>(rouletteData.itemList[i]);
		UINT8 amount = 1;

		if (y > (BASEVIDHEIGHT << FRACBITS) - space - pad)
		{
			x += space;
			y = -pad;
		}

		V_DrawFixedPatch(x, y, scale, V_SNAPTOTOP,
				K_GetSmallStaticCachedItemPatch(item), NULL);

		// Display amount for multi-items
		amount = K_ItemResultToAmount(item);
		if (amount > 1)
		{
			V_DrawStringScaled(
				x + (18 * scale),
				y + (23 * scale),
				scale, FRACUNIT, FRACUNIT,
				V_SNAPTOTOP,
				NULL, HU_FONT,
				va("x%d", amount)
			);
		}

		y += space;
	}

	V_DrawString((x >> FRACBITS) + 20, 2, V_SNAPTOTOP, va("useOdds[%u]", rouletteData.useOdds));
	V_DrawString((x >> FRACBITS) + 20, 10, V_SNAPTOTOP, va("speed = %u", rouletteData.speed));

	V_DrawString((x >> FRACBITS) + 20, 22, V_SNAPTOTOP, va("baseDist = %u", rouletteData.baseDist));
	V_DrawString((x >> FRACBITS) + 20, 30, V_SNAPTOTOP, va("dist = %u", rouletteData.dist));

	V_DrawString((x >> FRACBITS) + 20, 42, V_SNAPTOTOP, va("firstDist = %u", rouletteData.firstDist));
	V_DrawString((x >> FRACBITS) + 20, 50, V_SNAPTOTOP, va("secondDist = %u", rouletteData.secondDist));
	V_DrawString((x >> FRACBITS) + 20, 58, V_SNAPTOTOP, va("secondToFirst = %u", rouletteData.secondToFirst));

#ifndef ITEM_LIST_SIZE
	Z_Free(rouletteData.itemList);
#endif
}

static void K_DrawWaypointDebugger(void)
{
	if (cv_kartdebugwaypoints.value == 0)
		return;

	if (R_GetViewNumber() != 0) // only for p1
		return;

	constexpr int kH = 8;
	using srb2::Draw;
	Draw::TextElement label;
	label.font(Draw::Font::kThin);
	label.flags(V_AQUAMAP);
	Draw line = Draw(8, 110).font(Draw::Font::kMenu);
	auto put = [&](const char* label_str, auto&&... args)
	{
		constexpr int kTabWidth = 48;
		label.string(label_str);
		int x = label.width() + kTabWidth;
		x -= x % kTabWidth;
		line.size(x + 4, 2).y(7).fill(31);
		line.text(label);
		line.x(x).text(args...);
		line = line.y(kH);
	};

	if (netgame)
	{
		line = line.y(-kH);
		put("Online griefing:", "[{}, {}]", stplyr->griefValue/TICRATE, stplyr->griefStrikes);
	}

	put("Current Waypoint ID:", "{}", K_GetWaypointID(stplyr->currentwaypoint));
	put("Next Waypoint ID:", "{}{}", K_GetWaypointID(stplyr->nextwaypoint), ((stplyr->pflags & PF_WRONGWAY) ? " (WRONG WAY)" : ""));
	put("Respawn Waypoint ID:", "{}", K_GetWaypointID(stplyr->respawn.wp));
	put("Finishline Distance:", "{}", stplyr->distancetofinish);
	put("Last Safe Lap:", "{}", stplyr->lastsafelap);

	if (numcheatchecks > 0)
	{
		if (stplyr->cheatchecknum == numcheatchecks)
			put("Cheat Check:", "{} / {} (Can finish)", stplyr->cheatchecknum, numcheatchecks);
		else
			put("Cheat Check:", "{} / {}", stplyr->cheatchecknum, numcheatchecks);
		put("Last Safe Cheat Check:", "{}", stplyr->lastsafecheatcheck);
	}

	if (stplyr->bigwaypointgap)
	{
		put("Auto Respawn Timer:", "{}", stplyr->bigwaypointgap);
	}
}

static void K_DrawBotDebugger(void)
{
	player_t *bot = NULL;

	if (cv_kartdebugbots.value == 0)
	{
		return;
	}

	if (R_GetViewNumber() != 0) // only for p1
	{
		return;
	}

	if (stplyr->bot == true)
	{
		// we ARE the bot
		bot = stplyr;
	}
	else
	{
		// get winning bot
		size_t i;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			player_t *p = NULL;

			if (playeringame[i] == false)
			{
				continue;
			}

			p = &players[i];
			if (p->spectator == true || p->bot == false)
			{
				continue;
			}

			if (bot == NULL || p->distancetofinish < bot->distancetofinish)
			{
				bot = p;
			}
		}
	}

	if (bot == NULL)
	{
		// no bot exists?
		return;
	}

	V_DrawSmallString(16, 8, V_YELLOWMAP, va("Bot: %s", player_names[bot - players]));

	V_DrawSmallString(8, 14, 0, va("Difficulty: %d / %d", bot->botvars.difficulty, MAXBOTDIFFICULTY));
	V_DrawSmallString(8, 18, 0, va("Difficulty increase: %d", bot->botvars.diffincrease));
	V_DrawSmallString(8, 22, 0, va("Rival: %d", (UINT8)(bot->botvars.rival == true)));
	V_DrawSmallString(8, 26, 0, va("Rubberbanding: %.02f", FIXED_TO_FLOAT(bot->botvars.rubberband) * 100.0f));

	V_DrawSmallString(8, 32, 0, va("Item delay: %d", bot->botvars.itemdelay));
	V_DrawSmallString(8, 36, 0, va("Item confirm: %d", bot->botvars.itemconfirm));

	V_DrawSmallString(8, 42, 0, va("Turn: %d / %d / %d", -BOTTURNCONFIRM, bot->botvars.turnconfirm, BOTTURNCONFIRM));
	V_DrawSmallString(8, 46, 0, va("Spindash: %d / %d", bot->botvars.spindashconfirm, BOTSPINDASHCONFIRM));
	V_DrawSmallString(8, 50, 0, va("Respawn: %d / %d", bot->botvars.respawnconfirm, BOTRESPAWNCONFIRM));

	V_DrawSmallString(8, 56, 0, va("Item priority: %d", bot->botvars.roulettePriority));
	V_DrawSmallString(8, 60, 0, va("Item timeout: %d", bot->botvars.rouletteTimeout));

	V_DrawSmallString(8, 66, 0, va("Complexity: %d", K_GetTrackComplexity()));
	V_DrawSmallString(8, 70, 0, va("Bot modifier: %.2f", FixedToFloat(K_BotMapModifier())));
}

static void K_DrawGPRankDebugger(void)
{
	gp_rank_e grade = GRADE_E;
	char gradeChar = '?';

	if (cv_debugrank.value == 0)
	{
		return;
	}

	if (R_GetViewNumber() != 0) // only for p1
	{
		return;
	}

	if (grandprixinfo.gp == false)
	{
		return;
	}

	grade = K_CalculateGPGrade(&grandprixinfo.rank);

	V_DrawThinString(0, 0, V_SNAPTOTOP|V_SNAPTOLEFT,
		va("POS: %d / %d", grandprixinfo.rank.position, RANK_NEUTRAL_POSITION));
	V_DrawThinString(0, 10, V_SNAPTOTOP|V_SNAPTOLEFT,
		va("PTS: %d / %d", grandprixinfo.rank.winPoints, grandprixinfo.rank.totalPoints));
	V_DrawThinString(0, 20, V_SNAPTOTOP|V_SNAPTOLEFT,
		va("LAPS: %d / %d", grandprixinfo.rank.laps, grandprixinfo.rank.totalLaps));
	V_DrawThinString(0, 30, V_SNAPTOTOP|V_SNAPTOLEFT,
		va("CONTINUES: %d", grandprixinfo.rank.continuesUsed));
	V_DrawThinString(0, 40, V_SNAPTOTOP|V_SNAPTOLEFT,
		va("PRISONS: %d / %d", grandprixinfo.rank.prisons, grandprixinfo.rank.totalPrisons));
	V_DrawThinString(0, 50, V_SNAPTOTOP|V_SNAPTOLEFT,
		va("RINGS: %d / %d", grandprixinfo.rank.rings, grandprixinfo.rank.totalRings));
	V_DrawThinString(0, 60, V_SNAPTOTOP|V_SNAPTOLEFT,
		va("EMERALD: %s", (grandprixinfo.rank.specialWon == true) ? "YES" : "NO"));

	switch (grade)
	{
		case GRADE_E: { gradeChar = 'E'; break; }
		case GRADE_D: { gradeChar = 'D'; break; }
		case GRADE_C: { gradeChar = 'C'; break; }
		case GRADE_B: { gradeChar = 'B'; break; }
		case GRADE_A: { gradeChar = 'A'; break; }
		case GRADE_S: { gradeChar = 'S'; break; }
		default: { break; }
	}

	V_DrawThinString(0, 90, V_SNAPTOTOP|V_SNAPTOLEFT|V_YELLOWMAP,
		va(" ** FINAL GRADE: %c", gradeChar));
}

typedef enum
{
	MM_IN,
	MM_HOLD,
	MM_OUT,
} messagemode_t;

typedef struct
{
	std::string text;
	sfxenum_t sound;
} message_t;

struct messagestate_t
{
	std::deque<std::string> messages;
	std::string objective = "";
	tic_t timer = 0;
	boolean persist = false;
	messagemode_t mode = MM_IN;
	const tic_t speedyswitch = 2*TICRATE;
	const tic_t lazyswitch = 4*TICRATE;

	void add(std::string msg)
	{
		messages.push_back(msg);
	}

	void clear()
	{
		messages.clear();
		switch_mode(MM_IN);
	}

	void switch_mode(messagemode_t nextmode)
	{
		mode = nextmode;
		timer = 0;
	}

	void tick()
	{
		if (messages.size() == 0)
		{
			if (!objective.empty())
				restore();
			else
				return;
		}

		if (exitcountdown)
			return;

		if (timer == 0 && mode == MM_IN)
			S_StartSound(NULL, sfx_s3k47);

		timer++;

		switch (mode)
		{
			case MM_IN:
				if (timer > messages[0].length())
					switch_mode(MM_HOLD);
				break;
			case MM_HOLD:
				if (messages.size() > 1 && timer > speedyswitch) // Waiting message, switch to it right away!
					next();
				else if (timer > lazyswitch && !persist) // If there's no pending message, we can chill for a bit.
					switch_mode(MM_OUT);
				break;
			case MM_OUT:
				if (timer > messages[0].length())
					next();
				break;
		}
	}

	void restore()
	{
		switch_mode(MM_IN);
		persist = true;
		messages.clear();
		messages.push_front(objective);
	}

	void next()
	{
		switch_mode(MM_IN);
		persist = false;
		if (messages.size() > 0)
			messages.pop_front();
	}

};

static std::vector<messagestate_t> messagestates{MAXSPLITSCREENPLAYERS};

void K_AddMessage(const char *msg, boolean interrupt, boolean persist)
{
	for (auto &state : messagestates)
	{
		if (interrupt)
			state.clear();

		std::string parsedmsg = srb2::Draw::TextElement().parse(msg).string();

		if (persist)
			state.objective = parsedmsg;
		else
			state.add(parsedmsg);
	}
}

void K_ClearPersistentMessages()
{
	for (auto &state : messagestates)
	{
		state.objective = "";
		state.clear();
	}
}

// Return value can be used for "paired" splitscreen messages, true = was displayed
void K_AddMessageForPlayer(player_t *player, const char *msg, boolean interrupt, boolean persist)
{
	if (!player)
		return;

	if (player && !P_IsDisplayPlayer(player))
		return;

	if (player && K_PlayerUsesBotMovement(player))
		return;

	messagestate_t *state = &messagestates[G_PartyPosition(player - players)];

	if (interrupt)
		state->clear();

	std::string parsedmsg = srb2::Draw::TextElement().parse(msg).string();

	if (persist)
		state->objective = parsedmsg;
	else
		state->add(parsedmsg);
}

void K_ClearPersistentMessageForPlayer(player_t *player)
{
	if (!player)
		return;

	if (player && !P_IsDisplayPlayer(player))
		return;

	messagestate_t *state = &messagestates[G_PartyPosition(player - players)];
	state->objective = "";
}

void K_TickMessages()
{
	for (auto &state : messagestates)
	{
		state.tick();
	}
}

static void K_DrawMessageFeed(void)
{
	int i;

	if (exitcountdown)
		return;

	for (i = 0; i <= r_splitscreen; i++)
	{
		messagestate_t state = messagestates[i];

		if (state.messages.size() == 0)
			continue;

		std::string msg = state.messages[0];

		UINT8 sublen = state.timer;
		if (state.mode == MM_IN)
			sublen = state.timer;
		else if (state.mode == MM_HOLD)
			sublen = msg.length();
		else if (state.mode == MM_OUT)
			sublen = msg.length() - state.timer;

		std::string submsg = msg.substr(0, sublen);

		using srb2::Draw;

		Draw::TextElement text(submsg);

		text.font(Draw::Font::kMenu);

		UINT8 x = 160;
		UINT8 y = 10;
		SINT8 shift = 0;
		if (r_splitscreen >= 2)
		{
			text.font(Draw::Font::kThin);
			shift = -2;

			x = BASEVIDWIDTH/4;
			y = 5;

			if (i % 2)
				x += BASEVIDWIDTH/2;

			if (i >= 2)
				y += BASEVIDHEIGHT / 2;
		}
		else if (r_splitscreen >= 1)
		{
			y = 5;

			if (i >= 1)
				y += BASEVIDHEIGHT / 2;
		}
		UINT16 sw = text.width();

		K_DrawSticker(x - sw/2, y, sw, 0, true);
		Draw(x, y+shift).align(Draw::Align::kCenter).text(text);
	}
}

void K_drawKartHUD(void)
{
	boolean islonesome = false;
	UINT8 viewnum = R_GetViewNumber();
	boolean freecam = camera[viewnum].freecam;	//disable some hud elements w/ freecam

	// Define the X and Y for each drawn object
	// This is handled by console/menu values
	K_initKartHUD();

	// Draw that fun first person HUD! Drawn ASAP so it looks more "real".
	if (!camera[viewnum].chase && !freecam)
		K_drawKartFirstPerson();

	if (mapreset)
	{
		// HERE COMES A NEW CHALLENGER
		if (R_GetViewNumber() == 0)
			K_drawChallengerScreen();
		return;
	}

	// Draw full screen stuff that turns off the rest of the HUD
	if (R_GetViewNumber() == 0)
	{
		if (g_emeraldWin)
			K_drawEmeraldWin(false);
	}

	if (!demo.attract)
	{
		// Draw the CHECK indicator before the other items, so it's overlapped by everything else
		if (LUA_HudEnabled(hud_check))	// delete lua when?
			if (!splitscreen && !players[displayplayers[0]].exiting && !freecam)
				K_drawKartPlayerCheck();

		// nametags
		if (LUA_HudEnabled(hud_names) && cv_drawpickups.value)
			K_drawKartNameTags();

		// Draw WANTED status
#if 0
		if (gametype == GT_BATTLE)
		{
			if (LUA_HudEnabled(hud_wanted))
				K_drawKartWanted();
		}
#endif

		if (LUA_HudEnabled(hud_minimap))
			K_drawKartMinimap();
	}

	if (demo.attract)
		;
	else if (gametype == GT_TUTORIAL)
	{
		islonesome = true;
	}
	else if (!r_splitscreen)
	{
		// Draw the timestamp
		if (LUA_HudEnabled(hud_time))
		{
			bool ta = modeattacking && !demo.playback;
			INT32 flags = V_HUDTRANS|V_SLIDEIN|V_SNAPTOTOP|V_SNAPTORIGHT;

			tic_t realtime = stplyr->realtime;

			if (stplyr->karthud[khud_lapanimation]
				&& !stplyr->exiting
				&& stplyr->laptime[LAP_LAST] != 0
				&& stplyr->laptime[LAP_LAST] != UINT32_MAX)
			{
				if ((stplyr->karthud[khud_lapanimation] / 5) & 1)
				{
					realtime = stplyr->laptime[LAP_LAST];
				}
				else
				{
					realtime = UINT32_MAX;
				}
			}

			K_drawKartTimestamp(realtime, TIME_X, TIME_Y + (ta ? 2 : 0), flags, 0);

			if (modeattacking)
			{
				if (ta)
				{
					using srb2::Draw;
					Draw(BASEVIDWIDTH - 19, 2)
						.flags(flags | V_YELLOWMAP)
						.align(Draw::Align::kRight)
						.text("\xBE Restart");
				}
				else
				{
					using srb2::Draw;
					Draw row = Draw(BASEVIDWIDTH - 20, TIME_Y + 18).flags(flags).align(Draw::Align::kRight);
					auto insert = [&](const char *label, UINT32 tics)
					{
						Draw::TextElement text =
							tics != UINT32_MAX ?
							Draw::TextElement(
								"{:02}'{:02}\"{:02}",
								G_TicsToMinutes(tics, true),
								G_TicsToSeconds(tics),
								G_TicsToCentiseconds(tics)
							) :
							Draw::TextElement("--'--\"--");
						text.font(Draw::Font::kZVote);
						row.x(-text.width()).flags(V_ORANGEMAP).text(label);
						row.y(1).text(text);
						row = row.y(10);
					};
					if (modeattacking & ATTACKING_TIME)
						insert("Finish: ", hu_demotime);
					if (modeattacking & ATTACKING_LAP)
						insert("Best Lap: ", hu_demolap);
				}
			}
		}

		islonesome = K_drawKartPositionFaces();
	}
	else
	{
		islonesome = M_NotFreePlay() == false;

		if (r_splitscreen == 1)
		{
			if (LUA_HudEnabled(hud_time))
			{
				K_drawKart2PTimestamp();
			}

			if (viewnum == r_splitscreen && gametyperules & GTR_POINTLIMIT)
			{
				K_drawKartPositionFaces();
			}
		}
		else if (viewnum == r_splitscreen)
		{
			if (LUA_HudEnabled(hud_time))
			{
				K_drawKart4PTimestamp();
			}

			if (gametyperules & GTR_POINTLIMIT)
			{
				K_drawKartPositionFaces();
			}
		}
	}

	if (!stplyr->spectator && !freecam) // Bottom of the screen elements, don't need in spectate mode
	{
		if (demo.attract)
		{
			if (demo.attract == DEMO_ATTRACT_TITLE) // Draw logo on title screen demos
			{
				INT32 x = BASEVIDWIDTH - 8, y = BASEVIDHEIGHT-8, snapflags = V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SLIDEIN;
				patch_t *pat = static_cast<patch_t*>(W_CachePatchName((M_UseAlternateTitleScreen() ? "MTSJUMPR1" : "MTSBUMPR1"), PU_CACHE));
				const UINT8 *colormap = nullptr;

				if (INT32 fade = F_AttractDemoExitFade())
				{
					// TODO: Twodee cannot handle
					// V_DrawCustomFadeScreen.
					// However, since the screen fade just
					// uses a colormap, the same colormap can
					// be applied on a per-patch basis.
					// I'm only bothering to apply this
					// colormap to the attract mode sticker,
					// since it's the lone HUD element.
					if (lighttable_t *clm = V_LoadCustomFadeMap("FADEMAP0"))
					{
						// This must be statically allocated for Twodee
						static UINT8 *colormap_storage;
						const UINT8 *fadetable = V_OffsetIntoFadeMap(clm, fade);

						if (!colormap_storage)
							Z_MallocAlign(256, PU_STATIC, &colormap_storage, 8);

						memcpy(colormap_storage, fadetable, 256);
						colormap = colormap_storage;

						Z_Free(clm);
					}
				}

				if (r_splitscreen == 3)
				{
					x = BASEVIDWIDTH/2;
					y = BASEVIDHEIGHT/2;
					snapflags = 0;
				}

				V_DrawMappedPatch(x-(SHORT(pat->width)), y-(SHORT(pat->height)), snapflags, pat, colormap);
			}
		}
		else
		{
			boolean gametypeinfoshown = false;

			if (K_PlayerTallyActive(stplyr) == true)
			{
				K_DrawPlayerTally();
			}

			if (LUA_HudEnabled(hud_position))
			{
				if (bossinfo.valid)
				{
					K_drawBossHealthBar();
				}
				else if (freecam)
					;
				else if ((gametyperules & GTR_POWERSTONES) && !K_PlayerTallyActive(stplyr))
				{
					if (!battleprisons)
						K_drawKartEmeralds();
				}
				else if (!islonesome && !K_Cooperative())
					K_DrawKartPositionNum(stplyr->position);
			}

			if (LUA_HudEnabled(hud_gametypeinfo))
			{
				if (gametyperules & GTR_CIRCUIT)
				{
					if (numlaps != 1)
					{
						K_drawKartLaps();
						gametypeinfoshown = true;
					}
				}
				else if (gametyperules & GTR_BUMPERS)
				{
					K_drawKartBumpersOrKarma();
					gametypeinfoshown = true;
				}
			}

			// Draw the speedometer and/or accessibility icons
			if (cv_kartspeedometer.value && !r_splitscreen && (LUA_HudEnabled(hud_speedometer)))
			{
				K_drawKartSpeedometer(gametypeinfoshown);
			}
			else
			{
				K_drawKartAccessibilityIcons(gametypeinfoshown, 0);
			}

			if (gametyperules & GTR_SPHERES)
			{
				K_drawBlueSphereMeter(gametypeinfoshown);
			}
			else
			{
				K_drawRingCounter(gametypeinfoshown);
			}

			// Draw the item window
			if (LUA_HudEnabled(hud_item) && !freecam)
			{
				if (stplyr->itemRoulette.ringbox && stplyr->itemamount == 0 && stplyr->itemtype == 0)
				{
					K_drawKartSlotMachine();
				}
				else
				{
					K_drawKartItem();
				}
			}
		}
	}

	// Draw the countdowns after everything else.
	if (stplyr->lives <= 0 && stplyr->playerstate == PST_DEAD)
	{
		;
	}
	else if (stplyr->karthud[khud_fault] != 0 && stplyr->karthud[khud_finish] == 0)
	{
		K_drawKartFinish(false);
	}
	else if (starttime != introtime
	&& leveltime >= introtime
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
			V_DrawTimerString((BASEVIDWIDTH/2)-karlen, LAPS_Y+3, V_SPLITSCREEN, countstr);
		}
	}

	// Race overlays
	if (!freecam)
	{
		if (stplyr->exiting)
			K_drawKartFinish(true);
		else if (!(gametyperules & GTR_CIRCUIT))
			;
		else if (stplyr->karthud[khud_lapanimation] && !r_splitscreen)
			K_drawLapStartAnim();
	}

	// trick panel cool trick
	if (stplyr->karthud[khud_trickcool])
		K_drawTrickCool();

	if ((freecam || stplyr->spectator) && LUA_HudEnabled(hud_textspectator))
	{
		K_drawSpectatorHUD(false);
	}

	if (R_GetViewNumber() == 0 && g_emeraldWin)
		K_drawEmeraldWin(true);

	if (modeattacking || freecam) // everything after here is MP and debug only
	{
		K_drawInput();
		goto debug;
	}

	if ((gametyperules & GTR_KARMA) && !r_splitscreen && (stplyr->karthud[khud_yougotem] % 2)) // * YOU GOT EM *
		V_DrawScaledPatch(BASEVIDWIDTH/2 - (SHORT(kp_yougotem->width)/2), 32, V_HUDTRANS, kp_yougotem);

	// Draw FREE PLAY.
	K_drawKartFreePlay();

	if ((netgame || cv_mindelay.value) && r_splitscreen && Playing())
	{
		K_drawMiniPing();
	}

	K_drawKartPowerUps();

	if (K_DirectorIsAvailable(viewnum) == true && LUA_HudEnabled(hud_textspectator))
	{
		K_drawSpectatorHUD(true);
	}
	else
	{
		K_drawInput();
	}

	if (cv_kartdebugdistribution.value)
		K_drawDistributionDebugger();

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
				UINT8 *cm = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(c), GTC_CACHE);
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

debug:
	K_DrawWaypointDebugger();
	K_DrawBotDebugger();
	K_DrawDirectorDebugger();
	K_DrawGPRankDebugger();
	K_DrawMessageFeed();
}

void K_DrawSticker(INT32 x, INT32 y, INT32 width, INT32 flags, boolean isSmall)
{
	patch_t *stickerEnd;
	INT32 height;

	if (isSmall == true)
	{
		stickerEnd = static_cast<patch_t*>(W_CachePatchName("K_STIKE2", PU_CACHE));
		height = 6;
	}
	else
	{
		stickerEnd = static_cast<patch_t*>(W_CachePatchName("K_STIKEN", PU_CACHE));
		height = 11;
	}

	V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, flags, stickerEnd, NULL);
	V_DrawFill(x, y, width, height, 24|flags);
	V_DrawFixedPatch((x + width)*FRACUNIT, y*FRACUNIT, FRACUNIT, flags|V_FLIP, stickerEnd, NULL);
}
