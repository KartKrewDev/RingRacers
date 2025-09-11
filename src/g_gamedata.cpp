// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "g_gamedata.h"

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <exception>
#include <filesystem>

#include <fmt/format.h>

#include "io/streams.hpp"
#include "d_main.h"
#include "m_argv.h"
#include "m_cond.h"
#include "g_game.h"
#include "r_skins.h"
#include "z_zone.h"

namespace fs = std::filesystem;

#define GD_VERSION_MAJOR (0xBA5ED321)
#define GD_VERSION_MINOR (2)

#define GD_MINIMUM_SPRAYCANSV2 (2)
#define GD_MINIMUM_TIMEATTACKV2 (2)
#define GD_MINIMUM_TUTORIALLOCK (2)

void srb2::save_ng_gamedata()
{
	if (gamedata == NULL || !gamedata->loaded)
		return; // If never loaded (-nodata), don't save

	gamedata->deferredsave = false;

	if (usedCheats)
	{
#ifdef DEVELOP
		CONS_Alert(CONS_WARNING, M_GetText("Cheats used - Gamedata will not be saved.\n"));
#endif
		return;
	}

	GamedataJson ng {};

	ng.playtime.total = gamedata->totalplaytime;
	ng.playtime.netgame = gamedata->totalnetgametime;
	ng.playtime.timeattack = gamedata->timeattackingtotaltime;
	ng.playtime.spbattack = gamedata->spbattackingtotaltime;
	ng.playtime.race = gamedata->modeplaytime[GDGT_RACE];
	ng.playtime.battle = gamedata->modeplaytime[GDGT_BATTLE];
	ng.playtime.prisons = gamedata->modeplaytime[GDGT_PRISONS];
	ng.playtime.special = gamedata->modeplaytime[GDGT_SPECIAL];
	ng.playtime.custom = gamedata->modeplaytime[GDGT_CUSTOM];
	ng.playtime.menus = gamedata->totalmenutime;
	ng.playtime.statistics = gamedata->totaltimestaringatstatistics;
	ng.rings.total = gamedata->totalrings;
	ng.playtime.tumble = gamedata->totaltumbletime;
	ng.rounds.race = gamedata->roundsplayed[GDGT_RACE];
	ng.rounds.battle = gamedata->roundsplayed[GDGT_BATTLE];
	ng.rounds.prisons = gamedata->roundsplayed[GDGT_PRISONS];
	ng.rounds.special = gamedata->roundsplayed[GDGT_SPECIAL];
	ng.rounds.custom = gamedata->roundsplayed[GDGT_CUSTOM];
	ng.challengekeys.pendingkeyrounds = gamedata->pendingkeyrounds;
	ng.challengekeys.pendingkeyroundoffset = gamedata->pendingkeyroundoffset;
	ng.challengekeys.keyspending = gamedata->keyspending;
	ng.challengekeys.chaokeys = gamedata->chaokeys;
	ng.milestones.everloadedaddon = gamedata->everloadedaddon;
	ng.milestones.everfinishcredits = gamedata->everfinishedcredits;
	ng.milestones.eversavedreplay = gamedata->eversavedreplay;
	ng.milestones.everseenspecial = gamedata->everseenspecial;
	ng.milestones.chaokeytutorial = gamedata->chaokeytutorial;
	ng.milestones.majorkeyskipattempted = gamedata->majorkeyskipattempted;
	ng.milestones.finishedtutorialchallenge = gamedata->finishedtutorialchallenge;
	ng.milestones.enteredtutorialchallenge = gamedata->enteredtutorialchallenge;
	ng.milestones.sealedswapalerted = gamedata->sealedswapalerted;
	ng.milestones.tutorialdone = gamedata->tutorialdone;
	ng.milestones.playgroundroute = gamedata->playgroundroute;
	ng.milestones.gonerlevel = gamedata->gonerlevel;
	ng.prisons.thisprisoneggpickup = gamedata->thisprisoneggpickup;
	ng.prisons.prisoneggstothispickup = gamedata->prisoneggstothispickup;
	ng.tafolderhash = quickncasehash(timeattackfolder, 64);
	ng.emblems.resize(MAXEMBLEMS, false);
	for (int i = 0; i < MAXEMBLEMS; i++)
	{
		ng.emblems[i] = gamedata->collected[i];
	}
	ng.unlockables.resize(MAXUNLOCKABLES, false);
	for (int i = 0; i < MAXUNLOCKABLES; i++)
	{
		ng.unlockables[i] = gamedata->unlocked[i];
	}
	ng.unlockpending.resize(MAXUNLOCKABLES, false);
	for (int i = 0; i < MAXUNLOCKABLES; i++)
	{
		ng.unlockpending[i] = gamedata->unlockpending[i];
	}
	ng.conditionsets.resize(MAXCONDITIONSETS, false);
	for (int i = 0; i < MAXCONDITIONSETS; i++)
	{
		ng.conditionsets[i] = gamedata->achieved[i];
	}
	if (gamedata->challengegrid)
	{
		ng.challengegrid.width = gamedata->challengegridwidth;
		ng.challengegrid.grid.resize(gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT, 0);
		for (int i = 0; i < gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT; i++)
		{
			ng.challengegrid.grid[i] = gamedata->challengegrid[i];
		}
	}
	ng.timesBeaten = gamedata->timesBeaten;

	auto skintojson = [](skinrecord_t *records)
	{
		srb2::GamedataSkinJson skin {};
		skin.records.wins = records->wins;
		skin.records.rounds = records->rounds;
		skin.records.time.total = records->timeplayed;
		skin.records.time.race = records->modetimeplayed[GDGT_RACE];
		skin.records.time.battle = records->modetimeplayed[GDGT_BATTLE];
		skin.records.time.prisons = records->modetimeplayed[GDGT_PRISONS];
		skin.records.time.special = records->modetimeplayed[GDGT_SPECIAL];
		skin.records.time.custom = records->modetimeplayed[GDGT_CUSTOM];
		skin.records.time.tumble = records->tumbletime;

		return skin;
	};

	for (int i = 0; i < numskins; i++)
	{
		skin_t& memskin = *skins[i];

		auto skin = skintojson(&memskin.records);
		srb2::String name { memskin.name };
		ng.skins[name] = std::move(skin);
	}
	for (auto unloadedskin = unloadedskins; unloadedskin; unloadedskin = unloadedskin->next)
	{
		auto skin = skintojson(&unloadedskin->records);
		srb2::String name { unloadedskin->name };
		ng.skins[name] = std::move(skin);
	}

	for (int i = 0; i < gamedata->numspraycans; i++)
	{
		uint16_t col = gamedata->spraycans[i].col;

		if (col >= SKINCOLOR_FIRSTFREESLOT)
		{
			col = SKINCOLOR_NONE;
		}

		ng.spraycans_v2.emplace_back(String(skincolors[col].name));
	}

	auto maptojson = [](recorddata_t *records)
	{
		srb2::GamedataMapJson map {};
		map.visited.visited = records->mapvisited & MV_VISITED;
		map.visited.beaten = records->mapvisited & MV_BEATEN;
		map.visited.encore = records->mapvisited & MV_ENCORE;
		map.visited.spbattack = records->mapvisited & MV_SPBATTACK;
		map.visited.mysticmelody = records->mapvisited & MV_MYSTICMELODY;
		map.stats.timeattack.besttime = records->timeattack.time;
		map.stats.timeattack.bestlap = records->timeattack.lap;
		map.stats.spbattack.besttime = records->spbattack.time;
		map.stats.spbattack.bestlap = records->spbattack.lap;
		map.stats.time.total = records->timeplayed;
		map.stats.time.netgame = records->netgametimeplayed;
		map.stats.time.race = records->modetimeplayed[GDGT_RACE];
		map.stats.time.battle = records->modetimeplayed[GDGT_BATTLE];
		map.stats.time.prisons = records->modetimeplayed[GDGT_PRISONS];
		map.stats.time.special = records->modetimeplayed[GDGT_SPECIAL];
		map.stats.time.custom = records->modetimeplayed[GDGT_CUSTOM];
		map.stats.time.timeattack = records->timeattacktimeplayed;
		map.stats.time.spbattack = records->spbattacktimeplayed;
		map.spraycan = records->spraycan;

		return map;
	};

	for (int i = 0; i < nummapheaders; i++)
	{
		auto map = maptojson(&mapheaderinfo[i]->records);
		srb2::String lumpname { mapheaderinfo[i]->lumpname };
		ng.maps[lumpname] = std::move(map);
	}
	for (auto unloadedmap = unloadedmapheaders; unloadedmap; unloadedmap = unloadedmap->next)
	{
		auto map = maptojson(&unloadedmap->records);
		srb2::String lumpname { unloadedmap->lumpname };
		ng.maps[lumpname] = std::move(map);
	}

	auto cuptojson = [](cupwindata_t *windata)
	{
		srb2::GamedataCupJson cupdata {};

		for (size_t i = 0; i < KARTGP_MAX; i++)
		{
			srb2::GamedataCupRecordsJson newrecords {};

			newrecords.bestgrade = windata[i].best_grade;
			newrecords.bestplacement = windata[i].best_placement;
			skinreference_t& skinref = windata[i].best_skin;
			if (skinref.unloaded)
			{
				newrecords.bestskin = String(skinref.unloaded->name);
			}
			else if (skinref.id < numskins)
			{
				newrecords.bestskin = String(skins[skinref.id]->name);
			}
			newrecords.gotemerald = windata[i].got_emerald;

			cupdata.records.emplace_back(std::move(newrecords));
		}

		return cupdata;
	};

	for (auto cup = kartcupheaders; cup; cup = cup->next)
	{
		if (cup->windata[0].best_placement == 0 && cup->windata[1].got_emerald == false)
		{
			continue;
		}

		auto cupdata = cuptojson(cup->windata);
		cupdata.name = String(cup->name);
		ng.cups[cupdata.name] = std::move(cupdata);
	}
	for (auto unloadedcup = unloadedcupheaders; unloadedcup; unloadedcup = unloadedcup->next)
	{
		if (unloadedcup->windata[0].best_placement == 0)
		{
			continue;
		}

		auto cupdata = cuptojson(unloadedcup->windata);
		cupdata.name = String(unloadedcup->name);
		ng.cups[cupdata.name] = std::move(cupdata);
	}

	for (int i = 0; (i < GDMAX_SEALEDSWAPS && gamedata->sealedswaps[i]); i++)
	{
		srb2::GamedataSealedSwapJson sealedswap {};

		cupheader_t* cup = gamedata->sealedswaps[i];

		sealedswap.name = String(cup->name);

		ng.sealedswaps.emplace_back(std::move(sealedswap));
	}

	String gamedataname_s {gamedatafilename};
	String savepath_string = srb2::format("{}/{}", srb2home, gamedataname_s);
	String baksavepath_string = srb2::format("{}/{}.bak", srb2home, gamedataname_s);
	fs::path savepath { static_cast<std::string_view>(savepath_string) };
	fs::path baksavepath { static_cast<std::string_view>(srb2::format("{}/{}.bak", srb2home, gamedataname_s)) };

	JsonValue ngdata_json { JsonObject() };
	to_json(ngdata_json, ng);

	if (fs::exists(savepath))
	{
		try
		{
			fs::rename(savepath, baksavepath);
		}
		catch (const fs::filesystem_error& ex)
		{
			CONS_Alert(CONS_ERROR, "Failed to record backup save. Not attempting to save. %s\n", ex.what());
			return;
		}
	}

	try
	{
		String savepathstring = savepath.string();
		srb2::io::FileStream file {savepathstring, srb2::io::FileStreamMode::kWrite};

		// The header is necessary to validate during loading.
		srb2::io::write(static_cast<uint32_t>(GD_VERSION_MAJOR), file); // major
		srb2::io::write(static_cast<uint8_t>(GD_VERSION_MINOR), file); // minor/flags
		srb2::io::write(static_cast<uint8_t>(gamedata->evercrashed), file); // dirty (crash recovery)

		srb2::Vector<std::byte> ubjson = ngdata_json.to_ubjson();
		srb2::io::write_exact(file, tcb::as_bytes(tcb::make_span(ubjson)));
		file.close();
	}
	catch (const std::exception& ex)
	{
		CONS_Alert(CONS_ERROR, "NG Gamedata save failed. Check directory for a ringdata.dat.bak. %s\n", ex.what());
	}
	catch (...)
	{
		CONS_Alert(CONS_ERROR, "NG Gamedata save failed. Check directory for a ringdata.dat.bak.\n");
	}
}

// G_SaveGameData
// Saves the main data file, which stores information such as emblems found, etc.
void G_SaveGameData(void)
{
	try
	{
		srb2::save_ng_gamedata();
	}
	catch (...)
	{
		CONS_Alert(CONS_ERROR, "Gamedata save failed\n");
		return;
	}

	// Also save profiles here.
	PR_SaveProfiles();

	#ifdef DEVELOP
		CONS_Alert(CONS_NOTICE, M_GetText("Gamedata saved.\n"));
	#endif
}

static const char *G_GameDataFolder(void)
{
	if (strcmp(srb2home,"."))
		return srb2home;
	else
		return "the Ring Racers folder";
}

void srb2::load_ng_gamedata()
{
	// Stop saving, until we successfully load it again.
	gamedata->loaded = false;

	// Clear things so previously read gamedata doesn't transfer
	// to new gamedata
	// see also M_EraseDataResponse
	G_ClearRecords(); // records
	M_ClearStats(); // statistics
	M_ClearSecrets(); // emblems, unlocks, maps visited, etc

	if (M_CheckParm("-nodata"))
	{
		// Don't load at all.
		// The following used to be in M_ClearSecrets, but that was silly.
		M_UpdateUnlockablesAndExtraEmblems(false, true);
		M_FinaliseGameData();
		gamedata->loaded = false;
		return;
	}

	if (M_CheckParm("-resetdata"))
	{
		// Don't load, but do save. (essentially, reset)
		M_FinaliseGameData();
		gamedata->loaded = true;
		return;
	}

	String datapath {srb2::format("{}/{}", srb2home, gamedatafilename)};

	srb2::io::BufferedInputStream<srb2::io::FileStream> bis;
	try
	{
		srb2::io::FileStream file {datapath, srb2::io::FileStreamMode::kRead };
		bis = srb2::io::BufferedInputStream(std::move(file));
	}
	catch (const srb2::io::FileStreamException& ex)
	{
		M_FinaliseGameData();
		gamedata->loaded = true;
		return;
	}

	uint32_t majorversion;
	uint8_t minorversion;
	uint8_t dirty;
	bool converted = false;
	try
	{
		majorversion = srb2::io::read_uint32(bis);
		minorversion = srb2::io::read_uint8(bis);
		dirty = srb2::io::read_uint8(bis);
	}
	catch (...)
	{
		CONS_Alert(CONS_ERROR, "Failed to read ng gamedata header\n");
		return;
	}

	if (majorversion != GD_VERSION_MAJOR)
	{
		const char* gdfolder = G_GameDataFolder();
		I_Error("Game data is not for Ring Racers v2.0.\nDelete %s (maybe in %s) and try again.", gamedatafilename, gdfolder);
		return;
	}

	srb2::Vector<std::byte> remainder = srb2::io::read_to_vec(bis);

	GamedataJson js;
	try
	{
		JsonValue parsed = JsonValue::from_ubjson(remainder);
		from_json(parsed, js);
	}
	catch (const std::exception& ex)
	{
		const char* gdfolder = G_GameDataFolder();
		const char* what = ex.what();
		I_Error("Game data is corrupt.\nDelete %s (maybe in %s) and try again.\n\nException: %s", gamedatafilename, gdfolder, what);
		return;
	}
	catch (...)
	{
		const char* gdfolder = G_GameDataFolder();
		I_Error("Game data is corrupt.\nDelete %s (maybe in %s) and try again.", gamedatafilename, gdfolder);
		return;
	}

	// Quick & dirty hash for what mod this save file is for.
	if (js.tafolderhash != quickncasehash(timeattackfolder, 64))
	{
		const char* gdfolder = G_GameDataFolder();
		I_Error("Game data is corrupt.\nDelete %s (maybe in %s) and try again.", gamedatafilename, gdfolder);
		return;
	}

	// Now we extract the json struct's data and put it into the C-side gamedata.

	gamedata->evercrashed = dirty;

	gamedata->totalplaytime = js.playtime.total;
	gamedata->totalnetgametime = js.playtime.netgame;
	gamedata->timeattackingtotaltime = js.playtime.timeattack;
	gamedata->spbattackingtotaltime = js.playtime.spbattack;
	gamedata->modeplaytime[GDGT_RACE] = js.playtime.race;
	gamedata->modeplaytime[GDGT_BATTLE] = js.playtime.battle;
	gamedata->modeplaytime[GDGT_PRISONS] = js.playtime.prisons;
	gamedata->modeplaytime[GDGT_SPECIAL] = js.playtime.special;
	gamedata->modeplaytime[GDGT_CUSTOM] = js.playtime.custom;
	gamedata->totalmenutime = js.playtime.menus;
	gamedata->totaltimestaringatstatistics = js.playtime.statistics;
	gamedata->totalrings = js.rings.total;
	gamedata->totaltumbletime = js.playtime.tumble;
	gamedata->roundsplayed[GDGT_RACE] = js.rounds.race;
	gamedata->roundsplayed[GDGT_BATTLE] = js.rounds.battle;
	gamedata->roundsplayed[GDGT_PRISONS] = js.rounds.prisons;
	gamedata->roundsplayed[GDGT_SPECIAL] = js.rounds.special;
	gamedata->roundsplayed[GDGT_CUSTOM] = js.rounds.custom;
	gamedata->pendingkeyrounds = js.challengekeys.pendingkeyrounds;
	gamedata->pendingkeyroundoffset = js.challengekeys.pendingkeyroundoffset;
	gamedata->keyspending = js.challengekeys.keyspending;
	gamedata->chaokeys = js.challengekeys.chaokeys;
	gamedata->everloadedaddon = js.milestones.everloadedaddon;
	gamedata->everfinishedcredits = js.milestones.everfinishcredits;
	gamedata->eversavedreplay = js.milestones.eversavedreplay;
	gamedata->everseenspecial = js.milestones.everseenspecial;
	gamedata->chaokeytutorial = js.milestones.chaokeytutorial;
	gamedata->majorkeyskipattempted = js.milestones.majorkeyskipattempted;
	gamedata->finishedtutorialchallenge = js.milestones.finishedtutorialchallenge;
	gamedata->enteredtutorialchallenge = js.milestones.enteredtutorialchallenge;
	gamedata->sealedswapalerted = js.milestones.sealedswapalerted;
	gamedata->tutorialdone = js.milestones.tutorialdone;
	gamedata->playgroundroute = js.milestones.playgroundroute;
	gamedata->gonerlevel = js.milestones.gonerlevel;
	gamedata->thisprisoneggpickup = js.prisons.thisprisoneggpickup;

	gamedata->prisoneggstothispickup = js.prisons.prisoneggstothispickup;
	if (gamedata->prisoneggstothispickup > GDINIT_PRISONSTOPRIZE)
	{
		gamedata->prisoneggstothispickup = GDINIT_PRISONSTOPRIZE;
	}

	size_t emblems_size = js.emblems.size();
	for (size_t i = 0; i < std::min((size_t)MAXEMBLEMS, emblems_size); i++)
	{
		gamedata->collected[i] = js.emblems[i];
	}

	size_t unlocks_size = js.unlockables.size();
	for (size_t i = 0; i < std::min((size_t)MAXUNLOCKABLES, unlocks_size); i++)
	{
		gamedata->unlocked[i] = js.unlockables[i];
	}

	size_t pending_unlocks_size = js.unlockpending.size();
	for (size_t i = 0; i < std::min((size_t)MAXUNLOCKABLES, pending_unlocks_size); i++)
	{
		gamedata->unlockpending[i] = js.unlockpending[i];
	}

	size_t conditions_size = js.conditionsets.size();
	for (size_t i = 0; i < std::min((size_t)MAXCONDITIONSETS, conditions_size); i++)
	{
		gamedata->achieved[i] = js.conditionsets[i];
	}

#ifdef DEVELOP
	if (!M_CheckParm("-resetchallengegrid"))
#endif
	{
		gamedata->challengegridwidth = std::max(js.challengegrid.width, (uint32_t)0);
		if (gamedata->challengegridwidth)
		{
			gamedata->challengegrid = static_cast<uint16_t*>(Z_Malloc(
				(gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT * sizeof(UINT16)),
				PU_STATIC, NULL));
			for (size_t i = 0; i < std::min<size_t>((size_t)(gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT), js.challengegrid.grid.size()); i++)
			{
				uint16_t gridvalue = js.challengegrid.grid[i];
				gamedata->challengegrid[i] = gridvalue;
			}

			M_SanitiseChallengeGrid();
		}
	}

	gamedata->timesBeaten = js.timesBeaten;

	// Main records

	for (auto& skinpair : js.skins)
	{
		INT32 skin = R_SkinAvailableEx(skinpair.first.c_str(), false);
		skinrecord_t dummyrecord {};

		dummyrecord.wins = skinpair.second.records.wins;
		dummyrecord.rounds = skinpair.second.records.rounds;

		// Used to be only for testing, but then there was a bug in release builds! Now conversion only
		if (minorversion < 2 && dummyrecord.rounds < dummyrecord.wins)
		{
			dummyrecord.rounds = dummyrecord.wins;
			converted = true;
		}

		dummyrecord.timeplayed = skinpair.second.records.time.total;
		dummyrecord.modetimeplayed[GDGT_RACE] = skinpair.second.records.time.race;
		dummyrecord.modetimeplayed[GDGT_BATTLE] = skinpair.second.records.time.battle;
		dummyrecord.modetimeplayed[GDGT_PRISONS] = skinpair.second.records.time.prisons;
		dummyrecord.modetimeplayed[GDGT_SPECIAL] = skinpair.second.records.time.special;
		dummyrecord.modetimeplayed[GDGT_CUSTOM] = skinpair.second.records.time.custom;
		dummyrecord.tumbletime = skinpair.second.records.time.tumble;

		if (skin != -1)
		{
			skins[skin]->records = dummyrecord;
		}
		else if (dummyrecord.wins)
		{
			// Invalid, but we don't want to lose all the juicy statistics.
			// Instead, update a FILO linked list of "unloaded skins".

			unloaded_skin_t *unloadedskin =
				static_cast<unloaded_skin_t*>(Z_Malloc(
					sizeof(unloaded_skin_t),
					PU_STATIC, NULL
				));

			// Establish properties, for later retrieval on file add.
			strlcpy(unloadedskin->name, skinpair.first.c_str(), sizeof(unloadedskin->name));
			unloadedskin->namehash = quickncasehash(unloadedskin->name, SKINNAMESIZE);

			// Insert at the head, just because it's convenient.
			unloadedskin->next = unloadedskins;
			unloadedskins = unloadedskin;

			// Finally, copy into.
			unloadedskin->records = dummyrecord;
		}
	}

	std::vector<candata_t> tempcans;

#ifdef DEVELOP
	if (M_CheckParm("-resetspraycans"))
		;
	else
#endif
	for (auto& cancolor : js.spraycans_v2)
	{
		// Version 2 behaviour - spraycans_v2, not spraycans!

		candata_t tempcan;
		tempcan.col = SKINCOLOR_NONE;
		tempcan.map = NEXTMAP_INVALID;

		// Find the skin color index for the name
		for (size_t i = 0; i < SKINCOLOR_FIRSTFREESLOT; i++)
		{
			if (cancolor != skincolors[i].name)
				continue;

			tempcan.col = i;
			break;
		}

		tempcans.emplace_back(std::move(tempcan));
	}

	for (auto& mappair : js.maps)
	{
		uint16_t mapnum = G_MapNumber(mappair.first.c_str());
		recorddata_t dummyrecord {};
		dummyrecord.mapvisited |= mappair.second.visited.visited ? MV_VISITED : 0;
		dummyrecord.mapvisited |= mappair.second.visited.beaten ? MV_BEATEN : 0;
		dummyrecord.mapvisited |= mappair.second.visited.encore ? MV_ENCORE : 0;
		dummyrecord.mapvisited |= mappair.second.visited.spbattack ? MV_SPBATTACK : 0;
		dummyrecord.mapvisited |= mappair.second.visited.mysticmelody ? MV_MYSTICMELODY : 0;

		if (minorversion >= GD_MINIMUM_TIMEATTACKV2)
		{
			dummyrecord.timeattack.time = mappair.second.stats.timeattack.besttime;
			dummyrecord.timeattack.lap = mappair.second.stats.timeattack.bestlap;
			dummyrecord.spbattack.time = mappair.second.stats.spbattack.besttime;
			dummyrecord.spbattack.lap = mappair.second.stats.spbattack.bestlap;
		}
		else
		{
			converted = true;

			dummyrecord.timeattack.time = dummyrecord.timeattack.lap = \
			dummyrecord.spbattack.time = dummyrecord.spbattack.lap = 0;
		}

		dummyrecord.timeplayed = mappair.second.stats.time.total;
		dummyrecord.netgametimeplayed = mappair.second.stats.time.netgame;
		dummyrecord.modetimeplayed[GDGT_RACE] = mappair.second.stats.time.race;
		dummyrecord.modetimeplayed[GDGT_BATTLE] = mappair.second.stats.time.battle;
		dummyrecord.modetimeplayed[GDGT_PRISONS] = mappair.second.stats.time.prisons;
		dummyrecord.modetimeplayed[GDGT_SPECIAL] = mappair.second.stats.time.special;
		dummyrecord.modetimeplayed[GDGT_CUSTOM] = mappair.second.stats.time.custom;
		dummyrecord.timeattacktimeplayed = mappair.second.stats.time.timeattack;
		dummyrecord.spbattacktimeplayed = mappair.second.stats.time.spbattack;

		dummyrecord.spraycan = (minorversion >= GD_MINIMUM_SPRAYCANSV2)
			? mappair.second.spraycan
			: MCAN_INVALID;

		if (mapnum < nummapheaders && mapheaderinfo[mapnum])
		{
			// Valid mapheader, time to populate with record data.

			// Infill Spray Can info
			if (
				dummyrecord.spraycan < tempcans.size()
				&& (mapnum < basenummapheaders)
				&& (tempcans[dummyrecord.spraycan].map >= basenummapheaders)
			)
			{
				// Assign map ID.
				tempcans[dummyrecord.spraycan].map = mapnum;
			}

			if (dummyrecord.spraycan != MCAN_INVALID)
			{
				// Yes, even if it's valid. We reassign later.
				dummyrecord.spraycan = MCAN_BONUS;
			}

			mapheaderinfo[mapnum]->records = dummyrecord;
		}
		else if (dummyrecord.mapvisited & (MV_VISITED|MV_BEATEN)
		|| dummyrecord.timeattack.time != 0 || dummyrecord.timeattack.lap != 0
		|| dummyrecord.spbattack.time != 0 || dummyrecord.spbattack.lap != 0
		|| dummyrecord.spraycan != MCAN_INVALID)
		{
			// Invalid, but we don't want to lose all the juicy statistics.
			// Instead, update a FILO linked list of "unloaded mapheaders".

			unloaded_mapheader_t *unloadedmap =
				static_cast<unloaded_mapheader_t*>(Z_Malloc(
					sizeof(unloaded_mapheader_t),
					PU_STATIC, NULL
				));

			// Establish properties, for later retrieval on file add.
			unloadedmap->lumpname = Z_StrDup(mappair.first.c_str());
			unloadedmap->lumpnamehash = quickncasehash(unloadedmap->lumpname, MAXMAPLUMPNAME);

			// Insert at the head, just because it's convenient.
			unloadedmap->next = unloadedmapheaders;
			unloadedmapheaders = unloadedmap;

			if (dummyrecord.spraycan != MCAN_INVALID)
			{
				// Invalidate non-bonus spraycans.
				dummyrecord.spraycan = MCAN_BONUS;
			}

			// Finally, copy into.
			unloadedmap->records = dummyrecord;
		}
	}

	if ((minorversion < GD_MINIMUM_SPRAYCANSV2) && (js.spraycans.size() > 1))
	{
		// Deprecated behaviour! Look above for spraycans_v2 handling

		converted = true;

		for (auto& deprecatedcan : js.spraycans)
		{
			candata_t tempcan;
			tempcan.col = SKINCOLOR_NONE;

			// Find the skin color index for the name
			for (size_t i = 0; i < SKINCOLOR_FIRSTFREESLOT; i++)
			{
				if (deprecatedcan.color != skincolors[i].name)
					continue;

				tempcan.col = i;
				break;
			}

			UINT16 mapnum = NEXTMAP_INVALID;
			if (!deprecatedcan.map.empty())
			{
				mapnum = G_MapNumber(deprecatedcan.map.c_str());
			}
			tempcan.map = mapnum;

			tempcans.emplace_back(std::move(tempcan));
		}
	}

	{
		// Post-process of Spray Cans - a component of both v1 and v2 spraycans behaviour

		// Determine sizes.
		for (auto& tempcan : tempcans)
		{
			if (tempcan.col == SKINCOLOR_NONE)
				continue;

			gamedata->numspraycans++;

			if (tempcan.map >= nummapheaders)
				continue;

			gamedata->gotspraycans++;
		}

		if (gamedata->numspraycans)
		{
			// Arrange with collected first
			std::stable_sort(tempcans.begin(), tempcans.end(), [ ]( auto& lhs, auto& rhs )
			{
			   return (rhs.map >= basenummapheaders && lhs.map < basenummapheaders);
			});

			gamedata->spraycans = static_cast<candata_t*>(Z_Malloc(
				(gamedata->numspraycans * sizeof(candata_t)),
				PU_STATIC, NULL));

			// Finally, fill can data.
			size_t i = 0;
			for (auto& tempcan : tempcans)
			{
				if (tempcan.col == SKINCOLOR_NONE)
					continue;

				skincolors[tempcan.col].cache_spraycan = i;

				if (tempcan.map < basenummapheaders)
					mapheaderinfo[tempcan.map]->records.spraycan = i;

				gamedata->spraycans[i] = tempcan;

				if (++i < gamedata->numspraycans)
					continue;

				break;
			}
		}
	}

	for (auto& cuppair : js.cups)
	{
		std::array<cupwindata_t, KARTGP_MAX> dummywindata {{}};
		cupheader_t* cup = nullptr;

		// Find the loaded cup
		for (cup = kartcupheaders; cup; cup = cup->next)
		{
			String cupname { cup->name };
			if (cupname == cuppair.first)
			{
				break;
			}
		}

		// Digest its data...
		for (size_t j = 0; j < std::min<size_t>(KARTGP_MAX, cuppair.second.records.size()); j++)
		{
			dummywindata[j].best_placement = cuppair.second.records[j].bestplacement;
			dummywindata[j].best_grade = static_cast<gp_rank_e>(cuppair.second.records[j].bestgrade);
			dummywindata[j].got_emerald = cuppair.second.records[j].gotemerald;

			dummywindata[j].best_skin.id = MAXSKINS;
			dummywindata[j].best_skin.unloaded = nullptr;

			int skinloaded = R_SkinAvailableEx(cuppair.second.records[j].bestskin.c_str(), false);
			if (skinloaded >= 0)
			{
				dummywindata[j].best_skin.id = skinloaded;
				continue;
			}
			for (auto unloadedskin = unloadedskins; unloadedskin; unloadedskin = unloadedskin->next)
			{
				String skinname { unloadedskin->name };
				if (skinname == cuppair.second.records[j].bestskin)
				{
					skinreference_t ref {};
					ref.id = MAXSKINS;
					ref.unloaded = unloadedskin;
					dummywindata[j].best_skin = ref;
					break;
				}
			}
		}

		if (cup)
		{
			// We found a cup, so assign the windata.

			memcpy(cup->windata, dummywindata.data(), sizeof(cup->windata));
		}
		else if (dummywindata[0].best_placement != 0)
		{
			// Invalid, but we don't want to lose all the juicy statistics.
			// Instead, update a FILO linked list of "unloaded cupheaders".

			unloaded_cupheader_t *unloadedcup =
				static_cast<unloaded_cupheader_t*>(Z_Malloc(
					sizeof(unloaded_cupheader_t),
					PU_STATIC, NULL
				));

			// Establish properties, for later retrieval on file add.
			strlcpy(unloadedcup->name, cuppair.first.c_str(), sizeof(unloadedcup->name));
			unloadedcup->namehash = quickncasehash(unloadedcup->name, MAXCUPNAME);

			// Insert at the head, just because it's convenient.
			unloadedcup->next = unloadedcupheaders;
			unloadedcupheaders = unloadedcup;

			// Finally, copy into.
			memcpy(unloadedcup->windata, dummywindata.data(), sizeof(cup->windata));
		}
	}

	size_t sealedswaps_size = js.sealedswaps.size();
	for (size_t i = 0; i < std::min((size_t)GDMAX_SEALEDSWAPS, sealedswaps_size); i++)
	{
		cupheader_t* cup = nullptr;

		// Find BASE cups only
		for (cup = kartcupheaders; cup; cup = cup->next)
		{
			if (cup->id >= basenumkartcupheaders)
			{
				cup = NULL;
				break;
			}

			String cupname { cup->name };
			if (cupname == js.sealedswaps[i].name)
			{
				break;
			}
		}

		if (cup)
		{
			gamedata->sealedswaps[i] = cup;
		}
	}

	UINT32 chao_key_rounds = GDCONVERT_ROUNDSTOKEY;
	UINT32 start_keys = GDINIT_CHAOKEYS;

	if (minorversion == 0)
	{
		chao_key_rounds = 14;
		start_keys = 3;
	}

	if (chao_key_rounds != GDCONVERT_ROUNDSTOKEY)
	{
		// Chao key rounds changed.
		// Just reset all round progress, because there is a dumbass
		// bug that can cause infinite chao keys from loading.
		gamedata->pendingkeyrounds = 0;
		gamedata->pendingkeyroundoffset = 0;
		gamedata->keyspending = 0;
		converted = true;
	}

	if (GDINIT_CHAOKEYS > start_keys)
	{
		// Chao key starting amount changed.
		// Give some free keys!
		gamedata->chaokeys += GDINIT_CHAOKEYS - start_keys;
		converted = true;
	}

	if (minorversion < GD_MINIMUM_TUTORIALLOCK && gamedata->gonerlevel >= GDGONER_DONE)
	{
		converted = true;
		uint16_t checklocks[] = {751, 752, 754}; // Brakes, Drifting, Springs
		for (uint16_t checklock : checklocks)
		{
			gamedata->unlocked[checklock - 1] = true;
		}
	}

	M_FinaliseGameData();

	if (converted)
	{
		CONS_Printf("Gamedata was converted from version %d to version %d\n",
			minorversion, GD_VERSION_MINOR);
	}
}

// G_LoadGameData
// Loads the main data file, which stores information such as emblems found, etc.
void G_LoadGameData(void)
{
	try
	{
		srb2::load_ng_gamedata();
	}
	catch (...)
	{
		CONS_Alert(CONS_ERROR, "NG Gamedata loading failed\n");
	}
}
