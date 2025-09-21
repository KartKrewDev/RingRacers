// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <variant>

#include "modp_b64/modp_b64.h"

#include "core/string.h"
#include "core/vector.hpp"
#include "cxxutil.hpp"

#include "command.h"
#include "d_main.h"
#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"
#include "f_finale.h"
#include "g_game.h"
#include "k_menu.h"
#include "m_cheat.h"
#include "m_random.h"
#include "m_cond.h"
#include "m_pw.h"
#include "m_pw_hash.h"
#include "s_sound.h"
#include "sounds.h"
#include "z_zone.h"

namespace
{

constexpr const UINT8 kRRSalt[17] = "0L4rlK}{9ay6'VJS";

std::array<UINT8, M_PW_BUF_SIZE> decode_hash(srb2::String encoded)
{
	std::array<UINT8, M_PW_BUF_SIZE> decoded;
	std::string encoded_stl { static_cast<std::string_view>(encoded) };
	if (modp::b64_decode(encoded_stl).size() != decoded.size())
		throw std::invalid_argument("hash is incorrectly sized");
	std::copy(encoded_stl.begin(), encoded_stl.end(), decoded.begin());
	return decoded;
}

struct Pw
{
	Pw(void (*cb)(), const char *encoded_hash) : cb_(cb), hash_(decode_hash(encoded_hash)) {}

	void (*cb_)();
	const std::array<UINT8, M_PW_BUF_SIZE> hash_;
};

srb2::Vector<Pw> passwords;

// m_cond.c
template <typename F>
void iter_conditions(F&& f)
{
	UINT32 i, j;
	conditionset_t *c;
	condition_t *cn;

	for (i = 0; i < MAXCONDITIONSETS; ++i)
	{
		c = &conditionSets[i];

		if (!c->numconditions || gamedata->achieved[i])
			continue;

		for (j = 0; j < c->numconditions; ++j)
		{
			cn = &c->condition[j];

			if (cn->type != UC_PASSWORD)
				continue;

			if (cn->stringvar == NULL)
				continue;

			f(cn);
		}
	}
}

//
// responders
//

void f_tournament()
{
	UINT16 i;
	boolean success = false;

	/*if (modifiedgame)
		return 0;*/

	// Unlock EVERYTHING.
	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		if (unlockables[i].conditionset == CH_FURYBIKE)
			continue;
		if (gamedata->unlocked[i])
			continue;

		gamedata->unlocked[i] = true;
		success = true;
	}

	// Unlock all hidden levels.
#define GD_MV_SET (MV_VISITED|MV_BEATEN)
	for (i = 0; i < nummapheaders; i++)
	{
		if ((mapheaderinfo[i]->records.mapvisited & GD_MV_SET) == GD_MV_SET)
			continue;
		if (mapheaderinfo[i]->typeoflevel & TOL_VERSUS)
			continue;
		if (!strcmp(mapheaderinfo[i]->lumpname, "RR_HIDDENPALACE"))
			continue;
		mapheaderinfo[i]->records.mapvisited |= GD_MV_SET;
		success = true;
	}
#undef GD_MV_SET

	// Goofy, but this call needs to be before M_ClearMenus because that path
	// calls G_LoadLevel, which will trigger a gamedata save. Garbage factory
	if (success)
	{
		gamedata->gonerlevel = GDGONER_DONE;
		gamedata->sealedswapalerted = true;
		G_SetUsedCheats();
	}

	M_ClearMenus(true);

	const char *text;

	if (success)
	{
		S_StartSound(0, sfx_kc42);

		text = M_GetText(
			"Unlocked\x83 almost\x80 everything.\n"
			"Saving is disabled - the game will\n"
			"return to normal on next launch.\n"
		);

		usedTourney = true;
	}
	else
	{
		S_StartSound(0, sfx_s3k7b);

		if (usedCheats)
		{
			text = M_GetText(
				"This is the correct password,\n"
				"but there's nothing to unlock\n"
				"right now -- nothing has changed.\n"
			);
		}
		else
		{
			text = M_GetText(
				"This is the correct password, but\n"
				"there's nothing to unlock right\n"
				"now, so saving is still allowed!\n"
			);

			usedTourney = true;
		}
	}

	M_StartMessage("Tournament Mode", text, NULL, MM_NOTHING, NULL, NULL);
}

void f_bighead()
{
	CV_SetValue(&cv_bighead, !cv_bighead.value);
	if (cv_bighead.value)
	{
		S_StartSound(NULL, sfx_gshbb);
	}
	else
	{
		S_StartSound(NULL, sfx_kc46);
	}
}

void f_shittysigns()
{
	CV_SetValue(&cv_shittysigns, !cv_shittysigns.value);
	if (cv_shittysigns.value)
	{
		S_StartSound(NULL, sfx_mixup);
	}
	else
	{
		S_StartSound(NULL, sfx_nghurt);
	}
}

void f_tastelesstaunts()
{
	CV_SetValue(&cv_tastelesstaunts, !cv_tastelesstaunts.value);
	if (cv_tastelesstaunts.value)
	{
		S_StartSound(NULL, sfx_d4getm);
	}
	else
	{
		S_StartSound(NULL, sfx_kc46);
	}
}

void f_4thgear()
{
	CV_SetValue(&cv_4thgear, !cv_4thgear.value);
	if (cv_4thgear.value)
	{
		M_StartMessage("Restraint device compromised!", "Local play sped up to ""\x85""4th Gear!""\x80""\nLet's see what you're made of!\n\n""\x86""No effect in netplay and attack modes.", NULL, MM_NOTHING, NULL, NULL);
		S_StartSound(NULL, sfx_gshc4);
	}
	else
	{
		S_StartSound(NULL, sfx_kc46);
	}
}

void f_levelskull()
{
	CV_SetValue(&cv_levelskull, !cv_levelskull.value);
	if (cv_levelskull.value)
	{
		M_StartMessage("It's over for humans!", "CPU difficulty raised to ""\x85""TRUE MAXIMUM!""\x80""\nThis isn't even remotely fair!", NULL, MM_NOTHING, NULL, NULL);
		S_StartSound(NULL, sfx_gshdf);
	}
	else
	{
		S_StartSound(NULL, sfx_kc46);
	}
}

void f_colors()
{
	UINT16 i;
	boolean success = false;

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		if (unlockables[i].conditionset == CH_FURYBIKE)
			continue;
		if (gamedata->unlocked[i])
			continue;
		if (unlockables[i].type != SECRET_COLOR)
			continue;

		gamedata->unlocked[i] = true;
		success = true;
	}

	if (success)
	{
		S_StartSound(0, sfx_kc42);
		M_StartMessage("Time for a new look!", "Unlocked all colors. Try not to show off!", NULL, MM_NOTHING, NULL, NULL);
		G_SaveGameData();
	}
	else
	{
		M_StartMessage("Time for a new look!", "You've already unlocked all colors.", NULL, MM_NOTHING, NULL, NULL);
	}
}

void f_followers()
{
	UINT16 i;
	boolean success = false;

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		if (unlockables[i].conditionset == CH_FURYBIKE)
			continue;
		if (gamedata->unlocked[i])
			continue;
		if (unlockables[i].type != SECRET_FOLLOWER)
			continue;

		gamedata->unlocked[i] = true;
		success = true;
	}

	if (success)
	{
		S_StartSound(0, sfx_kc42);
		M_StartMessage("Creatures captured!", "Unlocked all followers. Who's your favorite?", NULL, MM_NOTHING, NULL, NULL);
		G_SaveGameData();
	}
	else
	{
		M_StartMessage("Creatures captured!", "You've already unlocked all followers.", NULL, MM_NOTHING, NULL, NULL);
	}
}

void f_maps()
{
	UINT16 i;
	boolean success = false;

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		if (unlockables[i].conditionset == CH_FURYBIKE)
			continue;
		if (gamedata->unlocked[i])
			continue;
		if (unlockables[i].type != SECRET_MAP && unlockables[i].type != SECRET_CUP)
			continue;

		gamedata->unlocked[i] = true;
		success = true;
	}

#define GD_MV_SET (MV_VISITED|MV_BEATEN)
	for (i = 0; i < nummapheaders; i++)
	{
		if ((mapheaderinfo[i]->records.mapvisited & GD_MV_SET) == GD_MV_SET)
			continue;
		if (mapheaderinfo[i]->typeoflevel & TOL_VERSUS)
			continue;
		if (!strcmp(mapheaderinfo[i]->lumpname, "RR_HIDDENPALACE"))
			continue;
		mapheaderinfo[i]->records.mapvisited |= GD_MV_SET;
		success = true;
	}
#undef GD_MV_SET

	if (success)
	{
		S_StartSound(0, sfx_kc42);
		M_StartMessage("// FIXME don't crash in certification test", "Unlocked most maps. Go see the world!", NULL, MM_NOTHING, NULL, NULL);
		G_SaveGameData();
	}
	else
	{
		M_StartMessage("// FIXME don't crash in certification test", "There are no maps to unlock.", NULL, MM_NOTHING, NULL, NULL);
	}
}

void f_tutorials()
{
	UINT16 i;
	boolean success = false;

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		if (unlockables[i].conditionset == CH_FURYBIKE)
			continue;
		if (gamedata->unlocked[i])
			continue;
		if (unlockables[i].type != SECRET_MAP)
			continue;

		UINT16 mapnum = M_UnlockableMapNum(&unlockables[i]);
		if (mapnum >= nummapheaders || !mapheaderinfo[mapnum])
			continue;

		if (G_GuessGametypeByTOL(mapheaderinfo[mapnum]->typeoflevel) != GT_TUTORIAL)
			continue;

		gamedata->unlocked[i] = true;
		success = true;
	}

	if (success)
	{
		S_StartSound(0, sfx_kc42);
		G_SaveGameData();
	}

	const char *knucklesrap;
	const UINT8 numsections = 5;
	static UINT8 section = numsections;
	if (section == numsections)
		section = M_RandomKey(numsections);

	switch (section)
	{
		default:
			knucklesrap =
				"\x85""So here's what I'm thinkin',                    \n"
				"\x85""        last time smart guys got together,\n"
				"\x85""In a tough sandy place                        \n"
				"\x85""                    with a lot of hot weather,\n"
				"\x85""Playin' fundamental forces                  \n"
				"\x85""                      at the top of their class,\n"
				"\x85""They made the sky so much hotter        \n"
				"\x85""                      and that sand into glass!\x80";
			break;
		case 1:
			knucklesrap =
				"\x85""But somethin's different, see?           \n"
				"\x85""My homie right there, I trust him trustin'\n"
				"\x85""             the Eggman like it's nothin',\n"
				"\x85""A smart little guy                     \n"
				"\x85""         with a brother I like fightin',\n"
				"\x85""If there's somethin' cooking           \n"
				"\x85""        I'm sure he'll do the right thing.\x80";
			break;
		case 2:
			knucklesrap =
				"\x85""I watched a space station fall,          \n"
				"\x85""       straight down, fast fall, into the ground,\n"
				"\x85""Pieces of fire, shooting stars,                 \n"
				"\x85""                      don't make a wish, kids\n"
				"\x85""Last second it's, gone, I ask how,            \n"
				"\x85""              behold, they call it chaos control,\n"
				"\x85""I don't know it, never heard it, seen it, felt it,  \n"
				"\x85""  sensed it. Even to me, these powers are a mystery.\x80";
			break;
		case 3:
			knucklesrap =
				"\x85""The tide goes out, it carries time away,  \n"
				"\x85""                     we call it yesterday,\n"
				"\x85""The tide comes in, it rings, it sings,    \n"
				"\x85""                      it brings a new age,\n"
				"\x85""But right now it's just me and the water,  \n"
				"\x85""       thoughts clear, future lookin' hotter,\n"
				"\x85""I let myself sink in, feel the waves,     \n"
				"\x85""                 feel my body get lighter.\x80";
			break;
		case 4:
			knucklesrap =
				"\x85""Somethin' brewin' at the edges,            \n"
				"\x85""   that's what a ring is, nothin' but edges,\n"
				"\x85""Circled light in a band,                  \n"
				"\x85""                 hold 'em in in your hand,\n"
				"\x85""But it's a miracle a thousand times over,\n"
				"\x85""Small gasps of potential\n"
				"\x85""               floatin' over the sand.\x80";
			break;
	}

	section = (section + 1) % numsections;

	M_StartMessage("\"Broken Arrow\" ...for Sunbeam Paradise",
		va("\"%s\"\n\n%s",
		knucklesrap,
		(success ? "Unlocked all Tutorials." : "There are no more Tutorials to unlock.")),
		NULL, MM_NOTHING, NULL, NULL);
}

void f_characters()
{
	UINT16 i;
	boolean success = false;

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		if (unlockables[i].conditionset == CH_FURYBIKE)
			continue;
		if (gamedata->unlocked[i])
			continue;
		if (unlockables[i].type != SECRET_SKIN)
			continue;

		gamedata->unlocked[i] = true;
		success = true;
	}

	if (success)
	{
		S_StartSound(0, sfx_kc42);
		M_StartMessage("...Is that how you spell it?", "Unlocked most characters. All together now!", NULL, MM_NOTHING, NULL, NULL);
		G_SaveGameData();
	}
	else
	{
		M_StartMessage("...Is that how you spell it?", "There are no characters to unlock!", NULL, MM_NOTHING, NULL, NULL);
	}
}

void f_altmusic()
{
	UINT16 i;
	boolean success = false;

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		if (unlockables[i].conditionset == CH_FURYBIKE)
			continue;
		if (gamedata->unlocked[i])
			continue;
		if (unlockables[i].type != SECRET_ALTMUSIC && unlockables[i].type != SECRET_SOUNDTEST)
			continue;

		gamedata->unlocked[i] = true;
		success = true;
	}

	if (success)
	{
		S_StartSound(0, sfx_kc42);
		M_StartMessage("Wanna listen to some tunes?", "Unlocked all alternate music -- and Stereo Mode!", NULL, MM_NOTHING, NULL, NULL);
		G_SaveGameData();
	}
	else
	{
		M_StartMessage("Wanna listen to some tunes?", "You've already unlocked all music!", NULL, MM_NOTHING, NULL, NULL);
	}
}

void f_timeattack()
{
	UINT16 i;
	boolean success = false;
	boolean already_have_encore = M_SecretUnlocked(SECRET_ENCORE, true);

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		if (unlockables[i].conditionset == CH_FURYBIKE)
			continue;
		if (gamedata->unlocked[i])
			continue;

		if (unlockables[i].type == SECRET_TIMEATTACK
			|| unlockables[i].type == SECRET_PRISONBREAK
			|| unlockables[i].type == SECRET_SPECIALATTACK
			|| (unlockables[i].type == SECRET_SPBATTACK && already_have_encore))
		{
			gamedata->unlocked[i] = true;
			success = true;
		}
	}

	if (success)
	{
		S_StartSound(0, sfx_kc42);
		if (already_have_encore)
		{
			M_StartMessage("Time Trial ON, OK!", "Unlocked all Time Attack modes -- including SPB Attack!", NULL, MM_NOTHING, NULL, NULL);
		}
		else
		{
			M_StartMessage("Time Trial ON, OK!", "Unlocked all Time Attack modes!", NULL, MM_NOTHING, NULL, NULL);
		}
		G_SaveGameData();
	}
	else
	{
		M_StartMessage("Time Trial ON, OK!", "You already have all Time Attack modes.", NULL, MM_NOTHING, NULL, NULL);
	}
}

void f_encore()
{
	UINT16 i;
	boolean success = false;
	boolean already_have_timeattacks = (
		M_SecretUnlocked(SECRET_TIMEATTACK, true)
		&& M_SecretUnlocked(SECRET_PRISONBREAK, true)
		&& M_SecretUnlocked(SECRET_SPECIALATTACK, true)
	);

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		if (unlockables[i].conditionset == CH_FURYBIKE)
			continue;
		if (gamedata->unlocked[i])
			continue;

		if (unlockables[i].type == SECRET_ENCORE
			|| (unlockables[i].type == SECRET_SPBATTACK && already_have_timeattacks))
		{
			gamedata->unlocked[i] = true;
			success = true;
		}
	}

	if (success)
	{
		S_StartSound(0, sfx_kc42);
		if (already_have_timeattacks)
		{
			M_StartMessage("And turn it all around!", "Unlocked Encore Mode -- and SPB Attack!", NULL, MM_NOTHING, NULL, NULL);
		}
		else
		{
			M_StartMessage("And turn it all around!", "Unlocked Encore Mode!", NULL, MM_NOTHING, NULL, NULL);
		}
		G_SaveGameData();
	}
	else
	{
		M_StartMessage("And turn it all around!", "You already have Encore Mode.", NULL, MM_NOTHING, NULL, NULL);
	}
}

void f_difficulty()
{
	UINT16 i;
	boolean success = false;

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		if (unlockables[i].conditionset == CH_FURYBIKE)
			continue;
		if (gamedata->unlocked[i])
			continue;

		if (unlockables[i].type == SECRET_HARDSPEED || unlockables[i].type == SECRET_MASTERMODE)
		{
			gamedata->unlocked[i] = true;
			success = true;
		}
	}

	if (success)
	{
		S_StartSound(0, sfx_kc42);
		M_StartMessage("TWO SPEEDS,", "All Gear speeds unlocked!", NULL, MM_NOTHING, NULL, NULL);
		G_SaveGameData();
	}
	else
	{
		M_StartMessage("TWO SPEEDS,", "You already have all Gear speeds.", NULL, MM_NOTHING, NULL, NULL);
	}
}

void f_keys()
{
	INT32 givekeys = 25;

	if (gamedata->chaokeys > (GDMAX_CHAOKEYS - givekeys))
	{
		givekeys = GDMAX_CHAOKEYS - gamedata->chaokeys;
	}

	if (givekeys > 0)
	{
		S_StartSound(0, sfx_keygen);

		gamedata->chaokeys += givekeys;
		gamedata->chaokeytutorial = true;

		M_StartMessage("Dr. Robotnik's Ring Racers - Deluxe Edition", va("Claimed %d Chao Keys!", givekeys), NULL, MM_NOTHING, NULL, NULL);
		G_SaveGameData();
	}
	else
	{
		M_StartMessage("Dr. Robotnik's Ring Racers - Deluxe Edition", "You have the maximum number of Chao Keys!", NULL, MM_NOTHING, NULL, NULL);
	}
}

void f_devmode()
{
	INT32 i;

	if (modifiedgame)
		return;

	// Just unlock all the things and turn on -debug and console devmode.
	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
			continue;
		gamedata->unlocked[i] = true;
	}

	// Unlock all hidden levels.
	for (i = 0; i < nummapheaders; i++)
	{
		mapheaderinfo[i]->records.mapvisited = MV_MAX;
	}

	gamedata->gonerlevel = GDGONER_DONE;
	gamedata->sealedswapalerted = true;

	M_ClearMenus(true);

	// This is a developer feature, you know how to delete ringdata
	// G_SetUsedCheats();
	S_StartSound(0, sfx_kc42);

	devparm = true;
	cht_debug |= 0x8000;

	G_SaveGameData();
}

void f_proceed()
{
#if 0
	gamedata->gonerlevel = GDGONER_DONE;
	gamedata->finishedtutorialchallenge = true;
	M_UpdateUnlockablesAndExtraEmblems(true, true);

	M_ClearMenus(true);
	S_StartSound(0, sfx_kc42);

	G_SaveGameData();
#else
	F_StartIntro();
	M_ClearMenus(true);
	M_GonerResetText(true);
#endif
}

}; // namespace

try_password_e M_TryPassword(const char *password, boolean conditions)
{
	using var = std::variant<std::monostate, condition_t*, Pw*>;

	// Normalize input casing
	srb2::String key = password;
	strlwr((char*)key.data());

	UINT8 key_hash[M_PW_HASH_SIZE];
	M_HashPassword(key_hash, key.c_str(), kRRSalt);

	auto worker = [key_hash](const UINT8* hash, var result)
	{
		if (memcmp(key_hash, hash, M_PW_HASH_SIZE))
			result = std::monostate {}; // fail state
		return result;
	};

	var result;
	auto add_job = [&](auto&&... args)
	{
		if (var n = worker(args...); !std::holds_alternative<std::monostate>(n))
			result = n;
	};

	for (Pw& pw : passwords)
		add_job(pw.hash_.data(), &pw);

	// Only consider challenges passwords as needed.
	if (conditions && !usedTourney)
		iter_conditions([&](condition_t* cn) { add_job((const UINT8*)cn->stringvar, cn); });

	try_password_e return_code = M_PW_INVALID;
	if (!std::holds_alternative<std::monostate>(result))
	{
		// Evaluate the password's function.
		auto visitor = srb2::Overload {
			[&](condition_t* cn)
			{
				// Remove the password for this session.
				Z_Free(cn->stringvar);
				cn->stringvar = NULL;
				return_code = M_PW_CHALLENGES;
			},
			[&](Pw* pw)
			{
				pw->cb_();
				return_code = M_PW_EXTRAS;
			},
			[](std::monostate) {},
		};
		std::visit(visitor, result);
	}

	return return_code;
}

boolean M_TryExactPassword(const char *password, const char *encodedhash)
{
	// Normalize input casing
	srb2::String key = password;
	strlwr((char*)key.data());

	UINT8 key_hash[M_PW_HASH_SIZE];
	M_HashPassword(key_hash, key.c_str(), kRRSalt);

	auto hash = decode_hash(encodedhash);

	return (memcmp(key_hash, hash.data(), M_PW_HASH_SIZE) == 0);
}

#ifdef DEVELOP
void Command_Crypt_f(void)
{
	if (COM_Argc() == 1)
	{
		CONS_Printf(
			"crypt <password>: generate a password hash\n"
			"crypt -i <file>: generate multiple hashes by reading from file\n"
		);
		return;
	}

	auto gen = [](char *input)
	{
		UINT8 bin[M_PW_BUF_SIZE];
		strlwr(input);
		M_HashPassword(bin, input, kRRSalt);
		CONS_Printf("%s %s\n", input, modp::b64_encode((const char*)bin, M_PW_BUF_SIZE).c_str());
	};

	if (!stricmp(COM_Argv(1), "-i"))
	{
		if (COM_Argc() != 3)
		{
			CONS_Printf("crypt: missing file argument\n");
			return;
		}

		std::ifstream file{va(pandf, srb2home, COM_Argv(2))};
		if (!file.is_open())
		{
			CONS_Printf("crypt: file error\n");
			return;
		}

		for (std::string line; std::getline(file, line);)
		{
			// remove comments
			std::size_t p = line.find("#");
			if (p == line.npos)
				p = line.size();

			// remove trailing whitespace
			while (p > 0 && std::isspace(line[p - 1]))
				p--;

			line.erase(p);

			// ignore empty or completely filtered lines
			if (!line.empty())
				gen(line.data());
		}

		return;
	}

	gen(COM_Args());
}
#endif

void M_PasswordInit(void)
{
	passwords.emplace_back(f_tournament, "dSZpCST31Tu3rPJ4z18iR9Tcv+9Xi8/f7nQGplj2mvruy2A4CJJqZm1gzi6CQKl68pRXiNGUX0n4BI2LjaBcoA==");
	// Tee hee.
	passwords.emplace_back([] { M_WrongWarp(0); }, "WAJJ66pw2rSopXOuw4c4iKzIz3goKtivrv7b/THqYP8ev+E/sRn2LMXHqv8s+uzwMcVNoDxNn+AgG26xi+wgzg==");
	passwords.emplace_back([] { M_GonerGDQ(true); }, "B287p2gJUgmUikAABl1ndG/3r0zqdIMvsMDzBrypwo78BR58S9Whu+Doma00oV+DySTalWYi1VyTs/5GWzgFEg==");
	passwords.emplace_back([] { M_GonerGDQ(false); }, "1yO8FCDe0PhtgrQt0IQ4TPPfggSOnf4NiRaT86gnj4/PxMbyi4vXl4F4zpm/Xhf2oSStuhr+n7Qv2tcqv6lzaA==");
	passwords.emplace_back(f_bighead, "V+YkwthNUePKS7zs5uB90VwN6Jeqgl+1r663U5zSGOEIxAO6BoWipzZoxa5H//LM+5Ag9GIGRnEcLbU21hjGfQ==");
	passwords.emplace_back(f_4thgear, "zRMhR+s27VTYE0jgFf2l+PX51N3qJPvZ3oWuM/71oUaKY5zyQ2y7WIrIb464MFWn4IsK2P5rShsR9MotC/9ojQ==");
	passwords.emplace_back(f_shittysigns, "yd02TPSLRgBydXlkZaEJABqegGjfJfn1aIMODfc2CC5ymJ4ydG7FblW20CH6vbo1IB1X9eBKJShuunPBClnWOQ==");
	passwords.emplace_back(f_tastelesstaunts, "4QfCuCG0/7z5U5A1hxqlqx83uQTGmQ1aaWPBQ8pqQvw9KRGvxxiDq9UF0N24fDlu0+XYksgkPHJg4A5h5aEQiw==");
	passwords.emplace_back(f_devmode, "ybYqLUlREa9TJqV0uftxqGL8jPR1U+uEgrff/jast0kCfeIdzY15VxjveSZho8GOjfRuC3Zt4aJQTvhJcPAhkw==");
	passwords.emplace_back(f_proceed, "GZPKJsa++Tt134yS3eBKdP+8vdAHB1thwK2ys6VDfFxcIRtABtM9j4qt8WULFrI+KxCSYMZ6K0mwt5BVzcvvuw==");
	passwords.emplace_back(f_colors, "aSk8dw6FzJtTEmovh8fVEtUBpu6lj3QlRT/B5lwiEhAw8dAhRBQLdvtYlPaQcZISWI4wneAfAo6w5d6uf5r++g==");
	passwords.emplace_back(f_followers, "zYCIZw2qcnUbtF0P2ybLNHajdl8zrje0hzGex7yuMFe7fj4mvx4AegoMmvir28YvAbfAqkz/ekQRzr+RhrycHw==");
	passwords.emplace_back(f_maps, "u/Svaf+DCnCpJ8xmP3AVP4CK6X6X4O3fY73cmIq88ZJEygwz+n+L66q4Vhlv13vWgld1PEyRszFErzflQt9WZw==");
	passwords.emplace_back(f_tutorials, "G2FMttJpJ+lI/DeQu8tthL5i7AB4dk8uuksZR1c2N08Zrmpj3vTqWpbhxuSzSrhH10wJfWahR7QOgQdBkDbTdQ==");
	passwords.emplace_back(f_characters, "MohmPqpaGSd3MEHLfQKUFl/Yg8pHE+12X1LHEP59Gs/5w1u8mPtGUXNv1GYTF+c8gQqT5hXpZ3FeZ/EfCxo34g==");
	passwords.emplace_back(f_altmusic, "dZgxKNagOtB9F7wXqUUPzsuq4tfQlfK8ZqEeFXdI3Hd+k5tYfRm3ToLgbqawaNmwuLVrJ8PB+QnH4gT3ojnTMw==");
	passwords.emplace_back(f_timeattack, "mFu5OB9d6jnc2kth7HE66wJ42F/GHDzSvuciK1Qw++6iGnpBccxcKjpoxgOvD3eIoqR606ruBINuXi23proXHQ==");
	passwords.emplace_back(f_encore, "i5u5sIsMs5eITy+LzAXvKm6D9OzOVKhUqSy1mTTV/oUxJX6RPsk8OcyLbNaey9Vc6wXOhz+2+mTXILkIRzvXqA==");
	passwords.emplace_back(f_difficulty, "MKjOtEFLkgXf21uiECdBTU6XtbkuFWaGh7i8znKo7JrXXEDrCBJmGwINvPg0T3TLn0zlscLvmC5nve7I+NTrnA==");
	passwords.emplace_back(f_keys, "jgsD6UJ2Xa10QcS2ZDJwcvpd4iia3AXIG8wDDSsHX7kFH5jEXnym45yaNZG9hIKEvBMpVONKR0YTA6JBAQRCvg==");
	passwords.emplace_back(f_levelskull, "hpQP2tC+TGVQojDcYaC4236+QZZR8Tj/OQb1dAkjnMNpc0/AAdRAIQSveLqd7xW2Dw62Fc3noEkeYTHQkPa+WQ==");
}
