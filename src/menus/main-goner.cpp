// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/main-goner.cpp
/// \brief The Missing Setup.

#include "../k_menu.h"
#include "../m_cond.h"
#include "../r_skins.h"
#include "../s_sound.h"
#include "../f_finale.h"
#include "../music.h"
#include "../p_local.h" // P_AutoPause
#include "../st_stuff.h" // faceprefix
#include "../v_draw.hpp"
#include "../k_dialogue.hpp"
#include "../m_random.h"
#include "../r_main.h"
#include "../m_easing.h"
#include "../g_input.h"
#include "../m_pw.h"
#include "../z_zone.h"

#include <deque>

#include "../core/string.h"

static void M_GonerDrawer(void);
static void M_GonerChoiceDrawer(void);
static void M_GonerConclude(INT32 choice);
static boolean M_GonerInputs(INT32 ch);

static menuitem_t MAIN_GonerAccessibility[] =
{
	{IT_NOTHING, NULL, NULL, NULL, {NULL}, 0, 0},
};

static UINT32 goneraccessibilitytick = 0;

//#define HANDSTRAIN

#ifdef HANDSTRAIN
static void M_GonerHandStrain(INT32 ch)
{
	if (ch != MA_YES)
		return;

	CV_StealthSet(&cv_kickstartaccel[0], "On");
}
#endif

static void M_GonerPhotosensitivity(INT32 ch)
{
	// if (ch == MA_YES)
	// {
	// 	CV_StealthSet(&cv_reducevfx, "Yes");
	// 	CV_StealthSet(&cv_screenshake, "Off");
	// 	CV_StealthSet(&cv_tilting, "Off");
	// }

#ifdef HANDSTRAIN
	M_StartMessage("Hand strain warning",
		"You may be required to press many buttons\n"
		"at once in order to control your Ring Racer.\n"
		"\n"
		"There is an option for your Accel input\n"
		"to \"lock\" on after being held for 1 second.\n"
		"Would you like to turn it on?\n"
		, &M_GonerHandStrain, MM_YESNO, "Yes, I want Accel to \"lock\"", "No thanks");
#endif
}

static void M_GonerAccessibilityTick(void)
{
	if (goneraccessibilitytick)
	{
		if (!menumessage.active && !menutransition.dest)
		{
			M_SetupNextMenu(&MAIN_GonerDef, true);
			M_GonerTick(); // tick once, for safety
		}

		return;
	}

	goneraccessibilitytick++;

	M_StartMessage("Photosensitivity warning",
		"This game has ""\x87""flashing lights and high-contrast\n"
		"patterns.""\x80"" Listen to your body, and\n"
		"stop playing if you feel unwell.\n"
		"\n"
		"There is a ""\x88""special mode""\x80""\n"
		"to reduce some visual effects.\n"
		"\n"
		"You can turn it on within the\n"
		"Profile Setup > Accessibility menu.\n"
		, &M_GonerPhotosensitivity, MM_NOTHING, NULL, NULL);
	return;
}

static void M_GonerAccessibilityDrawer(void)
{
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
}

menu_t MAIN_GonerAccessibilityDef = {
	sizeof (MAIN_GonerAccessibility) / sizeof (menuitem_t),
	NULL,
	0,
	MAIN_GonerAccessibility,
	26, 160,
	0, 0,
	MBF_CANTRESTORE,
	"_GONER",
	0, 0,
	M_GonerAccessibilityDrawer,
	NULL,
	M_GonerAccessibilityTick,
	NULL,
	NULL,
	NULL,
};

static menuitem_t MAIN_Goner[] =
{
	{IT_STRING | IT_CVAR | IT_CV_STRING, "PASSWORD",
		"ATTEMPT ADMINISTRATOR ACCESS.", NULL,
		srb2::itemaction(& cv_dummyextraspassword), 0, 0},

	{IT_STRING | IT_CALL, "EXIT PROGRAM",
		"CONCLUDE OBSERVATIONS NOW.", NULL,
		srb2::itemaction(M_QuitSRB2), 0, 0},

	{IT_STRING | IT_CALL, "VIDEO OPTIONS",
		"CONFIGURE OCULAR PATHWAYS.", NULL,
		srb2::itemaction(M_VideoOptions), 0, 0},

	{IT_STRING | IT_CALL, "SOUND OPTIONS",
		"CALIBRATE AURAL DATASTREAM.", NULL,
		srb2::itemaction(M_SoundOptions), 0, 0},

	{IT_STRING | IT_CALL, "PROFILE SETUP",
		"ASSIGN VEHICLE INPUTS.", NULL,
		srb2::itemaction(M_GonerProfile), 0, 0},

	{IT_STRING | IT_CALL, "MAKE CHOICE",
		"PREPARE FOR INTEGRATION?", NULL,
		srb2::itemaction(M_GonerChoice), 0, 0},

	{IT_STRING | IT_CALL, "START GAME",
		"I WILL SUCCEED.", NULL,
		srb2::itemaction(M_GonerConclude), 0, 0},
};

menu_t MAIN_GonerDef = {
	2,
	NULL,
	1,
	MAIN_Goner,
	26, 160,
	0, sizeof (MAIN_Goner) / sizeof (menuitem_t), // extra2 is final width
	MBF_CANTRESTORE|MBF_UD_LR_FLIPPED,
	"_GONER",
	0, 0,
	M_GonerDrawer,
	M_DrawGonerBack,
	M_GonerTick,
	NULL,
	NULL,
	M_GonerInputs,
};

static menuitem_t MAIN_GonerChoice[] =
{
	{IT_STRING | IT_CALL, "Tails' way",
		"As a child scientist, Tails has recorded bits\n"
		"and pieces of an adventure he and Eggman went\n"
		"on while trying out their new Ring Racers.\n"
		"\n"
		"This is a structured, back-to-basics tutorial\n"
		"that will likely take ""\x88""10-20 minutes""\x80"" of your time.",
		NULL, srb2::itemaction(M_GonerTutorial), 0, 0},

	//{IT_STRING, NULL, NULL, NULL, {.routine = M_QuitSRB2}, 0, 0}, // will be replaced

	{IT_STRING | IT_CALL, "Eggman's way",
		"As a childlike scientist, Eggman has turned the\n"
		"wrecked Egg Carrier into a giant skatepark,\n"
		"dotted with fun collectables to test drivers.\n"
		"\n"
		"You can ""\x88""exit immediately""\x80"" and get to racing...\n"
		"or spend ""\x88""as long as you want""\x80"" in the playground!",
		NULL, srb2::itemaction(M_GonerPlayground), 0, 0},
};

static menu_t MAIN_GonerChoiceDef = {
	sizeof (MAIN_GonerChoice) / sizeof (menuitem_t),
	&MAIN_GonerDef,
	0,
	MAIN_GonerChoice,
	26, 160,
	0, 0,
	MBF_CANTRESTORE|MBF_UD_LR_FLIPPED|MBF_NOLOOPENTRIES,
	"_GONER",
	0, 0,
	M_GonerChoiceDrawer,
	M_DrawGonerBack,
	NULL,
	NULL,
	NULL,
	NULL,
};

namespace
{

typedef enum
{
	GONERCHOICE_TAILS = 0,
	//GONERCHOICE_NONEBINEY,
	GONERCHOICE_EGGMAN
} gonerchoices_t;

typedef enum
{
	GONERSPEAKER_EGGMAN = 0,
	GONERSPEAKER_TAILS,
	MAXGONERSPEAKERS
} gonerspeakers_t;

class GonerSpeaker
{
public:
	float offset = 0;

	GonerSpeaker(const srb2::String& skinName, float offset)
	{
		if (!skinName.empty())
		{
			this->skinID = R_SkinAvailableEx(skinName.c_str(), false);
		}

		this->offset = offset;
	};

	GonerSpeaker() {};

	sfxenum_t TalkSound(void)
	{
		if (!ValidID())
			return sfx_ktalk;

		return skins[ skinID ]
			->soundsid[ S_sfx[sfx_ktalk].skinsound ];
	};

	int GetSkinID(void)
	{
		if (!ValidID())
			return -1;

		return skinID;
	};

private:
	int skinID = -1;
	bool ValidID(void)
	{
		return (skinID >= 0 && skinID < numskins);
	};
};

std::array<GonerSpeaker, MAXGONERSPEAKERS> goner_speakers = {};

srb2::Dialogue::Typewriter goner_typewriter;

int goner_delay;
int goner_scroll = 0;
int goner_scrollend = -1;

class GonerChatLine
{
public:
	gonerspeakers_t speaker;
	srb2::String dialogue;
	int value; // Mutlipurpose.
	void (*routine)(void);

	GonerChatLine(gonerspeakers_t speaker, int delay, const srb2::String& dialogue)
	{
		char *newText = V_ScaledWordWrap(
			(BASEVIDWIDTH/2 + 6) << FRACBITS,
			FRACUNIT, FRACUNIT, FRACUNIT,
			0, TINY_FONT,
			dialogue.c_str()
		);

		this->speaker = speaker;
		this->dialogue = srb2::String(newText);
		this->value = delay;

		this->routine = nullptr;

		Z_Free(newText);
	};

	GonerChatLine(int delay, void (*routine)(void))
	{
		this->value = delay;
		this->routine = routine;

		this->speaker = MAXGONERSPEAKERS;
		this->dialogue = "";
	}

	// Returns true if line is text
	bool Handle(void)
	{
		goner_delay = value;

		if (routine != nullptr)
			routine();

		if (speaker >= MAXGONERSPEAKERS || dialogue.empty())
			return false;

		goner_typewriter.voiceSfx = goner_speakers[speaker].TalkSound();

		goner_typewriter.NewText(dialogue);

		value = 1; // this is now repurposed as the number of lines visible

		goner_scrollend++;

		return true;
	};
};

std::deque<GonerChatLine> LinesToDigest;
std::deque<GonerChatLine> LinesOutput;

class GonerBGData
{
public:
	bool miles_electric;
	fixed_t miles_electric_delta;
	bool miles_cameralook, miles_pendinglook;
	int miles_timetoblink, miles_next_timetoblink;

	gonerspeakers_t focuslast;
	fixed_t focusdelta;

	int focusmouth;

	fixed_t x, focusx;
	fixed_t y, focusy;

	int darkframes;

	GonerBGData()
	{
		x = focusx = 0;
		y = focusy = 20*FRACUNIT;

		darkframes = 0;

		focuslast = MAXGONERSPEAKERS;
		focusdelta = 0;

		focusmouth = 1;

		miles_electric = true;
		miles_electric_delta = FRACUNIT;
		miles_cameralook = false;
		miles_timetoblink = M_RandomRange(2*TICRATE, 4*TICRATE);
		miles_next_timetoblink = (4*TICRATE + 10) - miles_timetoblink;
	};

	bool NeutralMouthCheck()
	{
		return (currentMenu != &MAIN_GonerDef
			|| LinesOutput.empty()
			|| goner_typewriter.textDone
			|| goner_typewriter.text.empty());
	}

	fixed_t GetX()
	{
		return Easing_InOutCubic(focusdelta, focusx, x);
	}

	fixed_t GetY()
	{
		return Easing_InOutCubic(focusdelta, focusy, y);
	}

	void Tick()
	{
		// This is the visual feed sputtering in and out.
		if (darkframes)
			darkframes--;
		else if (gamedata->gonerlevel > GDGONER_VIDEO)
		{
			// Everything in here is Metal Sonic's response to stimulus.

			gonerspeakers_t focuscurrent = MAXGONERSPEAKERS;
			if (currentMenu == &MAIN_GonerDef && !LinesOutput.empty())
			{
				focuscurrent = LinesOutput.front().speaker;
			}

			if (focuslast != focuscurrent)
			{
				x = GetX();
				y = GetY();
				focusdelta = FRACUNIT;

				switch (focuscurrent)
				{
					case GONERSPEAKER_TAILS:
						focusx = -10*FRACUNIT;
						focusy = 0;
						break;
					case GONERSPEAKER_EGGMAN:
						focusx = 10*FRACUNIT;
						focusy = 0;
						break;
					default:
						focusx = 0;
						focusy = 20*FRACUNIT;
						break;
				}

				focuslast = focuscurrent;
			}
		}

		// Everything below this is the real world.

		if (miles_timetoblink == 0)
		{
			if (miles_next_timetoblink)
			{
				miles_timetoblink = miles_next_timetoblink;
				miles_next_timetoblink = 0;
			}
			else
			{
				miles_timetoblink = M_RandomRange(2*TICRATE, 4*TICRATE);
				miles_next_timetoblink = (4*TICRATE + 10) - miles_timetoblink;
			}
		}
		else if (miles_timetoblink == 5)
		{
			miles_cameralook = miles_pendinglook;
		}
		miles_timetoblink--;

		if (NeutralMouthCheck())
			focusmouth = 1;
		else
		{
			// The following is loosely based on code by Tyron,
			// generously donated from an unreleased SRB2Kart mod.

			char c = tolower(goner_typewriter.text.back());
			char incomingc = goner_typewriter.textDest.empty()
				? '\0'
				: tolower(goner_typewriter.textDest.back());
			switch (c)
			{
				// Close mouth
				case 'm':
				case 'w':
				case 'p':
				case 'b':
					focusmouth = 1;
					break;

				// Vowels
				case 'a': focusmouth = 2; break;
				case 'e': focusmouth = 3; break;
				case 'i': focusmouth = 4; break;
				case 'o': focusmouth = 5; break;
				case 'u': focusmouth = 6; break;

				// VOWELBIGUOUS
				case 'y': focusmouth = 7; break;

				// Hissth
				case 't':
				case 's':
				case 'r':
				case 'n':
					focusmouth = 7; break;

				// Approximation, since MS-1 is said a LOT by Tails.
				case '-':
					if (incomingc != '1')
						break;
					focusmouth = 5; break;
				case '1': focusmouth = 7; break;

				// Conclude dialogue
				case '.':
				case '!':
				case ',':
				case ':':
				case ';':
					focusmouth = 0; break;

				// No update for you!
				default:
					break;
			}
		}
	}
};

GonerBGData goner_background;

void Miles_SetPendingLook(bool set)
{
	if (goner_background.miles_pendinglook == set)
		return;

	goner_background.miles_pendinglook = set;

	if (goner_background.miles_timetoblink > 10)
	{
		goner_background.miles_timetoblink = 10;
		goner_background.miles_next_timetoblink = 0;
	}
	else if (goner_background.miles_timetoblink < 5)
		goner_background.miles_next_timetoblink = 10;
}

void Miles_SetElectric(bool set)
{
	if (goner_background.miles_electric == set)
		return;

	goner_background.miles_electric = set;
	goner_background.miles_electric_delta =
		FRACUNIT - goner_background.miles_electric_delta;
}

void Miles_Look_Camera()
{
	Miles_SetPendingLook(true);
}

void Miles_Look_Electric()
{
	Miles_SetElectric(true);
	Miles_SetPendingLook(false);
}

void Miles_Electric_Lower()
{
	Miles_SetElectric(false);
	Miles_Look_Camera();
}

int goner_levelworking = GDGONER_INIT;
bool goner_gdq = false;

static void Initial_Control_Info(void)
{
	if (cv_currprofile.value != -1)
		return;

	auto line = GonerChatLine(GONERSPEAKER_TAILS, 0,
		va("You should be able to use ""\x86""%s""\x80"" to operate this menu.",
			(!G_GetNumAvailableGamepads()
				? "Enter, ESC, and the Arrow Keys"
				: "your Gamepad"
			)
		)
	);

	LinesToDigest.push_back(line);
}

void M_AddGonerLines(void)
{
	SRB2_ASSERT(LinesToDigest.empty());

	static bool leftoff = false;

	goner_delay = TICRATE;

	// This one always plays, so it checks the levelworking instead of gamedata.
	if (goner_levelworking == GDGONER_INTRO)
	{
		if (!MAIN_Goner[0].mvar2)
		{
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
				"Metal Sonic. Are you online?");
		}

		leftoff = (goner_levelworking < gamedata->gonerlevel-1);

		if (leftoff)
		{
			LinesToDigest.emplace_back(0, Miles_Look_Camera);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"It must have run into some sort of error...");
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
				"Don't worry, your settings so far are saved. "\
				"Let's pick up where we left off.");

			// the -1 guarantees that one will be (re)played
			goner_levelworking = gamedata->gonerlevel-1;
		}

		return;
	}

	switch (gamedata->gonerlevel)
	{
		case GDGONER_VIDEO:
		{
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/2,
				"Take a close look, Miles. Moments ago he was at my throat! "\
				"Now he's docile as can be on that operating table.");

			LinesToDigest.emplace_back(0, Miles_Look_Camera);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"I don't feel very safe!");
			LinesToDigest.emplace_back(0, Miles_Electric_Lower);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/4,
				"But its programming is definitely locked down...");

			LinesToDigest.emplace_back(0, Miles_Look_Electric);
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
				"You've given me quite the headache, Metal. "\
				"Thankfully, Tails caught you in the act.");

			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/5,
				"Wait, I'm getting weird readings over the network.");
			LinesToDigest.emplace_back(0, Miles_Look_Camera);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"Metal Sonic is the unit labeled \"MS-1\", right?");
			LinesToDigest.emplace_back(0, Miles_Look_Electric);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE,
				"The ""\x87""viewport""\x80"" and ""\x87""audio""\x80"" "\
				"config looks like it got messed up.");

			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
				"So you're right. I wonder if it has anything to do with that outburst.");
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
				"Alright, Metal! I don't remember your specifications offhand. "\
				"First things first, go ahead and set up your "\
				"\x87""Video Options""\x80"" yourself.");

			LinesToDigest.emplace_back(0, Initial_Control_Info);

			break;
		}
		case GDGONER_SOUND:
		{
			if (!leftoff)
			{
				LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
					"Ah, you can see us now. Good.");
			}
			LinesToDigest.emplace_back(0, Miles_Look_Camera);
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
				"Now, calibrate your ""\x87""Sound Options""\x80"".");

			LinesToDigest.emplace_back(0, Miles_Electric_Lower);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"You always make your stuff so loud by default, Eggman. It might need a moment.");

			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
				"Not Metal! He always needed to be stealthy. But go on, set your sliders.");
			LinesToDigest.emplace_back(0, Miles_Look_Electric);
			break;
		}
		case GDGONER_PROFILE:
		{
			if (!leftoff)
			{
				LinesToDigest.emplace_back(0, Miles_Look_Electric);
				LinesToDigest.emplace_back(0, Miles_Look_Camera);
				LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/2,
					"Oh! Let's tell Metal about our project!");

				LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
					"Of course. I and my lab assista-");

				LinesToDigest.emplace_back(0, Miles_Electric_Lower);
				LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
					"Lab PARTNER.");

				LinesToDigest.emplace_back(0, Miles_Look_Electric);
				LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
					"Irrelevant!");
			}
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/4,
				"We made a machine together, Tails and I. "\
				"It's called a \"""\x82""Ring Racer""\x80""\".");
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE,
				"At its core, it is designed to utilise the boundless potential "\
				"of the ""\x83""High Voltage Ring""\x80"".");

			LinesToDigest.emplace_back(0, Miles_Look_Camera);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE,
				"We made this special ""\x83""Ring""\x80"" by combining the power of tens of "\
				"thousands of ordinary ""\x82""Rings""\x80"".");
			LinesToDigest.emplace_back(0, Miles_Electric_Lower);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/2,
				"We recorded some of our testing for you, MS-1. Maybe your neural "\
				"network could train on some less violent data for once.");

			LinesToDigest.emplace_back(0, Miles_Look_Electric);
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/4,
				"While that's uploading, why don't you set up your ""\x87""Profile Card""\x80""?");

			LinesToDigest.emplace_back(0, Miles_Electric_Lower);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"Yes! That's one of my contributions.");

			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
				"(I'm too used to my systems being designed for me alone...)");

			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"Every racer carries one, to contain their personal settings.");
			LinesToDigest.emplace_back(0, Miles_Look_Electric);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"It helps get your ""\x87""controls""\x80"" set up nice and quickly, "\
				"when starting your vehicle and navigating the menu.");
			LinesToDigest.emplace_back(0, Miles_Look_Camera);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"And it helps track your wins, too.");
			LinesToDigest.emplace_back(0, Miles_Look_Electric);

			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/5,
				"Bragging rights. My idea!");

			LinesToDigest.emplace_back(0, Miles_Look_Camera);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/2,
				"You can make the ID and player tag on there anything you want.");
			LinesToDigest.emplace_back(0, Miles_Electric_Lower);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/2,
				"Mine says \"Nine Tails\". That's the name of my original character! "\
				"He's like me if I never met my ""\x84""brother""\x80"". He'd use cool "\
				"robotics, but be kind of mean to protect himself...");

			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/5,
				"Mine says \"Robotnik\". You can't beat a classic.");

			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/2,
				"And I'm not sure if you'll need it, but we always tell new drivers to "\
				"look at the ""\x87""Accessibility""\x80"" settings. Often there's some "\
				"feature they're not expecting. Maybe you'd be surprised too?");

			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"So go on, do your ""\x87""Profile Setup""\x80""!");

			break;
		}
		case GDGONER_TUTORIAL:
		{
			if (!leftoff)
			{
				LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/2,
					"Now that that's been set up, you can use your ""\x87""Profile controls""\x80"" on menus from here on out, too.");

				LinesToDigest.emplace_back(0, Miles_Look_Electric);
				LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/5,
					"Miles. How's the upload going?");

				LinesToDigest.emplace_back(0, Miles_Look_Camera);
				LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
					"Just finished.");

				LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
					"Perfect.");
			}

			LinesToDigest.emplace_back(0, Miles_Electric_Lower);
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
				"Now, Metal... it's important you pay attention.");
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/5,
				"We have a ""\x88""choice""\x80"" ready for you.");

			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"You can play back our testing data as a sort of ""\x82""tutorial""\x80"\
				" and learn the core parts of driving in a safe environment...");

			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/5,
				"...or if you're too headstrong and want to figure things out"\
				" for yourself, we can let you loose in our ""\x85""playground""\x80""!");
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/2,
				"If you do run into trouble, the ""\x82""tutorial""\x80"" can"\
				" always be found in the ""\x87""Extras""\x80"" menu later on.");

			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"Either way, MS-1. Even when you move on from this setup,"\
				" you can always change your ""\x87""Options""\x80"" at any time.");
			LinesToDigest.emplace_back(0, Miles_Look_Electric);

			break;
		}
		case GDGONER_OUTRO:
		{
			if (!leftoff)
			{
				LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/3,
					"And... the training data is completed.");
			}
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/2,
				"It's kind of funny, actually.");
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/3,
				"Oh? Care to elucidate, Prower?");
			LinesToDigest.emplace_back(0, Miles_Look_Camera);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/2,
				"No matter how much time we took getting here, a machine like "\
				"Metal can play it back in minutes.");
			LinesToDigest.emplace_back(0, Miles_Electric_Lower);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/2,
				"It could have been five days or five years of development on "\
				"our ""\x82""Ring Racers""\x80"", and that would barely matter to it.");
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/4,
				"Ha! As if. I'd like to think our partnership hasn't felt "\
				"particularly protracted.");
			LinesToDigest.emplace_back(0, Miles_Look_Electric);
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/2,
				"But yes. Perhaps now you have a better appreciation of what "\
				"we're building here, Metal.");
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/5,
				"Now, I'm willing to let bygones be bygones.");
			LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/2,
				"As long as you keep your violence to the track, I'll be "\
				"giving you your autonomy back in a moment.");
			LinesToDigest.emplace_back(0, Miles_Electric_Lower);
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"We've kept the keys from you long enough!");
			break;
		}
		case GDGONER_DONE:
			break;

		default:
			LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
				"I am error");
	}

	leftoff = false;
}

gdgoner_t goner_lasttypelooking = GDGONER_INIT;
tic_t goner_youactuallylooked = 0;

void M_GonerRailroad(bool set)
{
	INT16 destsize = std::min(
		static_cast<INT16>(
			(set ? 2 : 1) // Quit + options + maybe 1 for extra access
			+ 1
			+ std::max(0, gamedata->gonerlevel - GDGONER_VIDEO)
		),
		currentMenu->extra2);
	currentMenu->numitems = destsize;

	if (!set)
		return;

	itemOn = destsize-1;
	S_StartSound(NULL, sfx_s3k63);
}

void M_GonerHidePassword(void)
{
	if (MAIN_Goner[0].mvar2)
		return;

#if 0
	MAIN_Goner[0] =
		{IT_STRING | IT_CALL, "EXIT PROGRAM",
			"CONCLUDE OBSERVATIONS NOW.", NULL,
			{.routine = M_QuitSRB2}, 0, 1};

	S_StartSound(NULL, sfx_s3k5b);
#else
	// Always keep the password field, but play menu jam only once.
	MAIN_Goner[0].mvar2 = 1;
#endif

	M_PlayMenuJam();
}

}; // namespace

void M_GonerResetLooking(int type)
{
	if (type == GDGONER_VIDEO)
		OPTIONS_MainDef.lastOn = mopt_video;
	else if (type == GDGONER_SOUND)
		OPTIONS_MainDef.lastOn = mopt_sound;
	else if (type == GDGONER_PROFILE)
		OPTIONS_MainDef.lastOn = mopt_profiles;
	else if (goner_youactuallylooked > 2*TICRATE
	&& goner_lasttypelooking == gamedata->gonerlevel)
	{
		gamedata->gonerlevel++;
		LinesToDigest.clear();
	}

	goner_lasttypelooking = static_cast<gdgoner_t>(type);
	goner_youactuallylooked = 0;
}

boolean M_GonerMusicPlayable(void)
{
	if (!MAIN_Goner[0].mvar2)
		return false;

	if (currentMenu == &OPTIONS_SoundDef)
		return true;

	if (gamedata->gonerlevel <= GDGONER_SOUND)
		return false;

	return true;
}

void M_GonerCheckLooking(void)
{
	if (goner_lasttypelooking != gamedata->gonerlevel)
		return;
	goner_youactuallylooked++;
}

void M_GonerBGTick(void)
{
	// Laundering CPP code through C-callable funcs ~toast 171223
	goner_background.Tick();
}

void M_GonerBGImplyPassageOfTime(void)
{
	goner_background = GonerBGData();
}

void M_GonerTick(void)
{
	static bool first = true;
	static int lastseenlevel = GDGONER_INIT;

	if (goner_levelworking == GDGONER_INIT)
	{
		first = true;

		// Init.
		goner_speakers[GONERSPEAKER_EGGMAN] = GonerSpeaker("eggman", 0);
		goner_speakers[GONERSPEAKER_TAILS] = GonerSpeaker("tails", 6);
	}
	else if (gamedata->gonerlevel == GDGONER_INIT)
	{
		first = true; // a lie, but only slightly...

		// Handle rewinding if you clear your gamedata.
		M_GonerResetText(true);
	}

	M_GonerResetLooking(GDGONER_INIT);

	if (first)
	{
		goner_background = GonerBGData();

		first = goner_gdq = false;

#if 0
		MAIN_Goner[0] =
			{IT_STRING | IT_CVAR | IT_CV_STRING, "PASSWORD",
				"ATTEMPT ADMINISTRATOR ACCESS.", NULL,
				{.cvar = &cv_dummyextraspassword}, 0, 0};
#endif

		MAIN_Goner[0].mvar2 = 0;

		if (gamedata->gonerlevel < GDGONER_INTRO)
			gamedata->gonerlevel = GDGONER_INTRO;

		M_GonerRailroad(false);
		itemOn = 1;

		lastseenlevel = gamedata->gonerlevel;
	}
	else if (gamedata->gonerlevel != lastseenlevel)
	{
		if (goner_levelworking >= gamedata->gonerlevel)
		{
			// If the valid range has changed, try the current one again
			goner_levelworking--;
		}

		lastseenlevel = gamedata->gonerlevel;
	}

	goner_typewriter.WriteText();

	if (menutyping.active || menumessage.active || P_AutoPause())
		return;

	if (cv_dummyextraspassword.string[0] != '\0')
	{
		// Challenges are not interpreted at this stage.
		// See M_ExtraTick for the full behaviour.
		auto result = M_TryPassword(cv_dummyextraspassword.string, false);

#if 0
		if (result != M_PW_EXTRAS)
		{
			if (LinesOutput.empty() && !LinesToDigest.empty())
			{
				// Remove "Metal Sonic. Are you online?"
				LinesToDigest.pop_front();
			}

			goner_delay = 0;
			LinesToDigest.emplace_front(GONERSPEAKER_EGGMAN, TICRATE,
				"Aha! Nice try. You're tricky enough WITHOUT admin access, thank you.");
			M_GonerHidePassword();
		}
#else
		(void)result;
#endif

		CV_StealthSet(&cv_dummyextraspassword, "");
	}

	if (goner_typewriter.textDone)
	{
		if (!LinesOutput.empty())
			M_GonerHidePassword();
		if (goner_delay > 0)
			goner_delay--;
		else if (!LinesToDigest.empty())
		{
			// Only add new lines if the scroll is invalid
			if (!goner_scroll)
			{
				if (!LinesOutput.empty())
				{
					LinesOutput.front().value = goner_typewriter.textLines;
				}

				auto line = LinesToDigest.front();
				if (line.Handle())
					LinesOutput.push_front(line);
				LinesToDigest.pop_front();
			}
		}
		else if (goner_levelworking <= gamedata->gonerlevel)
		{
			if (goner_levelworking == GDGONER_INTRO && gamedata->gonerlevel < GDGONER_VIDEO)
				gamedata->gonerlevel = lastseenlevel = GDGONER_VIDEO;

			if (++goner_levelworking > gamedata->gonerlevel)
			{
				// We've reached the end of the goner text for now.
				M_GonerRailroad(true);
			}
			else if (!goner_gdq)
			{
				M_AddGonerLines();
			}
		}
	}
}

void M_DrawGonerBack(void)
{
	srb2::Draw drawer = srb2::Draw();

	if (gamedata->gonerlevel <= GDGONER_VIDEO)
	{
		drawer
			.width(BASEVIDWIDTH)
			.height(BASEVIDHEIGHT)
			.fill(157);

		drawer
			.xy(10, 10)
			.font(srb2::Draw::Font::kConsole)
			.flags(V_ADD|V_10TRANS)
			.text("NO SIGNAL");
		return;
	}

	drawer
		.width(BASEVIDWIDTH)
		.height(BASEVIDHEIGHT)
		.fill(31);

	if (goner_background.darkframes)
		return;

	drawer = drawer.xy(
		FixedToFloat(goner_background.GetX()),
		FixedToFloat(goner_background.GetY())
	);

	if (goner_background.focusdelta && renderdeltatics <= 2*FRACUNIT)
	{
		goner_background.focusdelta -= renderdeltatics/TICRATE;
		if (goner_background.focusdelta < 0)
			goner_background.focusdelta = 0;
	}

	{
		srb2::Draw eggman = drawer.xy(-60, 20);
		bool eggfocus = (!goner_background.NeutralMouthCheck()
			&& LinesOutput.front().speaker == GONERSPEAKER_EGGMAN);

		// body
		eggman.patch("GON_EB");

		// head
		{
			int mouth = 7; // eggman grins by default
			if (eggfocus && goner_background.focusmouth)
				mouth = goner_background.focusmouth;

			eggman.patch(va("GON_E1H%u", mouth));
		}
	}

	{
		srb2::Draw miles = drawer.xy(205, 45);
		bool milesfocus = (!goner_background.NeutralMouthCheck()
			&& LinesOutput.front().speaker == GONERSPEAKER_TAILS);

		// body
		miles.patch("GON_T_B");

		// head/eyes
		miles.patch(
			va("GON_T1H%u",
			goner_background.miles_cameralook
				? 1
				: 2
			)
		);
		if (goner_background.miles_timetoblink < 10)
		{
			// eyelids 1-01 2-23 3-45 2-67 1-89
			int blink = ((goner_background.miles_timetoblink > 4)
				? 3 - (goner_background.miles_timetoblink - 4)/2
				: 1 + goner_background.miles_timetoblink/2
			);
			miles.patch(va("GON_T1E%u", blink));
		}

		// mouth
		{
			int mouth = 1;
			if (milesfocus && goner_background.focusmouth)
				mouth = goner_background.focusmouth;

			miles.patch(va("GON_T1M%u", mouth));
		}

		// miles electric and hands (and arms..?)
		{
			if (goner_background.miles_electric)
			{
				miles = miles
					.y(FixedToFloat(Easing_InOutBack( // raising requires a little effort
						goner_background.miles_electric_delta,
						20*FRACUNIT,
						0
					)))
					.x(FixedToFloat(Easing_InOutBack(
						goner_background.miles_electric_delta,
						3*FRACUNIT,
						0
					)));
			}
			else
			{
				miles = miles
					.y(FixedToFloat(Easing_OutBack( // dropping requires little effort
						goner_background.miles_electric_delta,
						0,
						20*FRACUNIT
					)))
					.x(FixedToFloat(Easing_OutBack(
						goner_background.miles_electric_delta,
						0,
						3*FRACUNIT
					)));
			}

			bool drawarms = false;

			if (goner_background.miles_electric_delta == FRACUNIT)
			{
				drawarms = (milesfocus && goner_background.miles_electric);
			}
			else
			{
				goner_background.miles_electric_delta += renderdeltatics/(TICRATE/3);
				if (goner_background.miles_electric_delta > FRACUNIT)
					goner_background.miles_electric_delta = FRACUNIT;
			}

			if (drawarms)
			{
				miles.patch("GON_TA1");
				miles
					.flags(V_TRANSLUCENT)
					.patch("GON_TME1");
			}
			else
			{
				miles.patch("GON_TME1");
			}
			miles.patch("GON_THA1");
		}
	}
}

static void M_GonerDrawer(void)
{
	srb2::Draw drawer = srb2::Draw();

	float newy = currentMenu->y - 12;

	boolean first = true;
	int lastspeaker = MAXGONERSPEAKERS;
	int workscroll = goner_scroll;

	if (workscroll)
	{
		float scrollamount = -72; // a bit more than BASEVIDHEIGHT/3

		for (auto & element : LinesOutput)
		{
			if (first)
			{
				scrollamount += goner_typewriter.textLines*12;
				first = false;
			}
			else
			{
				scrollamount += element.value*12;

				if (lastspeaker != element.speaker)
					scrollamount += 2;
			}

			if (!workscroll) break;
			workscroll--;

			lastspeaker = element.speaker;
		}

		if (scrollamount > 0)
			newy += scrollamount;

		first = true;
		lastspeaker = MAXGONERSPEAKERS;
		workscroll = goner_scroll;
	}

	for (auto & element : LinesOutput)
	{
		srb2::String text;
		INT32 flags;

		if (newy < 0) break;

		if (first)
		{
			text = goner_typewriter.text;
			newy -= goner_typewriter.textLines*12;
			first = false;
		}
		else
		{
			text = element.dialogue;
			newy -= element.value*12;

			if (lastspeaker != element.speaker)
				newy -= 2;
		}

		lastspeaker = element.speaker;

		flags = (workscroll == 0)
			? 0
			: V_TRANSLUCENT;
		workscroll--;

		if (newy > BASEVIDHEIGHT) continue;

		auto speaker = goner_speakers[element.speaker];

		srb2::Draw line = drawer
			.xy(BASEVIDWIDTH/4 + speaker.offset + 3, newy)
			.flags(flags);

		line
			.font(srb2::Draw::Font::kThin)
			.text( text.c_str() );

		int skinID = speaker.GetSkinID();
		if (skinID != -1)
		{
			line = line
				.xy(-16, -2)
				.colormap(skinID, static_cast<skincolornum_t>(skins[skinID]->prefcolor));
			if (gamedata->gonerlevel > GDGONER_VIDEO)
				line.patch(faceprefix[skinID][FACE_MINIMAP]);
			else
				line.patch("HUHMAP");
		}
	}

	if (goner_scroll)
	{
		const char *scrolltext = "SCROLL DOWN TO CONTINUE";
		const int width = V_StringWidth(scrolltext, 0) + 2;

		srb2::Draw popup = drawer.xy(BASEVIDWIDTH/2, currentMenu->y - 4 - 9);

		popup
			.xy(-width/2, -1)
			.width(width)
			.height(10)
			.fill(20);
		popup
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kConsole)
			.flags(V_GRAYMAP)
			.text(scrolltext);
	}

	M_DrawHorizontalMenu();
}

static void M_GonerChoiceDrawer(void)
{
	srb2::Draw drawer = srb2::Draw();

	const INT32 lex = (24 + BASEVIDWIDTH/2)/2;

	if (itemOn == GONERCHOICE_TAILS)
	{
		drawer
			.size((BASEVIDWIDTH/2) + 25, BASEVIDHEIGHT)
			.fill(60);

		drawer
			.xy((BASEVIDWIDTH/2) + 40 + 1, 28+3)
			.colormap(SKINCOLOR_ORANGE)
			.flags(V_FLIP)
			.patch("MENUPLTR");

		drawer
			.xy(lex, 28)
			.font(srb2::Draw::Font::kGamemode)
			.align(srb2::Draw::Align::kCenter)
			.text(currentMenu->menuitems[itemOn].text);

		drawer
			.xy(8, 72)
			.font(srb2::Draw::Font::kThin)
			.align(srb2::Draw::Align::kLeft)
			.text(currentMenu->menuitems[itemOn].tooltip);

		drawer
			.xy(lex, 154)
			.font(srb2::Draw::Font::kFreeplay)
			.align(srb2::Draw::Align::kCenter)
			.text("(unlocks 20     )");

		drawer
			.xy(lex, 154+14)
			.font(srb2::Draw::Font::kThin)
			.align(srb2::Draw::Align::kCenter)
			.flags(V_TRANSLUCENT)
			.text("+ more surprises to find");

		drawer
			.xy(lex + 26, 154-4)
			.patch("UN_CHA00");
	}
	else if (itemOn == GONERCHOICE_EGGMAN)
	{
		drawer
			.x((BASEVIDWIDTH/2) - 24)
			.size((BASEVIDWIDTH/2) + 24, BASEVIDHEIGHT)
			.fill(44);

		drawer
			.xy((BASEVIDWIDTH/2) - 40, 28+3)
			.colormap(SKINCOLOR_RED)
			.patch("MENUPLTR");

		drawer
			.xy(BASEVIDWIDTH - lex, 28)
			.font(srb2::Draw::Font::kGamemode)
			.align(srb2::Draw::Align::kCenter)
			.text(currentMenu->menuitems[itemOn].text);

		drawer
			.xy(BASEVIDWIDTH - 8, 72)
			.font(srb2::Draw::Font::kThin)
			.align(srb2::Draw::Align::kRight)
			.text(currentMenu->menuitems[itemOn].tooltip);

		drawer
			.xy(BASEVIDWIDTH - lex, 154)
			.font(srb2::Draw::Font::kFreeplay)
			.align(srb2::Draw::Align::kCenter)
			.text("(unlocks Addons/Online)");

		drawer
			.xy(BASEVIDWIDTH - lex, 154+14)
			.font(srb2::Draw::Font::kThin)
			.align(srb2::Draw::Align::kCenter)
			.flags(V_TRANSLUCENT)
			.text("the other way has these too, just later");
	}

	// Un-highlighteds done this weird way because of GONERCHOICE_NONEBINEY

	if (itemOn != GONERCHOICE_TAILS)
	{
		drawer
			.size(20, BASEVIDHEIGHT)
			.fill(60);

		drawer
			.xy(25, 39)
			.font(srb2::Draw::Font::kFreeplay)
			.align(srb2::Draw::Align::kLeft)
			.text(currentMenu->menuitems[GONERCHOICE_TAILS].text);

		drawer
			.xy(20 - 3 - (skullAnimCounter/5), 39+6)
			.patch("CUPARROW");
	}

	if (itemOn != GONERCHOICE_EGGMAN)
	{
		drawer
			.x(BASEVIDWIDTH - 20)
			.size(20, BASEVIDHEIGHT)
			.fill(44);

		drawer
			.xy(BASEVIDWIDTH - 25, 39)
			.font(srb2::Draw::Font::kFreeplay)
			.align(srb2::Draw::Align::kRight)
			.text(currentMenu->menuitems[GONERCHOICE_EGGMAN].text);

		drawer
			.xy((BASEVIDWIDTH - 20 + 3) + (skullAnimCounter/5), 39+6)
			.flags(V_FLIP)
			.patch("CUPARROW");
	}
}

// ---

void M_GonerProfile(INT32 choice)
{
	(void)choice;

	optionsmenu.profilen = cv_ttlprofilen.value;

	const INT32 maxp = PR_GetNumProfiles();
	if (optionsmenu.profilen > maxp)
		optionsmenu.profilen = maxp;
	else if (optionsmenu.profilen < 1)
	{
		// Assume the first one is what we want..??
		CV_StealthSetValue(&cv_ttlprofilen, 1);
		optionsmenu.profilen = 1;
	}

	M_ResetOptions();

	// This will create a new profile if necessary.
	M_StartEditProfile(MA_YES);
	PR_ApplyProfilePretend(optionsmenu.profilen, 0);

	M_GonerResetLooking(GDGONER_PROFILE);
}

static void M_GonerTutorialResponse(INT32 ch)
{
	if (ch != MA_YES)
		return;

	M_GonerTutorial(0);
}

void M_GonerChoice(INT32 choice)
{
	if (cv_currprofile.value == -1)
	{
		const INT32 maxp = PR_GetNumProfiles();
		INT32 profilen = cv_ttlprofilen.value;
		if (profilen >= maxp)
			profilen = maxp-1;
		else if (profilen < 1)
			profilen = 1;

		PR_ApplyProfile(profilen, 0);
	}

	if (gamedata->gonerlevel >= GDGONER_OUTRO)
	{
		M_StartMessage("First Boot Tutorial",
			"You've already played the Tutorial!\n"
			"Do you want to see it again?",
			&M_GonerTutorialResponse, MM_YESNO, "I'd love to", "Not right now");
		return;
	}

	M_SetupNextMenu(&MAIN_GonerChoiceDef, false);
}

static void M_GonerSurveyResponse(INT32 ch)
{
	if (ch != MA_YES)
		return;

	if (gamedata->gonerlevel < GDGONER_OUTRO)
		gamedata->gonerlevel = GDGONER_OUTRO;

	if (currentMenu == &MAIN_GonerChoiceDef)
	{
		if (itemOn == GONERCHOICE_EGGMAN)
		{
			gamedata->playgroundroute = true;
			gamedata->gonerlevel = GDGONER_DONE;

			F_StartIntro();
			M_ClearMenus(true);
			M_GonerResetText(false);
			return;
		}

		M_GoBack(0);
	}
}

static void M_GonerSurvey(INT32 choice)
{
	(void)choice;

	// The game is incapable of progression, but I can't bring myself to put an I_Error here.
	M_StartMessage("First Boot Error",
		"YOU ACCEPT EVERYTHING THAT\nWILL HAPPEN FROM NOW ON.",
		&M_GonerSurveyResponse, MM_YESNO, "I agree", "Cancel");
}

void M_GonerTutorial(INT32 choice)
{
	(void)choice;

	// Please also see M_LevelSelectInit as called in extras-1.c
	levellist.netgame = false;
	levellist.canqueue = false;
	levellist.levelsearch.checklocked = true;
	levellist.levelsearch.grandprix = false;
	levellist.levelsearch.timeattack = false;

	if (!M_LevelListFromGametype(GT_TUTORIAL) && gamedata->gonerlevel < GDGONER_OUTRO)
	{
		M_GonerSurvey(0);
		return;
	}
}

void M_GonerPlayground(INT32 choice)
{
	(void)choice;

	UINT16 playgroundmap = NEXTMAP_INVALID;
	if (tutorialplaygroundmap)
		playgroundmap = G_MapNumber(tutorialplaygroundmap);

	if (playgroundmap >= nummapheaders)
	{
		M_GonerSurvey(0);
		return;
	}

	multiplayer = true;

	levellist.newgametype = GT_TUTORIAL;
	levellist.netgame = false;
	M_MenuToLevelPreamble(0, false);

	D_MapChange(
		playgroundmap+1,
		GT_TUTORIAL,
		false,
		true,
		0,
		false,
		false
	);

	M_ClearMenus(true);
	restoreMenu = NULL; // Playground Hack

	// need to do all this here because it will skip returning to goner and there are circumstances (game close) where DoCompleted won't be called
	gamedata->gonerlevel = GDGONER_DONE;
	gamedata->playgroundroute = true;
	gamedata->deferredsave = true;
	M_GonerResetText(true);
}

static void M_GonerConclude(INT32 choice)
{
	(void)choice;

	gamedata->gonerlevel = GDGONER_DONE;

	if (gamedata->chaokeys < 20)
		gamedata->chaokeys = 20;

	F_StartIntro();
	M_ClearMenus(true);
	M_GonerResetText(true);
}

void M_GonerGDQ(boolean opinion)
{
	if (currentMenu != &MAIN_GonerDef || goner_gdq == true)
		return;

	LinesToDigest.clear();
	goner_delay = TICRATE/2;
	goner_gdq = true;

	// The mapping of true/false to each opinion is completely
	// arbitrary, and is not a comment on the nature of the Metroid run.

	if (opinion) // Save The Animals
	{
		LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, TICRATE/2,
			"Why wouldn't you save the frames..?");

		LinesToDigest.emplace_back(0, Miles_Look_Camera);
		LinesToDigest.emplace_back(GONERSPEAKER_TAILS, 0,
			"Don't mind him. Good luck on the run!");
		LinesToDigest.emplace_back(0, Miles_Look_Electric);
	}
	else // Save The Frames
	{
		LinesToDigest.emplace_back(0, Miles_Electric_Lower);
		LinesToDigest.emplace_back(GONERSPEAKER_TAILS, TICRATE/2,
			"But what about all the little animals...");

		LinesToDigest.emplace_back(GONERSPEAKER_EGGMAN, 0,
			"It's just logical. I know you'll conquer this run.");
	}

	if (gamedata->gonerlevel <= GDGONER_TUTORIAL)
	{
		if (gamedata->gonerlevel <= GDGONER_VIDEO)
			goner_background.darkframes = 5; // handle abrupt transition

		goner_levelworking = gamedata->gonerlevel = GDGONER_TUTORIAL;
	}
}

static boolean M_GonerInputs(INT32 ch)
{
	const UINT8 pid = 0;
	static int holdtime = 0;
	const int magicscroll = 4; // hehe

	(void)ch;

	if (menucmd[pid].dpad_ud != 0)
	{
		if (((++holdtime) % magicscroll) == 1) // Instantly responsive
		{
			if (menucmd[pid].dpad_ud < 0)
			{
				if (goner_scroll < goner_scrollend)
					goner_scroll++;
			}
			else
			{
				if (goner_scroll > 0)
				{
					goner_scroll--;
					if (goner_delay < magicscroll)
						goner_delay = magicscroll;
				}
				else if (!menucmd[pid].prev_dpad_ud // taps only
				&& !goner_typewriter.textDone)
				{
					goner_typewriter.CompleteText();
					goner_delay = magicscroll;
				}
			}

			return true;
		}
	}

	holdtime = 0;

	return false;
}

void M_GonerResetText(boolean completely)
{
	goner_typewriter.ClearText();
	LinesToDigest.clear();
	LinesOutput.clear();

	goner_scroll = 0;
	goner_scrollend = -1;

	if (!completely)
		return;

	goner_levelworking = GDGONER_INIT;
}
