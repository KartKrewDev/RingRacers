// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_modinfo.cpp
/// \brief Mod metadata

#include "k_modinfo.h"

#include <string>
#include <vector>
#include <algorithm>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "w_wad.h"
#include "z_zone.h"

using nlohmann::json;

void mod_metadata_t::init_defaults(void)
{
	// Initialize default fields.
	_game_version = 2; // Defaults to the latest version
	_game_subversion = 3; // that did not support MODINFO lumps.
}

bool mod_metadata_t::parse_info_json(const char *contents, size_t contents_len)
{
	if (contents_len == 0)
	{
		CONS_Alert(CONS_ERROR, "could not parse MODINFO; contents are empty\n");
		return false;
	}

	json info_obj = json::parse(contents, contents + static_cast<unsigned int>(contents_len));
	if (info_obj.is_object() == false)
	{
		CONS_Alert(CONS_ERROR, "could not parse MODINFO; is not a JSON object\n");
		return false;
	}

	_name = info_obj.value("name", _name);
	_author = info_obj.value("author", _author);
	_description = info_obj.value("description", _description);
	_info_url = info_obj.value("info_url", _info_url);
	_version = info_obj.value("version", _version);

	std::string game_version_str = info_obj.value("game_version", "");
	if (game_version_str.empty() == false)
	{
		int game_version_int = 0, game_subversion_int = 0;
		int result = sscanf(game_version_str.c_str(), "%d.%d", &game_version_int, &game_subversion_int);

		if (result >= 1 && game_version_int >= 0 && game_version_int <= UINT16_MAX)
		{
			_game_version = static_cast<UINT16>(game_version_int);
			_game_subversion = 0; // Just inputting "2" should be the same as "2.0"
		}

		if (result >= 2 && game_subversion_int >= 0 && game_subversion_int <= UINT16_MAX)
		{
			_game_subversion = static_cast<UINT16>(game_subversion_int);
		}
	}

	return true;
}

// Create mod metadata from a WAD file ID.
// There is the possibility of allocating mod_metadata_t
// for an unloaded file for the addons menu, which should
// be a different constructor.
mod_metadata_t::mod_metadata_t(size_t wad_id)
{
	init_defaults();

	// Default to the WAD's name
	_name = wadfiles[wad_id]->filename;

	lumpnum_t lump_index;
	lumpinfo_t *lump_p = wadfiles[wad_id]->lumpinfo;

	lumpnum_t icon_lump = LUMPERROR;
	lumpnum_t info_lump = LUMPERROR;

	for (lump_index = 0; lump_index < wadfiles[wad_id]->numlumps; lump_index++, lump_p++)
	{
		if (info_lump == LUMPERROR && memcmp(lump_p->name, "MODINFO", 8) == 0)
		{
			info_lump = lump_index;
		}
		else if (icon_lump == LUMPERROR && memcmp(lump_p->name, "MODICON", 8) == 0)
		{
			icon_lump = lump_index;
		}

		if (info_lump != LUMPERROR && icon_lump != LUMPERROR)
		{
			// Found all lumps, exit early.
			break;
		}
	}

	if (info_lump != LUMPERROR)
	{
		// Parse info JSON
		size_t info_lump_len = W_LumpLengthPwad(wad_id, info_lump);
		char *info_contents = static_cast<char *>( W_CacheLumpNumPwad(wad_id, info_lump, PU_STATIC) );

		parse_info_json(info_contents, info_lump_len);

		Z_Free(info_contents);
	}

	if (icon_lump != LUMPERROR)
	{
		// We have an icon lump. Cache it!
		_icon = static_cast<patch_t *>( W_CachePatchNumPwad(wad_id, icon_lump, PU_STATIC) );
	}

	// TODO: Make incompatibile WADs exit early and unload?
	// It would require that we pass in the temporary lumpinfo and handle.
	// It doesn't look hard, just very tedious, and I can't be assed rn.
	mod_compat_e is_compatible = Compatible();
	switch (is_compatible)
	{
		case MOD_INCOMPATIBLE_FUTURE:
		{
			CONS_Alert(
				CONS_ERROR,
				"Mod '%s' was designed for a newer version of Ring Racers (mod is for v%d.%d, you have v%d.%d). Update your copy!\n",
				_name.c_str(),
				_game_version, _game_subversion,
				VERSION, SUBVERSION
			);
			break;
		}
		case MOD_INCOMPATIBLE_PAST:
		{
			CONS_Alert(
				CONS_ERROR,
				"Mod '%s' was designed for a backwards-incompatible version of Ring Racers (mod is for v%d.%d, you have v%d.%d). This mod will need updated before it can be used.\n",
				_name.c_str(),
				_game_version, _game_subversion,
				VERSION, SUBVERSION
			);
			break;
		}
		default:
		{
			// Mod is compatible!
			break;
		}
	}
}

//
// C interface functions
//

patch_t *ModMetadata_GetIcon(mod_metadata_t *meta)
{
	return meta->icon();
}

UINT16 ModMetadata_GetGameVersion(mod_metadata_t *meta)
{
	return meta->game_version();
}

UINT16 ModMetadata_GetGameSubVersion(mod_metadata_t *meta)
{
	return meta->game_subversion();
}

char *ModMetadata_GetName(mod_metadata_t *meta)
{
	return Z_StrDup( meta->name().c_str() );
}

char *ModMetadata_GetAuthor(mod_metadata_t *meta)
{
	return Z_StrDup( meta->author().c_str() );
}

char *ModMetadata_GetDescription(mod_metadata_t *meta)
{
	return Z_StrDup( meta->description().c_str() );
}

char *ModMetadata_GetInfoURL(mod_metadata_t *meta)
{
	return Z_StrDup( meta->info_url().c_str() );
}

char *ModMetadata_GetVersion(mod_metadata_t *meta)
{
	return Z_StrDup( meta->version().c_str() );
}

mod_compat_e ModMetadata_Compatible(mod_metadata_t *meta)
{
	return meta->Compatible();
}
