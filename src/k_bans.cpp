// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez
// Copyright (C) 2025 by Kart Krew
// Copyright (C) 2020 by Sonic Team Junior
// Copyright (C) 2000 by DooM Legacy Team
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_bans.c
/// \brief replacement for DooM Legacy ban system

#include <stdexcept>

#include <fmt/format.h>
#include <tcb/span.hpp>

#include "i_tcp_detail.h" // clientaddress

#include "core/json.hpp"
#include "core/string.h"
#include "io/streams.hpp"
#include "k_bans.h"
#include "byteptr.h" // READ/WRITE macros
#include "command.h"
#include "d_clisrv.h" // playernode
#include "d_main.h"	  // pandf
#include "d_net.h"	  // MAXNETNODES
#include "doomtype.h"
#include "k_profiles.h"
#include "m_misc.h"	 //FIL_WriteFile()
#include "p_saveg.h" // savebuffer_t
#include "time.h"
#include "z_zone.h"
#include "i_addrinfo.h" // I_getaddrinfo

using srb2::JsonArray;
using srb2::JsonObject;
using srb2::JsonValue;

static mysockaddr_t *DuplicateSockAddr(const mysockaddr_t *source)
{
	auto dup = static_cast<mysockaddr_t*>(Z_Malloc(sizeof *source, PU_STATIC, NULL));
	memcpy(dup, source, sizeof *source);
	return dup;
}

static srb2::Vector<banrecord_t> bans;

static uint8_t allZero[PUBKEYLENGTH];

static void load_bans_array_v1(const JsonArray& array)
{
	int index = 0;
	for (const JsonValue& object : array)
	{
		uint8_t public_key_bin[PUBKEYLENGTH];

		srb2::String public_key = object.at("public_key").get<srb2::String>();
		srb2::String ip_address = object.at("ip_address").get<srb2::String>();
		time_t expires = object.at("expires").get<int64_t>();
		UINT8 subnet_mask = object.at("subnet_mask").get<UINT8>();
		srb2::String username = object.at("username").get<srb2::String>();
		srb2::String reason = object.at("reason").get<srb2::String>();

		if (!FromPrettyRRID(public_key_bin, public_key.c_str()))
		{
			throw std::runtime_error("public_key has invalid format");
		}

		bool banned = SV_BanIP(
			ip_address.c_str(),
			subnet_mask,
			public_key_bin,
			expires,
			username.c_str(),
			reason.c_str()
		);

		if (!banned)
		{
			CONS_Alert(CONS_WARNING, "IP address of ban entry %d invalid\n", index);
		}
		index++;
	}
}

void SV_LoadBans(void)
{
	if (!server)
		return;

	JsonValue object;

	srb2::String banspath { srb2::format("{}/{}", srb2home, BANFILE) };
	srb2::io::BufferedInputStream<srb2::io::FileStream> bis;
	try
	{
		srb2::io::FileStream fs { banspath, srb2::io::FileStreamMode::kRead };
		bis = srb2::io::BufferedInputStream(std::move(fs));
	}
	catch (const srb2::io::FileStreamException& ex)
	{
		// file didn't open, likely doesn't exist
		return;
	}

	try
	{
		srb2::Vector<tcb::byte> data = srb2::io::read_to_vec(bis);
		srb2::String data_s;
		data_s.reserve(data.size());
		for (auto b : data)
		{
			data_s.push_back(std::to_integer<char>(b));
		}
		object = JsonValue::from_json_string(data_s);
	}
	catch (const std::exception& ex)
	{
		const char* gdfolder = "the Ring Racers folder";
		if (strcmp(srb2home, "."))
			gdfolder = srb2home;

		I_Error("%s\nNot a valid ban file.\nDelete %s (maybe in %s) and try again.", ex.what(), BANFILE, gdfolder);
	}

	if (!object.is_object())
	{
		return;
	}

	try
	{
		if (object.value("version", 1) == 1)
		{
			JsonValue& array = object.at("bans");

			if (array.is_array())
			{
				load_bans_array_v1(array.as_array());
			}
		}
	}
	catch (const std::exception& ex)
	{
		I_Error("%s: %s", BANFILE, ex.what());
	}
}

void SV_SaveBans(void)
{
	JsonValue object = JsonValue(JsonObject());

	object["version"] = 1;
	JsonValue& array_value = object["bans"];

	array_value = JsonValue(JsonArray());
	JsonArray& array = array_value.as_array();

	for (banrecord_t& ban : bans)
	{
		if (ban.deleted)
			continue;

		JsonObject ban_obj;
		ban_obj["public_key"] = srb2::String(GetPrettyRRID(ban.public_key, false));
		ban_obj["ip_address"] = srb2::String(SOCK_AddrToStr(ban.address));
		ban_obj["subnet_mask"] = ban.mask;
		ban_obj["expires"] = static_cast<int64_t>(ban.expires);
		ban_obj["username"] = srb2::String(ban.username);
		ban_obj["reason"] = srb2::String(ban.reason);
		array.push_back(std::move(ban_obj));
	}

	srb2::String json_string = object.to_json_string();
	srb2::String banfile_path = srb2::format("{}/{}", srb2home, BANFILE);

	try
	{
		srb2::io::FileStream fs { banfile_path, srb2::io::FileStreamMode::kWrite };
		srb2::io::write_exact(fs, tcb::as_bytes(tcb::span(json_string)));
	}
	catch (const std::exception& ex)
	{
		I_Error("%s\nCouldn't save bans. Are you out of disk space / playing in a protected folder?", ex.what());
	}
}

mysockaddr_t convertedaddress;
static mysockaddr_t* SV_NodeToBanAddress(UINT8 node)
{
	convertedaddress = SOCK_DirectNodeToAddr(node);

	if (convertedaddress.any.sa_family == AF_INET)
	{
		convertedaddress.ip4.sin_port = 0;
		// mask was 32?
	}
#ifdef HAVE_IPV6
	else if (convertedaddress.any.sa_family == AF_INET6)
	{
		convertedaddress.ip6.sin6_port = 0;
		// mask was 128?
	}
#endif

	return &convertedaddress;
}

static boolean SV_IsBanEnforced(banrecord_t *ban)
{
	if (ban->deleted)
		return false;
	if ((ban->expires != 0) && (time(NULL) > ban->expires))
		return false;
	return true;
}

banrecord_t* SV_GetBanByAddress(UINT8 node)
{
	mysockaddr_t* address = SV_NodeToBanAddress(node);

	for (banrecord_t& ban : bans)
	{
		if (!SV_IsBanEnforced(&ban))
			continue;
		if (SOCK_cmpaddr(ban.address, address, ban.mask))
			return &ban;
	}

	return NULL;
}

banrecord_t* SV_GetBanByKey(uint8_t* key)
{
	UINT32 hash;

	hash = quickncasehash((char*) key, PUBKEYLENGTH);

	for (banrecord_t& ban : bans)
	{
		if (!SV_IsBanEnforced(&ban))
			continue;
		if (hash != ban.hash) // Not crypto magic, just an early out with a faster comparison
			continue;
		if (memcmp(&ban.public_key, allZero, PUBKEYLENGTH) == 0)
			continue; // Don't ban GUESTs on accident, we have a cvar for this.
		if (memcmp(&ban.public_key, key, PUBKEYLENGTH) == 0)
			return &ban;
	}

	return NULL;
}

void SV_BanPlayer(INT32 pnum, time_t minutes, char* reason)
{
	mysockaddr_t targetaddress;
	UINT8 node = playernode[pnum];
	time_t expires = 0;

	memcpy(&targetaddress, SV_NodeToBanAddress(node), sizeof(mysockaddr_t));

	if (minutes != 0)
		expires = time(NULL) + minutes * 60;

	SV_Ban(targetaddress, 0, players[pnum].public_key, expires, player_names[pnum], reason);
}

boolean SV_BanIP(const char *address, UINT8 mask, uint8_t* public_key, time_t expires, const char* username, const char* reason)
{
	struct my_addrinfo *ai, *runp, hints;

	memset(&hints, 0x00, sizeof(hints));
	hints.ai_flags = 0;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	int gaie;
	gaie = I_getaddrinfo(address, "0", &hints, &ai);

	if (gaie != 0)
		return false;

	runp = ai;

	mysockaddr_t newaddr;
	memcpy(&newaddr, runp->ai_addr, runp->ai_addrlen);

	SV_Ban(newaddr, mask, public_key, expires, username, reason);

	return true;
}

static char noreason[] = "N/A";
static char nouser[] = "[Direct IP ban]";
void SV_Ban(mysockaddr_t address, UINT8 mask, uint8_t* public_key, time_t expires, const char* username, const char* reason)
{
	if (reason == NULL || strlen(reason) == 0)
		reason = noreason;
	if (!public_key)
		public_key = allZero;
	if (!expires)
		expires = 0;
	if (!username)
		username = nouser;

	banrecord_t ban{};

	ban.address = DuplicateSockAddr(&address);
	memcpy(ban.public_key, public_key, PUBKEYLENGTH);
	memcpy(&ban.expires, &expires, sizeof(time_t));
	ban.mask = mask;
	strlcpy(ban.username, username, MAXBANUSERNAME);
	strlcpy(ban.reason, reason, MAXBANREASON);

	ban.hash = quickncasehash((char*) ban.public_key, PUBKEYLENGTH);

	bans.push_back(ban);

	SV_SaveBans();
}

static void SV_BanSearch(boolean remove)
{
	const char* filtertarget = (COM_Argc() > 1) ? COM_Argv(1) : NULL;

	if (remove && !filtertarget)
	{
		CONS_Printf("unban <target>: removes all bans matching target username, key, or address\n");
		return;
	}

	time_t now = time(NULL);
	UINT32 records = 0;
	UINT32 matchedrecords = 0;
	boolean force = COM_CheckPartialParm("-f");

	// First pass: What records are we even looking for?
	for (banrecord_t& ban : bans)
	{
		ban.matchesquery = false;

		if (ban.deleted)
			continue;

		records++;

		const char* stringaddress = SOCK_AddrToStr(ban.address);
		const char* stringkey = GetPrettyRRID(ban.public_key, true);

		if (filtertarget != NULL && !(strcasestr(filtertarget, ban.username) || strcasestr(filtertarget, stringkey) ||
								strcasestr(filtertarget, stringaddress)))
			continue;

		ban.matchesquery = true;
		matchedrecords++;
	}

	boolean saferemove = remove && (matchedrecords <= 1 || force);

	// Second pass: Report and/or act on records.
	for (banrecord_t& ban : bans)
	{
		if (!ban.matchesquery)
			continue;

		const char* stringaddress = SOCK_AddrToStr(ban.address);
		const char* stringkey = GetPrettyRRID(ban.public_key, true);

		srb2::String recordprint = srb2::format(
			"{}{} - {} [{}] - {}",
			stringaddress,
			ban.mask && ban.mask != 32 ? srb2::format("/{}", ban.mask) : "",
			ban.username,
			stringkey,
			ban.reason
		);

		if (ban.expires)
		{
			if (ban.expires < now)
				recordprint += " - EXPIRED";
			else
				recordprint += srb2::format(" - expires {}m", (ban.expires - now)/60);
		}

		CONS_Printf("%s\n", recordprint.c_str());

		if (saferemove)
			ban.deleted = true;
	}

	if (records == 0)
		CONS_Printf("You haven't banned anyone yet.\n");
	else if (matchedrecords == 0)
		CONS_Printf("\x82""No matches in %d bans.\n", records);
	else if (remove && !saferemove)
		CONS_Printf("\x82""Didn't unban, would remove %d bans.\x80 Refine search or use 'unban <search> -f'.\n", matchedrecords);
	else if (saferemove)
		CONS_Printf("\x83""Removed %d/%d bans.\n", matchedrecords, records);
	else if (filtertarget)
		CONS_Printf("Matched %d/%d bans.\n", matchedrecords, records);
	else
		CONS_Printf("Showing %d bans. Try 'listbans [search]' to refine results.\n", matchedrecords);

	if (saferemove)
		SV_SaveBans();
}

void Command_Listbans(void)
{
	if (netgame && consoleplayer != serverplayer)
	{
		CONS_Printf("You must be the netgame host to do this.\n");
		return;
	}
	SV_BanSearch(false);
}

void Command_Unban(void)
{
	if (netgame && consoleplayer != serverplayer)
	{
		CONS_Printf("You must be the netgame host to do this.\n");
		return;
	}
	SV_BanSearch(true);
}

void Command_BanIP(void)
{
	if (COM_Argc() <= 1)
	{
		CONS_Printf("banip <target> [reason]: bans the specified IP from netgames you host\n");
		return;
	}

	char* input = Z_StrDup(COM_Argv(1));

	const char *address = strtok(input, "/");
	const char *mask = strtok(NULL, "");

	if (!mask)
		mask = "";

	char reason[MAXBANREASON+1] = "";
	if (COM_Argc() > 2)
		strncpy(reason, COM_Argv(2), MAXBANREASON);

	UINT8 nummask = atoi(mask);

	if (SV_BanIP(address, nummask, NULL, 0, NULL, reason))
	{
		CONS_Printf("Banned address %s.\n", address);
	}
	else
	{
		CONS_Printf("Not a valid IP address.\n");
	}

	Z_Free(input);
}
