// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_modinfo.h
/// \brief Mod metadata

#ifndef __K_MODINFO_H__
#define __K_MODINFO_H__

#include "typedef.h"
#include "doomtype.h"
#include "doomdef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	MOD_COMPATIBLE = 0, // Fully compatible.
	MOD_INCOMPATIBLE_FUTURE, // For a future version.
	MOD_INCOMPATIBLE_PAST // For a way way too old version.
} mod_compat_e;

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus

#include <string>

class mod_metadata_t
{
private:
	patch_t *_icon;
	UINT16 _game_version;
	UINT16 _game_subversion;

	std::string _name;
	std::string _author;
	std::string _description;
	std::string _info_url;
	std::string _version;

	void init_defaults(void);
	bool parse_info_json(const char *contents, size_t contents_len);

public:
	mod_metadata_t(size_t wad_id);

	patch_t *icon() const { return _icon; }
	UINT16 game_version() const { return _game_version; }
	UINT16 game_subversion() const { return _game_subversion; }

	std::string name() const { return _name; }
	std::string author() const { return _author; }
	std::string description() const { return _description; }
	std::string info_url() const { return _info_url; }
	std::string version() const { return _version; }

	mod_compat_e Compatible() const
	{
#if (VERSION == 0 && SUBVERSION == 0)
		// DEVELOP builds don't have any relevant version info.
		// Just disable the checks entirely, you probably
		// know what you're doing.
		return MOD_COMPATIBLE;
#else
		if (_game_version > VERSION || (_game_version == VERSION && _game_subversion > SUBVERSION))
		{
			// This mod might use features that don't exist
			// in earlier versions, so tell the user to
			// update their game to play with this mod.
			return MOD_INCOMPATIBLE_FUTURE;
		}

		if (_game_version < VERSION)
		{
			// Whenever VERSION is incremented, it's because mod
			// backwards compatibility has been broken beyond
			// repair. This mod is too old to support!
			return MOD_INCOMPATIBLE_PAST;
		}

		return MOD_COMPATIBLE;
#endif
	}
};

#else

// C compatibility interface
struct mod_metadata_t;
typedef struct mod_metadata_t mod_metadata_t;

#endif

#ifdef __cplusplus
extern "C" {
#endif

patch_t *ModMetadata_GetIcon(mod_metadata_t *meta);
UINT16 ModMetadata_GetGameVersion(mod_metadata_t *meta);
UINT16 ModMetadata_GetGameSubVersion(mod_metadata_t *meta);

char *ModMetadata_GetName(mod_metadata_t *meta);
char *ModMetadata_GetAuthor(mod_metadata_t *meta);
char *ModMetadata_GetDescription(mod_metadata_t *meta);
char *ModMetadata_GetInfoURL(mod_metadata_t *meta);
char *ModMetadata_GetVersion(mod_metadata_t *meta);

mod_compat_e ModMetadata_Compatible(mod_metadata_t *meta);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_MODINFO_H__
