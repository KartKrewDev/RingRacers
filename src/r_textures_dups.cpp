// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string_view>
#include <thread>

#include <fmt/format.h>

#include "core/hash_map.hpp"
#include "core/string.h"
#include "core/vector.hpp"
#include "cxxutil.hpp"
#include "doomstat.h"
#include "r_textures.h"
#include "w_wad.h"

namespace
{

srb2::HashMap<srb2::String, srb2::Vector<const texture_t*>> g_dups;
std::map<srb2::String, srb2::Vector<srb2::String>> g_warnings;
std::thread g_dups_thread;

srb2::String key8char(const char cstr[8])
{
	std::string_view view(cstr, 8);
	srb2::String key;

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

srb2::String texture_location(const texture_t& tex)
{
	if (tex.type == TEXTURETYPE_SINGLEPATCH)
	{
		SRB2_ASSERT(tex.patchcount == 1);

		const texpatch_t& texpat = tex.patches[0];
		const wadfile_t& wad = *wadfiles[texpat.wad];

		return srb2::format(
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
		const srb2::String key = key8char(t1->name);

		if (g_dups.find(key) != g_dups.end())
		{
			return;
		}

		int32_t idx = find_dup(t1, start);

		if (idx < end)
		{
			srb2::Vector<const texture_t*>& v = g_dups[key];

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

	for (auto [key, v] : g_dups)
	{
		std::for_each(v.cbegin(), v.cend(), print_dup);
	}

	g_dups = {};

	R_PrintTextureWarnings();
}

void R_InsertTextureWarning(const char *header, const char *warning)
{
	g_warnings[header].push_back(warning);
}

void R_PrintTextureWarnings(void)
{
	if (g_dups_thread.joinable())
	{
		return;
	}

	for (auto [header, v] : g_warnings)
	{
		CONS_Alert(CONS_WARNING, "\n%s", header.c_str());

		for (const srb2::String& warning : v)
		{
			CONS_Printf("%s\n", warning.c_str());
		}
	}

	g_warnings = {};
}
