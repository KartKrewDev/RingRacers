// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_G_GAMEDATA_H
#define SRB2_G_GAMEDATA_H

#ifdef __cplusplus

#include <cstdint>

#include "core/json.hpp"
#include "core/hash_map.hpp"
#include "core/string.h"
#include "core/vector.hpp"

namespace srb2
{

struct GamedataPlaytimeJson final
{
	uint32_t total;
	uint32_t netgame;
	uint32_t timeattack;
	uint32_t spbattack;
	uint32_t race;
	uint32_t battle;
	uint32_t prisons;
	uint32_t special;
	uint32_t custom;
	uint32_t menus;
	uint32_t statistics;
	uint32_t tumble;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
		GamedataPlaytimeJson,
		total,
		netgame,
		timeattack,
		spbattack,
		race,
		battle,
		prisons,
		special,
		custom,
		menus,
		statistics,
		tumble
	)
};

struct GamedataRingsJson final
{
	uint32_t total;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataRingsJson, total)
};

struct GamedataRoundsJson final
{
	uint32_t race;
	uint32_t battle;
	uint32_t prisons;
	uint32_t special;
	uint32_t custom;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataRoundsJson, race, battle, prisons, special, custom)
};

struct GamedataChallengeKeysJson final
{
	uint32_t pendingkeyrounds;
	uint8_t pendingkeyroundoffset;
	uint16_t keyspending;
	uint16_t chaokeys;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataChallengeKeysJson, pendingkeyrounds, pendingkeyroundoffset, keyspending, chaokeys)
};

struct GamedataMilestonesJson final
{
	uint32_t gonerlevel;
	bool everloadedaddon;
	bool everfinishcredits;
	bool eversavedreplay;
	bool everseenspecial;
	bool chaokeytutorial;
	bool majorkeyskipattempted;
	bool finishedtutorialchallenge;
	bool enteredtutorialchallenge;
	bool sealedswapalerted;
	bool tutorialdone;
	bool playgroundroute;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
		GamedataMilestonesJson,
		gonerlevel,
		everloadedaddon,
		everfinishcredits,
		eversavedreplay,
		everseenspecial,
		chaokeytutorial,
		majorkeyskipattempted,
		finishedtutorialchallenge,
		enteredtutorialchallenge,
		sealedswapalerted,
		tutorialdone,
		playgroundroute
	)
};

struct GamedataPrisonEggPickupsJson final
{
	uint16_t thisprisoneggpickup;
	uint16_t prisoneggstothispickup;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataPrisonEggPickupsJson, thisprisoneggpickup, prisoneggstothispickup)
};

struct GamedataChallengeGridJson final
{
	uint32_t width;
	Vector<uint16_t> grid;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataChallengeGridJson, width, grid)
};

struct GamedataSkinRecordsPlaytimeJson final
{
	uint32_t total;
	uint32_t race;
	uint32_t battle;
	uint32_t prisons;
	uint32_t special;
	uint32_t custom;
	uint32_t tumble;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
		GamedataSkinRecordsPlaytimeJson,
		total,
		race,
		battle,
		prisons,
		special,
		custom,
		tumble
	)
};

struct GamedataSkinRecordsJson final
{
	uint32_t wins;
	uint32_t rounds;
	GamedataSkinRecordsPlaytimeJson time;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
		GamedataSkinRecordsJson,
		wins,
		rounds,
		time
	)
};

struct GamedataSkinJson final
{
	GamedataSkinRecordsJson records;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataSkinJson, records)
};

struct GamedataMapVisitedJson final
{
	bool visited;
	bool beaten;
	bool encore;
	bool spbattack;
	bool mysticmelody;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataMapVisitedJson, visited, beaten, encore, spbattack, mysticmelody)
};

struct GamedataMapStatsTimeAttackJson final
{
	uint32_t besttime;
	uint32_t bestlap;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataMapStatsTimeAttackJson, besttime, bestlap)
};

struct GamedataMapStatsSpbAttackJson final
{
	uint32_t besttime;
	uint32_t bestlap;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataMapStatsSpbAttackJson, besttime, bestlap)
};

struct GamedataMapStatsPlaytimeJson final
{
	uint32_t total;
	uint32_t netgame;
	uint32_t race;
	uint32_t battle;
	uint32_t prisons;
	uint32_t special;
	uint32_t custom;
	uint32_t timeattack;
	uint32_t spbattack;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
		GamedataMapStatsPlaytimeJson,
		total,
		netgame,
		race,
		battle,
		prisons,
		special,
		custom,
		timeattack,
		spbattack
	)
};

struct GamedataMapStatsJson final
{
	GamedataMapStatsTimeAttackJson timeattack;
	GamedataMapStatsSpbAttackJson spbattack;
	GamedataMapStatsPlaytimeJson time;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
		GamedataMapStatsJson,
		timeattack,
		spbattack,
		time
	)
};

struct GamedataMapJson final
{
	GamedataMapVisitedJson visited;
	GamedataMapStatsJson stats;
	uint16_t spraycan;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataMapJson, visited, stats, spraycan)
};

// Deprecated
struct GamedataSprayCanJson final
{
	String map;
	String color;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataSprayCanJson, map, color)
};

struct GamedataCupRecordsJson final
{
	uint8_t bestplacement;
	uint8_t bestgrade;
	bool gotemerald;
	String bestskin;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataCupRecordsJson, bestplacement, bestgrade, gotemerald, bestskin)
};

struct GamedataCupJson final
{
	String name;
	Vector<GamedataCupRecordsJson> records;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataCupJson, name, records)
};

struct GamedataSealedSwapJson final
{
	String name;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataSealedSwapJson, name)
};

struct GamedataJson final
{
	GamedataPlaytimeJson playtime;
	GamedataRingsJson rings;
	GamedataRoundsJson rounds;
	GamedataChallengeKeysJson challengekeys;
	GamedataMilestonesJson milestones;
	GamedataPrisonEggPickupsJson prisons;
	uint32_t tafolderhash;
	Vector<bool> emblems;
	Vector<bool> unlockables;
	Vector<bool> unlockpending;
	Vector<bool> conditionsets;
	GamedataChallengeGridJson challengegrid;
	uint32_t timesBeaten;
	HashMap<String, GamedataSkinJson> skins;
	Vector<String> spraycans_v2;
	HashMap<String, GamedataMapJson> maps;
	Vector<GamedataSprayCanJson> spraycans; // Deprecated
	HashMap<String, GamedataCupJson> cups;
	Vector<GamedataSealedSwapJson> sealedswaps;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
		GamedataJson,
		playtime,
		rings,
		rounds,
		challengekeys,
		milestones,
		prisons,
		tafolderhash,
		emblems,
		unlockables,
		unlockpending,
		conditionsets,
		challengegrid,
		timesBeaten,
		skins,
		spraycans_v2,
		maps,
		spraycans,
		cups,
		sealedswaps
	)
};

void save_ng_gamedata(void);
void load_ng_gamedata(void);

}

extern "C"
{
#endif // __cplusplus

void G_SaveGameData(void);
void G_LoadGameData(void);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // SRB2_G_GAMEDATA_H
