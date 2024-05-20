// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by James Robert Roman
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <vector>

//
// Don't include anything here unless it's absolutely needed.
// If you need CV_PossibleValue_t arrays or "OnChange"
// functions, don't include headers -- extern them manually.
// Keep the includes short to minimize recompiles dependent on
// other parts of the codebase.
//
// If your cvar's possible values depend on constants defined
// in some headers, don't include it. Just define the values
// array in another file and extern it from here.
//
// Look at the - HOW TO ORGANIZE - section before changing or
// adding any cvars.
//

#include "command.h"
#include "doomdef.h"
#include "doomtype.h"
#include "m_fixed.h" // FRACUNIT
#include "r_skins.h" // DEFAULTSKIN

// There is a memset in one of consvar_t's constructors. It
// SHOULD be safe if there is no polymorphism, but just
// double-checking.
static_assert(std::is_trivially_copyable_v<consvar_t>);

struct consvar_t::Builder
{
	using values_list_t = std::initializer_list<CV_PossibleValue_t>;

	struct ValuesCollect
	{
		explicit ValuesCollect(int size) : values_{new CV_PossibleValue_t[size]} {}

		void release() { values_ = nullptr; }

		operator CV_PossibleValue_t*() const { return values_; }

		~ValuesCollect()
		{
			if (values_)
			{
				delete[] values_;
			}
		}

	private:
		CV_PossibleValue_t* values_;
	};

	CVarList* list_;
	consvar_t var_ = {nullptr, nullptr, 0, nullptr, nullptr};
	std::shared_ptr<ValuesCollect> values_collect_;

	explicit Builder(CVarList* list) : list_(list) {}

	Builder operator ()() const { return *this; }

	Builder operator ()(CVarList* list) const
	{
		Builder builder = *this;
		builder.list_ = list;
		return builder;
	}

	Builder operator ()(const char* name, const char* default_value) const
	{
		Builder builder = *this;
		builder.var_.name = name;
		builder.var_.defaultvalue = default_value;
		return builder;
	}

	Builder& description(const char* description)
	{
		var_.description = description;
		return *this;
	}

	Builder& step_amount(INT32 step_amount)
	{
		var_.step_amount = step_amount;
		return *this;
	}

	Builder& save()
	{
		var_.flags |= CV_SAVE;
		return *this;
	}

	Builder& dont_save()
	{
		var_.flags &= ~(CV_SAVE);
		return *this;
	}

	Builder& network()
	{
		var_.flags |= CV_NETVAR;
		return *this;
	}

	Builder& floating_point()
	{
		var_.flags |= CV_FLOAT;
		return *this;
	}

	Builder& cheat()
	{
		var_.flags |= CV_CHEAT;
		return *this;
	}

	Builder& flags(INT32 flags)
	{
		var_.flags |= flags;
		return *this;
	}

	Builder& on_off() { return values(CV_OnOff); }
	Builder& yes_no() { return values(CV_YesNo); }

	Builder& min_max(INT32 min, INT32 max, values_list_t values = {})
	{
		return combine_values({{min, "MIN"}, {max, "MAX"}}, values);
	}

	Builder& values(values_list_t values) { return combine_values({}, values); }

	Builder& values(CV_PossibleValue_t* values)
	{
		values_collect_ = {};
		var_.PossibleValue = values;
		return *this;
	}

	Builder& onchange(void (*fn)())
	{
		var_.flags |= CV_CALL;
		var_.func = fn;
		return *this;
	}

	Builder& onchange_noinit(void (*fn)())
	{
		var_.flags |= CV_CALL | CV_NOINIT;
		var_.func = fn;
		return *this;
	}

private:
	Builder& combine_values(values_list_t a, values_list_t b)
	{
		values_collect_ = std::make_shared<ValuesCollect>(a.size() + b.size() + 1);
		var_.PossibleValue = *values_collect_;

		std::copy(a.begin(), a.end(), var_.PossibleValue);
		std::copy(b.begin(), b.end(), var_.PossibleValue + a.size());
		var_.PossibleValue[a.size() + b.size()] = {}; // terminator

		return *this;
	}
};

struct CVarList
{
	void add(consvar_t& var) { vec_.push_back(&var); }

	void finish()
	{
		// load is guaranteed to only be called once, even
		// across different instances of CVarList.
		static int once_only = load();
		(void)once_only;

		for (consvar_t* var : vec_)
		{
			CV_RegisterVar(var);
		}

		vec_ = {};
	}

private:
	std::vector<consvar_t*> vec_;

	static int load();
};

consvar_t& consvar_t::operator=(Builder& builder)
{
	*this = builder.var_;

	builder.list_->add(*this);

	// Any memory that was allocated now belongs to the cvar
	// and can no longer be managed by the Builder.
	if (builder.values_collect_)
	{
		builder.values_collect_->release();
	}

	return *this;
}

void CV_RegisterList(CVarList* list)
{
	list->finish();
}

// clang-format off

extern "C" {

#define X(id) CVarList* id = new CVarList()

X (cvlist_player); // not required by dedicated servers
X (cvlist_server); // always required

X (cvlist_command);
X (cvlist_console);
X (cvlist_graphics_driver);
X (cvlist_opengl);
X (cvlist_screen);
X (cvlist_timer);

X (cvlist_execversion);

#ifdef DUMPCONSISTENCY
	X (cvlist_dumpconsistency);
#endif

#undef X

namespace
{

const auto Player = consvar_t::Builder(cvlist_player).save();
const auto Server = consvar_t::Builder(cvlist_server).save();

const auto NetVar = Server().network();
const auto UnsavedNetVar = NetVar().dont_save();

const auto OnlineCheat = UnsavedNetVar().cheat();
const auto ServerCheat = Server().cheat().dont_save();
const auto PlayerCheat = Player().cheat().dont_save();
const auto ObjectPlace = Player().flags(CV_NOTINNET).dont_save();

const auto Console = consvar_t::Builder(cvlist_console).save();
const auto OpenGL = consvar_t::Builder(cvlist_opengl).save();

const auto MenuDummy = Player().flags(CV_HIDDEN).dont_save();

const auto GraphicsDriver = consvar_t::Builder(cvlist_graphics_driver).save();

#ifdef DEVELOP
	const auto AuthDebug = Player().values(CV_Unsigned).dont_save();
#endif

}; // namespace


//
// - HOW TO ORGANIZE -
//
// - Prefer to make a section based on cvars being registered together.
//   - This is not a hard rule, but if a bunch of cvars are going against this rule, then make a new section.
//   - You can make a new section if there are a bunch of cvars following sharing some other kind of similarity.
// - Group cvars together under a CV_PossibleValue_t or "OnChange" function.
//   - You may group them together by some other rule if it makes more sense.
//   - The cvars in each group are alphabetically sorted.
//   - The groups are sorted by the first cvar in each group.
//   - Use the variable name, not the console command name, for sorting.
//


//
// Player local, not available on dedicated servers.
// These usually save...
//

consvar_t cv_addons_md5 = Player("addons_md5", "Name").values({{0, "Name"}, {1, "Contents"}});
consvar_t cv_addons_search_case = Player("addons_search_case", "No").yes_no();
consvar_t cv_addons_search_type = Player("addons_search_type", "Anywhere").values({{0, "Start"}, {1, "Anywhere"}});
consvar_t cv_addons_showall = Player("addons_showall", "No").yes_no();
consvar_t cv_alttitle = Player("alttitle", "Off").on_off();
consvar_t cv_alwaysgrabmouse = GraphicsDriver("alwaysgrabmouse", "Off").on_off();

consvar_t cv_apng_delay = Player("apng_speed", "1x").values({
	{1, "1x"},
	{2, "1/2x"},
	{3, "1/3x"},
	{4, "1/4x"},
});

consvar_t cv_apng_downscale = Player("apng_downscale", "On").on_off();

void Captioning_OnChange(void);
consvar_t cv_closedcaptioning = Player("closedcaptioning", "Off").on_off().onchange(Captioning_OnChange);

consvar_t cv_continuousmusic = Player("continuousmusic", "On").on_off();
consvar_t cv_streamersafemusic = Player("streamersafemusic", "Off").on_off();
consvar_t cv_controlperkey = Player("controlperkey", "One").values({{1, "One"}, {2, "Several"}});

// actual general (maximum) sound & music volume, saved into the config
// Volume scale is 0-100 in new mixer. 100 is treated as -0dB or 100% gain. No more weirdness to work around SDL_mixer
// problems
consvar_t cv_mastervolume = Player("volume", "80").min_max(0, 100);
consvar_t cv_digmusicvolume = Player("musicvolume", "80").min_max(0, 100);
consvar_t cv_soundvolume = Player("soundvolume", "80").min_max(0, 100);

#ifdef HAVE_DISCORDRPC
	void DRPC_UpdatePresence(void);
	consvar_t cv_discordasks = Player("discordasks", "Yes").yes_no().onchange(DRPC_UpdatePresence);
	consvar_t cv_discordrp = Player("discordrp", "On").on_off().onchange(DRPC_UpdatePresence);
	consvar_t cv_discordstreamer = Player("discordstreamer", "Off").on_off();
#endif

consvar_t cv_drawdist = Player("drawdist", "Normal").values({
	{3072,  "Shortest"},
	{4096,  "Shorter"},
	{6144,  "Short"},
	{8192,  "Normal"},
	{12288, "Far"},
	{16384, "Farther"},
	{20480, "Extreme"},
	{24576, "Penultimate"},
	{0,     "Infinite"},
});

consvar_t cv_drawdist_precip = Player("drawdist_precip", "Normal").values({
	{256,  "Shortest"},
	{512,  "Shorter"},
	{768,  "Short"},
	{1024, "Normal"},
	{1536, "Far"},
	{2048, "Farthest"},
	{0,    "None"},
});

consvar_t cv_drawinput = Player("drawinput", "Off").on_off();
consvar_t cv_ffloorclip = Player("ffloorclip", "On").on_off();

consvar_t cv_fpscap = Player("fpscap", "Match refresh rate").values({
#ifdef DEVELOP
	// Lower values are actually pretty useful for debugging interp problems!
	{1, "MIN"},
#else
	{TICRATE, "MIN"},
#endif
	{300, "MAX"},
	{-1, "Unlimited"},
	{0, "Match refresh rate"},
});

void SCR_ChangeFullscreen(void);
consvar_t cv_fullscreen = Player("fullscreen", "Yes").yes_no().onchange(SCR_ChangeFullscreen);

// Sound system toggles, saved into the config
void GameDigiMusic_OnChange(void);
void GameSounds_OnChange(void);
consvar_t cv_gamedigimusic = Player("music", "On").on_off().onchange_noinit(GameDigiMusic_OnChange);
consvar_t cv_gamesounds = Player("sounds", "On").on_off().onchange_noinit(GameSounds_OnChange);

// time attack ghost options are also saved to config
static CV_PossibleValue_t ghost_cons_t[] = {{0, "Hide"}, {1, "Show Character"}, {2, "Show All"}, {0, NULL}};
consvar_t cv_ghost_bestlap   = Player("ghost_bestlap",   "Show All").values(ghost_cons_t);
consvar_t cv_ghost_besttime  = Player("ghost_besttime",  "Show All").values(ghost_cons_t);
consvar_t cv_ghost_last      = Player("ghost_last",      "Show All").values(ghost_cons_t);

static CV_PossibleValue_t ghost2_cons_t[] = {{0, "Hide"}, {1, "Show"}, {0, NULL}};
consvar_t cv_ghost_guest     = Player("ghost_guest",     "Show").values(ghost2_cons_t);
consvar_t cv_ghost_staff     = Player("ghost_staff",     "Show").values(ghost2_cons_t);

void ItemFinder_OnChange(void);
consvar_t cv_itemfinder = Player("itemfinder", "Off").flags(CV_NOSHOWHELP).on_off().onchange(ItemFinder_OnChange).dont_save();

consvar_t cv_maxportals = Player("maxportals", "2").values({{0, "MIN"}, {12, "MAX"}}); // lmao rendering 32 portals, you're a card
consvar_t cv_menuframeskip = Player("menuframeskip", "Off").values({
	{35, "MIN"},
	{144, "MAX"},
	{0, "Off"},
});
consvar_t cv_movebob = Player("movebob", "1.0").floating_point().min_max(0, 4*FRACUNIT);
consvar_t cv_netstat = Player("netstat", "Off").on_off().dont_save(); // show bandwidth statistics
consvar_t cv_netticbuffer = Player("netticbuffer", "1").min_max(0, 3);

// number of channels available
void SetChannelsNum(void);
consvar_t cv_numChannels = Player("snd_channels", "64").values(CV_Unsigned).onchange(SetChannelsNum);

extern CV_PossibleValue_t soundmixingbuffersize_cons_t[];
consvar_t cv_soundmixingbuffersize = Player("snd_mixingbuffersize", "2048")
	.values(soundmixingbuffersize_cons_t)
	.onchange_noinit([]() { COM_ImmedExecute("restartaudio"); });

extern CV_PossibleValue_t perfstats_cons_t[];
consvar_t cv_perfstats = Player("perfstats", "Off").dont_save().values(perfstats_cons_t);

// Window focus sound sytem toggles
void BGAudio_OnChange(void);
void BGAudio_OnChange(void);
consvar_t cv_bgaudio = Player("bgaudio", "Nothing").onchange_noinit(BGAudio_OnChange).values({
	{0, "Nothing"},
	{1, "Music"},
	{2, "Sounds"},
	{3, "Music&Sounds"},
});

// Pause game upon window losing focus
consvar_t cv_pauseifunfocused = Player("pauseifunfocused", "Yes").yes_no();

extern CV_PossibleValue_t cv_renderer_t[];
consvar_t cv_renderer = Player("renderer", "Software").flags(CV_NOLUA).values(cv_renderer_t).onchange(SCR_ChangeRenderer);
consvar_t cv_parallelsoftware = Player("parallelsoftware", "On").on_off();

consvar_t cv_renderview = Player("renderview", "On").values({{0, "Off"}, {1, "On"}, {2, "Force"}}).dont_save();
consvar_t cv_rollingdemos = Player("rollingdemos", "On").on_off();
consvar_t cv_scr_depth = Player("scr_depth", "16 bits").values({{8, "8 bits"}, {16, "16 bits"}, {24, "24 bits"}, {32, "32 bits"}});

//added : 03-02-98: default screen mode, as loaded/saved in config
consvar_t cv_scr_width = Player("scr_width", "640").values(CV_Unsigned);
consvar_t cv_scr_height = Player("scr_height", "400").values(CV_Unsigned);
consvar_t cv_scr_effect = Player("scr_effect", "Sharp Bilinear").values({{0, "Nearest"}, {1, "Sharp Bilinear"}, {2, "SalCRT"}, {3, "SalCRT Sharp"}}).save();

consvar_t cv_scr_scale = Player("scr_scale", "1.0").floating_point();
consvar_t cv_scr_x = Player("scr_x", "0.0").floating_point();
consvar_t cv_scr_y = Player("scr_y", "0.0").floating_point();

consvar_t cv_seenames = Player("seenames", "On").on_off();
consvar_t cv_shadow = Player("shadow", "On").on_off();
consvar_t cv_showfocuslost = Player("showfocuslost", "Yes").yes_no();

void R_SetViewSize(void);
consvar_t cv_showhud = Player("showhud", "Yes").yes_no().onchange(R_SetViewSize).dont_save();

consvar_t cv_skybox = Player("skybox", "On").on_off();

// Display song credits
consvar_t cv_songcredits = Player("songcredits", "On").on_off();

void SoundTest_OnChange(void);
consvar_t cv_soundtest = Player("soundtest", "0").onchange(SoundTest_OnChange).dont_save();

// Cvar for using splitscreen with 1 device.
consvar_t cv_splitdevice = Player("splitdevice", "Off").on_off();

void Splitplayers_OnChange(void);
consvar_t cv_splitplayers = Player("splitplayers", "One").values({{1, "One"}, {2, "Two"}, {3, "Three"}, {4, "Four"}}).onchange(Splitplayers_OnChange).dont_save();

consvar_t cv_ticrate = Player(cvlist_screen)("showfps", "No").yes_no();
consvar_t cv_tilting = Player("tilting", "On").on_off();

// first time memory
consvar_t cv_tutorialprompt = Player("tutorialprompt", "On").on_off();

void I_StartupMouse(void);
consvar_t cv_usemouse = Player("use_mouse", "Off").values({{0, "Off"}, {1, "On"}, {2, "Force"}}).onchange(I_StartupMouse);

// synchronize page flipping with screen refresh
extern "C++"
{
namespace srb2::cvarhandler
{
void on_set_vid_wait();
}
}
consvar_t cv_vidwait = GraphicsDriver("vid_wait", "Off").on_off().onchange(srb2::cvarhandler::on_set_vid_wait);

// if true, all sounds are loaded at game startup
consvar_t precachesound = Player("precachesound", "Off").on_off();

// stereo reverse
consvar_t stereoreverse = Player("stereoreverse", "Off").on_off();



//
// Server local, also available on dedicated servers.
// Usually saved, not sycned though...
//

void Joinable_OnChange(void);
consvar_t cv_allownewplayer = Server("allowjoin", "On").on_off().onchange(Joinable_OnChange);
consvar_t cv_maxconnections = Server("maxconnections", "16").min_max(2, MAXPLAYERS).onchange(Joinable_OnChange);

// autorecord demos for time attack
consvar_t cv_autorecord = Server("autorecord", "Yes").yes_no().dont_save();

// Here for dedicated servers
consvar_t cv_discordinvites = Server("discordinvites", "Everyone").values({{0, "Admins Only"}, {1, "Everyone"}}).onchange(Joinable_OnChange);

consvar_t cv_downloading = Server("downloading", "On").on_off().dont_save();

// Okay, whoever said homremoval causes a performance hit should be shot.
consvar_t cv_homremoval = Server("homremoval", "Yes").values({{0, "No"}, {1, "Yes"}, {2, "Flash"}});

consvar_t cv_httpsource = Server("http_source", "");

void JoinTimeout_OnChange(void);
consvar_t cv_jointimeout = Server("jointimeout", "210").min_max(TICRATE/7, 60*TICRATE).onchange(JoinTimeout_OnChange);

consvar_t cv_karthorns = Server("taunthorns", "On").on_off();
consvar_t cv_kartvoices = Server("tauntvoices", "On").on_off();

consvar_t cv_kartspeedometer = Server("speedometer", "Percentage").values({{0, "Off"}, {1, "Percentage"}, {2, "Kilometers"}, {3, "Miles"}, {4, "Fracunits"}}); // use tics in display
consvar_t cv_kicktime = Server("kicktime", "20").values(CV_Unsigned);

void MasterServer_OnChange(void);
consvar_t cv_masterserver = Server("masterserver", "https://ms.kartkrew.org/ms/api").onchange(MasterServer_OnChange);
consvar_t cv_masterserver_nagattempts = Server("masterserver_nagattempts", "5").values(CV_Unsigned);

void MasterServer_Debug_OnChange (void);
consvar_t cv_masterserver_debug = Server("masterserver_debug", "Off").on_off().onchange(MasterServer_Debug_OnChange);

consvar_t cv_masterserver_timeout = Server("masterserver_timeout", "5").values(CV_Unsigned);
consvar_t cv_masterserver_token = Server("masterserver_token", "");

void MasterClient_Ticker(void);
consvar_t cv_masterserver_update_rate = Server("masterserver_update_rate", "15").min_max(2, 60).onchange_noinit(MasterClient_Ticker);

consvar_t cv_maxping = Server("maxdelay", "20").min_max(0, 30);
consvar_t cv_menujam = Server("menujam", "menu").values({{0, "menu"}, {1, "menu2"}, {2, "menu3"}});
consvar_t cv_menujam_update = Server("menujam_update", "Off").on_off();
consvar_t cv_netdemosyncquality = Server("netdemo_syncquality", "1").min_max(1, 35);
consvar_t cv_netdemosize = Server("netdemo_size", "6").values(CV_Natural);

void NetTimeout_OnChange(void);
consvar_t cv_nettimeout = Server("nettimeout", "210").min_max(TICRATE/7, 60*TICRATE).onchange(NetTimeout_OnChange);

consvar_t cv_pause = NetVar("pausepermission", "Server Admins").values({{0, "Server Admins"}, {1, "Everyone"}});
consvar_t cv_pingmeasurement = Server("pingmeasurement", "Frames").values({{0, "Frames"}, {1, "Milliseconds"}});
consvar_t cv_playbackspeed = Server("playbackspeed", "1").min_max(1, 10).dont_save();

static constexpr const char* kNetDemoRecordDefault =
#ifdef DEVELOP
	"Auto Save";
#else
	"Manual Save";
#endif

consvar_t cv_recordmultiplayerdemos = Server("netdemo_record", kNetDemoRecordDefault).values({{0, "Disabled"}, {1, "Manual Save"}, {2, "Auto Save"}});

consvar_t cv_reducevfx = Server("reducevfx", "No").yes_no();
consvar_t cv_screenshake = Server("screenshake", "Full").values({{0, "Off"}, {1, "Half"}, {2, "Full"}});

consvar_t cv_rendezvousserver = Server("holepunchserver", "relay.kartkrew.org");

void Update_parameters (void);
consvar_t cv_server_contact = Server("server_contact", "").onchange_noinit(Update_parameters);
consvar_t cv_servername = Server("servername", "Ring Racers server").onchange_noinit(Update_parameters);

void M_SortServerList(void);
consvar_t cv_serversort = Server("serversort", "Recommended").dont_save().onchange(M_SortServerList).values({
	{-1, "Recommended"},
	{ 0, "Ping"},
	{ 1, "AVG. Power Level"},
	{ 2, "Most Players"},
	{ 3, "Least Players"},
	{ 4, "Max Player Slots"},
	{ 5, "Gametype"},
});

consvar_t cv_showviewpointtext = Server("showviewpointtext", "On").on_off();
consvar_t cv_skipmapcheck = Server("skipmapcheck", "Off").on_off();
consvar_t cv_sleep = Server("cpusleep", "1").min_max(0, 1000/TICRATE);

#ifdef USE_STUN
	/* https://gist.github.com/zziuni/3741933 */
	/* I can only trust google to keep their shit up :y */
	consvar_t cv_stunserver = Server("stunserver", "stun.l.google.com:19302");
#endif


//
// Netvars - synced in netgames, also saved.
// There's a separate section for netvars that don't save...
//

void AutoBalance_OnChange(void);
consvar_t cv_autobalance = NetVar("autobalance", "Off").on_off().onchange(AutoBalance_OnChange);

consvar_t cv_blamecfail = NetVar("blamecfail", "Off").on_off();

// Speed of file downloading (in packets per tic)
consvar_t cv_downloadspeed = NetVar("downloadspeed", "32").min_max(1, 300);

#ifdef DUMPCONSISTENCY
	consvar_t cv_dumpconsistency = NetVar(cvlist_dumpconsistency)("dumpconsistency", "Off").on_off();
#endif

// Intermission time Tails 04-19-2002
consvar_t cv_inttime = NetVar("inttime", "10").min_max(0, 3600);

consvar_t cv_joindelay = NetVar("joindelay", "10").min_max(1, 3600, {{0, "Off"}});

#ifdef VANILLAJOINNEXTROUND
	consvar_t cv_joinnextround = NetVar("joinnextround", "Off").on_off(); /// \todo not done
#endif

void Lagless_OnChange(void);
consvar_t cv_lagless = NetVar("lagless", "Off").on_off().onchange(Lagless_OnChange);

// max file size to send to a player (in kilobytes)
consvar_t cv_maxsend = NetVar("maxsend", "51200").min_max(0, 51200);

consvar_t cv_noticedownload = NetVar("noticedownload", "Off").on_off();
consvar_t cv_pingtimeout = NetVar("maxdelaytimeout", "10").min_max(8, 120);
consvar_t cv_resynchattempts = NetVar("resynchattempts", "2").min_max(1, 20, {{0, "No"}});

void TeamScramble_OnChange(void);
consvar_t cv_scrambleonchange = NetVar("scrambleonchange", "Off").values({{0, "Off"}, {1, "Random"}, {2, "Points"}});
consvar_t cv_teamscramble = NetVar("teamscramble", "Off").values({{0, "Off"}, {1, "Random"}, {2, "Points"}}).onchange_noinit(TeamScramble_OnChange);

consvar_t cv_showjoinaddress = NetVar("showjoinaddress", "Off").on_off();
consvar_t cv_zvote_delay = NetVar("zvote_delay", "20").values(CV_Unsigned);
consvar_t cv_zvote_length = NetVar("zvote_length", "20").values(CV_Unsigned);
consvar_t cv_zvote_quorum = NetVar("zvote_quorum", "0.6").floating_point().min_max(0, FRACUNIT);
consvar_t cv_zvote_spectators = NetVar("zvote_spectator_votes", "Off").on_off();

consvar_t cv_allowguests = NetVar("allowguests", "On").on_off();
consvar_t cv_gamestochat = NetVar("gamestochat", "0").min_max(0, 99);


//
// Netvars that don't save...
//

consvar_t cv_advancemap = UnsavedNetVar("advancemap", "Vote").values({{0, "Same"}, {1, "Next"}, {2, "Random"}, {3, "Vote"}});

void Advertise_OnChange(void);
consvar_t cv_advertise = UnsavedNetVar("advertise", "No").yes_no().onchange_noinit(Advertise_OnChange);

consvar_t cv_allowexitlevel = UnsavedNetVar("allowexitlevel", "No").yes_no();
consvar_t cv_allowmlook = UnsavedNetVar("allowmlook", "Yes").yes_no();
consvar_t cv_allowteamchange = UnsavedNetVar("allowteamchange", "Yes").yes_no();
consvar_t cv_antigrief = NetVar("antigrief", "30").min_max(10, 180, {{0, "Off"}});
consvar_t cv_automate = UnsavedNetVar("automate", "On").on_off();

// If on and you're an admin, your messages will automatically become shouts.
consvar_t cv_autoshout = UnsavedNetVar("autoshout", "Off").on_off();

consvar_t cv_cheats = UnsavedNetVar(cvlist_command)("cheats",
#ifdef DEVELOP
	"On"
#else
	"Off"
#endif
).on_off().onchange_noinit(CV_CheatsChanged);

consvar_t cv_countdowntime = UnsavedNetVar("countdowntime", "30").min_max(15, 9999);
consvar_t cv_duelspectatorreentry = UnsavedNetVar("duelspectatorreentry", "180").min_max(0, 10*60);

// SRB2kart
consvar_t cv_items[] = {
	UnsavedNetVar("sneaker",			"On").on_off(),
	UnsavedNetVar("rocketsneaker",		"On").on_off(),
	UnsavedNetVar("invincibility",		"On").on_off(),
	UnsavedNetVar("banana",				"On").on_off(),
	UnsavedNetVar("eggmark",			"On").on_off(),
	UnsavedNetVar("orbinaut",			"On").on_off(),
	UnsavedNetVar("jawz",				"On").on_off(),
	UnsavedNetVar("mine",				"On").on_off(),
	UnsavedNetVar("landmine",			"On").on_off(),
	UnsavedNetVar("ballhog",			"On").on_off(),
	UnsavedNetVar("selfpropelledbomb",	"On").on_off(),
	UnsavedNetVar("grow",				"On").on_off(),
	UnsavedNetVar("shrink",				"On").on_off(),
	UnsavedNetVar("lightningshield",	"On").on_off(),
	UnsavedNetVar("bubbleshield",		"On").on_off(),
	UnsavedNetVar("flameshield",		"On").on_off(),
	UnsavedNetVar("hyudoro",			"On").on_off(),
	UnsavedNetVar("pogospring",			"On").on_off(),
	UnsavedNetVar("superring",			"On").on_off(),
	UnsavedNetVar("kitchensink",		"On").on_off(),
	UnsavedNetVar("droptarget",			"On").on_off(),
	UnsavedNetVar("gardentop",			"On").on_off(),
	UnsavedNetVar("gachabom",			"On").on_off(),
	UnsavedNetVar("dualsneaker",		"On").on_off(),
	UnsavedNetVar("triplesneaker",		"On").on_off(),
	UnsavedNetVar("triplebanana",		"On").on_off(),
	UnsavedNetVar("tripleorbinaut",		"On").on_off(),
	UnsavedNetVar("quadorbinaut",		"On").on_off(),
	UnsavedNetVar("dualjawz",			"On").on_off(),
	UnsavedNetVar("triplegachabom",		"On").on_off(),
};

consvar_t cv_kartbot = UnsavedNetVar("bots", "Off").values({
	{0, "Off"},
	{1, "Lv.1"},
	{2, "Lv.2"},
	{3, "Lv.3"},
	{4, "Lv.4"},
	{5, "Lv.5"},
	{6, "Lv.6"},
	{7, "Lv.7"},
	{8, "Lv.8"},
	{9, "Lv.9"},
	{10,"Lv.10"},
	{11,"Lv.11"},
	{12,"Lv.12"},
	{13,"Lv.MAX"},
});

consvar_t cv_kartbumpers = UnsavedNetVar("battlebumpers", "3").min_max(0, 12);

void KartEliminateLast_OnChange(void);
consvar_t cv_karteliminatelast = NetVar("eliminatelast", "Yes").yes_no().onchange(KartEliminateLast_OnChange);

void KartEncore_OnChange(void);
consvar_t cv_kartencore = UnsavedNetVar("encore", "Auto").values({{-1, "Auto"}, {0, "Off"}, {1, "On"}}).onchange_noinit(KartEncore_OnChange);

void KartFrantic_OnChange(void);
consvar_t cv_kartfrantic = UnsavedNetVar("franticitems", "Off").on_off().onchange_noinit(KartFrantic_OnChange);

void KartSpeed_OnChange(void);
consvar_t cv_kartspeed = UnsavedNetVar("gamespeed", "Auto Gear").values(kartspeed_cons_t).onchange_noinit(KartSpeed_OnChange);

consvar_t cv_kartusepwrlv = UnsavedNetVar("usepwrlv", "Yes").yes_no();

void LiveStudioAudience_OnChange(void);
#ifdef DEVELOP
	consvar_t cv_livestudioaudience = UnsavedNetVar("livestudioaudience", "On").on_off().onchange(LiveStudioAudience_OnChange);
#else
	consvar_t cv_livestudioaudience = UnsavedNetVar("livestudioaudience", "Off").on_off().onchange(LiveStudioAudience_OnChange);
#endif

consvar_t cv_maxplayers = NetVar("maxplayers", "8").min_max(1, MAXPLAYERS);

// Scoring type options
consvar_t cv_overtime = UnsavedNetVar("overtime", "Yes").yes_no();

extern CV_PossibleValue_t pointlimit_cons_t[];
void PointLimit_OnChange(void);
consvar_t cv_pointlimit = UnsavedNetVar("pointlimit", "Auto").values(pointlimit_cons_t).onchange_noinit(PointLimit_OnChange);

void Schedule_OnChange(void);
consvar_t cv_schedule = UnsavedNetVar("schedule", "On").on_off().onchange(Schedule_OnChange);

consvar_t cv_shoutcolor = UnsavedNetVar("shout_color", "Red").values({
	{-1, "Player color"},
	{0, "White"},
	{1, "Yellow"},
	{2, "Purple"},
	{3, "Green"},
	{4, "Blue"},
	{5, "Red"},
	{6, "Gray"},
	{7, "Orange"},
	{8, "Sky-blue"},
	{9, "Gold"},
	{10, "Lavender"},
	{11, "Aqua-green"},
	{12, "Magenta"},
	{13, "Pink"},
	{14, "Brown"},
	{15, "Tan"},
});

// Shout settings
// The relevant ones are CV_NETVAR because too lazy to send them any other way
consvar_t cv_shoutname = UnsavedNetVar("shout_name", "SERVER");

consvar_t cv_spectatorreentry = UnsavedNetVar("spectatorreentry", "30").min_max(0, 10*60);
consvar_t cv_thunderdome = UnsavedNetVar("thunderdome", "Off").on_off();

void TimeLimit_OnChange(void);
consvar_t cv_timelimit = UnsavedNetVar("timelimit", "Default").min_max(1, 30*60, {{0, "None"}, {-1, "Default"}}).onchange_noinit(TimeLimit_OnChange);

consvar_t cv_votetime = UnsavedNetVar("votetime", "20").min_max(10, 3600);


//
// Online cheats - synced in netgames.
// Cheats don't save...
//

consvar_t cv_barriertime = OnlineCheat("barriertime", "30").values(CV_Natural).description("How long it takes for the Barrier to shrink in Battle Overtime");
consvar_t cv_battlespawn = OnlineCheat("battlespawn", "0").values(CV_Unsigned).description("Spawn every player at the same spawnpoint in Battle (0 = random spawns)");
consvar_t cv_battletest = OnlineCheat("battletest", "Off").on_off().description("Free Play goes to Battle instead of Prisons");
consvar_t cv_battleufotest = OnlineCheat("battleufotest", "Off").on_off().description("Respawn Battle UFOs instantly after being destroyed");

#ifdef DEVELOP
	consvar_t cv_botcontrol = OnlineCheat("botcontrol", "On").on_off().description("Toggle bot AI movement");
#endif

extern CV_PossibleValue_t capsuletest_cons_t[];
void CapsuleTest_OnChange(void);
consvar_t cv_capsuletest = OnlineCheat("capsuletest", "Off").values(capsuletest_cons_t).onchange(CapsuleTest_OnChange).description("Force item capsule spawning rules");

consvar_t cv_debugcheese = OnlineCheat("debugcheese", "Off").on_off().description("Disable checks that prevent farming item boxes");
consvar_t cv_debugencorevote = OnlineCheat("debugencorevote", "Off").on_off().description("Force encore choice to appear on vote screen");
consvar_t cv_debuglapcheat = OnlineCheat("debuglapcheat", "Off").on_off().description("Permit far waypoint jumps and disable lap cheat prevention");
consvar_t cv_debugnewchallenger = OnlineCheat("debugnewchallenger", "Off").on_off().description("Do not restart the map to toggle Duel mode");
consvar_t cv_forcebots = OnlineCheat("forcebots", "No").yes_no().description("Force bots to appear, even in wrong game modes");

void ForceSkin_OnChange(void);
consvar_t cv_forceskin = OnlineCheat("forcecharacter", "None").onchange(ForceSkin_OnChange).description("Force all players to use one character");

consvar_t cv_fuzz = OnlineCheat("fuzz", "Off").on_off().description("Human players spam random inputs, get random items");

consvar_t cv_kartdebugamount = OnlineCheat("debugitemamount", "1").min_max(1, 255).description("If debugitem, give multiple copies of an item");
consvar_t cv_kartdebugbots = OnlineCheat("debugbots", "Off").on_off().description("Bot AI debugger");
consvar_t cv_kartdebugdistribution = OnlineCheat("debugitemodds", "Off").on_off().description("Show items that the roulette can roll");
consvar_t cv_kartdebughuddrop = OnlineCheat("debugitemdrop", "Off").on_off().description("Players drop paper items when damaged in any way");

consvar_t cv_kartdebugbotwhip = OnlineCheat("debugbotwhip", "Off").on_off().description("Disable bot ring and item pickups");

extern CV_PossibleValue_t kartdebugitem_cons_t[];
consvar_t cv_kartdebugitem = OnlineCheat("debugitem", "NONE").values(kartdebugitem_cons_t).description("Force item boxes to only roll one kind of item");

consvar_t cv_kartdebugstart = OnlineCheat("debugstart", "-1").min_max(-1, 16).description("Override playercount for POSITION time calcs. -1 default, 0 skip");
consvar_t cv_kartdebugwaypoints = OnlineCheat("debugwaypoints", "Off").values({{0, "Off"}, {1, "Forwards"}, {2, "Backwards"}}).description("Make waypoints visible");

#ifdef DEVELOP
	consvar_t cv_kartencoremap = OnlineCheat("encoremap", "On").on_off().description("Toggle Encore colormap");
#endif

extern CV_PossibleValue_t numlaps_cons_t[];
void NumLaps_OnChange(void);
consvar_t cv_numlaps = OnlineCheat("numlaps", "Map default").values(numlaps_cons_t).onchange(NumLaps_OnChange).description("Race maps always have the same number of laps");

consvar_t cv_restrictskinchange = OnlineCheat("restrictskinchange", "Yes").yes_no().description("Don't let players change their skin in the middle of gameplay");
consvar_t cv_spbtest = OnlineCheat("spbtest", "Off").on_off().description("SPB can never target a player");
consvar_t cv_showgremlins = OnlineCheat("showgremlins", "No").yes_no().description("Show line collision errors");
consvar_t cv_timescale = OnlineCheat(cvlist_timer)("timescale", "1.0").floating_point().min_max(FRACUNIT/20, 20*FRACUNIT).description("Overclock or slow down the game");

#ifdef DEVELOP
	// change the default value in doomdef.h (so it affects release builds too)
	consvar_t cv_debugtraversemax = OnlineCheat("debugtraversemax", TOSTR2(TRAVERSE_MAX)).min_max(0, 255).description("Improve line-of-sight detection (waypoints) but may slow down the game");
#endif

consvar_t cv_ufo_follow = OnlineCheat("ufo_follow", "0").min_max(0, MAXPLAYERS).description("Make UFO Catcher folow this player");
consvar_t cv_ufo_health = OnlineCheat("ufo_health", "-1").min_max(-1, 100).description("Override UFO Catcher health -- applied at spawn or when value is changed");

//
// Server local cheats.
// Not netsynced, not saved...
//

consvar_t cv_botscanvote = ServerCheat("botscanvote", "No").yes_no();

void Gravity_OnChange(void);
consvar_t cv_gravity = ServerCheat("gravity", "0.8").floating_point().min_max(0, 200*FRACUNIT).onchange(Gravity_OnChange).description("Change the default gravity"); // change DEFAULT_GRAVITY if you change this

consvar_t cv_kartdebugcolorize = ServerCheat("debugcolorize", "Off").on_off().description("Show all colorized options on the HUD");
consvar_t cv_kartdebugdirector = ServerCheat("debugdirector", "Off").on_off().description("Show director AI on the HUD");
consvar_t cv_kartdebugnodes = ServerCheat("debugnodes", "Off").on_off().description("Show player node and latency on the HUD");


//
// Player local cheats.
// Dedicated servers don't see these.
// Not saved...
//

consvar_t cv_1pswap = PlayerCheat("1pswap", "1").min_max(1, MAXSPLITSCREENPLAYERS).description("Let P1's Profile control a different splitscreen player");

#ifdef DEVELOP
	consvar_t cv_debugchallenges = PlayerCheat("debugchallenges", "Off").description("Chao keys are infinite, unlock any tile, relock tiles, animations play quickly").values({
		{1, "MIN"},
		{UINT16_MAX-1, "MAX"},
		{0, "Off"},
		{-1, "On"},
		{-2, "Tag"},
	});
#endif

consvar_t cv_debugfinishline = PlayerCheat("debugfinishline", "Off").on_off().description("Highlight finish lines, respawn lines, death pits and instakill planes with high contrast colors");
consvar_t cv_debughudtracker = PlayerCheat("debughudtracker", "Off").on_off().description("Highlight overlapping HUD tracker blocks");

#ifdef DEVELOP
	consvar_t cv_debugprisoncd = PlayerCheat("debugprisoncd", "Off").on_off().description("Always drop a CD from breaking Prisons");
#endif

consvar_t cv_debugrank = PlayerCheat("debugrank", "Off").description("Show GP rank state on the HUD; optionally force a rank grade").values({
	{0, "Off"},
	{1, "On"},
	// This matches the order of gp_rank_e
	{2, "E"},
	{3, "D"},
	{4, "C"},
	{5, "B"},
	{6, "A"},
	{7, "S"},
});

consvar_t cv_debugrender_contrast = PlayerCheat("debugrender_contrast", "0.0").floating_point().min_max(-FRACUNIT, FRACUNIT).description("Change level lighting");
consvar_t cv_debugrender_freezebsp = PlayerCheat("debugrender_freezebsp", "Off").on_off().description("Freeze level culling so you can observe how much of the level is being rendered");
consvar_t cv_debugrender_portal = PlayerCheat("debugrender_portal", "Off").on_off().description("Highlight visual portals in red");
consvar_t cv_debugrender_spriteclip = PlayerCheat("debugrender_spriteclip", "Off").on_off().description("Let sprites draw through walls");
consvar_t cv_debugrender_visplanes = PlayerCheat("debugrender_visplanes", "Off").on_off().description("Highlight the number of visplanes");
consvar_t cv_debugvirtualkeyboard = PlayerCheat("debugvirtualkeyboard", "Off").on_off().description("Always show virtual keyboard instead of using real keyboard input.");
consvar_t cv_devmode_screen = PlayerCheat("devmode_screen", "1").min_max(1, 4).description("Choose which splitscreen player devmode applies to");
consvar_t cv_drawpickups = PlayerCheat("drawpickups", "Yes").yes_no().description("Hide rings, spheres, item capsules, prison capsules (visual only)");

void lua_profile_OnChange(void);
consvar_t cv_lua_profile = PlayerCheat("lua_profile", "0").values(CV_Unsigned).onchange(lua_profile_OnChange).description("Show hook timings over an average of N tics");

void CV_palette_OnChange(void);
consvar_t cv_palette = PlayerCheat("palette", "").onchange_noinit(CV_palette_OnChange).description("Force palette to a different lump");
consvar_t cv_palettenum = PlayerCheat("palettenum", "0").values(CV_Unsigned).onchange_noinit(CV_palette_OnChange).description("Use a different sub-palette by default");

extern CV_PossibleValue_t renderhitbox_cons_t[];
consvar_t cv_renderhitbox = PlayerCheat("renderhitbox", "Off").values(renderhitbox_cons_t).description("Show hitboxes around objects");

consvar_t cv_bighead = Player("bighead", "Off").dont_save().values(CV_OnOff).flags(CV_NOSHOWHELP).description("Works out at the library");
consvar_t cv_shittysigns = Player("shittysigns", "Off").dont_save().values(CV_OnOff).flags(CV_NOSHOWHELP).description("It's better because it's worse");
consvar_t cv_tastelesstaunts = Player("tastelesstaunts", "Off").dont_save().values(CV_OnOff).flags(CV_NOSHOWHELP).description("Universally hated in dev");
consvar_t cv_4thgear = UnsavedNetVar("4thgear", "Off").values(CV_OnOff).flags(CV_NOSHOWHELP).description("Surpassing your limits!");
consvar_t cv_levelskull = UnsavedNetVar("levelskull", "Off").values(CV_OnOff).flags(CV_NOSHOWHELP).description("What Storm Rig looked like 2 months before 2.0");

//
// Dummy variables used solely in the menu system.
// todo: add a way to use non-console variables in the menu
//       or make these consvars legitimate like color or skin.
//

extern CV_PossibleValue_t skins_cons_t[];
consvar_t cv_chooseskin = MenuDummy(cvlist_server)("chooseskin", DEFAULTSKIN).values(skins_cons_t);

void M_UpdateAddonsSearch(void);
consvar_t cv_dummyaddonsearch = MenuDummy("dummyaddonsearch", "").onchange_noinit(M_UpdateAddonsSearch);

consvar_t cv_dummyextraspassword = MenuDummy("dummyextraspassword", "");

extern CV_PossibleValue_t gpdifficulty_cons_t[];
void Dummygpdifficulty_OnChange(void);
consvar_t cv_dummygpdifficulty = MenuDummy("dummygpdifficulty", "Intense").values(gpdifficulty_cons_t).onchange(Dummygpdifficulty_OnChange);
consvar_t cv_dummygpencore = MenuDummy("dummygpencore", "Off").on_off();

consvar_t cv_dummyip = MenuDummy("dummyip", "");
consvar_t cv_dummyipselect = MenuDummy("dummyipselect", "0").min_max(0, 2);

extern CV_PossibleValue_t dummykartspeed_cons_t[];
consvar_t cv_dummykartspeed = MenuDummy("dummykartspeed", "Gear 2").values(dummykartspeed_cons_t);

consvar_t cv_dummymatchbots = MenuDummy("dummymatchbots", "Off").values({
	{0, "Off"},
	{1, "Lv.1"},
	{2, "Lv.2"},
	{3, "Lv.3"},
	{4, "Lv.4"},
	{5, "Lv.5"},
	{6, "Lv.6"},
	{7, "Lv.7"},
	{8, "Lv.8"},
	{9, "Lv.9"},
	{10, "Lv.10"},
	{11, "Lv.11"},
	{12, "Lv.12"},
	{13, "Lv.MAX"},
});

void Dummymenuplayer_OnChange(void);
consvar_t cv_dummymenuplayer = MenuDummy("dummymenuplayer", "P1").onchange(Dummymenuplayer_OnChange).values({{0, "NOPE"}, {1, "P1"}, {2, "P2"}, {3, "P3"}, {4, "P4"}});

consvar_t cv_dummyprofileautoroulette = MenuDummy("dummyprofileautoroulette", "Off").on_off();
consvar_t cv_dummyprofilefov = MenuDummy("dummyprofilefov", "100").min_max(70, 110);
consvar_t cv_dummyprofilelitesteer = MenuDummy("dummyprofilelitesteer", "Off").on_off();
consvar_t cv_dummyprofileautoring = MenuDummy("dummyprofileautoring", "Off").on_off();
consvar_t cv_dummyprofilekickstart = MenuDummy("dummyprofilekickstart", "Off").on_off();
consvar_t cv_dummyprofilename = MenuDummy("dummyprofilename", "");
consvar_t cv_dummyprofileplayername = MenuDummy("dummyprofileplayername", "");
consvar_t cv_dummyprofilerumble = MenuDummy("dummyprofilerumble", "On").on_off();

consvar_t cv_dummyscramble = MenuDummy("dummyscramble", "Random").values({{0, "Random"}, {1, "Points"}});

void CV_SPBAttackChanged(void);
consvar_t cv_dummyspbattack = MenuDummy("dummyspbattack", "Off").on_off().onchange(CV_SPBAttackChanged);

consvar_t cv_dummyspectate = MenuDummy("dummyspectate", "Spectator").values({{0, "Spectator"}, {1, "Playing"}});

extern CV_PossibleValue_t dummystaff_cons_t[];
consvar_t cv_dummystaff = MenuDummy("dummystaff", "0").values(dummystaff_cons_t);

consvar_t cv_dummyteam = MenuDummy("dummyteam", "Spectator").values({{0, "Spectator"}, {1, "Red"}, {2, "Blue"}});


//
// lastprofile
//

extern CV_PossibleValue_t lastprofile_cons_t[];

// currently loaded profile for P1 menuing.
// You choose this profile when starting the game, this will also set lastprofile[0]
consvar_t cv_currprofile = MenuDummy("currprofile", "-1").values(lastprofile_cons_t);

// last selected profile, unaccessible cvar only set internally but is saved.
// It's used to know what profile to autoload you to when you get into the character setup.
consvar_t cv_lastprofile[MAXSPLITSCREENPLAYERS] = {
	Player("lastprofile", "0").flags(CV_HIDDEN).values(lastprofile_cons_t),
	Player("lastprofile2", "0").flags(CV_HIDDEN).values(lastprofile_cons_t),
	Player("lastprofile3", "0").flags(CV_HIDDEN).values(lastprofile_cons_t),
	Player("lastprofile4", "0").flags(CV_HIDDEN).values(lastprofile_cons_t),
};

// This one is used exclusively for the titlescreen
consvar_t cv_ttlprofilen = Player("ttlprofilen", "0").values(lastprofile_cons_t);


//
// GIF / Movie / Screenshot
//

consvar_t cv_gif_downscale = Player("gif_downscale", "On").on_off();
consvar_t cv_gif_dynamicdelay = Player("gif_dynamicdelay", "On").values({{0, "Off"}, {1, "On"}, {2, "Accurate, experimental"}});
consvar_t cv_gif_localcolortable = Player("gif_localcolortable", "On").on_off();
consvar_t cv_gif_optimize = Player("gif_optimize", "On").on_off();

extern CV_PossibleValue_t lossless_recorder_cons_t[];
consvar_t cv_lossless_recorder = Player("lossless_recorder", "GIF").values(lossless_recorder_cons_t);

#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
	extern CV_PossibleValue_t movie_resolution_cons_t[];
	consvar_t cv_movie_custom_resolution = Player("movie_custom_resolution", "640x400");
	consvar_t cv_movie_resolution = Player("movie_resolution", "Medium").values(movie_resolution_cons_t);

	static CV_PossibleValue_t movie_limit_cons_t[] = {{1, "MIN"}, {INT32_MAX, "MAX"}, {0, "Unlimited"}, {0, NULL}};
	consvar_t cv_movie_duration = Player("movie_duration", "Unlimited").floating_point().values(movie_limit_cons_t).step_amount(FRACUNIT);
	consvar_t cv_movie_size = Player("movie_size", "25.0").floating_point().values(movie_limit_cons_t).step_amount(FRACUNIT);

	consvar_t cv_movie_fps = Player("movie_fps", "60").values(CV_Natural);
	consvar_t cv_movie_showfps = Player("movie_showfps", "Yes").yes_no();
	consvar_t cv_movie_sound = Player("movie_sound", "On").on_off();
#endif

consvar_t cv_screenshot_colorprofile = Player("screenshot_colorprofile", "Yes").yes_no();

extern CV_PossibleValue_t zlib_mem_level_t[];
extern CV_PossibleValue_t zlib_level_t[];
extern CV_PossibleValue_t zlib_strategy_t[];
extern CV_PossibleValue_t zlib_window_bits_t[];

// zlib memory usage is as follows:
// (1 << (zlib_window_bits+2)) +  (1 << (zlib_level+9))
consvar_t cv_zlib_level = Player("png_compress_level", "(Optimal) 6").values(zlib_level_t);
consvar_t cv_zlib_memory = Player("png_memory_level", "7").values(zlib_mem_level_t);
consvar_t cv_zlib_strategy = Player("png_strategy", "Normal").values(zlib_strategy_t);
consvar_t cv_zlib_window_bits = Player("png_window_size", "32k").values(zlib_window_bits_t);

consvar_t cv_zlib_levela = Player("apng_compress_level", "4").values(zlib_level_t);
consvar_t cv_zlib_memorya = Player("apng_memory_level", "(Max Memory) 9").values(zlib_mem_level_t);
consvar_t cv_zlib_strategya = Player("apng_strategy", "RLE").values(zlib_strategy_t);
consvar_t cv_zlib_window_bitsa = Player("apng_window_size", "32k").values(zlib_window_bits_t);


//
// Splitscreen cvars - one each for P1, P2, P3 and P4.
// This is only its own section because it's an ugly mess.
// TODO: make it easier to register splitscreen cvars...
//

void weaponPrefChange(void);
void weaponPrefChange2(void);
void weaponPrefChange3(void);
void weaponPrefChange4(void);

consvar_t cv_autoroulette[MAXSPLITSCREENPLAYERS] = {
	Player("autoroulette", "Off").on_off().onchange(weaponPrefChange),
	Player("autoroulette2", "Off").on_off().onchange(weaponPrefChange2),
	Player("autoroulette3", "Off").on_off().onchange(weaponPrefChange3),
	Player("autoroulette4", "Off").on_off().onchange(weaponPrefChange4),
};

consvar_t cv_litesteer[MAXSPLITSCREENPLAYERS] = {
	Player("litesteer", "Off").on_off().onchange(weaponPrefChange),
	Player("litesteer2", "Off").on_off().onchange(weaponPrefChange2),
	Player("litesteer3", "Off").on_off().onchange(weaponPrefChange3),
	Player("litesteer4", "Off").on_off().onchange(weaponPrefChange4),
};

consvar_t cv_autoring[MAXSPLITSCREENPLAYERS] = {
	Player("autoring", "Off").on_off().onchange(weaponPrefChange),
	Player("autoring2", "Off").on_off().onchange(weaponPrefChange2),
	Player("autoring3", "Off").on_off().onchange(weaponPrefChange3),
	Player("autoring4", "Off").on_off().onchange(weaponPrefChange4),
};

consvar_t cv_cam_dist[MAXSPLITSCREENPLAYERS] = {
	Player("cam_dist", "190").floating_point(),
	Player("cam2_dist", "190").floating_point(),
	Player("cam3_dist", "190").floating_point(),
	Player("cam4_dist", "190").floating_point(),
};

consvar_t cv_cam_height[MAXSPLITSCREENPLAYERS] = {
	Player("cam_height", "95").floating_point(),
	Player("cam2_height", "95").floating_point(),
	Player("cam3_height", "95").floating_point(),
	Player("cam4_height", "95").floating_point(),
};

void CV_CamRotate_OnChange(void);
void CV_CamRotate2_OnChange(void);
void CV_CamRotate3_OnChange(void);
void CV_CamRotate4_OnChange(void);

consvar_t cv_cam_rotate[MAXSPLITSCREENPLAYERS] = {
	Player("cam_rotate", "0").min_max(-720, 720).onchange_noinit(CV_CamRotate_OnChange).dont_save(),
	Player("cam2_rotate", "0").min_max(-720, 720).onchange_noinit(CV_CamRotate2_OnChange).dont_save(),
	Player("cam3_rotate", "0").min_max(-720, 720).onchange_noinit(CV_CamRotate3_OnChange).dont_save(),
	Player("cam4_rotate", "0").min_max(-720, 720).onchange_noinit(CV_CamRotate4_OnChange).dont_save(),
};

consvar_t cv_cam_speed[MAXSPLITSCREENPLAYERS] = {
	Player("cam_speed", "0.4").floating_point().min_max(0, 1*FRACUNIT),
	Player("cam2_speed", "0.4").floating_point().min_max(0, 1*FRACUNIT),
	Player("cam3_speed", "0.4").floating_point().min_max(0, 1*FRACUNIT),
	Player("cam4_speed", "0.4").floating_point().min_max(0, 1*FRACUNIT),
};

consvar_t cv_cam_still[MAXSPLITSCREENPLAYERS] = {
	Player("cam_still", "Off").on_off().dont_save(),
	Player("cam2_still", "Off").on_off().dont_save(),
	Player("cam3_still", "Off").on_off().dont_save(),
	Player("cam4_still", "Off").on_off().dont_save(),
};

void ChaseCam_OnChange(void);
void ChaseCam2_OnChange(void);
void ChaseCam3_OnChange(void);
void ChaseCam4_OnChange(void);

consvar_t cv_chasecam[MAXSPLITSCREENPLAYERS] = {
	Player("chasecam", "On").on_off().onchange(ChaseCam_OnChange).dont_save(),
	Player("chasecam2", "On").on_off().onchange(ChaseCam2_OnChange).dont_save(),
	Player("chasecam3", "On").on_off().onchange(ChaseCam3_OnChange).dont_save(),
	Player("chasecam4", "On").on_off().onchange(ChaseCam4_OnChange).dont_save(),
};

consvar_t cv_deadzone[MAXSPLITSCREENPLAYERS] = {
	Player("deadzone", "0.25").floating_point().min_max(0, FRACUNIT),
	Player("deadzone2", "0.25").floating_point().min_max(0, FRACUNIT),
	Player("deadzone3", "0.25").floating_point().min_max(0, FRACUNIT),
	Player("deadzone4", "0.25").floating_point().min_max(0, FRACUNIT)
};

void Follower_OnChange(void);
void Follower2_OnChange(void);
void Follower3_OnChange(void);
void Follower4_OnChange(void);

// player's followers. Also saved.
consvar_t cv_follower[MAXSPLITSCREENPLAYERS] = {
	Player("follower", "None").onchange_noinit(Follower_OnChange),
	Player("follower2", "None").onchange_noinit(Follower2_OnChange),
	Player("follower3", "None").onchange_noinit(Follower3_OnChange),
	Player("follower4", "None").onchange_noinit(Follower4_OnChange),
};

extern CV_PossibleValue_t Followercolor_cons_t[];
void Followercolor_OnChange(void);
void Followercolor2_OnChange(void);
void Followercolor3_OnChange(void);
void Followercolor4_OnChange(void);

// player's follower colors... Also saved...
consvar_t cv_followercolor[MAXSPLITSCREENPLAYERS] = {
	Player("followercolor", "Match").values(Followercolor_cons_t).onchange_noinit(Followercolor_OnChange),
	Player("followercolor2", "Match").values(Followercolor_cons_t).onchange_noinit(Followercolor2_OnChange),
	Player("followercolor3", "Match").values(Followercolor_cons_t).onchange_noinit(Followercolor3_OnChange),
	Player("followercolor4", "Match").values(Followercolor_cons_t).onchange_noinit(Followercolor4_OnChange),
};

void Fov_OnChange(void);
consvar_t cv_fov[MAXSPLITSCREENPLAYERS] = {
	Player("fov", "100").floating_point().min_max(60*FRACUNIT, 179*FRACUNIT).onchange(Fov_OnChange).dont_save(),
	Player("fov2", "100").floating_point().min_max(60*FRACUNIT, 179*FRACUNIT).onchange(Fov_OnChange).dont_save(),
	Player("fov3", "100").floating_point().min_max(60*FRACUNIT, 179*FRACUNIT).onchange(Fov_OnChange).dont_save(),
	Player("fov4", "100").floating_point().min_max(60*FRACUNIT, 179*FRACUNIT).onchange(Fov_OnChange).dont_save(),
};

consvar_t cv_freecam_speed = Player("freecam_speed", "1").min_max(-64, 10).dont_save();

void I_JoyScale(void);
void I_JoyScale2(void);
void I_JoyScale3(void);
void I_JoyScale4(void);

consvar_t cv_joyscale[MAXSPLITSCREENPLAYERS] = {
	Player("padscale", "1").onchange(I_JoyScale),
	Player("padscale2", "1").onchange(I_JoyScale2),
	Player("padscale3", "1").onchange(I_JoyScale3),
	Player("padscale4", "1").onchange(I_JoyScale4),
};

consvar_t cv_kickstartaccel[MAXSPLITSCREENPLAYERS] = {
	Player("kickstartaccel", "Off").on_off().onchange(weaponPrefChange),
	Player("kickstartaccel2", "Off").on_off().onchange(weaponPrefChange2),
	Player("kickstartaccel3", "Off").on_off().onchange(weaponPrefChange3),
	Player("kickstartaccel4", "Off").on_off().onchange(weaponPrefChange4)
};

consvar_t cv_mindelay = Player("mindelay", "2").min_max(0, 15).onchange(weaponPrefChange);

extern CV_PossibleValue_t Color_cons_t[];
void Color1_OnChange(void);
void Color2_OnChange(void);
void Color3_OnChange(void);
void Color4_OnChange(void);

// player colors
consvar_t cv_playercolor[MAXSPLITSCREENPLAYERS] = {
	Player("color", "Default").values(Color_cons_t).onchange_noinit(Color1_OnChange),
	Player("color2", "Default").values(Color_cons_t).onchange_noinit(Color2_OnChange),
	Player("color3", "Default").values(Color_cons_t).onchange_noinit(Color3_OnChange),
	Player("color4", "Default").values(Color_cons_t).onchange_noinit(Color4_OnChange),
};

void Name1_OnChange(void);
void Name2_OnChange(void);
void Name3_OnChange(void);
void Name4_OnChange(void);

// names
consvar_t cv_playername[MAXSPLITSCREENPLAYERS] = {
	Player("name", "Dr. Eggman").onchange_noinit(Name1_OnChange),
	Player("name2", "Tails").onchange_noinit(Name2_OnChange),
	Player("name3", "Sonic").onchange_noinit(Name3_OnChange),
	Player("name4", "Knuckles").onchange_noinit(Name4_OnChange),
};

void rumble_off_handle(void);
void rumble_off_handle2(void);
void rumble_off_handle3(void);
void rumble_off_handle4(void);

consvar_t cv_rumble[MAXSPLITSCREENPLAYERS] = {
	Player("rumble", "On").on_off().onchange(rumble_off_handle),
	Player("rumble2", "On").on_off().onchange(rumble_off_handle2),
	Player("rumble3", "On").on_off().onchange(rumble_off_handle3),
	Player("rumble4", "On").on_off().onchange(rumble_off_handle4)
};

consvar_t cv_shrinkme[MAXSPLITSCREENPLAYERS] = {
	Player("shrinkme", "Off").on_off().onchange(weaponPrefChange).dont_save(),
	Player("shrinkme2", "Off").on_off().onchange(weaponPrefChange2).dont_save(),
	Player("shrinkme3", "Off").on_off().onchange(weaponPrefChange3).dont_save(),
	Player("shrinkme4", "Off").on_off().onchange(weaponPrefChange4).dont_save()
};

void Skin1_OnChange(void);
void Skin2_OnChange(void);
void Skin3_OnChange(void);
void Skin4_OnChange(void);

// player's skin, saved for commodity, when using a favorite skins wad..
consvar_t cv_skin[MAXSPLITSCREENPLAYERS] = {
	Player("skin", DEFAULTSKIN).onchange_noinit(Skin1_OnChange),
	Player("skin2", DEFAULTSKIN2).onchange_noinit(Skin2_OnChange),
	Player("skin3", DEFAULTSKIN3).onchange_noinit(Skin3_OnChange),
	Player("skin4", DEFAULTSKIN4).onchange_noinit(Skin4_OnChange),
};


//
// Developer Console
//

void CONS_backcolor_Change(void);
consvar_t cons_backcolor = Console("con_backcolor", "Black").onchange(CONS_backcolor_Change).values({
	{0, "White"},
	{1, "Black"},
	{2, "Sepia"},
	{3, "Brown"},
	{4, "Pink"},
	{5, "Red"},
	{6, "Orange"},
	{7, "Gold"},
	{8, "Yellow"},
	{9, "Peridot"},
	{10, "Green"},
	{11, "Aquamarine"},
	{12, "Cyan"},
	{13, "Steel"},
	{14, "Blue"},
	{15, "Purple"},
	{16, "Magenta"},
	{17, "Lavender"},
	{18, "Rose"},
});

// whether to use console background picture, or translucent mode
consvar_t cons_backpic = Console("con_backpic", "translucent").values({{0, "translucent"}, {1, "picture"}});

// percentage of screen height to use for console
void CONS_height_Change(void);
consvar_t cons_height = Console("con_height", "50").values(CV_Unsigned).onchange(CONS_height_Change);

// number of lines displayed on the HUD
extern CV_PossibleValue_t hudlines_cons_t[];
void CONS_hudlines_Change(void);
consvar_t cons_hudlines = Console("con_hudlines", "5").values(hudlines_cons_t).onchange(CONS_hudlines_Change);

// how many seconds the hud messages lasts on the screen
// CV_Unsigned can overflow when multiplied by TICRATE later, so let's use a 3-year limit instead
consvar_t cons_hudtime = Console("con_hudtime", "5").min_max(0, 99999999);

// number of lines console move per frame
// (con_speed needs a limit, apparently)
consvar_t cons_speed = Console("con_speed", "8").min_max(0, 64);

extern CV_PossibleValue_t constextsize_cons_t[];
void CV_constextsize_OnChange(void);
consvar_t cv_constextsize = Player(cvlist_screen)("con_textsize", "Medium").values(constextsize_cons_t).onchange(CV_constextsize_OnChange);


//
// Chat window
//

// minichat text background
consvar_t cv_chatbacktint = Player("chatbacktint", "On").on_off();

// chatheight
consvar_t cv_chatheight = Player("chatheight", "8").min_max(6, 22);

// chat notifications (do you want to hear beeps? I'd understand if you didn't.)
consvar_t cv_chatnotifications = Player("chatnotifications", "On").on_off();

// chat spam protection (why would you want to disable that???)
consvar_t cv_chatspamprotection = Player("chatspamprotection", "On").on_off();

// chat timer thingy
consvar_t cv_chattime = Player("chattime", "8").min_max(5, 999);

// chatwidth
consvar_t cv_chatwidth = Player("chatwidth", "150").min_max(64, 150);

// old shit console chat. (mostly exists for stuff like terminal, not because I cared if anyone liked the old chat.)
consvar_t cv_consolechat = Player("chatmode", "Yes").values({{0, "Yes"}, {2, "No"}});

void Mute_OnChange(void);
consvar_t cv_mute = UnsavedNetVar("mute", "Off").on_off().onchange(Mute_OnChange);


//
// OpenGL
//

#ifdef HWRENDER
	consvar_t cv_fovchange = OpenGL("gr_fovchange", "Off").on_off();

	extern CV_PossibleValue_t glshaders_cons_t[];
	consvar_t cv_glallowshaders = OpenGL("gr_allowclientshaders", "On").on_off().network().dont_save();
	consvar_t cv_glshaders = OpenGL("gr_shaders", "On").values(glshaders_cons_t);

	extern CV_PossibleValue_t glanisotropicmode_cons_t[];
	void CV_glanisotropic_OnChange(void);
	consvar_t cv_glanisotropicmode = OpenGL("gr_anisotropicmode", "1").values(glanisotropicmode_cons_t).onchange(CV_glanisotropic_OnChange);

	consvar_t cv_glbatching = OpenGL("gr_batching", "On").on_off().dont_save();

#ifdef ALAM_LIGHTING
		consvar_t cv_glcoronas = OpenGL("gr_coronas", "On").on_off();
		consvar_t cv_glcoronasize = OpenGL("gr_coronasize", "1").floating_point();
		consvar_t cv_gldynamiclighting = OpenGL("gr_dynamiclighting", "On").on_off();
		consvar_t cv_glstaticlighting  = OpenGL("gr_staticlighting", "On").on_off();
#endif

	extern CV_PossibleValue_t glfiltermode_cons_t[];
	void CV_glfiltermode_OnChange(void);
	consvar_t cv_glfiltermode = OpenGL("gr_filtermode", "Nearest").values(glfiltermode_cons_t).onchange(CV_glfiltermode_OnChange);

#ifdef BAD_MODEL_OPTIONS
	consvar_t cv_glmodelinterpolation = OpenGL("gr_modelinterpolation", "Sometimes").values({{0, "Off"}, {1, "Sometimes"}, {2, "Always"}});
	consvar_t cv_glmodellighting = OpenGL("gr_modellighting", "Off").on_off();
#endif

	consvar_t cv_glmodels = OpenGL("gr_models", "On").on_off();
	consvar_t cv_glshearing = OpenGL("gr_shearing", "Off").values({{0, "Off"}, {1, "On"}, {2, "Third-person"}});
	consvar_t cv_glskydome = OpenGL("gr_skydome", "On").on_off();
	consvar_t cv_glsolvetjoin = OpenGL("gr_solvetjoin", "On").on_off().dont_save();
	consvar_t cv_glspritebillboarding = OpenGL("gr_spritebillboarding", "On").on_off();
#endif // HWRENDER


//
// Objectplace
//

consvar_t cv_mapthingnum = ObjectPlace("op_mapthingnum", "0").min_max(0, 4095);
consvar_t cv_opflags = ObjectPlace("op_flags", "0").min_max(0, 15);
consvar_t cv_ophoopflags = ObjectPlace("op_hoopflags", "4").min_max(0, 15);
consvar_t cv_speed = ObjectPlace("op_speed", "16").min_max(1, 128);


//
// RRID debugging
//

#ifdef DEVELOP
	consvar_t cv_badip = AuthDebug("badip", "0");
	consvar_t cv_badjoin = AuthDebug("badjoin", "0");
	consvar_t cv_badresponse = AuthDebug("badresponse", "0");
	consvar_t cv_badresults = AuthDebug("badresults", "0");
	consvar_t cv_badtime = AuthDebug("badtime", "0");
	consvar_t cv_badtraffic = AuthDebug("badtraffic", "0");
	consvar_t cv_nochallenge = AuthDebug("nochallenge", "0");
	consvar_t cv_noresponse = AuthDebug("noresponse", "0");
	consvar_t cv_noresults = AuthDebug("noresults", "0");
#endif


//
// Special cvars that don't fit with the rest.
//

void DummyConsvar_OnChange(void);
consvar_t cv_dummyconsvar = Server("dummyconsvar", "Off").flags(CV_NOSHOWHELP).on_off().onchange(DummyConsvar_OnChange).dont_save();

void CV_EnforceExecVersion(void);
consvar_t cv_execversion = Server(cvlist_execversion)("execversion", "1").dont_save().values(CV_Unsigned).onchange(CV_EnforceExecVersion);


//
// Initialized by CVarList::load...
//

// color cube
consvar_t cv_globalgamma;
consvar_t cv_globalsaturation;
consvar_t cv_rgamma, cv_ygamma, cv_ggamma, cv_cgamma, cv_bgamma, cv_mgamma;
consvar_t cv_rhue, cv_yhue, cv_ghue, cv_chue, cv_bhue, cv_mhue;
consvar_t cv_rsaturation, cv_ysaturation, cv_gsaturation, cv_csaturation, cv_bsaturation, cv_msaturation;

// clang-format on

// This function can be used for more advanced cvar
// initialization. It's only called once, the first time that
// any cvlist is registered.
//
// Use it when you might need to create intermediate objects
// but don't want to pollute this file's namespace, or don't
// want to keep them around for the duration of the program.
int CVarList::load()
{
	// color cube
	{
		extern CV_PossibleValue_t hue_cons_t[];

		const auto Color = Player().onchange(CV_palette_OnChange);
		const auto Gamma = Color().min_max(-15, 5);
		const auto Saturation = Color().min_max(0, 10);
		const auto Hue = Color().values(hue_cons_t);

		cv_globalgamma = Gamma("gamma", "0");
		cv_globalsaturation = Saturation("saturation", "10");

		cv_rhue = Hue("rhue",  "0");
		cv_yhue = Hue("yhue",  "4");
		cv_ghue = Hue("ghue",  "8");
		cv_chue = Hue("chue", "12");
		cv_bhue = Hue("bhue", "16");
		cv_mhue = Hue("mhue", "20");

		cv_rgamma = Gamma("rgamma", "0");
		cv_ygamma = Gamma("ygamma", "0");
		cv_ggamma = Gamma("ggamma", "0");
		cv_cgamma = Gamma("cgamma", "0");
		cv_bgamma = Gamma("bgamma", "0");
		cv_mgamma = Gamma("mgamma", "0");

		cv_rsaturation = Saturation("rsaturation", "10");
		cv_ysaturation = Saturation("ysaturation", "10");
		cv_gsaturation = Saturation("gsaturation", "10");
		cv_csaturation = Saturation("csaturation", "10");
		cv_bsaturation = Saturation("bsaturation", "10");
		cv_msaturation = Saturation("msaturation", "10");
	}

	return 0;
}

} // extern "C"
