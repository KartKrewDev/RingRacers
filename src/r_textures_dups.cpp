// RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include "cxxutil.hpp"
#include "doomstat.h"
#include "r_textures.h"
#include "w_wad.h"

namespace
{

std::unordered_map<std::string, std::vector<const texture_t*>> g_dups;
std::thread g_dups_thread;

std::string key8char(const char cstr[8])
{
	std::string_view view(cstr, 8);
	std::string key;

	view = view.substr(0, view.find('\0')); // terminate by '\0'
	key.reserve(view.size());
	std::transform(
		view.cbegin(),
		view.cend(),
		std::back_inserter(key),
		[](unsigned char c) { return std::toupper(c); }
	);

	return key;
}

std::string texture_location(const texture_t& tex)
{
	if (tex.type == TEXTURETYPE_SINGLEPATCH)
	{
		SRB2_ASSERT(tex.patchcount == 1);

		const texpatch_t& texpat = tex.patches[0];
		const wadfile_t& wad = *wadfiles[texpat.wad];

		return fmt::format(
			"'{}/{}'",
			std::filesystem::path(wad.filename).filename().string(),
			wad.lumpinfo[texpat.lump].fullname
		);
	}
	else
	{
		return "TEXTURES";
	}
}

void print_dup(const texture_t* tex)
{
	// Do not use CONS_Alert because it is not thread-safe
	CONS_Printf(
		"\x82"
		"WARNING:"
		"\x80 duplicate texture '%.8s' (%s)\n",
		tex->name,
		texture_location(*tex).c_str()
	);
}

}; // namespace

void R_CheckTextureDuplicates(INT32 start, INT32 end)
{
	SRB2_ASSERT(start >= 0);
	SRB2_ASSERT(end <= numtextures);

	auto find_dup = [end](const texture_t* t1, int32_t idx)
	{
		while (++idx < end)
		{
			const texture_t* t2 = textures[idx];

			if (t1->hash == t2->hash && !strncmp(t1->name, t2->name, 8))
			{
				break;
			}
		}

		return idx;
	};

	auto collate_dups = [end, find_dup](int32_t start)
	{
		const texture_t* t1 = textures[start];
		const std::string key = key8char(t1->name);

		if (g_dups.find(key) != g_dups.end())
		{
			return;
		}

		int32_t idx = find_dup(t1, start);

		if (idx < end)
		{
			std::vector<const texture_t*>& v = g_dups[key];

			v.push_back(textures[start]);

			do
			{
				v.push_back(textures[idx]);
			} while ((idx = find_dup(t1, idx)) < end);
		}
	};

	auto worker = [start, end, collate_dups]
	{
		for (int32_t i = start; i < end; ++i)
		{
			collate_dups(i);
		}
	};

	SRB2_ASSERT(g_dups_thread.joinable() == false);
	g_dups_thread = std::thread(worker);
}

void R_PrintTextureDuplicates(void)
{
	if (g_dups_thread.joinable())
	{
		g_dups_thread.join();
	}

	if (g_dups.empty())
	{
		return;
	}

	for (auto [key, v] : g_dups)
	{
		std::for_each(v.cbegin(), v.cend(), print_dup);
	}

	g_dups = {};
}
