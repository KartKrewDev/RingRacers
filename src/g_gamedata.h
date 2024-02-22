// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_G_GAMEDATA_H
#define SRB2_G_GAMEDATA_H

#ifdef __cplusplus

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace srb2
{

struct GamedataPlaytimeJson final
{
	uint32_t total;
	uint32_t race;
	uint32_t battle;
	uint32_t prisons;
	uint32_t special;
	uint32_t custom;
	uint32_t tumble;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
		GamedataPlaytimeJson,
		total,
		race,
		battle,
		prisons,
		special,
		custom,
		tumble
	)
};

struct GamedataRingsJson final
{
	uint32_t total;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataRingsJson, total)
};

struct GamedataRoundsJson final
{
	uint32_t race;
	uint32_t battle;
	uint32_t prisons;
	uint32_t special;
	uint32_t custom;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataRoundsJson, race, battle, prisons, special, custom)
};

struct GamedataChallengeKeysJson final
{
	uint32_t pendingkeyrounds;
	uint8_t pendingkeyroundoffset;
	uint16_t keyspending;
	uint16_t chaokeys;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataChallengeKeysJson, pendingkeyrounds, pendingkeyroundoffset, keyspending, chaokeys)
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

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
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
		sealedswapalerted
	)
};

struct GamedataPrisonEggPickupsJson final
{
	uint16_t thisprisoneggpickup;
	uint16_t prisoneggstothispickup;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataPrisonEggPickupsJson, thisprisoneggpickup, prisoneggstothispickup)
};

struct GamedataChallengeGridJson final
{
	uint32_t width;
	std::vector<uint16_t> grid;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataChallengeGridJson, width, grid)
};

struct GamedataSkinRecordsJson final
{
	uint32_t wins;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataSkinRecordsJson, wins)
};

struct GamedataSkinJson final
{
	GamedataSkinRecordsJson records;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataSkinJson, records)
};

struct GamedataMapVisitedJson final
{
	bool visited;
	bool beaten;
	bool encore;
	bool spbattack;
	bool mysticmelody;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataMapVisitedJson, visited, beaten, encore, spbattack, mysticmelody)
};

struct GamedataMapStatsTimeAttackJson final
{
	uint32_t besttime;
	uint32_t bestlap;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataMapStatsTimeAttackJson, besttime, bestlap)
};

struct GamedataMapStatsSpbAttackJson final
{
	uint32_t besttime;
	uint32_t bestlap;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataMapStatsSpbAttackJson, besttime, bestlap)
};

struct GamedataMapStatsJson final
{
	GamedataMapStatsTimeAttackJson timeattack;
	GamedataMapStatsSpbAttackJson spbattack;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataMapStatsJson, timeattack, spbattack)
};

struct GamedataMapJson final
{
	GamedataMapVisitedJson visited;
	GamedataMapStatsJson stats;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataMapJson, visited, stats)
};

struct GamedataSprayCanJson final
{
	std::string map;
	std::string color;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataSprayCanJson, map, color)
};

struct GamedataCupRecordsJson final
{
	uint8_t bestplacement;
	uint8_t bestgrade;
	bool gotemerald;
	std::string bestskin;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataCupRecordsJson, bestplacement, bestgrade, gotemerald, bestskin)
};

struct GamedataCupJson final
{
	std::string name;
	std::vector<GamedataCupRecordsJson> records;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataCupJson, name, records)
};

struct GamedataSealedSwapJson final
{
	std::string name;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GamedataSealedSwapJson, name)
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
	std::vector<bool> emblems;
	std::vector<bool> unlockables;
	std::vector<bool> unlockpending;
	std::vector<bool> conditionsets;
	GamedataChallengeGridJson challengegrid;
	uint32_t timesBeaten;
	std::unordered_map<std::string, GamedataSkinJson> skins;
	std::unordered_map<std::string, GamedataMapJson> maps;
	std::vector<GamedataSprayCanJson> spraycans;
	std::unordered_map<std::string, GamedataCupJson> cups;
	std::vector<GamedataSealedSwapJson> sealedswaps;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
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
