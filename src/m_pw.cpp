// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Vivian "toastergrl" Grannell.
// Copyright (C) 2024 by James Robert Roman.
// Copyright (C) 2024 by Kart Krew.
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
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "modp_b64/modp_b64.h"

#include "cxxutil.hpp"

#include "command.h"
#include "d_main.h"
#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"
#include "g_game.h"
#include "k_menu.h"
#include "m_cheat.h"
#include "m_cond.h"
#include "m_pw.h"
#include "m_pw_hash.h"
#include "s_sound.h"
#include "sounds.h"
#include "z_zone.h"

namespace
{

constexpr const UINT8 kRRSalt[17] = "0L4rlK}{9ay6'VJS";

struct Pw
{
	Pw(void (*cb)(), const char *encoded_hash) : cb_(cb), hash_(decode_hash(encoded_hash)) {}

	void (*cb_)();
	const std::array<UINT8, M_PW_BUF_SIZE> hash_;

private:
	static std::array<UINT8, M_PW_BUF_SIZE> decode_hash(std::string encoded)
	{
		std::array<UINT8, M_PW_BUF_SIZE> decoded;
		if (modp::b64_decode(encoded).size() != decoded.size())
			throw std::invalid_argument("hash is incorrectly sized");
		std::copy(encoded.begin(), encoded.end(), decoded.begin());
		return decoded;
	}
};

std::vector<Pw> passwords;

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
			"All challenges temporarily unlocked.\n"
			"Saving is disabled - the game will\n"
			"return to normal on next launch.\n"
		);
	}
	else
	{
		S_StartSound(0, sfx_s3k7b);

		if (usedCheats)
		{
			text = M_GetText(
				"This is the correct password, but\n"
				"you already have every challenge\n"
				"unlocked, so nothing has changed.\n"
			);
		}
		else
		{
			text = M_GetText(
				"This is the correct password, but\n"
				"you already have every challenge\n"
				"unlocked, so saving is still allowed!\n"
			);
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
	gamedata->gonerlevel = GDGONER_DONE;
	gamedata->finishedtutorialchallenge = true;
	M_UpdateUnlockablesAndExtraEmblems(true, true);

	M_ClearMenus(true);
	S_StartSound(0, sfx_kc42);

	G_SaveGameData();
}

}; // namespace

try_password_e M_TryPassword(const char *password, boolean conditions)
{
	using var = std::variant<std::monostate, condition_t*, Pw*>;

	// Normalize input casing
	std::string key = password;
	strlwr(key.data());

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
	if (conditions)
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
	passwords.emplace_back(f_shittysigns, "fdqz0cQdVKfS4zgN4usUz75+5AhYdDPsrl61H7sIEKHoaEUHPOfcful0jEkVvrV/rpELE0/3srRpxmJYpQofmA==");
	passwords.emplace_back(f_tastelesstaunts, "4QfCuCG0/7z5U5A1hxqlqx83uQTGmQ1aaWPBQ8pqQvw9KRGvxxiDq9UF0N24fDlu0+XYksgkPHJg4A5h5aEQiw==");
	passwords.emplace_back(f_devmode, "ybYqLUlREa9TJqV0uftxqGL8jPR1U+uEgrff/jast0kCfeIdzY15VxjveSZho8GOjfRuC3Zt4aJQTvhJcPAhkw==");
	passwords.emplace_back(f_proceed, "GZPKJsa++Tt134yS3eBKdP+8vdAHB1thwK2ys6VDfFxcIRtABtM9j4qt8WULFrI+KxCSYMZ6K0mwt5BVzcvvuw==");
}
