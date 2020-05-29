// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2019 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief hardware renderer, using the standard HardWareRender driver DLL for SRB2

#include <math.h>

#include "../doomstat.h"

#ifdef HWRENDER
#include "hw_main.h"
#include "hw_glob.h"
#include "hw_drv.h"
#include "hw_md2.h"
#include "hw_clip.h"
#include "hw_light.h"

#include "../i_video.h" // for rendermode == render_glide
#include "../v_video.h"
#include "../p_local.h"
#include "../p_setup.h"
#include "../r_local.h"
<<<<<<< HEAD
#include "../r_bsp.h"	// R_NoEncore
#include "../r_main.h"	// cv_fov
=======
#include "../r_patch.h"
#include "../r_bsp.h"
>>>>>>> srb2/next
#include "../d_clisrv.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../r_splats.h"
#include "../g_game.h"
#include "../st_stuff.h"
#include "../i_system.h"
#include "../m_cheat.h"
<<<<<<< HEAD
#include "../r_things.h" // R_GetShadowZ

#ifdef ESLOPE
#include "../p_slopes.h"
#endif
=======
#include "../f_finale.h"
#include "../r_things.h" // R_GetShadowZ
#include "../p_slopes.h"
#include "hw_md2.h"
>>>>>>> srb2/next

#include <stdlib.h> // qsort

#define ABS(x) ((x) < 0 ? -(x) : (x))

// ==========================================================================
// the hardware driver object
// ==========================================================================
struct hwdriver_s hwdriver;

// ==========================================================================
// Commands and console variables
// ==========================================================================

<<<<<<< HEAD
static void CV_filtermode_ONChange(void);
static void CV_anisotropic_ONChange(void);

static CV_PossibleValue_t grfiltermode_cons_t[]= {{HWD_SET_TEXTUREFILTER_POINTSAMPLED, "Nearest"},
	{HWD_SET_TEXTUREFILTER_BILINEAR, "Bilinear"}, {HWD_SET_TEXTUREFILTER_TRILINEAR, "Trilinear"},
	{HWD_SET_TEXTUREFILTER_MIXED1, "Linear_Nearest"},
	{HWD_SET_TEXTUREFILTER_MIXED2, "Nearest_Linear"},
	{HWD_SET_TEXTUREFILTER_MIXED3, "Nearest_Mipmap"},
	{0, NULL}};
CV_PossibleValue_t granisotropicmode_cons_t[] = {{1, "MIN"}, {16, "MAX"}, {0, NULL}};

consvar_t cv_grrounddown = {"gr_rounddown", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};


consvar_t cv_grfiltermode = {"gr_filtermode", "Nearest", CV_CALL|CV_SAVE, grfiltermode_cons_t,
                             CV_filtermode_ONChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_granisotropicmode = {"gr_anisotropicmode", "1", CV_CALL|CV_SAVE, granisotropicmode_cons_t,
                             CV_anisotropic_ONChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grcorrecttricks = {"gr_correcttricks", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grsolvetjoin = {"gr_solvetjoin", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grbatching = {"gr_batching", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static void CV_filtermode_ONChange(void)
{
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREFILTERMODE, cv_grfiltermode.value);
}

static void CV_anisotropic_ONChange(void)
{
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREANISOTROPICMODE, cv_granisotropicmode.value);
}
=======

static void HWR_AddSprites(sector_t *sec);
static void HWR_ProjectSprite(mobj_t *thing);
#ifdef HWPRECIP
static void HWR_ProjectPrecipitationSprite(precipmobj_t *thing);
#endif

#ifdef SORTING
void HWR_AddTransparentFloor(levelflat_t *levelflat, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight,
                             INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, boolean fogplane, extracolormap_t *planecolormap);
void HWR_AddTransparentPolyobjectFloor(levelflat_t *levelflat, polyobj_t *polysector, boolean isceiling, fixed_t fixedheight,
                             INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, extracolormap_t *planecolormap);
#else
static void HWR_Add3DWater(levelflat_t *levelflat, extrasubsector_t *xsub, fixed_t fixedheight,
                           INT32 lightlevel, INT32 alpha, sector_t *FOFSector);
static void HWR_Render3DWater(void);
static void HWR_RenderTransparentWalls(void);
#endif
static void HWR_FoggingOn(void);
static UINT32 atohex(const char *s);

boolean drawsky = true;

/*
 * lookuptable for lightvalues
 * calculated as follow:
 * floatlight = (1.0-exp((light^3)*gamma)) / (1.0-exp(1.0*gamma));
 * gamma=-0,2;-2,0;-4,0;-6,0;-8,0
 * light = 0,0 .. 1,0
 */
static const float lighttable[5][256] =
{
  {
    0.00000f,0.00000f,0.00000f,0.00000f,0.00000f,0.00001f,0.00001f,0.00002f,0.00003f,0.00004f,
    0.00006f,0.00008f,0.00010f,0.00013f,0.00017f,0.00020f,0.00025f,0.00030f,0.00035f,0.00041f,
    0.00048f,0.00056f,0.00064f,0.00073f,0.00083f,0.00094f,0.00106f,0.00119f,0.00132f,0.00147f,
    0.00163f,0.00180f,0.00198f,0.00217f,0.00237f,0.00259f,0.00281f,0.00305f,0.00331f,0.00358f,
    0.00386f,0.00416f,0.00447f,0.00479f,0.00514f,0.00550f,0.00587f,0.00626f,0.00667f,0.00710f,
    0.00754f,0.00800f,0.00848f,0.00898f,0.00950f,0.01003f,0.01059f,0.01117f,0.01177f,0.01239f,
    0.01303f,0.01369f,0.01437f,0.01508f,0.01581f,0.01656f,0.01734f,0.01814f,0.01896f,0.01981f,
    0.02069f,0.02159f,0.02251f,0.02346f,0.02444f,0.02544f,0.02647f,0.02753f,0.02862f,0.02973f,
    0.03088f,0.03205f,0.03325f,0.03448f,0.03575f,0.03704f,0.03836f,0.03971f,0.04110f,0.04252f,
    0.04396f,0.04545f,0.04696f,0.04851f,0.05009f,0.05171f,0.05336f,0.05504f,0.05676f,0.05852f,
    0.06031f,0.06214f,0.06400f,0.06590f,0.06784f,0.06981f,0.07183f,0.07388f,0.07597f,0.07810f,
    0.08027f,0.08248f,0.08473f,0.08702f,0.08935f,0.09172f,0.09414f,0.09659f,0.09909f,0.10163f,
    0.10421f,0.10684f,0.10951f,0.11223f,0.11499f,0.11779f,0.12064f,0.12354f,0.12648f,0.12946f,
    0.13250f,0.13558f,0.13871f,0.14188f,0.14511f,0.14838f,0.15170f,0.15507f,0.15850f,0.16197f,
    0.16549f,0.16906f,0.17268f,0.17635f,0.18008f,0.18386f,0.18769f,0.19157f,0.19551f,0.19950f,
    0.20354f,0.20764f,0.21179f,0.21600f,0.22026f,0.22458f,0.22896f,0.23339f,0.23788f,0.24242f,
    0.24702f,0.25168f,0.25640f,0.26118f,0.26602f,0.27091f,0.27587f,0.28089f,0.28596f,0.29110f,
    0.29630f,0.30156f,0.30688f,0.31226f,0.31771f,0.32322f,0.32879f,0.33443f,0.34013f,0.34589f,
    0.35172f,0.35761f,0.36357f,0.36960f,0.37569f,0.38185f,0.38808f,0.39437f,0.40073f,0.40716f,
    0.41366f,0.42022f,0.42686f,0.43356f,0.44034f,0.44718f,0.45410f,0.46108f,0.46814f,0.47527f,
    0.48247f,0.48974f,0.49709f,0.50451f,0.51200f,0.51957f,0.52721f,0.53492f,0.54271f,0.55058f,
    0.55852f,0.56654f,0.57463f,0.58280f,0.59105f,0.59937f,0.60777f,0.61625f,0.62481f,0.63345f,
    0.64217f,0.65096f,0.65984f,0.66880f,0.67783f,0.68695f,0.69615f,0.70544f,0.71480f,0.72425f,
    0.73378f,0.74339f,0.75308f,0.76286f,0.77273f,0.78268f,0.79271f,0.80283f,0.81304f,0.82333f,
    0.83371f,0.84417f,0.85472f,0.86536f,0.87609f,0.88691f,0.89781f,0.90880f,0.91989f,0.93106f,
    0.94232f,0.95368f,0.96512f,0.97665f,0.98828f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00000f,0.00001f,0.00002f,0.00003f,0.00005f,0.00007f,0.00010f,
    0.00014f,0.00019f,0.00024f,0.00031f,0.00038f,0.00047f,0.00057f,0.00069f,0.00081f,0.00096f,
    0.00112f,0.00129f,0.00148f,0.00170f,0.00193f,0.00218f,0.00245f,0.00274f,0.00306f,0.00340f,
    0.00376f,0.00415f,0.00456f,0.00500f,0.00547f,0.00597f,0.00649f,0.00704f,0.00763f,0.00825f,
    0.00889f,0.00957f,0.01029f,0.01104f,0.01182f,0.01264f,0.01350f,0.01439f,0.01532f,0.01630f,
    0.01731f,0.01836f,0.01945f,0.02058f,0.02176f,0.02298f,0.02424f,0.02555f,0.02690f,0.02830f,
    0.02974f,0.03123f,0.03277f,0.03436f,0.03600f,0.03768f,0.03942f,0.04120f,0.04304f,0.04493f,
    0.04687f,0.04886f,0.05091f,0.05301f,0.05517f,0.05738f,0.05964f,0.06196f,0.06434f,0.06677f,
    0.06926f,0.07181f,0.07441f,0.07707f,0.07979f,0.08257f,0.08541f,0.08831f,0.09126f,0.09428f,
    0.09735f,0.10048f,0.10368f,0.10693f,0.11025f,0.11362f,0.11706f,0.12056f,0.12411f,0.12773f,
    0.13141f,0.13515f,0.13895f,0.14281f,0.14673f,0.15072f,0.15476f,0.15886f,0.16303f,0.16725f,
    0.17153f,0.17587f,0.18028f,0.18474f,0.18926f,0.19383f,0.19847f,0.20316f,0.20791f,0.21272f,
    0.21759f,0.22251f,0.22748f,0.23251f,0.23760f,0.24274f,0.24793f,0.25318f,0.25848f,0.26383f,
    0.26923f,0.27468f,0.28018f,0.28573f,0.29133f,0.29697f,0.30266f,0.30840f,0.31418f,0.32001f,
    0.32588f,0.33179f,0.33774f,0.34374f,0.34977f,0.35585f,0.36196f,0.36810f,0.37428f,0.38050f,
    0.38675f,0.39304f,0.39935f,0.40570f,0.41207f,0.41847f,0.42490f,0.43136f,0.43784f,0.44434f,
    0.45087f,0.45741f,0.46398f,0.47057f,0.47717f,0.48379f,0.49042f,0.49707f,0.50373f,0.51041f,
    0.51709f,0.52378f,0.53048f,0.53718f,0.54389f,0.55061f,0.55732f,0.56404f,0.57075f,0.57747f,
    0.58418f,0.59089f,0.59759f,0.60429f,0.61097f,0.61765f,0.62432f,0.63098f,0.63762f,0.64425f,
    0.65086f,0.65746f,0.66404f,0.67060f,0.67714f,0.68365f,0.69015f,0.69662f,0.70307f,0.70948f,
    0.71588f,0.72224f,0.72857f,0.73488f,0.74115f,0.74739f,0.75359f,0.75976f,0.76589f,0.77199f,
    0.77805f,0.78407f,0.79005f,0.79599f,0.80189f,0.80774f,0.81355f,0.81932f,0.82504f,0.83072f,
    0.83635f,0.84194f,0.84747f,0.85296f,0.85840f,0.86378f,0.86912f,0.87441f,0.87964f,0.88482f,
    0.88995f,0.89503f,0.90005f,0.90502f,0.90993f,0.91479f,0.91959f,0.92434f,0.92903f,0.93366f,
    0.93824f,0.94276f,0.94723f,0.95163f,0.95598f,0.96027f,0.96451f,0.96868f,0.97280f,0.97686f,
    0.98086f,0.98481f,0.98869f,0.99252f,0.99629f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00001f,0.00002f,0.00003f,0.00005f,0.00008f,0.00013f,0.00018f,
    0.00025f,0.00033f,0.00042f,0.00054f,0.00067f,0.00083f,0.00101f,0.00121f,0.00143f,0.00168f,
    0.00196f,0.00227f,0.00261f,0.00299f,0.00339f,0.00383f,0.00431f,0.00483f,0.00538f,0.00598f,
    0.00661f,0.00729f,0.00802f,0.00879f,0.00961f,0.01048f,0.01140f,0.01237f,0.01340f,0.01447f,
    0.01561f,0.01680f,0.01804f,0.01935f,0.02072f,0.02215f,0.02364f,0.02520f,0.02682f,0.02850f,
    0.03026f,0.03208f,0.03397f,0.03594f,0.03797f,0.04007f,0.04225f,0.04451f,0.04684f,0.04924f,
    0.05172f,0.05428f,0.05691f,0.05963f,0.06242f,0.06530f,0.06825f,0.07129f,0.07441f,0.07761f,
    0.08089f,0.08426f,0.08771f,0.09125f,0.09487f,0.09857f,0.10236f,0.10623f,0.11019f,0.11423f,
    0.11836f,0.12257f,0.12687f,0.13125f,0.13571f,0.14027f,0.14490f,0.14962f,0.15442f,0.15931f,
    0.16427f,0.16932f,0.17445f,0.17966f,0.18496f,0.19033f,0.19578f,0.20130f,0.20691f,0.21259f,
    0.21834f,0.22417f,0.23007f,0.23605f,0.24209f,0.24820f,0.25438f,0.26063f,0.26694f,0.27332f,
    0.27976f,0.28626f,0.29282f,0.29944f,0.30611f,0.31284f,0.31962f,0.32646f,0.33334f,0.34027f,
    0.34724f,0.35426f,0.36132f,0.36842f,0.37556f,0.38273f,0.38994f,0.39718f,0.40445f,0.41174f,
    0.41907f,0.42641f,0.43378f,0.44116f,0.44856f,0.45598f,0.46340f,0.47084f,0.47828f,0.48573f,
    0.49319f,0.50064f,0.50809f,0.51554f,0.52298f,0.53042f,0.53784f,0.54525f,0.55265f,0.56002f,
    0.56738f,0.57472f,0.58203f,0.58932f,0.59658f,0.60381f,0.61101f,0.61817f,0.62529f,0.63238f,
    0.63943f,0.64643f,0.65339f,0.66031f,0.66717f,0.67399f,0.68075f,0.68746f,0.69412f,0.70072f,
    0.70726f,0.71375f,0.72017f,0.72653f,0.73282f,0.73905f,0.74522f,0.75131f,0.75734f,0.76330f,
    0.76918f,0.77500f,0.78074f,0.78640f,0.79199f,0.79751f,0.80295f,0.80831f,0.81359f,0.81880f,
    0.82393f,0.82898f,0.83394f,0.83883f,0.84364f,0.84836f,0.85301f,0.85758f,0.86206f,0.86646f,
    0.87078f,0.87502f,0.87918f,0.88326f,0.88726f,0.89118f,0.89501f,0.89877f,0.90245f,0.90605f,
    0.90957f,0.91301f,0.91638f,0.91966f,0.92288f,0.92601f,0.92908f,0.93206f,0.93498f,0.93782f,
    0.94059f,0.94329f,0.94592f,0.94848f,0.95097f,0.95339f,0.95575f,0.95804f,0.96027f,0.96244f,
    0.96454f,0.96658f,0.96856f,0.97049f,0.97235f,0.97416f,0.97591f,0.97760f,0.97924f,0.98083f,
    0.98237f,0.98386f,0.98530f,0.98669f,0.98803f,0.98933f,0.99058f,0.99179f,0.99295f,0.99408f,
    0.99516f,0.99620f,0.99721f,0.99817f,0.99910f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00001f,0.00002f,0.00005f,0.00008f,0.00012f,0.00019f,0.00026f,
    0.00036f,0.00048f,0.00063f,0.00080f,0.00099f,0.00122f,0.00148f,0.00178f,0.00211f,0.00249f,
    0.00290f,0.00335f,0.00386f,0.00440f,0.00500f,0.00565f,0.00636f,0.00711f,0.00793f,0.00881f,
    0.00975f,0.01075f,0.01182f,0.01295f,0.01416f,0.01543f,0.01678f,0.01821f,0.01971f,0.02129f,
    0.02295f,0.02469f,0.02652f,0.02843f,0.03043f,0.03252f,0.03469f,0.03696f,0.03933f,0.04178f,
    0.04433f,0.04698f,0.04973f,0.05258f,0.05552f,0.05857f,0.06172f,0.06498f,0.06834f,0.07180f,
    0.07537f,0.07905f,0.08283f,0.08672f,0.09072f,0.09483f,0.09905f,0.10337f,0.10781f,0.11236f,
    0.11701f,0.12178f,0.12665f,0.13163f,0.13673f,0.14193f,0.14724f,0.15265f,0.15817f,0.16380f,
    0.16954f,0.17538f,0.18132f,0.18737f,0.19351f,0.19976f,0.20610f,0.21255f,0.21908f,0.22572f,
    0.23244f,0.23926f,0.24616f,0.25316f,0.26023f,0.26739f,0.27464f,0.28196f,0.28935f,0.29683f,
    0.30437f,0.31198f,0.31966f,0.32740f,0.33521f,0.34307f,0.35099f,0.35896f,0.36699f,0.37506f,
    0.38317f,0.39133f,0.39952f,0.40775f,0.41601f,0.42429f,0.43261f,0.44094f,0.44929f,0.45766f,
    0.46604f,0.47443f,0.48283f,0.49122f,0.49962f,0.50801f,0.51639f,0.52476f,0.53312f,0.54146f,
    0.54978f,0.55807f,0.56633f,0.57457f,0.58277f,0.59093f,0.59905f,0.60713f,0.61516f,0.62314f,
    0.63107f,0.63895f,0.64676f,0.65452f,0.66221f,0.66984f,0.67739f,0.68488f,0.69229f,0.69963f,
    0.70689f,0.71407f,0.72117f,0.72818f,0.73511f,0.74195f,0.74870f,0.75536f,0.76192f,0.76839f,
    0.77477f,0.78105f,0.78723f,0.79331f,0.79930f,0.80518f,0.81096f,0.81664f,0.82221f,0.82768f,
    0.83305f,0.83832f,0.84347f,0.84853f,0.85348f,0.85832f,0.86306f,0.86770f,0.87223f,0.87666f,
    0.88098f,0.88521f,0.88933f,0.89334f,0.89726f,0.90108f,0.90480f,0.90842f,0.91194f,0.91537f,
    0.91870f,0.92193f,0.92508f,0.92813f,0.93109f,0.93396f,0.93675f,0.93945f,0.94206f,0.94459f,
    0.94704f,0.94941f,0.95169f,0.95391f,0.95604f,0.95810f,0.96009f,0.96201f,0.96386f,0.96564f,
    0.96735f,0.96900f,0.97059f,0.97212f,0.97358f,0.97499f,0.97634f,0.97764f,0.97888f,0.98007f,
    0.98122f,0.98231f,0.98336f,0.98436f,0.98531f,0.98623f,0.98710f,0.98793f,0.98873f,0.98949f,
    0.99021f,0.99090f,0.99155f,0.99218f,0.99277f,0.99333f,0.99387f,0.99437f,0.99486f,0.99531f,
    0.99575f,0.99616f,0.99654f,0.99691f,0.99726f,0.99759f,0.99790f,0.99819f,0.99847f,0.99873f,
    0.99897f,0.99920f,0.99942f,0.99963f,0.99982f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00001f,0.00003f,0.00006f,0.00010f,0.00017f,0.00025f,0.00035f,
    0.00048f,0.00064f,0.00083f,0.00106f,0.00132f,0.00163f,0.00197f,0.00237f,0.00281f,0.00330f,
    0.00385f,0.00446f,0.00513f,0.00585f,0.00665f,0.00751f,0.00845f,0.00945f,0.01054f,0.01170f,
    0.01295f,0.01428f,0.01569f,0.01719f,0.01879f,0.02048f,0.02227f,0.02415f,0.02614f,0.02822f,
    0.03042f,0.03272f,0.03513f,0.03765f,0.04028f,0.04303f,0.04589f,0.04887f,0.05198f,0.05520f,
    0.05855f,0.06202f,0.06561f,0.06933f,0.07318f,0.07716f,0.08127f,0.08550f,0.08987f,0.09437f,
    0.09900f,0.10376f,0.10866f,0.11369f,0.11884f,0.12414f,0.12956f,0.13512f,0.14080f,0.14662f,
    0.15257f,0.15865f,0.16485f,0.17118f,0.17764f,0.18423f,0.19093f,0.19776f,0.20471f,0.21177f,
    0.21895f,0.22625f,0.23365f,0.24117f,0.24879f,0.25652f,0.26435f,0.27228f,0.28030f,0.28842f,
    0.29662f,0.30492f,0.31329f,0.32175f,0.33028f,0.33889f,0.34756f,0.35630f,0.36510f,0.37396f,
    0.38287f,0.39183f,0.40084f,0.40989f,0.41897f,0.42809f,0.43723f,0.44640f,0.45559f,0.46479f,
    0.47401f,0.48323f,0.49245f,0.50167f,0.51088f,0.52008f,0.52927f,0.53843f,0.54757f,0.55668f,
    0.56575f,0.57479f,0.58379f,0.59274f,0.60164f,0.61048f,0.61927f,0.62799f,0.63665f,0.64524f,
    0.65376f,0.66220f,0.67056f,0.67883f,0.68702f,0.69511f,0.70312f,0.71103f,0.71884f,0.72655f,
    0.73415f,0.74165f,0.74904f,0.75632f,0.76348f,0.77053f,0.77747f,0.78428f,0.79098f,0.79756f,
    0.80401f,0.81035f,0.81655f,0.82264f,0.82859f,0.83443f,0.84013f,0.84571f,0.85117f,0.85649f,
    0.86169f,0.86677f,0.87172f,0.87654f,0.88124f,0.88581f,0.89026f,0.89459f,0.89880f,0.90289f,
    0.90686f,0.91071f,0.91445f,0.91807f,0.92157f,0.92497f,0.92826f,0.93143f,0.93450f,0.93747f,
    0.94034f,0.94310f,0.94577f,0.94833f,0.95081f,0.95319f,0.95548f,0.95768f,0.95980f,0.96183f,
    0.96378f,0.96565f,0.96744f,0.96916f,0.97081f,0.97238f,0.97388f,0.97532f,0.97669f,0.97801f,
    0.97926f,0.98045f,0.98158f,0.98266f,0.98369f,0.98467f,0.98560f,0.98648f,0.98732f,0.98811f,
    0.98886f,0.98958f,0.99025f,0.99089f,0.99149f,0.99206f,0.99260f,0.99311f,0.99359f,0.99404f,
    0.99446f,0.99486f,0.99523f,0.99559f,0.99592f,0.99623f,0.99652f,0.99679f,0.99705f,0.99729f,
    0.99751f,0.99772f,0.99792f,0.99810f,0.99827f,0.99843f,0.99857f,0.99871f,0.99884f,0.99896f,
    0.99907f,0.99917f,0.99926f,0.99935f,0.99943f,0.99951f,0.99958f,0.99964f,0.99970f,0.99975f,
    0.99980f,0.99985f,0.99989f,0.99993f,0.99997f,1.00000f
  }
};

#define gld_CalcLightLevel(lightlevel) (lighttable[1][max(min((lightlevel),255),0)])

// ==========================================================================
//                                                               VIEW GLOBALS
// ==========================================================================
// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW ANGLE_90
#define ABS(x) ((x) < 0 ? -(x) : (x))

static angle_t gr_clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
static INT32 gr_viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
static angle_t gr_xtoviewangle[MAXVIDWIDTH+1];
>>>>>>> srb2/next

// ==========================================================================
// Globals
// ==========================================================================

// base values set at SetViewSize
static float gr_basecentery;
static float gr_basecenterx;

float gr_baseviewwindowy, gr_basewindowcentery;
float gr_baseviewwindowx, gr_basewindowcenterx;
float gr_viewwidth, gr_viewheight; // viewport clipping boundaries (screen coords)

static float gr_centerx;
static float gr_viewwindowx;
static float gr_windowcenterx; // center of view window, for projection

static float gr_centery;
static float gr_viewwindowy; // top left corner of view window
static float gr_windowcentery;

static float gr_pspritexscale, gr_pspriteyscale;


static seg_t *gr_curline;
static side_t *gr_sidedef;
static line_t *gr_linedef;
static sector_t *gr_frontsector;
static sector_t *gr_backsector;

boolean gr_shadersavailable = true;

// ==========================================================================
// View position
// ==========================================================================

FTransform atransform;

// Float variants of viewx, viewy, viewz, etc.
static float gr_viewx, gr_viewy, gr_viewz;
static float gr_viewsin, gr_viewcos;

static angle_t gr_aimingangle;
static float gr_viewludsin, gr_viewludcos;

static INT32 drawcount = 0;

// ==========================================================================
// Lighting
// ==========================================================================

void HWR_Lighting(FSurfaceInfo *Surface, INT32 light_level, extracolormap_t *colormap)
{
	RGBA_t poly_color, tint_color, fade_color;

	poly_color.rgba = 0xFFFFFFFF;
	tint_color.rgba = (colormap != NULL) ? (UINT32)colormap->rgba : GL_DEFAULTMIX;
	fade_color.rgba = (colormap != NULL) ? (UINT32)colormap->fadergba : GL_DEFAULTFOG;

	// Crappy backup coloring if you can't do shaders
	if (!(cv_grshaders.value && gr_shadersavailable))
	{
		// be careful, this may get negative for high lightlevel values.
		float tint_alpha, fade_alpha;
		float red, green, blue;

		red = (float)poly_color.s.red;
		green = (float)poly_color.s.green;
		blue = (float)poly_color.s.blue;

		// 48 is just an arbritrary value that looked relatively okay.
		tint_alpha = (float)(sqrt(tint_color.s.alpha) * 48) / 255.0f;

		// 8 is roughly the brightness of the "close" color in Software, and 16 the brightness of the "far" color.
		// 8 is too bright for dark levels, and 16 is too dark for bright levels.
		// 12 is the compromise value. It doesn't look especially good anywhere, but it's the most balanced.
		// (Also, as far as I can tell, fade_color's alpha is actually not used in Software, so we only use light level.)
		fade_alpha = (float)(sqrt(255-light_level) * 12) / 255.0f;

		// Clamp the alpha values
		tint_alpha = min(max(tint_alpha, 0.0f), 1.0f);
		fade_alpha = min(max(fade_alpha, 0.0f), 1.0f);

		red = (tint_color.s.red * tint_alpha) + (red * (1.0f - tint_alpha));
		green = (tint_color.s.green * tint_alpha) + (green * (1.0f - tint_alpha));
		blue = (tint_color.s.blue * tint_alpha) + (blue * (1.0f - tint_alpha));

		red = (fade_color.s.red * fade_alpha) + (red * (1.0f - fade_alpha));
		green = (fade_color.s.green * fade_alpha) + (green * (1.0f - fade_alpha));
		blue = (fade_color.s.blue * fade_alpha) + (blue * (1.0f - fade_alpha));

		poly_color.s.red = (UINT8)red;
		poly_color.s.green = (UINT8)green;
		poly_color.s.blue = (UINT8)blue;
	}

	Surface->PolyColor.rgba = poly_color.rgba;
	Surface->TintColor.rgba = tint_color.rgba;
	Surface->FadeColor.rgba = fade_color.rgba;
	Surface->LightInfo.light_level = light_level;
	Surface->LightInfo.fade_start = (colormap != NULL) ? colormap->fadestart : 0;
	Surface->LightInfo.fade_end = (colormap != NULL) ? colormap->fadeend : 31;
}

UINT8 HWR_FogBlockAlpha(INT32 light, extracolormap_t *colormap) // Let's see if this can work
{
	RGBA_t realcolor, surfcolor;
	INT32 alpha;

	realcolor.rgba = (colormap != NULL) ? colormap->rgba : GL_DEFAULTMIX;

	if (!(cv_grshaders.value && gr_shadersavailable))
	{
		light = light - (255 - light);

		// Don't go out of bounds
		if (light < 0)
			light = 0;
		else if (light > 255)
			light = 255;

		alpha = (realcolor.s.alpha*255)/25;

		// at 255 brightness, alpha is between 0 and 127, at 0 brightness alpha will always be 255
		surfcolor.s.alpha = (alpha*light) / (2*256) + 255-light;
	}
	else
	{
		surfcolor.s.alpha = (255 - light);
	}

	return surfcolor.s.alpha;
}

// Lightnum = current light
// line = the seg to get the light offset from
static FUINT HWR_CalcWallLight(seg_t *line, FUINT lightnum)
{
	INT16 finallight = lightnum;

	fixed_t extralight = 0;

	if (cv_grfakecontrast.value == 1) // Smooth setting
		extralight += line->hwLightOffset;
	else
		extralight += line->lightOffset * 8;

	if (extralight != 0)
	{
		finallight += extralight;

		if (finallight < 0)
			finallight = 0;
		if (finallight > 255)
			finallight = 255;
	}

	return (FUINT)finallight;
}

// ==========================================================================
// Floor and ceiling generation from subsectors
// ==========================================================================

<<<<<<< HEAD
// HWR_RenderPlane
// Render a floor or ceiling convex polygon
void HWR_RenderPlane(extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight, FBITFIELD PolyFlags, INT32 lightlevel, lumpnum_t lumpnum, sector_t *FOFsector, UINT8 alpha, extracolormap_t *planecolormap)
=======
#ifdef DOPLANES

// -----------------+
// HWR_RenderPlane  : Render a floor or ceiling convex polygon
// -----------------+
static void HWR_RenderPlane(subsector_t *subsector, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight,
                           FBITFIELD PolyFlags, INT32 lightlevel, levelflat_t *levelflat, sector_t *FOFsector, UINT8 alpha, boolean fogplane, extracolormap_t *planecolormap)
>>>>>>> srb2/next
{
	polyvertex_t *  pv;
	float           height; //constant y for all points on the convex flat polygon
	FOutVector      *v3d;
	INT32             nrPlaneVerts;   //verts original define of convex flat polygon
	INT32             i;
	float           flatxref,flatyref;
	float fflatwidth = 64.0f, fflatheight = 64.0f;
	INT32 flatflag = 63;
	boolean texflat = false;
	size_t len;
	float scrollx = 0.0f, scrolly = 0.0f;
	angle_t angle = 0;
	FSurfaceInfo    Surf;
	fixed_t tempxsow, tempytow;
	pslope_t *slope = NULL;

	static FOutVector *planeVerts = NULL;
	static UINT16 numAllocedPlaneVerts = 0;

	// no convex poly were generated for this subsector
	if (!xsub->planepoly)
		return;

	// Get the slope pointer to simplify future code
	if (FOFsector)
	{
		if (FOFsector->f_slope && !isceiling)
			slope = FOFsector->f_slope;
		else if (FOFsector->c_slope && isceiling)
			slope = FOFsector->c_slope;
	}
	else
	{
		if (gr_frontsector->f_slope && !isceiling)
			slope = gr_frontsector->f_slope;
		else if (gr_frontsector->c_slope && isceiling)
			slope = gr_frontsector->c_slope;
	}

	// Set fixedheight to the slope's height from our viewpoint, if we have a slope
	if (slope)
		fixedheight = P_GetSlopeZAt(slope, viewx, viewy);

	height = FIXED_TO_FLOAT(fixedheight);

	pv  = xsub->planepoly->pts;
	nrPlaneVerts = xsub->planepoly->numpts;

	if (nrPlaneVerts < 3)   //not even a triangle ?
		return;

<<<<<<< HEAD
=======
	// This check is so inconsistent between functions, it hurts.
>>>>>>> srb2/next
	if (nrPlaneVerts > INT16_MAX) // FIXME: exceeds plVerts size
	{
		CONS_Debug(DBG_RENDER, "polygon size of %d exceeds max value of %d vertices\n", nrPlaneVerts, INT16_MAX);
		return;
	}

	// Allocate plane-vertex buffer if we need to
	if (!planeVerts || nrPlaneVerts > numAllocedPlaneVerts)
	{
		numAllocedPlaneVerts = (UINT16)nrPlaneVerts;
		Z_Free(planeVerts);
		Z_Malloc(numAllocedPlaneVerts * sizeof (FOutVector), PU_LEVEL, &planeVerts);
	}

	// set texture for polygon
	if (levelflat != NULL)
	{
		if (levelflat->type == LEVELFLAT_TEXTURE)
		{
			fflatwidth = textures[levelflat->u.texture.num]->width;
			fflatheight = textures[levelflat->u.texture.num]->height;
			texflat = true;
		}
		else if (levelflat->type == LEVELFLAT_FLAT)
		{
			len = W_LumpLength(levelflat->u.flat.lumpnum);

			switch (len)
			{
				case 4194304: // 2048x2048 lump
					fflatwidth = fflatheight = 2048.0f;
					break;
				case 1048576: // 1024x1024 lump
					fflatwidth = fflatheight = 1024.0f;
					break;
				case 262144:// 512x512 lump
					fflatwidth = fflatheight = 512.0f;
					break;
				case 65536: // 256x256 lump
					fflatwidth = fflatheight = 256.0f;
					break;
				case 16384: // 128x128 lump
					fflatwidth = fflatheight = 128.0f;
					break;
				case 1024: // 32x32 lump
					fflatwidth = fflatheight = 32.0f;
					break;
				default: // 64x64 lump
					fflatwidth = fflatheight = 64.0f;
					break;
			}

			flatflag = ((INT32)fflatwidth)-1;
		}
	}
	else // set no texture
		HWD.pfnSetTexture(NULL);

	// reference point for flat texture coord for each vertex around the polygon
	flatxref = (float)(((fixed_t)pv->x & (~flatflag)) / fflatwidth);
	flatyref = (float)(((fixed_t)pv->y & (~flatflag)) / fflatheight);

	// transform
	if (FOFsector != NULL)
	{
		if (!isceiling) // it's a floor
		{
<<<<<<< HEAD
			scrollx = FIXED_TO_FLOAT(FOFsector->floor_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(FOFsector->floor_yoffs)/fflatsize;
=======
			scrollx = FIXED_TO_FLOAT(FOFsector->floor_xoffs)/fflatwidth;
			scrolly = FIXED_TO_FLOAT(FOFsector->floor_yoffs)/fflatheight;
>>>>>>> srb2/next
			angle = FOFsector->floorpic_angle;
		}
		else // it's a ceiling
		{
<<<<<<< HEAD
			scrollx = FIXED_TO_FLOAT(FOFsector->ceiling_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(FOFsector->ceiling_yoffs)/fflatsize;
=======
			scrollx = FIXED_TO_FLOAT(FOFsector->ceiling_xoffs)/fflatwidth;
			scrolly = FIXED_TO_FLOAT(FOFsector->ceiling_yoffs)/fflatheight;
>>>>>>> srb2/next
			angle = FOFsector->ceilingpic_angle;
		}
	}
	else if (gr_frontsector)
	{
		if (!isceiling) // it's a floor
		{
<<<<<<< HEAD
			scrollx = FIXED_TO_FLOAT(gr_frontsector->floor_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->floor_yoffs)/fflatsize;
=======
			scrollx = FIXED_TO_FLOAT(gr_frontsector->floor_xoffs)/fflatwidth;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->floor_yoffs)/fflatheight;
>>>>>>> srb2/next
			angle = gr_frontsector->floorpic_angle;
		}
		else // it's a ceiling
		{
<<<<<<< HEAD
			scrollx = FIXED_TO_FLOAT(gr_frontsector->ceiling_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->ceiling_yoffs)/fflatsize;
=======
			scrollx = FIXED_TO_FLOAT(gr_frontsector->ceiling_xoffs)/fflatwidth;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->ceiling_yoffs)/fflatheight;
>>>>>>> srb2/next
			angle = gr_frontsector->ceilingpic_angle;
		}
	}


	if (angle) // Only needs to be done if there's an altered angle
	{
<<<<<<< HEAD

		angle = (InvAngle(angle)+ANGLE_180)>>ANGLETOFINESHIFT;

		// This needs to be done so that it scrolls in a different direction after rotation like software
		/*tempxsow = FLOAT_TO_FIXED(scrollx);
		tempytow = FLOAT_TO_FIXED(scrolly);
		scrollx = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
		scrolly = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINESINE(angle)) + FixedMul(tempytow, FINECOSINE(angle))));*/

=======
		angle = InvAngle(angle)>>ANGLETOFINESHIFT;
>>>>>>> srb2/next
		// This needs to be done so everything aligns after rotation
		// It would be done so that rotation is done, THEN the translation, but I couldn't get it to rotate AND scroll like software does
		tempxsow = FLOAT_TO_FIXED(flatxref);
		tempytow = FLOAT_TO_FIXED(flatyref);
		flatxref = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
		flatyref = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINESINE(angle)) + FixedMul(tempytow, FINECOSINE(angle))));
	}

<<<<<<< HEAD

	for (i = 0; i < nrPlaneVerts; i++,v3d++,pv++)
	{
		// Hurdler: add scrolling texture on floor/ceiling
		v3d->s = (float)((pv->x / fflatsize) - flatxref + scrollx);
		v3d->t = (float)(flatyref - (pv->y / fflatsize) + scrolly);

		//v3d->s = (float)(pv->x / fflatsize);
		//v3d->t = (float)(pv->y / fflatsize);

		// Need to rotate before translate
		if (angle) // Only needs to be done if there's an altered angle
		{
			tempxsow = FLOAT_TO_FIXED(v3d->s);
			tempytow = FLOAT_TO_FIXED(v3d->t);
			v3d->s = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
			v3d->t = (FIXED_TO_FLOAT(-FixedMul(tempxsow, FINESINE(angle)) - FixedMul(tempytow, FINECOSINE(angle))));
		}

		//v3d->s = (float)(v3d->s - flatxref + scrollx);
		//v3d->t = (float)(flatyref - v3d->t + scrolly);
=======
#define SETUP3DVERT(vert, vx, vy) {\
		/* Hurdler: add scrolling texture on floor/ceiling */\
		if (texflat)\
		{\
			vert->sow = (float)((vx) / fflatwidth) + scrollx;\
			vert->tow = -(float)((vy) / fflatheight) + scrolly;\
		}\
		else\
		{\
			vert->sow = (float)(((vx) / fflatwidth) - flatxref + scrollx);\
			vert->tow = (float)(flatyref - ((vy) / fflatheight) + scrolly);\
		}\
\
		/* Need to rotate before translate */\
		if (angle) /* Only needs to be done if there's an altered angle */\
		{\
			tempxsow = FLOAT_TO_FIXED(vert->sow);\
			tempytow = FLOAT_TO_FIXED(vert->tow);\
			if (texflat)\
				tempytow = -tempytow;\
			vert->sow = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));\
			vert->tow = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINESINE(angle)) + FixedMul(tempytow, FINECOSINE(angle))));\
		}\
\
		vert->x = (vx);\
		vert->y = height;\
		vert->z = (vy);\
\
		if (slope)\
		{\
			fixedheight = P_GetSlopeZAt(slope, FLOAT_TO_FIXED((vx)), FLOAT_TO_FIXED((vy)));\
			vert->y = FIXED_TO_FLOAT(fixedheight);\
		}\
}
>>>>>>> srb2/next

	for (i = 0, v3d = planeVerts; i < nrPlaneVerts; i++,v3d++,pv++)
		SETUP3DVERT(v3d, pv->x, pv->y);

	HWR_Lighting(&Surf, lightlevel, planecolormap);

<<<<<<< HEAD
	if (PolyFlags & (PF_Translucent|PF_Fog))
=======
	// use different light tables
	// for horizontal / vertical / diagonal
	// note: try to get the same visual feel as the original
	Surf.FlatColor.s.red = Surf.FlatColor.s.green =
	Surf.FlatColor.s.blue = LightLevelToLum(lightlevel); //  Don't take from the frontsector, or the game will crash

	if (planecolormap)
>>>>>>> srb2/next
	{
		Surf.PolyColor.s.alpha = (UINT8)alpha;
		PolyFlags |= PF_Modulated;
	}
	else
		PolyFlags |= PF_Masked|PF_Modulated;

	if (PolyFlags & PF_Fog)
		HWD.pfnSetShader(6);	// fog shader
	else if (PolyFlags & PF_Ripple)
		HWD.pfnSetShader(5);	// water shader
	else
		HWD.pfnSetShader(1);	// floor shader

	HWD.pfnDrawPolygon(&Surf, planeVerts, nrPlaneVerts, PolyFlags);
<<<<<<< HEAD
=======

	if (subsector)
	{
		// Horizon lines
		FOutVector horizonpts[6];
		float dist, vx, vy;
		float x1, y1, xd, yd;
		UINT8 numplanes, j;
		vertex_t v; // For determining the closest distance from the line to the camera, to split render planes for minimum distortion;

		const float renderdist = 27000.0f; // How far out to properly render the plane
		const float farrenderdist = 32768.0f; // From here, raise plane to horizon level to fill in the line with some texture distortion

		seg_t *line = &segs[subsector->firstline];

		for (i = 0; i < subsector->numlines; i++, line++)
		{
			if (!line->glseg && line->linedef->special == HORIZONSPECIAL && R_PointOnSegSide(dup_viewx, dup_viewy, line) == 0)
			{
				P_ClosestPointOnLine(viewx, viewy, line->linedef, &v);
				dist = FIXED_TO_FLOAT(R_PointToDist(v.x, v.y));

				x1 = ((polyvertex_t *)line->pv1)->x;
				y1 = ((polyvertex_t *)line->pv1)->y;
				xd = ((polyvertex_t *)line->pv2)->x - x1;
				yd = ((polyvertex_t *)line->pv2)->y - y1;

				// Based on the seg length and the distance from the line, split horizon into multiple poly sets to reduce distortion
				dist = sqrtf((xd*xd) + (yd*yd)) / dist / 16.0f;
				if (dist > 100.0f)
					numplanes = 100;
				else
					numplanes = (UINT8)dist + 1;

				for (j = 0; j < numplanes; j++)
				{
					// Left side
					vx = x1 + xd * j / numplanes;
					vy = y1 + yd * j / numplanes;
					SETUP3DVERT((&horizonpts[1]), vx, vy);

					dist = sqrtf(powf(vx - gr_viewx, 2) + powf(vy - gr_viewy, 2));
					vx = (vx - gr_viewx) * renderdist / dist + gr_viewx;
					vy = (vy - gr_viewy) * renderdist / dist + gr_viewy;
					SETUP3DVERT((&horizonpts[0]), vx, vy);

					// Right side
					vx = x1 + xd * (j+1) / numplanes;
					vy = y1 + yd * (j+1) / numplanes;
					SETUP3DVERT((&horizonpts[2]), vx, vy);

					dist = sqrtf(powf(vx - gr_viewx, 2) + powf(vy - gr_viewy, 2));
					vx = (vx - gr_viewx) * renderdist / dist + gr_viewx;
					vy = (vy - gr_viewy) * renderdist / dist + gr_viewy;
					SETUP3DVERT((&horizonpts[3]), vx, vy);

					// Horizon fills
					vx = (horizonpts[0].x - gr_viewx) * farrenderdist / renderdist + gr_viewx;
					vy = (horizonpts[0].z - gr_viewy) * farrenderdist / renderdist + gr_viewy;
					SETUP3DVERT((&horizonpts[5]), vx, vy);
					horizonpts[5].y = gr_viewz;

					vx = (horizonpts[3].x - gr_viewx) * farrenderdist / renderdist + gr_viewx;
					vy = (horizonpts[3].z - gr_viewy) * farrenderdist / renderdist + gr_viewy;
					SETUP3DVERT((&horizonpts[4]), vx, vy);
					horizonpts[4].y = gr_viewz;

					// Draw
					HWD.pfnDrawPolygon(&Surf, horizonpts, 6, PolyFlags);
				}
			}
		}
	}

#ifdef ALAM_LIGHTING
	// add here code for dynamic lighting on planes
	HWR_PlaneLighting(planeVerts, nrPlaneVerts);
#endif
}

#ifdef POLYSKY
// this don't draw anything it only update the z-buffer so there isn't problem with
// wall/things upper that sky (map12)
static void HWR_RenderSkyPlane(extrasubsector_t *xsub, fixed_t fixedheight)
{
	polyvertex_t *pv;
	float height; //constant y for all points on the convex flat polygon
	FOutVector *v3d;
	INT32 nrPlaneVerts;   //verts original define of convex flat polygon
	INT32 i;

	// no convex poly were generated for this subsector
	if (!xsub->planepoly)
		return;

	height = FIXED_TO_FLOAT(fixedheight);

	pv  = xsub->planepoly->pts;
	nrPlaneVerts = xsub->planepoly->numpts;

	if (nrPlaneVerts < 3) // not even a triangle?
		return;

	if (nrPlaneVerts > MAXPLANEVERTICES) // FIXME: exceeds plVerts size
	{
		CONS_Debug(DBG_RENDER, "polygon size of %d exceeds max value of %d vertices\n", nrPlaneVerts, MAXPLANEVERTICES);
		return;
	}

	// transform
	v3d = planeVerts;
	for (i = 0; i < nrPlaneVerts; i++,v3d++,pv++)
	{
		v3d->sow = 0.0f;
		v3d->tow = 0.0f;
		v3d->x = pv->x;
		v3d->y = height;
		v3d->z = pv->y;
	}

	HWD.pfnDrawPolygon(NULL, planeVerts, nrPlaneVerts,
	 PF_Clip|PF_Invisible|PF_NoTexture|PF_Occlude);
>>>>>>> srb2/next
}

#ifdef WALLSPLATS
static void HWR_DrawSegsSplats(FSurfaceInfo * pSurf)
{
	FOutVector wallVerts[4];
	wallsplat_t *splat;
	GLPatch_t *gpatch;
	fixed_t i;
	// seg bbox
	fixed_t segbbox[4];

	M_ClearBox(segbbox);
	M_AddToBox(segbbox,
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->x),
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->y));
	M_AddToBox(segbbox,
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->x),
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->y));

	splat = (wallsplat_t *)gr_curline->linedef->splats;
	for (; splat; splat = splat->next)
	{
		//BP: don't draw splat extern to this seg
		//    this is quick fix best is explain in logboris.txt at 12-4-2000
		if (!M_PointInBox(segbbox,splat->v1.x,splat->v1.y) && !M_PointInBox(segbbox,splat->v2.x,splat->v2.y))
			continue;

		gpatch = W_CachePatchNum(splat->patch, PU_PATCH);
		HWR_GetPatch(gpatch);

		wallVerts[0].x = wallVerts[3].x = FIXED_TO_FLOAT(splat->v1.x);
		wallVerts[0].z = wallVerts[3].z = FIXED_TO_FLOAT(splat->v1.y);
		wallVerts[2].x = wallVerts[1].x = FIXED_TO_FLOAT(splat->v2.x);
		wallVerts[2].z = wallVerts[1].z = FIXED_TO_FLOAT(splat->v2.y);

		i = splat->top;
		if (splat->yoffset)
			i += *splat->yoffset;

		wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(i)+(gpatch->height>>1);
		wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(i)-(gpatch->height>>1);

		wallVerts[3].s = wallVerts[3].t = wallVerts[2].s = wallVerts[0].t = 0.0f;
		wallVerts[1].s = wallVerts[1].t = wallVerts[2].t = wallVerts[0].s = 1.0f;

		switch (splat->flags & SPLATDRAWMODE_MASK)
		{
			case SPLATDRAWMODE_OPAQUE :
				pSurf.PolyColor.s.alpha = 0xff;
				i = PF_Translucent;
				break;
			case SPLATDRAWMODE_TRANS :
				pSurf.PolyColor.s.alpha = 128;
				i = PF_Translucent;
				break;
			case SPLATDRAWMODE_SHADE :
				pSurf.PolyColor.s.alpha = 0xff;
				i = PF_Substractive;
				break;
		}

		HWD.pfnSetShader(2);	// wall shader
		HWD.pfnDrawPolygon(&pSurf, wallVerts, 4, i|PF_Modulated|PF_Decal);
	}
}
#endif

FBITFIELD HWR_TranstableToAlpha(INT32 transtablenum, FSurfaceInfo *pSurf)
{
	switch (transtablenum)
	{
		case tr_trans10 : pSurf->PolyColor.s.alpha = 0xe6;return  PF_Translucent;
		case tr_trans20 : pSurf->PolyColor.s.alpha = 0xcc;return  PF_Translucent;
		case tr_trans30 : pSurf->PolyColor.s.alpha = 0xb3;return  PF_Translucent;
		case tr_trans40 : pSurf->PolyColor.s.alpha = 0x99;return  PF_Translucent;
		case tr_trans50 : pSurf->PolyColor.s.alpha = 0x80;return  PF_Translucent;
		case tr_trans60 : pSurf->PolyColor.s.alpha = 0x66;return  PF_Translucent;
		case tr_trans70 : pSurf->PolyColor.s.alpha = 0x4c;return  PF_Translucent;
		case tr_trans80 : pSurf->PolyColor.s.alpha = 0x33;return  PF_Translucent;
		case tr_trans90 : pSurf->PolyColor.s.alpha = 0x19;return  PF_Translucent;
	}
	return PF_Translucent;
}

// ==========================================================================
// Wall generation from subsector segs
// ==========================================================================

//
// HWR_ProjectWall
//
void HWR_ProjectWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blendmode, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	HWR_Lighting(pSurf, lightlevel, wallcolormap);

	HWD.pfnSetShader(2);	// wall shader
	HWD.pfnDrawPolygon(pSurf, wallVerts, 4, blendmode|PF_Modulated|PF_Occlude);

#ifdef WALLSPLATS
	if (gr_curline->linedef->splats && cv_splats.value)
		HWR_DrawSegsSplats(pSurf);
#endif
}

static FUINT HWR_CalcWallLight(FUINT lightnum, fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y)
{
	INT16 finallight = lightnum;

	if (cv_grfakecontrast.value != 0)
	{
		const UINT8 contrast = 8;
		fixed_t extralight = 0;

		if (v1y == v2y)
			extralight = -contrast;
		else if (v1x == v2x)
			extralight = contrast;

		if (extralight != 0)
		{
			finallight += extralight;

			if (finallight < 0)
				finallight = 0;
			if (finallight > 255)
				finallight = 255;
		}
	}

	return (FUINT)finallight;
}

//
// HWR_SplitWall
//
void HWR_SplitWall(sector_t *sector, FOutVector *wallVerts, INT32 texnum, FSurfaceInfo* Surf, INT32 cutflag, ffloor_t *pfloor)
{
	/* SoM: split up and light walls according to the
	 lightlist. This may also include leaving out parts
	 of the wall that can't be seen */

	float realtop, realbot, top, bot;
	float pegt, pegb, pegmul;
	float height = 0.0f, bheight = 0.0f;

	float endrealtop, endrealbot, endtop, endbot;
	float endpegt, endpegb, endpegmul;
	float endheight = 0.0f, endbheight = 0.0f;

	// compiler complains when P_GetSlopeZAt is used in FLOAT_TO_FIXED directly
	// use this as a temp var to store P_GetSlopeZAt's return value each time
	fixed_t temp;

	fixed_t v1x = FLOAT_TO_FIXED(wallVerts[0].x);
	fixed_t v1y = FLOAT_TO_FIXED(wallVerts[0].z); // not a typo
	fixed_t v2x = FLOAT_TO_FIXED(wallVerts[1].x);
	fixed_t v2y = FLOAT_TO_FIXED(wallVerts[1].z); // not a typo

	INT32 solid, i;
	lightlist_t *  list = sector->lightlist;
<<<<<<< HEAD
	const UINT8 alpha = Surf->PolyColor.s.alpha;
	FUINT lightnum = HWR_CalcWallLight(gr_curline, sector->lightlevel);
=======
	const UINT8 alpha = Surf->FlatColor.s.alpha;
	FUINT lightnum = HWR_CalcWallLight(sector->lightlevel, v1x, v1y, v2x, v2y);
>>>>>>> srb2/next
	extracolormap_t *colormap = NULL;

	realtop = top = wallVerts[3].y;
	realbot = bot = wallVerts[0].y;
	pegt = wallVerts[3].t;
	pegb = wallVerts[0].t;
	pegmul = (pegb - pegt) / (top - bot);

	endrealtop = endtop = wallVerts[2].y;
	endrealbot = endbot = wallVerts[1].y;
	endpegt = wallVerts[2].t;
	endpegb = wallVerts[1].t;
	endpegmul = (endpegb - endpegt) / (endtop - endbot);

	for (i = 0; i < sector->numlights; i++)
	{
<<<<<<< HEAD
#ifdef ESLOPE
		if (endtop < endrealbot)
#endif
		if (top < realbot)
			return;

=======
        if (endtop < endrealbot && top < realbot)
			return;

		// There's a compiler warning here if this comment isn't here because of indentation
>>>>>>> srb2/next
		if (!(list[i].flags & FF_NOSHADE))
		{
			if (pfloor && (pfloor->flags & FF_FOG))
			{
<<<<<<< HEAD
				lightnum = HWR_CalcWallLight(gr_curline, pfloor->master->frontsector->lightlevel);
=======
				lightnum = HWR_CalcWallLight(pfloor->master->frontsector->lightlevel, v1x, v1y, v2x, v2y);
>>>>>>> srb2/next
				colormap = pfloor->master->frontsector->extra_colormap;
			}
			else
			{
<<<<<<< HEAD
				lightnum = HWR_CalcWallLight(gr_curline, *list[i].lightlevel);
				colormap = list[i].extra_colormap;
=======
				lightnum = HWR_CalcWallLight(*list[i].lightlevel, v1x, v1y, v2x, v2y);
				colormap = *list[i].extra_colormap;
>>>>>>> srb2/next
			}
		}

		solid = false;

		if ((sector->lightlist[i].flags & FF_CUTSOLIDS) && !(cutflag & FF_EXTRA))
			solid = true;
		else if ((sector->lightlist[i].flags & FF_CUTEXTRA) && (cutflag & FF_EXTRA))
		{
			if (sector->lightlist[i].flags & FF_EXTRA)
			{
				if ((sector->lightlist[i].flags & (FF_FOG|FF_SWIMMABLE)) == (cutflag & (FF_FOG|FF_SWIMMABLE))) // Only merge with your own types
					solid = true;
			}
			else
				solid = true;
		}
		else
			solid = false;

		temp = P_GetLightZAt(&list[i], v1x, v1y);
		height = FIXED_TO_FLOAT(temp);
		temp = P_GetLightZAt(&list[i], v2x, v2y);
		endheight = FIXED_TO_FLOAT(temp);
		if (solid)
		{
			temp = P_GetFFloorBottomZAt(list[i].caster, v1x, v1y);
			bheight = FIXED_TO_FLOAT(temp);
			temp = P_GetFFloorBottomZAt(list[i].caster, v2x, v2y);
			endbheight = FIXED_TO_FLOAT(temp);
		}

		if (endheight >= endtop && height >= top)
		{
			if (solid && top > bheight)
				top = bheight;
			if (solid && endtop > endbheight)
				endtop = endbheight;
		}

		if (i + 1 < sector->numlights)
		{
			temp = P_GetLightZAt(&list[i+1], v1x, v1y);
			bheight = FIXED_TO_FLOAT(temp);
			temp = P_GetLightZAt(&list[i+1], v2x, v2y);
			endbheight = FIXED_TO_FLOAT(temp);
		}
		else
		{
			bheight = realbot;
			endbheight = endrealbot;
		}

		if (endbheight >= endtop && bheight >= top)
			continue;

		//Found a break;
		bot = bheight;

		if (bot < realbot)
			bot = realbot;

		endbot = endbheight;

		if (endbot < endrealbot)
			endbot = endrealbot;
<<<<<<< HEAD
#endif
		Surf->PolyColor.s.alpha = alpha;
=======
		Surf->FlatColor.s.alpha = alpha;
>>>>>>> srb2/next

		wallVerts[3].t = pegt + ((realtop - top) * pegmul);
		wallVerts[2].t = endpegt + ((endrealtop - endtop) * endpegmul);
		wallVerts[0].t = pegt + ((realtop - bot) * pegmul);
		wallVerts[1].t = endpegt + ((endrealtop - endbot) * endpegmul);

		// set top/bottom coords
		wallVerts[3].y = top;
		wallVerts[2].y = endtop;
		wallVerts[0].y = bot;
		wallVerts[1].y = endbot;

		if (cutflag & FF_FOG)
			HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Fog|PF_NoTexture, true, lightnum, colormap);
		else if (cutflag & FF_TRANSLUCENT)
			HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Translucent, false, lightnum, colormap);
		else
			HWR_ProjectWall(wallVerts, Surf, PF_Masked, lightnum, colormap);

		top = bot;
		endtop = endbot;
	}

	bot = realbot;
	endbot = endrealbot;
	if (endtop <= endrealbot && top <= realbot)
		return;

	Surf->PolyColor.s.alpha = alpha;

	wallVerts[3].t = pegt + ((realtop - top) * pegmul);
	wallVerts[2].t = endpegt + ((endrealtop - endtop) * endpegmul);
	wallVerts[0].t = pegt + ((realtop - bot) * pegmul);
	wallVerts[1].t = endpegt + ((endrealtop - endbot) * endpegmul);

	// set top/bottom coords
	wallVerts[3].y = top;
	wallVerts[2].y = endtop;
	wallVerts[0].y = bot;
	wallVerts[1].y = endbot;

	if (cutflag & FF_FOG)
		HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Fog|PF_NoTexture, true, lightnum, colormap);
	else if (cutflag & FF_TRANSLUCENT)
		HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Translucent, false, lightnum, colormap);
	else
		HWR_ProjectWall(wallVerts, Surf, PF_Masked, lightnum, colormap);
}

// HWR_DrawSkyWall
// Draw walls into the depth buffer so that anything behind is culled properly
<<<<<<< HEAD
void HWR_DrawSkyWall(FOutVector *wallVerts, FSurfaceInfo *Surf)
=======
static void HWR_DrawSkyWall(wallVert3D *wallVerts, FSurfaceInfo *Surf)
>>>>>>> srb2/next
{
	HWD.pfnSetTexture(NULL);
	// no texture
	wallVerts[3].t = wallVerts[2].t = 0;
	wallVerts[0].t = wallVerts[1].t = 0;
	wallVerts[0].s = wallVerts[3].s = 0;
	wallVerts[2].s = wallVerts[1].s = 0;
<<<<<<< HEAD

	HWR_ProjectWall(wallVerts, Surf, PF_Invisible|PF_NoTexture, 255, NULL);
=======
	// this no longer sets top/bottom coords, this should be done before caling the function
	HWR_ProjectWall(wallVerts, Surf, PF_Invisible|PF_Clip|PF_NoTexture, 255, NULL);
>>>>>>> srb2/next
	// PF_Invisible so it's not drawn into the colour buffer
	// PF_NoTexture for no texture
	// PF_Occlude is set in HWR_ProjectWall to draw into the depth buffer
}

//
// HWR_ProcessSeg
// A portion or all of a wall segment will be drawn, from startfrac to endfrac,
//  where 0 is the start of the segment, 1 the end of the segment
// Anything between means the wall segment has been clipped with solidsegs,
//  reducing wall overdraw to a minimum
//
void HWR_ProcessSeg(void) // Sort of like GLWall::Process in GZDoom
{
	FOutVector wallVerts[4];
	v2d_t vs, ve; // start, end vertices of 2d line (view from above)

	fixed_t worldtop, worldbottom;
	fixed_t worldhigh = 0, worldlow = 0;
	fixed_t worldtopslope, worldbottomslope;
	fixed_t worldhighslope = 0, worldlowslope = 0;
	fixed_t v1x, v1y, v2x, v2y;

	GLTexture_t *grTex = NULL;
	float cliplow = 0.0f, cliphigh = 0.0f;
	INT32 gr_midtexture;
	fixed_t h, l; // 3D sides and 2s middle textures
	fixed_t hS, lS;

	FUINT lightnum = 0; // shut up compiler
	extracolormap_t *colormap;
	FSurfaceInfo Surf;

	gr_sidedef = gr_curline->sidedef;
	gr_linedef = gr_curline->linedef;

	vs.x = ((polyvertex_t *)gr_curline->pv1)->x;
	vs.y = ((polyvertex_t *)gr_curline->pv1)->y;
	ve.x = ((polyvertex_t *)gr_curline->pv2)->x;
	ve.y = ((polyvertex_t *)gr_curline->pv2)->y;

	v1x = FLOAT_TO_FIXED(vs.x);
	v1y = FLOAT_TO_FIXED(vs.y);
	v2x = FLOAT_TO_FIXED(ve.x);
	v2y = FLOAT_TO_FIXED(ve.y);

#define SLOPEPARAMS(slope, end1, end2, normalheight) \
	end1 = P_GetZAt(slope, v1x, v1y, normalheight); \
	end2 = P_GetZAt(slope, v2x, v2y, normalheight);

	SLOPEPARAMS(gr_frontsector->c_slope, worldtop,    worldtopslope,    gr_frontsector->ceilingheight)
	SLOPEPARAMS(gr_frontsector->f_slope, worldbottom, worldbottomslope, gr_frontsector->floorheight)

	// remember vertices ordering
	//  3--2
	//  | /|
	//  |/ |
	//  0--1
	// make a wall polygon (with 2 triangles), using the floor/ceiling heights,
	// and the 2d map coords of start/end vertices
	wallVerts[0].x = wallVerts[3].x = vs.x;
	wallVerts[0].z = wallVerts[3].z = vs.y;
	wallVerts[2].x = wallVerts[1].x = ve.x;
	wallVerts[2].z = wallVerts[1].z = ve.y;

	// x offset the texture
	{
		fixed_t texturehpeg = gr_sidedef->textureoffset + gr_curline->offset;
		cliplow = (float)texturehpeg;
		cliphigh = (float)(texturehpeg + (gr_curline->flength*FRACUNIT));
	}

<<<<<<< HEAD
	lightnum = HWR_CalcWallLight(gr_curline, gr_frontsector->lightlevel);
=======
	lightnum = HWR_CalcWallLight(gr_frontsector->lightlevel, vs.x, vs.y, ve.x, ve.y);
>>>>>>> srb2/next
	colormap = gr_frontsector->extra_colormap;

	if (gr_frontsector)
		Surf.PolyColor.s.alpha = 255;

	if (gr_backsector)
	{
		INT32 gr_toptexture = 0, gr_bottomtexture = 0;
		// two sided line
		boolean bothceilingssky = false; // turned on if both back and front ceilings are sky
		boolean bothfloorssky = false; // likewise, but for floors

		SLOPEPARAMS(gr_backsector->c_slope, worldhigh, worldhighslope, gr_backsector->ceilingheight)
		SLOPEPARAMS(gr_backsector->f_slope, worldlow,  worldlowslope,  gr_backsector->floorheight)
#undef SLOPEPARAMS

<<<<<<< HEAD
		// Sky culling
		if (!gr_curline->polyseg) // Don't do it for polyobjects
		{
			// Sky Ceilings
			wallVerts[3].y = wallVerts[2].y = FIXED_TO_FLOAT(INT32_MAX);

			if (gr_frontsector->ceilingpic == skyflatnum)
			{
				if (gr_backsector->ceilingpic == skyflatnum)
				{
					// Both front and back sectors are sky, needs skywall from the frontsector's ceiling, but only if the
					// backsector is lower
					if ((worldhigh <= worldtop && worldhighslope <= worldtopslope)// Assuming ESLOPE is always on with my changes
					&& (worldhigh != worldtop || worldhighslope != worldtopslope))
					// Removing the second line above will render more rarely visible skywalls. Example: Cave garden ceiling in Dark race
					{
#ifdef ESLOPE
						wallVerts[0].y = FIXED_TO_FLOAT(worldhigh);
						wallVerts[1].y = FIXED_TO_FLOAT(worldhighslope);
#else
						wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(worldhigh);
#endif
						HWR_DrawSkyWall(wallVerts, &Surf);
					}
				}
				else
				{
					// Only the frontsector is sky, just draw a skywall from the front ceiling
#ifdef ESLOPE
					wallVerts[0].y = FIXED_TO_FLOAT(worldtop);
					wallVerts[1].y = FIXED_TO_FLOAT(worldtopslope);
#else
					wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(worldtop);
#endif
					HWR_DrawSkyWall(wallVerts, &Surf);
				}
			}
			else if (gr_backsector->ceilingpic == skyflatnum)
			{
				// Only the backsector is sky, just draw a skywall from the front ceiling
#ifdef ESLOPE
				wallVerts[0].y = FIXED_TO_FLOAT(worldtop);
				wallVerts[1].y = FIXED_TO_FLOAT(worldtopslope);
#else
				wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(worldtop);
#endif
				HWR_DrawSkyWall(wallVerts, &Surf);
			}
=======
		// hack to allow height changes in outdoor areas
		// This is what gets rid of the upper textures if there should be sky
		if (gr_frontsector->ceilingpic == skyflatnum
			&& gr_backsector->ceilingpic  == skyflatnum)
		{
			bothceilingssky = true;
		}

		// likewise, but for floors and upper textures
		if (gr_frontsector->floorpic == skyflatnum
			&& gr_backsector->floorpic == skyflatnum)
		{
			bothfloorssky = true;
		}

		if (!bothceilingssky)
			gr_toptexture = R_GetTextureNum(gr_sidedef->toptexture);
		if (!bothfloorssky)
			gr_bottomtexture = R_GetTextureNum(gr_sidedef->bottomtexture);

		// check TOP TEXTURE
		if ((worldhighslope < worldtopslope || worldhigh < worldtop) && gr_toptexture)
		{
			{
				fixed_t texturevpegtop; // top

				grTex = HWR_GetTexture(gr_toptexture);

				// PEGGING
				if (gr_linedef->flags & ML_DONTPEGTOP)
					texturevpegtop = 0;
				else if (gr_linedef->flags & ML_EFFECT1)
					texturevpegtop = worldhigh + textureheight[gr_sidedef->toptexture] - worldtop;
				else
					texturevpegtop = gr_backsector->ceilingheight + textureheight[gr_sidedef->toptexture] - gr_frontsector->ceilingheight;
>>>>>>> srb2/next


			// Sky Floors
			wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(INT32_MIN);

			if (gr_frontsector->floorpic == skyflatnum)
			{
				if (gr_backsector->floorpic == skyflatnum)
				{
					// Both front and back sectors are sky, needs skywall from the backsector's floor, but only if the
					// it's higher, also needs to check for bottomtexture as the floors don't usually move down
					// when both sides are sky floors
					if ((worldlow >= worldbottom && worldlowslope >= worldbottomslope)
					&& (worldlow != worldbottom || worldlowslope != worldbottomslope)
					// Removing the second line above will render more rarely visible skywalls. Example: Cave garden ceiling in Dark race
					&& !(gr_sidedef->bottomtexture))
					{
#ifdef ESLOPE
						wallVerts[3].y = FIXED_TO_FLOAT(worldlow);
						wallVerts[2].y = FIXED_TO_FLOAT(worldlowslope);
#else
						wallVerts[3].y = wallVerts[2].y = FIXED_TO_FLOAT(worldlow);
#endif

						HWR_DrawSkyWall(wallVerts, &Surf);
					}
				}
				else
				{
					// Only the backsector has sky, just draw a skywall from the back floor
#ifdef ESLOPE
					wallVerts[3].y = FIXED_TO_FLOAT(worldbottom);
					wallVerts[2].y = FIXED_TO_FLOAT(worldbottomslope);
#else
					wallVerts[3].y = wallVerts[2].y = FIXED_TO_FLOAT(worldbottom);
#endif

					HWR_DrawSkyWall(wallVerts, &Surf);
				}
			}
			else if ((gr_backsector->floorpic == skyflatnum) && !(gr_sidedef->bottomtexture))
			{
				// Only the backsector has sky, just draw a skywall from the back floor if there's no bottomtexture
#ifdef ESLOPE
				wallVerts[3].y = FIXED_TO_FLOAT(worldlow);
				wallVerts[2].y = FIXED_TO_FLOAT(worldlowslope);
#else
				wallVerts[3].y = wallVerts[2].y = FIXED_TO_FLOAT(worldlow);
#endif

				HWR_DrawSkyWall(wallVerts, &Surf);
			}

		}

		// hack to allow height changes in outdoor areas
		// This is what gets rid of the upper textures if there should be sky
		if (gr_frontsector->ceilingpic == skyflatnum &&
			gr_backsector->ceilingpic  == skyflatnum)
		{
			worldtop = worldhigh;
#ifdef ESLOPE
			worldtopslope = worldhighslope;
#endif
		}

		gr_toptexture = R_GetTextureNum(gr_sidedef->toptexture);
		gr_bottomtexture = R_GetTextureNum(gr_sidedef->bottomtexture);

		// check TOP TEXTURE
		if ((
#ifdef ESLOPE
			worldhighslope < worldtopslope ||
#endif
            worldhigh < worldtop
            ) && gr_toptexture)
		{
			{
				fixed_t texturevpegtop; // top

				grTex = HWR_GetTexture(gr_toptexture);

				// PEGGING
				if (gr_linedef->flags & ML_DONTPEGTOP)
					texturevpegtop = 0;
#ifdef ESLOPE
				else if (gr_linedef->flags & ML_EFFECT1)
					texturevpegtop = worldhigh + textureheight[gr_sidedef->toptexture] - worldtop;
				else
					texturevpegtop = gr_backsector->ceilingheight + textureheight[gr_sidedef->toptexture] - gr_frontsector->ceilingheight;
#else
                else
                    texturevpegtop = worldhigh + textureheight[gr_sidedef->toptexture] - worldtop;
#endif

				texturevpegtop += gr_sidedef->rowoffset;

				// This is so that it doesn't overflow and screw up the wall, it doesn't need to go higher than the texture's height anyway
				texturevpegtop %= SHORT(textures[gr_toptexture]->height)<<FRACBITS;

				wallVerts[3].t = wallVerts[2].t = texturevpegtop * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (texturevpegtop + gr_frontsector->ceilingheight - gr_backsector->ceilingheight) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;

				// Adjust t value for sloped walls
				if (!(gr_linedef->flags & ML_EFFECT1))
				{
					// Unskewed
					wallVerts[3].t -= (worldtop - gr_frontsector->ceilingheight) * grTex->scaleY;
					wallVerts[2].t -= (worldtopslope - gr_frontsector->ceilingheight) * grTex->scaleY;
					wallVerts[0].t -= (worldhigh - gr_backsector->ceilingheight) * grTex->scaleY;
					wallVerts[1].t -= (worldhighslope - gr_backsector->ceilingheight) * grTex->scaleY;
				}
				else if (gr_linedef->flags & ML_DONTPEGTOP)
				{
					// Skewed by top
					wallVerts[0].t = (texturevpegtop + worldtop - worldhigh) * grTex->scaleY;
					wallVerts[1].t = (texturevpegtop + worldtopslope - worldhighslope) * grTex->scaleY;
				}
				else
				{
					// Skewed by bottom
					wallVerts[0].t = wallVerts[1].t = (texturevpegtop + worldtop - worldhigh) * grTex->scaleY;
					wallVerts[3].t = wallVerts[0].t - (worldtop - worldhigh) * grTex->scaleY;
					wallVerts[2].t = wallVerts[1].t - (worldtopslope - worldhighslope) * grTex->scaleY;
				}
			}

			// set top/bottom coords
			wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = FIXED_TO_FLOAT(worldhigh);
			wallVerts[2].y = FIXED_TO_FLOAT(worldtopslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldhighslope);

			if (gr_frontsector->numlights)
				HWR_SplitWall(gr_frontsector, wallVerts, gr_toptexture, &Surf, FF_CUTLEVEL, NULL);
			else if (grTex->mipmap.flags & TF_TRANSPARENT)
				HWR_AddTransparentWall(wallVerts, &Surf, gr_toptexture, PF_Environment, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
		}

		// check BOTTOM TEXTURE
		if ((
			worldlowslope > worldbottomslope ||
            worldlow > worldbottom) && gr_bottomtexture) //only if VISIBLE!!!
		{
			{
				fixed_t texturevpegbottom = 0; // bottom

				grTex = HWR_GetTexture(gr_bottomtexture);

				// PEGGING
				if (!(gr_linedef->flags & ML_DONTPEGBOTTOM))
					texturevpegbottom = 0;
				else if (gr_linedef->flags & ML_EFFECT1)
					texturevpegbottom = worldbottom - worldlow;
				else
					texturevpegbottom = gr_frontsector->floorheight - gr_backsector->floorheight;

				texturevpegbottom += gr_sidedef->rowoffset;

				// This is so that it doesn't overflow and screw up the wall, it doesn't need to go higher than the texture's height anyway
				texturevpegbottom %= SHORT(textures[gr_bottomtexture]->height)<<FRACBITS;

				wallVerts[3].t = wallVerts[2].t = texturevpegbottom * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (texturevpegbottom + gr_backsector->floorheight - gr_frontsector->floorheight) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;

				// Adjust t value for sloped walls
				if (!(gr_linedef->flags & ML_EFFECT1))
				{
					// Unskewed
					wallVerts[0].t -= (worldbottom - gr_frontsector->floorheight) * grTex->scaleY;
					wallVerts[1].t -= (worldbottomslope - gr_frontsector->floorheight) * grTex->scaleY;
					wallVerts[3].t -= (worldlow - gr_backsector->floorheight) * grTex->scaleY;
					wallVerts[2].t -= (worldlowslope - gr_backsector->floorheight) * grTex->scaleY;
				}
				else if (gr_linedef->flags & ML_DONTPEGBOTTOM)
				{
					// Skewed by bottom
					wallVerts[0].t = wallVerts[1].t = (texturevpegbottom + worldlow - worldbottom) * grTex->scaleY;
					//wallVerts[3].t = wallVerts[0].t - (worldlow - worldbottom) * grTex->scaleY; // no need, [3] is already this
					wallVerts[2].t = wallVerts[1].t - (worldlowslope - worldbottomslope) * grTex->scaleY;
				}
				else
				{
					// Skewed by top
					wallVerts[0].t = (texturevpegbottom + worldlow - worldbottom) * grTex->scaleY;
					wallVerts[1].t = (texturevpegbottom + worldlowslope - worldbottomslope) * grTex->scaleY;
				}
			}

			// set top/bottom coords
			wallVerts[3].y = FIXED_TO_FLOAT(worldlow);
			wallVerts[0].y = FIXED_TO_FLOAT(worldbottom);
			wallVerts[2].y = FIXED_TO_FLOAT(worldlowslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldbottomslope);

			if (gr_frontsector->numlights)
				HWR_SplitWall(gr_frontsector, wallVerts, gr_bottomtexture, &Surf, FF_CUTLEVEL, NULL);
			else if (grTex->mipmap.flags & TF_TRANSPARENT)
				HWR_AddTransparentWall(wallVerts, &Surf, gr_bottomtexture, PF_Environment, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
		}
		gr_midtexture = R_GetTextureNum(gr_sidedef->midtexture);
		if (gr_midtexture)
		{
			FBITFIELD blendmode;
			sector_t *front, *back;
			fixed_t  popentop, popenbottom, polytop, polybottom, lowcut, highcut;
			fixed_t     texturevpeg = 0;
			INT32 repeats;

			if (gr_linedef->frontsector->heightsec != -1)
				front = &sectors[gr_linedef->frontsector->heightsec];
			else
				front = gr_linedef->frontsector;

			if (gr_linedef->backsector->heightsec != -1)
				back = &sectors[gr_linedef->backsector->heightsec];
			else
				back = gr_linedef->backsector;

			if (gr_sidedef->repeatcnt)
				repeats = 1 + gr_sidedef->repeatcnt;
			else if (gr_linedef->flags & ML_EFFECT5)
			{
				fixed_t high, low;

				if (front->ceilingheight > back->ceilingheight)
					high = back->ceilingheight;
				else
					high = front->ceilingheight;

				if (front->floorheight > back->floorheight)
					low = front->floorheight;
				else
					low = back->floorheight;

				repeats = (high - low)/textureheight[gr_sidedef->midtexture];
				if ((high-low)%textureheight[gr_sidedef->midtexture])
					repeats++; // tile an extra time to fill the gap -- Monster Iestyn
			}
			else
				repeats = 1;

			// SoM: a little note: This code re-arranging will
			// fix the bug in Nimrod map02. popentop and popenbottom
			// record the limits the texture can be displayed in.
			// polytop and polybottom, are the ideal (i.e. unclipped)
			// heights of the polygon, and h & l, are the final (clipped)
			// poly coords.

			// NOTE: With polyobjects, whenever you need to check the properties of the polyobject sector it belongs to,
			// you must use the linedef's backsector to be correct
			// From CB
			if (gr_curline->polyseg)
			{
				popentop = back->ceilingheight;
				popenbottom = back->floorheight;
			}
			else
            {
				popentop = min(worldtop, worldhigh);
				popenbottom = max(worldbottom, worldlow);
			}

			if (gr_linedef->flags & ML_EFFECT2)
			{
				if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
				{
					polybottom = max(front->floorheight, back->floorheight) + gr_sidedef->rowoffset;
					polytop = polybottom + textureheight[gr_midtexture]*repeats;
				}
				else
				{
					polytop = min(front->ceilingheight, back->ceilingheight) + gr_sidedef->rowoffset;
					polybottom = polytop - textureheight[gr_midtexture]*repeats;
				}
			}
			else if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
			{
				polybottom = popenbottom + gr_sidedef->rowoffset;
				polytop = polybottom + textureheight[gr_midtexture]*repeats;
			}
			else
			{
				polytop = popentop + gr_sidedef->rowoffset;
				polybottom = polytop - textureheight[gr_midtexture]*repeats;
			}
			// CB
			// NOTE: With polyobjects, whenever you need to check the properties of the polyobject sector it belongs to,
			// you must use the linedef's backsector to be correct
			if (gr_curline->polyseg)
			{
				lowcut = polybottom;
				highcut = polytop;
			}
			else
			{
				// The cut-off values of a linedef can always be constant, since every line has an absoulute front and or back sector
				lowcut = popenbottom;
				highcut = popentop;
			}

			h = min(highcut, polytop);
			l = max(polybottom, lowcut);

			{
				// PEGGING
				if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
					texturevpeg = textureheight[gr_sidedef->midtexture]*repeats - h + polybottom;
				else
					texturevpeg = polytop - h;

				grTex = HWR_GetTexture(gr_midtexture);

				wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (h - l + texturevpeg) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;
			}

			// set top/bottom coords
			// Take the texture peg into account, rather than changing the offsets past
			// where the polygon might not be.
			wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(h);
			wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(l);

			// Correct to account for slopes
			{
				fixed_t midtextureslant;

				if (gr_linedef->flags & ML_EFFECT2)
					midtextureslant = 0;
				else if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
					midtextureslant = worldlow < worldbottom
							  ? worldbottomslope-worldbottom
							  : worldlowslope-worldlow;
				else
					midtextureslant = worldtop < worldhigh
							  ? worldtopslope-worldtop
							  : worldhighslope-worldhigh;

				polytop += midtextureslant;
				polybottom += midtextureslant;

				highcut += worldtop < worldhigh
						 ? worldtopslope-worldtop
						 : worldhighslope-worldhigh;
				lowcut += worldlow < worldbottom
						? worldbottomslope-worldbottom
						: worldlowslope-worldlow;

				// Texture stuff
				h = min(highcut, polytop);
				l = max(polybottom, lowcut);

				{
					// PEGGING
					if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
						texturevpeg = textureheight[gr_sidedef->midtexture]*repeats - h + polybottom;
					else
						texturevpeg = polytop - h;
					wallVerts[2].t = texturevpeg * grTex->scaleY;
					wallVerts[1].t = (h - l + texturevpeg) * grTex->scaleY;
				}

				wallVerts[2].y = FIXED_TO_FLOAT(h);
				wallVerts[1].y = FIXED_TO_FLOAT(l);
			}

			// set alpha for transparent walls (new boom and legacy linedef types)
			// ooops ! this do not work at all because render order we should render it in backtofront order
			switch (gr_linedef->special)
			{
				case 900:
					blendmode = HWR_TranstableToAlpha(tr_trans10, &Surf);
					break;
				case 901:
					blendmode = HWR_TranstableToAlpha(tr_trans20, &Surf);
					break;
				case 902:
					blendmode = HWR_TranstableToAlpha(tr_trans30, &Surf);
					break;
				case 903:
					blendmode = HWR_TranstableToAlpha(tr_trans40, &Surf);
					break;
				case 904:
					blendmode = HWR_TranstableToAlpha(tr_trans50, &Surf);
					break;
				case 905:
					blendmode = HWR_TranstableToAlpha(tr_trans60, &Surf);
					break;
				case 906:
					blendmode = HWR_TranstableToAlpha(tr_trans70, &Surf);
					break;
				case 907:
					blendmode = HWR_TranstableToAlpha(tr_trans80, &Surf);
					break;
				case 908:
					blendmode = HWR_TranstableToAlpha(tr_trans90, &Surf);
					break;
				//  Translucent
				case 102:
				case 121:
				case 123:
				case 124:
				case 125:
				case 141:
				case 142:
				case 144:
				case 145:
				case 174:
				case 175:
				case 192:
				case 195:
				case 221:
				case 253:
				case 256:
					blendmode = PF_Translucent;
					break;
				default:
					blendmode = PF_Masked;
					break;
			}

			if (gr_curline->polyseg && gr_curline->polyseg->translucency > 0)
			{
				if (gr_curline->polyseg->translucency >= NUMTRANSMAPS) // wall not drawn
				{
					Surf.PolyColor.s.alpha = 0x00; // This shouldn't draw anything regardless of blendmode
					blendmode = PF_Masked;
				}
				else
					blendmode = HWR_TranstableToAlpha(gr_curline->polyseg->translucency, &Surf);
			}

			if (gr_frontsector->numlights)
			{
				if (!(blendmode & PF_Masked))
					HWR_SplitWall(gr_frontsector, wallVerts, gr_midtexture, &Surf, FF_TRANSLUCENT, NULL);
				else
				{
					HWR_SplitWall(gr_frontsector, wallVerts, gr_midtexture, &Surf, FF_CUTLEVEL, NULL);
				}
			}
			else if (!(blendmode & PF_Masked))
				HWR_AddTransparentWall(wallVerts, &Surf, gr_midtexture, blendmode, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, blendmode, lightnum, colormap);
<<<<<<< HEAD
=======

			// If there is a colormap change, remove it.
/*			if (!(Surf.FlatColor.s.red + Surf.FlatColor.s.green + Surf.FlatColor.s.blue == Surf.FlatColor.s.red/3)
			{
				Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
				Surf.FlatColor.rgba = 0xffffffff;
			}*/
		}

#if 1
		// Sky culling
		// No longer so much a mess as before!
		if (!gr_curline->polyseg) // Don't do it for polyobjects
		{
			if (gr_frontsector->ceilingpic == skyflatnum)
			{
				if (gr_backsector->ceilingpic != skyflatnum) // don't cull if back sector is also sky
				{
					wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(INT32_MAX); // draw to top of map space
					wallVerts[0].y = FIXED_TO_FLOAT(worldtop);
					wallVerts[1].y = FIXED_TO_FLOAT(worldtopslope);
					HWR_DrawSkyWall(wallVerts, &Surf);
				}
			}

			if (gr_frontsector->floorpic == skyflatnum)
			{
				if (gr_backsector->floorpic != skyflatnum) // don't cull if back sector is also sky
				{
					wallVerts[3].y = FIXED_TO_FLOAT(worldbottom);
					wallVerts[2].y = FIXED_TO_FLOAT(worldbottomslope);
					wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(INT32_MIN); // draw to bottom of map space
					HWR_DrawSkyWall(wallVerts, &Surf);
				}
			}
>>>>>>> srb2/next
		}
#endif
	}
	else
	{
		// Single sided line... Deal only with the middletexture (if one exists)
		gr_midtexture = R_GetTextureNum(gr_sidedef->midtexture);
<<<<<<< HEAD
		if (gr_midtexture
			&& gr_linedef->special != HORIZONSPECIAL) // Ignore horizon line for OGL
=======
		if (gr_midtexture && gr_linedef->special != HORIZONSPECIAL) // Ignore horizon line for OGL
>>>>>>> srb2/next
		{
			{
				fixed_t     texturevpeg;
				// PEGGING
				if ((gr_linedef->flags & (ML_DONTPEGBOTTOM|ML_EFFECT2)) == (ML_DONTPEGBOTTOM|ML_EFFECT2))
					texturevpeg = gr_frontsector->floorheight + textureheight[gr_sidedef->midtexture] - gr_frontsector->ceilingheight + gr_sidedef->rowoffset;
				else if (gr_linedef->flags & ML_DONTPEGBOTTOM)
					texturevpeg = worldbottom + textureheight[gr_sidedef->midtexture] - worldtop + gr_sidedef->rowoffset;
				else
					// top of texture at top
					texturevpeg = gr_sidedef->rowoffset;

				grTex = HWR_GetTexture(gr_midtexture);

				wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (texturevpeg + gr_frontsector->ceilingheight - gr_frontsector->floorheight) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;

				// Texture correction for slopes
				if (gr_linedef->flags & ML_EFFECT2) {
					wallVerts[3].t += (gr_frontsector->ceilingheight - worldtop) * grTex->scaleY;
					wallVerts[2].t += (gr_frontsector->ceilingheight - worldtopslope) * grTex->scaleY;
					wallVerts[0].t += (gr_frontsector->floorheight - worldbottom) * grTex->scaleY;
					wallVerts[1].t += (gr_frontsector->floorheight - worldbottomslope) * grTex->scaleY;
				} else if (gr_linedef->flags & ML_DONTPEGBOTTOM) {
					wallVerts[3].t = wallVerts[0].t + (worldbottom-worldtop) * grTex->scaleY;
					wallVerts[2].t = wallVerts[1].t + (worldbottomslope-worldtopslope) * grTex->scaleY;
				} else {
					wallVerts[0].t = wallVerts[3].t - (worldbottom-worldtop) * grTex->scaleY;
					wallVerts[1].t = wallVerts[2].t - (worldbottomslope-worldtopslope) * grTex->scaleY;
				}
			}

			//Set textures properly on single sided walls that are sloped
			wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = FIXED_TO_FLOAT(worldbottom);
			wallVerts[2].y = FIXED_TO_FLOAT(worldtopslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldbottomslope);
<<<<<<< HEAD
#else
			// set top/bottom coords
			wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(worldbottom);
#endif

=======

			// I don't think that solid walls can use translucent linedef types...
>>>>>>> srb2/next
			if (gr_frontsector->numlights)
				HWR_SplitWall(gr_frontsector, wallVerts, gr_midtexture, &Surf, FF_CUTLEVEL, NULL);
			// I don't think that solid walls can use translucent linedef types...
			else
			{
				if (grTex->mipmap.flags & TF_TRANSPARENT)
					HWR_AddTransparentWall(wallVerts, &Surf, gr_midtexture, PF_Environment, false, lightnum, colormap);
				else
					HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
			}
		}
		else
		{
#ifdef ESLOPE
			//Set textures properly on single sided walls that are sloped
			wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = FIXED_TO_FLOAT(worldbottom);
			wallVerts[2].y = FIXED_TO_FLOAT(worldtopslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldbottomslope);
#else
			// set top/bottom coords
			wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(worldbottom);
#endif

			// When there's no midtexture, draw a skywall to prevent rendering behind it
			HWR_DrawSkyWall(wallVerts, &Surf);
		}


		// Single sided lines are simple for skywalls, just need to draw from the top or bottom of the sector if there's
		// a sky flat
		if (!gr_curline->polyseg)
		{
			if (gr_frontsector->ceilingpic == skyflatnum) // It's a single-sided line with sky for its sector
			{
<<<<<<< HEAD
				wallVerts[3].y = wallVerts[2].y = FIXED_TO_FLOAT(INT32_MAX);
#ifdef ESLOPE
				wallVerts[0].y = FIXED_TO_FLOAT(worldtop);
				wallVerts[1].y = FIXED_TO_FLOAT(worldtopslope);
#else
				wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(worldtop);
#endif
=======
				wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(INT32_MAX); // draw to top of map space
				wallVerts[0].y = FIXED_TO_FLOAT(worldtop);
				wallVerts[1].y = FIXED_TO_FLOAT(worldtopslope);
>>>>>>> srb2/next
				HWR_DrawSkyWall(wallVerts, &Surf);
			}
			if (gr_frontsector->floorpic == skyflatnum)
			{
<<<<<<< HEAD
#ifdef ESLOPE
				wallVerts[3].y = FIXED_TO_FLOAT(worldbottom);
				wallVerts[2].y = FIXED_TO_FLOAT(worldbottomslope);
#else
				wallVerts[3].y = wallVerts[2].y = FIXED_TO_FLOAT(worldbottom);
#endif
				wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(INT32_MIN);

=======
				wallVerts[3].y = FIXED_TO_FLOAT(worldbottom);
				wallVerts[2].y = FIXED_TO_FLOAT(worldbottomslope);
				wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(INT32_MIN); // draw to bottom of map space
>>>>>>> srb2/next
				HWR_DrawSkyWall(wallVerts, &Surf);
			}
		}
	}


	//Hurdler: 3d-floors test
	if (gr_frontsector && gr_backsector && gr_frontsector->tag != gr_backsector->tag && (gr_backsector->ffloors || gr_frontsector->ffloors))
	{
		ffloor_t * rover;
		fixed_t    highcut = 0, lowcut = 0;

		INT32 texnum;
		line_t * newline = NULL; // Multi-Property FOF

        ///TODO add slope support (fixing cutoffs, proper wall clipping) - maybe just disable highcut/lowcut if either sector or FOF has a slope
        ///     to allow fun plane intersecting in OGL? But then people would abuse that and make software look bad. :C
		highcut = gr_frontsector->ceilingheight < gr_backsector->ceilingheight ? gr_frontsector->ceilingheight : gr_backsector->ceilingheight;
		lowcut = gr_frontsector->floorheight > gr_backsector->floorheight ? gr_frontsector->floorheight : gr_backsector->floorheight;

		if (gr_backsector->ffloors)
		{
			for (rover = gr_backsector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERSIDES))
					continue;
				if (!(rover->flags & FF_ALLSIDES) && rover->flags & FF_INVERTSIDES)
					continue;
				if (*rover->topheight < lowcut || *rover->bottomheight > highcut)
					continue;

				texnum = R_GetTextureNum(sides[rover->master->sidenum[0]].midtexture);

				if (rover->master->flags & ML_TFERLINE)
				{
					size_t linenum = gr_curline->linedef-gr_backsector->lines[0];
					newline = rover->master->frontsector->lines[0] + linenum;
					texnum = R_GetTextureNum(sides[newline->sidenum[0]].midtexture);
				}

				h  = P_GetFFloorTopZAt   (rover, v1x, v1y);
				hS = P_GetFFloorTopZAt   (rover, v2x, v2y);
				l  = P_GetFFloorBottomZAt(rover, v1x, v1y);
				lS = P_GetFFloorBottomZAt(rover, v2x, v2y);
				if (!(*rover->t_slope) && !gr_frontsector->c_slope && !gr_backsector->c_slope && h > highcut)
					h = hS = highcut;
				if (!(*rover->b_slope) && !gr_frontsector->f_slope && !gr_backsector->f_slope && l < lowcut)
					l = lS = lowcut;
				//Hurdler: HW code starts here
				//FIXME: check if peging is correct
				// set top/bottom coords

				wallVerts[3].y = FIXED_TO_FLOAT(h);
				wallVerts[2].y = FIXED_TO_FLOAT(hS);
				wallVerts[0].y = FIXED_TO_FLOAT(l);
				wallVerts[1].y = FIXED_TO_FLOAT(lS);
				if (rover->flags & FF_FOG)
				{
					wallVerts[3].t = wallVerts[2].t = 0;
					wallVerts[0].t = wallVerts[1].t = 0;
					wallVerts[0].s = wallVerts[3].s = 0;
					wallVerts[2].s = wallVerts[1].s = 0;
				}
				else
				{
					fixed_t texturevpeg;
					boolean attachtobottom = false;
					boolean slopeskew = false; // skew FOF walls with slopes?

					// Wow, how was this missing from OpenGL for so long?
					// ...Oh well, anyway, Lower Unpegged now changes pegging of FOFs like in software
					// -- Monster Iestyn 26/06/18
					if (newline)
					{
						texturevpeg = sides[newline->sidenum[0]].rowoffset;
						attachtobottom = !!(newline->flags & ML_DONTPEGBOTTOM);
						slopeskew = !!(newline->flags & ML_DONTPEGTOP);
					}
					else
					{
						texturevpeg = sides[rover->master->sidenum[0]].rowoffset;
						attachtobottom = !!(gr_linedef->flags & ML_DONTPEGBOTTOM);
						slopeskew = !!(rover->master->flags & ML_DONTPEGTOP);
					}

					grTex = HWR_GetTexture(texnum);

					if (!slopeskew) // no skewing
					{
						if (attachtobottom)
							texturevpeg -= *rover->topheight - *rover->bottomheight;
						wallVerts[3].t = (*rover->topheight - h + texturevpeg) * grTex->scaleY;
						wallVerts[2].t = (*rover->topheight - hS + texturevpeg) * grTex->scaleY;
						wallVerts[0].t = (*rover->topheight - l + texturevpeg) * grTex->scaleY;
						wallVerts[1].t = (*rover->topheight - lS + texturevpeg) * grTex->scaleY;
					}
					else
					{
						if (!attachtobottom) // skew by top
						{
							wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
							wallVerts[0].t = (h - l + texturevpeg) * grTex->scaleY;
							wallVerts[1].t = (hS - lS + texturevpeg) * grTex->scaleY;
						}
						else // skew by bottom
						{
							wallVerts[0].t = wallVerts[1].t = texturevpeg * grTex->scaleY;
							wallVerts[3].t = wallVerts[0].t - (h - l) * grTex->scaleY;
							wallVerts[2].t = wallVerts[1].t - (hS - lS) * grTex->scaleY;
						}
					}

					wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
					wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;
				}
				if (rover->flags & FF_FOG)
				{
					FBITFIELD blendmode;

					blendmode = PF_Fog|PF_NoTexture;

<<<<<<< HEAD
					lightnum = HWR_CalcWallLight(gr_curline, rover->master->frontsector->lightlevel);
=======
					lightnum = HWR_CalcWallLight(rover->master->frontsector->lightlevel, vs.x, vs.y, ve.x, ve.y);
>>>>>>> srb2/next
					colormap = rover->master->frontsector->extra_colormap;

					Surf.PolyColor.s.alpha = HWR_FogBlockAlpha(rover->master->frontsector->lightlevel, rover->master->frontsector->extra_colormap);

					if (gr_frontsector->numlights)
						HWR_SplitWall(gr_frontsector, wallVerts, 0, &Surf, rover->flags, rover);
					else
						HWR_AddTransparentWall(wallVerts, &Surf, 0, blendmode, true, lightnum, colormap);
				}
				else
				{
					FBITFIELD blendmode = PF_Masked;

					if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256)
					{
						blendmode = PF_Translucent;
						Surf.PolyColor.s.alpha = (UINT8)rover->alpha-1 > 255 ? 255 : rover->alpha-1;
					}

					if (gr_frontsector->numlights)
						HWR_SplitWall(gr_frontsector, wallVerts, texnum, &Surf, rover->flags, rover);
					else
					{
						if (blendmode != PF_Masked)
							HWR_AddTransparentWall(wallVerts, &Surf, texnum, blendmode, false, lightnum, colormap);
						else
							HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
					}
				}
			}
		}

		if (gr_frontsector->ffloors) // Putting this seperate should allow 2 FOF sectors to be connected without too many errors? I think?
		{
			for (rover = gr_frontsector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERSIDES))
					continue;
				if (!(rover->flags & FF_ALLSIDES || rover->flags & FF_INVERTSIDES))
					continue;
				if (*rover->topheight < lowcut || *rover->bottomheight > highcut)
					continue;

				texnum = R_GetTextureNum(sides[rover->master->sidenum[0]].midtexture);

				if (rover->master->flags & ML_TFERLINE)
				{
					size_t linenum = gr_curline->linedef-gr_backsector->lines[0];
					newline = rover->master->frontsector->lines[0] + linenum;
					texnum = R_GetTextureNum(sides[newline->sidenum[0]].midtexture);
				}
				h  = P_GetFFloorTopZAt   (rover, v1x, v1y);
				hS = P_GetFFloorTopZAt   (rover, v2x, v2y);
				l  = P_GetFFloorBottomZAt(rover, v1x, v1y);
				lS = P_GetFFloorBottomZAt(rover, v2x, v2y);
				if (!(*rover->t_slope) && !gr_frontsector->c_slope && !gr_backsector->c_slope && h > highcut)
					h = hS = highcut;
				if (!(*rover->b_slope) && !gr_frontsector->f_slope && !gr_backsector->f_slope && l < lowcut)
					l = lS = lowcut;
				//Hurdler: HW code starts here
				//FIXME: check if peging is correct
				// set top/bottom coords

				wallVerts[3].y = FIXED_TO_FLOAT(h);
				wallVerts[2].y = FIXED_TO_FLOAT(hS);
				wallVerts[0].y = FIXED_TO_FLOAT(l);
				wallVerts[1].y = FIXED_TO_FLOAT(lS);
				if (rover->flags & FF_FOG)
				{
					wallVerts[3].t = wallVerts[2].t = 0;
					wallVerts[0].t = wallVerts[1].t = 0;
					wallVerts[0].s = wallVerts[3].s = 0;
					wallVerts[2].s = wallVerts[1].s = 0;
				}
				else
				{
					grTex = HWR_GetTexture(texnum);

					if (newline)
					{
						wallVerts[3].t = wallVerts[2].t = (*rover->topheight - h + sides[newline->sidenum[0]].rowoffset) * grTex->scaleY;
						wallVerts[0].t = wallVerts[1].t = (h - l + (*rover->topheight - h + sides[newline->sidenum[0]].rowoffset)) * grTex->scaleY;
					}
					else
					{
						wallVerts[3].t = wallVerts[2].t = (*rover->topheight - h + sides[rover->master->sidenum[0]].rowoffset) * grTex->scaleY;
						wallVerts[0].t = wallVerts[1].t = (h - l + (*rover->topheight - h + sides[rover->master->sidenum[0]].rowoffset)) * grTex->scaleY;
					}

					wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
					wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;
				}

				if (rover->flags & FF_FOG)
				{
					FBITFIELD blendmode;

					blendmode = PF_Fog|PF_NoTexture;

<<<<<<< HEAD
					lightnum = HWR_CalcWallLight(gr_curline, rover->master->frontsector->lightlevel);
=======
					lightnum = HWR_CalcWallLight(rover->master->frontsector->lightlevel, vs.x, vs.y, ve.x, ve.y);
>>>>>>> srb2/next
					colormap = rover->master->frontsector->extra_colormap;

					Surf.PolyColor.s.alpha = HWR_FogBlockAlpha(rover->master->frontsector->lightlevel, rover->master->frontsector->extra_colormap);

					if (gr_backsector->numlights)
						HWR_SplitWall(gr_backsector, wallVerts, 0, &Surf, rover->flags, rover);
					else
						HWR_AddTransparentWall(wallVerts, &Surf, 0, blendmode, true, lightnum, colormap);
				}
				else
				{
					FBITFIELD blendmode = PF_Masked;

					if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256)
					{
						blendmode = PF_Translucent;
						Surf.PolyColor.s.alpha = (UINT8)rover->alpha-1 > 255 ? 255 : rover->alpha-1;
					}

					if (gr_backsector->numlights)
						HWR_SplitWall(gr_backsector, wallVerts, texnum, &Surf, rover->flags, rover);
					else
					{
						if (blendmode != PF_Masked)
							HWR_AddTransparentWall(wallVerts, &Surf, texnum, blendmode, false, lightnum, colormap);
						else
							HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
					}
				}
			}
		}
	}
//Hurdler: end of 3d-floors test
}

// From PrBoom:
//
// e6y: Check whether the player can look beyond this line, rturns true if we can't
//

boolean checkforemptylines = true;
// Don't modify anything here, just check
// Kalaron: Modified for sloped linedefs
static boolean CheckClip(sector_t * afrontsector, sector_t * abacksector)
{
	fixed_t frontf1,frontf2, frontc1, frontc2; // front floor/ceiling ends
	fixed_t backf1, backf2, backc1, backc2; // back floor ceiling ends
	boolean bothceilingssky = false, bothfloorssky = false;

	if (abacksector->ceilingpic == skyflatnum && afrontsector->ceilingpic == skyflatnum)
		bothceilingssky = true;
	if (abacksector->floorpic == skyflatnum && afrontsector->floorpic == skyflatnum)
		bothfloorssky = true;

	// GZDoom method of sloped line clipping

	if (afrontsector->f_slope || afrontsector->c_slope || abacksector->f_slope || abacksector->c_slope)
	{
		fixed_t v1x, v1y, v2x, v2y; // the seg's vertexes as fixed_t
		v1x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->x);
		v1y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->y);
		v2x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->x);
		v2y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->y);
#define SLOPEPARAMS(slope, end1, end2, normalheight) \
		end1 = P_GetZAt(slope, v1x, v1y, normalheight); \
		end2 = P_GetZAt(slope, v2x, v2y, normalheight);

		SLOPEPARAMS(afrontsector->f_slope, frontf1, frontf2, afrontsector->  floorheight)
		SLOPEPARAMS(afrontsector->c_slope, frontc1, frontc2, afrontsector->ceilingheight)
		SLOPEPARAMS( abacksector->f_slope,  backf1,  backf2,  abacksector->  floorheight)
		SLOPEPARAMS( abacksector->c_slope,  backc1,  backc2,  abacksector->ceilingheight)
#undef SLOPEPARAMS
	}
	else
	{
		frontf1 = frontf2 = afrontsector->  floorheight;
		frontc1 = frontc2 = afrontsector->ceilingheight;
<<<<<<< HEAD
		backf1 = backf2 = abacksector->floorheight;
		backc1 = backc2 = abacksector->ceilingheight;
	}


	// now check for closed sectors!

	// here we're talking about a CEILING lower than a floor. ...yeah we don't even need to bother.
	if (backc1 <= frontf1 && backc2 <= frontf2)
	{
		checkforemptylines = false;
		return true;
	}

	// here we're talking about floors higher than ceilings, don't even bother either.
	if (backf1 >= frontc1 && backf2 >= frontc2)
	{
		checkforemptylines = false;
		return true;
	}

	// Lat: Ok, here's what we need to do, we want to draw thok barriers. Let's define what a thok barrier is;
	// -Must have ceilheight <= floorheight
	// -ceilpic must be skyflatnum
	// -an adjacant sector needs to have a ceilingheight or a floor height different than the one we have, otherwise, it's just a huge ass wall, we shouldn't render past it.
	// -said adjacant sector cannot also be a thok barrier, because that's also dumb and we could render far more than we need to as a result :V

	if (backc1 <= backf1 && backc2 <= backf2)
	{
		checkforemptylines = false;

		// before we do anything, if both sectors are thok barriers, GET ME OUT OF HERE!
		if (frontc1 <= backc1 && frontc2 <= backc2)
			return true;	// STOP RENDERING.

		// draw floors at the top of thok barriers:
		if (backc1 < frontc1 || backc2 < frontc2)
			return false;

		if (backf1 > frontf1 || backf2 > frontf2)
=======
		backf1  =  backf2 =  abacksector->  floorheight;
		backc1  =  backc2 =  abacksector->ceilingheight;
	}
	// properly render skies (consider door "open" if both ceilings are sky)
	// same for floors
	if (!bothceilingssky && !bothfloorssky)
	{
		// now check for closed sectors!
		if ((backc1 <= frontf1 && backc2 <= frontf2)
			|| (backf1 >= frontc1 && backf2 >= frontc2))
		{
			checkforemptylines = false;
			return true;
		}

		if (backc1 <= backf1 && backc2 <= backf2)
		{
			// preserve a kind of transparent door/lift special effect:
			if (((backc1 >= frontc1 && backc2 >= frontc2) || seg->sidedef->toptexture)
			&& ((backf1 <= frontf1 && backf2 <= frontf2) || seg->sidedef->bottomtexture))
			{
				checkforemptylines = false;
				return true;
			}
		}
	}

	if (!bothceilingssky) {
		if (backc1 != frontc1 || backc2 != frontc2)
		{
			checkforemptylines = false;
>>>>>>> srb2/next
			return false;
		}
	}

<<<<<<< HEAD
	// Window.
	// We know it's a window when the above isn't true and the back and front sectors don't match
	if (backc1 != frontc1 || backc2 != frontc2
		|| backf1 != frontf1 || backf2 != frontf2)
	{
		checkforemptylines = false;
		return false;
=======
	if (!bothfloorssky) {
		if (backf1 != frontf1 || backf2 != frontf2)
		{
			checkforemptylines = false;
			return false;
		}
>>>>>>> srb2/next
	}

	// In this case we just need to check whether there is actually a need to render any lines, so checkforempty lines
	// stays true
	return false;
}

// HWR_AddLine
// Clips the given segment and adds any visible pieces to the line list.
void HWR_AddLine(seg_t *line)
{
	angle_t angle1, angle2;

<<<<<<< HEAD
	// SoM: Backsector needs to be run through R_FakeFlat
	static sector_t tempsec;

	fixed_t v1x, v1y, v2x, v2y; // the seg's vertexes as fixed_t
#ifdef POLYOBJECTS
	if (line->polyseg && !(line->polyseg->flags & POF_RENDERSIDES))
=======
// needs fix: walls are incorrectly clipped one column less
static consvar_t cv_grclipwalls = {"gr_clipwalls", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static void printsolidsegs(void)
{
	cliprange_t *       start;
	if (!hw_newend)
>>>>>>> srb2/next
		return;
#endif

	gr_curline = line;

	v1x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->x);
	v1y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->y);
	v2x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->x);
	v2y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->y);

	// OPTIMIZE: quickly reject orthogonal back sides.
	angle1 = R_PointToAngleEx(viewx, viewy, v1x, v1y);
	angle2 = R_PointToAngleEx(viewx, viewy, v2x, v2y);

	 // PrBoom: Back side, i.e. backface culling - read: endAngle >= startAngle!
	if (angle2 - angle1 < ANGLE_180)
		return;

	// PrBoom: use REAL clipping math YAYYYYYYY!!!
	if (!gld_clipper_SafeCheckRange(angle2, angle1))
		return;

	checkforemptylines = true;

	gr_backsector = line->backsector;

	if (!line->backsector)
		gld_clipper_SafeAddClipRange(angle2, angle1);
	else
	{
		gr_backsector = R_FakeFlat(gr_backsector, &tempsec, NULL, NULL, true);
		if (CheckClip(gr_frontsector, gr_backsector))
		{
			gld_clipper_SafeAddClipRange(angle2, angle1);
			checkforemptylines = false;
		}
		// Reject empty lines used for triggers and special events.
		// Identical floor and ceiling on both sides,
		//  identical light levels on both sides,
		//  and no middle texture.
		if (checkforemptylines && R_IsEmptyLine(line, gr_frontsector, gr_backsector))
			return;
    }

<<<<<<< HEAD
	HWR_ProcessSeg(); // Doesn't need arguments because they're defined globally :D
	return;
=======
		if (last <= next->last)
		{
			// Bottom is contained in next.
			// Adjust the clip size.
			start->last = next->last;
			goto crunch;
		}
	}

	if (first == next->first+1) // 1 line texture
	{
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(first,last);
			poorhack = true;
		}
		else
			HWR_StoreWallRange(0, 1);
	}
	else
	{
	// There is a fragment after *next.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(first,last);
			poorhack = true;
		}
		else
		{
			lowfrac  = HWR_ClipViewSegment(next->last-1, (polyvertex_t *)gr_curline->pv1, (polyvertex_t *)gr_curline->pv2);
			HWR_StoreWallRange(lowfrac, 1);
		}
	}

	// Adjust the clip size.
	start->last = last;

	// Remove start+1 to next from the clip list,
	// because start now covers their area.
crunch:
	if (next == start)
	{
		printsolidsegs();
		// Post just extended past the bottom of one post.
		return;
	}


	while (next++ != hw_newend)
	{
		// Remove a post.
		*++start = *next;
	}

	hw_newend = start;
	printsolidsegs();
}

//
//  handle LineDefs with upper and lower texture (windows)
//
static void HWR_ClipPassWallSegment(INT32 first, INT32 last)
{
	cliprange_t *start;
	float lowfrac, highfrac;
	//to allow noclipwalls but still solidseg reject of non-visible walls
	boolean poorhack = false;

	// Find the first range that touches the range
	//  (adjacent pixels are touching).
	start = gr_solidsegs;
	while (start->last < first - 1)
		start++;

	if (first < start->first)
	{
		if (last < start->first-1)
		{
			// Post is entirely visible (above start).
			HWR_StoreWallRange(0, 1);
			return;
		}

		// There is a fragment above *start.
		if (!cv_grclipwalls.value)
		{	//20/08/99: Changed by Hurdler (taken from faB's code)
			if (!poorhack) HWR_StoreWallRange(0, 1);
			poorhack = true;
		}
		else
		{
			highfrac = HWR_ClipViewSegment(min(start->first + 1,
				start->last), (polyvertex_t *)gr_curline->pv1,
				(polyvertex_t *)gr_curline->pv2);
			HWR_StoreWallRange(0, highfrac);
		}
	}

	// Bottom contained in start?
	if (last <= start->last)
		return;

	while (last >= (start+1)->first-1)
	{
		// There is a fragment between two posts.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(0, 1);
			poorhack = true;
		}
		else
		{
			lowfrac  = HWR_ClipViewSegment(max(start->last-1,start->first), (polyvertex_t *)gr_curline->pv1, (polyvertex_t *)gr_curline->pv2);
			highfrac = HWR_ClipViewSegment(min((start+1)->first+1,(start+1)->last), (polyvertex_t *)gr_curline->pv1, (polyvertex_t *)gr_curline->pv2);
			HWR_StoreWallRange(lowfrac, highfrac);
		}
		start++;

		if (last <= start->last)
			return;
	}

	if (first == start->first+1) // 1 line texture
	{
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(0, 1);
			poorhack = true;
		}
		else
			HWR_StoreWallRange(0, 1);
	}
	else
	{
		// There is a fragment after *next.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(0,1);
			poorhack = true;
		}
		else
		{
			lowfrac = HWR_ClipViewSegment(max(start->last - 1,
				start->first), (polyvertex_t *)gr_curline->pv1,
				(polyvertex_t *)gr_curline->pv2);
			HWR_StoreWallRange(lowfrac, 1);
		}
	}
}

// --------------------------------------------------------------------------
//  HWR_ClipToSolidSegs check if it is hide by wall (solidsegs)
// --------------------------------------------------------------------------
static boolean HWR_ClipToSolidSegs(INT32 first, INT32 last)
{
	cliprange_t * start;

	// Find the first range that touches the range
	//  (adjacent pixels are touching).
	start = gr_solidsegs;
	while (start->last < first-1)
		start++;

	if (first < start->first)
		return true;

	// Bottom contained in start?
	if (last <= start->last)
		return false;

	return true;
}

//
// HWR_ClearClipSegs
//
static void HWR_ClearClipSegs(void)
{
	gr_solidsegs[0].first = -0x7fffffff;
	gr_solidsegs[0].last = -1;
	gr_solidsegs[1].first = vid.width; //viewwidth;
	gr_solidsegs[1].last = 0x7fffffff;
	hw_newend = gr_solidsegs+2;
}
#endif // NEWCLIP

// -----------------+
// HWR_AddLine      : Clips the given segment and adds any visible pieces to the line list.
// Notes            : gr_cursectorlight is set to the current subsector -> sector -> light value
//                  : (it may be mixed with the wall's own flat colour in the future ...)
// -----------------+
static void HWR_AddLine(seg_t * line)
{
	angle_t angle1, angle2;
#ifndef NEWCLIP
	INT32 x1, x2;
	angle_t span, tspan;
	boolean bothceilingssky = false, bothfloorssky = false;
#endif

	// SoM: Backsector needs to be run through R_FakeFlat
	static sector_t tempsec;

	fixed_t v1x, v1y, v2x, v2y; // the seg's vertexes as fixed_t
	if (line->polyseg && !(line->polyseg->flags & POF_RENDERSIDES))
		return;

	gr_curline = line;

	v1x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->x);
	v1y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->y);
	v2x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->x);
	v2y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->y);

	// OPTIMIZE: quickly reject orthogonal back sides.
	angle1 = R_PointToAngle(v1x, v1y);
	angle2 = R_PointToAngle(v2x, v2y);

#ifdef NEWCLIP
	 // PrBoom: Back side, i.e. backface culling - read: endAngle >= startAngle!
	if (angle2 - angle1 < ANGLE_180)
		return;

	// PrBoom: use REAL clipping math YAYYYYYYY!!!

	if (!gld_clipper_SafeCheckRange(angle2, angle1))
    {
		return;
    }

	checkforemptylines = true;
#else
	// Clip to view edges.
	span = angle1 - angle2;

	// backface culling : span is < ANGLE_180 if ang1 > ang2 : the seg is facing
	if (span >= ANGLE_180)
		return;

	// Global angle needed by segcalc.
	//rw_angle1 = angle1;
	angle1 -= dup_viewangle;
	angle2 -= dup_viewangle;

	tspan = angle1 + gr_clipangle;
	if (tspan > 2*gr_clipangle)
	{
		tspan -= 2*gr_clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return;

		angle1 = gr_clipangle;
	}
	tspan = gr_clipangle - angle2;
	if (tspan > 2*gr_clipangle)
	{
		tspan -= 2*gr_clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return;

		angle2 = (angle_t)-(signed)gr_clipangle;
	}

#if 0
	{
		float fx1,fx2,fy1,fy2;
		//BP: test with a better projection than viewangletox[R_PointToAngle(angle)]
		// do not enable this at release 4 mul and 2 div
		fx1 = ((polyvertex_t *)(line->pv1))->x-gr_viewx;
		fy1 = ((polyvertex_t *)(line->pv1))->y-gr_viewy;
		fy2 = (fx1 * gr_viewcos + fy1 * gr_viewsin);
		if (fy2 < 0)
			// the point is back
			fx1 = 0;
		else
			fx1 = gr_windowcenterx + (fx1 * gr_viewsin - fy1 * gr_viewcos) * gr_centerx / fy2;

		fx2 = ((polyvertex_t *)(line->pv2))->x-gr_viewx;
		fy2 = ((polyvertex_t *)(line->pv2))->y-gr_viewy;
		fy1 = (fx2 * gr_viewcos + fy2 * gr_viewsin);
		if (fy1 < 0)
			// the point is back
			fx2 = vid.width;
		else
			fx2 = gr_windowcenterx + (fx2 * gr_viewsin - fy2 * gr_viewcos) * gr_centerx / fy1;

		x1 = fx1+0.5f;
		x2 = fx2+0.5f;
	}
#else
	// The seg is in the view range,
	// but not necessarily visible.
	angle1 = (angle1+ANGLE_90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANGLE_90)>>ANGLETOFINESHIFT;

	x1 = gr_viewangletox[angle1];
	x2 = gr_viewangletox[angle2];
#endif
	// Does not cross a pixel?
//	if (x1 == x2)
/*	{
		// BP: HERE IS THE MAIN PROBLEM !
		//CONS_Debug(DBG_RENDER, "tineline\n");
		return;
	}
*/
#endif

	gr_backsector = line->backsector;

#ifdef NEWCLIP
	if (!line->backsector)
    {
		gld_clipper_SafeAddClipRange(angle2, angle1);
    }
    else
    {
		boolean bothceilingssky = false, bothfloorssky = false;

		gr_backsector = R_FakeFlat(gr_backsector, &tempsec, NULL, NULL, true);

		if (gr_backsector->ceilingpic == skyflatnum && gr_frontsector->ceilingpic == skyflatnum)
			bothceilingssky = true;
		if (gr_backsector->floorpic == skyflatnum && gr_frontsector->floorpic == skyflatnum)
			bothfloorssky = true;

		if (bothceilingssky && bothfloorssky) // everything's sky? let's save us a bit of time then
		{
			if (!line->polyseg &&
				!line->sidedef->midtexture
				&& ((!gr_frontsector->ffloors && !gr_backsector->ffloors)
					|| (gr_frontsector->tag == gr_backsector->tag)))
				return; // line is empty, don't even bother
			// treat like wide open window instead
			HWR_ProcessSeg(); // Doesn't need arguments because they're defined globally :D
			return;
		}

		if (CheckClip(line, gr_frontsector, gr_backsector))
		{
			gld_clipper_SafeAddClipRange(angle2, angle1);
			checkforemptylines = false;
		}
		// Reject empty lines used for triggers and special events.
		// Identical floor and ceiling on both sides,
		//  identical light levels on both sides,
		//  and no middle texture.
		if (checkforemptylines && R_IsEmptyLine(line, gr_frontsector, gr_backsector))
			return;
    }

	HWR_ProcessSeg(); // Doesn't need arguments because they're defined globally :D
	return;
#else
	// Single sided line?
	if (!gr_backsector)
		goto clipsolid;

	gr_backsector = R_FakeFlat(gr_backsector, &tempsec, NULL, NULL, true);

	if (gr_backsector->ceilingpic == skyflatnum && gr_frontsector->ceilingpic == skyflatnum)
		bothceilingssky = true;
	if (gr_backsector->floorpic == skyflatnum && gr_frontsector->floorpic == skyflatnum)
		bothfloorssky = true;

	if (bothceilingssky && bothfloorssky) // everything's sky? let's save us a bit of time then
	{
		if (!line->polyseg &&
			!line->sidedef->midtexture
			&& ((!gr_frontsector->ffloors && !gr_backsector->ffloors)
				|| (gr_frontsector->tag == gr_backsector->tag)))
			return; // line is empty, don't even bother

		goto clippass; // treat like wide open window instead
	}

	if (gr_frontsector->f_slope || gr_frontsector->c_slope || gr_backsector->f_slope || gr_backsector->c_slope)
	{
		fixed_t frontf1,frontf2, frontc1, frontc2; // front floor/ceiling ends
		fixed_t backf1, backf2, backc1, backc2; // back floor ceiling ends

#define SLOPEPARAMS(slope, end1, end2, normalheight) \
		end1 = P_GetZAt(slope, v1x, v1y, normalheight); \
		end2 = P_GetZAt(slope, v2x, v2y, normalheight);

		SLOPEPARAMS(gr_frontsector->f_slope, frontf1, frontf2, gr_frontsector->  floorheight)
		SLOPEPARAMS(gr_frontsector->c_slope, frontc1, frontc2, gr_frontsector->ceilingheight)
		SLOPEPARAMS( gr_backsector->f_slope,  backf1,  backf2,  gr_backsector->  floorheight)
		SLOPEPARAMS( gr_backsector->c_slope,  backc1,  backc2,  gr_backsector->ceilingheight)
#undef SLOPEPARAMS
		// if both ceilings are skies, consider it always "open"
		// same for floors
		if (!bothceilingssky && !bothfloorssky)
		{
			// Closed door.
			if ((backc1 <= frontf1 && backc2 <= frontf2)
				|| (backf1 >= frontc1 && backf2 >= frontc2))
			{
				goto clipsolid;
			}

			// Check for automap fix.
			if (backc1 <= backf1 && backc2 <= backf2
			&& ((backc1 >= frontc1 && backc2 >= frontc2) || gr_curline->sidedef->toptexture)
			&& ((backf1 <= frontf1 && backf2 >= frontf2) || gr_curline->sidedef->bottomtexture))
				goto clipsolid;
		}

		// Window.
		if (!bothceilingssky) // ceilings are always the "same" when sky
			if (backc1 != frontc1 || backc2 != frontc2)
				goto clippass;
		if (!bothfloorssky)	// floors are always the "same" when sky
			if (backf1 != frontf1 || backf2 != frontf2)
				goto clippass;
	}
	else
	{
		// if both ceilings are skies, consider it always "open"
		// same for floors
		if (!bothceilingssky && !bothfloorssky)
		{
			// Closed door.
			if (gr_backsector->ceilingheight <= gr_frontsector->floorheight ||
				gr_backsector->floorheight >= gr_frontsector->ceilingheight)
				goto clipsolid;

			// Check for automap fix.
			if (gr_backsector->ceilingheight <= gr_backsector->floorheight
			&& ((gr_backsector->ceilingheight >= gr_frontsector->ceilingheight) || gr_curline->sidedef->toptexture)
			&& ((gr_backsector->floorheight <= gr_backsector->floorheight) || gr_curline->sidedef->bottomtexture))
				goto clipsolid;
		}

		// Window.
		if (!bothceilingssky) // ceilings are always the "same" when sky
			if (gr_backsector->ceilingheight != gr_frontsector->ceilingheight)
				goto clippass;
		if (!bothfloorssky)	// floors are always the "same" when sky
			if (gr_backsector->floorheight != gr_frontsector->floorheight)
				goto clippass;
	}

	// Reject empty lines used for triggers and special events.
	// Identical floor and ceiling on both sides,
	//  identical light levels on both sides,
	//  and no middle texture.
	if (R_IsEmptyLine(gr_curline, gr_frontsector, gr_backsector))
		return;

clippass:
	if (x1 == x2)
		{  x2++;x1 -= 2; }
	HWR_ClipPassWallSegment(x1, x2-1);
	return;

clipsolid:
	if (x1 == x2)
		goto clippass;
	HWR_ClipSolidWallSegment(x1, x2-1);
#endif
>>>>>>> srb2/next
}

// HWR_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//
// modified to use local variables

boolean HWR_CheckBBox(fixed_t *bspcoord)
{
	INT32 boxpos;
	fixed_t px1, py1, px2, py2;
	angle_t angle1, angle2;

	// Find the corners of the box
	// that define the edges from current viewpoint.
	if (viewx <= bspcoord[BOXLEFT])
		boxpos = 0;
	else if (viewx < bspcoord[BOXRIGHT])
		boxpos = 1;
	else
		boxpos = 2;

	if (viewy >= bspcoord[BOXTOP])
		boxpos |= 0;
	else if (viewy > bspcoord[BOXBOTTOM])
		boxpos |= 1<<2;
	else
		boxpos |= 2<<2;

	if (boxpos == 5)
		return true;

	px1 = bspcoord[checkcoord[boxpos][0]];
	py1 = bspcoord[checkcoord[boxpos][1]];
	px2 = bspcoord[checkcoord[boxpos][2]];
	py2 = bspcoord[checkcoord[boxpos][3]];

	angle1 = R_PointToAngleEx(viewx, viewy, px1, py1);
	angle2 = R_PointToAngleEx(viewx, viewy, px2, py2);
	return gld_clipper_SafeCheckRange(angle2, angle1);
}

//
// HWR_AddPolyObjectSegs
//
// haleyjd 02/19/06
// Adds all segs in all polyobjects in the given subsector.
// Modified for hardware rendering.
//
void HWR_AddPolyObjectSegs(void)
{
	size_t i, j;
	seg_t *gr_fakeline = Z_Calloc(sizeof(seg_t), PU_STATIC, NULL);
	polyvertex_t *pv1 = Z_Calloc(sizeof(polyvertex_t), PU_STATIC, NULL);
	polyvertex_t *pv2 = Z_Calloc(sizeof(polyvertex_t), PU_STATIC, NULL);

	// Sort through all the polyobjects
	for (i = 0; i < numpolys; ++i)
	{
		// Render the polyobject's lines
		for (j = 0; j < po_ptrs[i]->segCount; ++j)
		{
			// Copy the info of a polyobject's seg, then convert it to OpenGL floating point
			M_Memcpy(gr_fakeline, po_ptrs[i]->segs[j], sizeof(seg_t));

			// Now convert the line to float and add it to be rendered
			pv1->x = FIXED_TO_FLOAT(gr_fakeline->v1->x);
			pv1->y = FIXED_TO_FLOAT(gr_fakeline->v1->y);
			pv2->x = FIXED_TO_FLOAT(gr_fakeline->v2->x);
			pv2->y = FIXED_TO_FLOAT(gr_fakeline->v2->y);

			gr_fakeline->pv1 = pv1;
			gr_fakeline->pv2 = pv2;

			HWR_AddLine(gr_fakeline);
		}
	}

	// Free temporary data no longer needed
	Z_Free(pv2);
	Z_Free(pv1);
	Z_Free(gr_fakeline);
}

<<<<<<< HEAD
#ifdef POLYOBJECTS_PLANES
void HWR_RenderPolyObjectPlane(polyobj_t *polysector, boolean isceiling, fixed_t fixedheight, FBITFIELD blendmode, UINT8 lightlevel, lumpnum_t lumpnum, sector_t *FOFsector, UINT8 alpha, extracolormap_t *planecolormap)
=======
static void HWR_RenderPolyObjectPlane(polyobj_t *polysector, boolean isceiling, fixed_t fixedheight,
									FBITFIELD blendmode, UINT8 lightlevel, levelflat_t *levelflat, sector_t *FOFsector,
									UINT8 alpha, extracolormap_t *planecolormap)
>>>>>>> srb2/next
{
	float           height; //constant y for all points on the convex flat polygon
	FOutVector      *v3d;
	INT32             i;
	float           flatxref,flatyref;
	float fflatwidth = 64.0f, fflatheight = 64.0f;
	INT32 flatflag = 63;
	boolean texflat = false;
	size_t len;
	float scrollx = 0.0f, scrolly = 0.0f;
	angle_t angle = 0;
	FSurfaceInfo    Surf;
	fixed_t tempxsow, tempytow;
	size_t nrPlaneVerts;

	static FOutVector *planeVerts = NULL;
	static UINT16 numAllocedPlaneVerts = 0;

	nrPlaneVerts = polysector->numVertices;

	height = FIXED_TO_FLOAT(fixedheight);

	if (nrPlaneVerts < 3)   //not even a triangle ?
		return;

<<<<<<< HEAD
	if (nrPlaneVerts > INT16_MAX) // FIXME: exceeds plVerts size
=======
	if (nrPlaneVerts > (size_t)UINT16_MAX) // FIXME: exceeds plVerts size
>>>>>>> srb2/next
	{
		CONS_Debug(DBG_RENDER, "polygon size of %s exceeds max value of %d vertices\n", sizeu1(nrPlaneVerts), UINT16_MAX);
		return;
	}

	// Allocate plane-vertex buffer if we need to
	if (!planeVerts || nrPlaneVerts > numAllocedPlaneVerts)
	{
		numAllocedPlaneVerts = (UINT16)nrPlaneVerts;
		Z_Free(planeVerts);
		Z_Malloc(numAllocedPlaneVerts * sizeof (FOutVector), PU_LEVEL, &planeVerts);
	}

	// set texture for polygon
	if (levelflat != NULL)
	{
		if (levelflat->type == LEVELFLAT_TEXTURE)
		{
			fflatwidth = textures[levelflat->u.texture.num]->width;
			fflatheight = textures[levelflat->u.texture.num]->height;
			texflat = true;
		}
		else if (levelflat->type == LEVELFLAT_FLAT)
		{
			len = W_LumpLength(levelflat->u.flat.lumpnum);

			switch (len)
			{
				case 4194304: // 2048x2048 lump
					fflatwidth = fflatheight = 2048.0f;
					break;
				case 1048576: // 1024x1024 lump
					fflatwidth = fflatheight = 1024.0f;
					break;
				case 262144:// 512x512 lump
					fflatwidth = fflatheight = 512.0f;
					break;
				case 65536: // 256x256 lump
					fflatwidth = fflatheight = 256.0f;
					break;
				case 16384: // 128x128 lump
					fflatwidth = fflatheight = 128.0f;
					break;
				case 1024: // 32x32 lump
					fflatwidth = fflatheight = 32.0f;
					break;
				default: // 64x64 lump
					fflatwidth = fflatheight = 64.0f;
					break;
			}

			flatflag = ((INT32)fflatwidth)-1;
		}
	}
	else // set no texture
		HWD.pfnSetTexture(NULL);

	// reference point for flat texture coord for each vertex around the polygon
	flatxref = (float)((polysector->origVerts[0].x & (~flatflag)) / fflatwidth);
	flatyref = (float)((polysector->origVerts[0].y & (~flatflag)) / fflatheight);

	// transform
	v3d = planeVerts;

	if (FOFsector != NULL)
	{
		if (!isceiling) // it's a floor
		{
			scrollx = FIXED_TO_FLOAT(FOFsector->floor_xoffs)/fflatwidth;
			scrolly = FIXED_TO_FLOAT(FOFsector->floor_yoffs)/fflatheight;
			angle = FOFsector->floorpic_angle>>ANGLETOFINESHIFT;
		}
		else // it's a ceiling
		{
			scrollx = FIXED_TO_FLOAT(FOFsector->ceiling_xoffs)/fflatwidth;
			scrolly = FIXED_TO_FLOAT(FOFsector->ceiling_yoffs)/fflatheight;
			angle = FOFsector->ceilingpic_angle>>ANGLETOFINESHIFT;
		}
	}
	else if (gr_frontsector)
	{
		if (!isceiling) // it's a floor
		{
			scrollx = FIXED_TO_FLOAT(gr_frontsector->floor_xoffs)/fflatwidth;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->floor_yoffs)/fflatheight;
			angle = gr_frontsector->floorpic_angle>>ANGLETOFINESHIFT;
		}
		else // it's a ceiling
		{
			scrollx = FIXED_TO_FLOAT(gr_frontsector->ceiling_xoffs)/fflatwidth;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->ceiling_yoffs)/fflatheight;
			angle = gr_frontsector->ceilingpic_angle>>ANGLETOFINESHIFT;
		}
	}

	if (angle) // Only needs to be done if there's an altered angle
	{
		// This needs to be done so that it scrolls in a different direction after rotation like software
		tempxsow = FLOAT_TO_FIXED(scrollx);
		tempytow = FLOAT_TO_FIXED(scrolly);
		scrollx = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
		scrolly = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINESINE(angle)) + FixedMul(tempytow, FINECOSINE(angle))));

		// This needs to be done so everything aligns after rotation
		// It would be done so that rotation is done, THEN the translation, but I couldn't get it to rotate AND scroll like software does
		tempxsow = FLOAT_TO_FIXED(flatxref);
		tempytow = FLOAT_TO_FIXED(flatyref);
		flatxref = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
		flatyref = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINESINE(angle)) + FixedMul(tempytow, FINECOSINE(angle))));
	}

	for (i = 0; i < (INT32)nrPlaneVerts; i++,v3d++)
	{
<<<<<<< HEAD
		// Hurdler: add scrolling texture on floor/ceiling
		v3d->s = (float)((FIXED_TO_FLOAT(polysector->origVerts[i].x) / fflatsize) - flatxref + scrollx); // Go from the polysector's original vertex locations
		v3d->t = (float)(flatyref - (FIXED_TO_FLOAT(polysector->origVerts[i].y) / fflatsize) + scrolly); // Means the flat is offset based on the original vertex locations
=======
		// Go from the polysector's original vertex locations
		// Means the flat is offset based on the original vertex locations
		if (texflat)
		{
			v3d->sow = (float)(FIXED_TO_FLOAT(polysector->origVerts[i].x) / fflatwidth) + scrollx;
			v3d->tow = -(float)(FIXED_TO_FLOAT(polysector->origVerts[i].y) / fflatheight) + scrolly;
		}
		else
		{
			v3d->sow = (float)((FIXED_TO_FLOAT(polysector->origVerts[i].x) / fflatwidth) - flatxref + scrollx);
			v3d->tow = (float)(flatyref - (FIXED_TO_FLOAT(polysector->origVerts[i].y) / fflatheight) + scrolly);
		}
>>>>>>> srb2/next

		// Need to rotate before translate
		if (angle) // Only needs to be done if there's an altered angle
		{
<<<<<<< HEAD
			tempxsow = FLOAT_TO_FIXED(v3d->s);
			tempytow = FLOAT_TO_FIXED(v3d->t);
			v3d->s = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
			v3d->t = (FIXED_TO_FLOAT(-FixedMul(tempxsow, FINESINE(angle)) - FixedMul(tempytow, FINECOSINE(angle))));
=======
			tempxsow = FLOAT_TO_FIXED(v3d->sow);
			tempytow = FLOAT_TO_FIXED(v3d->tow);
			if (texflat)
				tempytow = -tempytow;
			v3d->sow = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
			v3d->tow = (FIXED_TO_FLOAT(-FixedMul(tempxsow, FINESINE(angle)) - FixedMul(tempytow, FINECOSINE(angle))));
>>>>>>> srb2/next
		}

		v3d->x = FIXED_TO_FLOAT(polysector->vertices[i]->x);
		v3d->y = height;
		v3d->z = FIXED_TO_FLOAT(polysector->vertices[i]->y);
	}

	HWR_Lighting(&Surf, lightlevel, planecolormap);

	if (blendmode & PF_Translucent)
	{
		Surf.PolyColor.s.alpha = (UINT8)alpha;
		blendmode |= PF_Modulated|PF_Occlude;
	}
	else
		blendmode |= PF_Masked|PF_Modulated;

	HWD.pfnSetShader(1);	// floor shader
	HWD.pfnDrawPolygon(&Surf, planeVerts, nrPlaneVerts, blendmode);
}

void HWR_AddPolyObjectPlanes(void)
{
	size_t i;
	sector_t *polyobjsector;
	INT32 light = 0;

	// Polyobject Planes need their own function for drawing because they don't have extrasubsectors by themselves
	// It should be okay because polyobjects should always be convex anyway

	for (i  = 0; i < numpolys; i++)
	{
		polyobjsector = po_ptrs[i]->lines[0]->backsector; // the in-level polyobject sector

		if (!(po_ptrs[i]->flags & POF_RENDERPLANES)) // Only render planes when you should
			continue;

		if (po_ptrs[i]->translucency >= NUMTRANSMAPS)
			continue;

		if (polyobjsector->floorheight <= gr_frontsector->ceilingheight
			&& polyobjsector->floorheight >= gr_frontsector->floorheight
			&& (viewz < polyobjsector->floorheight))
		{
			light = R_GetPlaneLight(gr_frontsector, polyobjsector->floorheight, true);
			if (po_ptrs[i]->translucency > 0)
			{
				FSurfaceInfo Surf;
<<<<<<< HEAD
				FBITFIELD blendmode = HWR_TranstableToAlpha(po_ptrs[i]->translucency, &Surf);
				HWR_AddTransparentPolyobjectFloor(levelflats[polyobjsector->floorpic].lumpnum, po_ptrs[i], false, polyobjsector->floorheight,
													polyobjsector->lightlevel, Surf.PolyColor.s.alpha, polyobjsector, blendmode, NULL);
			}
			else
			{
				HWR_GetFlat(levelflats[polyobjsector->floorpic].lumpnum, R_NoEncore(polyobjsector, false));
=======
				FBITFIELD blendmode;
				memset(&Surf, 0x00, sizeof(Surf));
				blendmode = HWR_TranstableToAlpha(po_ptrs[i]->translucency, &Surf);
				HWR_AddTransparentPolyobjectFloor(&levelflats[polyobjsector->floorpic], po_ptrs[i], false, polyobjsector->floorheight,
													(light == -1 ? gr_frontsector->lightlevel : *gr_frontsector->lightlist[light].lightlevel), Surf.FlatColor.s.alpha, polyobjsector, blendmode, (light == -1 ? gr_frontsector->extra_colormap : *gr_frontsector->lightlist[light].extra_colormap));
			}
			else
			{
				HWR_GetLevelFlat(&levelflats[polyobjsector->floorpic]);
>>>>>>> srb2/next
				HWR_RenderPolyObjectPlane(po_ptrs[i], false, polyobjsector->floorheight, PF_Occlude,
										(light == -1 ? gr_frontsector->lightlevel : *gr_frontsector->lightlist[light].lightlevel), &levelflats[polyobjsector->floorpic],
										polyobjsector, 255, (light == -1 ? gr_frontsector->extra_colormap : *gr_frontsector->lightlist[light].extra_colormap));
			}
		}

		if (polyobjsector->ceilingheight >= gr_frontsector->floorheight
			&& polyobjsector->ceilingheight <= gr_frontsector->ceilingheight
			&& (viewz > polyobjsector->ceilingheight))
		{
			light = R_GetPlaneLight(gr_frontsector, polyobjsector->ceilingheight, true);
			if (po_ptrs[i]->translucency > 0)
			{
				FSurfaceInfo Surf;
				FBITFIELD blendmode;
				memset(&Surf, 0x00, sizeof(Surf));
				blendmode = HWR_TranstableToAlpha(po_ptrs[i]->translucency, &Surf);
<<<<<<< HEAD
				HWR_AddTransparentPolyobjectFloor(levelflats[polyobjsector->ceilingpic].lumpnum, po_ptrs[i], true, polyobjsector->ceilingheight,
				                                  polyobjsector->lightlevel, Surf.PolyColor.s.alpha, polyobjsector, blendmode, NULL);
			}
			else
			{
				HWR_GetFlat(levelflats[polyobjsector->ceilingpic].lumpnum, R_NoEncore(polyobjsector, true));
=======
				HWR_AddTransparentPolyobjectFloor(&levelflats[polyobjsector->ceilingpic], po_ptrs[i], true, polyobjsector->ceilingheight,
				                                  (light == -1 ? gr_frontsector->lightlevel : *gr_frontsector->lightlist[light].lightlevel), Surf.FlatColor.s.alpha, polyobjsector, blendmode, (light == -1 ? gr_frontsector->extra_colormap : *gr_frontsector->lightlist[light].extra_colormap));
			}
			else
			{
				HWR_GetLevelFlat(&levelflats[polyobjsector->ceilingpic]);
>>>>>>> srb2/next
				HWR_RenderPolyObjectPlane(po_ptrs[i], true, polyobjsector->ceilingheight, PF_Occlude,
				                          (light == -1 ? gr_frontsector->lightlevel : *gr_frontsector->lightlist[light].lightlevel), &levelflats[polyobjsector->ceilingpic],
				                          polyobjsector, 255, (light == -1 ? gr_frontsector->extra_colormap : *gr_frontsector->lightlist[light].extra_colormap));
			}
		}
	}
}

// -----------------+
// HWR_Subsector    : Determine floor/ceiling planes.
//                  : Add sprites of things in sector.
//                  : Draw one or more line segments.
// -----------------+
void HWR_Subsector(size_t num)
{
	INT16 count;
	seg_t *line;
	subsector_t *sub;
	static sector_t tempsec; //SoM: 4/7/2000
	INT32 floorlightlevel;
	INT32 ceilinglightlevel;
	INT32 locFloorHeight, locCeilingHeight;
	INT32 cullFloorHeight, cullCeilingHeight;
	INT32 light = 0;
	extracolormap_t *floorcolormap;
	extracolormap_t *ceilingcolormap;

#ifdef PARANOIA //no risk while developing, enough debugging nights!
	if (num >= addsubsector)
		I_Error("HWR_Subsector: ss %s with numss = %s, addss = %s\n",
			sizeu1(num), sizeu2(numsubsectors), sizeu3(addsubsector));
#endif

	if (num < numsubsectors)
	{
		// subsector
		sub = &subsectors[num];
		// sector
		gr_frontsector = sub->sector;
		// how many linedefs
		count = sub->numlines;
		// first line seg
		line = &segs[sub->firstline];
	}
	else
	{
		// there are no segs but only planes
		sub = &subsectors[0];
		gr_frontsector = sub->sector;
		count = 0;
		line = NULL;
	}

	//SoM: 4/7/2000: Test to make Boom water work in Hardware mode.
	gr_frontsector = R_FakeFlat(gr_frontsector, &tempsec, &floorlightlevel,
								&ceilinglightlevel, false);
	//FIXME: Use floorlightlevel and ceilinglightlevel insted of lightlevel.

	floorcolormap = ceilingcolormap = gr_frontsector->extra_colormap;

// ----- for special tricks with HW renderer -----
	if (gr_frontsector->pseudoSector)
	{
		cullFloorHeight = locFloorHeight = gr_frontsector->virtualFloorheight;
		cullCeilingHeight = locCeilingHeight = gr_frontsector->virtualCeilingheight;
	}
	else if (gr_frontsector->virtualFloor)
	{
		///@TODO Is this whole virtualFloor mess even useful? I don't think it even triggers ever.
		cullFloorHeight = locFloorHeight = gr_frontsector->virtualFloorheight;
		if (gr_frontsector->virtualCeiling)
			cullCeilingHeight = locCeilingHeight = gr_frontsector->virtualCeilingheight;
		else
			cullCeilingHeight = locCeilingHeight = gr_frontsector->ceilingheight;
	}
	else if (gr_frontsector->virtualCeiling)
	{
		cullCeilingHeight = locCeilingHeight = gr_frontsector->virtualCeilingheight;
		cullFloorHeight   = locFloorHeight   = gr_frontsector->floorheight;
	}
	else
	{
		cullFloorHeight   = P_GetSectorFloorZAt  (gr_frontsector, viewx, viewy);
		cullCeilingHeight = P_GetSectorCeilingZAt(gr_frontsector, viewx, viewy);
		locFloorHeight    = P_GetSectorFloorZAt  (gr_frontsector, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);
		locCeilingHeight  = P_GetSectorCeilingZAt(gr_frontsector, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);
	}
// ----- end special tricks -----

	if (gr_frontsector->ffloors)
	{
		if (gr_frontsector->moved)
		{
			gr_frontsector->numlights = sub->sector->numlights = 0;
			R_Prep3DFloors(gr_frontsector);
			sub->sector->lightlist = gr_frontsector->lightlist;
			sub->sector->numlights = gr_frontsector->numlights;
			sub->sector->moved = gr_frontsector->moved = false;
		}

		light = R_GetPlaneLight(gr_frontsector, locFloorHeight, false);
		if (gr_frontsector->floorlightsec == -1)
			floorlightlevel = *gr_frontsector->lightlist[light].lightlevel;
		floorcolormap = *gr_frontsector->lightlist[light].extra_colormap;

		light = R_GetPlaneLight(gr_frontsector, locCeilingHeight, false);
		if (gr_frontsector->ceilinglightsec == -1)
			ceilinglightlevel = *gr_frontsector->lightlist[light].lightlevel;
		ceilingcolormap = *gr_frontsector->lightlist[light].extra_colormap;
	}

	sub->sector->extra_colormap = gr_frontsector->extra_colormap;

	//R_PlaneLightOverride(gr_frontsector, false, &floorlightlevel);
	//R_PlaneLightOverride(gr_frontsector, true, &ceilinglightlevel);

	// render floor ?
	// yeah, easy backface cull! :)
	if (cullFloorHeight < viewz)
	{
		if (gr_frontsector->floorpic != skyflatnum)
		{
			if (sub->validcount != validcount)
			{
<<<<<<< HEAD
				HWR_GetFlat(levelflats[gr_frontsector->floorpic].lumpnum, R_NoEncore(gr_frontsector, false));
				HWR_RenderPlane(&extrasubsectors[num], false,
					// Hack to make things continue to work around slopes.
					locFloorHeight == cullFloorHeight ? locFloorHeight : gr_frontsector->floorheight,
					// We now return you to your regularly scheduled rendering.
					PF_Occlude, floorlightlevel, levelflats[gr_frontsector->floorpic].lumpnum, NULL, 255, floorcolormap);
=======
				HWR_GetLevelFlat(&levelflats[gr_frontsector->floorpic]);
				HWR_RenderPlane(sub, &extrasubsectors[num], false,
					// Hack to make things continue to work around slopes.
					locFloorHeight == cullFloorHeight ? locFloorHeight : gr_frontsector->floorheight,
					// We now return you to your regularly scheduled rendering.
					PF_Occlude, floorlightlevel, &levelflats[gr_frontsector->floorpic], NULL, 255, false, floorcolormap);
>>>>>>> srb2/next
			}
		}
	}

	if (cullCeilingHeight > viewz)
	{
		if (gr_frontsector->ceilingpic != skyflatnum)
		{
			if (sub->validcount != validcount)
			{
<<<<<<< HEAD
				HWR_GetFlat(levelflats[gr_frontsector->ceilingpic].lumpnum, R_NoEncore(gr_frontsector, true));
				HWR_RenderPlane(&extrasubsectors[num], true,
					// Hack to make things continue to work around slopes.
					locCeilingHeight == cullCeilingHeight ? locCeilingHeight : gr_frontsector->ceilingheight,
					// We now return you to your regularly scheduled rendering.
					PF_Occlude, ceilinglightlevel, levelflats[gr_frontsector->ceilingpic].lumpnum,NULL, 255, ceilingcolormap);
=======
				HWR_GetLevelFlat(&levelflats[gr_frontsector->ceilingpic]);
				HWR_RenderPlane(sub, &extrasubsectors[num], true,
					// Hack to make things continue to work around slopes.
					locCeilingHeight == cullCeilingHeight ? locCeilingHeight : gr_frontsector->ceilingheight,
					// We now return you to your regularly scheduled rendering.
					PF_Occlude, ceilinglightlevel, &levelflats[gr_frontsector->ceilingpic], NULL, 255, false, ceilingcolormap);
>>>>>>> srb2/next
			}
		}
	}

	if (gr_frontsector->ffloors)
	{
		/// \todo fix light, xoffs, yoffs, extracolormap ?
		ffloor_t * rover;
		for (rover = gr_frontsector->ffloors;
			rover; rover = rover->next)
		{
			fixed_t cullHeight, centerHeight;

            // bottom plane
			cullHeight   = P_GetFFloorBottomZAt(rover, viewx, viewy);
			centerHeight = P_GetFFloorBottomZAt(rover, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);

			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERPLANES))
				continue;
			if (sub->validcount == validcount)
				continue;

			if (centerHeight <= locCeilingHeight &&
			    centerHeight >= locFloorHeight &&
<<<<<<< HEAD
			    ((viewz < cullHeight && !(rover->flags & FF_INVERTPLANES)) ||
			     (viewz > cullHeight && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
=======
			    ((dup_viewz < cullHeight && (rover->flags & FF_BOTHPLANES || !(rover->flags & FF_INVERTPLANES))) ||
			     (dup_viewz > cullHeight && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
>>>>>>> srb2/next
			{
				if (rover->flags & FF_FOG)
				{
					UINT8 alpha;

					light = R_GetPlaneLight(gr_frontsector, centerHeight, viewz < cullHeight ? true : false);

					alpha = HWR_FogBlockAlpha(*gr_frontsector->lightlist[light].lightlevel, rover->master->frontsector->extra_colormap);

					HWR_AddTransparentFloor(NULL,
					                       &extrasubsectors[num],
										   false,
					                       *rover->bottomheight,
					                       *gr_frontsector->lightlist[light].lightlevel,
					                       alpha, rover->master->frontsector, PF_Fog|PF_NoTexture,
										   true, rover->master->frontsector->extra_colormap);
				}
				else if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256) // SoM: Flags are more efficient
				{
<<<<<<< HEAD
					light = R_GetPlaneLight(gr_frontsector, centerHeight, viewz < cullHeight ? true : false);
					HWR_AddTransparentFloor(levelflats[*rover->bottompic].lumpnum,
=======
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
#ifndef SORTING
					HWR_Add3DWater(&levelflats[*rover->bottompic],
					               &extrasubsectors[num],
					               *rover->bottomheight,
					               *gr_frontsector->lightlist[light].lightlevel,
					               rover->alpha-1, rover->master->frontsector);
#else
					HWR_AddTransparentFloor(&levelflats[*rover->bottompic],
>>>>>>> srb2/next
					                       &extrasubsectors[num],
										   false,
					                       *rover->bottomheight,
					                       *gr_frontsector->lightlist[light].lightlevel,
<<<<<<< HEAD
					                       rover->alpha-1 > 255 ? 255 : rover->alpha-1, rover->master->frontsector, (rover->flags & FF_RIPPLE ? PF_Ripple : 0)|PF_Translucent,
					                       false, gr_frontsector->lightlist[light].extra_colormap);
				}
				else
				{
					HWR_GetFlat(levelflats[*rover->bottompic].lumpnum, R_NoEncore(gr_frontsector, false));
					light = R_GetPlaneLight(gr_frontsector, centerHeight, viewz < cullHeight ? true : false);
					HWR_RenderPlane(&extrasubsectors[num], false, *rover->bottomheight, (rover->flags & FF_RIPPLE ? PF_Ripple : 0)|PF_Occlude, *gr_frontsector->lightlist[light].lightlevel, levelflats[*rover->bottompic].lumpnum,
					                rover->master->frontsector, 255, gr_frontsector->lightlist[light].extra_colormap);
=======
					                       rover->alpha-1 > 255 ? 255 : rover->alpha-1, rover->master->frontsector, PF_Translucent,
					                       false, *gr_frontsector->lightlist[light].extra_colormap);
#endif
				}
				else
				{
					HWR_GetLevelFlat(&levelflats[*rover->bottompic]);
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
					HWR_RenderPlane(sub, &extrasubsectors[num], false, *rover->bottomheight, PF_Occlude, *gr_frontsector->lightlist[light].lightlevel, &levelflats[*rover->bottompic],
					                rover->master->frontsector, 255, false, *gr_frontsector->lightlist[light].extra_colormap);
>>>>>>> srb2/next
				}
			}

			// top plane
			cullHeight   = P_GetFFloorTopZAt(rover, viewx, viewy);
			centerHeight = P_GetFFloorTopZAt(rover, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);

			if (centerHeight >= locFloorHeight &&
			    centerHeight <= locCeilingHeight &&
<<<<<<< HEAD
			    ((viewz > cullHeight && !(rover->flags & FF_INVERTPLANES)) ||
			     (viewz < cullHeight && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
=======
			    ((dup_viewz > cullHeight && (rover->flags & FF_BOTHPLANES || !(rover->flags & FF_INVERTPLANES))) ||
			     (dup_viewz < cullHeight && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
>>>>>>> srb2/next
			{
				if (rover->flags & FF_FOG)
				{
					UINT8 alpha;

					light = R_GetPlaneLight(gr_frontsector, centerHeight, viewz < cullHeight ? true : false);

					alpha = HWR_FogBlockAlpha(*gr_frontsector->lightlist[light].lightlevel, rover->master->frontsector->extra_colormap);

					HWR_AddTransparentFloor(NULL,
					                       &extrasubsectors[num],
										   true,
					                       *rover->topheight,
					                       *gr_frontsector->lightlist[light].lightlevel,
					                       alpha, rover->master->frontsector, PF_Fog|PF_NoTexture,
										   true, rover->master->frontsector->extra_colormap);
				}
				else if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256)
				{
<<<<<<< HEAD
					light = R_GetPlaneLight(gr_frontsector, centerHeight, viewz < cullHeight ? true : false);
					HWR_AddTransparentFloor(levelflats[*rover->toppic].lumpnum,
=======
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
#ifndef SORTING
					HWR_Add3DWater(&levelflats[*rover->toppic],
					                          &extrasubsectors[num],
					                          *rover->topheight,
					                          *gr_frontsector->lightlist[light].lightlevel,
					                          rover->alpha-1, rover->master->frontsector);
#else
					HWR_AddTransparentFloor(&levelflats[*rover->toppic],
>>>>>>> srb2/next
					                        &extrasubsectors[num],
											true,
					                        *rover->topheight,
					                        *gr_frontsector->lightlist[light].lightlevel,
<<<<<<< HEAD
					                        rover->alpha-1 > 255 ? 255 : rover->alpha-1, rover->master->frontsector, (rover->flags & FF_RIPPLE ? PF_Ripple : 0)|PF_Translucent,
					                        false, gr_frontsector->lightlist[light].extra_colormap);
				}
				else
				{
					HWR_GetFlat(levelflats[*rover->toppic].lumpnum, R_NoEncore(gr_frontsector, true));
					light = R_GetPlaneLight(gr_frontsector, centerHeight, viewz < cullHeight ? true : false);
					HWR_RenderPlane(&extrasubsectors[num], true, *rover->topheight, (rover->flags & FF_RIPPLE ? PF_Ripple : 0)|PF_Occlude, *gr_frontsector->lightlist[light].lightlevel, levelflats[*rover->toppic].lumpnum,
					                  rover->master->frontsector, 255, gr_frontsector->lightlist[light].extra_colormap);
=======
					                        rover->alpha-1 > 255 ? 255 : rover->alpha-1, rover->master->frontsector, PF_Translucent,
					                        false, *gr_frontsector->lightlist[light].extra_colormap);
#endif

				}
				else
				{
					HWR_GetLevelFlat(&levelflats[*rover->toppic]);
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
					HWR_RenderPlane(sub, &extrasubsectors[num], true, *rover->topheight, PF_Occlude, *gr_frontsector->lightlist[light].lightlevel, &levelflats[*rover->toppic],
					                  rover->master->frontsector, 255, false, *gr_frontsector->lightlist[light].extra_colormap);
>>>>>>> srb2/next
				}
			}
		}
	}

	// Draw all the polyobjects in this subsector
	if (sub->polyList)
	{
		polyobj_t *po = sub->polyList;

		numpolys = 0;

		// Count all the polyobjects, reset the list, and recount them
		while (po)
		{
			++numpolys;
			po = (polyobj_t *)(po->link.next);
		}

		// Sort polyobjects
		R_SortPolyObjects(sub);

		// Draw polyobject lines.
		HWR_AddPolyObjectSegs();

		if (sub->validcount != validcount) // This validcount situation seems to let us know that the floors have already been drawn.
		{
			// Draw polyobject planes
			HWR_AddPolyObjectPlanes();
		}
	}

// Hurder ici se passe les choses INT32�essantes!
// on vient de tracer le sol et le plafond
// on trace �pr�ent d'abord les sprites et ensuite les murs
// hurdler: faux: on ajoute seulement les sprites, le murs sont trac� d'abord
	if (line)
	{
		// draw sprites first, coz they are clipped to the solidsegs of
		// subsectors more 'in front'
		HWR_AddSprites(gr_frontsector);

		//Hurdler: at this point validcount must be the same, but is not because
		//         gr_frontsector doesn't point anymore to sub->sector due to
		//         the call gr_frontsector = R_FakeFlat(...)
		//         if it's not done, the sprite is drawn more than once,
		//         what looks really bad with translucency or dynamic light,
		//         without talking about the overdraw of course.
		sub->sector->validcount = validcount;/// \todo fix that in a better way

		while (count--)
		{

			if (!line->glseg && !line->polyseg) // ignore segs that belong to polyobjects
				HWR_AddLine(line);
			line++;
		}
	}

	sub->validcount = validcount;
}

//
// Renders all subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.

void HWR_RenderBSPNode(INT32 bspnum)
{
	node_t *bsp = &nodes[bspnum];

	// Decide which side the view point is on
	INT32 side;

	// Found a subsector?
	if (bspnum & NF_SUBSECTOR)
	{
		if (bspnum != -1)
			HWR_Subsector(bspnum&(~NF_SUBSECTOR));
		return;
	}

	// Decide which side the view point is on.
	side = R_PointOnSide(viewx, viewy, bsp);

	// Recursively divide front space.
	HWR_RenderBSPNode(bsp->children[side]);

	// Possibly divide back space.
	if (HWR_CheckBBox(bsp->bbox[side^1]))
		HWR_RenderBSPNode(bsp->children[side^1]);
}

// ==========================================================================
// gr_things.c
// ==========================================================================

// sprites are drawn after all wall and planes are rendered, so that
// sprite translucency effects apply on the rendered view (instead of the background sky!!)

static UINT32 gr_visspritecount;
static gr_vissprite_t *gr_visspritechunks[MAXVISSPRITES >> VISSPRITECHUNKBITS] = {NULL};

// --------------------------------------------------------------------------
// HWR_ClearSprites
// Called at frame start.
// --------------------------------------------------------------------------
static void HWR_ClearSprites(void)
{
	gr_visspritecount = 0;
}

// --------------------------------------------------------------------------
// HWR_NewVisSprite
// --------------------------------------------------------------------------
static gr_vissprite_t gr_overflowsprite;

static gr_vissprite_t *HWR_GetVisSprite(UINT32 num)
{
		UINT32 chunk = num >> VISSPRITECHUNKBITS;

		// Allocate chunk if necessary
		if (!gr_visspritechunks[chunk])
			Z_Malloc(sizeof(gr_vissprite_t) * VISSPRITESPERCHUNK, PU_LEVEL, &gr_visspritechunks[chunk]);

		return gr_visspritechunks[chunk] + (num & VISSPRITEINDEXMASK);
}

static gr_vissprite_t *HWR_NewVisSprite(void)
{
	if (gr_visspritecount == MAXVISSPRITES)
		return &gr_overflowsprite;

	return HWR_GetVisSprite(gr_visspritecount++);
}

//
// HWR_DoCulling
// Hardware version of R_DoCulling
// (see r_main.c)
static boolean HWR_DoCulling(line_t *cullheight, line_t *viewcullheight, float vz, float bottomh, float toph)
{
	float cullplane;

	if (!cullheight)
		return false;

	cullplane = FIXED_TO_FLOAT(cullheight->frontsector->floorheight);
	if (cullheight->flags & ML_NOCLIMB) // Group culling
	{
		if (!viewcullheight)
			return false;

		// Make sure this is part of the same group
		if (viewcullheight->frontsector == cullheight->frontsector)
		{
			// OK, we can cull
			if (vz > cullplane && toph < cullplane) // Cull if below plane
				return true;

			if (bottomh > cullplane && vz <= cullplane) // Cull if above plane
				return true;
		}
	}
	else // Quick culling
	{
		if (vz > cullplane && toph < cullplane) // Cull if below plane
			return true;

		if (bottomh > cullplane && vz <= cullplane) // Cull if above plane
			return true;
	}

	return false;
}

<<<<<<< HEAD
static void HWR_DrawDropShadow(mobj_t *thing, fixed_t scale)
{
	const fixed_t thingxpos = thing->x + thing->sprxoff;
	const fixed_t thingypos = thing->y + thing->spryoff;
	const fixed_t thingzpos = thing->z + thing->sprzoff;

	GLPatch_t *gpatch;
	FOutVector shadowVerts[4];
	FSurfaceInfo sSurf;
	float fscale; float fx; float fy; float offset;
	UINT8 lightlevel = 0;
	extracolormap_t *colormap = NULL;
	UINT8 i;

	INT32 light;
	fixed_t scalemul;
	UINT16 alpha;
	fixed_t floordiff;
	fixed_t floorz;
	fixed_t slopez;
	pslope_t *floorslope;

	floorz = R_GetShadowZ(thing, &floorslope);
	floordiff = abs(thingzpos - floorz);
=======
static void HWR_DrawDropShadow(mobj_t *thing, gr_vissprite_t *spr, fixed_t scale)
{
	GLPatch_t *gpatch;
	FOutVector shadowVerts[4];
	FSurfaceInfo sSurf;
	float fscale; float fx; float fy; float offset;
	UINT8 lightlevel = 255;
	extracolormap_t *colormap = NULL;
	UINT8 i;

	INT32 light;
	fixed_t scalemul;
	UINT16 alpha;
	fixed_t floordiff;
	fixed_t floorz;
	fixed_t slopez;
	pslope_t *floorslope;

	floorz = R_GetShadowZ(thing, &floorslope);

	//if (abs(floorz - gr_viewz) / tz > 4) return; // Prevent stretchy shadows and possible crashes

	floordiff = abs(thing->z - floorz);
>>>>>>> srb2/next

	alpha = floordiff / (4*FRACUNIT) + 75;
	if (alpha >= 255) return;
	alpha = 255 - alpha;

<<<<<<< HEAD
	if (thing->whiteshadow)
	{
		gpatch = (GLPatch_t *)W_CachePatchName("LSHADOW", PU_CACHE);
		lightlevel = 255;
	}
	else
	{
		gpatch = (GLPatch_t *)W_CachePatchName("DSHADOW", PU_CACHE);
		lightlevel = 0;
	}

=======
	gpatch = (GLPatch_t *)W_CachePatchName("DSHADOW", PU_CACHE);
>>>>>>> srb2/next
	if (!(gpatch && gpatch->mipmap->grInfo.format)) return;
	HWR_GetPatch(gpatch);

	scalemul = FixedMul(FRACUNIT - floordiff/640, scale);
<<<<<<< HEAD
	scalemul = FixedMul(scalemul, (thing->radius*2) / gpatch->height);

	fscale = FIXED_TO_FLOAT(scalemul);
	fx = FIXED_TO_FLOAT(thingxpos);
	fy = FIXED_TO_FLOAT(thingypos);
=======
	scalemul = FixedMul(scalemul, (thing->radius*2) / SHORT(gpatch->height));

	fscale = FIXED_TO_FLOAT(scalemul);
	fx = FIXED_TO_FLOAT(thing->x);
	fy = FIXED_TO_FLOAT(thing->y);
>>>>>>> srb2/next

	//  3--2
	//  | /|
	//  |/ |
	//  0--1

	if (thing && fabsf(fscale - 1.0f) > 1.0E-36f)
<<<<<<< HEAD
		offset = (gpatch->height/2) * fscale;
	else
		offset = (float)(gpatch->height/2);

	shadowVerts[2].x = shadowVerts[3].x = fx + offset;
	shadowVerts[1].x = shadowVerts[0].x = fx - offset;
	shadowVerts[1].z = shadowVerts[2].z = fy - offset;
	shadowVerts[0].z = shadowVerts[3].z = fy + offset;

	for (i = 0; i < 4; i++)
	{
		float oldx = shadowVerts[i].x;
		float oldy = shadowVerts[i].z;
		shadowVerts[i].x = fx + ((oldx - fx) * gr_viewcos) - ((oldy - fy) * gr_viewsin);
		shadowVerts[i].z = fy + ((oldx - fx) * gr_viewsin) + ((oldy - fy) * gr_viewcos);
=======
		offset = (SHORT(gpatch->height)/2) * fscale;
	else
		offset = (float)(SHORT(gpatch->height)/2);

	shadowVerts[0].x = shadowVerts[3].x = fx - offset;
	shadowVerts[2].x = shadowVerts[1].x = fx + offset;
	shadowVerts[0].z = shadowVerts[1].z = fy - offset;
	shadowVerts[3].z = shadowVerts[2].z = fy + offset;

	if (floorslope)
	{
		for (i = 0; i < 4; i++)
		{
			slopez = P_GetSlopeZAt(floorslope, FLOAT_TO_FIXED(shadowVerts[i].x), FLOAT_TO_FIXED(shadowVerts[i].z));
			shadowVerts[i].y = FIXED_TO_FLOAT(slopez) + 0.05f;
		}
	}
	else
	{
		for (i = 0; i < 4; i++)
			shadowVerts[i].y = FIXED_TO_FLOAT(floorz) + 0.05f;
	}

	if (spr->flip)
	{
		shadowVerts[0].sow = shadowVerts[3].sow = gpatch->max_s;
		shadowVerts[2].sow = shadowVerts[1].sow = 0;
	}
	else
	{
		shadowVerts[0].sow = shadowVerts[3].sow = 0;
		shadowVerts[2].sow = shadowVerts[1].sow = gpatch->max_s;
>>>>>>> srb2/next
	}

	if (floorslope)
	{
<<<<<<< HEAD
		for (i = 0; i < 4; i++)
		{
			slopez = P_GetZAt(floorslope, FLOAT_TO_FIXED(shadowVerts[i].x), FLOAT_TO_FIXED(shadowVerts[i].z));
			shadowVerts[i].y = FIXED_TO_FLOAT(slopez) + 0.05f;
		}
	}
	else
	{
		for (i = 0; i < 4; i++)
			shadowVerts[i].y = FIXED_TO_FLOAT(floorz) + 0.05f;
	}

	shadowVerts[0].s = shadowVerts[3].s = 0;
	shadowVerts[2].s = shadowVerts[1].s = gpatch->max_s;

	shadowVerts[3].t = shadowVerts[2].t = 0;
	shadowVerts[0].t = shadowVerts[1].t = gpatch->max_t;

	if (thing->subsector->sector->numlights)
	{
		light = R_GetPlaneLight(thing->subsector->sector, floorz, false); // Always use the light at the top instead of whatever I was doing before

		if (thing->subsector->sector->lightlist[light].extra_colormap)
			colormap = thing->subsector->sector->lightlist[light].extra_colormap;
	}
	else
	{
=======
		shadowVerts[3].tow = shadowVerts[2].tow = gpatch->max_t;
		shadowVerts[0].tow = shadowVerts[1].tow = 0;
	}
	else
	{
		shadowVerts[3].tow = shadowVerts[2].tow = 0;
		shadowVerts[0].tow = shadowVerts[1].tow = gpatch->max_t;
	}

	if (thing->subsector->sector->numlights)
	{
		light = R_GetPlaneLight(thing->subsector->sector, floorz, false); // Always use the light at the top instead of whatever I was doing before

		if (*thing->subsector->sector->lightlist[light].lightlevel > 255)
			lightlevel = 255;
		else
			lightlevel = *thing->subsector->sector->lightlist[light].lightlevel;

		if (*thing->subsector->sector->lightlist[light].extra_colormap)
			colormap = *thing->subsector->sector->lightlist[light].extra_colormap;
	}
	else
	{
		lightlevel = thing->subsector->sector->lightlevel;

>>>>>>> srb2/next
		if (thing->subsector->sector->extra_colormap)
			colormap = thing->subsector->sector->extra_colormap;
	}

<<<<<<< HEAD
	HWR_Lighting(&sSurf, lightlevel, colormap);
	sSurf.PolyColor.s.alpha = alpha;

	HWD.pfnSetShader(3);	// sprite shader
	HWD.pfnDrawPolygon(&sSurf, shadowVerts, 4, PF_Translucent|PF_Modulated);
=======
	if (colormap)
		sSurf.FlatColor.rgba = HWR_Lighting(lightlevel, colormap->rgba, colormap->fadergba, false, false);
	else
		sSurf.FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, false, false);

	sSurf.FlatColor.s.alpha = alpha;

	HWD.pfnDrawPolygon(&sSurf, shadowVerts, 4, PF_Translucent|PF_Modulated|PF_Clip);
>>>>>>> srb2/next
}

// This is expecting a pointer to an array containing 4 wallVerts for a sprite
static void HWR_RotateSpritePolyToAim(gr_vissprite_t *spr, FOutVector *wallVerts, const boolean precip)
{
	if (cv_grspritebillboarding.value && spr && spr->mobj && !(spr->mobj->frame & FF_PAPERSPRITE) && wallVerts)
	{
		float basey = FIXED_TO_FLOAT(spr->mobj->z);
		float lowy = wallVerts[0].y;
		if (!precip && P_MobjFlip(spr->mobj) == -1) // precip doesn't have eflags so they can't flip
		{
			basey = FIXED_TO_FLOAT(spr->mobj->z + spr->mobj->height);
		}
		// Rotate sprites to fully billboard with the camera
		// X, Y, AND Z need to be manipulated for the polys to rotate around the
		// origin, because of how the origin setting works I believe that should
		// be mobj->z or mobj->z + mobj->height
		wallVerts[2].y = wallVerts[3].y = (spr->ty - basey) * gr_viewludsin + basey;
		wallVerts[0].y = wallVerts[1].y = (lowy - basey) * gr_viewludsin + basey;
		// translate back to be around 0 before translating back
		wallVerts[3].x += ((spr->ty - basey) * gr_viewludcos) * gr_viewcos;
		wallVerts[2].x += ((spr->ty - basey) * gr_viewludcos) * gr_viewcos;

		wallVerts[0].x += ((lowy - basey) * gr_viewludcos) * gr_viewcos;
		wallVerts[1].x += ((lowy - basey) * gr_viewludcos) * gr_viewcos;

		wallVerts[3].z += ((spr->ty - basey) * gr_viewludcos) * gr_viewsin;
		wallVerts[2].z += ((spr->ty - basey) * gr_viewludcos) * gr_viewsin;

		wallVerts[0].z += ((lowy - basey) * gr_viewludcos) * gr_viewsin;
		wallVerts[1].z += ((lowy - basey) * gr_viewludcos) * gr_viewsin;
	}
}

static void HWR_SplitSprite(gr_vissprite_t *spr)
{
	float this_scale = 1.0f;
	FOutVector wallVerts[4];
	FOutVector baseWallVerts[4]; // This is what the verts should end up as
	GLPatch_t *gpatch;
	FSurfaceInfo Surf;
	const boolean hires = (spr->mobj && spr->mobj->skin && ((skin_t *)spr->mobj->skin)->flags & SF_HIRES);
	extracolormap_t *colormap;
	FUINT lightlevel;
	FBITFIELD blend = 0;
	UINT8 alpha;

	INT32 i;
	float realtop, realbot, top, bot;
	float towtop, towbot, towmult;
	float bheight;
	float realheight, heightmult;
	const sector_t *sector = spr->mobj->subsector->sector;
	const lightlist_t *list = sector->lightlist;
	float endrealtop, endrealbot, endtop, endbot;
	float endbheight;
	float endrealheight;
	fixed_t temp;
	fixed_t v1x, v1y, v2x, v2y;

	this_scale = FIXED_TO_FLOAT(spr->mobj->scale);

	if (hires)
		this_scale = this_scale * FIXED_TO_FLOAT(((skin_t *)spr->mobj->skin)->highresscale);

	gpatch = spr->gpatch; //W_CachePatchNum(spr->patchlumpnum, PU_CACHE);

	// cache the patch in the graphics card memory
	//12/12/99: Hurdler: same comment as above (for md2)
	//Hurdler: 25/04/2000: now support colormap in hardware mode
	HWR_GetMappedPatch(gpatch, spr->colormap);

	baseWallVerts[0].x = baseWallVerts[3].x = spr->x1;
	baseWallVerts[2].x = baseWallVerts[1].x = spr->x2;
	baseWallVerts[0].z = baseWallVerts[3].z = spr->z1;
	baseWallVerts[1].z = baseWallVerts[2].z = spr->z2;

	baseWallVerts[2].y = baseWallVerts[3].y = spr->ty;
	if (spr->mobj && fabsf(this_scale - 1.0f) > 1.0E-36f)
		baseWallVerts[0].y = baseWallVerts[1].y = spr->ty - gpatch->height * this_scale;
	else
		baseWallVerts[0].y = baseWallVerts[1].y = spr->ty - gpatch->height;

	v1x = FLOAT_TO_FIXED(spr->x1);
	v1y = FLOAT_TO_FIXED(spr->z1);
	v2x = FLOAT_TO_FIXED(spr->x2);
	v2y = FLOAT_TO_FIXED(spr->z2);

	if (spr->flip)
	{
		baseWallVerts[0].s = baseWallVerts[3].s = gpatch->max_s;
		baseWallVerts[2].s = baseWallVerts[1].s = 0;
	}
	else
	{
		baseWallVerts[0].s = baseWallVerts[3].s = 0;
		baseWallVerts[2].s = baseWallVerts[1].s = gpatch->max_s;
	}

	// flip the texture coords (look familiar?)
	if (spr->vflip)
	{
		baseWallVerts[3].t = baseWallVerts[2].t = gpatch->max_t;
		baseWallVerts[0].t = baseWallVerts[1].t = 0;
	}
	else
	{
		baseWallVerts[3].t = baseWallVerts[2].t = 0;
		baseWallVerts[0].t = baseWallVerts[1].t = gpatch->max_t;
	}

	// Let dispoffset work first since this adjust each vertex
	HWR_RotateSpritePolyToAim(spr, baseWallVerts, false);

	// push it toward the camera to mitigate floor-clipping sprites
	{
		float sprdist = sqrtf((spr->x1 - gr_viewx)*(spr->x1 - gr_viewx) + (spr->z1 - gr_viewy)*(spr->z1 - gr_viewy) + (spr->ty - gr_viewz)*(spr->ty - gr_viewz));
		float distfact = ((2.0f*spr->dispoffset) + 20.0f) / sprdist;
		for (i = 0; i < 4; i++)
		{
			baseWallVerts[i].x += (gr_viewx - baseWallVerts[i].x)*distfact;
			baseWallVerts[i].z += (gr_viewy - baseWallVerts[i].z)*distfact;
			baseWallVerts[i].y += (gr_viewz - baseWallVerts[i].y)*distfact;
		}
	}

	realtop = top = baseWallVerts[3].y;
	realbot = bot = baseWallVerts[0].y;
	towtop = baseWallVerts[3].t;
	towbot = baseWallVerts[0].t;
	towmult = (towbot - towtop) / (top - bot);

	endrealtop = endtop = baseWallVerts[2].y;
	endrealbot = endbot = baseWallVerts[1].y;

	// copy the contents of baseWallVerts into the drawn wallVerts array
	// baseWallVerts is used to know the final shape to easily get the vertex
	// co-ordinates
	memcpy(wallVerts, baseWallVerts, sizeof(baseWallVerts));

	if (!cv_translucency.value) // translucency disabled
	{
		Surf.PolyColor.s.alpha = 0xFF;
		blend = PF_Translucent|PF_Occlude;
	}
	else if (spr->mobj->flags2 & MF2_SHADOW)
	{
		Surf.PolyColor.s.alpha = 0x40;
		blend = PF_Translucent;
	}
	else if (spr->mobj->frame & FF_TRANSMASK)
		blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
	else
	{
		// BP: i agree that is little better in environement but it don't
		//     work properly under glide nor with fogcolor to ffffff :(
		// Hurdler: PF_Environement would be cool, but we need to fix
		//          the issue with the fog before
		Surf.PolyColor.s.alpha = 0xFF;
		blend = PF_Translucent|PF_Occlude;
	}

	alpha = Surf.PolyColor.s.alpha;

	temp = FLOAT_TO_FIXED(realtop);

#ifdef ESLOPE
	// Start with the lightlevel and colormap from the top of the sprite
<<<<<<< HEAD
	lightlevel = 255;
	colormap = list[sector->numlights - 1].extra_colormap;
=======
	lightlevel = *list[sector->numlights - 1].lightlevel;
	colormap = *list[sector->numlights - 1].extra_colormap;
	i = 0;
	temp = FLOAT_TO_FIXED(realtop);
>>>>>>> srb2/next

	if (!(spr->mobj->frame & FF_FULLBRIGHT))
	{
		lightlevel = *list[sector->numlights - 1].lightlevel;
		if (spr->mobj->frame & FF_SEMIBRIGHT)
			lightlevel = 128 + (lightlevel>>1);
	}

	for (i = 1; i < sector->numlights; i++)
	{
		fixed_t h = P_GetLightZAt(&sector->lightlist[i], spr->mobj->x, spr->mobj->y);
		if (h <= temp)
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
<<<<<<< HEAD
			{
				lightlevel = *list[i-1].lightlevel;
				if (spr->mobj->frame & FF_SEMIBRIGHT)
					lightlevel = 128 + (lightlevel>>1);
			}
			colormap = list[i-1].extra_colormap;
			break;
		}
	}
#else
	i = R_GetPlaneLight(sector, temp, false);
	if (!(spr->mobj->frame & FF_FULLBRIGHT))
	{
		lightlevel = *list[i].lightlevel;
		if (spr->mobj->frame & FF_SEMIBRIGHT)
			lightlevel = 128 + (lightlevel>>1);
	}
	colormap = list[i].extra_colormap;
#endif
=======
				lightlevel = *list[i-1].lightlevel > 255 ? 255 : *list[i-1].lightlevel;
			colormap = *list[i-1].extra_colormap;
			break;
		}
	}
>>>>>>> srb2/next

	for (i = 0; i < sector->numlights; i++)
	{
		if (endtop < endrealbot && top < realbot)
			return;

		// even if we aren't changing colormap or lightlevel, we still need to continue drawing down the sprite
		if (!(list[i].flags & FF_NOSHADE) && (list[i].flags & FF_CUTSPRITES))
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
<<<<<<< HEAD
			{
				lightlevel = *list[i].lightlevel;
				if (spr->mobj->frame & FF_SEMIBRIGHT)
					lightlevel = 128 + (lightlevel>>1);
			}
			colormap = list[i].extra_colormap;
=======
				lightlevel = *list[i].lightlevel > 255 ? 255 : *list[i].lightlevel;
			colormap = *list[i].extra_colormap;
>>>>>>> srb2/next
		}

		if (i + 1 < sector->numlights)
		{
			temp = P_GetLightZAt(&list[i+1], v1x, v1y);
			bheight = FIXED_TO_FLOAT(temp);
			temp = P_GetLightZAt(&list[i+1], v2x, v2y);
			endbheight = FIXED_TO_FLOAT(temp);
		}
		else
		{
			bheight = realbot;
			endbheight = endrealbot;
		}

		if (endbheight >= endtop && bheight >= top)
			continue;

		bot = bheight;

		if (bot < realbot)
			bot = realbot;

		endbot = endbheight;

		if (endbot < endrealbot)
			endbot = endrealbot;

<<<<<<< HEAD
#ifdef ESLOPE
		wallVerts[3].t = towtop + ((realtop - top) * towmult);
		wallVerts[2].t = towtop + ((endrealtop - endtop) * towmult);
		wallVerts[0].t = towtop + ((realtop - bot) * towmult);
		wallVerts[1].t = towtop + ((endrealtop - endbot) * towmult);
=======
		wallVerts[3].tow = towtop + ((realtop - top) * towmult);
		wallVerts[2].tow = towtop + ((endrealtop - endtop) * towmult);
		wallVerts[0].tow = towtop + ((realtop - bot) * towmult);
		wallVerts[1].tow = towtop + ((endrealtop - endbot) * towmult);
>>>>>>> srb2/next

		wallVerts[3].y = top;
		wallVerts[2].y = endtop;
		wallVerts[0].y = bot;
		wallVerts[1].y = endbot;

		// The x and y only need to be adjusted in the case that it's not a papersprite
		if (cv_grspritebillboarding.value && spr->mobj && !(spr->mobj->frame & FF_PAPERSPRITE))
		{
			// Get the x and z of the vertices so billboarding draws correctly
			realheight = realbot - realtop;
			endrealheight = endrealbot - endrealtop;
			heightmult = (realtop - top) / realheight;
			wallVerts[3].x = baseWallVerts[3].x + (baseWallVerts[3].x - baseWallVerts[0].x) * heightmult;
			wallVerts[3].z = baseWallVerts[3].z + (baseWallVerts[3].z - baseWallVerts[0].z) * heightmult;

			heightmult = (endrealtop - endtop) / endrealheight;
<<<<<<< HEAD
			wallVerts[2].x = baseWallVerts[2].x + (baseWallVerts[2].x - baseWallVerts[1].x) * heightmult;
			wallVerts[2].z = baseWallVerts[2].z + (baseWallVerts[2].z - baseWallVerts[1].z) * heightmult;

			heightmult = (realtop - bot) / realheight;
			wallVerts[0].x = baseWallVerts[3].x + (baseWallVerts[3].x - baseWallVerts[0].x) * heightmult;
			wallVerts[0].z = baseWallVerts[3].z + (baseWallVerts[3].z - baseWallVerts[0].z) * heightmult;

			heightmult = (endrealtop - endbot) / endrealheight;
			wallVerts[1].x = baseWallVerts[2].x + (baseWallVerts[2].x - baseWallVerts[1].x) * heightmult;
			wallVerts[1].z = baseWallVerts[2].z + (baseWallVerts[2].z - baseWallVerts[1].z) * heightmult;
		}
#else
		wallVerts[3].t = wallVerts[2].t = towtop + ((realtop - top) * towmult);
		wallVerts[0].t = wallVerts[1].t = towtop + ((realtop - bot) * towmult);

		wallVerts[2].y = wallVerts[3].y = top;
		wallVerts[0].y = wallVerts[1].y = bot;

		// The x and y only need to be adjusted in the case that it's not a papersprite
		if (cv_grspritebillboarding.value && spr->mobj && !(spr->mobj->frame & FF_PAPERSPRITE))
		{
			// Get the x and z of the vertices so billboarding draws correctly
			realheight = realbot - realtop;
			heightmult = (realtop - top) / realheight;
			wallVerts[3].x = baseWallVerts[3].x + (baseWallVerts[3].x - baseWallVerts[0].x) * heightmult;
			wallVerts[3].z = baseWallVerts[3].z + (baseWallVerts[3].z - baseWallVerts[0].z) * heightmult;
=======
>>>>>>> srb2/next
			wallVerts[2].x = baseWallVerts[2].x + (baseWallVerts[2].x - baseWallVerts[1].x) * heightmult;
			wallVerts[2].z = baseWallVerts[2].z + (baseWallVerts[2].z - baseWallVerts[1].z) * heightmult;

			heightmult = (realtop - bot) / realheight;
			wallVerts[0].x = baseWallVerts[3].x + (baseWallVerts[3].x - baseWallVerts[0].x) * heightmult;
			wallVerts[0].z = baseWallVerts[3].z + (baseWallVerts[3].z - baseWallVerts[0].z) * heightmult;

			heightmult = (endrealtop - endbot) / endrealheight;
			wallVerts[1].x = baseWallVerts[2].x + (baseWallVerts[2].x - baseWallVerts[1].x) * heightmult;
			wallVerts[1].z = baseWallVerts[2].z + (baseWallVerts[2].z - baseWallVerts[1].z) * heightmult;
		}

		HWR_Lighting(&Surf, lightlevel, colormap);

		Surf.PolyColor.s.alpha = alpha;

		HWD.pfnSetShader(3);	// sprite shader
		HWD.pfnDrawPolygon(&Surf, wallVerts, 4, blend|PF_Modulated);

		top = bot;
		endtop = endbot;
	}

	bot = realbot;
	endbot = endrealbot;
	if (endtop <= endrealbot && top <= realbot)
		return;

	// If we're ever down here, somehow the above loop hasn't draw all the light levels of sprite
<<<<<<< HEAD
#ifdef ESLOPE
	wallVerts[3].t = towtop + ((realtop - top) * towmult);
	wallVerts[2].t = towtop + ((endrealtop - endtop) * towmult);
	wallVerts[0].t = towtop + ((realtop - bot) * towmult);
	wallVerts[1].t = towtop + ((endrealtop - endbot) * towmult);
=======
	wallVerts[3].tow = towtop + ((realtop - top) * towmult);
	wallVerts[2].tow = towtop + ((endrealtop - endtop) * towmult);
	wallVerts[0].tow = towtop + ((realtop - bot) * towmult);
	wallVerts[1].tow = towtop + ((endrealtop - endbot) * towmult);
>>>>>>> srb2/next

	wallVerts[3].y = top;
	wallVerts[2].y = endtop;
	wallVerts[0].y = bot;
	wallVerts[1].y = endbot;
<<<<<<< HEAD
#else
	wallVerts[3].t = wallVerts[2].t = towtop + ((realtop - top) * towmult);
	wallVerts[0].t = wallVerts[1].t = towtop + ((realtop - bot) * towmult);

	wallVerts[2].y = wallVerts[3].y = top;
	wallVerts[0].y = wallVerts[1].y = bot;
#endif
=======
>>>>>>> srb2/next

	HWR_Lighting(&Surf, lightlevel, colormap);

	Surf.PolyColor.s.alpha = alpha;

	HWD.pfnSetShader(3);	// sprite shader
	HWD.pfnDrawPolygon(&Surf, wallVerts, 4, blend|PF_Modulated);
}

// -----------------+
// HWR_DrawSprite   : Draw flat sprites
//                  : (monsters, bonuses, weapons, lights, ...)
// Returns          :
// -----------------+
static void HWR_DrawSprite(gr_vissprite_t *spr)
{
	float this_scale = 1.0f;
	FOutVector wallVerts[4];
	GLPatch_t *gpatch; // sprite patch converted to hardware
	FSurfaceInfo Surf;
	const boolean hires = (spr->mobj && spr->mobj->skin && ((skin_t *)spr->mobj->skin)->flags & SF_HIRES);
	if (spr->mobj)
		this_scale = FIXED_TO_FLOAT(spr->mobj->scale);
	if (hires)
		this_scale = this_scale * FIXED_TO_FLOAT(((skin_t *)spr->mobj->skin)->highresscale);

	if (!spr->mobj)
		return;

	if (!spr->mobj->subsector)
		return;

	if (spr->mobj->subsector->sector->numlights)
	{
		HWR_SplitSprite(spr);
		return;
	}

	// cache sprite graphics
	//12/12/99: Hurdler:
	//          OK, I don't change anything for MD2 support because I want to be
	//          sure to do it the right way. So actually, we keep normal sprite
	//          in memory and we add the md2 model if it exists for that sprite

	gpatch = spr->gpatch; //W_CachePatchNum(spr->patchlumpnum, PU_CACHE);

	// create the sprite billboard
	//
	//  3--2
	//  | /|
	//  |/ |
	//  0--1

	// these were already scaled in HWR_ProjectSprite
	wallVerts[0].x = wallVerts[3].x = spr->x1;
	wallVerts[2].x = wallVerts[1].x = spr->x2;
	wallVerts[2].y = wallVerts[3].y = spr->ty;
	if (spr->mobj && fabsf(this_scale - 1.0f) > 1.0E-36f)
		wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height * this_scale;
	else
		wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height;

	// make a wall polygon (with 2 triangles), using the floor/ceiling heights,
	// and the 2d map coords of start/end vertices
	wallVerts[0].z = wallVerts[3].z = spr->z1;
	wallVerts[1].z = wallVerts[2].z = spr->z2;

	if (spr->flip)
	{
		wallVerts[0].s = wallVerts[3].s = gpatch->max_s;
		wallVerts[2].s = wallVerts[1].s = 0;
	}else{
		wallVerts[0].s = wallVerts[3].s = 0;
		wallVerts[2].s = wallVerts[1].s = gpatch->max_s;
	}

	// flip the texture coords (look familiar?)
	if (spr->vflip)
	{
		wallVerts[3].t = wallVerts[2].t = gpatch->max_t;
		wallVerts[0].t = wallVerts[1].t = 0;
	}else{
		wallVerts[3].t = wallVerts[2].t = 0;
		wallVerts[0].t = wallVerts[1].t = gpatch->max_t;
	}

	// cache the patch in the graphics card memory
	//12/12/99: Hurdler: same comment as above (for md2)
	//Hurdler: 25/04/2000: now support colormap in hardware mode
	HWR_GetMappedPatch(gpatch, spr->colormap);

	// if it has a dispoffset, push it a little towards the camera
	if (spr->dispoffset) {
		float co = -gr_viewcos*(0.05f*spr->dispoffset);
		float si = -gr_viewsin*(0.05f*spr->dispoffset);
		wallVerts[0].z = wallVerts[3].z = wallVerts[0].z+si;
		wallVerts[1].z = wallVerts[2].z = wallVerts[1].z+si;
		wallVerts[0].x = wallVerts[3].x = wallVerts[0].x+co;
		wallVerts[1].x = wallVerts[2].x = wallVerts[1].x+co;
	}

	// Let dispoffset work first since this adjust each vertex
<<<<<<< HEAD
	// ...nah
	HWR_RotateSpritePolyToAim(spr, wallVerts);
=======
	HWR_RotateSpritePolyToAim(spr, wallVerts, false);
>>>>>>> srb2/next

	// push it toward the camera to mitigate floor-clipping sprites
	{
		float sprdist = sqrtf((spr->x1 - gr_viewx)*(spr->x1 - gr_viewx) + (spr->z1 - gr_viewy)*(spr->z1 - gr_viewy) + (spr->ty - gr_viewz)*(spr->ty - gr_viewz));
		float distfact = ((2.0f*spr->dispoffset) + 20.0f) / sprdist;
		size_t i;
		for (i = 0; i < 4; i++)
		{
			wallVerts[i].x += (gr_viewx - wallVerts[i].x)*distfact;
			wallVerts[i].z += (gr_viewy - wallVerts[i].z)*distfact;
			wallVerts[i].y += (gr_viewz - wallVerts[i].y)*distfact;
		}
	}

	// This needs to be AFTER the shadows so that the regular sprites aren't drawn completely black.
	// sprite lighting by modulating the RGB components
	/// \todo coloured

	// colormap test
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = sector->extra_colormap;

		if (!(spr->mobj->frame & FF_FULLBRIGHT))
<<<<<<< HEAD
		{
			lightlevel = sector->lightlevel;
			if (spr->mobj->frame & FF_SEMIBRIGHT)
				lightlevel = 128 + (lightlevel>>1);
		}
=======
			lightlevel = sector->lightlevel > 255 ? 255 : sector->lightlevel;
>>>>>>> srb2/next

		HWR_Lighting(&Surf, lightlevel, colormap);
	}

	{
		FBITFIELD blend = 0;
		if (!cv_translucency.value) // translucency disabled
		{
			Surf.PolyColor.s.alpha = 0xFF;
			blend = PF_Translucent|PF_Occlude;
		}
		else if (spr->mobj->flags2 & MF2_SHADOW)
		{
			Surf.PolyColor.s.alpha = 0x40;
			blend = PF_Translucent;
		}
		else if (spr->mobj->frame & FF_TRANSMASK)
			blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
		else
		{
			// BP: i agree that is little better in environement but it don't
			//     work properly under glide nor with fogcolor to ffffff :(
			// Hurdler: PF_Environement would be cool, but we need to fix
			//          the issue with the fog before
			Surf.PolyColor.s.alpha = 0xFF;
			blend = PF_Translucent|PF_Occlude;
		}

		HWD.pfnSetShader(3);	// sprite shader
		HWD.pfnDrawPolygon(&Surf, wallVerts, 4, blend|PF_Modulated);
	}
}

// Sprite drawer for precipitation
static inline void HWR_DrawPrecipitationSprite(gr_vissprite_t *spr)
{
	FBITFIELD blend = 0;
	FOutVector wallVerts[4];
	GLPatch_t *gpatch; // sprite patch converted to hardware
	FSurfaceInfo Surf;

	if (!spr->mobj)
		return;

	if (!spr->mobj->subsector)
		return;

	// cache sprite graphics
	gpatch = spr->gpatch; //W_CachePatchNum(spr->patchlumpnum, PU_CACHE);

	// create the sprite billboard
	//
	//  3--2
	//  | /|
	//  |/ |
	//  0--1
	wallVerts[0].x = wallVerts[3].x = spr->x1;
	wallVerts[2].x = wallVerts[1].x = spr->x2;
	wallVerts[2].y = wallVerts[3].y = spr->ty;
	wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height;

	// make a wall polygon (with 2 triangles), using the floor/ceiling heights,
	// and the 2d map coords of start/end vertices
	wallVerts[0].z = wallVerts[3].z = spr->z1;
	wallVerts[1].z = wallVerts[2].z = spr->z2;

	// Let dispoffset work first since this adjust each vertex
	HWR_RotateSpritePolyToAim(spr, wallVerts, true);

	wallVerts[0].s = wallVerts[3].s = 0;
	wallVerts[2].s = wallVerts[1].s = gpatch->max_s;

	wallVerts[3].t = wallVerts[2].t = 0;
	wallVerts[0].t = wallVerts[1].t = gpatch->max_t;

	// cache the patch in the graphics card memory
	//12/12/99: Hurdler: same comment as above (for md2)
	//Hurdler: 25/04/2000: now support colormap in hardware mode
	HWR_GetMappedPatch(gpatch, spr->colormap);

	// colormap test
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = sector->extra_colormap;

		if (sector->numlights)
		{
			INT32 light;

			light = R_GetPlaneLight(sector, spr->mobj->z + spr->mobj->height, false); // Always use the light at the top instead of whatever I was doing before

			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *sector->lightlist[light].lightlevel > 255 ? 255 : *sector->lightlist[light].lightlevel;

			if (*sector->lightlist[light].extra_colormap)
				colormap = *sector->lightlist[light].extra_colormap;
		}
		else
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = sector->lightlevel > 255 ? 255 : sector->lightlevel;

			if (sector->extra_colormap)
				colormap = sector->extra_colormap;
		}

		if (spr->mobj->frame & FF_SEMIBRIGHT)
			lightlevel = 128 + (lightlevel>>1);

		HWR_Lighting(&Surf, lightlevel, colormap);
	}

	if (spr->mobj->flags2 & MF2_SHADOW)
	{
		Surf.PolyColor.s.alpha = 0x40;
		blend = PF_Translucent;
	}
	else if (spr->mobj->frame & FF_TRANSMASK)
		blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
	else
	{
		// BP: i agree that is little better in environement but it don't
		//     work properly under glide nor with fogcolor to ffffff :(
		// Hurdler: PF_Environement would be cool, but we need to fix
		//          the issue with the fog before
		Surf.PolyColor.s.alpha = 0xFF;
		blend = PF_Translucent|PF_Occlude;
	}

	HWD.pfnSetShader(3);	// sprite shader
	HWD.pfnDrawPolygon(&Surf, wallVerts, 4, blend|PF_Modulated);
}

// --------------------------------------------------------------------------
// Sort vissprites by distance
// --------------------------------------------------------------------------

gr_vissprite_t* gr_vsprorder[MAXVISSPRITES];

// For more correct transparency the transparent sprites would need to be
// sorted and drawn together with transparent surfaces.
static int CompareVisSprites(const void *p1, const void *p2)
{
	gr_vissprite_t* spr1 = *(gr_vissprite_t*const*)p1;
	gr_vissprite_t* spr2 = *(gr_vissprite_t*const*)p2;
	int idiff;
	float fdiff;

	// make transparent sprites last
	// "boolean to int"

	int transparency1 = (spr1->mobj->flags2 & MF2_SHADOW) || (spr1->mobj->frame & FF_TRANSMASK);
	int transparency2 = (spr2->mobj->flags2 & MF2_SHADOW) || (spr2->mobj->frame & FF_TRANSMASK);
	idiff = transparency1 - transparency2;
	if (idiff != 0) return idiff;

	fdiff = spr2->tz - spr1->tz;// this order seems correct when checking with apitrace. Back to front.
	if (fabsf(fdiff) < 1.0E-36f)
		return spr1->dispoffset - spr2->dispoffset;// smallest dispoffset first if sprites are at (almost) same location.
	else if (fdiff > 0)
		return 1;
	else
		return -1;
}


static void HWR_SortVisSprites(void)
{
	UINT32 i;
	for (i = 0; i < gr_visspritecount; i++)
	{
		gr_vsprorder[i] = HWR_GetVisSprite(i);
	}
	qsort(gr_vsprorder, gr_visspritecount, sizeof(gr_vissprite_t*), CompareVisSprites);
}

// A drawnode is something that points to a 3D floor, 3D side, or masked
// middle texture. This is used for sorting with sprites.
typedef struct
{
	FOutVector    wallVerts[4];
	FSurfaceInfo  Surf;
	INT32         texnum;
	FBITFIELD     blend;
	INT32         drawcount;
	boolean fogwall;
	INT32 lightlevel;
	extracolormap_t *wallcolormap; // Doing the lighting in HWR_RenderWall now for correct fog after sorting
} wallinfo_t;

static wallinfo_t *wallinfo = NULL;
static size_t numwalls = 0; // a list of transparent walls to be drawn

typedef struct
{
	extrasubsector_t *xsub;
	boolean isceiling;
	fixed_t fixedheight;
	INT32 lightlevel;
	levelflat_t *levelflat;
	INT32 alpha;
	sector_t *FOFSector;
	FBITFIELD blend;
	boolean fogplane;
	extracolormap_t *planecolormap;
	INT32 drawcount;
} planeinfo_t;

static size_t numplanes = 0; // a list of transparent floors to be drawn
static planeinfo_t *planeinfo = NULL;

typedef struct
{
	polyobj_t *polysector;
	boolean isceiling;
	fixed_t fixedheight;
	INT32 lightlevel;
	levelflat_t *levelflat;
	INT32 alpha;
	sector_t *FOFSector;
	FBITFIELD blend;
	extracolormap_t *planecolormap;
	INT32 drawcount;
} polyplaneinfo_t;

static size_t numpolyplanes = 0; // a list of transparent poyobject floors to be drawn
static polyplaneinfo_t *polyplaneinfo = NULL;

<<<<<<< HEAD
=======
#ifndef SORTING
size_t numfloors = 0;
#else
//Hurdler: 3D water sutffs
>>>>>>> srb2/next
typedef struct gr_drawnode_s
{
	planeinfo_t *plane;
	polyplaneinfo_t *polyplane;
	wallinfo_t *wall;
	gr_vissprite_t *sprite;
} gr_drawnode_t;

#define MAX_TRANSPARENTWALL 256
#define MAX_TRANSPARENTFLOOR 512

// This will likely turn into a copy of HWR_Add3DWater and replace it.
<<<<<<< HEAD
void HWR_AddTransparentFloor(lumpnum_t lumpnum, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, boolean fogplane, extracolormap_t *planecolormap)
=======
void HWR_AddTransparentFloor(levelflat_t *levelflat, extrasubsector_t *xsub, boolean isceiling,
	fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, boolean fogplane, extracolormap_t *planecolormap)
>>>>>>> srb2/next
{
	static size_t allocedplanes = 0;

	// Force realloc if buffer has been freed
	if (!planeinfo)
		allocedplanes = 0;

	if (allocedplanes < numplanes + 1)
	{
		allocedplanes += MAX_TRANSPARENTFLOOR;
		Z_Realloc(planeinfo, allocedplanes * sizeof (*planeinfo), PU_LEVEL, &planeinfo);
	}

	planeinfo[numplanes].isceiling = isceiling;
	planeinfo[numplanes].fixedheight = fixedheight;
	planeinfo[numplanes].lightlevel = (planecolormap && (planecolormap->flags & CMF_FOG)) ? lightlevel : 255;
	planeinfo[numplanes].levelflat = levelflat;
	planeinfo[numplanes].xsub = xsub;
	planeinfo[numplanes].alpha = alpha;
	planeinfo[numplanes].FOFSector = FOFSector;
	planeinfo[numplanes].blend = blend;
	planeinfo[numplanes].fogplane = fogplane;
	planeinfo[numplanes].planecolormap = planecolormap;
	planeinfo[numplanes].drawcount = drawcount++;

	numplanes++;
}

// Adding this for now until I can create extrasubsector info for polyobjects
// When that happens it'll just be done through HWR_AddTransparentFloor and HWR_RenderPlane
<<<<<<< HEAD
void HWR_AddTransparentPolyobjectFloor(lumpnum_t lumpnum, polyobj_t *polysector, boolean isceiling, fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, extracolormap_t *planecolormap)
=======
void HWR_AddTransparentPolyobjectFloor(levelflat_t *levelflat, polyobj_t *polysector, boolean isceiling,
	fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, extracolormap_t *planecolormap)
>>>>>>> srb2/next
{
	static size_t allocedpolyplanes = 0;

	// Force realloc if buffer has been freed
	if (!polyplaneinfo)
		allocedpolyplanes = 0;

	if (allocedpolyplanes < numpolyplanes + 1)
	{
		allocedpolyplanes += MAX_TRANSPARENTFLOOR;
		Z_Realloc(polyplaneinfo, allocedpolyplanes * sizeof (*polyplaneinfo), PU_LEVEL, &polyplaneinfo);
	}

	polyplaneinfo[numpolyplanes].isceiling = isceiling;
	polyplaneinfo[numpolyplanes].fixedheight = fixedheight;
	polyplaneinfo[numpolyplanes].lightlevel = (planecolormap && (planecolormap->flags & CMF_FOG)) ? lightlevel : 255;
	polyplaneinfo[numpolyplanes].levelflat = levelflat;
	polyplaneinfo[numpolyplanes].polysector = polysector;
	polyplaneinfo[numpolyplanes].alpha = alpha;
	polyplaneinfo[numpolyplanes].FOFSector = FOFSector;
	polyplaneinfo[numpolyplanes].blend = blend;
	polyplaneinfo[numpolyplanes].planecolormap = planecolormap;
	polyplaneinfo[numpolyplanes].drawcount = drawcount++;
	numpolyplanes++;
}

// putting sortindex and sortnode here so the comparator function can see them
gr_drawnode_t *sortnode;
size_t *sortindex;

static int CompareDrawNodes(const void *p1, const void *p2)
{
	size_t n1 = *(const size_t*)p1;
	size_t n2 = *(const size_t*)p2;
	INT32 v1 = 0;
	INT32 v2 = 0;
	INT32 diff;
	if (sortnode[n1].plane)
		v1 = sortnode[n1].plane->drawcount;
	else if (sortnode[n1].polyplane)
		v1 = sortnode[n1].polyplane->drawcount;
	else if (sortnode[n1].wall)
		v1 = sortnode[n1].wall->drawcount;
	else I_Error("n1 unknown");

	if (sortnode[n2].plane)
		v2 = sortnode[n2].plane->drawcount;
	else if (sortnode[n2].polyplane)
		v2 = sortnode[n2].polyplane->drawcount;
	else if (sortnode[n2].wall)
		v2 = sortnode[n2].wall->drawcount;
	else I_Error("n2 unknown");

	diff = v2 - v1;
	if (diff == 0) I_Error("diff is zero");
	return diff;
}

static int CompareDrawNodePlanes(const void *p1, const void *p2)
{
	size_t n1 = *(const size_t*)p1;
	size_t n2 = *(const size_t*)p2;
	if (!sortnode[n1].plane) I_Error("Uh.. This isn't a plane! (n1)");
	if (!sortnode[n2].plane) I_Error("Uh.. This isn't a plane! (n2)");
	return ABS(sortnode[n2].plane->fixedheight - viewz) - ABS(sortnode[n1].plane->fixedheight - viewz);
}

// HWR_RenderDrawNodes
// Creates, sorts and renders a list of drawnodes for the current frame.
void HWR_RenderDrawNodes(void)
{
	UINT32 i = 0, p = 0;
	size_t run_start = 0;

	// Dump EVERYTHING into a huge drawnode list. Then we'll sort it!
	// Could this be optimized into _AddTransparentWall/_AddTransparentPlane?
	// Hell yes! But sort algorithm must be modified to use a linked list.
	sortnode = Z_Calloc((sizeof(planeinfo_t)*numplanes)
									+ (sizeof(polyplaneinfo_t)*numpolyplanes)
									+ (sizeof(wallinfo_t)*numwalls)
									,PU_STATIC, NULL);
	// todo:
	// However, in reality we shouldn't be re-copying and shifting all this information
	// that is already lying around. This should all be in some sort of linked list or lists.
	sortindex = Z_Calloc(sizeof(size_t) * (numplanes + numpolyplanes + numwalls), PU_STATIC, NULL);

	for (i = 0; i < numplanes; i++, p++)
	{
		sortnode[p].plane = &planeinfo[i];
		sortindex[p] = p;
	}

	for (i = 0; i < numpolyplanes; i++, p++)
	{
		sortnode[p].polyplane = &polyplaneinfo[i];
		sortindex[p] = p;
	}

	for (i = 0; i < numwalls; i++, p++)
	{
		sortnode[p].wall = &wallinfo[i];
		sortindex[p] = p;
	}

	// p is the number of stuff to sort

	// Add the 3D floors, thicksides, and masked textures...
	// Instead of going through drawsegs, we need to iterate
	// through the lists of masked textures and
	// translucent ffloors being drawn.

	// im not sure if this sort on the next line is needed.
	// it sorts the list based on the value of the 'drawcount' member of the drawnodes.
	// im thinking the list might already be in that order, but i havent bothered to check yet.
	// anyway doing this sort does not hurt and does not take much time.
	// the while loop after this sort is important however!
	qsort(sortindex, p, sizeof(size_t), CompareDrawNodes);

	// try solving floor order here. for each consecutive run of floors in the list, sort that run.
	while (run_start < p-1)// p-1 because a 1 plane run at the end of the list does not count
	{
		// locate run start
		if (sortnode[sortindex[run_start]].plane)
		{
			// found it, now look for run end
			size_t run_end;// (inclusive)
			for (i = run_start+1; i < p; i++)// size_t and UINT32 being used mixed here... shouldnt break anything though..
			{
				if (!sortnode[sortindex[i]].plane) break;
			}
			run_end = i-1;
			if (run_end > run_start)// if there are multiple consecutive planes, not just one
			{
				// consecutive run of planes found, now sort it
				// not sure how long these runs can be in reality...
				qsort(sortindex + run_start, run_end - run_start + 1, sizeof(size_t), CompareDrawNodePlanes);
			}
			run_start = run_end + 1;// continue looking for runs coming right after this one
		}
		else
		{
			// this wasnt the run start, try next one
			run_start++;
		}
	}

	// Okay! Let's draw it all! Woo!
	HWD.pfnSetTransform(&atransform);
	HWD.pfnSetShader(0);

	for (i = 0; i < p; i++)
	{
		if (sortnode[sortindex[i]].plane)
		{
			// We aren't traversing the BSP tree, so make gr_frontsector null to avoid crashes.
			gr_frontsector = NULL;

			if (!(sortnode[sortindex[i]].plane->blend & PF_NoTexture))
<<<<<<< HEAD
				HWR_GetFlat(sortnode[sortindex[i]].plane->lumpnum, R_NoEncore(sortnode[sortindex[i]].plane->FOFSector, sortnode[sortindex[i]].plane->isceiling));
			HWR_RenderPlane(sortnode[sortindex[i]].plane->xsub, sortnode[sortindex[i]].plane->isceiling, sortnode[sortindex[i]].plane->fixedheight, sortnode[sortindex[i]].plane->blend, sortnode[sortindex[i]].plane->lightlevel,
				sortnode[sortindex[i]].plane->lumpnum, sortnode[sortindex[i]].plane->FOFSector, sortnode[sortindex[i]].plane->alpha, /*sortnode[sortindex[i]].plane->fogplane,*/ sortnode[sortindex[i]].plane->planecolormap);
=======
				HWR_GetLevelFlat(sortnode[sortindex[i]].plane->levelflat);
			HWR_RenderPlane(NULL, sortnode[sortindex[i]].plane->xsub, sortnode[sortindex[i]].plane->isceiling, sortnode[sortindex[i]].plane->fixedheight, sortnode[sortindex[i]].plane->blend, sortnode[sortindex[i]].plane->lightlevel,
				sortnode[sortindex[i]].plane->levelflat, sortnode[sortindex[i]].plane->FOFSector, sortnode[sortindex[i]].plane->alpha, sortnode[sortindex[i]].plane->fogplane, sortnode[sortindex[i]].plane->planecolormap);
>>>>>>> srb2/next
		}
		else if (sortnode[sortindex[i]].polyplane)
		{
			// We aren't traversing the BSP tree, so make gr_frontsector null to avoid crashes.
			gr_frontsector = NULL;

			if (!(sortnode[sortindex[i]].polyplane->blend & PF_NoTexture))
<<<<<<< HEAD
				HWR_GetFlat(sortnode[sortindex[i]].polyplane->lumpnum, R_NoEncore(sortnode[sortindex[i]].polyplane->FOFSector, sortnode[sortindex[i]].polyplane->isceiling));
=======
				HWR_GetLevelFlat(sortnode[sortindex[i]].polyplane->levelflat);
>>>>>>> srb2/next
			HWR_RenderPolyObjectPlane(sortnode[sortindex[i]].polyplane->polysector, sortnode[sortindex[i]].polyplane->isceiling, sortnode[sortindex[i]].polyplane->fixedheight, sortnode[sortindex[i]].polyplane->blend, sortnode[sortindex[i]].polyplane->lightlevel,
				sortnode[sortindex[i]].polyplane->levelflat, sortnode[sortindex[i]].polyplane->FOFSector, sortnode[sortindex[i]].polyplane->alpha, sortnode[sortindex[i]].polyplane->planecolormap);
		}
		else if (sortnode[sortindex[i]].wall)
		{
			if (!(sortnode[sortindex[i]].wall->blend & PF_NoTexture))
				HWR_GetTexture(sortnode[sortindex[i]].wall->texnum);
			HWR_RenderWall(sortnode[sortindex[i]].wall->wallVerts, &sortnode[sortindex[i]].wall->Surf, sortnode[sortindex[i]].wall->blend, sortnode[sortindex[i]].wall->fogwall,
				sortnode[sortindex[i]].wall->lightlevel, sortnode[sortindex[i]].wall->wallcolormap);
		}
	}

	numwalls = 0;
	numplanes = 0;
	numpolyplanes = 0;

	// No mem leaks, please.
	Z_Free(sortnode);
	Z_Free(sortindex);
}

// --------------------------------------------------------------------------
//  Draw all vissprites
// --------------------------------------------------------------------------
void HWR_DrawSprites(void)
{
	UINT32 i;
	for (i = 0; i < gr_visspritecount; i++)
	{
		gr_vissprite_t *spr = gr_vsprorder[i];

		if (spr->precip)
		{
			HWR_DrawPrecipitationSprite(spr);
		}
		else
		{
			if (spr->mobj && spr->mobj->shadowscale && cv_shadow.value)
			{
				HWR_DrawDropShadow(spr->mobj, spr->mobj->shadowscale);
			}

			if (spr->mobj && spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
			{
				// 8/1/19: Only don't display player models if no default SPR_PLAY is found.
				if (!cv_grmdls.value
				|| ((md2_playermodels[(skin_t*)spr->mobj->skin-skins].notfound
				|| md2_playermodels[(skin_t*)spr->mobj->skin-skins].scale < 0.0f)
				&& ((!cv_grfallbackplayermodel.value)
				|| md2_models[SPR_PLAY].notfound
				|| md2_models[SPR_PLAY].scale < 0.0f))
				|| spr->mobj->state == &states[S_PLAY_SIGN])
				{
					HWR_DrawSprite(spr);
				}
				else
				{
					HWR_DrawMD2(spr);
				}
			}
			else
<<<<<<< HEAD
			{
				if (!cv_grmdls.value || md2_models[spr->mobj->sprite].notfound || md2_models[spr->mobj->sprite].scale < 0.0f)
				{
					HWR_DrawSprite(spr);
				}
				else
				{
					HWR_DrawMD2(spr);
=======
#endif
			{
				if (spr->mobj && spr->mobj->shadowscale && cv_shadow.value)
				{
					HWR_DrawDropShadow(spr->mobj, spr, spr->mobj->shadowscale);
				}

				if (spr->mobj && spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
				{
					if (!cv_grmodels.value || md2_playermodels[(skin_t*)spr->mobj->skin-skins].notfound || md2_playermodels[(skin_t*)spr->mobj->skin-skins].scale < 0.0f)
						HWR_DrawSprite(spr);
					else
					{
						if (!HWR_DrawModel(spr))
							HWR_DrawSprite(spr);
					}
				}
				else
				{
					if (!cv_grmodels.value || md2_models[spr->mobj->sprite].notfound || md2_models[spr->mobj->sprite].scale < 0.0f)
						HWR_DrawSprite(spr);
					else
					{
						if (!HWR_DrawModel(spr))
							HWR_DrawSprite(spr);
					}
>>>>>>> srb2/next
				}
			}
		}
	}
}

// --------------------------------------------------------------------------
// HWR_AddSprites
// During BSP traversal, this adds sprites by sector.
// --------------------------------------------------------------------------
void HWR_AddSprites(sector_t *sec)
{
	mobj_t *thing;
	precipmobj_t *precipthing;
<<<<<<< HEAD
	fixed_t approx_dist, limit_dist;
=======
#endif
	fixed_t limit_dist, hoop_limit_dist;
>>>>>>> srb2/next

	INT32 splitflags;
	boolean split_drawsprite;	// drawing with splitscreen flags

	// BSP is traversed by subsector.
	// A sector might have been split into several
	//  subsectors during BSP building.
	// Thus we check whether its already added.
	if (sec->validcount == validcount)
		return;

	// Well, now it will be done.
	sec->validcount = validcount;

	// Handle all things in sector.
	// If a limit exists, handle things a tiny bit different.
<<<<<<< HEAD
	if ((limit_dist = (fixed_t)(cv_drawdist.value) * mapobjectscale))
	{
		for (thing = sec->thinglist; thing; thing = thing->snext)
		{

			split_drawsprite = false;

			if (thing->sprite == SPR_NULL || thing->flags2 & MF2_DONTDRAW)
				continue;

			splitflags = thing->eflags & (MFE_DRAWONLYFORP1|MFE_DRAWONLYFORP2|MFE_DRAWONLYFORP3|MFE_DRAWONLYFORP4);

			if (r_splitscreen && splitflags)
			{
				if (thing->eflags & MFE_DRAWONLYFORP1)
					if (viewssnum == 0)
						split_drawsprite = true;

				if (thing->eflags & MFE_DRAWONLYFORP2)
					if (viewssnum == 1)
						split_drawsprite = true;

				if (thing->eflags & MFE_DRAWONLYFORP3 && splitscreen > 1)
					if (viewssnum == 2)
						split_drawsprite = true;

				if (thing->eflags & MFE_DRAWONLYFORP4 && splitscreen > 2)
					if (viewssnum == 3)
						split_drawsprite = true;
			}
			else
				split_drawsprite = true;

			if (!split_drawsprite)
				continue;

			approx_dist = P_AproxDistance(viewx-thing->x, viewy-thing->y);

			if (approx_dist > limit_dist)
				continue;

			HWR_ProjectSprite(thing);
		}
	}
	else
	{
		// Draw everything in sector, no checks
		for (thing = sec->thinglist; thing; thing = thing->snext)
		{

			split_drawsprite = false;

			if (thing->sprite == SPR_NULL || thing->flags2 & MF2_DONTDRAW)
				continue;

			splitflags = thing->eflags & (MFE_DRAWONLYFORP1|MFE_DRAWONLYFORP2|MFE_DRAWONLYFORP3|MFE_DRAWONLYFORP4);

			if (r_splitscreen && splitflags)
			{
				if (thing->eflags & MFE_DRAWONLYFORP1)
					if (viewssnum == 0)
						split_drawsprite = true;

				if (thing->eflags & MFE_DRAWONLYFORP2)
					if (viewssnum == 1)
						split_drawsprite = true;

				if (thing->eflags & MFE_DRAWONLYFORP3 && splitscreen > 1)
					if (viewssnum == 2)
						split_drawsprite = true;

				if (thing->eflags & MFE_DRAWONLYFORP4 && splitscreen > 2)
					if (viewssnum == 3)
						split_drawsprite = true;
			}
			else
				split_drawsprite = true;

			if (!split_drawsprite)
				continue;

			HWR_ProjectSprite(thing);
		}
	}

	// No to infinite precipitation draw distance.
	if ((limit_dist = (fixed_t)(cv_drawdist_precip.value) * mapobjectscale))
	{
		for (precipthing = sec->preciplist; precipthing; precipthing = precipthing->snext)
		{
			if (precipthing->precipflags & PCF_INVISIBLE)
				continue;

			approx_dist = P_AproxDistance(viewx-precipthing->x, viewy-precipthing->y);

			if (approx_dist > limit_dist)
				continue;

			HWR_ProjectPrecipitationSprite(precipthing);
		}
	}
=======
	limit_dist = (fixed_t)(cv_drawdist.value) << FRACBITS;
	hoop_limit_dist = (fixed_t)(cv_drawdist_nights.value) << FRACBITS;
	for (thing = sec->thinglist; thing; thing = thing->snext)
	{
		if (R_ThingVisibleWithinDist(thing, limit_dist, hoop_limit_dist))
			HWR_ProjectSprite(thing);
	}

#ifdef HWPRECIP
	// no, no infinite draw distance for precipitation. this option at zero is supposed to turn it off
	if ((limit_dist = (fixed_t)cv_drawdist_precip.value << FRACBITS))
	{
		for (precipthing = sec->preciplist; precipthing; precipthing = precipthing->snext)
		{
			if (R_PrecipThingVisible(precipthing, limit_dist))
				HWR_ProjectPrecipitationSprite(precipthing);
		}
	}
#endif
>>>>>>> srb2/next
}

// --------------------------------------------------------------------------
// HWR_ProjectSprite
//  Generates a vissprite for a thing if it might be visible.
// --------------------------------------------------------------------------
// BP why not use xtoviexangle/viewangletox like in bsp ?....
void HWR_ProjectSprite(mobj_t *thing)
{
	const fixed_t thingxpos = thing->x + thing->sprxoff;
	const fixed_t thingypos = thing->y + thing->spryoff;
	const fixed_t thingzpos = thing->z + thing->sprzoff;

	gr_vissprite_t *vis;
	float tr_x, tr_y;
	float tz;
	float x1, x2;
	float z1, z2;
	float rightsin, rightcos;
	float this_scale;
	float gz, gzt;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	spriteinfo_t *sprinfo;
	md2_t *md2;
	size_t lumpoff;
	unsigned rot;
<<<<<<< HEAD
	UINT8 flip;
	boolean vflip = (!(thing->eflags & MFE_VERTICALFLIP) != !(thing->frame & FF_VERTICALFLIP));
=======
	UINT16 flip;
	boolean vflip = (!(thing->eflags & MFE_VERTICALFLIP) != !(thing->frame & FF_VERTICALFLIP));

>>>>>>> srb2/next
	angle_t ang;
	const boolean papersprite = (thing->frame & FF_PAPERSPRITE);
<<<<<<< HEAD
	INT32 heightsec, phs;
=======
	angle_t mobjangle = (thing->player ? thing->player->drawangle : thing->angle);
	float z1, z2;
>>>>>>> srb2/next

	fixed_t spr_width, spr_height;
	fixed_t spr_offset, spr_topoffset;
#ifdef ROTSPRITE
	patch_t *rotsprite = NULL;
	INT32 rollangle = 0;
#endif

	if (!thing)
		return;

	this_scale = FIXED_TO_FLOAT(thing->scale);

	// transform the origin point
	tr_x = FIXED_TO_FLOAT(thingxpos) - gr_viewx;
	tr_y = FIXED_TO_FLOAT(thingypos) - gr_viewy;

	// rotation around vertical axis
	tz = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);

	// thing is behind view plane?
<<<<<<< HEAD
	if (tz < ZCLIP_PLANE && !papersprite && (!cv_grmdls.value || md2_models[thing->sprite].notfound == true)) //Yellow: Only MD2's dont disappear
		return;
=======
	if (tz < ZCLIP_PLANE && !papersprite)
	{
		if (cv_grmodels.value) //Yellow: Only MD2's dont disappear
		{
			if (thing->skin && thing->sprite == SPR_PLAY)
				md2 = &md2_playermodels[( (skin_t *)thing->skin - skins )];
			else
				md2 = &md2_models[thing->sprite];

			if (md2->notfound || md2->scale < 0.0f)
				return;
		}
		else
			return;
	}
>>>>>>> srb2/next

	// The above can stay as it works for cutting sprites that are too close
	tr_x = FIXED_TO_FLOAT(thingxpos);
	tr_y = FIXED_TO_FLOAT(thingypos);

	// decide which patch to use for sprite relative to player
#ifdef RANGECHECK
	if ((unsigned)thing->sprite >= numsprites)
		I_Error("HWR_ProjectSprite: invalid sprite number %i ", thing->sprite);
#endif

	rot = thing->frame&FF_FRAMEMASK;

	//Fab : 02-08-98: 'skin' override spritedef currently used for skin
	if (thing->skin && thing->sprite == SPR_PLAY)
	{
		sprdef = &((skin_t *)thing->skin)->sprites[thing->sprite2];
		sprinfo = &((skin_t *)thing->skin)->sprinfo[thing->sprite2];
	}
	else
	{
		sprdef = &sprites[thing->sprite];
		sprinfo = NULL;
	}

	if (rot >= sprdef->numframes)
	{
		CONS_Alert(CONS_ERROR, M_GetText("HWR_ProjectSprite: invalid sprite frame %s/%s for %s\n"),
			sizeu1(rot), sizeu2(sprdef->numframes), sprnames[thing->sprite]);
		thing->sprite = states[S_UNKNOWN].sprite;
		thing->frame = states[S_UNKNOWN].frame;
		sprdef = &sprites[thing->sprite];
		sprinfo = NULL;
		rot = thing->frame&FF_FRAMEMASK;
		thing->state->sprite = thing->sprite;
		thing->state->frame = thing->frame;
	}

	sprframe = &sprdef->spriteframes[rot];

#ifdef PARANOIA
	if (!sprframe)
		I_Error("sprframes NULL for sprite %d\n", thing->sprite);
#endif

<<<<<<< HEAD
	if (thing->player)
		ang = R_PointToAngle (thingxpos, thingypos) - thing->player->frameangle;
	else
		ang = R_PointToAngle (thingxpos, thingypos) - thing->angle;
=======
	ang = R_PointToAngle (thing->x, thing->y) - mobjangle;
>>>>>>> srb2/next

	if (sprframe->rotate == SRF_SINGLE)
	{
		// use single rotation for all views
		rot = 0;                        //Fab: for vis->patch below
		lumpoff = sprframe->lumpid[0];     //Fab: see note above
		flip = sprframe->flip; // Will only be 0x00 or 0xFF

		if (papersprite && ang < ANGLE_180)
			flip ^= 0xFFFF;
	}
	else
	{
		// choose a different rotation based on player view
		if ((sprframe->rotate & SRF_RIGHT) && (ang < ANGLE_180)) // See from right
			rot = 6; // F7 slot
		else if ((sprframe->rotate & SRF_LEFT) && (ang >= ANGLE_180)) // See from left
			rot = 2; // F3 slot
		else if (sprframe->rotate & SRF_3DGE) // 16-angle mode
		{
			rot = (ang+ANGLE_180+ANGLE_11hh)>>28;
			rot = ((rot & 1)<<3)|(rot>>1);
		}
		else // Normal behaviour
			rot = (ang+ANGLE_202h)>>29;

		//Fab: lumpid is the index for spritewidth,spriteoffset... tables
		lumpoff = sprframe->lumpid[rot];
		flip = sprframe->flip & (1<<rot);

		if (papersprite && ang < ANGLE_180)
			flip ^= (1<<rot);
	}

	if (thing->skin && ((skin_t *)thing->skin)->flags & SF_HIRES)
		this_scale = this_scale * FIXED_TO_FLOAT(((skin_t *)thing->skin)->highresscale);

	spr_width = spritecachedinfo[lumpoff].width;
	spr_height = spritecachedinfo[lumpoff].height;
	spr_offset = spritecachedinfo[lumpoff].offset;
	spr_topoffset = spritecachedinfo[lumpoff].topoffset;

#ifdef ROTSPRITE
	if (thing->rollangle)
	{
		rollangle = R_GetRollAngle(thing->rollangle);
		if (!(sprframe->rotsprite.cached & (1<<rot)))
			R_CacheRotSprite(thing->sprite, (thing->frame & FF_FRAMEMASK), sprinfo, sprframe, rot, flip);
		rotsprite = sprframe->rotsprite.patch[rot][rollangle];
		if (rotsprite != NULL)
		{
			spr_width = SHORT(rotsprite->width) << FRACBITS;
			spr_height = SHORT(rotsprite->height) << FRACBITS;
			spr_offset = SHORT(rotsprite->leftoffset) << FRACBITS;
			spr_topoffset = SHORT(rotsprite->topoffset) << FRACBITS;
			// flip -> rotate, not rotate -> flip
			flip = 0;
		}
	}
#endif

	if (papersprite)
	{
		rightsin = FIXED_TO_FLOAT(FINESINE((mobjangle)>>ANGLETOFINESHIFT));
		rightcos = FIXED_TO_FLOAT(FINECOSINE((mobjangle)>>ANGLETOFINESHIFT));
	}
	else
	{
		rightsin = FIXED_TO_FLOAT(FINESINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
		rightcos = FIXED_TO_FLOAT(FINECOSINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	}

	if (flip)
	{
		x1 = (FIXED_TO_FLOAT(spr_width - spr_offset) * this_scale);
		x2 = (FIXED_TO_FLOAT(spr_offset) * this_scale);
	}
	else
	{
		x1 = (FIXED_TO_FLOAT(spr_offset) * this_scale);
		x2 = (FIXED_TO_FLOAT(spr_width - spr_offset) * this_scale);
	}

	// test if too close
/*
	if (papersprite)
	{
		z1 = tz - x1 * angle_scalez;
		z2 = tz + x2 * angle_scalez;

		if (max(z1, z2) < ZCLIP_PLANE)
			return;
	}
*/

	z1 = tr_y + x1 * rightsin;
	z2 = tr_y - x2 * rightsin;
	x1 = tr_x + x1 * rightcos;
	x2 = tr_x - x2 * rightcos;

	if (vflip)
	{
<<<<<<< HEAD
		gz = FIXED_TO_FLOAT(thingzpos + thing->height) - FIXED_TO_FLOAT(spritecachedinfo[lumpoff].topoffset) * this_scale;
		gzt = gz + FIXED_TO_FLOAT(spritecachedinfo[lumpoff].height) * this_scale;
	}
	else
	{
		gzt = FIXED_TO_FLOAT(thingzpos) + FIXED_TO_FLOAT(spritecachedinfo[lumpoff].topoffset) * this_scale;
		gz = gzt - FIXED_TO_FLOAT(spritecachedinfo[lumpoff].height) * this_scale;
=======
		gz = FIXED_TO_FLOAT(thing->z+thing->height) - FIXED_TO_FLOAT(spr_topoffset) * this_scale;
		gzt = gz + FIXED_TO_FLOAT(spr_height) * this_scale;
	}
	else
	{
		gzt = FIXED_TO_FLOAT(thing->z) + FIXED_TO_FLOAT(spr_topoffset) * this_scale;
		gz = gzt - FIXED_TO_FLOAT(spr_height) * this_scale;
>>>>>>> srb2/next
	}

	if (thing->subsector->sector->cullheight)
	{
		if (HWR_DoCulling(thing->subsector->sector->cullheight, viewsector->cullheight, gr_viewz, gz, gzt))
			return;
	}

	heightsec = thing->subsector->sector->heightsec;
	if (viewplayer->mo && viewplayer->mo->subsector)
		phs = viewplayer->mo->subsector->sector->heightsec;
	else
		phs = -1;

	if (heightsec != -1 && phs != -1) // only clip things which are in special sectors
	{
		if (gr_viewz < FIXED_TO_FLOAT(sectors[phs].floorheight) ?
		FIXED_TO_FLOAT(thingzpos) >= FIXED_TO_FLOAT(sectors[heightsec].floorheight) :
		gzt < FIXED_TO_FLOAT(sectors[heightsec].floorheight))
			return;
		if (gr_viewz > FIXED_TO_FLOAT(sectors[phs].ceilingheight) ?
		gzt < FIXED_TO_FLOAT(sectors[heightsec].ceilingheight) && gr_viewz >= FIXED_TO_FLOAT(sectors[heightsec].ceilingheight) :
		FIXED_TO_FLOAT(thingzpos) >= FIXED_TO_FLOAT(sectors[heightsec].ceilingheight))
			return;
	}

	if ((thing->flags2 & MF2_LINKDRAW) && thing->tracer)
	{
		// bodge support - not nearly as comprehensive as r_things.c, but better than nothing
		if (! R_ThingVisible(thing->tracer))
			return;
	}

	// store information in a vissprite
	vis = HWR_NewVisSprite();
	vis->x1 = x1;
	vis->x2 = x2;
	vis->tz = tz; // Keep tz for the simple sprite sorting that happens
	vis->dispoffset = thing->info->dispoffset; // Monster Iestyn: 23/11/15: HARDWARE SUPPORT AT LAST
	//vis->patchlumpnum = sprframe->lumppat[rot];
#ifdef ROTSPRITE
	if (rotsprite)
		vis->gpatch = (GLPatch_t *)rotsprite;
	else
#endif
		vis->gpatch = (GLPatch_t *)W_CachePatchNum(sprframe->lumppat[rot], PU_CACHE);
	vis->flip = flip;
	vis->mobj = thing;


	//Hurdler: 25/04/2000: now support colormap in hardware mode
	if ((vis->mobj->flags & (MF_ENEMY|MF_BOSS)) && (vis->mobj->flags2 & MF2_FRET) && !(vis->mobj->flags & MF_GRENADEBOUNCE) && (leveltime & 1)) // Bosses "flash"
	{
		if (vis->mobj->type == MT_CYBRAKDEMON || vis->mobj->colorized)
			vis->colormap = R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);
		else if (vis->mobj->type == MT_METALSONIC_BATTLE)
			vis->colormap = R_GetTranslationColormap(TC_METALSONIC, 0, GTC_CACHE);
		else
			vis->colormap = R_GetTranslationColormap(TC_BOSS, 0, GTC_CACHE);
	}
	else if (thing->color)
	{
		// New colormap stuff for skins Tails 06-07-2002
		if (thing->colorized)
			vis->colormap = R_GetTranslationColormap(TC_RAINBOW, thing->color, GTC_CACHE);
<<<<<<< HEAD
=======
		else if (thing->player && thing->player->dashmode >= DASHMODE_THRESHOLD
			&& (thing->player->charflags & SF_DASHMODE)
			&& ((leveltime/2) & 1))
		{
			if (thing->player->charflags & SF_MACHINE)
				vis->colormap = R_GetTranslationColormap(TC_DASHMODE, 0, GTC_CACHE);
			else
				vis->colormap = R_GetTranslationColormap(TC_RAINBOW, thing->color, GTC_CACHE);
		}
>>>>>>> srb2/next
		else if (thing->skin && thing->sprite == SPR_PLAY) // This thing is a player!
		{
			size_t skinnum = (skin_t*)thing->skin-skins;
			vis->colormap = R_GetTranslationColormap((INT32)skinnum, thing->color, GTC_CACHE);
		}
		else
			vis->colormap = R_GetTranslationColormap(TC_DEFAULT, thing->color, GTC_CACHE);
	}
	else
	{
		vis->colormap = colormaps;
#ifdef GLENCORE
		if (encoremap && (thing->flags & (MF_SCENERY|MF_NOTHINK)) && !(thing->flags & MF_DONTENCOREMAP))
			vis->colormap += (256*32);
#endif
	}

	// set top/bottom coords
	vis->ty = gzt;

	//CONS_Debug(DBG_RENDER, "------------------\nH: sprite  : %d\nH: frame   : %x\nH: type    : %d\nH: sname   : %s\n\n",
	//            thing->sprite, thing->frame, thing->type, sprnames[thing->sprite]);

	vis->vflip = vflip;

	vis->precip = false;
}

// Precipitation projector for hardware mode
void HWR_ProjectPrecipitationSprite(precipmobj_t *thing)
{
	gr_vissprite_t *vis;
	float tr_x, tr_y;
	float tz;
	float x1, x2;
	float z1, z2;
	float rightsin, rightcos;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	size_t lumpoff;
	unsigned rot = 0;
	UINT8 flip;

	// transform the origin point
	tr_x = FIXED_TO_FLOAT(thing->x) - gr_viewx;
	tr_y = FIXED_TO_FLOAT(thing->y) - gr_viewy;

	// rotation around vertical axis
	tz = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);

	// thing is behind view plane?
	if (tz < ZCLIP_PLANE)
		return;

	tr_x = FIXED_TO_FLOAT(thing->x);
	tr_y = FIXED_TO_FLOAT(thing->y);

	// decide which patch to use for sprite relative to player
	if ((unsigned)thing->sprite >= numsprites)
#ifdef RANGECHECK
		I_Error("HWR_ProjectPrecipitationSprite: invalid sprite number %i ",
		        thing->sprite);
#else
		return;
#endif

	sprdef = &sprites[thing->sprite];

	if ((size_t)(thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
#ifdef RANGECHECK
		I_Error("HWR_ProjectPrecipitationSprite: invalid sprite frame %i : %i for %s",
		        thing->sprite, thing->frame, sprnames[thing->sprite]);
#else
		return;
#endif

	sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

	// use single rotation for all views
	lumpoff = sprframe->lumpid[0];
	flip = sprframe->flip; // Will only be 0x00 or 0xFF

	rightsin = FIXED_TO_FLOAT(FINESINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	rightcos = FIXED_TO_FLOAT(FINECOSINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	if (flip)
	{
		x1 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset);
		x2 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset);
	}
	else
	{
		x1 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset);
		x2 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset);
	}

	z1 = tr_y + x1 * rightsin;
	z2 = tr_y - x2 * rightsin;
	x1 = tr_x + x1 * rightcos;
	x2 = tr_x - x2 * rightcos;

<<<<<<< HEAD
	// okay, we can't return now... this is a hack, but weather isn't networked, so it should be ok
	if (!(thing->precipflags & PCF_THUNK))
	{
		P_PrecipThinker(thing);
		thing->precipflags |= PCF_THUNK;
	}

=======
>>>>>>> srb2/next
	//
	// store information in a vissprite
	//
	vis = HWR_NewVisSprite();
	vis->x1 = x1;
	vis->x2 = x2;
	vis->z1 = z1;
	vis->z2 = z2;
	vis->tz = tz;
	vis->dispoffset = 0; // Monster Iestyn: 23/11/15: HARDWARE SUPPORT AT LAST
	//vis->patchlumpnum = sprframe->lumppat[rot];
	vis->gpatch = (GLPatch_t *)W_CachePatchNum(sprframe->lumppat[rot], PU_CACHE);
	vis->flip = flip;
	vis->mobj = (mobj_t *)thing;

	vis->colormap = colormaps;

#ifdef GLENCORE
	if (encoremap && !(thing->flags & MF_DONTENCOREMAP))
		vis->colormap += (256*32);
#endif

	// set top/bottom coords
	vis->ty = FIXED_TO_FLOAT(thing->z + spritecachedinfo[lumpoff].topoffset);

	vis->precip = true;

	// okay... this is a hack, but weather isn't networked, so it should be ok
	if (!(thing->precipflags & PCF_THUNK))
	{
		if (thing->precipflags & PCF_RAIN)
			P_RainThinker(thing);
		else
			P_SnowThinker(thing);
		thing->precipflags |= PCF_THUNK;
	}
}
<<<<<<< HEAD

static boolean drewsky = false;

void HWR_DrawSkyBackground(float fpov)
{
	FTransform dometransform;

	if (drewsky)
		return;

	memset(&dometransform, 0x00, sizeof(FTransform));

	//04/01/2000: Hurdler: added for T&L
	//                     It should replace all other gr_viewxxx when finished
	if (!atransform.shearing)
		dometransform.anglex = (float)(aimingangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
	dometransform.angley = (float)((viewangle-ANGLE_270)>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);

	dometransform.flip = atransform.flip;
	dometransform.mirror = atransform.mirror;
	dometransform.shearing = atransform.shearing;
	dometransform.viewaiming = atransform.viewaiming;

	dometransform.scalex = 1;
	dometransform.scaley = (float)vid.width/vid.height;
	dometransform.scalez = 1;
	dometransform.fovxangle = fpov; // Tails
	dometransform.fovyangle = fpov; // Tails
	dometransform.splitscreen = splitscreen;

	HWR_GetTexture(texturetranslation[skytexture]);
	HWD.pfnSetShader(7);	// sky shader
	HWD.pfnRenderSkyDome(skytexture, textures[skytexture]->width, textures[skytexture]->height, dometransform);
	HWD.pfnSetShader(0);
=======
#endif

// ==========================================================================
//
// ==========================================================================
static void HWR_DrawSkyBackground(player_t *player)
{
	if (cv_grskydome.value)
	{
		FTransform dometransform;
		const float fpov = FIXED_TO_FLOAT(cv_fov.value+player->fovadd);
		postimg_t *type;

		if (splitscreen && player == &players[secondarydisplayplayer])
			type = &postimgtype2;
		else
			type = &postimgtype;

		memset(&dometransform, 0x00, sizeof(FTransform));

		//04/01/2000: Hurdler: added for T&L
		//                     It should replace all other gr_viewxxx when finished
		dometransform.anglex = (float)(aimingangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
		dometransform.angley = (float)((viewangle-ANGLE_270)>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);

		if (*type == postimg_flip)
			dometransform.flip = true;
		else
			dometransform.flip = false;

		dometransform.scalex = 1;
		dometransform.scaley = (float)vid.width/vid.height;
		dometransform.scalez = 1;
		dometransform.fovxangle = fpov; // Tails
		dometransform.fovyangle = fpov; // Tails
		if (player->viewrollangle != 0)
		{
			fixed_t rol = AngleFixed(player->viewrollangle);
			dometransform.rollangle = FIXED_TO_FLOAT(rol);
			dometransform.roll = true;
		}
		dometransform.splitscreen = splitscreen;

		HWR_GetTexture(texturetranslation[skytexture]);
		HWD.pfnRenderSkyDome(skytexture, textures[skytexture]->width, textures[skytexture]->height, dometransform);
	}
	else
	{
		FOutVector v[4];
		angle_t angle;
		float dimensionmultiply;
		float aspectratio;
		float angleturn;

		HWR_GetTexture(texturetranslation[skytexture]);
		aspectratio = (float)vid.width/(float)vid.height;

		//Hurdler: the sky is the only texture who need 4.0f instead of 1.0
		//         because it's called just after clearing the screen
		//         and thus, the near clipping plane is set to 3.99
		// Sryder: Just use the near clipping plane value then

		//  3--2
		//  | /|
		//  |/ |
		//  0--1
		v[0].x = v[3].x = -ZCLIP_PLANE-1;
		v[1].x = v[2].x =  ZCLIP_PLANE+1;
		v[0].y = v[1].y = -ZCLIP_PLANE-1;
		v[2].y = v[3].y =  ZCLIP_PLANE+1;

		v[0].z = v[1].z = v[2].z = v[3].z = ZCLIP_PLANE+1;

		// X

		// NOTE: This doesn't work right with texture widths greater than 1024
		// software doesn't draw any further than 1024 for skies anyway, but this doesn't overlap properly
		// The only time this will probably be an issue is when a sky wider than 1024 is used as a sky AND a regular wall texture

		angle = (dup_viewangle + gr_xtoviewangle[0]);

		dimensionmultiply = ((float)textures[texturetranslation[skytexture]]->width/256.0f);

		v[0].sow = v[3].sow = (-1.0f * angle) / ((ANGLE_90-1)*dimensionmultiply); // left
		v[2].sow = v[1].sow = v[0].sow + (1.0f/dimensionmultiply); // right (or left + 1.0f)
		// use +angle and -1.0f above instead if you wanted old backwards behavior

		// Y
		angle = aimingangle;
		dimensionmultiply = ((float)textures[texturetranslation[skytexture]]->height/(128.0f*aspectratio));

		if (splitscreen)
		{
			dimensionmultiply *= 2;
			angle *= 2;
		}

		// Middle of the sky should always be at angle 0
		// need to keep correct aspect ratio with X
		if (atransform.flip)
		{
			// During vertical flip the sky should be flipped and it's y movement should also be flipped obviously
			v[3].tow = v[2].tow = -(0.5f-(0.5f/dimensionmultiply)); // top
			v[0].tow = v[1].tow = v[3].tow - (1.0f/dimensionmultiply); // bottom (or top - 1.0f)
		}
		else
		{
			v[0].tow = v[1].tow = -(0.5f-(0.5f/dimensionmultiply)); // bottom
			v[3].tow = v[2].tow = v[0].tow - (1.0f/dimensionmultiply); // top (or bottom - 1.0f)
		}

		angleturn = (((float)ANGLE_45-1.0f)*aspectratio)*dimensionmultiply;

		if (angle > ANGLE_180) // Do this because we don't want the sky to suddenly teleport when crossing over 0 to 360 and vice versa
		{
			angle = InvAngle(angle);
			v[3].tow = v[2].tow += ((float) angle / angleturn);
			v[0].tow = v[1].tow += ((float) angle / angleturn);
		}
		else
		{
			v[3].tow = v[2].tow -= ((float) angle / angleturn);
			v[0].tow = v[1].tow -= ((float) angle / angleturn);
		}

		HWD.pfnDrawPolygon(NULL, v, 4, 0);
	}
>>>>>>> srb2/next
}


// -----------------+
// HWR_ClearView : clear the viewwindow, with maximum z value
// -----------------+
static inline void HWR_ClearView(void)
{
	HWD.pfnGClipRect((INT32)gr_viewwindowx,
	                 (INT32)gr_viewwindowy,
	                 (INT32)(gr_viewwindowx + gr_viewwidth),
	                 (INT32)(gr_viewwindowy + gr_viewheight),
	                 ZCLIP_PLANE);
	HWD.pfnClearBuffer(false, true, 0);
}


// -----------------+
// HWR_SetViewSize  : set projection and scaling values
// -----------------+
void HWR_SetViewSize(void)
{
	// setup view size
	gr_viewwidth = (float)vid.width;
	gr_viewheight = (float)vid.height;

	if (r_splitscreen)
		gr_viewheight /= 2;

	if (r_splitscreen > 1)
		gr_viewwidth /= 2;

	gr_basecenterx = gr_viewwidth / 2;
	gr_basecentery = gr_viewheight / 2;

	gr_baseviewwindowy = 0;
	gr_basewindowcentery = (float)(gr_viewheight / 2);

	gr_baseviewwindowx = 0;
	gr_basewindowcenterx = (float)(gr_viewwidth / 2);

	gr_pspritexscale = ((vid.width*gr_pspriteyscale*BASEVIDHEIGHT)/BASEVIDWIDTH)/vid.height;
	gr_pspriteyscale = ((vid.height*gr_pspritexscale*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width;

	HWD.pfnFlushScreenTextures();
}


// ==========================================================================
// Render the current frame.
// ==========================================================================
void HWR_RenderFrame(INT32 viewnumber, player_t *player, boolean skybox)
{
<<<<<<< HEAD
	angle_t a1;
	const float fpov = FIXED_TO_FLOAT(cv_fov.value+player->fovadd);
	postimg_t *postprocessor = &postimgtype[0];
	INT32 i;
=======
	const float fpov = FIXED_TO_FLOAT(cv_fov.value+player->fovadd);
	postimg_t *type;

	if (splitscreen && player == &players[secondarydisplayplayer])
		type = &postimgtype2;
	else
		type = &postimgtype;

	{
		// do we really need to save player (is it not the same)?
		player_t *saved_player = stplyr;
		stplyr = player;
		ST_doPaletteStuff();
		stplyr = saved_player;
#ifdef ALAM_LIGHTING
		HWR_SetLights(viewnumber);
#endif
	}

	// note: sets viewangle, viewx, viewy, viewz
	R_SkyboxFrame(player);

	// copy view cam position for local use
	dup_viewx = viewx;
	dup_viewy = viewy;
	dup_viewz = viewz;
	dup_viewangle = viewangle;
>>>>>>> srb2/next

	// set window position
	gr_centerx = gr_basecenterx;
	gr_viewwindowx = gr_baseviewwindowx;
	gr_windowcenterx = gr_basewindowcenterx;
	gr_centery = gr_basecentery;
	gr_viewwindowy = gr_baseviewwindowy;
	gr_windowcentery = gr_basewindowcentery;

	if ((r_splitscreen == 1 && viewnumber == 1) || (r_splitscreen > 1 && viewnumber > 1))
	{
		gr_viewwindowy += gr_viewheight;
		gr_windowcentery += gr_viewheight;
	}

	if (r_splitscreen > 1 && viewnumber & 1)
	{
		gr_viewwindowx += gr_viewwidth;
		gr_windowcenterx += gr_viewwidth;
	}

	// check for new console commands.
	NetUpdate();

	gr_viewx = FIXED_TO_FLOAT(viewx);
	gr_viewy = FIXED_TO_FLOAT(viewy);
	gr_viewz = FIXED_TO_FLOAT(viewz);
	gr_viewsin = FIXED_TO_FLOAT(viewsin);
	gr_viewcos = FIXED_TO_FLOAT(viewcos);

<<<<<<< HEAD
	// Set T&L transform
	atransform.x = gr_viewx;
	atransform.y = gr_viewy;
	atransform.z = gr_viewz;
=======
	gr_viewludsin = FIXED_TO_FLOAT(FINECOSINE(aimingangle>>ANGLETOFINESHIFT));
	gr_viewludcos = FIXED_TO_FLOAT(-FINESINE(aimingangle>>ANGLETOFINESHIFT));

	//04/01/2000: Hurdler: added for T&L
	//                     It should replace all other gr_viewxxx when finished
	memset(&atransform, 0x00, sizeof(FTransform));

	atransform.anglex = (float)(aimingangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
	atransform.angley = (float)(viewangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);

	if (*type == postimg_flip)
		atransform.flip = true;
	else
		atransform.flip = false;
>>>>>>> srb2/next

	atransform.scalex = 1;
	atransform.scaley = (float)vid.width/vid.height;
	atransform.scalez = 1;

	// 14042019
	gr_aimingangle = aimingangle;
	atransform.shearing = false;
	atransform.viewaiming = aimingangle;

	if (cv_grshearing.value)
	{
		gr_aimingangle = 0;
		atransform.shearing = true;
	}

	gr_viewludsin = FIXED_TO_FLOAT(FINECOSINE(gr_aimingangle>>ANGLETOFINESHIFT));
	gr_viewludcos = FIXED_TO_FLOAT(-FINESINE(gr_aimingangle>>ANGLETOFINESHIFT));

	atransform.anglex = (float)(gr_aimingangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
	atransform.angley = (float)(viewangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);

	atransform.fovxangle = fpov; // Tails
	atransform.fovyangle = fpov; // Tails
<<<<<<< HEAD
	atransform.splitscreen = r_splitscreen;
=======
	if (player->viewrollangle != 0)
	{
		fixed_t rol = AngleFixed(player->viewrollangle);
		atransform.rollangle = FIXED_TO_FLOAT(rol);
		atransform.roll = true;
	}
	atransform.splitscreen = splitscreen;
>>>>>>> srb2/next

	for (i = 0; i <= splitscreen; i++)
	{
		if (player == &players[displayplayers[i]])
			postprocessor = &postimgtype[i];
	}

	atransform.flip = false;
	if (*postprocessor == postimg_flip)
		atransform.flip = true;

	atransform.mirror = false;
	if (*postprocessor == postimg_mirror)
		atransform.mirror = true;

<<<<<<< HEAD
	// Clear view, set viewport (glViewport), set perspective...
	HWR_ClearView();
	HWR_ClearSprites();
=======
	if (drawsky)
		HWR_DrawSkyBackground(player);
>>>>>>> srb2/next

	ST_doPaletteStuff();

	// Draw the sky background.
	HWR_DrawSkyBackground(fpov);
	if (skybox)
		drewsky = true;

	a1 = gld_FrustumAngle(gr_aimingangle);
	gld_clipper_Clear();
	gld_clipper_SafeAddClipRange(viewangle + a1, viewangle - a1);
#ifdef HAVE_SPHEREFRUSTRUM
	gld_FrustrumSetup();
#endif

	// Set transform.
	HWD.pfnSetTransform(&atransform);

	// Reset the shader state.
	HWD.pfnSetSpecialState(HWD_SET_SHADERS, cv_grshaders.value);
	HWD.pfnSetShader(0);

	if (cv_grbatching.value)
		HWD.pfnStartBatching();

	drawcount = 0;
	validcount++;

	// Recursively "render" the BSP tree.
	HWR_RenderBSPNode((INT32)numnodes-1);

<<<<<<< HEAD
	if (cv_grbatching.value)
=======
#ifndef NEWCLIP
	// Make a viewangle int so we can render things based on mouselook
	if (player == &players[consoleplayer])
		viewangle = localaiming;
	else if (splitscreen && player == &players[secondarydisplayplayer])
		viewangle = localaiming2;

	// Handle stuff when you are looking farther up or down.
	if ((aimingangle || cv_fov.value+player->fovadd > 90*FRACUNIT))
>>>>>>> srb2/next
	{
		int dummy = 0;// the vars in RenderBatches are meant for render stats. But we don't have that stuff in this branch
					// so that stuff could be removed...
		HWD.pfnRenderBatches(&dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);
	}

	// Check for new console commands.
	// this was removed since it caused crashes on leaving record attack with models on since it was removing mobjs that were about to be rendered
	//NetUpdate();

	// Draw MD2 and sprites
	HWR_SortVisSprites();
	HWR_DrawSprites();

	if (numplanes || numpolyplanes || numwalls) // Render FOFs and translucent walls after everything
		HWR_RenderDrawNodes();

	// Unset transform and shader
	HWD.pfnSetTransform(NULL);
	HWD.pfnUnSetShader();

	// Run post processor effects
	if (!skybox)
		HWR_DoPostProcessor(player);

	// Check for new console commands.
	NetUpdate();

	// added by Hurdler for correct splitscreen
	// moved here by hurdler so it works with the new near clipping plane
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
}

// ==========================================================================
// Render the player view.
// ==========================================================================
void HWR_RenderPlayerView(INT32 viewnumber, player_t *player)
{
<<<<<<< HEAD
=======
	const float fpov = FIXED_TO_FLOAT(cv_fov.value+player->fovadd);
	postimg_t *type;

>>>>>>> srb2/next
	const boolean skybox = (skyboxmo[0] && cv_skybox.value); // True if there's a skybox object and skyboxes are on

	// Clear the color buffer, stops HOMs. Also seems to fix the skybox issue on Intel GPUs.
	if (viewnumber == 0) // Only do it if it's the first screen being rendered
	{
		FRGBAFloat ClearColor;

		ClearColor.red = 0.0f;
		ClearColor.green = 0.0f;
		ClearColor.blue = 0.0f;
		ClearColor.alpha = 1.0f;

		HWD.pfnClearBuffer(true, false, &ClearColor);
	}

<<<<<<< HEAD
	if (viewnumber > 3)
		return;
=======
	// note: sets viewangle, viewx, viewy, viewz
	R_SetupFrame(player);
	framecount++; // timedemo

	// copy view cam position for local use
	dup_viewx = viewx;
	dup_viewy = viewy;
	dup_viewz = viewz;
	dup_viewangle = viewangle;
>>>>>>> srb2/next

	// Render the skybox if there is one.
	drewsky = false;
	if (skybox)
	{
		R_SkyboxFrame(player);
		HWR_RenderFrame(viewnumber, player, true);
	}

	R_SetupFrame(player, false); // This can stay false because it is only used to set viewsky in r_main.c, which isn't used here
	HWR_RenderFrame(viewnumber, player, false);
}

<<<<<<< HEAD
// ==========================================================================
//                                                         3D ENGINE COMMANDS
// ==========================================================================

// **************************************************************************
//                                                            3D ENGINE SETUP
// **************************************************************************
=======
	//04/01/2000: Hurdler: added for T&L
	//                     It should replace all other gr_viewxxx when finished
	memset(&atransform, 0x00, sizeof(FTransform));

	atransform.anglex = (float)(aimingangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
	atransform.angley = (float)(viewangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);

	if (*type == postimg_flip)
		atransform.flip = true;
	else
		atransform.flip = false;

	atransform.x      = gr_viewx;  // FIXED_TO_FLOAT(viewx)
	atransform.y      = gr_viewy;  // FIXED_TO_FLOAT(viewy)
	atransform.z      = gr_viewz;  // FIXED_TO_FLOAT(viewz)
	atransform.scalex = 1;
	atransform.scaley = (float)vid.width/vid.height;
	atransform.scalez = 1;
	atransform.fovxangle = fpov; // Tails
	atransform.fovyangle = fpov; // Tails
	if (player->viewrollangle != 0)
	{
		fixed_t rol = AngleFixed(player->viewrollangle);
		atransform.rollangle = FIXED_TO_FLOAT(rol);
		atransform.roll = true;
	}
	atransform.splitscreen = splitscreen;

	gr_fovlud = (float)(1.0l/tan((double)(fpov*M_PIl/360l)));

	//------------------------------------------------------------------------
	HWR_ClearView(); // Clears the depth buffer and resets the view I believe

if (0)
{ // I don't think this is ever used.
	if (cv_grfog.value)
		HWR_FoggingOn(); // First of all, turn it on, set the default user settings too
	else
		HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 0); // Turn it off
}

	if (!skybox && drawsky) // Don't draw the regular sky if there's a skybox
		HWR_DrawSkyBackground(player);

	//Hurdler: it doesn't work in splitscreen mode
	drawsky = splitscreen;

	HWR_ClearSprites();

#ifdef SORTING
	drawcount = 0;
#endif
#ifdef NEWCLIP
	if (rendermode == render_opengl)
	{
		angle_t a1 = gld_FrustumAngle();
		gld_clipper_Clear();
		gld_clipper_SafeAddClipRange(viewangle + a1, viewangle - a1);
#ifdef HAVE_SPHEREFRUSTRUM
		gld_FrustrumSetup();
#endif
	}
#else
	HWR_ClearClipSegs();
#endif

	//04/01/2000: Hurdler: added for T&L
	//                     Actually it only works on Walls and Planes
	HWD.pfnSetTransform(&atransform);

	validcount++;

	HWR_RenderBSPNode((INT32)numnodes-1);

#ifndef NEWCLIP
	// Make a viewangle int so we can render things based on mouselook
	if (player == &players[consoleplayer])
		viewangle = localaiming;
	else if (splitscreen && player == &players[secondarydisplayplayer])
		viewangle = localaiming2;

	// Handle stuff when you are looking farther up or down.
	if ((aimingangle || cv_fov.value+player->fovadd > 90*FRACUNIT))
	{
		dup_viewangle += ANGLE_90;
		HWR_ClearClipSegs();
		HWR_RenderBSPNode((INT32)numnodes-1); //left

		dup_viewangle += ANGLE_90;
		if (((INT32)aimingangle > ANGLE_45 || (INT32)aimingangle<-ANGLE_45))
		{
			HWR_ClearClipSegs();
			HWR_RenderBSPNode((INT32)numnodes-1); //back
		}

		dup_viewangle += ANGLE_90;
		HWR_ClearClipSegs();
		HWR_RenderBSPNode((INT32)numnodes-1); //right

		dup_viewangle += ANGLE_90;
	}
#endif

	// Check for new console commands.
	NetUpdate();

#ifdef ALAM_LIGHTING
	//14/11/99: Hurdler: moved here because it doesn't work with
	// subsector, see other comments;
	HWR_ResetLights();
#endif

	// Draw MD2 and sprites
#ifdef SORTING
	HWR_SortVisSprites();
#endif

#ifdef SORTING
	HWR_DrawSprites();
#endif
#ifdef NEWCORONAS
	//Hurdler: they must be drawn before translucent planes, what about gl fog?
	HWR_DrawCoronas();
#endif

#ifdef SORTING
	if (numplanes || numpolyplanes || numwalls) //Hurdler: render 3D water and transparent walls after everything
	{
		HWR_CreateDrawNodes();
	}
#else
	if (numfloors || numpolyplanes || numwalls)
	{
		HWD.pfnSetTransform(&atransform);
		if (numfloors)
			HWR_Render3DWater();
		if (numwalls)
			HWR_RenderTransparentWalls();
	}
#endif

	HWD.pfnSetTransform(NULL);

	// put it off for menus etc
	if (cv_grfog.value)
		HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 0);

	HWR_DoPostProcessor(player);

	// Check for new console commands.
	NetUpdate();

	// added by Hurdler for correct splitscreen
	// moved here by hurdler so it works with the new near clipping plane
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
}

// ==========================================================================
//                                                                        FOG
// ==========================================================================

/// \author faB

static UINT32 atohex(const char *s)
{
	INT32 iCol;
	const char *sCol;
	char cCol;
	INT32 i;

	if (strlen(s)<6)
		return 0;

	iCol = 0;
	sCol = s;
	for (i = 0; i < 6; i++, sCol++)
	{
		iCol <<= 4;
		cCol = *sCol;
		if (cCol >= '0' && cCol <= '9')
			iCol |= cCol - '0';
		else
		{
			if (cCol >= 'F')
				cCol -= 'a' - 'A';
			if (cCol >= 'A' && cCol <= 'F')
				iCol = iCol | (cCol - 'A' + 10);
		}
	}
	//CONS_Debug(DBG_RENDER, "col %x\n", iCol);
	return iCol;
}

static void HWR_FoggingOn(void)
{
	HWD.pfnSetSpecialState(HWD_SET_FOG_COLOR, atohex(cv_grfogcolor.string));
	HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, cv_grfogdensity.value);
	HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 1);
}

// ==========================================================================
//                                                         3D ENGINE COMMANDS
// ==========================================================================

static CV_PossibleValue_t grsoftwarefog_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "LightPlanes"}, {0, NULL}};
static CV_PossibleValue_t grmodelinterpolation_cons_t[] = {{0, "Off"}, {1, "Sometimes"}, {2, "Always"}, {0, NULL}};

static void CV_grmodellighting_OnChange(void);
static void CV_grfiltermode_OnChange(void);
static void CV_granisotropic_OnChange(void);
static void CV_grfogdensity_OnChange(void);

static CV_PossibleValue_t grfiltermode_cons_t[]= {{HWD_SET_TEXTUREFILTER_POINTSAMPLED, "Nearest"},
	{HWD_SET_TEXTUREFILTER_BILINEAR, "Bilinear"}, {HWD_SET_TEXTUREFILTER_TRILINEAR, "Trilinear"},
	{HWD_SET_TEXTUREFILTER_MIXED1, "Linear_Nearest"},
	{HWD_SET_TEXTUREFILTER_MIXED2, "Nearest_Linear"},
	{HWD_SET_TEXTUREFILTER_MIXED3, "Nearest_Mipmap"},
	{0, NULL}};
CV_PossibleValue_t granisotropicmode_cons_t[] = {{1, "MIN"}, {16, "MAX"}, {0, NULL}};

consvar_t cv_fovchange = {"gr_fovchange", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grfog = {"gr_fog", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grfogcolor = {"gr_fogcolor", "AAAAAA", CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grsoftwarefog = {"gr_softwarefog", "Off", CV_SAVE, grsoftwarefog_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

#ifdef ALAM_LIGHTING
consvar_t cv_grdynamiclighting = {"gr_dynamiclighting", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grstaticlighting  = {"gr_staticlighting", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grcoronas = {"gr_coronas", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grcoronasize = {"gr_coronasize", "1", CV_SAVE|CV_FLOAT, 0, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif

consvar_t cv_grmodels = {"gr_models", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grmodelinterpolation = {"gr_modelinterpolation", "Sometimes", CV_SAVE, grmodelinterpolation_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grmodellighting = {"gr_modellighting", "Off", CV_SAVE|CV_CALL, CV_OnOff, CV_grmodellighting_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grspritebillboarding = {"gr_spritebillboarding", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grskydome = {"gr_skydome", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grfakecontrast = {"gr_fakecontrast", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grrounddown = {"gr_rounddown", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grfogdensity = {"gr_fogdensity", "150", CV_CALL|CV_NOINIT, CV_Unsigned,
                             CV_grfogdensity_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grfiltermode = {"gr_filtermode", "Nearest", CV_SAVE|CV_CALL, grfiltermode_cons_t,
                             CV_grfiltermode_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_granisotropicmode = {"gr_anisotropicmode", "1", CV_CALL, granisotropicmode_cons_t,
                             CV_granisotropic_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grcorrecttricks = {"gr_correcttricks", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grsolvetjoin = {"gr_solvetjoin", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static void CV_grmodellighting_OnChange(void)
{
	if (rendermode == render_opengl)
		HWD.pfnSetSpecialState(HWD_SET_MODEL_LIGHTING, cv_grmodellighting.value);
}

static void CV_grfogdensity_OnChange(void)
{
	if (rendermode == render_opengl)
		HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, cv_grfogdensity.value);
}

static void CV_grfiltermode_OnChange(void)
{
	if (rendermode == render_opengl)
		HWD.pfnSetSpecialState(HWD_SET_TEXTUREFILTERMODE, cv_grfiltermode.value);
}

static void CV_granisotropic_OnChange(void)
{
	if (rendermode == render_opengl)
		HWD.pfnSetSpecialState(HWD_SET_TEXTUREANISOTROPICMODE, cv_granisotropicmode.value);
}
>>>>>>> srb2/next

//added by Hurdler: console varibale that are saved
void HWR_AddCommands(void)
{
<<<<<<< HEAD
	CV_RegisterVar(&cv_grrounddown);
=======
	CV_RegisterVar(&cv_fovchange);

	CV_RegisterVar(&cv_grfogdensity);
	CV_RegisterVar(&cv_grfogcolor);
	CV_RegisterVar(&cv_grfog);
	CV_RegisterVar(&cv_grsoftwarefog);

#ifdef ALAM_LIGHTING
	CV_RegisterVar(&cv_grstaticlighting);
	CV_RegisterVar(&cv_grdynamiclighting);
	CV_RegisterVar(&cv_grcoronasize);
	CV_RegisterVar(&cv_grcoronas);
#endif

	CV_RegisterVar(&cv_grmodellighting);
	CV_RegisterVar(&cv_grmodelinterpolation);
	CV_RegisterVar(&cv_grmodels);

	CV_RegisterVar(&cv_grskydome);
	CV_RegisterVar(&cv_grspritebillboarding);
	CV_RegisterVar(&cv_grfakecontrast);

>>>>>>> srb2/next
	CV_RegisterVar(&cv_grfiltermode);
	CV_RegisterVar(&cv_grrounddown);
	CV_RegisterVar(&cv_grcorrecttricks);
	CV_RegisterVar(&cv_grsolvetjoin);

<<<<<<< HEAD
	CV_RegisterVar(&cv_grbatching);
=======
#ifndef NEWCLIP
	CV_RegisterVar(&cv_grclipwalls);
#endif
}

void HWR_AddSessionCommands(void)
{
	static boolean alreadycalled = false;
	if (alreadycalled)
		return;

	CV_RegisterVar(&cv_granisotropicmode);

	alreadycalled = true;
>>>>>>> srb2/next
}

// --------------------------------------------------------------------------
// Setup the hardware renderer
// --------------------------------------------------------------------------
void HWR_Startup(void)
{
	static boolean startupdone = false;

	// do this once
	if (!startupdone)
	{
		CONS_Printf("HWR_Startup()...\n");
<<<<<<< HEAD
		HWR_InitTextureCache();
		HWR_InitMD2();
=======
		HWR_InitPolyPool();
		HWR_AddSessionCommands();
		HWR_InitTextureCache();
		HWR_InitModels();
#ifdef ALAM_LIGHTING
		HWR_InitLight();
#endif
>>>>>>> srb2/next
	}

	if (rendermode == render_opengl)
		textureformat = patchformat = GR_RGBA;

	startupdone = true;

	// jimita
	HWD.pfnKillShaders();
	if (!HWD.pfnLoadShaders())
		gr_shadersavailable = false;
}

// --------------------------------------------------------------------------
// Called after switching to the hardware renderer
// --------------------------------------------------------------------------
void HWR_Switch(void)
{
	// Set special states from CVARs
	HWD.pfnSetSpecialState(HWD_SET_MODEL_LIGHTING, cv_grmodellighting.value);
	HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, cv_grfogdensity.value);
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREFILTERMODE, cv_grfiltermode.value);
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREANISOTROPICMODE, cv_granisotropicmode.value);
}

// --------------------------------------------------------------------------
// Free resources allocated by the hardware renderer
// --------------------------------------------------------------------------
void HWR_Shutdown(void)
{
	CONS_Printf("HWR_Shutdown()\n");
	HWR_FreeExtraSubsectors();
	HWR_FreeTextureCache();
	HWD.pfnFlushScreenTextures();
}

<<<<<<< HEAD
void HWR_AddTransparentWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, INT32 texnum, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap)
=======
void transform(float *cx, float *cy, float *cz)
{
	float tr_x,tr_y;
	// translation
	tr_x = *cx - gr_viewx;
	tr_y = *cz - gr_viewy;
//	*cy = *cy;

	// rotation around vertical y axis
	*cx = (tr_x * gr_viewsin) - (tr_y * gr_viewcos);
	tr_x = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);

	//look up/down ----TOTAL SUCKS!!!--- do the 2 in one!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	tr_y = *cy - gr_viewz;

	*cy = (tr_x * gr_viewludcos) + (tr_y * gr_viewludsin);
	*cz = (tr_x * gr_viewludsin) - (tr_y * gr_viewludcos);

	//scale y before frustum so that frustum can be scaled to screen height
	*cy *= ORIGINAL_ASPECT * gr_fovlud;
	*cx *= gr_fovlud;
}


//Hurdler: 3D Water stuff
#ifndef SORTING

#define MAX_3DWATER 512

static void HWR_Add3DWater(levelflat_t *levelflat, extrasubsector_t *xsub,
	fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector)
{
	static size_t allocedplanes = 0;

	// Force realloc if buffer has been freed
	if (!planeinfo)
		allocedplanes = 0;

	if (allocedplanes < numfloors + 1)
	{
		allocedplanes += MAX_3DWATER;
		Z_Realloc(planeinfo, allocedplanes * sizeof (*planeinfo), PU_LEVEL, &planeinfo);
	}
	planeinfo[numfloors].fixedheight = fixedheight;
	planeinfo[numfloors].lightlevel = lightlevel;
	planeinfo[numfloors].levelflat = levelflat;
	planeinfo[numfloors].xsub = xsub;
	planeinfo[numfloors].alpha = alpha;
	planeinfo[numfloors].FOFSector = FOFSector;
	numfloors++;
}

#define DIST_PLANE(i) ABS(planeinfo[(i)].fixedheight-dup_viewz)

static void HWR_QuickSortPlane(INT32 start, INT32 finish)
{
	INT32 left = start;
	INT32 right = finish;
	INT32 starterval = (INT32)((right+left)/2); //pick a starter

	planeinfo_t temp;

	//'sort of sort' the two halves of the data list about starterval
	while (right > left);
	{
		while (DIST_PLANE(left) < DIST_PLANE(starterval)) left++; //attempt to find a bigger value on the left
		while (DIST_PLANE(right) > DIST_PLANE(starterval)) right--; //attempt to find a smaller value on the right

		if (left < right) //if we haven't gone too far
		{
			//switch them
			M_Memcpy(&temp, &planeinfo[left], sizeof (planeinfo_t));
			M_Memcpy(&planeinfo[left], &planeinfo[right], sizeof (planeinfo_t));
			M_Memcpy(&planeinfo[right], &temp, sizeof (planeinfo_t));
			//move the bounds
			left++;
			right--;
		}
	}

	if (start < right) HWR_QuickSortPlane(start, right);
	if (left < finish) HWR_QuickSortPlane(left, finish);
}

static void HWR_Render3DWater(void)
{
	size_t i;

	//bubble sort 3D Water for correct alpha blending
	{
		boolean permut = true;
		while (permut)
		{
			size_t j;
			for (j = 0, permut= false; j < numfloors-1; j++)
			{
				if (ABS(planeinfo[j].fixedheight-dup_viewz) < ABS(planeinfo[j+1].fixedheight-dup_viewz))
				{
					planeinfo_t temp;
					M_Memcpy(&temp, &planeinfo[j+1], sizeof (planeinfo_t));
					M_Memcpy(&planeinfo[j+1], &planeinfo[j], sizeof (planeinfo_t));
					M_Memcpy(&planeinfo[j], &temp, sizeof (planeinfo_t));
					permut = true;
				}
			}
		}
	}
#if 0 //thanks epat, but it's goes looping forever on CTF map Silver Cascade Zone
	HWR_QuickSortPlane(0, numplanes-1);
#endif

	gr_frontsector = NULL; //Hurdler: gr_fronsector is no longer valid
	for (i = 0; i < numfloors; i++)
	{
		HWR_GetLevelFlat(planeinfo[i].levelflat);
		HWR_RenderPlane(NULL, planeinfo[i].xsub, planeinfo[i].isceiling, planeinfo[i].fixedheight, PF_Translucent, planeinfo[i].lightlevel, planeinfo[i].levelflat,
			planeinfo[i].FOFSector, planeinfo[i].alpha, planeinfo[i].fogplane, planeinfo[i].planecolormap);
	}
	numfloors = 0;
}
#endif

static void HWR_AddTransparentWall(wallVert3D *wallVerts, FSurfaceInfo *pSurf, INT32 texnum, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap)
>>>>>>> srb2/next
{
	static size_t allocedwalls = 0;

	// Force realloc if buffer has been freed
	if (!wallinfo)
		allocedwalls = 0;

	if (allocedwalls < numwalls + 1)
	{
		allocedwalls += MAX_TRANSPARENTWALL;
		Z_Realloc(wallinfo, allocedwalls * sizeof (*wallinfo), PU_LEVEL, &wallinfo);
	}

	M_Memcpy(wallinfo[numwalls].wallVerts, wallVerts, sizeof (wallinfo[numwalls].wallVerts));
	M_Memcpy(&wallinfo[numwalls].Surf, pSurf, sizeof (FSurfaceInfo));
	wallinfo[numwalls].texnum = texnum;
	wallinfo[numwalls].blend = blend;
	wallinfo[numwalls].drawcount = drawcount++;
	wallinfo[numwalls].fogwall = fogwall;
	wallinfo[numwalls].lightlevel = lightlevel;
	wallinfo[numwalls].wallcolormap = wallcolormap;
	numwalls++;
}

<<<<<<< HEAD
void HWR_RenderWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap)
=======
#ifndef SORTING
static void HWR_RenderTransparentWalls(void)
>>>>>>> srb2/next
{
	FBITFIELD blendmode = blend;
	UINT8 alpha = pSurf->PolyColor.s.alpha; // retain the alpha

	// Lighting is done here instead so that fog isn't drawn incorrectly on transparent walls after sorting
	HWR_Lighting(pSurf, lightlevel, wallcolormap);

<<<<<<< HEAD
	pSurf->PolyColor.s.alpha = alpha; // put the alpha back after lighting
=======
	for (i = 0; i < numwalls; i++)
	{
		HWR_GetTexture(wallinfo[i].texnum);
		HWR_RenderWall(wallinfo[i].wallVerts, &wallinfo[i].Surf, wallinfo[i].blend, wallinfo[i].wall->fogwall, wallinfo[i].wall->lightlevel, wallinfo[i].wall->wallcolormap);
	}
	numwalls = 0;
}
#endif

static void HWR_RenderWall(wallVert3D   *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	FOutVector  trVerts[4];
	UINT8       i;
	FOutVector  *wv;
	UINT8 alpha;
>>>>>>> srb2/next

	HWD.pfnSetShader(2);	// wall shader

	if (blend & PF_Environment)
		blendmode |= PF_Occlude;	// PF_Occlude must be used for solid objects

	if (fogwall)
	{
		blendmode |= PF_Fog;
		HWD.pfnSetShader(6);	// fog shader
	}

	blendmode |= PF_Modulated;	// No PF_Occlude means overlapping (incorrect) transparency

	HWD.pfnDrawPolygon(pSurf, wallVerts, 4, blendmode);

#ifdef WALLSPLATS
	if (gr_curline->linedef->splats && cv_splats.value)
		HWR_DrawSegsSplats(pSurf);
#endif
<<<<<<< HEAD
=======
#endif
}

INT32 HWR_GetTextureUsed(void)
{
	return HWD.pfnGetTextureUsed();
>>>>>>> srb2/next
}

void HWR_DoPostProcessor(player_t *player)
{
	postimg_t *type = &postimgtype[0];
	UINT8 i;

	HWD.pfnUnSetShader();

	for (i = r_splitscreen; i > 0; i--)
	{
		if (player == &players[displayplayers[i]])
		{
			type = &postimgtype[i];
			break;
		}
	}

	// Armageddon Blast Flash!
	// Could this even be considered postprocessor?
	if (player->flashcount)
	{
		FOutVector      v[4];
		FSurfaceInfo Surf;

		v[0].x = v[2].y = v[3].x = v[3].y = -4.0f;
		v[0].y = v[1].x = v[1].y = v[2].x = 4.0f;
		v[0].z = v[1].z = v[2].z = v[3].z = 4.0f; // 4.0 because of the same reason as with the sky, just after the screen is cleared so near clipping plane is 3.99

		// This won't change if the flash palettes are changed unfortunately, but it works for its purpose
		if (player->flashpal == PAL_NUKE)
		{
			Surf.PolyColor.s.red = 0xff;
			Surf.PolyColor.s.green = Surf.PolyColor.s.blue = 0x7F; // The nuke palette is kind of pink-ish
		}
		else
			Surf.PolyColor.s.red = Surf.PolyColor.s.green = Surf.PolyColor.s.blue = 0xff;

		Surf.PolyColor.s.alpha = 0xc0; // match software mode

		HWD.pfnDrawPolygon(&Surf, v, 4, PF_Modulated|PF_Translucent|PF_NoTexture|PF_NoDepthTest);
	}

	// Capture the screen for intermission and screen waving
	if(gamestate != GS_INTERMISSION)
		HWD.pfnMakeScreenTexture();

	if (r_splitscreen) // Not supported in splitscreen - someone want to add support?
		return;

	// Drunken vision! WooOOooo~
	if (*type == postimg_water || *type == postimg_heat)
	{
		// 10 by 10 grid. 2 coordinates (xy)
		float v[SCREENVERTS][SCREENVERTS][2];
		double disStart = leveltime;
		UINT8 x, y;
		INT32 WAVELENGTH;
		INT32 AMPLITUDE;
		INT32 FREQUENCY;

		// Modifies the wave.
		if (*type == postimg_water)
		{
			WAVELENGTH = 20; // Lower is longer
			AMPLITUDE = 20; // Lower is bigger
			FREQUENCY = 16; // Lower is faster
		}
		else
		{
			WAVELENGTH = 10; // Lower is longer
			AMPLITUDE = 30; // Lower is bigger
			FREQUENCY = 4; // Lower is faster
		}

		for (x = 0; x < SCREENVERTS; x++)
		{
			for (y = 0; y < SCREENVERTS; y++)
			{
				// Change X position based on its Y position.
				v[x][y][0] = (x/((float)(SCREENVERTS-1.0f)/9.0f))-4.5f + (float)sin((disStart+(y*WAVELENGTH))/FREQUENCY)/AMPLITUDE;
				v[x][y][1] = (y/((float)(SCREENVERTS-1.0f)/9.0f))-4.5f;
			}
		}
		HWD.pfnPostImgRedraw(v);

		// Capture the screen again for screen waving on the intermission
		if(gamestate != GS_INTERMISSION)
			HWD.pfnMakeScreenTexture();
	}
	// Flipping of the screen isn't done here anymore
}

void HWR_StartScreenWipe(void)
{
	HWD.pfnStartScreenWipe();
}

void HWR_EndScreenWipe(void)
{
<<<<<<< HEAD
=======
	//CONS_Debug(DBG_RENDER, "In HWR_EndScreenWipe()\n");
>>>>>>> srb2/next
	HWD.pfnEndScreenWipe();
}

void HWR_DrawIntermissionBG(void)
{
	HWD.pfnDrawIntermissionBG();
}

//
// hwr mode wipes
//
static lumpnum_t wipelumpnum;

// puts wipe lumpname in wipename[9]
static boolean HWR_WipeCheck(UINT8 wipenum, UINT8 scrnnum)
{
	static char lumpname[9] = "FADEmmss";
	size_t lsize;

	// not a valid wipe number
	if (wipenum > 99 || scrnnum > 99)
		return false; // shouldn't end up here really, the loop should've stopped running beforehand

	// puts the numbers into the wipename
	lumpname[4] = '0'+(wipenum/10);
	lumpname[5] = '0'+(wipenum%10);
	lumpname[6] = '0'+(scrnnum/10);
	lumpname[7] = '0'+(scrnnum%10);
	wipelumpnum = W_CheckNumForName(lumpname);

	// again, shouldn't be here really
	if (wipelumpnum == LUMPERROR)
		return false;

	lsize = W_LumpLength(wipelumpnum);
	if (!(lsize == 256000 || lsize == 64000 || lsize == 16000 || lsize == 4000))
	{
		CONS_Alert(CONS_WARNING, "Fade mask lump %s of incorrect size, ignored\n", lumpname);
		return false; // again, shouldn't get here if it is a bad size
	}

<<<<<<< HEAD
	HWR_GetFadeMask(lumpnum);
	HWD.pfnDoScreenWipe();
=======
	return true;
}

void HWR_DoWipe(UINT8 wipenum, UINT8 scrnnum)
{
	if (!HWR_WipeCheck(wipenum, scrnnum))
		return;

	HWR_GetFadeMask(wipelumpnum);
	HWD.pfnDoScreenWipe();
}

void HWR_DoTintedWipe(UINT8 wipenum, UINT8 scrnnum)
{
	// It does the same thing
	HWR_DoWipe(wipenum, scrnnum);
>>>>>>> srb2/next
}

void HWR_MakeScreenFinalTexture(void)
{
    HWD.pfnMakeScreenFinalTexture();
}

void HWR_DrawScreenFinalTexture(int width, int height)
{
    HWD.pfnDrawScreenFinalTexture(width, height);
}

// jimita 18032019
typedef struct
{
	char type[16];
	INT32 id;
} shaderxlat_t;

static inline UINT16 HWR_CheckShader(UINT16 wadnum)
{
	UINT16 i;
	lumpinfo_t *lump_p;

	lump_p = wadfiles[wadnum]->lumpinfo;
	for (i = 0; i < wadfiles[wadnum]->numlumps; i++, lump_p++)
		if (memcmp(lump_p->name, "SHADERS", 7) == 0)
			return i;

	return INT16_MAX;
}

void HWR_LoadShaders(UINT16 wadnum, boolean PK3)
{
	UINT16 lump;
	char *shaderdef, *line;
	char *stoken;
	char *value;
	size_t size;
	int linenum = 1;
	int shadertype = 0;
	int i;

	#define SHADER_TYPES 7
	shaderxlat_t shaderxlat[SHADER_TYPES] =
	{
		{"Flat", 1},
		{"WallTexture", 2},
		{"Sprite", 3},
		{"Model", 4},
		{"WaterRipple", 5},
		{"Fog", 6},
		{"Sky", 7},
	};

	lump = HWR_CheckShader(wadnum);
	if (lump == INT16_MAX)
		return;

	shaderdef = W_CacheLumpNumPwad(wadnum, lump, PU_CACHE);
	size = W_LumpLengthPwad(wadnum, lump);

	line = Z_Malloc(size+1, PU_STATIC, NULL);
	if (!line)
		I_Error("HWR_LoadShaders: No more free memory\n");

	M_Memcpy(line, shaderdef, size);
	line[size] = '\0';

	stoken = strtok(line, "\r\n ");
	while (stoken)
	{
		if ((stoken[0] == '/' && stoken[1] == '/')
			|| (stoken[0] == '#'))// skip comments
		{
			stoken = strtok(NULL, "\r\n");
			goto skip_field;
		}

		if (!stricmp(stoken, "GLSL"))
		{
			value = strtok(NULL, "\r\n ");
			if (!value)
			{
				CONS_Alert(CONS_WARNING, "HWR_LoadShaders: Missing shader type (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				stoken = strtok(NULL, "\r\n"); // skip end of line
				goto skip_lump;
			}

			if (!stricmp(value, "VERTEX"))
				shadertype = 1;
			else if (!stricmp(value, "FRAGMENT"))
				shadertype = 2;

skip_lump:
			stoken = strtok(NULL, "\r\n ");
			linenum++;
		}
		else
		{
			value = strtok(NULL, "\r\n= ");
			if (!value)
			{
				CONS_Alert(CONS_WARNING, "HWR_LoadShaders: Missing shader target (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				stoken = strtok(NULL, "\r\n"); // skip end of line
				goto skip_field;
			}

			if (!shadertype)
			{
				CONS_Alert(CONS_ERROR, "HWR_LoadShaders: Missing shader type (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				Z_Free(line);
				return;
			}

			for (i = 0; i < SHADER_TYPES; i++)
			{
				if (!stricmp(shaderxlat[i].type, stoken))
				{
					size_t shader_size;
					char *shader_source;
					char *shader_lumpname;
					UINT16 shader_lumpnum;

					if (PK3)
					{
						shader_lumpname = Z_Malloc(strlen(value) + 12, PU_STATIC, NULL);
						strcpy(shader_lumpname, "Shaders/sh_");
						strcat(shader_lumpname, value);
						shader_lumpnum = W_CheckNumForFullNamePK3(shader_lumpname, wadnum, 0);
					}
					else
					{
						shader_lumpname = Z_Malloc(strlen(value) + 4, PU_STATIC, NULL);
						strcpy(shader_lumpname, "SH_");
						strcat(shader_lumpname, value);
						shader_lumpnum = W_CheckNumForNamePwad(shader_lumpname, wadnum, 0);
					}

					if (shader_lumpnum == INT16_MAX)
					{
						CONS_Alert(CONS_ERROR, "HWR_LoadShaders: Missing shader source %s (file %s, line %d)\n", shader_lumpname, wadfiles[wadnum]->filename, linenum);
						Z_Free(shader_lumpname);
						continue;
					}

					shader_size = W_LumpLengthPwad(wadnum, shader_lumpnum);
					shader_source = Z_Malloc(shader_size, PU_STATIC, NULL);
					W_ReadLumpPwad(wadnum, shader_lumpnum, shader_source);

					HWD.pfnLoadCustomShader(shaderxlat[i].id, shader_source, shader_size, (shadertype == 2));

					Z_Free(shader_source);
					Z_Free(shader_lumpname);
				}
			}

skip_field:
			stoken = strtok(NULL, "\r\n= ");
			linenum++;
		}
	}

	HWD.pfnInitCustomShaders();

	Z_Free(line);
	return;
}

#endif // HWRENDER
