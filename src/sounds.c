// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  sounds.c
/// \brief music/sound tables, and related sound routines

#include "doomtype.h"
#include "i_sound.h"
#include "sounds.h"
#include "r_defs.h"
#include "r_skins.h"
#include "z_zone.h"
#include "w_wad.h"
#include "lua_script.h"

//
// Information about all the sfx
//

sfxinfo_t S_sfx[NUMSFX] =
{

/*****
	Legacy doesn't use the PITCH variable, so now it is used for
	various flags. See soundflags_t.
*****/
  // S_sfx[0] needs to be a dummy for odd reasons. (don't modify this comment)
//  name, singularity, priority, pitch, volume, data, length, skinsound, usefulness, lumpnum, caption
  {"none" ,  false,   0,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "///////////////////////////////"}, // maximum length

  // A HUMBLE REQUEST FROM YOUR FRIENDLY NEIGHBORHOOD toaster!
  //
  // If you see a caption that's just "" (shows the lumpname in-game),
  // and you intend to use the sound associated with it in a mod,
  // PLEASE give it a caption through SOC or Lua.
  //
  // If the first character of the caption is '/', no caption will be
  // produced; only do  this for "unimportant" sounds that aren't used
  // to indicate gameplay.
  //
  // (to whomstever updates the sounds list wiki page for 2.2, please
  // either copy this comment across, or make sure its desire is
  // codified in the initial paragraph of the page.)
  //
  // Closed Captioning may be a niche feature, but it's an important one.
  // Thank you! ^u^

  // Skin Sounds
  {"altdi1", false, 192, 16, -1, NULL, 0, SKSPLDET1,  -1, LUMPERROR, "Dying"},
  {"altdi2", false, 192, 16, -1, NULL, 0, SKSPLDET2,  -1, LUMPERROR, "Dying"},
  {"altdi3", false, 192, 16, -1, NULL, 0, SKSPLDET3,  -1, LUMPERROR, "Dying"},
  {"altdi4", false, 192, 16, -1, NULL, 0, SKSPLDET4,  -1, LUMPERROR, "Dying"},
  {"altow1", false, 192, 16, -1, NULL, 0, SKSPLPAN1,  -1, LUMPERROR, "Ring loss"},
  {"altow2", false, 192, 16, -1, NULL, 0, SKSPLPAN2,  -1, LUMPERROR, "Ring loss"},
  {"altow3", false, 192, 16, -1, NULL, 0, SKSPLPAN3,  -1, LUMPERROR, "Ring loss"},
  {"altow4", false, 192, 16, -1, NULL, 0, SKSPLPAN4,  -1, LUMPERROR, "Ring loss"},
  {"victr1", false,  64, 16, -1, NULL, 0, SKSPLVCT1,  -1, LUMPERROR, "/"},
  {"victr2", false,  64, 16, -1, NULL, 0, SKSPLVCT2,  -1, LUMPERROR, "/"},
  {"victr3", false,  64, 16, -1, NULL, 0, SKSPLVCT3,  -1, LUMPERROR, "/"},
  {"victr4", false,  64, 16, -1, NULL, 0, SKSPLVCT4,  -1, LUMPERROR, "/"},
  {"gasp" ,  false,  64,  0, -1, NULL, 0,   SKSGASP,  -1, LUMPERROR, "Bubble gasp"},
  {"jump" ,  false, 140,  0, -1, NULL, 0,   SKSJUMP,  -1, LUMPERROR, "Jump"},
  {"pudpud", false,  64,  0, -1, NULL, 0, SKSPUDPUD,  -1, LUMPERROR, "Tired flight"},
  {"putput", false,  64,  0, -1, NULL, 0, SKSPUTPUT,  -1, LUMPERROR, "Flight"}, // not as high a priority
  {"spin" ,  false, 100,  0, -1, NULL, 0,   SKSSPIN,  -1, LUMPERROR, "Spin"},
  {"spndsh", false,  64,  1, -1, NULL, 0, SKSSPNDSH,  -1, LUMPERROR, "Spindash"},
  {"thok" ,  false,  96,  0, -1, NULL, 0,   SKSTHOK,  -1, LUMPERROR, "Thok"},
  {"zoom" ,  false, 120,  1, -1, NULL, 0,   SKSZOOM,  -1, LUMPERROR, "Spin launch"},
  {"skid",   false,  64, 32, -1, NULL, 0,   SKSSKID,  -1, LUMPERROR, "Skid"},

  // Ambience/background objects/etc
  {"ambint",  true,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Obnoxious disco music"},

  {"alarm",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Alarm"},
  {"buzz1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric zap"},
  {"buzz2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric zap"},
  {"buzz3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wacky worksurface"},
  {"buzz4",   true,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Buzz"},
  {"crumbl",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crumbling"}, // Platform Crumble Tails 03-16-2001
  {"fire",   false,   8, 32, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flamethrower"},
  {"grind",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic grinding"},
  {"laser",   true,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laser hum"},
  {"mswing", false,  16, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, "Swinging mace"},
  {"pstart", false, 100,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "/"},
  {"pstop",  false, 100,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crusher stomp"},
  {"steam1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Steam jet"}, // Tails 06-19-2001
  {"steam2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Steam jet"}, // Tails 06-19-2001
  {"wbreak",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wood breaking"},
  {"ambmac", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Machinery"},
  {"spsmsh",  true,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Heavy impact"},

  {"rainin",  true,  24,  4, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rain"},
  {"litng1", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"litng2", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"litng3", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"litng4", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"athun1", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thunder"},
  {"athun2", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thunder"},

  {"amwtr1", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr2", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr3", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr4", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr5", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr6", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr7", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr8", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"bubbl1", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"bubbl2", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"bubbl3", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"bubbl4", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"bubbl5", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"floush", false,  16,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"splash", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"}, // labeling sfx_splash as "glub" and sfx_splish as "splash" seems wrong but isn't
  {"splish", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Splash"}, // Splish Tails 12-08-2000
  {"wdrip1", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip2", false,   8 , 0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip3", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip4", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip5", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip6", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip7", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip8", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wslap",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Splash"}, // Water Slap Tails 12-13-2000

  {"doora1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sliding open"},
  {"doorb1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sliding open"},
  {"doorc1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wooden door opening"},
  {"doorc2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Slamming shut"},
  {"doord1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Creaking open"},
  {"doord2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Slamming shut"},
  {"eleva1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Starting elevator"},
  {"eleva2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Moving elevator"},
  {"eleva3", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stopping elevator"},
  {"elevb1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Starting elevator"},
  {"elevb2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Moving elevator"},
  {"elevb3", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stopping elevator"},

  {"ambin2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Natural vibrations"},
  {"lavbub", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubbling lava"},
  {"rocks1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling rocks"},
  {"rocks2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling rocks"},
  {"rocks3", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling rocks"},
  {"rocks4", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling rocks"},
  {"rumbam", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},
  {"rumble", false,  64, 24, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},

  // Game objects, etc
  {"appear", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Appearing platform"},
  {"bkpoof", false,  70,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Armageddon pow"},
  {"bnce1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bounce"}, // Boing!
  {"bnce2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Scatter"}, // Boing!
  {"cannon", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powerful shot"},
  {"cgot" ,   true, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Emerald"}, // Got Emerald! Tails 09-02-2001
  {"cybdth", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},
  {"deton",   true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Threatening beeping"},
  {"ding",   false, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ding"},
  {"dmpain", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Machine damage"},
  {"drown",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drowning"},
  {"fizzle", false, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric fizzle"},
  {"gbeep",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Threatening beeping"}, // Grenade beep
  {"wepfir", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing weapon"}, // defaults to thok
  {"ghit" ,  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Goop splash"},
  {"gloop",  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Splash"},
  {"gspray", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Goop sling"},
  {"gravch", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Recycler"},
  {"itemup",  true, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"},
  {"jet",    false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jet engine"},
  {"jshard",  true, 167,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Life transfer"}, // placeholder repurpose; original string was "Got Shard"
  {"lose" ,  false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Failure"},
  {"lvpass", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spinning signpost"},
  {"mindig", false,   8, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tunnelling"},
  {"mixup",   true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Teleport"},
  {"monton", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Golden Monitor activated"},
  {"pogo" ,  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical pogo"},
  {"pop"  ,  false,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pop"},
  {"rail1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing rail"},
  {"rail2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crashing rail"},
  {"rlaunc", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing"},
  {"shield", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pity Shield"}, // generic GET!
  {"wirlsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Whirlwind Shield"}, // Whirlwind GET!
  {"forcsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Force Shield"}, // Force GET!
  {"frcssg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Weak Force Shield"}, // Force GET...? (consider making a custom shield with this instead of a single-hit force shield!)
  {"elemsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Elemental Shield"}, // Elemental GET!
  {"armasg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Armageddon Shield"}, // Armaggeddon GET!
  {"attrsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Attraction Shield"}, // Attract GET!
  {"shldls", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"}, // You LOSE!
  {"spdpad", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Speedpad"},
  {"spkdth", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spiked"},
  {"spring", false, 112,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spring"},
  {"statu1",  true,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pushing a statue"},
  {"statu2",  true,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pushing a statue"},
  {"strpst",  true, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cheatcheck"},
  {"supert",  true, 127,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Transformation"},
  {"telept", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dash"},
  {"tink" ,  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tink"},
  {"token" ,  true, 224,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Token"},
  {"trfire",  true,  60,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laser fired"},
  {"trpowr",  true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering up"},
  {"turhit", false,  40,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laser hit"},
  {"wdjump", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Whirlwind jump"},
  {"shrpsp",  true,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spincushion"},
  {"shrpgo", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Launch"},
  {"mswarp", false,  60, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spinning out"},
  {"mspogo",  true,  60,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Breaking through"},
  {"boingf", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bouncing"},
  {"corkp",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cork fired"},
  {"corkh",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cork hit"},
  {"alart",  false, 200,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Caught red handed!"},
  {"vwre",   false, 200,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Clone fighter!"},
  {"bowl",   false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bowling"},
  {"chuchu", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Train horn"},
  {"sprong", false, 112,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power spring"},
  {"lvfal1",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rumble"},
  {"pscree", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "SCREE!"},
  {"iceb",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ice crack"},
  {"shattr",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Shattering"},
  {"antiri",  true, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Depletion"},

  // Menu, interface
  {"chchng", false, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Score"},
  {"dwnind", false, 212,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thinking about air"},
  {"emfind", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Radar beep"},
  {"flgcap", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flag captured"},
  {"menu1",   true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Menu beep"},
  {"oneup",   true, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "One-up"},
  {"ptally",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tally"}, // Point tally is identical to menu for now
  {"radio",  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Notification"},
  {"wepchg",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Weapon change"}, // Weapon switch is identical to menu for now
  {"wtrdng",  true, 212,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aquaphobia"}, // make sure you can hear the DING DING! Tails 03-08-2000
  {"zelda",  false, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Discovery"},
  {"adderr",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Error"},
  {"notadd",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Reject"},
  {"addfil",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Accept"},

  // NiGHTS
  {"ideya",  false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Success"},
  {"xideya", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Success"}, // Xmas
  {"nbmper", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper"},
  {"nxbump", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper"}, // Xmas
  {"ncchip", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got chip"},
  {"ncitem", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got special"},
  {"nxitem", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got special"}, // Xmas
  {"ngdone",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bonus time start"},
  {"nxdone",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bonus time start"}, // Xmas
  {"drill1", false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drill"},
  {"drill2", false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drill"},
  {"ncspec", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power-up"}, // Tails 12-15-2003
  {"nghurt", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"},
  {"ngskid", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Force stop"},
  {"hoop1",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hoop"},
  {"hoop2",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hoop+"},
  {"hoop3",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hoop++"},
  {"hidden", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Discovery"},
  {"prloop", false, 104,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Gust of wind"},
  {"timeup",  true, 256,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous Countdown"},
  {"ngjump", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jump"},
  {"peww",   false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pew"},

  // Halloween
  {"lntsit", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cacolantern awake"},
  {"lntdie", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cacolantern death"},
  {"pumpkn", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pumpkin smash"}, // idspispopd
  {"ghosty", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laughter"},

  // Mario
  {"koopfr" , true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fire"},
  {"mario1", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hit"},
  {"mario2", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bonk"},
  {"mario3", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power-up"},
  {"mario4",  true,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got coin"},
  {"mario5", false,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Boot-stomp"},
  {"mario6", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jump"},
  {"mario7", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fire"},
  {"mario8", false,  48,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"},
  {"mario9",  true, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Emerging power-up"},
  {"marioa",  true, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "One-up"},
  {"thwomp",  true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thwomp"},

  // Black Eggman
  {"bebomb", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Big explosion"},
  {"bechrg", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering up"},
  {"becrsh", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crash"},
  {"bedeen", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic crash"},
  {"bedie1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman crying"},
  {"bedie2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman crying"},
  {"beeyow", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Failing machinery"},
  {"befall", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic slam"},
  {"befire", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing goop"},
  {"beflap", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical jump"},
  {"begoop", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powerful shot"},
  {"begrnd", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic grinding"},
  {"behurt", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman shocked"},
  {"bejet1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jetpack charge"},
  {"belnch", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical jump"},
  {"beoutb", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Failed shot"},
  {"beragh", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman screaming"},
  {"beshot", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing missile"},
  {"bestep", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical stomp"},
  {"bestp2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical stomp"},
  {"bewar1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bewar2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bewar3", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bewar4", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bexpld", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},
  {"bgxpld", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},

  // Cybrakdemon
  {"beelec", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electricity"},
  {"brakrl", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rocket launch"},
  {"brakrx", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rocket explosion"},

  // Sonic 1 sounds
  {"s1a0",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a1",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a2",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Monty Mole attach
  {"s1a3",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a4",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a5",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a6",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a7",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a8",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a9",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1aa",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1ab",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Monty Mole twitch
  {"s1ac",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Misc. obstacle destroyed
  {"s1ad",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1ae",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1af",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b0",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b1",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b2",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b3",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b4",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Toggle all items
  {"s1b5",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b6",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b7",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b8",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b9",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1ba",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Toggle item
  {"s1bb",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Thwomp drop
  {"s1bc",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1bd",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Ballhog bounce
  {"s1be",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1bf",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c0",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c1",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Monokuma death
  {"s1c2",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c3",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c4",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c5",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c6",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c7",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c8",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c9",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Item hit w/ voices off
  {"s1ca",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1cb",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1cc",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1cd",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1ce",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1cf",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Sonic 2 sounds
  {"s220",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s221",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Lap sound
  {"s222",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s223",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s224",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s225",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s226",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s227",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s228",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s229",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s230",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s231",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s232",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s233",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s234",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s235",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s236",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s237",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s238",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s239",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s23a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s23b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s23c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Drift boost
  {"s23d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s23e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Title screen opening
  {"s23f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s240",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s241",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s242",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s243",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s244",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s245",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s246",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s247",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s248",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s249",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s250",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s251",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s252",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s253",   false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // 1st place finish
  {"s254",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s255",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s256",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s257",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s258",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s259",   false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Item clashing
  {"s25a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Perfect start boost
  {"s260",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s261",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s262",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s263",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s264",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s265",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s266",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s267",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s268",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s269",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s26a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s26b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s26c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s26d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Missed checkpoint
  {"s26e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s26f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s270",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // S3&K sounds
  {"s3k2b",   true, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Emerald"}, // Got Emerald!
  {"s3k33",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"}, // stereo in original game, identical to latter
  {"s3k34",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"}, // mono in original game, identical to previous
  {"s3k35",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"},
  {"s3k36",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Skid"},
  {"s3k37",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spiked"},
  {"s3k38",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubble gasp"},
  {"s3k39",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Splash"},
  {"s3k3a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Shield"}, // Kart item shield
  {"s3k3b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drowning"},
  {"s3k3c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spin"},
  {"s3k3d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pop"},
  {"s3k3e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flame Shield"}, // Kart Flame Shield spawned
  {"s3k3f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubble Shield"}, // Kart Bubble Shield spawned
  {"s3k40",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Attraction blast"},
  {"s3k41",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thunder Shield"}, // Kart Thunder Shield spawned
  {"s3k42",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Twinspin"},
  {"s3k43",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flame burst"},
  {"s3k44",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubble bounce"}, // Kart Bubble Shield reflect
  {"s3k45",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning zap"},
  {"s3k46",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Transformation"},
  {"s3k47",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising dust"}, // Kart AIZ dust
  {"s3k48",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pulse"},
  {"s3k49",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Impact"}, // Kart bump sound
  {"s3k4a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Grab"},
  {"s3k4b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Splash"},
  {"s3k4c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Heavy hit"},
  {"s3k4d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing bullet"},
  {"s3k4e",  false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Big explosion"}, // Kart explosion
  {"s3k4f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flamethrower"},
  {"s3k50",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Siren"},
  {"s3k51",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling"},
  {"s3k52",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spike"},
  {"s3k53",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering up"},
  {"s3k54",  false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing"}, // MetalSonic shot fire
  {"s3k55",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical movement"},
  {"s3k56",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Heavy landing"},
  {"s3k57",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Burst"},
  {"s3k58",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical movement"},
  {"s3k59",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crumbling"},
  {"s3k5a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aiming"},
  {"s3k5b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Menu beep"},
  {"s3k5c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric spark"}, // Kart Mine tick
  {"s3k5d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Heavy hit"},
  {"s3k5e",  false, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Releasing charge"},
  {"s3k5f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crusher stomp"},
  {"s3k60",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Accelerating"},
  {"s3k61",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drilling"},
  {"s3k62",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jump"},
  {"s3k63",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cheatcheck"},
  {"s3k64",  false,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Clatter"},
  {"s3k65",  false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got sphere"}, // Blue Spheres
  {"s3k66",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Special stage end"},
  {"s3k67",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing missile"},
  {"s3k68",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Discovery"}, // Kart final lap
  {"s3k69",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Switch click"},
  {"s3k6a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Special stage clear"}, // Kart finish
  {"s3k6b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Punch"},
  {"s3k6c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Burst"},
  {"s3k6d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k6e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical damage"},
  {"s3k6f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},
  {"s3k70",   true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Burst"},
  {"s3k71",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Basic Shield"},
  {"s3k72",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Movement"},
  {"s3k73",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Warp"},
  {"s3k74",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Gong"},
  {"s3k75",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising"},
  {"s3k76",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Click"},
  {"s3k77",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Balloon pop"},
  {"s3k78",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Magnet"},
  {"s3k79",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric charge"},
  {"s3k7a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising from lava"},
  {"s3k7b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Soft bounce"},
  {"s3k7c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Magnet"},
  {"s3k7d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k7e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dust"},
  {"s3k7f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Freeze"},
  {"s3k80",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ice spike burst"},
  {"s3k81",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Burst"},
  {"s3k82",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Burst"},
  {"s3k83",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Collapsing"},
  {"s3k84",  false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering up"}, // Lightning Shield Charge
  {"s3k85",  false,  64, 24, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering down"},
  {"s3k86",  false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Alarm"},
  {"s3k87",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bounce"},
  {"s3k88",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic squeak"},
  {"s3k89",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Advanced tech"},
  {"s3k8a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Boing"},
  {"s3k8b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powerful hit"},
  {"s3k8c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Humming power"},
  {"s3k8d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "/"},
  {"s3k8e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Accelerating"},
  {"s3k8f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Opening"},
  {"s3k90",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Impact"},
  {"s3k91",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Closed"},
  {"s3k92",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ghost"},
  {"s3k93",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Gas release"},
  {"s3k94",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spike"},
  {"s3k95",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lava burst"},
  {"s3k96",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Landing"},
  {"s3k97",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wind"},
  {"s3k98",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling spike"},
  {"s3k99",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bounce"},
  {"s3k9a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Click"},
  {"s3k9b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crusher stomp"},
  {"s3k9c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Super Emerald"},
  {"s3k9d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Targeting"},
  {"s3k9e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wham"},
  {"s3k9f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Transformation"},
  {"s3ka0",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Launch"},
  {"s3ka1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3ka2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Launch"},
  {"s3ka3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising charge"},
  {"s3ka4",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering up"},
  {"s3ka5",  false,  64,  16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // bufo x8away
  {"s3ka6",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Attraction fizzle"},
  {"s3ka7",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Countdown beep"},
  {"s3ka8",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Energy"},
  {"s3ka9",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aquaphobia"},
  {"s3kaa",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper"},
  {"s3kab",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab3", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab4", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab5", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab6", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab7", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab8", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab9", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kaba", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kabb", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kabc", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kabd", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kabe", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kabf", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kac",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Continue"},
  {"s3kad",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "GO!"},
  {"s3kae",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pinball flipper"},
  {"s3kaf",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "To Special Stage"},
  {"s3kb0",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Score"},
  {"s3kb1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spring"},
  {"s3kb2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Failure"},
  {"s3kb3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Warp"},
  {"s3kb4",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},
  {"s3kb5",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Clink"},
  {"s3kb6",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spin launch"},
  {"s3kb7",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tumbler"},
  {"s3kb8",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spinning signpost"},
  {"s3kb9",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ring loss"},
  {"s3kba",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flight"},
  {"s3kbb",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tired flight"},
  {"s3kbcs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kbcl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // long version of previous
  {"s3kbds", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying fortress"},
  {"s3kbdl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying fortress"}, // ditto
  {"s3kbes", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying"},
  {"s3kbel", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying"}, // ditto
  {"s3kbfs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turbine"},
  {"s3kbfl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turbine"}, // ditto
  {"s3kc0s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turbine"},
  {"s3kc0l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turbine"}, // ditto
  {"s3kc1s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fan"},
  {"s3kc1l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fan"}, // ditto
  {"s3kc2s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flamethrower"},
  {"s3kc2l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flamethrower"}, // ditto
  {"s3kc3s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Levitation"},
  {"s3kc3l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Levitation"}, // ditto
  {"s3kc4s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing laser"},
  {"s3kc4l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing laser"}, // ditto
  {"s3kc5s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Revving up"},
  {"s3kc5l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Revving up"}, // ditto
  {"s3kc6s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Orbiting"},
  {"s3kc6l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Orbiting"}, // ditto
  {"s3kc7s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aiming"},
  {"s3kc7l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aiming"}, // ditto
  {"s3kc8s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sliding"},
  {"s3kc8l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sliding"}, // ditto
  {"s3kc9s", false,  64, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, "Swinging"},
  {"s3kc9l", false,  64, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, "Swinging"}, // ditto
  {"s3kcas", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Energy"},
  {"s3kcal", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Energy"}, // ditto
  {"s3kcbs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},
  {"s3kcbl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"}, // ditto
  {"s3kccs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bursting"},
  {"s3kccl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bursting"}, // ditto
  {"s3kcds", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},
  {"s3kcdl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"}, // ditto
  {"s3kces", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wind tunnel"},
  {"s3kcel", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dust devil"}, // ditto
  {"s3kcfs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kcfl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kd0s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising"},
  {"s3kd0l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising"}, // ditto
  {"s3kd1s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kd1l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kd2s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turning"},
  {"s3kd2l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Moving chain"}, // ditto
  {"s3kd3s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Digging"},
  {"s3kd3l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Digging"}, // ditto
  {"s3kd4s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Engine"},
  {"s3kd4l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Engine"}, // ditto
  {"s3kd5s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling lava"},
  {"s3kd5l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling lava"}, // ditto
  {"s3kd6s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kd6l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kd7s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Movement"},
  {"s3kd7l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Movement"}, // ditto
  {"s3kd8s", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Acceleration"}, // Sharp Spin (maybe use the long/L version?)
  {"s3kd8l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Acceleration"}, // ditto
  {"s3kd9s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Magnetism"},
  {"s3kd9l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Magnetism"}, // ditto
  {"s3kdas", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Click"},
  {"s3kdal", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Click"}, // ditto
  {"s3kdbs", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running on water"},
  {"s3kdbl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running on water"}, // ditto

  // 3D Blast sounds (the "missing" ones are direct copies of S3K's, no minor differences what-so-ever)
  {"3db06",  false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Collection"},
  {"3db09",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Peep"},
  {"3db14",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Chirp"},
  {"3db16",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Sonic CD sounds
  {"cdfm00", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Skid"},
  {"cdfm01", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm02", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jump"},
  {"cdfm03", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dying"},
  {"cdfm04", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ring loss"},
  {"cdfm05", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"},
  {"cdfm06", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pop"},
  {"cdfm07", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Shield"},
  {"cdfm08", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spring"},
  {"cdfm09", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm10", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm11", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm12", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm13", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm14", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm15", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm16", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm17", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm18", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm19", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm20", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm21", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm22", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm23", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm24", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm25", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm26", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm27", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm28", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm29", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubble gasp"},
  {"cdfm30", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cheatcheck"},
  {"cdfm31", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Warp"},
  {"cdfm32", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm33", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm34", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm35", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm36", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm37", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm38", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drowning"},
  {"cdfm39", false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm40", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power up"},
  {"cdfm41", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm42", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm43", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm44", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Extra time"},
  {"cdfm45", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm46", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm47", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm48", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm49", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aquaphobia"},
  {"cdfm50", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm51", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm52", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm53", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm54", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm55", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm56", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Warp"},
  {"cdfm57", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm58", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm59", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm60", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm61", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm62", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Speed boost"},
  {"cdfm63", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm64", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm65", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm66", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm67", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm68", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm69", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm70", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm71", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm72", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm73", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm74", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Broly ki
  {"cdfm75", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm76", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm77", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm78", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm79", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdpcm0", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Future."},
  {"cdpcm1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Past."},
  {"cdpcm2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "All right!"},
  {"cdpcm3", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "I'm outta here..."},
  {"cdpcm4", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Yes!"},
  {"cdpcm5", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Yeah!"},
  {"cdpcm6", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Giggles"},
  {"cdpcm7", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eep!"},
  {"cdpcm8", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdpcm9", false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper"},

  // Knuckles Chaotix sounds
  {"kc2a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc2b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc2c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc2d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc2e",   false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc2f",   false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc30",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc31",   false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc32",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc33",   false,  64,  16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, //x8away
  {"kc34",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc35",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc36",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc37",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc38",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc39",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc40",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc41",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc42",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Confirm"},
  {"kc43",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc44",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc45",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc46",   false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc47",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc48",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Select"},
  {"kc49",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc4a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc4b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc4c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pop-shot"},
  {"kc4d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power up"},
  {"kc4e",   false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, //x8away
  {"kc4f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc50",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc51",   false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc52",   false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc53",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc54",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc55",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc56",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc57",   false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sheer terror"},
  {"kc58",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc59",   false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Shrink"},
  {"kc5a",   false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Grow"},
  {"kc5b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc5c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc5d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc5e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc5f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc60",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc61",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc62",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc63",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc64",   false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Terrifying rumble"},
  {"kc65",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power down"},
  {"kc66",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc67",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc68",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc69",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc6b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ascending"},
  {"kc6c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc6d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc6e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Mean Bean Machine sounds
  {"mbs41",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs42",  true,   64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs43",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs44",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs45",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs46",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs47",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs48",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs49",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs4a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs4b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs4c",  true,   64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs4d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs4e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs4f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs50",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs51",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs52",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs53",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs54",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs55",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs56",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs57",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs58",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs59",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs5a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs5b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs5c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs5d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs5e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs5f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs60",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs61",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs62",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs63",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs64",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs67",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs68",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs69",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs6a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs6b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs6d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs6e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs70",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs71",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbs72",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv81",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv82",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv83",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv84",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv85",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv86",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv87",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv88",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv89",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv8a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv8b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv8c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv8d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv8e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv8f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv90",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv91",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv92",  false,  64,  16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Changed falloff for use in instashield parry.
  {"mbv93",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv94",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv95",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv96",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"mbv97",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // SegaSonic Arcade sounds
  {"ssa001", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa002", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa003", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa004", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa005", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa006", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa007", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa008", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa009", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa010", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa011", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa012", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa013", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa014", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa015", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa016", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa017", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa018", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa019", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa020", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa021", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa022", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa023", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa024", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa025", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa026", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa027", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa028", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa029", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa030", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa031", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa032", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa033", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa034", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa035", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa036", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa037", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa038", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa039", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa040", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa041", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa042", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa043", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa044", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa045", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa046", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa047", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa048", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa049", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa050", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa051", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa052", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa053", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa054", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa055", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa056", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa057", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa058", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa059", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa060", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa061", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa062", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa063", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa064", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa065", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa066", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa067", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa068", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa069", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa070", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa071", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa072", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa073", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa074", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa075", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa076", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa077", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa078", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa079", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa080", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa081", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa082", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa083", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa084", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa085", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa086", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa087", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa088", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa089", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa090", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa091", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa092", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa093", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa094", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa095", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa096", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa097", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa098", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa099", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa100", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa101", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa102", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa103", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa104", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa105", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa106", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa107", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa108", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa109", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa110", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa111", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa112", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa113", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa114", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa115", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa116", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa117", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa118", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa119", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa120", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa121", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa122", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa123", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa124", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa125", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa126", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa127", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa128", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa129", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"ssa130", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // SRB2kart
  {"slip",   false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Spinout
  {"screec", false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Tight turning screech
  {"drift",  false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Drifting ambient
  {"ruburn", false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Rubber-burn turn ambient
  {"ddash",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Respawn Drop Dash
  {"tossed", false, 192,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Item fired
  {"peel",   false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Edited S25A for banana landing
  {"hogbom", false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Ballhog explosions
  {"zio3",   false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Thunder Shield use
  {"kpogos", false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Pogo Spring use
  {"alarmi", false, 255,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Invincibility alarm
  {"alarmg", false, 255,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Grow alarm
  {"itrol1",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Roulette spinning
  {"itrol2",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"itrol3",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"itrol4",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"itrol5",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"itrol6",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"itrol7",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"itrol8",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"itrolf",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Roulette end
  {"itrolm",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Roulette end (mashed)
  {"itrolk",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Roulette end (karma enhanced)
  {"itrole",  true,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Roulette end (Eggman)
  {"vroom",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Kart Krew opening vroom
  {"chaooo", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Chao audience cheer
  {"yeeeah", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Voting picks you
  {"noooo1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Voting picks hell (by chance)
  {"noooo2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Voting picks hell (on purpose)
  {"ruby1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Phantom Ruby charge up
  {"ruby2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Phantom Ruby teleport
  {"tcart",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Twinkle Cart
  {"bfare",  false, 128,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Pleasure Castle fanfare
  {"merry",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Pleasure Castle merry-go-round ambient
  {"bowlh",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Pleasure Castle pins
  {"tppop",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Pleasure Castle bombs
  {"hsdoor", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Red Barrage Area door
  {"hstrn",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Red Barrage Area train
  {"aspkb",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Red Barrage Area spikeballs
  {"wind1",  false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Midnight Channel monsters
  {"fire2",  false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"chain",  false, 255,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Mementos Reaper
  {"mkuma",  false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Trigger Happy Havoc Monokuma
  {"toada",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Arid Sands Toad scream
  {"bhurry", false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // v1.0.2 Battle overtime
  {"bsnipe", false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Banana sniping
  {"sploss", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Down to yellow sparks
  {"join",    true,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Player joined server
  {"leave",   true,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Player left server
  {"requst",  true,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Got a Discord join request
  {"syfail",  true,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Funny sync failure
  {"itfree", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // :shitsfree:
  {"dbgsal", false, 255,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Debug notification
  {"cock",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Hammer cocks, bang bang
  {"itcaps", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, "Item capsule"},
  {"kstart", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Sonic Adventure shwing!
  {"typri1", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SA2 boss typewriting 1
  {"typri2", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SA2 final boss-type typewriting
  {"eggspr", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Sonic Unleashed Trap Spring
  {"achiev", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Achievement"},
  {"keygen", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Key Generated"},
  {"gpmetr", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // SRB2Kart - Ring Box
  {"slot00", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bar"},
  {"slot01", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Double Bar"},
  {"slot02", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Triple Bar"},
  {"slot03", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Slot Ring"},
  {"slot04", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Seven"},
  {"slot05", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "JACKPOT!"},

  // RR - Flame Shield
  {"fshld0", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flame Shield activate"},
  {"fshld1", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flame Shield hold"},
  {"fshld2", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flame Shield burst"},
  {"fshld3", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flame Shield cooldown"},

  // RR - Trick Panel
  {"trick0", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Trick confirm"},
  {"trick1", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Trick"},

  // RR - Ballhog Charge
  {"bhog00", false, 64,  64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ballhog charging"},
  {"bhog01", false, 64,  64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ballhog charging"},
  {"bhog02", false, 64,  64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ballhog charging"},
  {"bhog03", false, 64,  64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ballhog charging"},
  {"bhog04", false, 64,  64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ballhog charging"},
  {"bhog05", false, 64,  64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ballhog charging"},

  // RR - Gachabom rebound
  {"grbnd1", false, 64,  64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Gachabom returning"},
  {"grbnd2", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Gachabom orbiting"},
  {"grbnd3", false, 64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Gachabom re-collected"},

  // SRB2Kart - Drop target sounds
  {"kdtrg1", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Low energy, SF_X8AWAYSOUND
  {"kdtrg2", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Medium energy, SF_X8AWAYSOUND
  {"kdtrg3", false,  64, 80, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // High energy, SF_X2AWAYSOUND|SF_X8AWAYSOUND

    // SRB2kart - Grow/invinc clash
  {"parry",  false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X8AWAYSOUND

  {"ffbonc", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Shout message sound effect
  {"sysmsg", false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Server notification"},

  // Shrink laser beam
  {"beam01", false,  32, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // SPB seeking
  {"spbska", false,  32, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"spbskb", false,  32, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"spbskc", false,  32, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Juicebox for SPB
  {"gate01", false,  32, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gate02", false,  32, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gate03", false,  32, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gate04", false,  32, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gate05", false,  32, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Wavedash
  {"waved1", false,  32, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"waved2", false,  32, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"waved3", false,  32, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"waved4", false,  32, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"waved5", false,  32, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Sonic Boom & Subsonic
  {"sonbo1", false,  32, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"sonbo2", false,  32, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"sonbo3", false,  32, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Passing sounds
  {"pass01", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass02", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass03", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass04", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass05", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass06", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass07", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass08", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass09", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass10", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass11", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass12", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass13", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass14", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass15", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"pass16", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // SRB2Kart - Blocked damage
  {"grownd", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X8AWAYSOUND
  {"invind", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X8AWAYSOUND

    // SRB2Kart - Claw SFX
  {"claw01", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw02", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw03", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw04", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw05", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw06", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw07", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw08", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw09", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw10", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw11", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw12", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw13", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw14", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw15", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"claw16", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"clawht", false,  64, 8, -1, NULL, 0,         -1,  -1, LUMPERROR, ""}, // SF_X4AWAYSOUND
  {"clawzm", false,  64, 16, -1, NULL, 0,         -1,  -1, LUMPERROR, ""}, // SF_X8AWAYSOUND
  {"clawk1", false,  64, 16, -1, NULL, 0,         -1,  -1, LUMPERROR, ""}, // SF_X8AWAYSOUND
  {"clawk2", false,  64, 16, -1, NULL, 0,         -1,  -1, LUMPERROR, ""}, // SF_X8AWAYSOUND

  // SRB2Kart - whip charge/hold
  {"wchrg1", false,  64, 64, -1, NULL, 0,         -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND
  {"wchrg2", false,  64, 64, -1, NULL, 0,         -1,  -1, LUMPERROR, ""}, // SF_X2AWAYSOUND

  {"horn00", false,  255, 0, -1, NULL, 0,         -1,  -1, LUMPERROR, "/"}, // HORNCODE
  {"melody", false,  255, 0, -1, NULL, 0,         -1,  -1, LUMPERROR, "/"}, // Mystic Melody
  {"cdsprk", false,  255, 0, -1, NULL, 0,         -1,  -1, LUMPERROR, "/"}, // Prison Egg CD sparkling
  {"monch",  false,  255, 0, -1, NULL, 0,         -1,  -1, LUMPERROR, ""},
  {"etexpl", false,  255, 0, -1, NULL, 0,         -1,  -1, LUMPERROR, "Game crash"},
  {"d4getm", false,  255, 0, -1, NULL, 0,         -1,  -1, LUMPERROR, "Don't forget me"},

  {"iwhp", false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Instawhip attack
  {"gbrk", false, 255,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Guard break!

  // Super Flicky Power-Up
  {"supflk", false, 255, 32, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Woodpecking - SF_NOINTERRUPT
  {"fbost1", false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Slowing down
  {"fbird",  false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Speeding up
  {"fhurt1", false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Whipped
  {"fhurt2", false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Hunting

  {"dashr",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Regular Dash Ring
  {"rainbr", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Rainbow Dash Ring

  {"rank",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Rank slam

  {"ridr1",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Boarding Rideroid"}, // Rideroid Activation
  {"ridr2",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Rideroid Diveroll
  {"ridr3",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Rideroid Loop
  {"ridr4",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Leaving Rideroid

  {"befan1",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Whisking"}, // Blend Eye whisk startup
  {"befan2",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Whisking"}, // Blend Eye whisk

  {"glgz1",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ice Cube shatters"},

  {"cratew",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crate shatters"},
  {"cratem",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crate shatters"},

  {"ivobal",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Ivo Ball

  {"lcfuel",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fuel Capsule explodes"},

  {"ssthnk",  false,  64,  16, -1, NULL, 0,        -1,  -1, LUMPERROR, "Chain rattles"}, // SF_X8AWAYSOUND
  {"powerd",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "UNDESCRIBED POWERD"},
  {"vault",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "UNDESCRIBED VAULT"},
  {"revcym",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "UNDESCRIBED REVCYM"},
  {"ssbmpr",  false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "UNDESCRIBED SSBMPR"}, // SF_X4AWAYSOUND
  {"chcrun",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "UNDESCRIBED CHCRUN"},

  {"hint",    false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hint Ring"},

  {"exp",    false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "EXP Crystal"}, // When it gets sucked in


  // Damage sounds
  {"dmga1",  false, 255, 8, -1, NULL, 0,         -1,  -1, LUMPERROR, "Damaged"},
  {"dmga2",  false, 255, 8, -1, NULL, 0,         -1,  -1, LUMPERROR, "Damaged"},
  {"dmga3",  false, 255, 8, -1, NULL, 0,         -1,  -1, LUMPERROR, "Damaged"},
  {"dmga4",  false, 255, 8, -1, NULL, 0,         -1,  -1, LUMPERROR, "Damaged"},
  {"dmgb1",  false, 255, 8, -1, NULL, 0,         -1,  -1, LUMPERROR, "Damaged"},
  {"dmgb2",  false, 255, 8, -1, NULL, 0,         -1,  -1, LUMPERROR, "Damaged"},
  {"dmgb3",  false, 255, 8, -1, NULL, 0,         -1,  -1, LUMPERROR, "Damaged"},
  {"dmgb4",  false, 255, 8, -1, NULL, 0,         -1,  -1, LUMPERROR, "Damaged"},

  // Powerup sounds
  {"bpwrua",  false, 255, 16, -1, NULL, 0,         -1,  -1, LUMPERROR, "Super Power"},
  {"bpwrub",  false, 255, 16, -1, NULL, 0,         -1,  -1, LUMPERROR, "Mega Barrier"},
  {"bpwruc",  false, 255, 16, -1, NULL, 0,         -1,  -1, LUMPERROR, "Bumper Restock"},
  {"bpwrud",  false, 255, 16, -1, NULL, 0,         -1,  -1, LUMPERROR, "Rhythm Badge"},
  {"bpwrue",  false, 255, 16, -1, NULL, 0,         -1,  -1, LUMPERROR, "Super Flicky"},
  {"bpwruf",  false, 255, 16, -1, NULL, 0,         -1,  -1, LUMPERROR, "Bonus"},

  // Misc announcer calls
  {"duelmb",  false, 255, 16, -1, NULL, 0,         -1,  -1, LUMPERROR, "Margin Boost"},

  // SRB2Kart - Engine sounds
  // Engine class A
  {"krta00", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta01", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta02", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta03", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta04", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta05", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta06", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta07", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta08", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta09", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta10", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta11", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krta12", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  // Engine class B
  {"krtb00", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb01", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb02", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb03", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb04", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb05", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb06", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb07", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb08", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb09", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb10", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb11", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtb12", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  // Engine class C
  {"krtc00", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc01", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc02", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc03", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc04", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc05", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc06", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc07", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc08", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc09", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc10", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc11", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtc12", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  // Engine class D
  {"krtd00", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd01", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd02", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd03", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd04", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd05", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd06", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd07", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd08", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd09", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd10", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd11", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtd12", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  // Engine class E
  {"krte00", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte01", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte02", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte03", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte04", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte05", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte06", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte07", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte08", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte09", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte10", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte11", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krte12", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  // Engine class F
  {"krtf00", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf01", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf02", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf03", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf04", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf05", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf06", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf07", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf08", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf09", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf10", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf11", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtf12", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  // Engine class G
  {"krtg00", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg01", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg02", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg03", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg04", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg05", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg06", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg07", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg08", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg09", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg10", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg11", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krtg12", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  // Engine class H
  {"krth00", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth01", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth02", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth03", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth04", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth05", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth06", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth07", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth08", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth09", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth10", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth11", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krth12", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  // Engine class I
  {"krti00", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti01", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti02", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti03", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti04", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti05", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti06", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti07", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti08", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti09", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti10", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti11", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"krti12", false,  48, 65, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Gunstar Heroes
  {"gsha0", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsha1", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsha2", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsha3", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsha4", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsha5", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsha6", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsha7", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsha7l",false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsha8", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsha9", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshaa", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshab", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshac", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshad", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshae", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshaf", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshb0", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshb1", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshb2", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, //x8away
  {"gshb3", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshb4", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshb5", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshb6", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshb7", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshb8", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshb9", false,  64, 8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, //x4away
  {"gshba", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshbb", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshbc", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshbd", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, //x8away
  {"gshbe", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshbf", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc0a", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc0b", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc0c", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc0d", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc0e", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc0f", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc0g", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc1", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc2", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc3", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc4", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc5", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, //x8away, ProxMineBOOM!
  {"gshc6", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc7", false,  64, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, //x8away
  {"gshc8", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshc9", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshca", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshcb", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshcc", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshcd", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshce", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshcf", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshd0", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshd1", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshd2", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshd3", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshd4", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshd5", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshd6", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshd7", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshd8", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshd9", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshda", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshdb", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshdc", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshdd", false,  96, 8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, //New ballhog explosion
  {"gshde", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshdf", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshe0", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshe1", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshe2", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshe3", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshe4", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshe5", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshe6", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshe7", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshe8", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshe9", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshea", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gsheb", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshec", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshed", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshee", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshef", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshf0", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"gshf1", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Pinball
  {"cftbl0", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cftbl1", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cftbl2", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // S-erotonin
  {"srank", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  
  // Generic menu SFX
  {"tmxatt", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Attention
  {"tmxawd", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Award
  {"tmxbak", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Back
  {"tmxdsm", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Dismiss
  {"tmxerr", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Error
  {"tmxfwd", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Forward
  {"tmxnah", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // "Nah"
  {"tmxqst", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Question
  {"tmxsuc", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Success
  {"tmxunx", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Unexpected item in bagging area
  {"tmxbdn", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Button down
  {"tmxbup", false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Button up

  // SMS
  {"sting0", false,  64, 2, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // Ring loss

  // Patching up base sounds
  {"s226l",  false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // s2 spikes LOUD

  // Destroyed Kart
  {"die00",  false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"die01",  false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"die02",  false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"die03",  false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Walltransfer
  {"ggfall",  false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // :apple:
  {"aple",  false,  64, 0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Ancient Gear
  {"gotgea", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // SRB2kart - Skin sounds
  {"kwin",   false,  64, 96, -1, NULL, 0,   SKSKWIN,  -1, LUMPERROR, ""},
  {"klose",  false,  64, 96, -1, NULL, 0,  SKSKLOSE,  -1, LUMPERROR, ""},
  {"khurt1", false,  64, 96, -1, NULL, 0,  SKSKPAN1,  -1, LUMPERROR, ""},
  {"khurt2", false,  64, 96, -1, NULL, 0,  SKSKPAN2,  -1, LUMPERROR, ""},
  {"kattk1", false,  64, 96, -1, NULL, 0,  SKSKATK1,  -1, LUMPERROR, ""},
  {"kattk2", false,  64, 96, -1, NULL, 0,  SKSKATK2,  -1, LUMPERROR, ""},
  {"kbost1", false,  64, 96, -1, NULL, 0,  SKSKBST1,  -1, LUMPERROR, ""},
  {"kbost2", false,  64, 96, -1, NULL, 0,  SKSKBST2,  -1, LUMPERROR, ""},
  {"kslow",  false,  64, 32, -1, NULL, 0,  SKSKSLOW,  -1, LUMPERROR, ""},
  {"khitem", false, 128, 32, -1, NULL, 0,  SKSKHITM,  -1, LUMPERROR, ""},
  {"kgloat", false,  64, 48, -1, NULL, 0,  SKSKPOWR,  -1, LUMPERROR, ""},
  {"ktalk",  false,  64, 48, -1, NULL, 0,  SKSKTALK,  -1, LUMPERROR, ""},

  // skin sounds free slots to add sounds at run time (Boris HACK!!!)
  // initialized to NULL
};

char freeslotnames[sfx_freeslot0 + NUMSFXFREESLOTS + NUMSKINSFXSLOTS][7];

// Prepare free sfx slots to add sfx at run time
void S_InitRuntimeSounds (void)
{
	sfxenum_t i;
	INT32 value;
	char soundname[10];

	for (i = sfx_freeslot0; i <= sfx_lastskinsoundslot; i++)
	{
		value = (i+1) - sfx_freeslot0;

		if (value < 10)
			sprintf(soundname, "fre00%d", value);
		else if (value < 100)
			sprintf(soundname, "fre0%d", value);
		else if (value < 1000)
			sprintf(soundname, "fre%d", value);
		else
			sprintf(soundname, "fr%d", value);

		strcpy(freeslotnames[value-1], soundname);

		S_sfx[i].name = freeslotnames[value-1];
		S_sfx[i].singularity = false;
		S_sfx[i].priority = 0;
		S_sfx[i].pitch = 0;
		S_sfx[i].volume = -1;
		S_sfx[i].data = NULL;
		S_sfx[i].length = 0;
		S_sfx[i].skinsound = -1;
		S_sfx[i].usefulness = -1;
		S_sfx[i].lumpnum = LUMPERROR;
		//strlcpy(S_sfx[i].caption, "", 1);
		S_sfx[i].caption[0] = '\0';
	}
}

sfxenum_t sfxfree = sfx_freeslot0;

// Add a new sound fx into a free sfx slot.
//
sfxenum_t S_AddSoundFx(const char *name, boolean singular, INT32 flags, boolean skinsound)
{
	sfxenum_t i;

	if (skinsound)
	{
		for (i = sfx_skinsoundslot0; i < NUMSFX; i++)
		{
			if (S_sfx[i].priority)
				continue;
			break;
		}
	}
	else
		i = sfxfree;

	if (i < NUMSFX)
	{
		strncpy(freeslotnames[i-sfx_freeslot0], name, 6);
		S_sfx[i].singularity = singular;
		S_sfx[i].priority = 60;
		S_sfx[i].pitch = flags;
		S_sfx[i].volume = -1;
		S_sfx[i].lumpnum = LUMPERROR;
		S_sfx[i].skinsound = -1;
		S_sfx[i].usefulness = -1;

		/// \todo if precached load it here
		S_sfx[i].data = NULL;

		if (!skinsound)
			sfxfree++;

		return i;
	}
	I_Error("Out of Sound Freeslots while allocating \"%s\"\nLoad less addons to fix this.", name);
	return 0;
}

void S_RemoveSoundFx(sfxenum_t id)
{
	if (id >= sfx_freeslot0 && id <= sfx_lastskinsoundslot
		&& S_sfx[id].priority != 0)
	{
		S_sfx[id].lumpnum = LUMPERROR;
		I_FreeSfx(&S_sfx[id]);
		S_sfx[id].priority = 0;
	}
}
