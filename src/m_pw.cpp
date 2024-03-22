// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Vivian "toastergrl" Grannell.
// Copyright (C) 2024      by James Robert Roman.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <future>
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
#include "stun.h" // csprng
#include "z_zone.h"

namespace
{

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

}; // namespace

try_password_e M_TryPassword(const char *password, boolean conditions)
{
	using var = std::variant<std::monostate, condition_t*, Pw*>;

	// Normalize input casing
	std::string key = password;
	strlwr(key.data());

	auto worker = [&key](const UINT8* hash, var result)
	{
		if (M_HashCompare(hash, key.c_str()))
			result = std::monostate {}; // fail state
		return result;
	};

	// Because hashing is time consuming, do the work in parallel.
	std::vector<std::future<var>> jobs;
	auto add_job = [&](auto&&... args)
	{
		jobs.push_back(std::move(std::async(std::launch::async, worker, args...)));
	};

	for (Pw& pw : passwords)
		add_job(pw.hash_.data(), &pw);

	// Only consider challenges passwords as needed.
	if (conditions)
		iter_conditions([&](condition_t* cn) { add_job((const UINT8*)cn->stringvar, cn); });

	var result;
	for (auto& job : jobs)
	{
		SRB2_ASSERT(job.valid());
		// Wait for every thread to finish, then retrieve the last matched password (if any).
		if (var n = job.get(); !std::holds_alternative<std::monostate>(n))
			result = n;
	}

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
		UINT8* salt = &bin[M_PW_HASH_SIZE];
		csprng(salt, M_PW_SALT_SIZE); // randomize salt

		strlwr(input);
		M_HashPassword(bin, input, salt);
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
}
