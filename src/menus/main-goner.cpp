/// \file  menus/main-goner.cpp
/// \brief The Goner Setup.

#include "../k_menu.h"
#include "../r_skins.h"
#include "../st_stuff.h" // faceprefix
#include "../v_draw.hpp"
#include "../k_dialogue.hpp"

#include <forward_list>

menuitem_t MAIN_Goner[] =
{
	{IT_STRING | IT_CVAR | IT_CV_STRING, "Password",
		"ATTEMPT ADMINISTRATOR ACCESS.", NULL,
		{.cvar = &cv_dummyextraspassword}, 0, 0},

	{IT_STRING | IT_CALL, "Quit",
		"CONCLUDE OBSERVATIONS NOW.", NULL,
		{.routine = M_QuitSRB2}, 0, 0},

	{IT_STRING | IT_CALL, "Video Options",
		"CONFIGURE OCULAR PATHWAYS.", NULL,
		{.routine = M_VideoOptions}, 0, 0},

	{IT_STRING | IT_CALL, "Sound Options",
		"CALIBRATE AURAL DATASTREAM.", NULL, 
		{.routine = M_SoundOptions}, 0, 0},

	{IT_STRING | IT_CALL, "Profile Setup",
		"ASSIGN VEHICLE INPUTS.", NULL,
		{.routine = M_GonerProfile}, 0, 0},

	{IT_STRING | IT_CALL, "Begin Tutorial",
		"PREPARE FOR INTEGRATION.", NULL,
		{.routine = M_GonerTutorial}, 0, 0},
};

static boolean M_GonerInputs(INT32 ch);
static void M_GonerDrawer(void);

menu_t MAIN_GonerDef = {
	sizeof (MAIN_Goner) / sizeof (menuitem_t),
	NULL,
	0,
	MAIN_Goner,
	32, 160,
	0, 0,
	MBF_UD_LR_FLIPPED,
	"_GONER",
	0, 0,
	M_GonerDrawer,
	M_GonerTick,
	NULL,
	NULL,
	M_GonerInputs,
};

// ---

typedef enum
{
	GONERSPEAKER_EGGMAN = 0,
	GONERSPEAKER_TAILS,
	MAXGONERSPEAKERS
} gonerspeakers_t;

class GonerSpeaker
{
public:
	float offset;

	GonerSpeaker(std::string skinName, float offset)
	{
		if (!skinName.empty())
		{
			this->skinID = R_SkinAvailable(skinName.c_str());
		}

		this->offset = offset;
	};

	sfxenum_t TalkSound(void)
	{
		if (!ValidID())
			return sfx_ktalk;

		return skins[ skinID ]
			.soundsid[ S_sfx[sfx_ktalk].skinsound ];
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

std::array<std::optional<GonerSpeaker>, MAXGONERSPEAKERS> goner_speakers = {};

srb2::Dialogue::Typewriter goner_typewriter;

int goner_delay;

class GonerChatLine
{
public:
	gonerspeakers_t speaker;
	std::string dialogue;
	int value; // Mutlipurpose.

	GonerChatLine(gonerspeakers_t speaker, int delay, std::string dialogue)
	{
		this->speaker = speaker;
		this->dialogue = V_ScaledWordWrap(
			(BASEVIDWIDTH/2 + 12) << FRACBITS,
			FRACUNIT, FRACUNIT, FRACUNIT,
			0, TINY_FONT,
			dialogue.c_str()
		);
		this->value = delay;
	};

	// Returns true if line is text
	bool Handle(void)
	{
		if (speaker >= MAXGONERSPEAKERS)
			return false;

		goner_typewriter.voiceSfx = sfx_ktalk;
		if (goner_speakers[speaker])
		{
			goner_typewriter.voiceSfx = (*goner_speakers[speaker]).TalkSound();
		}

		goner_typewriter.NewText(dialogue);

		goner_delay = value;

		value = 1; // this is now repurposed as the number of lines visible

		return true;
	};
};

std::forward_list<GonerChatLine> LinesToDigest;
std::forward_list<GonerChatLine> LinesOutput;

// ---

void M_GonerTick(void)
{
	static bool speakersinit = false;
	if (!speakersinit)
	{
		goner_delay = TICRATE;

		goner_speakers[GONERSPEAKER_EGGMAN] = GonerSpeaker("eggman", 0);
		goner_speakers[GONERSPEAKER_TAILS] = GonerSpeaker("tails", 12);

		LinesToDigest.emplace_front(GONERSPEAKER_EGGMAN, TICRATE,
			"Metal Sonic. Are you online?");
		LinesToDigest.emplace_front(GONERSPEAKER_EGGMAN, TICRATE/2,
			"Take a close look, Miles. Moments ago he was at my throat!\
			Now he's docile as can be on that operating table.");

		LinesToDigest.emplace_front(GONERSPEAKER_TAILS, 0,
			"I don't feel very safe!");
		LinesToDigest.emplace_front(GONERSPEAKER_TAILS, TICRATE/4,
			"But its programming is definitely locked down...");

		LinesToDigest.emplace_front(GONERSPEAKER_EGGMAN, 0,
			"You've given me quite the headache, Metal.\
			Thankfully, Tails caught you in the act.");

		LinesToDigest.emplace_front(GONERSPEAKER_TAILS, TICRATE/5,
			"Wait, I'm getting weird readings over the network.");
		LinesToDigest.emplace_front(GONERSPEAKER_TAILS, 0,
			"Metal Sonic is the unit labeled \"MS1\", right?");
		LinesToDigest.emplace_front(GONERSPEAKER_TAILS, TICRATE,
			"The ""\x87""viewport""\x80"" and ""\x87""audio""\x80"" config looks like it got messed up.");

		LinesToDigest.emplace_front(GONERSPEAKER_EGGMAN, 0,
			"So you're right. I wonder if it has anything to do with that outburst.");
		LinesToDigest.emplace_front(GONERSPEAKER_EGGMAN, 0,
			"Alright, Metal! I don't remember your specifications offhand. First things first, go ahead and set up your ""\x87""Video Options""\x80"" yourself.");

		LinesToDigest.reverse();

		speakersinit = true;
	}

	if (menutyping.active == false && cv_dummyextraspassword.string[0] != '\0')
	{
		// Challenges are not interpreted at this stage.
		// See M_ExtraTick for the full behaviour.

		cht_Interpret(cv_dummyextraspassword.string);
		CV_StealthSet(&cv_dummyextraspassword, "");
	}

	goner_typewriter.WriteText();

	if (goner_typewriter.textDone)
	{
		if (goner_delay > 0)
			goner_delay--;
		else if (!LinesToDigest.empty())
		{
			if (!LinesOutput.empty())
				LinesOutput.front().value = goner_typewriter.textLines;

			auto line = LinesToDigest.front();
			if (line.Handle())
				LinesOutput.push_front(line);
			LinesToDigest.pop_front();
		}
	}
}

static void M_GonerDrawer(void)
{
	srb2::Draw drawer = srb2::Draw();

	drawer
		.width(BASEVIDWIDTH)
		.height(BASEVIDHEIGHT)
		.fill(31);

	drawer = drawer.x(BASEVIDWIDTH/4);

	float newy = BASEVIDHEIGHT/2 + (3*12);
	boolean first = true;

	for (auto & element : LinesOutput)
	{
		INT32 flags = V_TRANSLUCENT;
		std::string text;
		if (first)
		{
			text = goner_typewriter.text;
			newy -= goner_typewriter.textLines*12;
			flags = 0;
			first = false;
		}
		else
		{
			text = element.dialogue;
			newy -= element.value*12;
		}

		if (newy < 0) break;

		//if (newy > BASEVIDHEIGHT) continue; -- not needed yet

		if (!goner_speakers[element.speaker])
			continue;

		auto speaker = *goner_speakers[element.speaker];

		srb2::Draw line = drawer
			.xy(speaker.offset, newy)
			.flags(flags);

		int skinID = speaker.GetSkinID();
		if (skinID != -1)
		{
			line
				.xy(-16, -2)
				.colormap(skinID, static_cast<skincolornum_t>(skins[skinID].prefcolor))
				.patch(faceprefix[skinID][FACE_MINIMAP]);
		}

		line
			.font(srb2::Draw::Font::kThin)
			.text( text.c_str() );
	}

	M_DrawHorizontalMenu();
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
}

void M_GonerTutorial(INT32 choice)
{
	(void)choice;

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

	// Please also see M_LevelSelectInit as called in extras-1.c
	levellist.netgame = false;
	levellist.levelsearch.checklocked = true;
	cupgrid.grandprix = false;
	levellist.levelsearch.timeattack = false;

	if (!M_LevelListFromGametype(GT_TUTORIAL))
	{
		// The game is incapable of progression, but I can't bring myself to put an I_Error here.
		M_StartMessage("SURVEY_PROGRAM",
			"YOU ACCEPT EVERYTHING THAT WILL HAPPEN FROM NOW ON.",
			&M_QuitResponse, MM_YESNO, "I agree", "Cancel");
	}
}

static boolean M_GonerInputs(INT32 ch)
{
	const UINT8 pid = 0;
	(void)ch;

	if (M_MenuBackPressed(pid))
	{
		// No returning to the title screen.
		M_QuitSRB2(-1);
		return true;
	}

	return false;
}
