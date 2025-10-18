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
/// \file  info.c
/// \brief Thing frame/state LUT

// Data.
#include "doomdef.h"
#include "doomstat.h"
#include "sounds.h"
#include "p_mobj.h"
#include "p_local.h" // DMG_ constants
#include "m_misc.h"
#include "z_zone.h"
#include "d_player.h"
#include "v_video.h" // V_*MAP constants
#include "lzf.h"

// Hey, moron! If you change this table, don't forget about the sprite enum in info.h and the sprite lights in hw_light.c!
// EXCEPT HW_LIGHT.C DOESN'T EXIST ANYMORE LOVE CONTINUOUSLY FALLING ON MY ASS THROUGHOUT THIS CODEBASE - Tyron 2022-05-12
// For the sake of constant merge conflicts, let's spread this out
char sprnames[NUMSPRITES + 1][5] =
{
	"NULL", // invisible object
	"NONE", // invisible but still rendered
	"UNKN",

	"THOK", // Thok! mobj
	"PLAY",
	"KART",
	"TIRE",

	// Enemies
	"POSS", // Crawla (Blue)

	"BARX", // bomb explosion (also used by barrel)
	"BARD", // bomb dust (also used by barrel)

	// Collectible Items
	"RING",
	"DEBT",
	"BSPH", // Sphere
	"EMBM",
	"SPCN", // Spray Can
	"SBON", // Spray Can replacement bonus
	"MMSH", // Ancient Shrine
	"MORB", // One Morbillion
	"EMRC", // Chaos Emeralds
	"SEMR", // Super Emeralds
	"ESPK",

	// Prison Egg Drops
	"ALTM",

	// Interactive Objects
	"BBLS", // water bubble source
	"SIGN", // Level end sign
	"SPIK", // Spike Ball
	"SFLM", // Spin fire
	"USPK", // Floor spike
	"WSPK", // Wall spike
	"WSPB", // Wall spike base

	// Projectiles
	"CBLL", // Cannonball
	"CFIR", // Colored fire of various sorts

	// Greenflower Scenery
	"FWR1",
	"FWR2", // GFZ Sunflower
	"FWR3", // GFZ budding flower
	"FWR4",
	"BUS1", // GFZ Bush w/ berries
	"BUS2", // GFZ Bush w/o berries
	"BUS3", // GFZ Bush w/ BLUE berries
	// Trees (both GFZ and misc)
	"TRE1", // GFZ
	"TRE2", // Checker
	"TRE3", // Frozen Hillside
	"TRE4", // Polygon
	"TRE5", // Bush tree
	"TRE6", // Spring tree

	// Techno Hill Scenery
	"THZP", // THZ1 Steam Flower
	"FWR5", // THZ1 Spin flower (red)
	"FWR6", // THZ1 Spin flower (yellow)
	"THZT", // Steam Whistle tree/bush
	"ALRM", // THZ2 Alarm

	// Deep Sea Scenery
	"GARG", // Deep Sea Gargoyle
	"SEWE", // Deep Sea Seaweed
	"DRIP", // Dripping water
	"CORL", // Coral
	"BCRY", // Blue Crystal
	"KELP", // Kelp
	"ALGA", // Animated algae top
	"ALGB", // Animated algae segment
	"DSTG", // DSZ Stalagmites
	"LIBE", // DSZ Light beam

	// Castle Eggman Scenery
	"CHAN", // CEZ Chain
	"FLAM", // Flame
	"ESTA", // Eggman esta una estatua!
	"SMCH", // Small Mace Chain
	"BMCH", // Big Mace Chain
	"SMCE", // Small Mace
	"BMCE", // Big Mace

	"SFBR", // Small Firebar
	"BFBR", // Big Firebar
	"BANR", // Banner/pole
	"PINE", // Pine Tree
	"CEZB", // Bush
	"CNDL", // Candle/pricket
	"FLMH", // Flame holder
	"CTRC", // Fire torch
	"CFLG", // Waving flag/segment
	"CSTA", // Crawla statue
	"CABR", // Brambles

	// Arid Canyon Scenery
	"BTBL", // Big tumbleweed
	"STBL", // Small tumbleweed
	"CACT", // Cacti
	"WWSG", // Caution Sign
	"WWS2", // Cacti Sign
	"WWS3", // Sharp Turn Sign
	"OILL", // Oil lamp
	"OILF", // Oil lamp flare
	"BARR", // TNT barrel
	"REMT", // TNT proximity shell
	"TAZD", // Dust devil
	"ADST", // Arid dust

	// Red Volcano Scenery
	"FLME", // Flame jet
	"DFLM", // Blade's flame
	"LFAL", // Lavafall
	"JPLA", // Jungle palm
	"TFLO", // Torch flower
	"WVIN", // Wall vines

	// Dark City Scenery

	// Egg Rock Scenery

	// Christmas Scenery
	"XMS1", // Christmas Pole
	"XMS2", // Candy Cane
	"XMS3", // Snowman
	"XMS4", // Lamppost
	"XMS5", // Hanging Star
	"XMS6", // Mistletoe
	"FHZI", // FHZ Ice

	// Halloween Scenery
	"PUMK", // Pumpkins
	"HHPL", // Dr Seuss Trees
	"SHRM", // Mushroom
	"HHZM", // Misc

	// Azure Temple Scenery
	"BGAR", // ATZ Gargoyles
	"CFLM", // Green torch flame

	// Botanic Serenity Scenery
	"BSZ1", // Tall flowers
	"BSZ2", // Medium flowers
	"BSZ3", // Small flowers
	"BSZ4", // Tulips
	"BST1", // Red tulip
	"BST2", // Purple tulip
	"BST3", // Blue tulip
	"BST4", // Cyan tulip
	"BST5", // Yellow tulip
	"BST6", // Orange tulip
	"BSZ5", // Cluster of Tulips
	"BSZ6", // Bush
	"BSZ7", // Vine
	"BSZ8", // Misc things

	// Misc Scenery
	"STLG", // Stalagmites
	"DBAL", // Disco

	// Powerup Indicators
	"SSPK", // Super Sonic Spark

	// Flickies
	"FBUB", // Flicky-sized bubble
	"FL01", // Bluebird
	"FL02", // Rabbit
	"FL03", // Chicken
	"FL04", // Seal
	"FL05", // Pig
	"FL06", // Chipmunk
	"FL07", // Penguin
	"FL08", // Fish
	"FL09", // Ram
	"FL10", // Puffin
	"FL11", // Cow
	"FL12", // Rat
	"FL13", // Bear
	"FL14", // Dove
	"FL15", // Cat
	"FL16", // Canary
	"FS01", // Spider
	"FS02", // Bat

	// Springs
	"STEM", // Steam riser
	"BLON", // Balloons
	"SPVY", // Yellow Vertical Spring
	"SPVR", // Red Vertical Spring
	"SPVB", // Blue Vertical Spring
	"SPVG", // Grey Vertical Spring
	"SPDY", // Yellow Diagonal Spring
	"SPDR", // Red Diagonal Spring
	"SPDB", // Blue Diagonal Spring
	"SPDG", // Grey Diagonal Spring
	"SPHY", // Yellow Horizontal Spring
	"SPHR", // Red Horizontal Spring
	"SPHB", // Blue Horizontal Spring
	"SPHG", // Grey Horizontal Spring
	"POGS", // Pogo Spring

	// Environmental Effects
	"RAIN", // Rain
	"SNO1", // Snowflake
	"SNO2", // Blizzard Snowball
	"SPLH", // Water Splish
	"LSPL", // Lava Splish
	"SPLA", // Water Splash
	"SMOK",
	"BUBL", // Bubble
	"WZAP",
	"DUST", // Spindash dust
	"FPRT", // Spindash dust (flame)
	"SEED", // Sonic CD flower seed
	"PRTL", // Particle (for fans", etc.)

	// Game Indicators
	"DRWN", // Drowning Timer

	"CORK",
	"LHRT",

	// NiGHTS Stuff
	"HOOP",
	"CAPS", // Capsule thingy for NiGHTS

	// Secret badniks and hazards", shhhh
	"FMCE",
	"HMCE",
	"HBAT",

	// Debris
	"SPRK", // Sparkle
	"BOM1", // Robot Explosion
	"BOM2", // Boss Explosion 1
	"BOM3", // Boss Explosion 2
	"BOM4", // Underwater Explosion
	"LSSJ", // My ki is overflowing!!

	// Crumbly rocks
	"ROIA",
	"ROIB",
	"ROIC",
	"ROID",
	"ROIE",
	"ROIF",
	"ROIG",
	"ROIH",
	"ROII",
	"ROIJ",
	"ROIK",
	"ROIL",
	"ROIM",
	"ROIN",
	"ROIO",
	"ROIP",

	// Level debris
	"GFZD", // GFZ debris
	"BRIC", // Bricks
	"WDDB", // Wood Debris

	//SRB2kart Sprites (sort later)
	"RNDM", // Random Item Box
	"SBOX", // Sphere Box (for Battle)
	"RBOX", // Ring Box
	"ITRI", // Item Box Debris
	"ITPA", // Paper item backdrop
	"SGNS", // Signpost sparkle
	"FAST", // Speed boost trail
	"DSHR", // Speed boost dust release
	"BOST", // Sneaker booster flame
	"BOSM", // Sneaker booster smoke
	"KFRE", // Sneaker fire trail
	"KINV", // Lighter invincibility sparkle trail
	"KINB", // Darker invincibility sparkle trail
	"KINF", // Invincibility flash
	"INVI", // Invincibility speedlines
	"ICAP", // Item capsules
	"IMON", // Item Monitor
	"MGBX", // Heavy Magician transform box
	"MGBT", // Heavy Magician transform box top
	"MGBB", // Heavy Magician transform box bottom
	"SSMA", // Mine radius
	"SSMB",
	"SSMC",
	"SSMD",
	"MSHD", // Item Monitor Big Shard
	"IMDB", // Item Monitor Small Shard (Debris)
	"MTWK", // Item Monitor Glass Twinkle

	"SLPT", // Wavedash indicator
	"TRBS", // Trickdash indicator

	"IWHP", // Instawhip
	"WPRE", // Instawhip Recharge
	"WPRJ", // Instawhip Reject
	"GRNG", // Guard ring
	"GBDY", // Guard body

	"BAIL", // Bail charge
	"BAIB", // Bail after effect
	"BAIC", // Bail sparkle
	"TECH", // Bail tech charge

	"TRC1", // Charge aura
	"TRC2", // Charge fall
	"TRC3", // Charge flicker/sparks
	"TRC4", // Charge release
	"TRC5", // Charge extra

	"DHND", // Servant Hand

	"HORN", // Horncode

	"WIPD", // Wipeout dust trail
	"DRIF", // Drift Sparks
	"BDRF", // Brake drift sparks
	"BRAK", // Brake dust
	"DRWS", // Drift dust sparks
	"DREL", // Drift electricity
	"DRES", // Drift electric sparks
	"JANK", // Stair janking sparks
	"HFX1", // Hitlag stage 1
	"HFX2", // Hitlag stage 2
	"HFX3", // Hitlag stage 3
	"HFX4", // Hitlag stage 4
	"HFX5", // Hitlag stage 5
	"HFX6", // Hitlag stage 6
	"HFX8", // Hitlag stage 8
	"HFX9", // Hitlag stage 9
	"HFXX", // Hitlag stage 10

	// Kart Items
	"RSHE", // Rocket sneaker
	"FITM", // Eggman Monitor
	"BANA", // Banana Peel
	"BAND", // Banana Peel death particles
	"ORBN", // Orbinaut
	"JAWZ", // Jawz
	"SSMN", // SS Mine
	"KRBM", // SS Mine BOOM
	"LNDM", // Land Mine
	"DTRG", // Drop Target
	"BHOG", // Ballhog
	"BHBM", // Ballhog BOOM
	"BHGR", // Ballhog reticule
	"SPBM", // Self-Propelled Bomb
	"TRIS", // SPB Manta Ring start
	"TRNQ", // SPB Manta Ring loop
	"THNS", // Lightning Shield
	"THNC", // Lightning Shield Top Flash
	"THNA", // Lightning Shield Top Swoosh
	"THNB", // Lightning Shield Bottom Swoosh
	"THND", // Lightning attack
	"THNE", // Lightning attack
	"THNH", // Lightning attack
	"THNF", // Lightning attack
	"THNG", // Lightning attack
	"BUBS", // Bubble Shield (not Bubs)
	"BUBT", // Bubble Shield trap
	"BUBA", // Bubble Shield Outline
	"BUBB", // Bubble Shield Top Wave
	"BUBC", // Bubble Shield Bottom Wave
	"BUBD", // Bubble Shield Reflection
	"BUBE", // Bubble Shield Underline
	"BUBG", // Bubble Shield drag
	"BWVE", // Bubble Shield waves
	"FLMS", // Flame Shield
	"FLMA", // Flame Shield Top Layer
	"FLMB", // Flame Shield Bottom Layer
	"FLMD", // Flame Shield dash
	"FLMP", // Flame Shield paper sprites
	"FLML", // Flame Shield speed lines
	"FLMF", // Flame Shield flash
	"GTOP", // Marble Garden Zone Spinning Top
	"GTAR", // Garden Top Arrow
	"HYUU", // Hyudoro
	"GRWP", // Grow
	"POHB", // Shrink Poh-Bee
	"POHC", // Shrink Poh-Bee chain
	"SHRG", // Shrink gun
	"SHRL", // Shrink laser
	"SINK", // Kitchen Sink
	"SITR", // Kitchen Sink Trail
	"KBLN", // Battle Mode Bumper
	"BEXC", // Battle Bumper Explosion: Crystal
	"BEXS", // Battle Bumper Explosion: Shell
	"BDEB", // Battle Bumper Explosion: Debris
	"BEXB", // Battle Bumper Explosion: Blast
	"TWBS", // Tripwire Boost
	"TWBT", // Tripwire BLASTER
	"TWBP", // Tripwire approach
	"SMLD", // Smooth landing

	// Trick Effects
	"TRK1",
	"TRK2",
	"TRK3",
	"TRK4",
	"TRK5",
	"TRK6",
	"TRK7",

	"TIRG", // Tire grabbers
	"RSHT", // DEZ Ring Shooter

	"DEZL", // DEZ Laser respawn

	// Additional Kart Objects
	"AUDI", // Audience members
	"BUZB", // Sapphire Coast Buzz Mk3
	"SACO", // Sapphire Coast Fauna
	"BRNG", // Chaotix Big Ring

	// Ark Arrows
	"SYM0",
	"SYM1",
	"SYM2",
	"SYM3",
	"SYM4",
	"SYM5",
	"SYM6",
	"SYM7",
	"SYM8",
	"SYM9",
	"SYMA",
	"SYMB",
	"SYMC",
	"SYMD",
	"SYME",
	"SYMF",
	"SYMG",
	"SYMH",
	"SYMI",
	"SYMJ",
	"SYMK",
	"SYML",
	"SYMM",
	"SYMN",
	"SYMO",
	"SYMP",
	"SYMQ",
	"SYMR",
	"SYMS",
	"SYMT",
	"SYMU",
	"SYMV",
	"SYMW",
	"SYMX",
	"SYMY",
	"SYMZ",
	"ARK0",
	"ARK1",
	"ARK2",
	"ARK3",
	"ARK4",
	"ARK5",

	"BUMP", // Player/shell bump
	"FLEN", // Shell hit graphics stuff
	"CLAS", // items clash
	"PSHW", // thrown indicator
	"ISTA", // instashield layer A
	"ISTB", // instashield layer B

	"PWCL", // Invinc/grow clash VFX
	"GBRK", // Guard break

	"ITEM",
	"ITMO",
	"ITMI",
	"IBON",
	"ITMN",
	"PWRB",
	"RBOW", // power-up aura

	"PBOM", // player bomb

	"HIT1", // battle points
	"HIT2", // battle points
	"HIT3", // battle points

	"RETI", // player reticule

	"AIDU",

	"KSPK", // Spark radius for the lightning shield
	"LZI1", // Lightning that falls on the player for lightning shield
	"LZI2", // ditto
	"KLIT", // You have a twisted mind. But this actually is for the diagonal lightning.

	"FZSM", // F-Zero NO CONTEST explosion
	"FZBM",

	// Dash Rings
	"RAIR",

	// Adventure Air Booster
	"ADVR",
	"ADVE",

	// Sneaker Panels
	"BSTP",
	"BSTS",
	"BSTT",

	"MARB", // Marble Zone sprites
	"FUFO", // CD Special Stage UFO (don't ask me why it begins with an F)

	"RUST", // Rusty Rig sprites

	// Ports of gardens
	"PGTR",

	// Egg Zeppelin
	"PPLR",

	// Desert Palace
	"DPPT",

	// Aurora Atoll
	"AATR",
	"COCO",

	// Barren Badlands
	"BDST",
	"FROG",
	"CBRA",
	"HOLE",
	"BBRA",

	// Eerie Grove
	"EGFG",

	// Chaos Chute
	"SARC",
	"SSBM",

	// Hanagumi Hall
	"HGSP",
	"HGC0",
	"HGCA",
	"HGCB",
	"HGCC",
	"HGCD",
	"HGCE",
	"HGCF",
	"HGCG",

	// Dimension Disaster
	"DVDD",
	"SPRC",

	"TUST",
	"TULE",

	"FWRK",
	"MXCL",
	"RGSP",
	"LENS",
	"DRAF",
	"GRES",

	"OTBU",
	"OTLS",
	"OTCP",

	"DBOS", // Drift boost flame

	"WAYP",
	"EGOO",

	"AMPA",
	"AMPB",
	"AMPC",
	"AMPD",

	"EXPC",
	
	"TWBB",
	"TWOK",
	"TW_L",

	"SOR_",

	"WTRL", // Water Trail

	"GCHA", // follower: generic chao
	"CHEZ", // follower: cheese

	"DBCL", // Drift boost clip
	"DBNC", // Drift boost clip's sparks
	"DBST", // Drift boost plume

	"SDDS", // Spindash dust
	"SDWN", // Spindash wind
	"EBRK", // Soft Landing / Ebrake aura stuff.
	"HMTR", // Down Lines
	"HBUB", // HOLD! Bubble

	"TRCK",

	"FLBM",

	"UFOB",
	"UFOA",
	"UFOS",
	"SSCA",
	"SSCB",

	"UQMK",

	"GBOM",
	"GCHX",

	"3DFR",

	"BUFO", // Battle/Power-UP UFO

	"CPT1", // Checkpoint Orb
	"CPT2", // Checkpoint Stick
	"CPT3", // Checkpoint Base

	// rideroid (see info.h for detail)
	"RDRD",
	"RDRA",
	"RDRC",
	"RDRL",

	// leaf storm egg ball.
	"LSZB",

	// Dead Line Zone
	"DLZH",
	"DLZR",
	"DLZS",
	"DLZA",

	// Water Palace Zone
	"WPWL",	// turbine
	"WPZF",	// fountain
	"WPZK",	// klagen

	"SA2S", // SA2-style Ball Switch

	"STRG", // Spiked Target

	"BLEA", // m'A'in unit
	"BLEB", // o'B'server
	"BLEC", // 'C'lear glass
	"BLED", // shiel'D'
	"BLEE", // 'E'ggbeater
	"BLEF", // 'F'lamejet
	"BLEG", // 'G'enerator

	// Puyo hazards
	"PUYA",
	"PUYB",
	"PUYC",
	"PUYD",
	"PUYE",

	// Aerial Highlands
	"BCLD",

	// Avant Garden
	"AGTU",
	"AGTL",
	"AGTS",
	"AGTR",
	"AGFL",
	"AGFF",
	"AGCL",

	// Sky Sanctuary
	"SSCL",

	"MGSH", // Mega Barrier

	// GPZ Seasaw
	"GPPS",
	"GPZS",

	// Gust Planet Trees
	"GPTB",
	"GPTM",
	"GPTS",

	"GGZ1",
	"GGZ2",
	"GGZ3",
	"GGZ6",
	"GGZ7",
	"GGZ8",
	"FBTN",
	"SFTR",

	"SABX",
	"ICBL",

	"BSSP",
	"BSPB",
	"BSPR",
	"BSSR",
	"BLMS",
	"BLMM",
	"BLML",
	"BSWL",
	"BSWC",

	"LCLA",

	"AIZ1",
	"AIZ2",
	"AIZ3",
	"AIZ4",
	"AIZ5",
	"AIZ6",
	"AZR1",
	"AZR2",

	"EMR1",
	"EMR2",
	"EMR3",
	"EMFC",

	// Joypolis Trick Balloons
	"TKBR",
	"TKBY",

	// Waterfall particles
	"WTRP",

	// Sealed Stars
	"SCND",
	"SCNF",
	"SSBI",
	"SSCR",
	"SSFI",
	"SSSQ",
	"SSCO",
	"SGOB",
	"SSLA",
	"SWIN",
	"SWIS",
	"SBMP",
	"SSCH",
	"GCTA",
	"SENB",
	"SENC",
	"SEAS",
	"S_SP",

	// Tutorial
	"TLKP", // Talk Point

	// Destroyed Kart
	"DIEA", // tire
	"DIEB", // pipeframe bar
	"DIEC", // pedal tip
	"DIED", // right pedal
	"DIEE", // steering wheel
	"DIEF", // kart
	"DIEG", // left pedal
	"DIEH", // strut
	"DIEI", // wheel axle bar
	"DIEJ", // screw
	"DIEK", // electric engine
	"DIEL", // fire
	"DIEM", // smoke
	"DIEN", // explosion

	// Flybot767 (stun)
	"STUN",

	"STON",
	"TOXA",
	"TOXB",

	"GEAR",

	"MHPL",

	// Pulley
	"HCCH",
	"HCHK",

	// First person view sprites; this is a sprite so that it can be replaced by a specialized MD2 draw later
	"VIEW",
};

char spr2names[NUMPLAYERSPRITES][5] =
{
	"STIN", "STIL", "STIR", // Still
	"STGL", "STGR", // Still (glance back)
	"STLL", "STLR", // Still (look back)

	"SLWN", "SLWL", "SLWR", // Slow driving
	"SLGL", "SLGR", // Slow (glance back)
	"SLLL", "SLLR", // Slow (look back)

	"FSTN", "FSTL", "FSTR", // Fast driving
	"FSGL", "FSGR", // Fast (glance back)
	"FSLL", "FSLR", // Fast (look back)

	"DRLN", "DRLO", "DRLI", // Drifting left
	"DRRN", "DRRO", "DRRI", // Drifting right

	"SPIN", // Spinout
	"DEAD", // Dead

	"SIGN", "SIGL", "SSIG", // Finish signpost
	"XTRA", // Three Faces of Darkness
	"TALK", // Dialogue
	"DKRA", // Kart husk particle (A)
	"DKRB", // Kart husk particle (B)
	"DKRC", // Kart husk particle (C)
	"DKRD", // Kart husk particle (D)
	"DKRE", // Kart husk particle (E)
	"DKRF", // Kart husk particle (F) AKA The kart husk itself
	"DKRG", // Kart husk particle (G)
	"DKRH", // Kart husk particle (H)
	"DKRI", // Kart husk particle (I)
	"DKRJ", // Kart husk particle (J)
	"DKRK", // Kart husk particle (K)
};
playersprite_t free_spr2 = SPR2_FIRSTFREESLOT;

playersprite_t spr2defaults[NUMPLAYERSPRITES] = {
	0, // SPR2_STIN
	SPR2_STIN, // SPR2_STIL
	SPR2_STIN, // SPR2_STIR
	SPR2_STIN, // SPR2_STGL
	SPR2_STIN, // SPR2_STGR
	SPR2_STGL, // SPR2_STLL
	SPR2_STGR, // SPR2_STLR

	0, // SPR2_SLWN
	SPR2_SLWN, // SPR2_SLWL
	SPR2_SLWN, // SPR2_SLWR
	SPR2_SLWN, // SPR2_SLGL
	SPR2_SLWN, // SPR2_SLGR
	SPR2_SLGL, // SPR2_SLLL
	SPR2_SLGR, // SPR2_SLLR

	0, // SPR2_FSTN
	SPR2_FSTN, // SPR2_FSTL
	SPR2_FSTN, // SPR2_FSTR
	SPR2_FSTN, // SPR2_FSGL
	SPR2_FSTN, // SPR2_FSGR
	SPR2_FSGL, // SPR2_FSLL
	SPR2_FSGR, // SPR2_FSLR

	0, // SPR2_DRLN
	SPR2_DRLN, // SPR2_DRLO
	SPR2_DRLN, // SPR2_DRLI

	0, // SPR2_DRRN
	SPR2_DRRN, // SPR2_DRRO
	SPR2_DRRN, // SPR2_DRRI

	0, // SPR2_SPIN
	0, // SPR2_DEAD

	0, // SPR2_SIGN
	SPR2_SIGN, // SPR2_SIGL
	SPR2_SIGN, // SPR2_SSIG
	0, // SPR2_XTRA
	0, // SPR2_TALK
	0, // SPR2_DKRA
	0, // SPR2_DKRB
	0, // SPR2_DKRC
	0, // SPR2_DKRD
	0, // SPR2_DKRE
	0, // SPR2_DKRF
	0, // SPR2_DKRG
	0, // SPR2_DKRH
	0, // SPR2_DKRI
	0, // SPR2_DKRJ
	0, // SPR2_DKRK
};

// Doesn't work with g++, needs actionf_p1 (don't modify this comment)
state_t states[NUMSTATES] =
{
	// frame is masked through FF_FRAMEMASK
	// FF_ANIMATE makes simple state animations (var1 #frames, var2 tic delay) (var1 is ignored in P_SetupStateAnimation() if sprite is SPR_PLAY)
	// FF_FULLBRIGHT activates the fullbright colormap
	// use FF_TRANS10 - FF_TRANS90 for easy translucency
	// (or tr_trans10<<FF_TRANSSHIFT if you want to make it hard on yourself)

	// Keep this comment directly above S_NULL.
	{SPR_NULL, 0,  1, {NULL}, 0, 0, S_NULL}, // S_NULL
	{SPR_UNKN, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_UNKNOWN
	{SPR_NULL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_INVISIBLE

	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 0, 0, S_NULL}, // S_SPAWNSTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 1, 0, S_NULL}, // S_SEESTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 2, 0, S_NULL}, // S_MELEESTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 3, 0, S_NULL}, // S_MISSILESTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 4, 0, S_NULL}, // S_DEATHSTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 5, 0, S_NULL}, // S_XDEATHSTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 6, 0, S_NULL}, // S_RAISESTATE

	{SPR_THOK, FF_TRANS50, 8, {NULL}, 0, 0, S_NULL}, // S_THOK
	{SPR_NONE, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SHADOW

	// Player
	{SPR_PLAY, SPR2_STIN,					  1, {NULL}, 0, 0, S_KART_STILL},				// S_KART_STILL
	{SPR_PLAY, SPR2_STIL,					  1, {NULL}, 0, 0, S_KART_STILL_L},				// S_KART_STILL_L
	{SPR_PLAY, SPR2_STIR,					  1, {NULL}, 0, 0, S_KART_STILL_R},				// S_KART_STILL_R
	{SPR_PLAY, SPR2_STGL,					  1, {NULL}, 0, 0, S_KART_STILL_GLANCE_L},		// S_KART_STILL_GLANCE_L
	{SPR_PLAY, SPR2_STGR,					  1, {NULL}, 0, 0, S_KART_STILL_GLANCE_R},		// S_KART_STILL_GLANCE_R
	{SPR_PLAY, SPR2_STLL,					  1, {NULL}, 0, 0, S_KART_STILL_LOOK_L},		// S_KART_STILL_LOOK_L
	{SPR_PLAY, SPR2_STLR,					  1, {NULL}, 0, 0, S_KART_STILL_LOOK_R},		// S_KART_STILL_LOOK_R
	{SPR_PLAY, SPR2_SLWN,					  1, {NULL}, 0, 0, S_KART_SLOW},				// S_KART_SLOW
	{SPR_PLAY, SPR2_SLWL,					  1, {NULL}, 0, 0, S_KART_SLOW_L},				// S_KART_SLOW_L
	{SPR_PLAY, SPR2_SLWR,					  1, {NULL}, 0, 0, S_KART_SLOW_R},				// S_KART_SLOW_R
	{SPR_PLAY, SPR2_SLGL,					  1, {NULL}, 0, 0, S_KART_SLOW_GLANCE_L},		// S_KART_SLOW_GLANCE_L
	{SPR_PLAY, SPR2_SLGR,					  1, {NULL}, 0, 0, S_KART_SLOW_GLANCE_R},		// S_KART_SLOW_GLANCE_R
	{SPR_PLAY, SPR2_SLLL,					  1, {NULL}, 0, 0, S_KART_SLOW_LOOK_L},			// S_KART_SLOW_LOOK_L
	{SPR_PLAY, SPR2_SLLR,					  1, {NULL}, 0, 0, S_KART_SLOW_LOOK_R},			// S_KART_SLOW_LOOK_R
	{SPR_PLAY, SPR2_FSTN,					  1, {NULL}, 0, 0, S_KART_FAST},				// S_KART_FAST
	{SPR_PLAY, SPR2_FSTL,					  1, {NULL}, 0, 0, S_KART_FAST_L},				// S_KART_FAST_L
	{SPR_PLAY, SPR2_FSTR,					  1, {NULL}, 0, 0, S_KART_FAST_R},				// S_KART_FAST_R
	{SPR_PLAY, SPR2_FSGL,					  1, {NULL}, 0, 0, S_KART_FAST_GLANCE_L},		// S_KART_FAST_GLANCE_L
	{SPR_PLAY, SPR2_FSGR,					  1, {NULL}, 0, 0, S_KART_FAST_GLANCE_R},		// S_KART_FAST_GLANCE_R
	{SPR_PLAY, SPR2_FSLL,					  1, {NULL}, 0, 0, S_KART_FAST_LOOK_L},			// S_KART_FAST_LOOK_L
	{SPR_PLAY, SPR2_FSLR,					  1, {NULL}, 0, 0, S_KART_FAST_LOOK_R},			// S_KART_FAST_LOOK_R
	{SPR_PLAY, SPR2_DRLN,					  1, {NULL}, 0, 0, S_KART_DRIFT_L},				// S_KART_DRIFT_L
	{SPR_PLAY, SPR2_DRLO,					  1, {NULL}, 0, 0, S_KART_DRIFT_L_OUT},			// S_KART_DRIFT_L_OUT
	{SPR_PLAY, SPR2_DRLI,					  1, {NULL}, 0, 0, S_KART_DRIFT_L_IN},			// S_KART_DRIFT_L_IN
	{SPR_PLAY, SPR2_DRRN,					  1, {NULL}, 0, 0, S_KART_DRIFT_R},				// S_KART_DRIFT_R
	{SPR_PLAY, SPR2_DRRO,					  1, {NULL}, 0, 0, S_KART_DRIFT_R_OUT},			// S_KART_DRIFT_R_OUT
	{SPR_PLAY, SPR2_DRRI,					  1, {NULL}, 0, 0, S_KART_DRIFT_R_IN},			// S_KART_DRIFT_R_IN
	{SPR_PLAY, SPR2_SPIN|FF_ANIMATE,		350, {NULL}, 0, 1, S_KART_STILL},				// S_KART_SPINOUT
	{SPR_PLAY, SPR2_DEAD|FF_SEMIBRIGHT,		  3, {NULL}, 0, 0, S_KART_DEAD},				// S_KART_DEAD
	{SPR_PLAY, SPR2_SIGN|FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 0, 1, 0},					// S_KART_SIGN
	{SPR_PLAY, SPR2_SIGL|FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 0, 1, 0},					// S_KART_SIGL

	{SPR_NULL, 0, -1, {NULL}, 0, 0, S_OBJPLACE_DUMMY}, // S_OBJPLACE_DUMMY

	{SPR_KART, 0, -1, {NULL}, 0, 0, S_NULL}, // S_KART_LEFTOVER
	{SPR_DIEF, 0, -1, {NULL}, 0, 0, S_NULL}, // S_KART_LEFTOVER_NOTIRES
	{SPR_PLAY, SPR2_DKRA, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_A}, // S_KART_LEFTOVER_PARTICLE_CUSTOM_A
	{SPR_PLAY, SPR2_DKRB, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_B}, // S_KART_LEFTOVER_PARTICLE_CUSTOM_B
	{SPR_PLAY, SPR2_DKRC, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_C}, // S_KART_LEFTOVER_PARTICLE_CUSTOM_C
	{SPR_PLAY, SPR2_DKRD, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_D}, // S_KART_LEFTOVER_PARTICLE_CUSTOM_D
	{SPR_PLAY, SPR2_DKRE, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_E}, // S_KART_LEFTOVER_PARTICLE_CUSTOM_E
	{SPR_PLAY, SPR2_DKRF, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_F},// S_KART_LEFTOVER_PARTICLE_CUSTOM_F
	{SPR_PLAY, SPR2_DKRG, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_G}, // S_KART_LEFTOVER_PARTICLE_CUSTOM_G
	{SPR_PLAY, SPR2_DKRH, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_H}, // S_KART_LEFTOVER_PARTICLE_CUSTOM_H
	{SPR_PLAY, SPR2_DKRI, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_I}, // S_KART_LEFTOVER_PARTICLE_CUSTOM_I
	{SPR_PLAY, SPR2_DKRJ, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_J}, // S_KART_LEFTOVER_PARTICLE_CUSTOM_J
	{SPR_PLAY, SPR2_DKRK, 3, {NULL}, 0, 0, S_KART_LEFTOVER_PARTICLE_CUSTOM_K}, // S_KART_LEFTOVER_PARTICLE_CUSTOM_K

	{SPR_TIRE, 0, -1, {NULL}, 0, 0, S_NULL}, // S_KART_TIRE1
	{SPR_TIRE, 1, -1, {NULL}, 0, 0, S_NULL}, // S_KART_TIRE2

	{SPR_DIEL, 0|FF_ANIMATE, 12, {NULL}, 11, 1, S_NULL}, // S_KART_FIRE
	{SPR_DIEM, FF_SEMIBRIGHT|FF_ANIMATE|FF_TRANS30, 30, {NULL}, 9, 3, S_NULL}, // S_KART_SMOKE

	{SPR_DIEN, 0|FF_PAPERSPRITE|FF_ADD, 3, {NULL}, 0, 0, S_KART_XPL02}, // S_KART_XPL01
	{SPR_DIEN, 1|FF_PAPERSPRITE|FF_ADD|FF_ANIMATE, 4, {NULL}, 1, 2, S_KART_XPL03}, // S_KART_XPL02
	{SPR_DIEN, 3|FF_PAPERSPRITE|FF_ADD|FF_ANIMATE, 10, {NULL}, 4, 2, S_NULL}, // S_KART_XPL03

	// Boss Explosion
	{SPR_BOM2, FF_FULLBRIGHT|FF_ANIMATE, (5*7), {NULL}, 6, 5, S_NULL}, // S_BOSSEXPLODE

	// S3&K Boss Explosion
	{SPR_BOM3, FF_FULLBRIGHT,   1, {NULL}, 0, 0, S_SONIC3KBOSSEXPLOSION2}, // S_SONIC3KBOSSEXPLOSION1
	{SPR_BOM3, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_SONIC3KBOSSEXPLOSION3}, // S_SONIC3KBOSSEXPLOSION2
	{SPR_BOM3, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_SONIC3KBOSSEXPLOSION4}, // S_SONIC3KBOSSEXPLOSION3
	{SPR_BOM3, FF_FULLBRIGHT|3, 2, {NULL}, 0, 0, S_SONIC3KBOSSEXPLOSION5}, // S_SONIC3KBOSSEXPLOSION4
	{SPR_BOM3, FF_FULLBRIGHT|4, 3, {NULL}, 0, 0, S_SONIC3KBOSSEXPLOSION6}, // S_SONIC3KBOSSEXPLOSION5
	{SPR_BOM3, FF_FULLBRIGHT|5, 4, {NULL}, 0, 0, S_NULL}, // S_SONIC3KBOSSEXPLOSION6

	// Ring
	{SPR_RING, FF_SEMIBRIGHT|FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 23, 1, S_RING}, // S_RING
	{SPR_RING,  0, 1, {NULL}, 0, 0, S_FASTRING2}, // S_FASTRING1
	{SPR_RING,  2, 1, {NULL}, 0, 0, S_FASTRING3}, // S_FASTRING2
	{SPR_RING,  4, 1, {NULL}, 0, 0, S_FASTRING4}, // S_FASTRING3
	{SPR_RING,  6, 1, {NULL}, 0, 0, S_FASTRING5}, // S_FASTRING4
	{SPR_RING,  8, 1, {NULL}, 0, 0, S_FASTRING6}, // S_FASTRING5
	{SPR_RING, 10, 1, {NULL}, 0, 0, S_FASTRING7}, // S_FASTRING6
	{SPR_RING, 12, 1, {NULL}, 0, 0, S_FASTRING8}, // S_FASTRING7
	{SPR_RING, 14, 1, {NULL}, 0, 0, S_FASTRING9}, // S_FASTRING8
	{SPR_RING, 16, 1, {NULL}, 0, 0, S_FASTRING10}, // S_FASTRING9
	{SPR_RING, 18, 1, {NULL}, 0, 0, S_FASTRING11}, // S_FASTRING10
	{SPR_RING, 20, 1, {NULL}, 0, 0, S_FASTRING12}, // S_FASTRING11
	{SPR_RING, 22, 1, {NULL}, 0, 0, S_FASTRING1}, // S_FASTRING12

	// Blue Sphere
	{SPR_BSPH, FF_SEMIBRIGHT|2, TICRATE, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE1}, // S_BLUESPHERE
	{SPR_BSPH, FF_SEMIBRIGHT|2, TICRATE, {A_SetRandomTics}, 1, TICRATE, S_BLUESPHERE_BOUNCE1}, // S_BLUESPHERE_SPAWN

	{SPR_BSPH, FF_SEMIBRIGHT, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE2}, // S_BLUESPHERE_BOUNCE1
	{SPR_BSPH, FF_SEMIBRIGHT|4, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE3}, // S_BLUESPHERE_BOUNCE2

	{SPR_BSPH, FF_SEMIBRIGHT, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE4}, // S_BLUESPHERE_BOUNCE3
	{SPR_BSPH, FF_SEMIBRIGHT|4, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE5}, // S_BLUESPHERE_BOUNCE4

	{SPR_BSPH, FF_SEMIBRIGHT, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE6}, // S_BLUESPHERE_BOUNCE5
	{SPR_BSPH, FF_SEMIBRIGHT|2, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE7}, // S_BLUESPHERE_BOUNCE6
	{SPR_BSPH, FF_SEMIBRIGHT|4, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE8}, // S_BLUESPHERE_BOUNCE7
	{SPR_BSPH, FF_SEMIBRIGHT|2, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE9}, // S_BLUESPHERE_BOUNCE8

	{SPR_BSPH, FF_SEMIBRIGHT, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE10}, // S_BLUESPHERE_BOUNCE9
	{SPR_BSPH, FF_SEMIBRIGHT|2, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE11}, // S_BLUESPHERE_BOUNCE10
	{SPR_BSPH, FF_SEMIBRIGHT|4, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE12}, // S_BLUESPHERE_BOUNCE11
	{SPR_BSPH, FF_SEMIBRIGHT|2, 1, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE13}, // S_BLUESPHERE_BOUNCE12

	{SPR_BSPH, FF_SEMIBRIGHT, 2, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE14}, // S_BLUESPHERE_BOUNCE13
	{SPR_BSPH, FF_SEMIBRIGHT|1, 2, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE15}, // S_BLUESPHERE_BOUNCE14
	{SPR_BSPH, FF_SEMIBRIGHT|2, 2, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE16}, // S_BLUESPHERE_BOUNCE15
	{SPR_BSPH, FF_SEMIBRIGHT|3, 2, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE17}, // S_BLUESPHERE_BOUNCE16
	{SPR_BSPH, FF_SEMIBRIGHT|4, 2, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE18}, // S_BLUESPHERE_BOUNCE17
	{SPR_BSPH, FF_SEMIBRIGHT|3, 4, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE19}, // S_BLUESPHERE_BOUNCE18
	{SPR_BSPH, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE20}, // S_BLUESPHERE_BOUNCE19
	{SPR_BSPH, FF_SEMIBRIGHT|1, 4, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE21}, // S_BLUESPHERE_BOUNCE20

	{SPR_BSPH, FF_SEMIBRIGHT, 6, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE22}, // S_BLUESPHERE_BOUNCE21
	{SPR_BSPH, FF_SEMIBRIGHT|1, 6, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE23}, // S_BLUESPHERE_BOUNCE22
	{SPR_BSPH, FF_SEMIBRIGHT|2, 6, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE24}, // S_BLUESPHERE_BOUNCE23
	{SPR_BSPH, FF_SEMIBRIGHT|3, 9, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE25}, // S_BLUESPHERE_BOUNCE24
	{SPR_BSPH, FF_SEMIBRIGHT|4, 9, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE26}, // S_BLUESPHERE_BOUNCE25
	{SPR_BSPH, FF_SEMIBRIGHT|3, 9, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE27}, // S_BLUESPHERE_BOUNCE26
	{SPR_BSPH, FF_SEMIBRIGHT|2, 9, {NULL}, 0, 0, S_BLUESPHERE_BOUNCE28}, // S_BLUESPHERE_BOUNCE27
	{SPR_BSPH, FF_SEMIBRIGHT|1, 9, {NULL}, 0, 0, S_BLUESPHERE}, // S_BLUESPHERE_BOUNCE28

	// Emblem
	{SPR_EMBM,  0, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM1
	{SPR_EMBM,  1, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM2
	{SPR_EMBM,  2, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM3
	{SPR_EMBM,  3, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM4
	{SPR_EMBM,  4, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM5
	{SPR_EMBM,  5, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM6
	{SPR_EMBM,  6, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM7
	{SPR_EMBM,  7, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM8
	{SPR_EMBM,  8, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM9
	{SPR_EMBM,  9, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM10
	{SPR_EMBM, 10, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM11
	{SPR_EMBM, 11, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM12
	{SPR_EMBM, 12, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM13
	{SPR_EMBM, 13, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM14
	{SPR_EMBM, 14, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM15
	{SPR_EMBM, 15, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM16
	{SPR_EMBM, 16, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM17
	{SPR_EMBM, 17, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM18
	{SPR_EMBM, 18, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM19
	{SPR_EMBM, 19, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM20
	{SPR_EMBM, 20, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM21
	{SPR_EMBM, 21, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM22
	{SPR_EMBM, 22, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM23
	{SPR_EMBM, 23, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM24
	{SPR_EMBM, 24, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM25
	{SPR_EMBM, 25, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM26

	// Spray Can
	{SPR_SPCN, FF_ANIMATE|FF_SEMIBRIGHT, -1, {NULL}, 15, 2, S_NULL}, // S_SPRAYCAN

	// Ancient Shrine
	{SPR_MMSH, 0, -1, {NULL}, 0, 0, S_NULL}, // S_ANCIENTSHRINE

	{SPR_MORB, 0|FF_ADD, 1, {A_FireShrink}, 2*FRACUNIT/3, 12, S_MORB2}, // S_MORB1
	{SPR_MORB, 1|FF_ADD, 1, {NULL}, 0, 0, S_MORB3},  // S_MORB2
	{SPR_MORB, 2|FF_ADD, 1, {NULL}, 0, 0, S_MORB4},  // S_MORB3
	{SPR_MORB, 3|FF_ADD, 1, {NULL}, 0, 0, S_MORB5},  // S_MORB4
	{SPR_MORB, 4|FF_ADD, 1, {NULL}, 0, 0, S_MORB6},  // S_MORB5
	{SPR_MORB, 5|FF_ADD, 1, {NULL}, 0, 0, S_MORB7},  // S_MORB6
	{SPR_MORB, 6|FF_ADD, 1, {NULL}, 0, 0, S_MORB8},  // S_MORB7
	{SPR_MORB, 7|FF_ADD, 4, {NULL}, 0, 0, S_MORB9},  // S_MORB8
	{SPR_MORB, 6|FF_ADD, 1, {A_FireShrink},            1, 12, S_MORB10}, // S_MORB9
	{SPR_MORB, 5|FF_ADD, 1, {NULL}, 0, 0, S_MORB11}, // S_MORB10
	{SPR_MORB, 4|FF_ADD, 1, {NULL}, 0, 0, S_MORB12}, // S_MORB11
	{SPR_MORB, 3|FF_ADD, 1, {NULL}, 0, 0, S_MORB13}, // S_MORB12
	{SPR_MORB, 2|FF_ADD, 1, {NULL}, 0, 0, S_MORB14}, // S_MORB13
	{SPR_MORB, 1|FF_ADD, 1, {NULL}, 0, 0, S_MORB15}, // S_MORB14
	{SPR_MORB, 0|FF_ADD, 1, {NULL}, 0, 0, S_NULL},   // S_MORB15

	// Chaos Emeralds
	{SPR_EMRC, FF_FULLBRIGHT,           1, {NULL}, 0, 0, S_CHAOSEMERALD2}, // S_CHAOSEMERALD1
	{SPR_EMRC, FF_FULLBRIGHT|FF_ADD,    1, {NULL}, 0, 0, S_CHAOSEMERALD1}, // S_CHAOSEMERALD2
	{SPR_EMRC, FF_FULLBRIGHT|1, -1, {NULL}, 1, 0, S_NULL}, // S_CHAOSEMERALD_UNDER

	// Super Emeralds
	{SPR_SEMR, FF_FULLBRIGHT,           1, {NULL}, 0, 0, S_SUPEREMERALD2}, // S_SUPEREMERALD1
	{SPR_SEMR, FF_FULLBRIGHT|FF_ADD,    1, {NULL}, 0, 0, S_SUPEREMERALD1}, // S_SUPEREMERALD2
	{SPR_SEMR, FF_FULLBRIGHT|1, -1, {NULL}, 1, 0, S_NULL}, // S_SUPEREMERALD_UNDER

	{SPR_ESPK, FF_FULLBRIGHT,   3, {NULL}, 0, 0, S_EMERALDSPARK2}, // S_EMERALDSPARK1
	{SPR_ESPK, FF_FULLBRIGHT|1, 3, {NULL}, 0, 0, S_EMERALDSPARK3}, // S_EMERALDSPARK2
	{SPR_ESPK, FF_FULLBRIGHT|2, 3, {NULL}, 0, 0, S_EMERALDSPARK4}, // S_EMERALDSPARK3
	{SPR_ESPK, FF_FULLBRIGHT|3, 3, {NULL}, 0, 0, S_EMERALDSPARK5}, // S_EMERALDSPARK4
	{SPR_ESPK, FF_FULLBRIGHT|4, 3, {NULL}, 0, 0, S_EMERALDSPARK6}, // S_EMERALDSPARK5
	{SPR_ESPK, FF_FULLBRIGHT|5, 3, {NULL}, 0, 0, S_EMERALDSPARK7}, // S_EMERALDSPARK6
	{SPR_ESPK, FF_FULLBRIGHT|6, 3, {NULL}, 0, 0, S_NULL}, // S_EMERALDSPARK7

	{SPR_LENS, FF_FULLBRIGHT|FF_ADD|FF_TRANS10|FF_ANIMATE|11, 8, {NULL}, 7, 1, S_GAINAX_MID2}, // S_EMERALDFLARE1

	// Prison Egg Drops
	{SPR_ALTM, 0|FF_PAPERSPRITE|FF_SEMIBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_PRISONEGGDROP_CD
	{SPR_LENS, 14|FF_FULLBRIGHT|FF_ADD|FF_TRANS10, 1, {NULL}, 0, 0, S_PRISONEGGDROP_FLAREA2}, // S_PRISONEGGDROP_FLAREA1
	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_PRISONEGGDROP_FLAREA1}, // S_PRISONEGGDROP_FLAREA2
	{SPR_LENS, 11|FF_FULLBRIGHT|FF_ADD|FF_TRANS10|FF_ANIMATE, 16, {NULL}, 7, 2, S_PRISONEGGDROP_FLAREB2}, // S_PRISONEGGDROP_FLAREB1
	{SPR_LENS, 19|FF_FULLBRIGHT|FF_ADD|FF_TRANS10|FF_ANIMATE, 6, {NULL}, 1, 2, S_NULL}, // S_PRISONEGGDROP_FLAREB2

	// Bubble Source
	{SPR_BBLS, 0, 8, {A_BubbleSpawn}, 2048, 0, S_BUBBLES2}, // S_BUBBLES1
	{SPR_BBLS, 1, 8, {A_BubbleCheck}, 0, 0, S_BUBBLES3}, // S_BUBBLES2
	{SPR_BBLS, 2, 8, {A_BubbleSpawn}, 2048, 0, S_BUBBLES4}, // S_BUBBLES3
	{SPR_BBLS, 3, 8, {A_BubbleCheck}, 0, 0, S_BUBBLES1}, // S_BUBBLES4

	// Level End Sign
	{SPR_SIGN, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SIGN_POLE
	{SPR_SIGN, 1|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_SIGN_BACK
	{SPR_SIGN, 2|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_SIGN_SIDE
	{SPR_SIGN, 3|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_SIGN_FACE
	{SPR_SIGN, 19|FF_PAPERSPRITE, -1, {NULL},  0, 0, S_NULL}, // S_SIGN_DEFAULT

	// Spike Ball
	{SPR_SPIK, 0, 1, {NULL}, 0, 0, S_SPIKEBALL2}, // S_SPIKEBALL1
	{SPR_SPIK, 1, 1, {NULL}, 0, 0, S_SPIKEBALL3}, // S_SPIKEBALL2
	{SPR_SPIK, 2, 1, {NULL}, 0, 0, S_SPIKEBALL4}, // S_SPIKEBALL3
	{SPR_SPIK, 3, 1, {NULL}, 0, 0, S_SPIKEBALL5}, // S_SPIKEBALL4
	{SPR_SPIK, 4, 1, {NULL}, 0, 0, S_SPIKEBALL6}, // S_SPIKEBALL5
	{SPR_SPIK, 5, 1, {NULL}, 0, 0, S_SPIKEBALL7}, // S_SPIKEBALL6
	{SPR_SPIK, 6, 1, {NULL}, 0, 0, S_SPIKEBALL8}, // S_SPIKEBALL7
	{SPR_SPIK, 7, 1, {NULL}, 0, 0, S_SPIKEBALL1}, // S_SPIKEBALL8

	// Elemental Shield's Spawn
	{SPR_SFLM, FF_FULLBRIGHT,   2, {NULL}, 0, 0, S_SPINFIRE2}, // S_SPINFIRE1
	{SPR_SFLM, FF_FULLBRIGHT|1, 2, {NULL}, 0, 0, S_SPINFIRE3}, // S_SPINFIRE2
	{SPR_SFLM, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_SPINFIRE4}, // S_SPINFIRE3
	{SPR_SFLM, FF_FULLBRIGHT|3, 2, {NULL}, 0, 0, S_SPINFIRE5}, // S_SPINFIRE4
	{SPR_SFLM, FF_FULLBRIGHT|4, 2, {NULL}, 0, 0, S_SPINFIRE6}, // S_SPINFIRE5
	{SPR_SFLM, FF_FULLBRIGHT|5, 2, {NULL}, 0, 0, S_SPINFIRE1}, // S_SPINFIRE6

	// Floor Spike
	{SPR_USPK, FF_SEMIBRIGHT|0,-1, {A_SpikeRetract}, 1, 0, S_SPIKE2}, // S_SPIKE1 -- Fully extended
	{SPR_USPK, FF_SEMIBRIGHT|1, 2, {A_Pain},         0, 0, S_SPIKE3}, // S_SPIKE2
	{SPR_USPK, FF_SEMIBRIGHT|0, 2, {NULL},           0, 0, S_SPIKE4}, // S_SPIKE3
	{SPR_USPK, FF_SEMIBRIGHT|3,-1, {A_SpikeRetract}, 0, 0, S_SPIKE5}, // S_SPIKE4 -- Fully retracted
	{SPR_USPK, FF_SEMIBRIGHT|2, 2, {A_Pain},         0, 0, S_SPIKE6}, // S_SPIKE5
	{SPR_USPK, FF_SEMIBRIGHT|1, 2, {NULL},           0, 0, S_SPIKE1}, // S_SPIKE6
	{SPR_USPK, FF_SEMIBRIGHT|4,-1, {NULL}, 0, 0, S_NULL}, // S_SPIKED1 -- Busted spike particles
	{SPR_USPK, FF_SEMIBRIGHT|5,-1, {NULL}, 0, 0, S_NULL}, // S_SPIKED2

	// Wall Spike
	{SPR_WSPK, 0|FF_SEMIBRIGHT|FF_PAPERSPRITE,-1, {A_SpikeRetract}, 1, 0, S_WALLSPIKE2}, // S_WALLSPIKE1 -- Fully extended
	{SPR_WSPK, 1|FF_SEMIBRIGHT|FF_PAPERSPRITE, 2, {A_Pain},         0, 0, S_WALLSPIKE3}, // S_WALLSPIKE2
	{SPR_WSPK, 2|FF_SEMIBRIGHT|FF_PAPERSPRITE, 2, {NULL},           0, 0, S_WALLSPIKE4}, // S_WALLSPIKE3
	{SPR_WSPK, 3|FF_SEMIBRIGHT|FF_PAPERSPRITE,-1, {A_SpikeRetract}, 0, 0, S_WALLSPIKE5}, // S_WALLSPIKE4 -- Fully retracted
	{SPR_WSPK, 2|FF_SEMIBRIGHT|FF_PAPERSPRITE, 2, {A_Pain},         0, 0, S_WALLSPIKE6}, // S_WALLSPIKE5
	{SPR_WSPK, 1|FF_SEMIBRIGHT|FF_PAPERSPRITE, 2, {NULL},           0, 0, S_WALLSPIKE1}, // S_WALLSPIKE6
	{SPR_WSPB, 0|FF_SEMIBRIGHT|FF_PAPERSPRITE,-1, {NULL}, 0, 0, S_NULL}, // S_WALLSPIKEBASE -- Base
	{SPR_WSPK, 4,-1, {NULL}, 0, 0, S_NULL}, // S_WALLSPIKED1 -- Busted spike particles
	{SPR_WSPK, 5,-1, {NULL}, 0, 0, S_NULL}, // S_WALLSPIKED2

	// Cannon launcher
	{SPR_NULL, 0, 1,    {A_FindTarget},     MT_PLAYER,         0, S_CANNONLAUNCHER2}, // S_CANNONLAUNCHER1
	{SPR_NULL, 0, 1,       {A_LobShot}, MT_CANNONBALL, 4*TICRATE, S_CANNONLAUNCHER3}, // S_CANNONLAUNCHER2
	{SPR_NULL, 0, 2, {A_SetRandomTics},     TICRATE/2, 3*TICRATE, S_CANNONLAUNCHER1}, // S_CANNONLAUNCHER3

	{SPR_CBLL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CANNONBALL1

	// GFZ flowers
	{SPR_FWR1, FF_ANIMATE, -1, {NULL},  7, 3, S_NULL}, // S_GFZFLOWERA
	{SPR_FWR2, FF_ANIMATE, -1, {NULL}, 19, 3, S_NULL}, // S_GFZFLOWERB
	{SPR_FWR3, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_GFZFLOWERC

	{SPR_BUS3, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BLUEBERRYBUSH
	{SPR_BUS1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BERRYBUSH
	{SPR_BUS2, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BUSH

	// Trees
	{SPR_TRE1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_GFZTREE
	{SPR_TRE1, 1, -1, {NULL}, 0, 0, S_NULL}, // S_GFZBERRYTREE
	{SPR_TRE1, 2, -1, {NULL}, 0, 0, S_NULL}, // S_GFZCHERRYTREE
	{SPR_TRE2, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CHECKERTREE
	{SPR_TRE2, 1, -1, {NULL}, 0, 0, S_NULL}, // S_CHECKERSUNSETTREE
	{SPR_TRE3, 0, -1, {NULL}, 0, 0, S_NULL}, // S_FHZTREE
	{SPR_TRE3, 1, -1, {NULL}, 0, 0, S_NULL}, // S_FHZPINKTREE
	{SPR_TRE4, 0, -1, {NULL}, 0, 0, S_NULL}, // S_POLYGONTREE
	{SPR_TRE5, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BUSHTREE
	{SPR_TRE5, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BUSHREDTREE
	{SPR_TRE6, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SPRINGTREE

	// THZ flowers
	{SPR_THZP, FF_ANIMATE, -1, {NULL},  7, 4, S_NULL}, // S_THZFLOWERA
	{SPR_FWR5, FF_ANIMATE, -1, {NULL}, 19, 2, S_NULL}, // S_THZFLOWERB
	{SPR_FWR6, FF_ANIMATE, -1, {NULL}, 19, 2, S_NULL}, // S_THZFLOWERC

	// THZ Steam Whistle tree/bush
	{SPR_THZT, 0, -1, {NULL}, 0, 0, S_NULL}, // S_THZTREE
	{SPR_THZT,  1|FF_PAPERSPRITE, 40, {NULL}, 0, 0, S_THZTREEBRANCH2}, // S_THZTREEBRANCH1
	{SPR_THZT,  2|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH3}, // S_THZTREEBRANCH2
	{SPR_THZT,  3|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH4}, // S_THZTREEBRANCH3
	{SPR_THZT,  4|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH5}, // S_THZTREEBRANCH4
	{SPR_THZT,  5|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH6}, // S_THZTREEBRANCH5
	{SPR_THZT,  6|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH7}, // S_THZTREEBRANCH6
	{SPR_THZT,  7|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH8}, // S_THZTREEBRANCH7
	{SPR_THZT,  8|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH9}, // S_THZTREEBRANCH8
	{SPR_THZT,  9|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH10}, // S_THZTREEBRANCH9
	{SPR_THZT, 10|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH11}, // S_THZTREEBRANCH10
	{SPR_THZT, 11|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH12}, // S_THZTREEBRANCH11
	{SPR_THZT, 12|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH13}, // S_THZTREEBRANCH12
	{SPR_THZT, 13|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH1}, // S_THZTREEBRANCH13

	// THZ Alarm
	{SPR_ALRM, FF_FULLBRIGHT, 35, {A_Scream}, 0, 0, S_ALARM1}, // S_ALARM1

	// Deep Sea Gargoyle
	{SPR_GARG, 0, -1, {NULL}, 0, 0, S_NULL},  // S_GARGOYLE
	{SPR_GARG, 1, -1, {NULL}, 0, 0, S_NULL},  // S_BIGGARGOYLE

	// DSZ Seaweed
	{SPR_SEWE, 0, -1, {NULL}, 0, 0, S_SEAWEED2}, // S_SEAWEED1
	{SPR_SEWE, 1, 5, {NULL}, 0, 0, S_SEAWEED3}, // S_SEAWEED2
	{SPR_SEWE, 2, 5, {NULL}, 0, 0, S_SEAWEED4}, // S_SEAWEED3
	{SPR_SEWE, 3, 5, {NULL}, 0, 0, S_SEAWEED5}, // S_SEAWEED4
	{SPR_SEWE, 4, 5, {NULL}, 0, 0, S_SEAWEED6}, // S_SEAWEED5
	{SPR_SEWE, 5, 5, {NULL}, 0, 0, S_SEAWEED1}, // S_SEAWEED6

	// Dripping water
	{SPR_NULL, FF_TRANS30  , 3*TICRATE, {NULL},                  0, 0, S_DRIPA2}, // S_DRIPA1
	{SPR_DRIP, FF_TRANS30  ,         2, {NULL},                  0, 0, S_DRIPA3}, // S_DRIPA2
	{SPR_DRIP, FF_TRANS30|1,         2, {NULL},                  0, 0, S_DRIPA4}, // S_DRIPA3
	{SPR_DRIP, FF_TRANS30|2,         2, {A_SpawnObjectRelative}, 0, MT_WATERDROP, S_DRIPA1}, // S_DRIPA4
	{SPR_DRIP, FF_TRANS30|3,        -1, {NULL},                  0, 0, S_DRIPB1}, // S_DRIPB1
	{SPR_DRIP, FF_TRANS30|4,         1, {NULL},                  0, 0, S_DRIPC2}, // S_DRIPC1
	{SPR_DRIP, FF_TRANS30|5,         1, {NULL},                  0, 0,   S_NULL}, // S_DRIPC2

	// Coral
	{SPR_CORL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CORAL1
	{SPR_CORL, 1, -1, {NULL}, 0, 0, S_NULL}, // S_CORAL2
	{SPR_CORL, 2, -1, {NULL}, 0, 0, S_NULL}, // S_CORAL3
	{SPR_CORL, 3, -1, {NULL}, 0, 0, S_NULL}, // S_CORAL4
	{SPR_CORL, 4, -1, {NULL}, 0, 0, S_NULL}, // S_CORAL5

	// Blue Crystal
	{SPR_BCRY, 32768, -1, {NULL}, 0, 0, S_NULL}, // S_BLUECRYSTAL1

	// Kelp
	{SPR_KELP, 0, -1, {NULL}, 0, 0, S_NULL}, // S_KELP

	// Animated algae
	{SPR_ALGA, 0, 1, {A_ConnectToGround}, MT_ANIMALGAESEG, 0, S_ANIMALGAETOP2}, // S_ANIMALGAETOP1
	{SPR_ALGA, 0|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 11, 4, S_NULL},          // S_ANIMALGAETOP2
	{SPR_ALGB, 0|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 11, 4, S_NULL},          // S_ANIMALGAESEG

	// DSZ Stalagmites
	{SPR_DSTG, 0, -1, {NULL}, 0, 0, S_NULL}, // S_DSZSTALAGMITE
	{SPR_DSTG, 1, -1, {NULL}, 0, 0, S_NULL}, // S_DSZ2STALAGMITE

	// DSZ Light beam
	{SPR_LIBE, 0|FF_TRANS80|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {A_LightBeamReset}, 0, 0, S_LIGHTBEAM2}, // S_LIGHTBEAM1
	{SPR_LIBE, 0|FF_TRANS70|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {NULL}, 0, 0, S_LIGHTBEAM3},  // S_LIGHTBEAM2
	{SPR_LIBE, 0|FF_TRANS60|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {NULL}, 0, 0, S_LIGHTBEAM4},  // S_LIGHTBEAM3
	{SPR_LIBE, 0|FF_TRANS50|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {NULL}, 0, 0, S_LIGHTBEAM5},  // S_LIGHTBEAM4
	{SPR_LIBE, 0|FF_TRANS40|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {NULL}, 0, 0, S_LIGHTBEAM6},  // S_LIGHTBEAM5
	{SPR_LIBE, 0|FF_TRANS30|FF_FULLBRIGHT|FF_PAPERSPRITE, 9, {NULL}, 0, 0, S_LIGHTBEAM7},  // S_LIGHTBEAM6
	{SPR_LIBE, 0|FF_TRANS40|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {NULL}, 0, 0, S_LIGHTBEAM8},  // S_LIGHTBEAM7
	{SPR_LIBE, 0|FF_TRANS50|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {NULL}, 0, 0, S_LIGHTBEAM9},  // S_LIGHTBEAM8
	{SPR_LIBE, 0|FF_TRANS60|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {NULL}, 0, 0, S_LIGHTBEAM10}, // S_LIGHTBEAM9
	{SPR_LIBE, 0|FF_TRANS70|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {NULL}, 0, 0, S_LIGHTBEAM11}, // S_LIGHTBEAM10
	{SPR_LIBE, 0|FF_TRANS80|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {NULL}, 0, 0, S_LIGHTBEAM12}, // S_LIGHTBEAM11
	{SPR_NULL, 0, 2, {A_SetRandomTics}, 4, 35, S_LIGHTBEAM1}, // S_LIGHTBEAM12

	// CEZ Chain
	{SPR_CHAN, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CEZCHAIN

	// Flame
	{SPR_FLAM, FF_FULLBRIGHT|FF_ANIMATE|FF_ADD,       3*8, {A_FlameParticle}, 7, 3, S_FLAME}, // S_FLAME
	{SPR_FLAM, FF_FULLBRIGHT|FF_ANIMATE|FF_ADD|8, TICRATE,            {NULL}, 3, 3, S_NULL},  // S_FLAMEPARTICLE
	{SPR_FLAM, FF_FULLBRIGHT|FF_ANIMATE|FF_ADD,        -1,            {NULL}, 7, 3, S_NULL},  // S_FLAMEREST

	// Eggman statue
	{SPR_ESTA, 0, -1, {NULL}, 0, 0, S_NULL}, // S_EGGSTATUE1

	// Hidden sling appears
	{SPR_NULL, 0, -1, {NULL},          0, 0, S_SLING2}, // S_SLING1
	{SPR_NULL, 0, -1, {A_SlingAppear}, 0, 0, S_NULL},   // S_SLING2

	// CEZ maces and chains
	{SPR_SMCH, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SMALLMACECHAIN
	{SPR_BMCH, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BIGMACECHAIN
	{SPR_SMCE, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SMALLMACE
	{SPR_BMCE, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BIGMACE
	{SPR_SMCH, 1, -1, {NULL}, 0, 0, S_NULL}, // S_SMALLGRABCHAIN
	{SPR_BMCH, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BIGGRABCHAIN

	// Small Firebar
	{SPR_SFBR, FF_FULLBRIGHT,     1, {NULL},            0, 0, S_SMALLFIREBAR2},  // S_SMALLFIREBAR1
	{SPR_SFBR, FF_FULLBRIGHT| 1,  1, {NULL},            0, 0, S_SMALLFIREBAR3},  // S_SMALLFIREBAR2
	{SPR_SFBR, FF_FULLBRIGHT| 2,  1, {A_FlameParticle}, 0, 0, S_SMALLFIREBAR4},  // S_SMALLFIREBAR3
	{SPR_SFBR, FF_FULLBRIGHT| 3,  1, {NULL},            0, 0, S_SMALLFIREBAR5},  // S_SMALLFIREBAR4
	{SPR_SFBR, FF_FULLBRIGHT| 4,  1, {NULL},            0, 0, S_SMALLFIREBAR6},  // S_SMALLFIREBAR5
	{SPR_SFBR, FF_FULLBRIGHT| 5,  1, {NULL},            0, 0, S_SMALLFIREBAR7},  // S_SMALLFIREBAR6
	{SPR_SFBR, FF_FULLBRIGHT| 6,  1, {A_FlameParticle}, 0, 0, S_SMALLFIREBAR8},  // S_SMALLFIREBAR7
	{SPR_SFBR, FF_FULLBRIGHT| 7,  1, {NULL},            0, 0, S_SMALLFIREBAR9},  // S_SMALLFIREBAR8
	{SPR_SFBR, FF_FULLBRIGHT| 8,  1, {NULL},            0, 0, S_SMALLFIREBAR10}, // S_SMALLFIREBAR9
	{SPR_SFBR, FF_FULLBRIGHT| 9,  1, {NULL},            0, 0, S_SMALLFIREBAR11}, // S_SMALLFIREBAR10
	{SPR_SFBR, FF_FULLBRIGHT|10,  1, {A_FlameParticle}, 0, 0, S_SMALLFIREBAR12}, // S_SMALLFIREBAR11
	{SPR_SFBR, FF_FULLBRIGHT|11,  1, {NULL},            0, 0, S_SMALLFIREBAR13}, // S_SMALLFIREBAR12
	{SPR_SFBR, FF_FULLBRIGHT|12,  1, {NULL},            0, 0, S_SMALLFIREBAR14}, // S_SMALLFIREBAR13
	{SPR_SFBR, FF_FULLBRIGHT|13,  1, {NULL},            0, 0, S_SMALLFIREBAR15}, // S_SMALLFIREBAR14
	{SPR_SFBR, FF_FULLBRIGHT|14,  1, {A_FlameParticle}, 0, 0, S_SMALLFIREBAR16}, // S_SMALLFIREBAR15
	{SPR_SFBR, FF_FULLBRIGHT|15,  1, {NULL},            0, 0, S_SMALLFIREBAR1},  // S_SMALLFIREBAR16

	// Big Firebar
	{SPR_BFBR, FF_FULLBRIGHT,     1, {NULL},            0, 0, S_BIGFIREBAR2},  // S_BIGFIREBAR1
	{SPR_BFBR, FF_FULLBRIGHT| 1,  1, {NULL},            0, 0, S_BIGFIREBAR3},  // S_BIGFIREBAR2
	{SPR_BFBR, FF_FULLBRIGHT| 2,  1, {A_FlameParticle}, 0, 0, S_BIGFIREBAR4},  // S_BIGFIREBAR3
	{SPR_BFBR, FF_FULLBRIGHT| 3,  1, {NULL},            0, 0, S_BIGFIREBAR5},  // S_BIGFIREBAR4
	{SPR_BFBR, FF_FULLBRIGHT| 4,  1, {NULL},            0, 0, S_BIGFIREBAR6},  // S_BIGFIREBAR5
	{SPR_BFBR, FF_FULLBRIGHT| 5,  1, {NULL},            0, 0, S_BIGFIREBAR7},  // S_BIGFIREBAR6
	{SPR_BFBR, FF_FULLBRIGHT| 6,  1, {A_FlameParticle}, 0, 0, S_BIGFIREBAR8},  // S_BIGFIREBAR7
	{SPR_BFBR, FF_FULLBRIGHT| 7,  1, {NULL},            0, 0, S_BIGFIREBAR9},  // S_BIGFIREBAR8
	{SPR_BFBR, FF_FULLBRIGHT| 8,  1, {NULL},            0, 0, S_BIGFIREBAR10}, // S_BIGFIREBAR9
	{SPR_BFBR, FF_FULLBRIGHT| 9,  1, {NULL},            0, 0, S_BIGFIREBAR11}, // S_BIGFIREBAR10
	{SPR_BFBR, FF_FULLBRIGHT|10,  1, {A_FlameParticle}, 0, 0, S_BIGFIREBAR12}, // S_BIGFIREBAR11
	{SPR_BFBR, FF_FULLBRIGHT|11,  1, {NULL},            0, 0, S_BIGFIREBAR13}, // S_BIGFIREBAR12
	{SPR_BFBR, FF_FULLBRIGHT|12,  1, {NULL},            0, 0, S_BIGFIREBAR14}, // S_BIGFIREBAR13
	{SPR_BFBR, FF_FULLBRIGHT|13,  1, {NULL},            0, 0, S_BIGFIREBAR15}, // S_BIGFIREBAR14
	{SPR_BFBR, FF_FULLBRIGHT|14,  1, {A_FlameParticle}, 0, 0, S_BIGFIREBAR16}, // S_BIGFIREBAR15
	{SPR_BFBR, FF_FULLBRIGHT|15,  1, {NULL},            0, 0, S_BIGFIREBAR1},  // S_BIGFIREBAR16

	{SPR_FWR4, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CEZFLOWER
	{SPR_BANR, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CEZPOLE

	{SPR_BANR, FF_PAPERSPRITE|1, -1, {NULL}, 0, 0, S_NULL}, // S_CEZBANNER1
	{SPR_BANR, FF_PAPERSPRITE|2, -1, {NULL}, 0, 0, S_NULL}, // S_CEZBANNER2

	{SPR_PINE, 0, -1, {NULL}, 0, 0, S_NULL}, // S_PINETREE
	{SPR_CEZB, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CEZBUSH1
	{SPR_CEZB, 1, -1, {NULL}, 0, 0, S_NULL}, // S_CEZBUSH2

	{SPR_CNDL, FF_FULLBRIGHT,   -1, {NULL}, 0, 0, S_NULL}, // S_CANDLE
	{SPR_CNDL, FF_FULLBRIGHT|1, -1, {NULL}, 0, 0, S_NULL}, // S_CANDLEPRICKET

	{SPR_FLMH, 0, -1, {NULL}, 0, 0, S_NULL}, // S_FLAMEHOLDER

	{SPR_CTRC, FF_FULLBRIGHT|FF_ANIMATE, 8*3, {A_FlameParticle}, 3, 3, S_FIRETORCH}, // S_FIRETORCH

	{SPR_CFLG,                0, -1, {NULL}, 0, 0, S_NULL}, // S_WAVINGFLAG
	{SPR_CFLG, FF_PAPERSPRITE|1, -1, {NULL}, 0, 0, S_NULL}, // S_WAVINGFLAGSEG1
	{SPR_CFLG, FF_PAPERSPRITE|2, -1, {NULL}, 0, 0, S_NULL}, // S_WAVINGFLAGSEG2

	{SPR_CSTA, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CRAWLASTATUE

	{SPR_CABR, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BRAMBLES

	// Big Tumbleweed
	{SPR_BTBL, 0, -1, {NULL}, 0, 0, S_NULL},                // S_BIGTUMBLEWEED
	{SPR_BTBL, 0,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL2}, // S_BIGTUMBLEWEED_ROLL1
	{SPR_BTBL, 1,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL3}, // S_BIGTUMBLEWEED_ROLL2
	{SPR_BTBL, 2,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL4}, // S_BIGTUMBLEWEED_ROLL3
	{SPR_BTBL, 3,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL5}, // S_BIGTUMBLEWEED_ROLL4
	{SPR_BTBL, 4,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL6}, // S_BIGTUMBLEWEED_ROLL5
	{SPR_BTBL, 5,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL7}, // S_BIGTUMBLEWEED_ROLL6
	{SPR_BTBL, 6,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL8}, // S_BIGTUMBLEWEED_ROLL7
	{SPR_BTBL, 7,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL1}, // S_BIGTUMBLEWEED_ROLL8

	// Little Tumbleweed
	{SPR_STBL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_LITTLETUMBLEWEED
	{SPR_STBL, 0, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL2}, // S_LITTLETUMBLEWEED_ROLL1
	{SPR_STBL, 1, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL3}, // S_LITTLETUMBLEWEED_ROLL2
	{SPR_STBL, 2, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL4}, // S_LITTLETUMBLEWEED_ROLL3
	{SPR_STBL, 3, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL5}, // S_LITTLETUMBLEWEED_ROLL4
	{SPR_STBL, 4, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL6}, // S_LITTLETUMBLEWEED_ROLL5
	{SPR_STBL, 5, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL7}, // S_LITTLETUMBLEWEED_ROLL6
	{SPR_STBL, 6, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL8}, // S_LITTLETUMBLEWEED_ROLL7
	{SPR_STBL, 7, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL1}, // S_LITTLETUMBLEWEED_ROLL8

	// Cacti
	{SPR_CACT, 0, -1, {A_ConnectToGround}, MT_CACTITINYSEG, 0, S_NULL}, // S_CACTI1
	{SPR_CACT, 1, -1, {A_ConnectToGround}, MT_CACTISMALLSEG, 0, S_NULL}, // S_CACTI2
	{SPR_CACT, 2, -1, {A_ConnectToGround}, MT_CACTITINYSEG, 0, S_NULL}, // S_CACTI3
	{SPR_CACT, 3, -1, {A_ConnectToGround}, MT_CACTISMALLSEG, 0, S_NULL}, // S_CACTI4
	{SPR_CACT, 4, -1, {NULL}, 0, 0, S_NULL}, // S_CACTI5
	{SPR_CACT, 5, -1, {NULL}, 0, 0, S_NULL}, // S_CACTI6
	{SPR_CACT, 6, -1, {NULL}, 0, 0, S_NULL}, // S_CACTI7
	{SPR_CACT, 7, -1, {NULL}, 0, 0, S_NULL}, // S_CACTI8
	{SPR_CACT, 8, -1, {NULL}, 0, 0, S_NULL}, // S_CACTI9
	{SPR_CACT, 9, -1, {A_ConnectToGround}, MT_CACTITINYSEG, 0, S_NULL}, // S_CACTI10
	{SPR_CACT, 10, -1, {A_ConnectToGround}, MT_CACTISMALLSEG, 0, S_NULL}, // S_CACTI11
	{SPR_CACT, 11, -1, {NULL}, 0, 0, S_NULL}, // S_CACTITINYSEG
	{SPR_CACT, 12, -1, {NULL}, 0, 0, S_NULL}, // S_CACTISMALLSEG

	// Warning Signs
	{SPR_WWSG, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_ARIDSIGN_CAUTION
	{SPR_WWS2, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_ARIDSIGN_CACTI
	{SPR_WWS3, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_ARIDSIGN_SHARPTURN

	// Oil lamp
	{SPR_OILL, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_OILLAMP
	{SPR_OILF, FF_TRANS90|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_OILLAMPFLARE

	// TNT barrel
	{SPR_BARR, 0, -1, {NULL}, 0, 0, S_NULL}, // S_TNTBARREL_STND1
	{SPR_BARX, 0, 0, {A_RollAngle}, 0, 1, S_TNTBARREL_EXPL2}, // S_TNTBARREL_EXPL1
	{SPR_BARX, 0|FF_FULLBRIGHT, 3, {A_SetObjectFlags}, MF_NOCLIP|MF_NOGRAVITY|MF_NOBLOCKMAP, 0, S_TNTBARREL_EXPL3}, // S_TNTBARREL_EXPL2
	{SPR_BARX, 1|FF_FULLBRIGHT, 2, {A_TNTExplode}, MT_TNTDUST, 0, S_TNTBARREL_EXPL4}, // S_TNTBARREL_EXPL3
	{SPR_BARX, 1|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_TNTBARREL_EXPL5}, // S_TNTBARREL_EXPL4
	{SPR_BARX, 2|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_TNTBARREL_EXPL6}, // S_TNTBARREL_EXPL5
	{SPR_BARX, 3|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_TNTBARREL_EXPL7}, // S_TNTBARREL_EXPL6
	{SPR_NULL, 0, 35, {NULL}, 0, 0, S_NULL}, // S_TNTBARREL_EXPL7
#ifndef ROTSPRITE
	{SPR_BARR, 1|FF_ANIMATE, -1, {NULL}, 7, 2, S_NULL}, // S_TNTBARREL_FLYING
#else
	{SPR_BARR, 1, 1, {A_RollAngle}, 14, 0, S_TNTBARREL_FLYING}, // S_TNTBARREL_FLYING
#endif
	{SPR_BARD, 0|FF_TRANS90, 2, {NULL}, 0, 0, S_TNTDUST_2}, // S_TNTDUST_1
	{SPR_BARD, 0|FF_TRANS30, 2*TICRATE, {A_SetRandomTics}, 2, TICRATE, S_TNTDUST_3}, // S_TNTDUST_2
	{SPR_BARD, 0|FF_TRANS40, 10, {NULL}, 0, 0, S_TNTDUST_4}, // S_TNTDUST_3
	{SPR_BARD, 0|FF_TRANS50, 10, {NULL}, 0, 0, S_TNTDUST_5}, // S_TNTDUST_4
	{SPR_BARD, 0|FF_TRANS60, 10, {NULL}, 0, 0, S_TNTDUST_6}, // S_TNTDUST_5
	{SPR_BARD, 0|FF_TRANS70, 10, {NULL}, 0, 0, S_TNTDUST_7}, // S_TNTDUST_6
	{SPR_BARD, 0|FF_TRANS80, 10, {NULL}, 0, 0, S_TNTDUST_8}, // S_TNTDUST_7
	{SPR_BARD, 0|FF_TRANS90, 10, {NULL}, 0, 0, S_NULL}, // S_TNTDUST_8

	// TNT proximity shell
	{SPR_REMT, 0, 10, {A_Look}, 33554433, 0, S_PROXIMITY_TNT}, // S_PROXIMITY_TNT
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER2}, // S_PROXIMITY_TNT_TRIGGER1
	{SPR_REMT, 0, 16, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER3}, // S_PROXIMITY_TNT_TRIGGER2
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER4}, // S_PROXIMITY_TNT_TRIGGER3
	{SPR_REMT, 0, 16, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER5}, // S_PROXIMITY_TNT_TRIGGER4
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER6}, // S_PROXIMITY_TNT_TRIGGER5
	{SPR_REMT, 0, 4, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER7}, // S_PROXIMITY_TNT_TRIGGER6
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER8}, // S_PROXIMITY_TNT_TRIGGER7
	{SPR_REMT, 0, 4, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER9}, // S_PROXIMITY_TNT_TRIGGER8
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER10}, // S_PROXIMITY_TNT_TRIGGER9
	{SPR_REMT, 0, 4, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER11}, // S_PROXIMITY_TNT_TRIGGER10
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER12}, // S_PROXIMITY_TNT_TRIGGER11
	{SPR_REMT, 0, 4, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER13}, // S_PROXIMITY_TNT_TRIGGER12
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER14}, // S_PROXIMITY_TNT_TRIGGER13
	{SPR_REMT, 0, 2, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER15}, // S_PROXIMITY_TNT_TRIGGER14
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER16}, // S_PROXIMITY_TNT_TRIGGER15
	{SPR_REMT, 0, 2, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER17}, // S_PROXIMITY_TNT_TRIGGER16
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER18}, // S_PROXIMITY_TNT_TRIGGER17
	{SPR_REMT, 0, 2, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER19}, // S_PROXIMITY_TNT_TRIGGER18
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER20}, // S_PROXIMITY_TNT_TRIGGER19
	{SPR_REMT, 0, 2, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER21}, // S_PROXIMITY_TNT_TRIGGER20
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER22}, // S_PROXIMITY_TNT_TRIGGER21
	{SPR_REMT, 0, 2, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER23}, // S_PROXIMITY_TNT_TRIGGER22
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_TNTBARREL_EXPL1}, // S_PROXIMITY_TNT_TRIGGER23

	// Dust devil
	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_DUSTDEVIL}, //S_DUSTDEVIL
	{SPR_TAZD, 1|FF_PAPERSPRITE|FF_TRANS70, 2 * TICRATE, {NULL}, 0, 0, S_DUSTLAYER2}, // S_DUSTLAYER1
	{SPR_TAZD, 1|FF_PAPERSPRITE|FF_TRANS70, 5, {NULL}, 0, 0, S_DUSTLAYER3}, // S_DUSTLAYER2
	{SPR_TAZD, 1|FF_PAPERSPRITE|FF_TRANS80, 5, {NULL}, 0, 0, S_DUSTLAYER4}, // S_DUSTLAYER3
	{SPR_TAZD, 1|FF_PAPERSPRITE|FF_TRANS80, 5, {NULL}, 0, 0, S_DUSTLAYER5}, // S_DUSTLAYER4
	{SPR_TAZD, 1|FF_PAPERSPRITE|FF_TRANS90, 5, {NULL}, 0, 0, S_NULL}, // S_DUSTLAYER5
	{SPR_ADST, 0|FF_ANIMATE, 24, {NULL}, 3, 8, S_NULL}, // S_ARIDDUST1
	{SPR_ADST, 3|FF_ANIMATE, 24, {NULL}, 3, 8, S_NULL}, // S_ARIDDUST2
	{SPR_ADST, 6|FF_ANIMATE, 24, {NULL}, 3, 8, S_NULL}, // S_ARIDDUST3

	// Flame jet
	{SPR_NULL, 0, 2*TICRATE, {NULL},             0, 0, S_FLAMEJETSTART}, // S_FLAMEJETSTND
	{SPR_NULL, 0, 3*TICRATE, {A_ToggleFlameJet}, 0, 0,  S_FLAMEJETSTOP}, // S_FLAMEJETSTART
	{SPR_NULL, 0,         1, {A_ToggleFlameJet}, 0, 0,  S_FLAMEJETSTND}, // S_FLAMEJETSTOP
	{SPR_FLME, FF_FULLBRIGHT  ,  4, {NULL}, 0, 0, S_FLAMEJETFLAME2}, // S_FLAMEJETFLAME1
	{SPR_FLME, FF_FULLBRIGHT|1,  5, {NULL}, 0, 0, S_FLAMEJETFLAME3}, // S_FLAMEJETFLAME2
	{SPR_FLME, FF_FULLBRIGHT|2, 11, {NULL}, 0, 0,           S_NULL}, // S_FLAMEJETFLAME3
	{SPR_FLME, FF_FULLBRIGHT|3,  4, {NULL}, 0, 0, S_FLAMEJETFLAME5}, // S_FLAMEJETFLAME4
	{SPR_FLME, FF_FULLBRIGHT|4,  5, {NULL}, 0, 0, S_FLAMEJETFLAME6}, // S_FLAMEJETFLAME5
	{SPR_FLME, FF_FULLBRIGHT|5, 11, {NULL}, 0, 0,           S_NULL}, // S_FLAMEJETFLAME6
	{SPR_FLME, FF_FULLBRIGHT|6,  4, {NULL}, 0, 0, S_FLAMEJETFLAME8}, // S_FLAMEJETFLAME7
	{SPR_FLME, FF_FULLBRIGHT|7,  5, {NULL}, 0, 0, S_FLAMEJETFLAME9}, // S_FLAMEJETFLAME8
	{SPR_FLME, FF_FULLBRIGHT|8, 11, {NULL}, 0, 0,           S_NULL}, // S_FLAMEJETFLAME9

	// Spinning flame jets
	// A: Counter-clockwise
	{SPR_NULL, 0, 1,            {A_TrapShot}, MT_FLAMEJETFLAMEB, -(16<<16)|(1<<15)|64, S_FJSPINAXISA2}, // S_FJSPINAXISA1
	{SPR_NULL, 0, 2, {A_ChangeAngleRelative},                 6,         6, S_FJSPINAXISA1}, // S_FJSPINAXISA2

	// B: Clockwise
	{SPR_NULL, 0, 1,            {A_TrapShot}, MT_FLAMEJETFLAMEB, -(16<<16)|(1<<15)|64, S_FJSPINAXISB2}, // S_FJSPINAXISB1
	{SPR_NULL, 0, 2, {A_ChangeAngleRelative},                -6,        -6, S_FJSPINAXISB1}, // S_FJSPINAXISB2

	// Blade's flame
	{SPR_DFLM, FF_FULLBRIGHT|FF_TRANS40, 1, {A_MoveRelative}, 0, 5, S_FLAMEJETFLAMEB2}, // S_FLAMEJETFLAMEB1
	{SPR_DFLM, FF_FULLBRIGHT|FF_TRANS40, 1, {A_MoveRelative}, 0, 7, S_FLAMEJETFLAMEB3}, // S_FLAMEJETFLAMEB2
	{SPR_DFLM, FF_FULLBRIGHT|FF_TRANS40|FF_ANIMATE, (12*7), {NULL}, 7, 12, S_NULL},  // S_FLAMEJETFLAMEB3

	// Lavafall
	{SPR_LFAL, 5, 1, {NULL}, 0, 0, S_LAVAFALL_DORMANT}, // S_LAVAFALL_DORMANT
	{SPR_LFAL, 6|FF_ANIMATE, 4, {A_LavafallRocks}, 1, 2, S_LAVAFALL_TELL}, // S_LAVAFALL_TELL
	{SPR_LFAL, 9|FF_FULLBRIGHT|FF_ANIMATE, 2, {A_LavafallLava}, 1, 1, S_LAVAFALL_SHOOT}, // S_LAVAFALL_SHOOT
	{SPR_LFAL, FF_FULLBRIGHT, 1, {A_FallingLavaCheck}, 0, 0, S_LAVAFALL_LAVA2}, // S_LAVAFALL_LAVA1
	{SPR_LFAL, FF_FULLBRIGHT, 1, {A_FallingLavaCheck}, 0, 0, S_LAVAFALL_LAVA1}, // S_LAVAFALL_LAVA2
	{SPR_LFAL, 2|FF_FULLBRIGHT|FF_ANIMATE, 9, {NULL}, 2, 3, S_NULL}, // S_LAVAFALL_LAVA3
	{SPR_LFAL, 11|FF_ANIMATE|FF_RANDOMANIM, 12, {NULL}, 3, 3, S_LAVAFALLROCK}, // S_LAVAFALLROCK

	// RVZ scenery
	{SPR_JPLA, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_BIGFERNLEAF
	{SPR_JPLA, 1, 1, {NULL}, 0, 0, S_BIGFERN2}, // S_BIGFERN1
	{SPR_JPLA, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BIGFERN2
	{SPR_JPLA, 2, -1, {NULL}, 0, 0, S_NULL}, // S_JUNGLEPALM
	{SPR_TFLO, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_TORCHFLOWER}, // S_TORCHFLOWER
	{SPR_WVIN, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_WALLVINE_LONG
	{SPR_WVIN, 1|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_WALLVINE_SHORT

	// Stalagmites
	{SPR_STLG, 0, -1, {NULL}, 0, 0, S_NULL}, // S_STG0
	{SPR_STLG, 1, -1, {NULL}, 0, 0, S_NULL}, // S_STG1
	{SPR_STLG, 2, -1, {NULL}, 0, 0, S_NULL}, // S_STG2
	{SPR_STLG, 3, -1, {NULL}, 0, 0, S_NULL}, // S_STG3
	{SPR_STLG, 4, -1, {NULL}, 0, 0, S_NULL}, // S_STG4
	{SPR_STLG, 5, -1, {NULL}, 0, 0, S_NULL}, // S_STG5
	{SPR_STLG, 6, -1, {NULL}, 0, 0, S_NULL}, // S_STG6
	{SPR_STLG, 7, -1, {NULL}, 0, 0, S_NULL}, // S_STG7
	{SPR_STLG, 8, -1, {NULL}, 0, 0, S_NULL}, // S_STG8
	{SPR_STLG, 9, -1, {NULL}, 0, 0, S_NULL}, // S_STG9

	// Xmas-specific stuff
	{SPR_XMS1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_XMASPOLE
	{SPR_XMS2, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CANDYCANE
	{SPR_XMS3, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SNOWMAN
	{SPR_XMS3, 1, -1, {NULL}, 0, 0, S_NULL}, // S_SNOWMANHAT
	{SPR_XMS4, 0, -1, {NULL}, 0, 0, S_NULL}, // S_LAMPPOST1
	{SPR_XMS4, 1, -1, {NULL}, 0, 0, S_NULL}, // S_LAMPPOST2
	{SPR_XMS5, 0, -1, {NULL}, 0, 0, S_NULL}, // S_HANGSTAR
	{SPR_XMS6, 0, -1, {NULL}, 0, 0, S_NULL}, // S_MISTLETOE
	// Xmas GFZ bushes
	{SPR_BUS3, 1, -1, {NULL}, 0, 0, S_NULL}, // S_XMASBLUEBERRYBUSH
	{SPR_BUS1, 1, -1, {NULL}, 0, 0, S_NULL}, // S_XMASBERRYBUSH
	{SPR_BUS2, 1, -1, {NULL}, 0, 0, S_NULL}, // S_XMASBUSH
	// FHZ
	{SPR_FHZI, 0, -1, {NULL}, 0, 0, S_NULL}, // S_FHZICE1
	{SPR_FHZI, 1, -1, {NULL}, 0, 0, S_NULL}, // S_FHZICE2

	// Halloween Scenery
	// Pumpkins
	{SPR_PUMK,  0, -1, {NULL}, 0, 0, S_NULL}, // S_JACKO1
	{SPR_PUMK,  3|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO1OVERLAY_2}, // S_JACKO1OVERLAY_1
	{SPR_PUMK,  4|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO1OVERLAY_3}, // S_JACKO1OVERLAY_2
	{SPR_PUMK,  5|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO1OVERLAY_4}, // S_JACKO1OVERLAY_3
	{SPR_PUMK,  4|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO1OVERLAY_1}, // S_JACKO1OVERLAY_4
	{SPR_PUMK,  1, -1, {NULL}, 0, 0, S_NULL}, // S_JACKO2
	{SPR_PUMK,  6|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO2OVERLAY_2}, // S_JACKO2OVERLAY_1
	{SPR_PUMK,  7|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO2OVERLAY_3}, // S_JACKO2OVERLAY_2
	{SPR_PUMK,  8|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO2OVERLAY_4}, // S_JACKO2OVERLAY_3
	{SPR_PUMK,  7|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO2OVERLAY_1}, // S_JACKO2OVERLAY_4
	{SPR_PUMK,  2, -1, {NULL}, 0, 0, S_NULL}, // S_JACKO3
	{SPR_PUMK,  9|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO3OVERLAY_2}, // S_JACKO3OVERLAY_1
	{SPR_PUMK, 10|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO3OVERLAY_3}, // S_JACKO3OVERLAY_2
	{SPR_PUMK, 11|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO3OVERLAY_4}, // S_JACKO3OVERLAY_3
	{SPR_PUMK, 10|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO3OVERLAY_1}, // S_JACKO3OVERLAY_4
	// Dr Seuss Trees
	{SPR_HHPL, 2, -1, {A_ConnectToGround}, MT_HHZTREE_PART, 0, S_NULL}, // S_HHZTREE_TOP,
	{SPR_HHPL, 1, -1, {NULL}, 0, 0, S_NULL}, // S_HHZTREE_TRUNK,
	{SPR_HHPL, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_HHZTREE_LEAF,
	// Mushroom
	{SPR_SHRM, 4,  3, {NULL}, 0, 0, S_HHZSHROOM_2},  // S_HHZSHROOM_1,
	{SPR_SHRM, 3,  3, {NULL}, 0, 0, S_HHZSHROOM_3},  // S_HHZSHROOM_2,
	{SPR_SHRM, 2,  2, {NULL}, 0, 0, S_HHZSHROOM_4},  // S_HHZSHROOM_3,
	{SPR_SHRM, 1,  1, {NULL}, 0, 0, S_HHZSHROOM_5},  // S_HHZSHROOM_4,
	{SPR_SHRM, 0,  1, {NULL}, 0, 0, S_HHZSHROOM_6},  // S_HHZSHROOM_5,
	{SPR_SHRM, 1,  4, {NULL}, 0, 0, S_HHZSHROOM_7},  // S_HHZSHROOM_6,
	{SPR_SHRM, 2,  2, {NULL}, 0, 0, S_HHZSHROOM_8},  // S_HHZSHROOM_7,
	{SPR_SHRM, 3,  3, {NULL}, 0, 0, S_HHZSHROOM_9},  // S_HHZSHROOM_8,
	{SPR_SHRM, 4,  3, {NULL}, 0, 0, S_HHZSHROOM_10}, // S_HHZSHROOM_9,
	{SPR_SHRM, 3,  3, {NULL}, 0, 0, S_HHZSHROOM_11}, // S_HHZSHROOM_10,
	{SPR_SHRM, 5,  2, {NULL}, 0, 0, S_HHZSHROOM_12}, // S_HHZSHROOM_11,
	{SPR_SHRM, 6,  1, {NULL}, 0, 0, S_HHZSHROOM_13}, // S_HHZSHROOM_12,
	{SPR_SHRM, 7,  1, {NULL}, 0, 0, S_HHZSHROOM_14}, // S_HHZSHROOM_13,
	{SPR_SHRM, 6,  4, {NULL}, 0, 0, S_HHZSHROOM_15}, // S_HHZSHROOM_14,
	{SPR_SHRM, 5,  2, {NULL}, 0, 0, S_HHZSHROOM_16}, // S_HHZSHROOM_15,
	{SPR_SHRM, 3,  3, {NULL}, 0, 0, S_HHZSHROOM_1},  // S_HHZSHROOM_16,
	// Misc
	{SPR_HHZM, 0, -1, {NULL}, 0, 0, S_NULL}, // S_HHZGRASS,
	{SPR_HHZM, 1, -1, {NULL}, 0, 0, S_NULL}, // S_HHZTENT1,
	{SPR_HHZM, 2, -1, {NULL}, 0, 0, S_NULL}, // S_HHZTENT2,
	{SPR_HHZM, 4, -1, {NULL}, 0, 0, S_NULL}, // S_HHZSTALAGMITE_TALL,
	{SPR_HHZM, 5, -1, {NULL}, 0, 0, S_NULL}, // S_HHZSTALAGMITE_SHORT,

	// Loads of Botanic Serenity bullshit
	{SPR_BSZ1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_RED
	{SPR_BSZ1, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_PURPLE
	{SPR_BSZ1, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_BLUE
	{SPR_BSZ1, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_CYAN
	{SPR_BSZ1, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_YELLOW
	{SPR_BSZ1, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_ORANGE
	{SPR_BSZ2, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_RED
	{SPR_BSZ2, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_PURPLE
	{SPR_BSZ2, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_BLUE
	{SPR_BSZ2, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_CYAN
	{SPR_BSZ2, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_YELLOW
	{SPR_BSZ2, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_ORANGE
	{SPR_BSZ3, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_RED
	{SPR_BSZ3, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_PURPLE
	{SPR_BSZ3, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_BLUE
	{SPR_BSZ3, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_CYAN
	{SPR_BSZ3, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_YELLOW
	{SPR_BSZ3, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_ORANGE
	{SPR_BST1, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_RED
	{SPR_BST2, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_PURPLE
	{SPR_BST3, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_BLUE
	{SPR_BST4, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_CYAN
	{SPR_BST5, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_YELLOW
	{SPR_BST6, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_ORANGE
	{SPR_BSZ5, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_RED
	{SPR_BSZ5, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_PURPLE
	{SPR_BSZ5, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_BLUE
	{SPR_BSZ5, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_CYAN
	{SPR_BSZ5, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_YELLOW
	{SPR_BSZ5, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_ORANGE
	{SPR_BSZ6, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_RED
	{SPR_BSZ6, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_PURPLE
	{SPR_BSZ6, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_BLUE
	{SPR_BSZ6, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_CYAN
	{SPR_BSZ6, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_YELLOW
	{SPR_BSZ6, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_ORANGE
	{SPR_BSZ7, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_RED
	{SPR_BSZ7, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_PURPLE
	{SPR_BSZ7, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_BLUE
	{SPR_BSZ7, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_CYAN
	{SPR_BSZ7, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_YELLOW
	{SPR_BSZ7, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_ORANGE
	{SPR_BSZ8, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHRUB
	{SPR_BSZ8, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLOVER
	{SPR_BSZ8, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BIG_PALMTREE_TRUNK
	{SPR_BSZ8, 3, -1, {A_ConnectToGround}, MT_BIG_PALMTREE_TRUNK, 0, S_NULL}, // S_BIG_PALMTREE_TOP
	{SPR_BSZ8, 4, -1, {NULL}, 0, 0, S_NULL}, // S_PALMTREE_TRUNK
	{SPR_BSZ8, 5, -1, {A_ConnectToGround},     MT_PALMTREE_TRUNK, 0, S_NULL}, // S_PALMTREE_TOP

	// Disco ball
	{SPR_DBAL, FF_FULLBRIGHT,   5, {NULL}, 0, 0, S_DBALL2}, // S_DBALL1
	{SPR_DBAL, FF_FULLBRIGHT|1, 5, {NULL}, 0, 0, S_DBALL3}, // S_DBALL2
	{SPR_DBAL, FF_FULLBRIGHT|2, 5, {NULL}, 0, 0, S_DBALL4}, // S_DBALL3
	{SPR_DBAL, FF_FULLBRIGHT|3, 5, {NULL}, 0, 0, S_DBALL5}, // S_DBALL4
	{SPR_DBAL, FF_FULLBRIGHT|4, 5, {NULL}, 0, 0, S_DBALL6}, // S_DBALL5
	{SPR_DBAL, FF_FULLBRIGHT|5, 5, {NULL}, 0, 0, S_DBALL1}, // S_DBALL6

	{SPR_ESTA, 1, -1, {NULL}, 0, 0, S_NULL}, // S_EGGSTATUE2

	// Super Sonic Spark
	{SPR_SSPK,   FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_SSPK2}, // S_SSPK1
	{SPR_SSPK, 1|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_SSPK3}, // S_SSPK2
	{SPR_SSPK, 2|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_SSPK4}, // S_SSPK3
	{SPR_SSPK, 1|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_SSPK5}, // S_SSPK4
	{SPR_SSPK,   FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL},  // S_SSPK5

	// Flicky-sized bubble
	{SPR_FBUB, 0, -1, {NULL}, 0, 0, S_NULL}, // S_FLICKY_BUBBLE

	// Bluebird
	{SPR_FL01, 0, 2, {A_FlickyCheck}, S_FLICKY_01_FLAP1, S_FLICKY_01_FLAP1, S_FLICKY_01_OUT},   // S_FLICKY_01_OUT
	{SPR_FL01, 1, 3, {A_FlickyFly},          4*FRACUNIT,       16*FRACUNIT, S_FLICKY_01_FLAP2}, // S_FLICKY_01_FLAP1
	{SPR_FL01, 2, 3, {A_FlickyFly},          4*FRACUNIT,       16*FRACUNIT, S_FLICKY_01_FLAP3}, // S_FLICKY_01_FLAP2
	{SPR_FL01, 3, 3, {A_FlickyFly},          4*FRACUNIT,       16*FRACUNIT, S_FLICKY_01_FLAP1}, // S_FLICKY_01_FLAP3
	{SPR_FL01, FF_ANIMATE|1, -1, {NULL}, 2, 3, S_NULL},                                         // S_FLICKY_01_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_01, 384*FRACUNIT, S_FLICKY_01_CENTER},        // S_FLICKY_01_CENTER

	// Rabbit
	{SPR_FL02, 0, 2, {A_FlickyCheck}, S_FLICKY_02_AIM,                0, S_FLICKY_02_OUT},  // S_FLICKY_02_OUT
	{SPR_FL02, 1, 1, {A_FlickyAim},             ANG30,      32*FRACUNIT, S_FLICKY_02_HOP},  // S_FLICKY_02_AIM
	{SPR_FL02, 1, 1, {A_FlickyHop},        6*FRACUNIT,       4*FRACUNIT, S_FLICKY_02_UP},   // S_FLICKY_02_HOP
	{SPR_FL02, 2, 2, {A_FlickyCheck}, S_FLICKY_02_AIM, S_FLICKY_02_DOWN, S_FLICKY_02_UP},   // S_FLICKY_02_UP
	{SPR_FL02, 3, 2, {A_FlickyCheck}, S_FLICKY_02_AIM,                0, S_FLICKY_02_DOWN}, // S_FLICKY_02_DOWN
	{SPR_FL02, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_02_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_02, 384*FRACUNIT, S_FLICKY_02_CENTER},        // S_FLICKY_02_CENTER

	// Chicken
	{SPR_FL03, 0, 2, {A_FlickyCheck},   S_FLICKY_03_AIM, S_FLICKY_03_FLAP1, S_FLICKY_03_OUT},   // S_FLICKY_03_OUT
	{SPR_FL03, 1, 1, {A_FlickyAim},            ANGLE_45,       32*FRACUNIT, S_FLICKY_03_HOP},   // S_FLICKY_03_AIM
	{SPR_FL03, 1, 1, {A_FlickyHop},          7*FRACUNIT,        2*FRACUNIT, S_FLICKY_03_UP},    // S_FLICKY_03_HOP
	{SPR_FL03, 2, 2, {A_FlickyFlutter}, S_FLICKY_03_HOP, S_FLICKY_03_FLAP1, S_FLICKY_03_UP},    // S_FLICKY_03_UP
	{SPR_FL03, 3, 2, {A_FlickyFlutter}, S_FLICKY_03_HOP,                 0, S_FLICKY_03_FLAP2}, // S_FLICKY_03_FLAP1
	{SPR_FL03, 4, 2, {A_FlickyFlutter}, S_FLICKY_03_HOP,                 0, S_FLICKY_03_FLAP1}, // S_FLICKY_03_FLAP2
	{SPR_FL03, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_03_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_03, 384*FRACUNIT, S_FLICKY_03_CENTER},        // S_FLICKY_03_CENTER

	// Seal
	{SPR_FL04, 0, 2, {A_FlickyCheck}, S_FLICKY_04_AIM,                 0, S_FLICKY_04_OUT},   // S_FLICKY_04_OUT
	{SPR_FL04, 1, 1, {A_FlickyAim},             ANG30,       32*FRACUNIT, S_FLICKY_04_HOP},   // S_FLICKY_04_AIM
	{SPR_FL04, 1, 1, {A_FlickyHop},        3*FRACUNIT,        2*FRACUNIT, S_FLICKY_04_UP},    // S_FLICKY_04_HOP
	{SPR_FL04, 2, 4, {A_FlickyCheck}, S_FLICKY_04_AIM,  S_FLICKY_04_DOWN, S_FLICKY_04_UP},    // S_FLICKY_04_UP
	{SPR_FL04, 3, 4, {A_FlickyCheck}, S_FLICKY_04_AIM,                 0, S_FLICKY_04_DOWN},  // S_FLICKY_04_DOWN
	{SPR_FL04, 3, 4, {A_FlickyFly},        2*FRACUNIT,       48*FRACUNIT, S_FLICKY_04_SWIM2}, // S_FLICKY_04_SWIM1
	{SPR_FL04, 4, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_04_SWIM1, S_FLICKY_04_SWIM3}, // S_FLICKY_04_SWIM2
	{SPR_FL04, 3, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_04_SWIM1, S_FLICKY_04_SWIM4}, // S_FLICKY_04_SWIM3
	{SPR_FL04, 5, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_04_SWIM1, S_FLICKY_04_SWIM1}, // S_FLICKY_04_SWIM4
	{SPR_FL04, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_04_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_04, 384*FRACUNIT, S_FLICKY_04_CENTER},        // S_FLICKY_04_CENTER

	// Pig
	{SPR_FL05, 0, 2, {A_FlickyCheck}, S_FLICKY_05_AIM,                0, S_FLICKY_05_OUT},  // S_FLICKY_05_OUT
	{SPR_FL05, 1, 1, {A_FlickyAim},             ANG20,      32*FRACUNIT, S_FLICKY_05_HOP},  // S_FLICKY_05_AIM
	{SPR_FL05, 1, 1, {A_FlickyHop},        4*FRACUNIT,       3*FRACUNIT, S_FLICKY_05_UP},   // S_FLICKY_05_HOP
	{SPR_FL05, 2, 2, {A_FlickyCheck}, S_FLICKY_05_AIM, S_FLICKY_05_DOWN, S_FLICKY_05_UP},   // S_FLICKY_05_UP
	{SPR_FL05, 3, 2, {A_FlickyCheck}, S_FLICKY_05_AIM,                0, S_FLICKY_05_DOWN}, // S_FLICKY_05_DOWN
	{SPR_FL05, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_05_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_05, 384*FRACUNIT, S_FLICKY_05_CENTER},        // S_FLICKY_05_CENTER

	// Chipmunk
	{SPR_FL06, 0, 2, {A_FlickyCheck}, S_FLICKY_06_AIM,                0, S_FLICKY_06_OUT},  // S_FLICKY_06_OUT
	{SPR_FL06, 1, 1, {A_FlickyAim},          ANGLE_90,      32*FRACUNIT, S_FLICKY_06_HOP},  // S_FLICKY_06_AIM
	{SPR_FL06, 1, 1, {A_FlickyHop},        5*FRACUNIT,       6*FRACUNIT, S_FLICKY_06_UP},   // S_FLICKY_06_HOP
	{SPR_FL06, 2, 2, {A_FlickyCheck}, S_FLICKY_06_AIM, S_FLICKY_06_DOWN, S_FLICKY_06_UP},   // S_FLICKY_06_UP
	{SPR_FL06, 3, 2, {A_FlickyCheck}, S_FLICKY_06_AIM,                0, S_FLICKY_06_DOWN}, // S_FLICKY_06_DOWN
	{SPR_FL06, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_06_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_06, 384*FRACUNIT, S_FLICKY_06_CENTER},        // S_FLICKY_06_CENTER

	// Penguin
	{SPR_FL07, 0, 2, {A_FlickyCheck}, S_FLICKY_07_AIML,                 0, S_FLICKY_07_OUT},   // S_FLICKY_07_OUT
	{SPR_FL07, 1, 1, {A_FlickyAim},              ANG30,       32*FRACUNIT, S_FLICKY_07_HOPL},  // S_FLICKY_07_AIML
	{SPR_FL07, 1, 1, {A_FlickyHop},         4*FRACUNIT,        2*FRACUNIT, S_FLICKY_07_UPL},   // S_FLICKY_07_HOPL
	{SPR_FL07, 2, 4, {A_FlickyCheck}, S_FLICKY_07_AIMR, S_FLICKY_07_DOWNL, S_FLICKY_07_UPL},   // S_FLICKY_07_UPL
	{SPR_FL07, 1, 4, {A_FlickyCheck}, S_FLICKY_07_AIMR,                 0, S_FLICKY_07_DOWNL}, // S_FLICKY_07_DOWNL
	{SPR_FL07, 1, 1, {A_FlickyAim},              ANG30,       32*FRACUNIT, S_FLICKY_07_HOPR},  // S_FLICKY_07_AIMR
	{SPR_FL07, 1, 1, {A_FlickyHop},         4*FRACUNIT,        2*FRACUNIT, S_FLICKY_07_UPR},   // S_FLICKY_07_HOPR
	{SPR_FL07, 3, 4, {A_FlickyCheck}, S_FLICKY_07_AIML, S_FLICKY_07_DOWNR, S_FLICKY_07_UPR},   // S_FLICKY_07_UPR
	{SPR_FL07, 1, 4, {A_FlickyCheck}, S_FLICKY_07_AIML,                 0, S_FLICKY_07_DOWNR}, // S_FLICKY_07_DOWNR
	{SPR_FL07, 4, 4, {A_FlickyFly},         3*FRACUNIT,       72*FRACUNIT, S_FLICKY_07_SWIM2}, // S_FLICKY_07_SWIM1
	{SPR_FL07, 5, 4, {A_FlickyCoast},         FRACUNIT, S_FLICKY_07_SWIM1, S_FLICKY_07_SWIM3}, // S_FLICKY_07_SWIM2
	{SPR_FL07, 6, 4, {A_FlickyCoast},       2*FRACUNIT, S_FLICKY_07_SWIM1, S_FLICKY_07_SWIM3}, // S_FLICKY_07_SWIM3
	{SPR_FL07, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_07_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_07, 384*FRACUNIT, S_FLICKY_07_CENTER},        // S_FLICKY_07_CENTER

	// Fish
	{SPR_FL08, 0, 2, {A_FlickyCheck}, S_FLICKY_08_AIM,                 0, S_FLICKY_08_OUT},   // S_FLICKY_08_OUT
	{SPR_FL08, 2, 1, {A_FlickyAim},             ANG30,       32*FRACUNIT, S_FLICKY_08_HOP},   // S_FLICKY_08_AIM
	{SPR_FL08, 2, 1, {A_FlickyFlounder},   2*FRACUNIT,        1*FRACUNIT, S_FLICKY_08_FLAP1}, // S_FLICKY_08_HOP
	{SPR_FL08, 0, 4, {A_FlickyCheck}, S_FLICKY_08_AIM,                 0, S_FLICKY_08_FLAP2}, // S_FLICKY_08_FLAP1
	{SPR_FL08, 1, 4, {A_FlickyCheck}, S_FLICKY_08_AIM,                 0, S_FLICKY_08_FLAP3}, // S_FLICKY_08_FLAP2
	{SPR_FL08, 0, 4, {A_FlickyCheck}, S_FLICKY_08_AIM,                 0, S_FLICKY_08_FLAP4}, // S_FLICKY_08_FLAP3
	{SPR_FL08, 2, 4, {A_FlickyCheck}, S_FLICKY_08_AIM,                 0, S_FLICKY_08_FLAP1}, // S_FLICKY_08_FLAP4
	{SPR_FL08, 0, 4, {A_FlickyFly},        3*FRACUNIT,       64*FRACUNIT, S_FLICKY_08_SWIM2}, // S_FLICKY_08_SWIM1
	{SPR_FL08, 1, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_08_SWIM1, S_FLICKY_08_SWIM3}, // S_FLICKY_08_SWIM2
	{SPR_FL08, 0, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_08_SWIM1, S_FLICKY_08_SWIM4}, // S_FLICKY_08_SWIM3
	{SPR_FL08, 2, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_08_SWIM1, S_FLICKY_08_SWIM4}, // S_FLICKY_08_SWIM4
	{SPR_FL08, FF_ANIMATE, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_08_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_08, 384*FRACUNIT, S_FLICKY_08_CENTER},        // S_FLICKY_08_CENTER

	// Ram
	{SPR_FL09, 0, 2, {A_FlickyCheck}, S_FLICKY_09_AIM,                0, S_FLICKY_09_OUT},  // S_FLICKY_09_OUT
	{SPR_FL09, 1, 1, {A_FlickyAim},             ANG30,      32*FRACUNIT, S_FLICKY_09_HOP},  // S_FLICKY_09_AIM
	{SPR_FL09, 1, 1, {A_FlickyHop},        7*FRACUNIT,       2*FRACUNIT, S_FLICKY_09_UP},   // S_FLICKY_09_HOP
	{SPR_FL09, 2, 2, {A_FlickyCheck}, S_FLICKY_09_AIM, S_FLICKY_09_DOWN, S_FLICKY_09_UP},   // S_FLICKY_09_UP
	{SPR_FL09, 3, 2, {A_FlickyCheck}, S_FLICKY_09_AIM,                0, S_FLICKY_09_DOWN}, // S_FLICKY_09_DOWN
	{SPR_FL09, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_09_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_09, 384*FRACUNIT, S_FLICKY_09_CENTER},        // S_FLICKY_09_CENTER

	// Puffin
	{SPR_FL10, 0, 2, {A_FlickyCheck}, S_FLICKY_10_FLAP1, S_FLICKY_10_FLAP1, S_FLICKY_10_OUT},   // S_FLICKY_10_OUT
	{SPR_FL10, 1, 3, {A_FlickySoar},         4*FRACUNIT,       16*FRACUNIT, S_FLICKY_10_FLAP2}, // S_FLICKY_10_FLAP1
	{SPR_FL10, 2, 3, {A_FlickySoar},         4*FRACUNIT,       16*FRACUNIT, S_FLICKY_10_FLAP1}, // S_FLICKY_10_FLAP2
	{SPR_FL10, FF_ANIMATE|1, -1, {NULL}, 1, 3, S_NULL}, // S_FLICKY_10_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_10, 384*FRACUNIT, S_FLICKY_10_CENTER},        // S_FLICKY_10_CENTER

	// Cow
	{SPR_FL11, 0, 2, {A_FlickyCheck}, S_FLICKY_11_AIM,           0, S_FLICKY_11_OUT},  // S_FLICKY_11_OUT
	{SPR_FL11, 1, 1, {A_FlickyAim},          ANGLE_90, 64*FRACUNIT, S_FLICKY_11_RUN1}, // S_FLICKY_11_AIM
	{SPR_FL11, 1, 3, {A_FlickyHop},        FRACUNIT/2,  2*FRACUNIT, S_FLICKY_11_RUN2}, // S_FLICKY_11_RUN1
	{SPR_FL11, 2, 4, {A_FlickyHop},        FRACUNIT/2,  2*FRACUNIT, S_FLICKY_11_RUN3}, // S_FLICKY_11_RUN2
	{SPR_FL11, 3, 4, {A_FlickyHop},        FRACUNIT/2,  2*FRACUNIT, S_FLICKY_11_AIM},  // S_FLICKY_11_RUN3
	{SPR_FL11, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_11_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_11, 384*FRACUNIT, S_FLICKY_11_CENTER},        // S_FLICKY_11_CENTER

	// Rat
	{SPR_FL12, 0, 2, {A_FlickyCheck}, S_FLICKY_12_AIM,           0, S_FLICKY_12_OUT},  // S_FLICKY_12_OUT
	{SPR_FL12, 1, 1, {A_FlickyAim},          ANGLE_90, 32*FRACUNIT, S_FLICKY_12_RUN1}, // S_FLICKY_12_AIM
	{SPR_FL12, 1, 2, {A_FlickyHop},                 1, 12*FRACUNIT, S_FLICKY_12_RUN2}, // S_FLICKY_12_RUN1
	{SPR_FL12, 2, 3, {A_FlickyHop},                 1, 12*FRACUNIT, S_FLICKY_12_RUN3}, // S_FLICKY_12_RUN2
	{SPR_FL12, 3, 3, {A_FlickyHop},                 1, 12*FRACUNIT, S_FLICKY_12_AIM},  // S_FLICKY_12_RUN3
	{SPR_FL12, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_12_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_12, 384*FRACUNIT, S_FLICKY_12_CENTER},        // S_FLICKY_12_CENTER

	// Bear
	{SPR_FL13, 0, 2, {A_FlickyCheck}, S_FLICKY_13_AIM,                0, S_FLICKY_13_OUT},  // S_FLICKY_13_OUT
	{SPR_FL13, 1, 1, {A_FlickyAim},             ANG30,      32*FRACUNIT, S_FLICKY_13_HOP},  // S_FLICKY_13_AIM
	{SPR_FL13, 1, 1, {A_FlickyHop},        5*FRACUNIT,       3*FRACUNIT, S_FLICKY_13_UP},   // S_FLICKY_13_HOP
	{SPR_FL13, 2, 2, {A_FlickyCheck}, S_FLICKY_13_AIM, S_FLICKY_13_DOWN, S_FLICKY_13_UP},   // S_FLICKY_13_UP
	{SPR_FL13, 3, 2, {A_FlickyCheck}, S_FLICKY_13_AIM,                0, S_FLICKY_13_DOWN}, // S_FLICKY_13_DOWN
	{SPR_FL13, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_13_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_13, 384*FRACUNIT, S_FLICKY_13_CENTER},        // S_FLICKY_13_CENTER

	// Dove
	{SPR_FL14, 0, 2, {A_FlickyCheck}, S_FLICKY_14_FLAP1, S_FLICKY_14_FLAP1, S_FLICKY_14_OUT},   // S_FLICKY_14_OUT
	{SPR_FL14, 1, 3, {A_FlickySoar},         4*FRACUNIT,       32*FRACUNIT, S_FLICKY_14_FLAP2}, // S_FLICKY_14_FLAP1
	{SPR_FL14, 2, 3, {A_FlickySoar},         4*FRACUNIT,       32*FRACUNIT, S_FLICKY_14_FLAP3}, // S_FLICKY_14_FLAP2
	{SPR_FL14, 3, 3, {A_FlickySoar},         4*FRACUNIT,       32*FRACUNIT, S_FLICKY_14_FLAP1}, // S_FLICKY_14_FLAP3
	{SPR_FL14, FF_ANIMATE|1, -1, {NULL}, 2, 3, S_NULL}, // S_FLICKY_14_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_14, 384*FRACUNIT, S_FLICKY_14_CENTER},        // S_FLICKY_14_CENTER

	// Cat
	{SPR_FL15, 0, 2, {A_FlickyCheck}, S_FLICKY_15_AIM,                0, S_FLICKY_15_OUT},  // S_FLICKY_15_OUT
	{SPR_FL15, 1, 1, {A_FlickyAim},             ANG30,      32*FRACUNIT, S_FLICKY_15_HOP},  // S_FLICKY_15_AIM
	{SPR_FL15, 1, 1, {A_FlickyFlounder},   2*FRACUNIT,       6*FRACUNIT, S_FLICKY_15_UP},   // S_FLICKY_15_HOP
	{SPR_FL15, 2, 2, {A_FlickyCheck}, S_FLICKY_15_AIM, S_FLICKY_15_DOWN, S_FLICKY_15_UP},   // S_FLICKY_15_UP
	{SPR_FL15, 3, 2, {A_FlickyCheck}, S_FLICKY_15_AIM,                0, S_FLICKY_15_DOWN}, // S_FLICKY_15_DOWN
	{SPR_FL15, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_15_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_15, 384*FRACUNIT, S_FLICKY_15_CENTER},        // S_FLICKY_15_CENTER

	// Canary
	{SPR_FL16, 0, 2, {A_FlickyHeightCheck}, S_FLICKY_16_FLAP1,          0, S_FLICKY_16_OUT},   // S_FLICKY_16_OUT
	{SPR_FL16, 1, 3, {A_FlickyFly},                4*FRACUNIT, 8*FRACUNIT, S_FLICKY_16_FLAP2}, // S_FLICKY_16_FLAP1
	{SPR_FL16, 2, 3, {A_SetObjectFlags},         MF_NOGRAVITY,          1, S_FLICKY_16_FLAP3}, // S_FLICKY_16_FLAP2
	{SPR_FL16, 3, 3, {A_FlickyHeightCheck}, S_FLICKY_16_FLAP1,          0, S_FLICKY_16_FLAP3}, // S_FLICKY_16_FLAP3
	{SPR_FL16, FF_ANIMATE|1, -1, {NULL}, 2, 3, S_NULL}, // S_FLICKY_16_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_16, 384*FRACUNIT, S_FLICKY_16_CENTER},        // S_FLICKY_16_CENTER

	// Spider
	{SPR_FS01, 0, 2, {A_FlickyCheck}, S_SECRETFLICKY_01_AIM,                      0, S_SECRETFLICKY_01_OUT},  // S_SECRETFLICKY_01_OUT
	{SPR_FS01, 1, 1, {A_FlickyAim},                   ANG30,            32*FRACUNIT, S_SECRETFLICKY_01_HOP},  // S_SECRETFLICKY_01_AIM
	{SPR_FS01, 1, 1, {A_FlickyFlounder},         2*FRACUNIT,             6*FRACUNIT, S_SECRETFLICKY_01_UP},   // S_SECRETFLICKY_01_HOP
	{SPR_FS01, 2, 2, {A_FlickyCheck}, S_SECRETFLICKY_01_AIM, S_SECRETFLICKY_01_DOWN, S_SECRETFLICKY_01_UP},   // S_SECRETFLICKY_01_UP
	{SPR_FS01, 3, 2, {A_FlickyCheck}, S_SECRETFLICKY_01_AIM,                      0, S_SECRETFLICKY_01_DOWN}, // S_SECRETFLICKY_01_DOWN
	{SPR_FS01, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_SECRETFLICKY_01_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_SECRETFLICKY_01, 384*FRACUNIT, S_SECRETFLICKY_01_CENTER},        // S_SECRETFLICKY_01_CENTER

	// Bat
	{SPR_FS02, 0, 2, {A_FlickyHeightCheck}, S_SECRETFLICKY_02_FLAP1, S_SECRETFLICKY_02_FLAP1, S_SECRETFLICKY_02_OUT},   // S_SECRETFLICKY_02_OUT
	{SPR_FS02, 1, 3, {A_FlickyFly},                      4*FRACUNIT,             16*FRACUNIT, S_SECRETFLICKY_02_FLAP2}, // S_SECRETFLICKY_02_FLAP1
	{SPR_FS02, 2, 3, {A_FlickyFly},                      4*FRACUNIT,             16*FRACUNIT, S_SECRETFLICKY_02_FLAP3}, // S_SECRETFLICKY_02_FLAP2
	{SPR_FS02, 3, 3, {A_FlickyFly},                      4*FRACUNIT,             16*FRACUNIT, S_SECRETFLICKY_02_FLAP1}, // S_SECRETFLICKY_02_FLAP3
	{SPR_FS02, FF_ANIMATE|1, -1, {NULL}, 2, 2, S_NULL}, // S_SECRETFLICKY_02_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_SECRETFLICKY_02, 384*FRACUNIT, S_SECRETFLICKY_02_CENTER},        // S_SECRETFLICKY_02_CENTER

	// Steam Riser
	{SPR_STEM, 0, 2, {A_SetSolidSteam}, 0, 0, S_STEAM2},   // S_STEAM1
	{SPR_STEM, 1, 2, {A_UnsetSolidSteam}, 0, 0, S_STEAM3}, // S_STEAM2
	{SPR_STEM, 2, 2, {NULL}, 0, 0, S_STEAM4},              // S_STEAM3
	{SPR_STEM, 3, 2, {NULL}, 0, 0, S_STEAM5},              // S_STEAM4
	{SPR_STEM, 4, 2, {NULL}, 0, 0, S_STEAM6},              // S_STEAM5
	{SPR_STEM, 5, 2, {NULL}, 0, 0, S_STEAM7},              // S_STEAM6
	{SPR_STEM, 6, 2, {NULL}, 0, 0, S_STEAM8},              // S_STEAM7
	{SPR_NULL, 0, 18, {NULL}, 0, 0, S_STEAM1},             // S_STEAM8

	// Balloons
	{SPR_BLON, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 5, S_BALLOON}, // S_BALLOON
	{SPR_NULL, 0, 2, {NULL},             0, 0, S_BALLOONPOP2}, // S_BALLOONPOP1
	{SPR_BLON, 3, 1, {A_Pain},           0, 0, S_BALLOONPOP3}, // S_BALLOONPOP2
	{SPR_BLON, 4, 1, {NULL},             0, 0, S_BALLOONPOP4}, // S_BALLOONPOP3
	{SPR_NULL, 0, TICRATE, {A_CheckFlags2}, MF2_AMBUSH, S_BALLOONPOP5, S_NULL}, // S_BALLOONPOP4
	{SPR_NULL, 0, 15*TICRATE, {NULL},    0, 0, S_BALLOONPOP6}, // S_BALLOONPOP5
	{SPR_NULL, 0, 0, {A_SpawnFreshCopy}, 0, 0, S_NULL},        // S_BALLOONPOP6

	// Yellow Spring
	{SPR_SPVY, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},           // S_YELLOWSPRING1
	{SPR_SPVY, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_YELLOWSPRING3}, // S_YELLOWSPRING2
	{SPR_SPVY, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_YELLOWSPRING4},   // S_YELLOWSPRING3
	{SPR_SPVY, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_YELLOWSPRING1},   // S_YELLOWSPRING4

	// Red Spring
	{SPR_SPVR, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},        // S_REDSPRING1
	{SPR_SPVR, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_REDSPRING3}, // S_REDSPRING2
	{SPR_SPVR, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_REDSPRING4},   // S_REDSPRING3
	{SPR_SPVR, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_REDSPRING1},   // S_REDSPRING4

	// Blue Spring
	{SPR_SPVB, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},         // S_BLUESPRING1
	{SPR_SPVB, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_BLUESPRING3}, // S_BLUESPRING2
	{SPR_SPVB, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_BLUESPRING4},   // S_BLUESPRING3
	{SPR_SPVB, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_BLUESPRING1},   // S_BLUESPRING4

	// Grey Spring
	{SPR_SPVG, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},         // S_GREYSPRING1
	{SPR_SPVG, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_GREYSPRING3}, // S_GREYSPRING2
	{SPR_SPVG, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_GREYSPRING4},   // S_GREYSPRING3
	{SPR_SPVG, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_GREYSPRING1},   // S_GREYSPRING4

	// Orange Spring (Pogo)
	{SPR_POGS, 0, -1, {NULL}, 0, 0, S_NULL},         // S_POGOSPRING1
	{SPR_POGS, 1, 2, {A_Pain}, 0, 0, S_POGOSPRING3}, // S_POGOSPRING2
	{SPR_POGS, 1, 2, {A_PlaySeeSound}, 0, 0, S_POGOSPRING3}, // S_POGOSPRING2B
	{SPR_POGS, 0, 1, {NULL}, 0, 0, S_POGOSPRING4},   // S_POGOSPRING3
	{SPR_POGS, 2, 4, {NULL}, 0, 0, S_POGOSPRING1},   // S_POGOSPRING4

	// Yellow Diagonal Spring
	{SPR_SPDY, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},    // S_YDIAG1
	{SPR_SPDY, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_YDIAG3}, // S_YDIAG2
	{SPR_SPDY, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_YDIAG4},   // S_YDIAG3
	{SPR_SPDY, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_YDIAG1},   // S_YDIAG4

	// Red Diagonal Spring
	{SPR_SPDR, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},    // S_RDIAG1
	{SPR_SPDR, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_RDIAG3}, // S_RDIAG2
	{SPR_SPDR, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_RDIAG4},   // S_RDIAG3
	{SPR_SPDR, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_RDIAG1},   // S_RDIAG4

	// Blue Diagonal Spring
	{SPR_SPDB, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},    // S_BDIAG1
	{SPR_SPDB, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_BDIAG3}, // S_BDIAG2
	{SPR_SPDB, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_BDIAG4},   // S_BDIAG3
	{SPR_SPDB, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_BDIAG1},   // S_BDIAG4

	// Grey Diagonal Spring
	{SPR_SPDG, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},    // S_GDIAG1
	{SPR_SPDG, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_GDIAG3}, // S_GDIAG2
	{SPR_SPDG, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_GDIAG4},   // S_GDIAG3
	{SPR_SPDG, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_GDIAG1},   // S_GDIAG4

	// Yellow Horizontal Spring
	{SPR_SPHY, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},     // S_YHORIZ1
	{SPR_SPHY, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_YHORIZ3}, // S_YHORIZ2
	{SPR_SPHY, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_YHORIZ4},   // S_YHORIZ3
	{SPR_SPHY, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_YHORIZ1},   // S_YHORIZ4

	// Red Horizontal Spring
	{SPR_SPHR, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},     // S_RHORIZ1
	{SPR_SPHR, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_RHORIZ3}, // S_RHORIZ2
	{SPR_SPHR, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_RHORIZ4},   // S_RHORIZ3
	{SPR_SPHR, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_RHORIZ1},   // S_RHORIZ4

	// Blue Horizontal Spring
	{SPR_SPHB, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},     // S_BHORIZ1
	{SPR_SPHB, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_BHORIZ3}, // S_BHORIZ2
	{SPR_SPHB, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_BHORIZ4},   // S_BHORIZ3
	{SPR_SPHB, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_BHORIZ1},   // S_BHORIZ4

	// Grey Horizontal Spring
	{SPR_SPHG, FF_SEMIBRIGHT|0, -1, {NULL}, 0, 0, S_NULL},     // S_GHORIZ1
	{SPR_SPHG, FF_SEMIBRIGHT|1, 1, {A_Pain}, 0, 0, S_GHORIZ3}, // S_GHORIZ2
	{SPR_SPHG, FF_SEMIBRIGHT|0, 1, {NULL}, 0, 0, S_GHORIZ4},   // S_GHORIZ3
	{SPR_SPHG, FF_SEMIBRIGHT|2, 4, {NULL}, 0, 0, S_GHORIZ1},   // S_GHORIZ4

	// Rain
	{SPR_RAIN, 0, -1, {NULL}, 0, 0, S_NULL}, // S_RAIN1
	{SPR_NULL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_RAINRETURN

	// Snowflake
	{SPR_SNO1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SNOW1
	{SPR_SNO1, 1, -1, {NULL}, 0, 0, S_NULL}, // S_SNOW2
	{SPR_SNO1, 2, -1, {NULL}, 0, 0, S_NULL}, // S_SNOW3

	// Blizzard Snowball
	{SPR_SNO2, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BLIZZARDSNOW1
	{SPR_SNO2, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BLIZZARDSNOW2
	{SPR_SNO2, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BLIZZARDSNOW3

	// Water Splish
	{SPR_SPLH, FF_TRANS50  , 2, {NULL}, 0, 0, S_SPLISH2}, // S_SPLISH1
	{SPR_SPLH, FF_TRANS50|1, 2, {NULL}, 0, 0, S_SPLISH3}, // S_SPLISH2
	{SPR_SPLH, FF_TRANS50|2, 2, {NULL}, 0, 0, S_SPLISH4}, // S_SPLISH3
	{SPR_SPLH, FF_TRANS50|3, 2, {NULL}, 0, 0, S_SPLISH5}, // S_SPLISH4
	{SPR_SPLH, FF_TRANS50|4, 2, {NULL}, 0, 0, S_SPLISH6}, // S_SPLISH5
	{SPR_SPLH, FF_TRANS50|5, 2, {NULL}, 0, 0, S_SPLISH7}, // S_SPLISH6
	{SPR_SPLH, FF_TRANS50|6, 2, {NULL}, 0, 0, S_SPLISH8}, // S_SPLISH7
	{SPR_SPLH, FF_TRANS50|7, 2, {NULL}, 0, 0, S_SPLISH9}, // S_SPLISH8
	{SPR_SPLH, FF_TRANS50|8, 2, {NULL}, 0, 0, S_NULL},    // S_SPLISH9

	// Lava splish
	{SPR_LSPL, FF_ANIMATE, 16, {NULL}, 7, 2, S_NULL}, // S_LAVASPLISH

	// Water Splash
	{SPR_SPLA, 0, 3, {NULL}, 0, 0, S_SPLASH2}, // S_SPLASH1
	{SPR_SPLA, 1, 3, {NULL}, 0, 0, S_SPLASH3}, // S_SPLASH2
	{SPR_SPLA, 2, 3, {NULL}, 0, 0, S_NULL}, // S_SPLASH3

	// Smoke
	{SPR_SMOK, FF_TRANS50  , 4, {NULL}, 0, 0, S_SMOKE2}, // S_SMOKE1
	{SPR_SMOK, FF_TRANS50|1, 5, {NULL}, 0, 0, S_SMOKE3}, // S_SMOKE2
	{SPR_SMOK, FF_TRANS50|2, 6, {NULL}, 0, 0, S_SMOKE4}, // S_SMOKE3
	{SPR_SMOK, FF_TRANS50|3, 7, {NULL}, 0, 0, S_SMOKE5}, // S_SMOKE4
	{SPR_SMOK, FF_TRANS50|4, 8, {NULL}, 0, 0, S_NULL},   // S_SMOKE5

	// Bubbles
	{SPR_BUBL, FF_SEMIBRIGHT,   1, {A_BubbleRise}, 1, 1024, S_SMALLBUBBLE},  // S_SMALLBUBBLE
	{SPR_BUBL, FF_SEMIBRIGHT|1, 1, {A_BubbleRise}, 1, 1024, S_MEDIUMBUBBLE}, // S_MEDIUMBUBBLE

	// Extra Large Bubble (breathable)
	{SPR_BUBL, FF_TRANS50|FF_FULLBRIGHT|2,   8, {A_BubbleRise}, 0, 1024, S_LARGEBUBBLE2}, // S_LARGEBUBBLE1
	{SPR_BUBL, FF_TRANS50|FF_FULLBRIGHT|3,   8, {A_BubbleRise}, 0, 1024, S_EXTRALARGEBUBBLE}, // S_LARGEBUBBLE2
	{SPR_BUBL, FF_TRANS50|FF_FULLBRIGHT|4,  16, {A_BubbleRise}, 0, 1024, S_EXTRALARGEBUBBLE}, // S_EXTRALARGEBUBBLE

	// Extra Large Bubble goes POP!
	{SPR_BUBL, 5, 16, {NULL}, 0, 0, S_NULL}, // S_POP1

	{SPR_WZAP, FF_TRANS10|FF_ANIMATE|FF_RANDOMANIM, 4, {NULL}, 3, 2, S_NULL},  // S_WATERZAP

	// Spindash dust
	{SPR_DUST,                          0, 7, {NULL}, 0, 0, S_SPINDUST2}, // S_SPINDUST1
	{SPR_DUST,                          1, 6, {NULL}, 0, 0, S_SPINDUST3}, // S_SPINDUST2
	{SPR_DUST,               FF_TRANS30|2, 4, {NULL}, 0, 0, S_SPINDUST4}, // S_SPINDUST3
	{SPR_DUST,               FF_TRANS60|3, 3, {NULL}, 0, 0, S_NULL}, // S_SPINDUST4
	{SPR_BUBL,                          0, 7, {NULL}, 0, 0, S_SPINDUST_BUBBLE2}, // S_SPINDUST_BUBBLE1
	{SPR_BUBL,                          0, 6, {NULL}, 0, 0, S_SPINDUST_BUBBLE3}, // S_SPINDUST_BUBBLE2
	{SPR_BUBL,               FF_TRANS30|0, 4, {NULL}, 0, 0, S_SPINDUST_BUBBLE4}, // S_SPINDUST_BUBBLE3
	{SPR_BUBL,               FF_TRANS60|0, 3, {NULL}, 0, 0, S_NULL}, // S_SPINDUST_BUBBLE4
	{SPR_FPRT,            FF_FULLBRIGHT|0, 7, {NULL}, 0, 0, S_SPINDUST_FIRE2}, // S_SPINDUST_FIRE1
	{SPR_FPRT,            FF_FULLBRIGHT|0, 6, {NULL}, 0, 0, S_SPINDUST_FIRE3}, // S_SPINDUST_FIRE2
	{SPR_FPRT, FF_FULLBRIGHT|FF_TRANS30|0, 4, {NULL}, 0, 0, S_SPINDUST_FIRE4}, // S_SPINDUST_FIRE3
	{SPR_FPRT, FF_FULLBRIGHT|FF_TRANS60|0, 3, {NULL}, 0, 0, S_NULL}, // S_SPINDUST_FIRE4

	// Flower Seed
	{SPR_SEED, FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 2, 2, S_NULL}, // S_SEED

	// Particle sprite
	{SPR_PRTL, 0, 2*TICRATE, {NULL}, 0, 0, S_NULL}, // S_PARTICLE

	// Drowning Timer Numbers
	{SPR_DRWN, 0, 40, {NULL}, 0, 0, S_NULL}, // S_ZERO1
	{SPR_DRWN, 1, 40, {NULL}, 0, 0, S_NULL}, // S_ONE1
	{SPR_DRWN, 2, 40, {NULL}, 0, 0, S_NULL}, // S_TWO1
	{SPR_DRWN, 3, 40, {NULL}, 0, 0, S_NULL}, // S_THREE1
	{SPR_DRWN, 4, 40, {NULL}, 0, 0, S_NULL}, // S_FOUR1
	{SPR_DRWN, 5, 40, {NULL}, 0, 0, S_NULL}, // S_FIVE1

	{SPR_DRWN,  6, 40, {NULL}, 0, 0, S_NULL}, // S_ZERO2
	{SPR_DRWN,  7, 40, {NULL}, 0, 0, S_NULL}, // S_ONE2
	{SPR_DRWN,  8, 40, {NULL}, 0, 0, S_NULL}, // S_TWO2
	{SPR_DRWN,  9, 40, {NULL}, 0, 0, S_NULL}, // S_THREE2
	{SPR_DRWN, 10, 40, {NULL}, 0, 0, S_NULL}, // S_FOUR2
	{SPR_DRWN, 11, 40, {NULL}, 0, 0, S_NULL}, // S_FIVE2

	{SPR_CORK,             0, -1, {NULL}, 0, 0, S_NULL}, // S_CORK
	{SPR_LHRT, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_LHRT

	{SPR_NULL, 0, 1, {A_RingExplode}, 0, 0, S_XPLD1}, // S_RINGEXPLODE

	{SPR_HOOP, 0, -1, {NULL}, 0, 0, S_NULL}, // S_HOOP
	{SPR_HOOP, 1, -1, {NULL}, 0, 0, S_NULL}, // S_HOOP_XMASA
	{SPR_HOOP, 2, -1, {NULL}, 0, 0, S_NULL}, // S_HOOP_XMASB

	{SPR_CAPS, 0, -1, {NULL}, 0, 0, S_NULL}, // S_EGGCAPSULE

	// Secret badniks and hazards, shhhh
	{SPR_FMCE, 0, 20, {NULL}, 0, 0, S_SMASHSPIKE_EASE1}, // S_SMASHSPIKE_FLOAT
	{SPR_FMCE, 0,  4, {A_ZThrust},  4, (1<<16)|1, S_SMASHSPIKE_EASE2}, // S_SMASHSPIKE_EASE1
	{SPR_FMCE, 0,  4, {A_ZThrust},  0, (1<<16)|1, S_SMASHSPIKE_FALL},  // S_SMASHSPIKE_EASE2
	{SPR_FMCE, 0,  2, {A_ZThrust}, -6,         1, S_SMASHSPIKE_FALL},  // S_SMASHSPIKE_FALL
	{SPR_FMCE, 1,  2, {A_MultiShotDist}, (MT_DUST<<16)|10, -48, S_SMASHSPIKE_STOMP2}, // S_SMASHSPIKE_STOMP1
	{SPR_FMCE, 2, 14, {NULL}, 0, 0, S_SMASHSPIKE_RISE1}, // S_SMASHSPIKE_STOMP2
	{SPR_FMCE, 1,  2, {NULL}, 0, 0, S_SMASHSPIKE_RISE2}, // S_SMASHSPIKE_RISE1
	{SPR_FMCE, 0,  2, {A_ZThrust}, 6, (1<<16)|1, S_SMASHSPIKE_RISE2}, // S_SMASHSPIKE_RISE2

	{SPR_NULL, 0,  35, {NULL}, 0, 0, S_CRUMBLE2}, // S_CRUMBLE1
	{SPR_NULL, 0, 105, {A_Scream}, 0, 0, S_NULL}, // S_CRUMBLE2

	// Spark
	{SPR_NULL, 0, 1, {A_ModuloToState}, 2, S_SPRK2, S_SPRK3},  // S_SPRK1
	{SPR_SPRK, FF_TRANS20|FF_ANIMATE|0, 18, {NULL}, 8, 2, S_NULL},  // S_SPRK2
	{SPR_SPRK, FF_TRANS20|FF_ANIMATE|9, 18, {NULL}, 8, 2, S_NULL},  // S_SPRK3

	// Robot Explosion
	{SPR_BOM1, 0, 0, {A_FlickySpawn}, 0, 0, S_XPLD1}, // S_XPLD_FLICKY
	{SPR_BOM1, 0, 2, {A_Scream},      0, 0, S_XPLD2}, // S_XPLD1
	{SPR_BOM1, 1, 2, {NULL},          0, 0, S_XPLD3}, // S_XPLD2
	{SPR_BOM1, 2, 3, {NULL},          0, 0, S_XPLD4}, // S_XPLD3
	{SPR_BOM1, 3, 3, {NULL},          0, 0, S_XPLD5}, // S_XPLD4
	{SPR_BOM1, 4, 4, {NULL},          0, 0, S_XPLD6}, // S_XPLD5
	{SPR_BOM1, 5, 4, {NULL},          0, 0, S_NULL},  // S_XPLD6

	{SPR_BOM1, FF_ANIMATE,   21, {NULL},          5, 4, S_INVISIBLE}, // S_XPLD_EGGTRAP

	// Underwater Explosion
	{SPR_BOM4, 0, 3, {A_Scream}, 0, 0, S_WPLD2}, // S_WPLD1
	{SPR_BOM4, 1, 3, {NULL},     0, 0, S_WPLD3}, // S_WPLD2
	{SPR_BOM4, 2, 3, {NULL},     0, 0, S_WPLD4}, // S_WPLD3
	{SPR_BOM4, 3, 3, {NULL},     0, 0, S_WPLD5}, // S_WPLD4
	{SPR_BOM4, 4, 3, {NULL},     0, 0, S_WPLD6}, // S_WPLD5
	{SPR_BOM4, 5, 3, {NULL},     0, 0, S_NULL},  // S_WPLD6

	{SPR_DUST,   FF_TRANS40, 4, {NULL}, 0, 0, S_DUST2}, // S_DUST1
	{SPR_DUST, 1|FF_TRANS50, 5, {NULL}, 0, 0, S_DUST3}, // S_DUST2
	{SPR_DUST, 2|FF_TRANS60, 3, {NULL}, 0, 0, S_DUST4}, // S_DUST3
	{SPR_DUST, 3|FF_TRANS70, 2, {NULL}, 0, 0, S_NULL},  // S_DUST4

	{SPR_NULL, 0, 1, {A_RockSpawn}, 0, 0, S_ROCKSPAWN}, // S_ROCKSPAWN

	{SPR_ROIA, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 4, 2, S_NULL}, // S_ROCKCRUMBLEA
	{SPR_ROIB, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEB
	{SPR_ROIC, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEC
	{SPR_ROID, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLED
	{SPR_ROIE, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEE
	{SPR_ROIF, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEF
	{SPR_ROIG, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 4, 2, S_NULL}, // S_ROCKCRUMBLEG
	{SPR_ROIH, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 4, 2, S_NULL}, // S_ROCKCRUMBLEH
	{SPR_ROII, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEI
	{SPR_ROIJ, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 4, 2, S_NULL}, // S_ROCKCRUMBLEJ
	{SPR_ROIK, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 4, 2, S_NULL}, // S_ROCKCRUMBLEK
	{SPR_ROIL, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEL
	{SPR_ROIM, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEM
	{SPR_ROIN, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEN
	{SPR_ROIO, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEO
	{SPR_ROIP, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEP

	{SPR_GFZD, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 31, 1, S_NULL}, // S_GFZDEBRIS
	{SPR_BRIC, FF_ANIMATE, -1, {A_DebrisRandom}, 7, 2, S_NULL}, // S_BRICKDEBRIS
	{SPR_WDDB, FF_ANIMATE, -1, {A_DebrisRandom}, 7, 2, S_NULL}, // S_WOODDEBRIS

	// SRB2kart
	{SPR_RNDM,    FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM2},	// S_RANDOMITEM1
	{SPR_RNDM,  2|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM3},	// S_RANDOMITEM2
	{SPR_RNDM,  4|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM4},	// S_RANDOMITEM3
	{SPR_RNDM,  6|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM5},	// S_RANDOMITEM4
	{SPR_RNDM,  8|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM6},	// S_RANDOMITEM5
	{SPR_RNDM, 10|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM7},	// S_RANDOMITEM6
	{SPR_RNDM, 12|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM8},	// S_RANDOMITEM7
	{SPR_RNDM, 14|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM9},	// S_RANDOMITEM8
	{SPR_RNDM, 16|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM10},	// S_RANDOMITEM9
	{SPR_RNDM, 18|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM11},	// S_RANDOMITEM10
	{SPR_RNDM, 20|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM12},	// S_RANDOMITEM11
	{SPR_RNDM, 22|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RANDOMITEM1},	// S_RANDOMITEM12

	{SPR_RBOX,    FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX2},		// S_RINGBOX1
	{SPR_RBOX,  2|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX3},		// S_RINGBOX2
	{SPR_RBOX,  4|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX4},		// S_RINGBOX3
	{SPR_RBOX,  6|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX5},		// S_RINGBOX4
	{SPR_RBOX,  8|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX6},		// S_RINGBOX5
	{SPR_RBOX, 10|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX7},		// S_RINGBOX6
	{SPR_RBOX, 12|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX8},		// S_RINGBOX7
	{SPR_RBOX, 14|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX9},		// S_RINGBOX8
	{SPR_RBOX, 16|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX10},	// S_RINGBOX9
	{SPR_RBOX, 18|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX11},	// S_RINGBOX10
	{SPR_RBOX, 20|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX12},	// S_RINGBOX11
	{SPR_RBOX, 22|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_RINGBOX1},		// S_RINGBOX12

	{SPR_SBOX,    FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX2},	// S_SPHEREBOX1
	{SPR_SBOX,  2|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX3},	// S_SPHEREBOX2
	{SPR_SBOX,  4|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX4},	// S_SPHEREBOX3
	{SPR_SBOX,  6|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX5},	// S_SPHEREBOX4
	{SPR_SBOX,  8|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX6},	// S_SPHEREBOX5
	{SPR_SBOX, 10|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX7},	// S_SPHEREBOX6
	{SPR_SBOX, 12|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX8},	// S_SPHEREBOX7
	{SPR_SBOX, 14|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX9},	// S_SPHEREBOX8
	{SPR_SBOX, 16|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX10},	// S_SPHEREBOX9
	{SPR_SBOX, 18|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX11},	// S_SPHEREBOX10
	{SPR_SBOX, 20|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX12},	// S_SPHEREBOX11
	{SPR_SBOX, 22|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_SPHEREBOX1},	// S_SPHEREBOX12

	{SPR_ITRI, FF_FULLBRIGHT|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 19, 1, S_NULL}, // S_ITEM_DEBRIS
	{SPR_NULL, 0, 0, {A_Repeat}, 16, S_ITEM_DEBRIS_CLOUD_SPAWNER2, S_NULL}, // S_ITEM_DEBRIS_CLOUD_SPAWNER1
	{SPR_NULL, 0, 7, {A_SpawnItemDebrisCloud}, 20, 0, S_ITEM_DEBRIS_CLOUD_SPAWNER1}, // S_ITEM_DEBRIS_CLOUD_SPAWNER2

	{SPR_NULL, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_ITEMICON
	{SPR_ITPA, FF_FULLBRIGHT, -1, {NULL}, 1, 0, S_NULL}, // S_ITEMBACKDROP

	{SPR_ICAP,                         FF_ADD|0, -1, {NULL}, 0, 0, S_NULL}, // S_ITEMCAPSULE
	{SPR_ICAP,                 FF_PAPERSPRITE|1, -1, {NULL}, 0, 0, S_NULL}, // S_ITEMCAPSULE_TOP_SIDE
	{SPR_ICAP, FF_VERTICALFLIP|FF_PAPERSPRITE|1, -1, {NULL}, 0, 0, S_NULL}, // S_ITEMCAPSULE_BOTTOM_SIDE_AIR
	{SPR_ICAP,                 FF_PAPERSPRITE|2, -1, {NULL}, 0, 0, S_NULL}, // S_ITEMCAPSULE_BOTTOM_SIDE_GROUND
	//{SPR_ICAP,                 FF_FLOORSPRITE|3, -1, {NULL}, 0, 0, S_NULL}, // S_ITEMCAPSULE_TOP
	//{SPR_ICAP,                 FF_FLOORSPRITE|4, -1, {NULL}, 0, 0, S_NULL}, // S_ITEMCAPSULE_BOTTOM
	//{SPR_ICAP,                 FF_FLOORSPRITE|5, -1, {NULL}, 0, 0, S_NULL}, // S_ITEMCAPSULE_INSIDE

	{SPR_NULL, 0, 1, {NULL}, 6, 1, S_SPAWNSTATE}, // S_MONITOR_DAMAGE
	{SPR_NULL, 0, 50, {NULL}, 0, 0, S_NULL}, // S_MONITOR_DEATH
	{SPR_IMON, FF_PAPERSPRITE|1, 1, {NULL}, 3, 1, S_MONITOR_SCREEN1B}, // S_MONITOR_SCREEN1A
	{SPR_IMON, FF_PAPERSPRITE|0, 1, {NULL}, 3, 1, S_MONITOR_SCREEN2A}, // S_MONITOR_SCREEN1B
	{SPR_IMON, FF_PAPERSPRITE|2, 1, {NULL}, 3, 1, S_MONITOR_SCREEN2B}, // S_MONITOR_SCREEN2A
	{SPR_IMON, FF_PAPERSPRITE|0, 1, {NULL}, 3, 1, S_MONITOR_SCREEN3A}, // S_MONITOR_SCREEN2B
	{SPR_IMON, FF_PAPERSPRITE|3, 1, {NULL}, 3, 1, S_MONITOR_SCREEN3B}, // S_MONITOR_SCREEN3A
	{SPR_IMON, FF_PAPERSPRITE|0, 1, {NULL}, 3, 1, S_MONITOR_SCREEN4A}, // S_MONITOR_SCREEN3B
	{SPR_IMON, FF_PAPERSPRITE|4, 1, {NULL}, 3, 1, S_MONITOR_SCREEN4B}, // S_MONITOR_SCREEN4A
	{SPR_IMON, FF_PAPERSPRITE|0, 1, {NULL}, 3, 1, S_MONITOR_SCREEN1A}, // S_MONITOR_SCREEN4B
	{SPR_IMON, FF_PAPERSPRITE|5, -1, {NULL}, 3, 1, S_NULL}, // S_MONITOR_STAND
	{SPR_NULL, FF_PAPERSPRITE|FF_TRANS50|FF_ADD|6, -1, {NULL}, 3, 35, S_NULL}, // S_MONITOR_CRACKA
	{SPR_NULL, FF_PAPERSPRITE|FF_TRANS50|FF_SUBTRACT|10, -1, {NULL}, 3, 35, S_NULL}, // S_MONITOR_CRACKB

	{SPR_MSHD, FF_FULLBRIGHT|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_MONITOR_BIG_SHARD
	{SPR_IMDB, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_MONITOR_SMALL_SHARD
	{SPR_MTWK, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_MONITOR_TWINKLE

	{SPR_MGBX,                 FF_PAPERSPRITE|0, -1, {NULL}, 0, 0, S_NULL}, // S_MAGICIANBOX
	{SPR_MGBT,                 FF_FLOORSPRITE|0, -1, {NULL}, 0, 0, S_NULL}, // S_MAGICIANBOX_TOP
	{SPR_MGBB,                 FF_FLOORSPRITE|0, -1, {NULL}, 0, 0, S_NULL}, // S_MAGICIANBOX_BOTTOM

	{SPR_SSMA,                 FF_PAPERSPRITE|FF_ANIMATE|FF_ADD, -1, {NULL}, 59, 1, S_NULL}, // S_MINERADIUS

	{SPR_SLPT,                 FF_PAPERSPRITE|0, -1, {NULL}, 0, 0, S_NULL}, // S_WAVEDASH

	{SPR_IWHP,                 FF_FLOORSPRITE|FF_ANIMATE|0, -1, {NULL}, 6, 2, S_NULL}, // S_INSTAWHIP
	{SPR_NULL,                 0, 1, {NULL}, 0, 0, S_INSTAWHIP_RECHARGE2}, // S_INSTAWHIP_RECHARGE1
	{SPR_NULL,                 0, 0, {A_PlaySound}, sfx_s3ka0, 2, S_INSTAWHIP_RECHARGE3}, // S_INSTAWHIP_RECHARGE2
	{SPR_WPRE,                 FF_FULLBRIGHT|FF_FLOORSPRITE|FF_ANIMATE|0, 36, {NULL}, 17, 2, S_INSTAWHIP_RECHARGE4}, // S_INSTAWHIP_RECHARGE3
	{SPR_NULL,                 0, 0, {A_PlaySound}, sfx_s3k7c, 2, S_NULL}, // S_INSTAWHIP_RECHARGE4
	{SPR_WPRJ,                 FF_FULLBRIGHT|FF_ANIMATE, 9, {NULL}, 8, 1, S_INSTAWHIP_REJECT}, // S_INSTAWHIP_REJECT
	{SPR_GRNG,                 FF_FULLBRIGHT|FF_PAPERSPRITE|0, -1, {NULL}, 0, 0, S_NULL}, // S_BLOCKRING
	{SPR_GBDY,                 FF_FULLBRIGHT|FF_ANIMATE|0, -1, {NULL}, 4, 2, S_NULL}, // S_BLOCKBODY

	// why can we not use actions on spawn? I'd love to fix it but I imagine all sorts of crazy pain if I change something fundamental like that
	{SPR_BAIL,                 FF_FULLBRIGHT|FF_ANIMATE|0, 9, {NULL}, 8, 1, S_BAIB1}, // S_BAIL
	{SPR_BAIB,                 0, 0, {A_PlaySound}, sfx_gshb2, 2, S_BAIB2}, // S_BAIB1
	{SPR_BAIB,                 0, 0, {A_PlaySound}, sfx_gshbd, 2, S_BAIB3}, // S_BAIB2
	{SPR_BAIB,                 FF_FULLBRIGHT|FF_ANIMATE|0, 10, {NULL}, 9, 1, S_NULL}, // S_BAIB3

	{SPR_BAIC,                 FF_FULLBRIGHT|FF_ANIMATE|0, 11, {NULL}, 10, 1, S_NULL}, // S_BAIC

	{SPR_TECH,                 1, -1, {NULL}, 41, 1, S_NULL}, // S_BAILCHARGE

	{SPR_AMPB,                 FF_FULLBRIGHT|FF_PAPERSPRITE|0, -1, {NULL}, 0, 0, S_NULL}, // S_AMPRING
	{SPR_AMPC,                 FF_FULLBRIGHT|FF_ANIMATE|0, -1, {NULL}, 4, 2, S_NULL}, // S_AMPBODY
	{SPR_AMPD,                 FF_FULLBRIGHT|FF_ANIMATE|0, -1, {NULL}, 4, 2, S_NULL}, // S_AMPAURA
	{SPR_AMPB,                 FF_FULLBRIGHT|FF_ADD|FF_PAPERSPRITE|2, -1, {NULL}, 4, 2, S_NULL}, // S_AMPBURST

	{SPR_TWBB,                 FF_ADD|FF_PAPERSPRITE|0, -1, {NULL}, 0, 0, S_NULL}, // S_SONICBOOM
	{SPR_TWOK,                 FF_FULLBRIGHT|FF_ANIMATE|0, 56, {NULL}, 55, 1, S_NULL}, // S_TRIPWIREOK
	{SPR_TW_L,                 FF_FULLBRIGHT|FF_ANIMATE|0, 56, {NULL}, 55, 1, S_NULL}, // S_TRIPWIRELOCKOUT

	{SPR_SOR_,                 FF_FULLBRIGHT|FF_ANIMATE|0, 28, {NULL}, 27, 1, S_NULL}, // S_GOTIT

	{SPR_TRC1,                 FF_FULLBRIGHT|FF_ANIMATE|0, -1, {NULL}, 4, 2, S_NULL}, // S_CHARGEAURA
	{SPR_TRC2,                 FF_FULLBRIGHT|FF_ANIMATE|0, 20, {NULL}, 19, 1, S_NULL}, // S_CHARGEFALL
	{SPR_TRC3,                 FF_FULLBRIGHT|FF_ADD|0, 2, {NULL}, 0, 0, S_NULL}, // S_CHARGEFLICKER
	{SPR_TRC3,                 FF_FULLBRIGHT|FF_ADD|1, 2, {NULL}, 0, 0, S_NULL}, // S_CHARGESPARK
	{SPR_TRC4,                 FF_FULLBRIGHT|FF_ADD|FF_ANIMATE|0, -1, {NULL}, 4, 1, S_NULL}, // S_CHARGERELEASE
	{SPR_TRC5,                 FF_FULLBRIGHT|FF_ADD|FF_ANIMATE|0, -1, {NULL}, 4, 1, S_NULL}, // S_CHARGEEXTRA

	{SPR_DHND, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_SERVANTHAND

	{SPR_HORN, 0, -1, {NULL}, 0, 0, S_NULL}, // S_HORNCODE

	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_SIGNSPARK2}, // S_SIGNSPARK1
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_SIGNSPARK3}, // S_SIGNSPARK2
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_SIGNSPARK4}, // S_SIGNSPARK3
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_SIGNSPARK5}, // S_SIGNSPARK4
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_SIGNSPARK6}, // S_SIGNSPARK5
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|5, 1, {NULL}, 0, 0, S_SIGNSPARK7}, // S_SIGNSPARK6
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|6, 1, {NULL}, 0, 0, S_SIGNSPARK8}, // S_SIGNSPARK7
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|7, 1, {NULL}, 0, 0, S_SIGNSPARK9}, // S_SIGNSPARK8
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|8, 1, {NULL}, 0, 0, S_SIGNSPARK10}, // S_SIGNSPARK9
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_SIGNSPARK11}, // S_SIGNSPARK10
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_NULL}, // S_SIGNSPARK11

	{SPR_DRIF, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_DRIFTSPARK_A2}, // S_DRIFTSPARK_A1
	{SPR_DRIF, FF_FULLBRIGHT|FF_TRANS20|1, 1, {NULL}, 0, 0, S_DRIFTSPARK_A3}, // S_DRIFTSPARK_A2
	{SPR_DRIF, FF_FULLBRIGHT|FF_TRANS50,   1, {NULL}, 0, 0, S_NULL}, // S_DRIFTSPARK_A3

	{SPR_DRIF, FF_FULLBRIGHT|1, 2, {NULL}, 0, 0, S_DRIFTSPARK_A2}, // S_DRIFTSPARK_B1 (Loop back to A2)

	{SPR_DRIF, FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_DRIFTSPARK_C2}, // S_DRIFTSPARK_C1
	{SPR_DRIF, FF_FULLBRIGHT|FF_TRANS20, 1, {NULL}, 0, 0, S_DRIFTSPARK_A3}, // S_DRIFTSPARK_C2 (Loop back to A3)

	{SPR_DRIF, FF_FULLBRIGHT|3, 2, {NULL}, 0, 0, S_DRIFTSPARK_D2}, // S_DRIFTSPARK_D1
	{SPR_DRIF, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_DRIFTSPARK_A2}, // S_DRIFTSPARK_D2 (Loop back to A2)

	{SPR_BDRF, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, -1, {NULL}, 5, 2, S_BRAKEDRIFT}, // S_BRAKEDRIFT

	{SPR_BRAK, 0, 1, {NULL}, 0, 0, S_BRAKEDUST2}, // S_BRAKEDUST1
	{SPR_BRAK, FF_ANIMATE, 7, {NULL}, 6, 1, S_NULL}, // S_BRAKEDUST2

	{SPR_DUST, 0,  3, {NULL}, 0, 0, S_DRIFTDUST2}, // S_DRIFTDUST1
	{SPR_DUST, 1,  3, {NULL}, 0, 0, S_DRIFTDUST3}, // S_DRIFTDUST2
	{SPR_DUST, FF_TRANS20|2,  3, {NULL}, 0, 0, S_DRIFTDUST4}, // S_DRIFTDUST3
	{SPR_DUST, FF_TRANS20|3,  3, {NULL}, 0, 0, S_NULL}, // S_DRIFTDUST4

	{SPR_DRWS, FF_FULLBRIGHT|0,  3, {NULL}, 0, 0, S_DRIFTWARNSPARK2}, // S_DRIFTWARNSPARK1
	{SPR_DRWS, FF_FULLBRIGHT|1,  3, {NULL}, 0, 0, S_DRIFTWARNSPARK3}, // S_DRIFTWARNSPARK2
	{SPR_DRWS, FF_FULLBRIGHT|FF_TRANS20|2,  3, {NULL}, 0, 0, S_DRIFTWARNSPARK4}, // S_DRIFTWARNSPARK3
	{SPR_DRWS, FF_FULLBRIGHT|FF_TRANS20|3,  3, {NULL}, 0, 0, S_NULL}, // S_DRIFTWARNSPARK4

	{SPR_DREL, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE|FF_GLOBALANIM, 2, {NULL}, 5, 2, S_NULL}, // S_DRIFTELECTRICITY
	{SPR_DRES, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 20, {NULL}, 1, 10, S_NULL}, // S_DRIFTELECTRICSPARK

	{SPR_FAST, FF_PAPERSPRITE|FF_FULLBRIGHT,   1, {NULL}, 0, 0, S_FASTLINE2}, // S_FASTLINE1
	{SPR_FAST, FF_PAPERSPRITE|FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_FASTLINE3}, // S_FASTLINE2
	{SPR_FAST, FF_PAPERSPRITE|FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_FASTLINE4}, // S_FASTLINE3
	{SPR_FAST, FF_PAPERSPRITE|FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_FASTLINE5}, // S_FASTLINE4
	{SPR_FAST, FF_PAPERSPRITE|FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_NULL}, // S_FASTLINE5

	{SPR_DSHR, FF_PAPERSPRITE,   1, {NULL}, 0, 0, S_FASTDUST2}, // S_FASTDUST1
	{SPR_DSHR, FF_PAPERSPRITE|1, 1, {NULL}, 0, 0, S_FASTDUST3}, // S_FASTDUST2
	{SPR_DSHR, FF_PAPERSPRITE|2, 1, {NULL}, 0, 0, S_FASTDUST4}, // S_FASTDUST3
	{SPR_DSHR, FF_PAPERSPRITE|3, 1, {NULL}, 0, 0, S_FASTDUST5}, // S_FASTDUST4
	{SPR_DSHR, FF_PAPERSPRITE|4, 1, {NULL}, 0, 0, S_FASTDUST6}, // S_FASTDUST5
	{SPR_DSHR, FF_PAPERSPRITE|5, 1, {NULL}, 0, 0, S_FASTDUST7}, // S_FASTDUST6
	{SPR_DSHR, FF_PAPERSPRITE|6, 1, {NULL}, 0, 0, S_NULL}, // S_FASTDUST7

	{SPR_DBOS,                FF_FULLBRIGHT,   1, {NULL}, 6, 1, S_DRIFTEXPLODE2}, // S_DRIFTEXPLODE1
	{SPR_DBST, FF_PAPERSPRITE|FF_FULLBRIGHT,   1, {NULL}, 6, 1, S_DRIFTEXPLODE3}, // S_DRIFTEXPLODE2
	{SPR_DBOS,                FF_FULLBRIGHT|1, 1, {NULL}, 6, 1, S_DRIFTEXPLODE4}, // S_DRIFTEXPLODE3
	{SPR_DBST, FF_PAPERSPRITE|FF_FULLBRIGHT|1, 1, {NULL}, 6, 1, S_DRIFTEXPLODE5}, // S_DRIFTEXPLODE4
	{SPR_DBOS,                FF_FULLBRIGHT|2, 1, {NULL}, 6, 1, S_DRIFTEXPLODE6}, // S_DRIFTEXPLODE5
	{SPR_DBST, FF_PAPERSPRITE|FF_FULLBRIGHT|2, 1, {NULL}, 6, 1, S_DRIFTEXPLODE7}, // S_DRIFTEXPLODE6
	{SPR_DBOS,                FF_FULLBRIGHT|3, 1, {NULL}, 6, 1, S_DRIFTEXPLODE8}, // S_DRIFTEXPLODE7
	{SPR_DBST, FF_PAPERSPRITE|FF_FULLBRIGHT|3, 1, {NULL}, 6, 1, S_DRIFTEXPLODE1}, // S_DRIFTEXPLODE8

	{SPR_DBCL, FF_FULLBRIGHT|0x0, 1, {NULL}, 6, 1, S_DRIFTCLIPA2},  // S_DRIFTCLIPA1
	{SPR_DBCL, FF_FULLBRIGHT|0x8, 1, {NULL}, 6, 1, S_DRIFTCLIPA3},  // S_DRIFTCLIPA2
	{SPR_DBCL, FF_FULLBRIGHT|0x1, 1, {NULL}, 6, 1, S_DRIFTCLIPA4},  // S_DRIFTCLIPA3
	{SPR_DBCL, FF_FULLBRIGHT|0x9, 1, {NULL}, 6, 1, S_DRIFTCLIPA5},  // S_DRIFTCLIPA4
	{SPR_DBCL, FF_FULLBRIGHT|0x2, 1, {NULL}, 6, 1, S_DRIFTCLIPA6},  // S_DRIFTCLIPA5
	{SPR_DBCL, FF_FULLBRIGHT|0xA, 1, {NULL}, 6, 1, S_DRIFTCLIPA7},  // S_DRIFTCLIPA6
	{SPR_DBCL, FF_FULLBRIGHT|0x3, 1, {NULL}, 6, 1, S_DRIFTCLIPA8},  // S_DRIFTCLIPA7
	{SPR_DBCL, FF_FULLBRIGHT|0xB, 1, {NULL}, 6, 1, S_DRIFTCLIPA9},  // S_DRIFTCLIPA8
	{SPR_DBCL, FF_FULLBRIGHT|0x4, 1, {NULL}, 6, 1, S_DRIFTCLIPA10}, // S_DRIFTCLIPA9
	{SPR_DBCL, FF_FULLBRIGHT|0xC, 1, {NULL}, 6, 1, S_DRIFTCLIPA11}, // S_DRIFTCLIPA10
	{SPR_DBCL, FF_FULLBRIGHT|0x5, 1, {NULL}, 6, 1, S_DRIFTCLIPA12}, // S_DRIFTCLIPA11
	{SPR_DBCL, FF_FULLBRIGHT|0xD, 1, {NULL}, 6, 1, S_DRIFTCLIPA13}, // S_DRIFTCLIPA12
	{SPR_DBCL, FF_FULLBRIGHT|0x6, 1, {NULL}, 6, 1, S_DRIFTCLIPA14}, // S_DRIFTCLIPA13
	{SPR_DBCL, FF_FULLBRIGHT|0xE, 1, {NULL}, 6, 1, S_DRIFTCLIPA15}, // S_DRIFTCLIPA14
	{SPR_DBCL, FF_FULLBRIGHT|0x7, 1, {NULL}, 6, 1, S_DRIFTCLIPA16}, // S_DRIFTCLIPA15
	{SPR_DBCL, FF_FULLBRIGHT|0xF, 1, {NULL}, 6, 1, S_DRIFTCLIPB1},  // S_DRIFTCLIPA16

	{SPR_DBCL, FF_FULLBRIGHT,   2, {NULL}, 6, 1, S_DRIFTCLIPB2}, // S_DRIFTCLIPB1
	{SPR_DBCL, FF_FULLBRIGHT|1, 2, {NULL}, 6, 1, S_DRIFTCLIPB3}, // S_DRIFTCLIPB2
	{SPR_DBCL, FF_FULLBRIGHT|2, 2, {NULL}, 6, 1, S_DRIFTCLIPB4}, // S_DRIFTCLIPB3
	{SPR_DBCL, FF_FULLBRIGHT|3, 2, {NULL}, 6, 1, S_DRIFTCLIPB5}, // S_DRIFTCLIPB4
	{SPR_DBCL, FF_FULLBRIGHT|4, 2, {NULL}, 6, 1, S_DRIFTCLIPB6}, // S_DRIFTCLIPB5
	{SPR_DBCL, FF_FULLBRIGHT|5, 2, {NULL}, 6, 1, S_DRIFTCLIPB7}, // S_DRIFTCLIPB6
	{SPR_DBCL, FF_FULLBRIGHT|6, 2, {NULL}, 6, 1, S_DRIFTCLIPB8}, // S_DRIFTCLIPB7
	{SPR_DBCL, FF_FULLBRIGHT|7, 2, {NULL}, 6, 1, S_DRIFTCLIPB1}, // S_DRIFTCLIPB8

	{SPR_DBNC, FF_FULLBRIGHT|FF_ANIMATE, 14, {NULL}, 6, 1, S_NULL}, // S_DRIFTCLIPSPARK

	{SPR_BOST, FF_FULLBRIGHT|FF_ADD|FF_ANIMATE, TICRATE, {NULL}, 7, 1, S_BOOSTSMOKESPAWNER}, // S_BOOSTFLAME
	{SPR_NULL, 0,                      TICRATE/2, {NULL}, 0, 0, S_NULL}, // S_BOOSTSMOKESPAWNER

	{SPR_BOSM, FF_TRANS50, 3, {NULL}, 0, 0, S_BOOSTSMOKE2}, // S_BOOSTSMOKE1
	{SPR_BOSM, FF_TRANS50|1, 3, {NULL}, 0, 0, S_BOOSTSMOKE3}, // S_BOOSTSMOKE2
	{SPR_BOSM, FF_TRANS50|2, 3, {NULL}, 0, 0, S_BOOSTSMOKE4}, // S_BOOSTSMOKE3
	{SPR_BOSM, FF_TRANS50|3, 3, {NULL}, 0, 0, S_BOOSTSMOKE5}, // S_BOOSTSMOKE4
	{SPR_BOSM, FF_TRANS50|4, 3, {NULL}, 0, 0, S_BOOSTSMOKE6}, // S_BOOSTSMOKE5
	{SPR_BOSM, FF_TRANS50|5, 3, {NULL}, 0, 0, S_NULL}, // S_BOOSTSMOKE6

	{SPR_NULL, 0,               10, {NULL}, 0, 0, S_KARTFIRE2}, // S_KARTFIRE1
	{SPR_KFRE, FF_FULLBRIGHT,    2, {NULL}, 0, 0, S_KARTFIRE3}, // S_KARTFIRE2
	{SPR_KFRE, FF_FULLBRIGHT|1,  2, {NULL}, 0, 0, S_KARTFIRE4}, // S_KARTFIRE3
	{SPR_KFRE, FF_FULLBRIGHT|2,  2, {NULL}, 0, 0, S_KARTFIRE5}, // S_KARTFIRE4
	{SPR_KFRE, FF_FULLBRIGHT|3,  2, {NULL}, 0, 0, S_KARTFIRE6}, // S_KARTFIRE5
	{SPR_KFRE, FF_FULLBRIGHT|4,  2, {NULL}, 0, 0, S_KARTFIRE7}, // S_KARTFIRE6
	{SPR_KFRE, FF_FULLBRIGHT|5,  2, {NULL}, 0, 0, S_KARTFIRE8}, // S_KARTFIRE7
	{SPR_KFRE, FF_FULLBRIGHT|6,  2, {NULL}, 0, 0, S_NULL},      // S_KARTFIRE8

	{SPR_AIDU, FF_ANIMATE|FF_PAPERSPRITE, 5*2, {NULL}, 5, 2, S_NULL}, // S_KARTAIZDRIFTSTRAT

	{SPR_KINV, FF_FULLBRIGHT,    1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN2},	// S_KARTINVULN1
	{SPR_KINV, FF_FULLBRIGHT|1,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN3},	// S_KARTINVULN2
	{SPR_KINV, FF_FULLBRIGHT|2,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN4},	// S_KARTINVULN3
	{SPR_KINV, FF_FULLBRIGHT|3,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN5},	// S_KARTINVULN4
	{SPR_KINV, FF_FULLBRIGHT|4,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN6},	// S_KARTINVULN5
	{SPR_KINV, FF_FULLBRIGHT|5,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN7},	// S_KARTINVULN6
	{SPR_KINV, FF_FULLBRIGHT|6,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN8},	// S_KARTINVULN7
	{SPR_KINV, FF_FULLBRIGHT|7,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN9},	// S_KARTINVULN8
	{SPR_KINV, FF_FULLBRIGHT|8,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN10},	// S_KARTINVULN9
	{SPR_KINV, FF_FULLBRIGHT|9,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN11},	// S_KARTINVULN10
	{SPR_KINV, FF_FULLBRIGHT|10,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULN12},	// S_KARTINVULN11
	{SPR_KINV, FF_FULLBRIGHT|11,  1, {A_InvincSparkleRotate}, 0, 0, S_NULL},	// S_KARTINVULN12

	{SPR_KINB, FF_FULLBRIGHT,    1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB2},	// S_KARTINVULNB1
	{SPR_KINB, FF_FULLBRIGHT|1,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB3},	// S_KARTINVULNB2
	{SPR_KINB, FF_FULLBRIGHT|2,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB4},	// S_KARTINVULNB3
	{SPR_KINB, FF_FULLBRIGHT|3,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB5},	// S_KARTINVULNB4
	{SPR_KINB, FF_FULLBRIGHT|4,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB6},	// S_KARTINVULNB5
	{SPR_KINB, FF_FULLBRIGHT|5,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB7},	// S_KARTINVULNB6
	{SPR_KINB, FF_FULLBRIGHT|6,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB8},	// S_KARTINVULNB7
	{SPR_KINB, FF_FULLBRIGHT|7,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB9},	// S_KARTINVULNB8
	{SPR_KINB, FF_FULLBRIGHT|8,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB10},	// S_KARTINVULNB9
	{SPR_KINB, FF_FULLBRIGHT|9,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB11},	// S_KARTINVULNB10
	{SPR_KINB, FF_FULLBRIGHT|10,  1, {A_InvincSparkleRotate}, 0, 0, S_KARTINVULNB12},	// S_KARTINVULNB11
	{SPR_KINB, FF_FULLBRIGHT|11,  1, {A_InvincSparkleRotate}, 0, 0, S_NULL},	// S_KARTINVULNB12

	{SPR_KINF, FF_FULLBRIGHT|FF_TRANS80|FF_ADD, 1, {NULL}, 0, 0, S_INVULNFLASH2},	// S_INVULNFLASH1
	{SPR_NULL, FF_FULLBRIGHT|FF_TRANS80|FF_ADD, 1, {NULL}, 0, 0, S_INVULNFLASH3},	// S_INVULNFLASH2
	{SPR_KINF, FF_FULLBRIGHT|FF_TRANS80|FF_ADD|1, 1, {NULL}, 0, 0, S_INVULNFLASH4},	// S_INVULNFLASH3
	{SPR_NULL, FF_FULLBRIGHT|FF_TRANS80|FF_ADD, 1, {NULL}, 0, 0, S_INVULNFLASH1},	// S_INVULNFLASH4

	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE,    1, {NULL}, 0, 0, S_KARTINVLINES2}, // S_KARTINVLINES1
	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE|1,  1, {NULL}, 0, 0, S_KARTINVLINES3}, // S_KARTINVLINES2
	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE|2,  1, {NULL}, 0, 0, S_KARTINVLINES4}, // S_KARTINVLINES3
	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE|3,  1, {NULL}, 0, 0, S_KARTINVLINES5}, // S_KARTINVLINES4
	{SPR_INVI, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE|4,  1, {NULL}, 2, 1, S_KARTINVLINES6}, // S_KARTINVLINES5
	{SPR_INVI, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE|4,  1, {NULL}, 2, 1, S_KARTINVLINES7}, // S_KARTINVLINES6
	{SPR_INVI, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE|4,  1, {NULL}, 2, 1, S_KARTINVLINES8}, // S_KARTINVLINES7
	{SPR_INVI, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE|4,  1, {NULL}, 2, 1, S_KARTINVLINES9}, // S_KARTINVLINES8
	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE|5,  1, {NULL}, 0, 0, S_KARTINVLINES10}, // S_KARTINVLINES9
	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE|6,  3, {NULL}, 0, 0, S_KARTINVLINES11}, // S_KARTINVLINES10
	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE|7,  1, {NULL}, 0, 0, S_KARTINVLINES12},      // S_KARTINVLINES11
	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE|8,  1, {NULL}, 0, 0, S_KARTINVLINES13},      // S_KARTINVLINES12
	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE|9,  1, {NULL}, 0, 0, S_KARTINVLINES14},      // S_KARTINVLINES13
	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE|10,  1, {NULL}, 0, 0, S_KARTINVLINES15},      // S_KARTINVLINES14
	{SPR_INVI, FF_FULLBRIGHT|FF_PAPERSPRITE|11,  1, {NULL}, 0, 0, S_NULL},      // S_KARTINVLINES15

	{SPR_WIPD, 0, 1, {NULL}, 0, 0, S_WIPEOUTTRAIL2}, // S_WIPEOUTTRAIL1
	{SPR_WIPD, 1, 1, {NULL}, 0, 0, S_WIPEOUTTRAIL3}, // S_WIPEOUTTRAIL2
	{SPR_WIPD, 2, 1, {NULL}, 0, 0, S_WIPEOUTTRAIL4}, // S_WIPEOUTTRAIL3
	{SPR_WIPD, 3, 1, {NULL}, 0, 0, S_WIPEOUTTRAIL5}, // S_WIPEOUTTRAIL4
	{SPR_WIPD, 4, 1, {NULL}, 0, 0, S_WIPEOUTTRAIL6}, // S_WIPEOUTTRAIL5
	{SPR_WIPD, 5, 1, {NULL}, 0, 0, S_WIPEOUTTRAIL7}, // S_WIPEOUTTRAIL6
	{SPR_WIPD, 6, 1, {NULL}, 0, 0, S_WIPEOUTTRAIL8}, // S_WIPEOUTTRAIL7
	{SPR_WIPD, 7, 1, {NULL}, 0, 0, S_WIPEOUTTRAIL9}, // S_WIPEOUTTRAIL8
	{SPR_WIPD, 8, 1, {NULL}, 0, 0, S_WIPEOUTTRAIL10}, // S_WIPEOUTTRAIL9
	{SPR_WIPD, 9, 1, {NULL}, 0, 0, S_WIPEOUTTRAIL11}, // S_WIPEOUTTRAIL10
	{SPR_WIPD, 10, 1, {NULL}, 0, 0, S_NULL}, 		// S_WIPEOUTTRAIL11
	
	{SPR_WIPD, 0, 1, {NULL}, 0, 0, S_FIREWORKTRAIL2}, // S_FIREWORKTRAIL
	{SPR_WIPD, 1|FF_TRANS10, 1, {NULL}, 0, 0, S_FIREWORKTRAIL3}, // S_FIREWORKTRAIL2
	{SPR_WIPD, 2|FF_TRANS20, 1, {NULL}, 0, 0, S_FIREWORKTRAIL4}, // S_FIREWORKTRAIL3
	{SPR_WIPD, 3|FF_TRANS30, 1, {NULL}, 0, 0, S_FIREWORKTRAIL5}, // S_FIREWORKTRAIL4
	{SPR_WIPD, 4|FF_TRANS40, 1, {NULL}, 0, 0, S_FIREWORKTRAIL6}, // S_FIREWORKTRAIL5
	{SPR_WIPD, 5|FF_TRANS50, 1, {NULL}, 0, 0, S_FIREWORKTRAIL7}, // S_FIREWORKTRAIL6
	{SPR_WIPD, 6|FF_TRANS60, 1, {NULL}, 0, 0, S_FIREWORKTRAIL8}, // S_FIREWORKTRAIL7
	{SPR_WIPD, 7|FF_TRANS70, 1, {NULL}, 0, 0, S_FIREWORKTRAIL9}, // S_FIREWORKTRAIL8
	{SPR_WIPD, 8|FF_TRANS80, 1, {NULL}, 0, 0, S_FIREWORKTRAIL10}, // S_FIREWORKTRAIL9
	{SPR_WIPD, 9|FF_TRANS90, 1, {NULL}, 0, 0, S_FIREWORKTRAIL11}, // S_FIREWORKTRAIL10
	{SPR_WIPD, 10|FF_TRANS90, 1, {NULL}, 0, 0, S_NULL}, 		// S_FIREWORKTRAIL11

	{SPR_RSHE, 0, -1, {NULL}, 0, 0, S_NULL}, // S_ROCKETSNEAKER_L
	{SPR_RSHE, 1, -1, {NULL}, 0, 0, S_NULL}, // S_ROCKETSNEAKER_R
	{SPR_RSHE, 2, -1, {NULL}, 0, 0, S_NULL}, // S_ROCKETSNEAKER_LVIBRATE
	{SPR_RSHE, 3, -1, {NULL}, 0, 0, S_NULL}, // S_ROCKETSNEAKER_RVIBRATE

	{SPR_FITM,    FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM2},	// S_EGGMANITEM1
	{SPR_FITM,  2|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM3},	// S_EGGMANITEM2
	{SPR_FITM,  4|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM4},	// S_EGGMANITEM3
	{SPR_FITM,  6|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM5},	// S_EGGMANITEM4
	{SPR_FITM,  8|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM6},	// S_EGGMANITEM5
	{SPR_FITM, 10|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM7},	// S_EGGMANITEM6
	{SPR_FITM, 12|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM8},	// S_EGGMANITEM7
	{SPR_FITM, 14|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM9},	// S_EGGMANITEM8
	{SPR_FITM, 16|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM10},	// S_EGGMANITEM9
	{SPR_FITM, 18|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM11},	// S_EGGMANITEM10
	{SPR_FITM, 20|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM12},	// S_EGGMANITEM11
	{SPR_FITM, 22|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_EGGMANITEM1},	// S_EGGMANITEM12
	{SPR_FITM, 24|FF_FULLBRIGHT, 175, {NULL}, 0, 0, S_NULL},								// S_EGGMANITEM_DEAD

	{SPR_BANA, 0,  -1, {NULL}, 0, 0, S_NULL}, // S_BANANA
	{SPR_BANA, 1, 175, {NULL}, 0, 0, S_NULL}, // S_BANANA_DEAD

	{SPR_BAND, 0, -1, {NULL}, 0, 0, S_BANANA_SPARK2}, // S_BANANA_SPARK
	{SPR_BAND, 1|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_BANANA_SPARK3}, // S_BANANA_SPARK2
	{SPR_BAND, 2|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_BANANA_SPARK4}, // S_BANANA_SPARK3
	{SPR_BAND, 3|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_BANANA_SPARK4

	{SPR_ORBN,  0, 1, {NULL}, 0, 0, S_ORBINAUT2},			// S_ORBINAUT1
	{SPR_ORBN,  1, 1, {NULL}, 0, 0, S_ORBINAUT3},			// S_ORBINAUT2
	{SPR_ORBN,  2, 1, {NULL}, 0, 0, S_ORBINAUT4},			// S_ORBINAUT3
	{SPR_ORBN,  3, 1, {NULL}, 0, 0, S_ORBINAUT5},			// S_ORBINAUT4S_ORBINAUT4
	{SPR_ORBN,  4, 1, {NULL}, 0, 0, S_ORBINAUT6},			// S_ORBINAUT5
	{SPR_ORBN,  5, 1, {NULL}, 0, 0, S_ORBINAUT1},			// S_ORBINAUT6
	{SPR_ORBN,  0, 175, {NULL}, 0, 0, S_NULL},			// S_ORBINAUT_DEAD
	{SPR_ORBN,  6, 3, {NULL}, 0, 0, S_ORBINAUT_SHIELD2},	// S_ORBINAUT_SHIELD1
	{SPR_ORBN,  7, 3, {NULL}, 0, 0, S_ORBINAUT_SHIELD3},	// S_ORBINAUT_SHIELD2
	{SPR_ORBN,  8, 3, {NULL}, 0, 0, S_ORBINAUT_SHIELD4},	// S_ORBINAUT_SHIELD3
	{SPR_ORBN,  9, 3, {NULL}, 0, 0, S_ORBINAUT_SHIELD5},	// S_ORBINAUT_SHIELD4
	{SPR_ORBN, 10, 3, {NULL}, 0, 0, S_ORBINAUT_SHIELD6},	// S_ORBINAUT_SHIELD5
	{SPR_ORBN, 11, 3, {NULL}, 0, 0, S_ORBINAUT_SHIELD1},	// S_ORBINAUT_SHIELD6
	{SPR_ORBN,  6, 175, {NULL}, 0, 0, S_NULL},			// S_ORBINAUT_SHIELDDEAD

	{SPR_JAWZ, 0, 1, {NULL}, 0, 0, S_JAWZ2},	// S_JAWZ1
	{SPR_JAWZ, 4, 1, {NULL}, 0, 0, S_JAWZ3},	// S_JAWZ2
	{SPR_JAWZ, 1, 1, {NULL}, 0, 0, S_JAWZ4},	// S_JAWZ3
	{SPR_JAWZ, 4, 1, {NULL}, 0, 0, S_JAWZ5},	// S_JAWZ4
	{SPR_JAWZ, 2, 1, {NULL}, 0, 0, S_JAWZ6},	// S_JAWZ5
	{SPR_JAWZ, 4, 1, {NULL}, 0, 0, S_JAWZ7},	// S_JAWZ6
	{SPR_JAWZ, 3, 1, {NULL}, 0, 0, S_JAWZ8},	// S_JAWZ7
	{SPR_JAWZ, 4, 1, {NULL}, 0, 0, S_JAWZ1},	// S_JAWZ8

	{SPR_JAWZ, 0, 1, {NULL}, 0, 0, S_JAWZ_SHIELD2},	// S_JAWZ_SHIELD1
	{SPR_JAWZ, 4, 1, {NULL}, 0, 0, S_JAWZ_SHIELD3},	// S_JAWZ_SHIELD2
	{SPR_JAWZ, 1, 1, {NULL}, 0, 0, S_JAWZ_SHIELD4},	// S_JAWZ_SHIELD3
	{SPR_JAWZ, 4, 1, {NULL}, 0, 0, S_JAWZ_SHIELD5},	// S_JAWZ_SHIELD4
	{SPR_JAWZ, 2, 1, {NULL}, 0, 0, S_JAWZ_SHIELD6},	// S_JAWZ_SHIELD5
	{SPR_JAWZ, 4, 1, {NULL}, 0, 0, S_JAWZ_SHIELD7},	// S_JAWZ_SHIELD6
	{SPR_JAWZ, 3, 1, {NULL}, 0, 0, S_JAWZ_SHIELD8},	// S_JAWZ_SHIELD7
	{SPR_JAWZ, 4, 1, {NULL}, 0, 0, S_JAWZ_SHIELD1},	// S_JAWZ_SHIELD8

	{SPR_JAWZ, 5, 175, {NULL}, 0, 0, S_JAWZ_DEAD2},	// S_JAWZ_DEAD1
	{SPR_NULL, 0, 1, {A_JawzExplode}, 0, 0, S_NULL},	// S_JAWZ_DEAD2

	{SPR_RETI, FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL}, // S_PLAYERRETICULE

	{SPR_SSMN,  0,               30, {NULL}, 0, 0, S_SSMINE2},							// S_SSMINE1
	{SPR_SSMN,  3|FF_FULLBRIGHT,  3, {NULL}, 0, 0, S_SSMINE3},							// S_SSMINE2
	{SPR_SSMN,  2|FF_FULLBRIGHT,  3, {NULL}, 0, 0, S_SSMINE4},							// S_SSMINE3
	{SPR_SSMN,  1|FF_SEMIBRIGHT,  3, {NULL}, 0, 0, S_SSMINE1},							// S_SSMINE4
	{SPR_SSMN,  4,                1, {NULL}, 0, 0, S_SSMINE_SHIELD2},					// S_SSMINE_SHIELD1
	{SPR_SSMN,  5|FF_FULLBRIGHT,  1, {NULL}, 0, 0, S_SSMINE_SHIELD1},					// S_SSMINE_SHIELD2
	{SPR_SSMN,  4,                1, {NULL}, 0, 0, S_SSMINE_AIR2},						// S_SSMINE_AIR1
	{SPR_SSMN,  5,                1, {NULL}, 0, 0, S_SSMINE_AIR1},						// S_SSMINE_AIR2
	{SPR_SSMN,  6,                3, {NULL}, 0, 0, S_SSMINE_DEPLOY2},					// S_SSMINE_DEPLOY1
	{SPR_SSMN,  7,                5, {NULL}, 0, 0, S_SSMINE_DEPLOY3},					// S_SSMINE_DEPLOY2
	{SPR_SSMN,  8,                7, {NULL}, 0, 0, S_SSMINE_DEPLOY4},					// S_SSMINE_DEPLOY3
	{SPR_SSMN,  9,                1, {NULL}, 0, 0, S_SSMINE_DEPLOY5},					// S_SSMINE_DEPLOY4
	{SPR_SSMN, 10,                1, {NULL}, 0, 0, S_SSMINE_DEPLOY6},					// S_SSMINE_DEPLOY5
	{SPR_SSMN,  9,                1, {NULL}, 0, 0, S_SSMINE_DEPLOY7},					// S_SSMINE_DEPLOY6
	{SPR_SSMN, 10,                3, {NULL}, 0, 0, S_SSMINE_DEPLOY8},					// S_SSMINE_DEPLOY7
	{SPR_SSMN, 11,                1, {A_PlaySound}, sfx_cdfm39, 1, S_SSMINE_DEPLOY9},	// S_SSMINE_DEPLOY8
	{SPR_SSMN, 10,                1, {NULL}, 0, 0, S_SSMINE_DEPLOY10},					// S_SSMINE_DEPLOY9
	{SPR_SSMN, 11,                3, {NULL}, 0, 0, S_SSMINE_DEPLOY11},					// S_SSMINE_DEPLOY10
	{SPR_SSMN, 12,                3, {NULL}, 0, 0, S_SSMINE_DEPLOY12},					// S_SSMINE_DEPLOY11
	{SPR_SSMN, 13,                3, {NULL}, 0, 0, S_SSMINE_DEPLOY13},					// S_SSMINE_DEPLOY12
	{SPR_SSMN, 14,                3, {NULL}, 0, 0, S_SSMINE1},							// S_SSMINE_DEPLOY13
	{SPR_SSMN,  3,                1, {A_SSMineExplode}, 0, 0, S_SSMINE_EXPLODE2},		// S_SSMINE_EXPLODE
	{SPR_NULL,  0,               12, {A_SSMineExplode}, 1, 0, S_NULL},					// S_SSMINE_EXPLODE2

	{SPR_KRBM, FF_FULLBRIGHT,   1, {NULL}, 0, 0, S_QUICKBOOM2},		// S_QUICKBOOM1
	{SPR_KRBM, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_QUICKBOOM3},		// S_QUICKBOOM2
	{SPR_KRBM, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_QUICKBOOM4},		// S_QUICKBOOM3
	{SPR_KRBM, FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_QUICKBOOM5},		// S_QUICKBOOM4
	{SPR_KRBM, FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_QUICKBOOM6},		// S_QUICKBOOM5
	{SPR_KRBM, FF_FULLBRIGHT|5, 1, {NULL}, 0, 0, S_QUICKBOOM7},		// S_QUICKBOOM6
	{SPR_KRBM, FF_FULLBRIGHT|6, 1, {NULL}, 0, 0, S_QUICKBOOM8},		// S_QUICKBOOM7
	{SPR_KRBM, FF_FULLBRIGHT|7, 2, {NULL}, 0, 0, S_QUICKBOOM9},		// S_QUICKBOOM8
	{SPR_KRBM, FF_FULLBRIGHT|8, 2, {NULL}, 0, 0, S_QUICKBOOM10},		// S_QUICKBOOM9
	{SPR_KRBM, FF_FULLBRIGHT|9, 2, {NULL}, 0, 0, S_NULL},				// S_QUICKBOOM10

	{SPR_KRBM, FF_FULLBRIGHT,   3, {NULL}, 0, 0, S_SLOWBOOM2},		// S_SLOWBOOM1
	{SPR_KRBM, FF_FULLBRIGHT|1, 3, {NULL}, 0, 0, S_SLOWBOOM3},		// S_SLOWBOOM2
	{SPR_KRBM, FF_FULLBRIGHT|2, 3, {NULL}, 0, 0, S_SLOWBOOM4},		// S_SLOWKBOOM3
	{SPR_KRBM, FF_FULLBRIGHT|3, 3, {NULL}, 0, 0, S_SLOWBOOM5},		// S_SLOWBOOM4
	{SPR_KRBM, FF_FULLBRIGHT|4, 3, {NULL}, 0, 0, S_SLOWBOOM6},		// S_SLOWBOOM5
	{SPR_KRBM, FF_FULLBRIGHT|5, 3, {NULL}, 0, 0, S_SLOWBOOM7},		// S_SLOWBOOM6
	{SPR_KRBM, FF_FULLBRIGHT|6, 3, {NULL}, 0, 0, S_SLOWBOOM8},		// S_SLOWBOOM7
	{SPR_KRBM, FF_FULLBRIGHT|7, 5, {NULL}, 0, 0, S_SLOWBOOM9},		// S_SLOWBOOM8
	{SPR_KRBM, FF_FULLBRIGHT|8, 5, {NULL}, 0, 0, S_SLOWBOOM10},		// S_SLOWBOOM9
	{SPR_KRBM, FF_FULLBRIGHT|9, 5, {NULL}, 0, 0, S_NULL},				// S_SLOWBOOM10

	{SPR_LNDM, 0, -1, {NULL}, 0, 0, S_LANDMINE},	// S_LANDMINE
	{SPR_NULL, 0,  1, {A_LandMineExplode}, 0, 0, S_NULL},		// S_LANDMINE_EXPLODE

	{SPR_DTRG, 0, -1, {NULL}, 0, 0, S_NULL},	// S_DROPTARGET
	{SPR_DTRG, 1, -1, {NULL}, 0, 0, S_NULL},	// S_DROPTARGET_SPIN

	{SPR_BHOG,               0, 3, {A_PlaySound}, sfx_s1bd, 1, S_BALLHOG2},	// S_BALLHOG1
	{SPR_BHOG, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_BALLHOG3},					// S_BALLHOG2
	{SPR_BHOG, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_BALLHOG4},					// S_BALLHOG3
	{SPR_BHOG, FF_FULLBRIGHT|3, 3, {NULL}, 0, 0, S_BALLHOG5}, 				// S_BALLHOG4
	{SPR_BHOG, FF_FULLBRIGHT|4, 3, {NULL}, 0, 0, S_BALLHOG6}, 				// S_BALLHOG5
	{SPR_BHOG,               5, 2, {NULL}, 0, 0, S_BALLHOG7}, 				// S_BALLHOG6
	{SPR_BHOG,               6, 1, {NULL}, 0, 0, S_BALLHOG8}, 				// S_BALLHOG7
	{SPR_BHOG,               7, 1, {NULL}, 0, 0, S_BALLHOG1}, 				// S_BALLHOG8
	{SPR_NULL,               0, 1, {A_BallhogExplode}, 0, 0, S_NULL},			// S_BALLHOG_DEAD
	{SPR_BHBM, FF_ANIMATE|FF_FULLBRIGHT, 27, {NULL}, 26, 1, S_NULL}, // S_BALLHOGBOOM
	{SPR_BHGR, FF_ANIMATE|FF_FULLBRIGHT|0, 2*TICRATE, {NULL}, 5, 3, S_NULL},	// S_BALLHOG_RETICULE

	{SPR_SPBM, 0, 1, {NULL}, 0, 0,  S_SPB2}, // S_SPB1
	{SPR_SPBM, 1, 1, {NULL}, 0, 0,  S_SPB3}, // S_SPB2
	{SPR_SPBM, 0, 1, {NULL}, 0, 0,  S_SPB4}, // S_SPB3
	{SPR_SPBM, 2, 1, {NULL}, 0, 0,  S_SPB5}, // S_SPB4
	{SPR_SPBM, 0, 1, {NULL}, 0, 0,  S_SPB6}, // S_SPB5
	{SPR_SPBM, 3, 1, {NULL}, 0, 0,  S_SPB7}, // S_SPB6
	{SPR_SPBM, 0, 1, {NULL}, 0, 0,  S_SPB8}, // S_SPB7
	{SPR_SPBM, 4, 1, {NULL}, 0, 0,  S_SPB9}, // S_SPB8
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_SPB10}, // S_SPB9
	{SPR_SPBM, 5, 1, {NULL}, 0, 0, S_SPB11}, // S_SPB10
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_SPB12}, // S_SPB11
	{SPR_SPBM, 6, 1, {NULL}, 0, 0, S_SPB13}, // S_SPB12
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_SPB14}, // S_SPB13
	{SPR_SPBM, 7, 1, {NULL}, 0, 0, S_SPB15}, // S_SPB14
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_SPB16}, // S_SPB15
	{SPR_SPBM, 8, 1, {NULL}, 0, 0, S_SPB17}, // S_SPB16
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_SPB18}, // S_SPB17
	{SPR_SPBM, 8, 1, {NULL}, 0, 0, S_SPB19}, // S_SPB18
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_SPB20}, // S_SPB19
	{SPR_SPBM, 8, 1, {NULL}, 0, 0,  S_SPB1}, // S_SPB20
	{SPR_SPBM, 8, 175, {NULL}, 0, 0, S_NULL}, // S_SPB_DEAD

	{SPR_TRIS, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE|FF_ADD,  9, {NULL},  2, 3, S_MANTA2}, // S_MANTA1
	{SPR_TRNQ, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE|FF_ADD, -1, {NULL}, 7, 1, S_NULL},   // S_MANTA2

	{SPR_THNS, FF_FULLBRIGHT|9,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD2},		// S_LIGHTNINGSHIELD1
	{SPR_THNS, FF_FULLBRIGHT|10, 2, {NULL}, 0, 0, S_LIGHTNINGSHIELD3},		// S_LIGHTNINGSHIELD2
	{SPR_THNS, FF_FULLBRIGHT|11, 2, {NULL}, 0, 0, S_LIGHTNINGSHIELD4},		// S_LIGHTNINGSHIELD3
	{SPR_THNS, FF_FULLBRIGHT,    2, {NULL}, 0, 0, S_LIGHTNINGSHIELD5},		// S_LIGHTNINGSHIELD4
	{SPR_THNS, FF_FULLBRIGHT|1,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD6},		// S_LIGHTNINGSHIELD5
	{SPR_THNS, FF_FULLBRIGHT|2,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD7},		// S_LIGHTNINGSHIELD6
	{SPR_THNS, FF_FULLBRIGHT|3,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD8},		// S_LIGHTNINGSHIELD7
	{SPR_THNS, FF_FULLBRIGHT|4,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD9},		// S_LIGHTNINGSHIELD8
	{SPR_THNS, FF_FULLBRIGHT|5,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD10},		// S_LIGHTNINGSHIELD9
	{SPR_THNS, FF_FULLBRIGHT|6,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD11},		// S_LIGHTNINGSHIELD10
	{SPR_THNS, FF_FULLBRIGHT|7,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD12},		// S_LIGHTNINGSHIELD11
	{SPR_THNS, FF_FULLBRIGHT|8,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD13},		// S_LIGHTNINGSHIELD12
	{SPR_THNS, FF_FULLBRIGHT|9,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD14},		// S_LIGHTNINGSHIELD13
	{SPR_THNS, FF_FULLBRIGHT|10, 2, {NULL}, 0, 0, S_LIGHTNINGSHIELD15},		// S_LIGHTNINGSHIELD14
	{SPR_THNS, FF_FULLBRIGHT|11, 2, {NULL}, 0, 0, S_LIGHTNINGSHIELD16},		// S_LIGHTNINGSHIELD15
	{SPR_THNS, FF_FULLBRIGHT|8,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD17},		// S_LIGHTNINGSHIELD16
	{SPR_THNS, FF_FULLBRIGHT|7,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD18},		// S_LIGHTNINGSHIELD17
	{SPR_THNS, FF_FULLBRIGHT|6,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD19},		// S_LIGHTNINGSHIELD18
	{SPR_THNS, FF_FULLBRIGHT|5,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD20},		// S_LIGHTNINGSHIELD19
	{SPR_THNS, FF_FULLBRIGHT|4,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD21},		// S_LIGHTNINGSHIELD20
	{SPR_THNS, FF_FULLBRIGHT|3,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD22},		// S_LIGHTNINGSHIELD21
	{SPR_THNS, FF_FULLBRIGHT|2,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD23},		// S_LIGHTNINGSHIELD22
	{SPR_THNS, FF_FULLBRIGHT|1,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD24},		// S_LIGHTNINGSHIELD23
	{SPR_THNS, FF_FULLBRIGHT|0,  2, {NULL}, 0, 0, S_LIGHTNINGSHIELD1},		// S_LIGHTNINGSHIELD24
																			//
	// Lightning Shield Visuals
	{SPR_THNC, FF_ADD|FF_FULLBRIGHT|FF_ANIMATE, 11, {NULL}, 10, 1, S_THNA1}, // S_THNC1
	{SPR_THNA, FF_ADD|FF_FULLBRIGHT|FF_ANIMATE, 44, {NULL}, 43, 1, S_THNC2}, // S_THNA1
	{SPR_THNC, FF_ADD|FF_FULLBRIGHT|FF_ANIMATE, 11, {NULL}, 10, 1, S_THNB1}, // S_THNC2
	{SPR_THNB, FF_ADD|FF_FULLBRIGHT|FF_ANIMATE, 43, {NULL}, 42, 1, S_THNC1}, // S_THNB1

	{SPR_THND, FF_ADD|FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 10, 1, S_THND}, // S_THND
	{SPR_THNE, FF_FULLBRIGHT|FF_ANIMATE, 34, {NULL}, 33, 1, S_THNH}, // S_THNE
	{SPR_NULL, FF_FULLBRIGHT, 34, {NULL}, 33, 1, S_THNE}, // S_THNH
	{SPR_THNF, FF_FULLBRIGHT|FF_ANIMATE, 4, {NULL}, 3, 1, S_THNG}, // S_THNF
	{SPR_THNG, FF_FULLBRIGHT|FF_ANIMATE, 64, {NULL}, 63, 1, S_THNF}, // S_THNG

	{SPR_BUBS, FF_FULLBRIGHT,     2, {NULL}, 0, 0, S_BUBBLESHIELD2},		// S_BUBBLESHIELD1
	{SPR_BUBS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_BUBBLESHIELD3},		// S_BUBBLESHIELD2
	{SPR_BUBS, FF_FULLBRIGHT|1,   2, {NULL}, 0, 0, S_BUBBLESHIELD4},		// S_BUBBLESHIELD3
	{SPR_BUBS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_BUBBLESHIELD5},		// S_BUBBLESHIELD4
	{SPR_BUBS, FF_FULLBRIGHT|2,   2, {NULL}, 0, 0, S_BUBBLESHIELD6},		// S_BUBBLESHIELD5
	{SPR_BUBS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_BUBBLESHIELD7},		// S_BUBBLESHIELD6
	{SPR_BUBS, FF_FULLBRIGHT|3,   2, {NULL}, 0, 0, S_BUBBLESHIELD8},		// S_BUBBLESHIELD7
	{SPR_BUBS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_BUBBLESHIELD9},		// S_BUBBLESHIELD8
	{SPR_BUBS, FF_FULLBRIGHT|4,   2, {NULL}, 0, 0, S_BUBBLESHIELD10},		// S_BUBBLESHIELD9
	{SPR_BUBS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_BUBBLESHIELD11},		// S_BUBBLESHIELD10
	{SPR_BUBS, FF_FULLBRIGHT|5,   2, {NULL}, 0, 0, S_BUBBLESHIELD12},		// S_BUBBLESHIELD11
	{SPR_BUBS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_BUBBLESHIELD13},		// S_BUBBLESHIELD12
	{SPR_BUBS, FF_FULLBRIGHT|6,   2, {NULL}, 0, 0, S_BUBBLESHIELD14},		// S_BUBBLESHIELD13
	{SPR_BUBS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_BUBBLESHIELD15},		// S_BUBBLESHIELD14
	{SPR_BUBS, FF_FULLBRIGHT|7,   2, {NULL}, 0, 0, S_BUBBLESHIELD16},		// S_BUBBLESHIELD15
	{SPR_BUBS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_BUBBLESHIELD17},		// S_BUBBLESHIELD16
	{SPR_BUBS, FF_FULLBRIGHT|8,   2, {NULL}, 0, 0, S_BUBBLESHIELD18},		// S_BUBBLESHIELD17
	{SPR_BUBS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_BUBBLESHIELD1},		// S_BUBBLESHIELD18
	{SPR_BUBS, FF_FULLBRIGHT|13, -1, {NULL}, 0, 0, S_BUBBLESHIELDBLOWUP},	// S_BUBBLESHIELDBLOWUP
	{SPR_BUBT, FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 7, 3, S_NULL},	// S_BUBBLESHIELDTRAP1
	{SPR_BUBS, FF_FULLBRIGHT|14,  2, {NULL}, 0, 0, S_BUBBLESHIELDTRAP3},	// S_BUBBLESHIELDTRAP2
	{SPR_BUBS, FF_FULLBRIGHT|15,  2, {NULL}, 0, 0, S_BUBBLESHIELDTRAP4},	// S_BUBBLESHIELDTRAP3
	{SPR_BUBS, FF_FULLBRIGHT|14,  2, {NULL}, 0, 0, S_BUBBLESHIELDTRAP5},	// S_BUBBLESHIELDTRAP4
	{SPR_BUBS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_BUBBLESHIELDTRAP6},	// S_BUBBLESHIELDTRAP5
	{SPR_BUBS, FF_FULLBRIGHT|12,  2, {NULL}, 0, 0, S_BUBBLESHIELDTRAP7},	// S_BUBBLESHIELDTRAP6
	{SPR_BUBS, FF_FULLBRIGHT|11,  2, {NULL}, 0, 0, S_BUBBLESHIELDTRAP8},	// S_BUBBLESHIELDTRAP7
	{SPR_BUBS, FF_FULLBRIGHT|12,  2, {NULL}, 0, 0, S_BUBBLESHIELDTRAP1},	// S_BUBBLESHIELDTRAP8
	{SPR_BWVE, FF_FULLBRIGHT,     1, {NULL}, 0, 0, S_BUBBLESHIELDWAVE2},	// S_BUBBLESHIELDWAVE1
	{SPR_BWVE, FF_FULLBRIGHT|1,   1, {NULL}, 0, 0, S_BUBBLESHIELDWAVE3},	// S_BUBBLESHIELDWAVE2
	{SPR_BWVE, FF_FULLBRIGHT|2,   1, {NULL}, 0, 0, S_BUBBLESHIELDWAVE4},	// S_BUBBLESHIELDWAVE3
	{SPR_BWVE, FF_FULLBRIGHT|3,   1, {NULL}, 0, 0, S_BUBBLESHIELDWAVE5},	// S_BUBBLESHIELDWAVE4
	{SPR_BWVE, FF_FULLBRIGHT|4,   1, {NULL}, 0, 0, S_BUBBLESHIELDWAVE6},	// S_BUBBLESHIELDWAVE5
	{SPR_BWVE, FF_FULLBRIGHT|5,   1, {NULL}, 0, 0, S_NULL},					// S_BUBBLESHIELDWAVE6

	// Bubble Shield Visuals
	{SPR_BUBA, FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_BUBA1}, // S_BUBA1
	{SPR_BUBB, FF_FULLBRIGHT|FF_ANIMATE, 36, {NULL}, 8, 4, S_BUBB1}, // S_BUBB1
	{SPR_NULL, 0, 5, {NULL}, 0, 0, S_BUBB1}, // S_BUBB2
	{SPR_BUBC, FF_FULLBRIGHT|FF_ANIMATE, 36, {NULL}, 8, 4, S_BUBC1}, // S_BUBC1
	{SPR_NULL, 0, 5, {NULL}, 0, 0, S_BUBC1}, // S_BUBC2
	{SPR_BUBD, FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_BUBD1}, // S_BUBD1
	{SPR_BUBE, FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_BUBE1}, // S_BUBE1
	{SPR_BUBG, FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_BUBG1}, // S_BUBG1

	{SPR_FLMS, FF_FULLBRIGHT,     2, {NULL}, 0, 0, S_FLAMESHIELD2},			// S_FLAMESHIELD1
	{SPR_FLMS, FF_FULLBRIGHT|9,   2, {NULL}, 0, 0, S_FLAMESHIELD3},			// S_FLAMESHIELD2
	{SPR_FLMS, FF_FULLBRIGHT|1,   2, {NULL}, 0, 0, S_FLAMESHIELD4},			// S_FLAMESHIELD3
	{SPR_FLMS, FF_FULLBRIGHT|10,  2, {NULL}, 0, 0, S_FLAMESHIELD5},			// S_FLAMESHIELD4
	{SPR_FLMS, FF_FULLBRIGHT|2,   2, {NULL}, 0, 0, S_FLAMESHIELD6},			// S_FLAMESHIELD5
	{SPR_FLMS, FF_FULLBRIGHT|11,  2, {NULL}, 0, 0, S_FLAMESHIELD7},			// S_FLAMESHIELD6
	{SPR_FLMS, FF_FULLBRIGHT|3,   2, {NULL}, 0, 0, S_FLAMESHIELD8},			// S_FLAMESHIELD7
	{SPR_FLMS, FF_FULLBRIGHT|12,  2, {NULL}, 0, 0, S_FLAMESHIELD9},			// S_FLAMESHIELD8
	{SPR_FLMS, FF_FULLBRIGHT|4,   2, {NULL}, 0, 0, S_FLAMESHIELD10},		// S_FLAMESHIELD9
	{SPR_FLMS, FF_FULLBRIGHT|13,  2, {NULL}, 0, 0, S_FLAMESHIELD11},		// S_FLAMESHIELD10
	{SPR_FLMS, FF_FULLBRIGHT|5,   2, {NULL}, 0, 0, S_FLAMESHIELD12},		// S_FLAMESHIELD11
	{SPR_FLMS, FF_FULLBRIGHT|14,  2, {NULL}, 0, 0, S_FLAMESHIELD13},		// S_FLAMESHIELD12
	{SPR_FLMS, FF_FULLBRIGHT|6,   2, {NULL}, 0, 0, S_FLAMESHIELD14},		// S_FLAMESHIELD13
	{SPR_FLMS, FF_FULLBRIGHT|15,  2, {NULL}, 0, 0, S_FLAMESHIELD15},		// S_FLAMESHIELD14
	{SPR_FLMS, FF_FULLBRIGHT|7,   2, {NULL}, 0, 0, S_FLAMESHIELD16},		// S_FLAMESHIELD15
	{SPR_FLMS, FF_FULLBRIGHT|16,  2, {NULL}, 0, 0, S_FLAMESHIELD17},		// S_FLAMESHIELD16
	{SPR_FLMS, FF_FULLBRIGHT|8,   2, {NULL}, 0, 0, S_FLAMESHIELD18},		// S_FLAMESHIELD17
	{SPR_FLMS, FF_FULLBRIGHT|17,  2, {NULL}, 0, 0, S_FLAMESHIELD1},			// S_FLAMESHIELD18
																			//
	// Flame Shield Visuals
	{SPR_FLMA, FF_ADD|FF_FULLBRIGHT|FF_ANIMATE, 28, {NULL}, 27, 1, S_FLMA2}, // S_FLMA1
	{SPR_NULL, 0, 16, {NULL}, 0, 0, S_FLMA1}, // S_FLMA2
	{SPR_FLMB, FF_FULLBRIGHT|FF_ANIMATE, 44, {NULL}, 43, 1, S_FLMB1}, // S_FLMB1

	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|1, 1, {NULL},               0, 0, S_FLAMESHIELDDASH2},		// S_FLAMESHIELDDASH1
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|5, 1, {NULL},               0, 0, S_FLAMESHIELDDASH3},		// S_FLAMESHIELDDASH2
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT,   1, {A_FlameShieldPaper}, 0, 2, S_FLAMESHIELDDASH4},		// S_FLAMESHIELDDASH3
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|2, 1, {NULL},               0, 0, S_FLAMESHIELDDASH5},		// S_FLAMESHIELDDASH4
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|6, 1, {NULL},               0, 0, S_FLAMESHIELDDASH6},		// S_FLAMESHIELDDASH5
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT,   1, {A_FlameShieldPaper}, 1, 3, S_FLAMESHIELDDASH7},		// S_FLAMESHIELDDASH6
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|3, 1, {NULL},               0, 0, S_FLAMESHIELDDASH8},		// S_FLAMESHIELDDASH7
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|7, 1, {NULL},               0, 0, S_FLAMESHIELDDASH9},		// S_FLAMESHIELDDASH8
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT,   1, {A_FlameShieldPaper}, 2, 0, S_FLAMESHIELDDASH10},	// S_FLAMESHIELDDASH9
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|4, 1, {NULL},               0, 0, S_FLAMESHIELDDASH11},	// S_FLAMESHIELDDASH10
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|8, 1, {NULL},               0, 0, S_FLAMESHIELDDASH12},	// S_FLAMESHIELDDASH11
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT,   1, {A_FlameShieldPaper}, 3, 1, S_FLAMESHIELDDASH1},		// S_FLAMESHIELDDASH12

	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|9, 2, {NULL}, 0, 0, S_NULL},	// S_FLAMESHIELDDASH2_UNDERLAY
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|10, 2, {NULL}, 0, 0, S_NULL},	// S_FLAMESHIELDDASH5_UNDERLAY
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|11, 2, {NULL}, 0, 0, S_NULL},	// S_FLAMESHIELDDASH8_UNDERLAY
	{SPR_FLMD, FF_ADD|FF_FULLBRIGHT|12, 2, {NULL}, 0, 0, S_NULL},	// S_FLAMESHIELDDASH11_UNDERLAY

	{SPR_FLMP, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {NULL}, 0, 0, S_NULL}, // S_FLAMESHIELDPAPER
	{SPR_FLML, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 7, {NULL}, 6, 1, S_NULL}, // S_FLAMESHIELDLINE1
	{SPR_FLML, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE|7, 7, {NULL}, 6, 1, S_NULL}, // S_FLAMESHIELDLINE2
	{SPR_FLML, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE|14, 7, {NULL}, 6, 1, S_NULL}, // S_FLAMESHIELDLINE3
	{SPR_FLMF, FF_ADD|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL}, // S_FLAMESHIELDFLASH

	{SPR_GTOP, FF_ANIMATE, -1, {NULL}, 5, 1, S_NULL}, // S_GARDENTOP_FLOATING
	{SPR_GTOP, 0, 1, {NULL}, 5, 1, S_GARDENTOP_SINKING2}, // S_GARDENTOP_SINKING1
	{SPR_GTOP, 2, 1, {NULL}, 5, 1, S_GARDENTOP_SINKING3}, // S_GARDENTOP_SINKING2
	{SPR_GTOP, 4, 1, {NULL}, 5, 1, S_GARDENTOP_SINKING1}, // S_GARDENTOP_SINKING3
	{SPR_GTOP, FF_ANIMATE, 100, {A_Scream}, 5, 1, S_NULL}, // S_GARDENTOP_DEAD
	{SPR_BDRF, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 5, 2, S_NULL}, // S_GARDENTOPSPARK
	{SPR_GTAR, FF_FULLBRIGHT|FF_PAPERSPRITE, -1, {NULL}, 5, 2, S_NULL}, // S_GARDENTOPARROW

	{SPR_HYUU, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_HYUDORO
	{SPR_HYUU, FF_FULLBRIGHT|1, -1, {NULL}, 0, 0, S_NULL}, // S_HYUDORO_RETURNING

	{SPR_GRWP, FF_FULLBRIGHT|FF_ANIMATE, 13, {NULL}, 7, 1, S_NULL}, // S_GROW_PARTICLE

	{SPR_POHB, 0, 1, {NULL}, 0, 0, S_SHRINK_POHBEE2}, // S_SHRINK_POHBEE
	{SPR_POHB, 1, 1, {NULL}, 0, 0, S_SHRINK_POHBEE3}, // S_SHRINK_POHBEE2
	{SPR_POHB, 0, 1, {NULL}, 0, 0, S_SHRINK_POHBEE4}, // S_SHRINK_POHBEE3
	{SPR_POHB, 2, 1, {NULL}, 0, 0, S_SHRINK_POHBEE5}, // S_SHRINK_POHBEE4
	{SPR_POHB, 0, 1, {NULL}, 0, 0, S_SHRINK_POHBEE6}, // S_SHRINK_POHBEE5
	{SPR_POHB, 3, 1, {NULL}, 0, 0, S_SHRINK_POHBEE7}, // S_SHRINK_POHBEE6
	{SPR_POHB, 0, 1, {NULL}, 0, 0, S_SHRINK_POHBEE8}, // S_SHRINK_POHBEE7
	{SPR_POHB, 2, 1, {NULL}, 0, 0, S_SHRINK_POHBEE}, // S_SHRINK_POHBEE8

	{SPR_POHC, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SHRINK_CHAIN

	{SPR_SHRG, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SHRINK_GUN
	{SPR_SHRG, FF_FULLBRIGHT|1, -1, {NULL}, 0, 0, S_NULL}, // S_SHRINK_GUN_OVERLAY

	{SPR_SHRL, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_SHRINK_LASER
	{SPR_SHRL, FF_FULLBRIGHT|1, -1, {NULL}, 0, 0, S_NULL}, // S_SHRINK_PARTICLE

	{SPR_SINK, 0,  1, {A_SmokeTrailer}, MT_SINKTRAIL, 0, S_SINK},	// S_SINK
	{SPR_SINK, 0|FF_TRANS80|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_SINK_SHIELD}, // S_SINK_SHIELD
	{SPR_SITR, 0,  1, {NULL}, 0, 0, S_SINKTRAIL2},				// S_SINKTRAIL1
	{SPR_SITR, 1,  5, {NULL}, 0, 0, S_SINKTRAIL3},				// S_SINKTRAIL2
	{SPR_SITR, 2,  3, {NULL}, 0, 0, S_NULL},						// S_SINKTRAIL3

	{SPR_KBLN, FF_FULLBRIGHT,   -1, {NULL}, 0, 0, S_NULL}, // S_BATTLEBUMPER1
	{SPR_KBLN, FF_FULLBRIGHT|1, -1, {NULL}, 0, 0, S_NULL}, // S_BATTLEBUMPER2
	{SPR_KBLN, FF_FULLBRIGHT|2, -1, {NULL}, 0, 0, S_NULL}, // S_BATTLEBUMPER3

	{SPR_BEXC, FF_SEMIBRIGHT,   1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALA2}, // S_BATTLEBUMPER_EXCRYSTALA1
	{SPR_BEXC, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALA3}, // S_BATTLEBUMPER_EXCRYSTALA2
	{SPR_BEXC, FF_SEMIBRIGHT,   1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALA4}, // S_BATTLEBUMPER_EXCRYSTALA3
	{SPR_BEXC, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALA1}, // S_BATTLEBUMPER_EXCRYSTALA4

	{SPR_BEXC, FF_SEMIBRIGHT|3, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALB2}, // S_BATTLEBUMPER_EXCRYSTALB1
	{SPR_BEXC, FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALB3}, // S_BATTLEBUMPER_EXCRYSTALB2
	{SPR_BEXC, FF_SEMIBRIGHT|3, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALB4}, // S_BATTLEBUMPER_EXCRYSTALB3
	{SPR_BEXC, FF_FULLBRIGHT|5, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALB1}, // S_BATTLEBUMPER_EXCRYSTALB4

	{SPR_BEXC, FF_SEMIBRIGHT|6, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALC2}, // S_BATTLEBUMPER_EXCRYSTALC1
	{SPR_BEXC, FF_FULLBRIGHT|7, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALC3}, // S_BATTLEBUMPER_EXCRYSTALC2
	{SPR_BEXC, FF_SEMIBRIGHT|6, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALC4}, // S_BATTLEBUMPER_EXCRYSTALC3
	{SPR_BEXC, FF_FULLBRIGHT|8, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXCRYSTALC1}, // S_BATTLEBUMPER_EXCRYSTALC4

	{SPR_BEXS, FF_FULLBRIGHT,   1, {NULL}, 0, 0, S_BATTLEBUMPER_EXSHELLA2}, // S_BATTLEBUMPER_EXSHELLA1
	{SPR_BEXS, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXSHELLA1}, // S_BATTLEBUMPER_EXSHELLA2

	{SPR_BEXS, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXSHELLB2}, // S_BATTLEBUMPER_EXSHELLB1
	{SPR_BEXS, FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXSHELLB1}, // S_BATTLEBUMPER_EXSHELLB2

	{SPR_BEXS, FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXSHELLC2}, // S_BATTLEBUMPER_EXSHELLC1
	{SPR_BEXS, FF_FULLBRIGHT|5, 1, {NULL}, 0, 0, S_BATTLEBUMPER_EXSHELLC1}, // S_BATTLEBUMPER_EXSHELLC2

	{SPR_BDEB, FF_FULLBRIGHT|FF_ANIMATE, 84, {NULL}, 13, 6, S_BATTLEBUMPER_EXDEBRIS2}, // S_BATTLEBUMPER_EXDEBRIS1
	{SPR_BDEB, FF_FULLBRIGHT|13, 20, {NULL}, 0, 0, S_NULL}, // S_BATTLEBUMPER_EXDEBRIS2

	{SPR_BEXB, FF_FULLBRIGHT|FF_PAPERSPRITE,            2, {NULL}, 0, 0, S_BATTLEBUMPER_EXBLAST2}, // S_BATTLEBUMPER_EXBLAST1
	{SPR_BEXB, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_TRANS10, 2, {NULL}, 0, 0, S_BATTLEBUMPER_EXBLAST3}, // S_BATTLEBUMPER_EXBLAST2
	{SPR_BEXB, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_TRANS20, 2, {NULL}, 0, 0, S_BATTLEBUMPER_EXBLAST4}, // S_BATTLEBUMPER_EXBLAST3
	{SPR_BEXB, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_TRANS30, 2, {NULL}, 0, 0, S_BATTLEBUMPER_EXBLAST5}, // S_BATTLEBUMPER_EXBLAST4
	{SPR_BEXB, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_TRANS40, 2, {NULL}, 0, 0, S_BATTLEBUMPER_EXBLAST6}, // S_BATTLEBUMPER_EXBLAST5
	{SPR_BEXB, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_TRANS50, 2, {NULL}, 0, 0, S_BATTLEBUMPER_EXBLAST7}, // S_BATTLEBUMPER_EXBLAST6
	{SPR_BEXB, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_TRANS60, 2, {NULL}, 0, 0, S_BATTLEBUMPER_EXBLAST8}, // S_BATTLEBUMPER_EXBLAST7
	{SPR_BEXB, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_TRANS70, 2, {NULL}, 0, 0, S_BATTLEBUMPER_EXBLAST9}, // S_BATTLEBUMPER_EXBLAST8
	{SPR_BEXB, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_TRANS80, 2, {NULL}, 0, 0, S_BATTLEBUMPER_EXBLAST10}, // S_BATTLEBUMPER_EXBLAST9
	{SPR_BEXB, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_TRANS90, 2, {NULL}, 0, 0, S_NULL}, // S_BATTLEBUMPER_EXBLAST10

	{SPR_TWBS, FF_FULLBRIGHT|FF_ADD|FF_ANIMATE,                                   -1, {NULL}, 6, 2, S_NULL}, // S_TRIPWIREBOOST_TOP
	{SPR_TWBS, FF_FULLBRIGHT|FF_ADD|FF_ANIMATE|FF_VERTICALFLIP|FF_HORIZONTALFLIP, -1, {NULL}, 6, 2, S_NULL}, // S_TRIPWIREBOOST_BOTTOM

	{SPR_TWBT, FF_FULLBRIGHT|FF_ADD|FF_ANIMATE,                                   -1, {NULL}, 6, 2, S_NULL}, // S_TRIPWIREBOOST_BLAST_TOP
	{SPR_TWBT, FF_FULLBRIGHT|FF_ADD|FF_ANIMATE|FF_VERTICALFLIP|FF_HORIZONTALFLIP, -1, {NULL}, 6, 2, S_NULL}, // S_TRIPWIREBOOST_BLAST_BOTTOM

	{SPR_TWBP, FF_FULLBRIGHT|FF_ADD|FF_ANIMATE, -1, {NULL}, 11, 1, S_NULL}, // S_TRIPWIREAPPROACH

	{SPR_SMLD, FF_FULLBRIGHT|FF_ADD|FF_ANIMATE, -1, {NULL}, 7, 2, S_NULL}, // S_SMOOTHLANDING

	{SPR_TRK1, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE|FF_ADD, -1, {NULL},  3, 3, S_NULL},      // S_TRICKINDICATOR_OVERLAY,
	{SPR_TRK2, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE,        -1, {NULL},  3, 3, S_NULL},      // S_TRICKINDICATOR_UNDERLAY,
	{SPR_TRK3, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE|FF_ADD, 13, {NULL}, 12, 1, S_INVISIBLE}, // S_TRICKINDICATOR_OVERLAY_ARROW,
	{SPR_NULL, 0, 1, {NULL}, 12, 1, S_TRICKINDICATOR_UNDERLAY_ARROW2}, // S_TRICKINDICATOR_UNDERLAY_ARROW,
	{SPR_TRK4, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE,        13, {NULL}, 12, 1, S_INVISIBLE}, // S_TRICKINDICATOR_UNDERLAY_ARROW2,

	{SPR_TRK5, FF_FULLBRIGHT|FF_PAPERSPRITE,            -1, {NULL}, 0, 0, S_NULL}, // S_SIDETRICK,
	{SPR_TRK6, FF_FULLBRIGHT|FF_PAPERSPRITE,            -1, {NULL}, 0, 0, S_NULL}, // S_BACKTRICK,
	{SPR_TRK7, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, -1, {NULL}, 6, 1, S_NULL}, // S_FORWARDTRICK,

	{SPR_TIRG, FF_ANIMATE, -1, {NULL}, 1, 1, S_NULL}, // S_TIREGRABBER
	{SPR_RSHT,               FF_PAPERSPRITE|0,         -1,              {NULL}, 0, 0, S_NULL}, // S_RINGSHOOTER_SIDE
	{SPR_RSHT, FF_SEMIBRIGHT|FF_PAPERSPRITE|2,         -1,              {NULL}, 0, 0, S_NULL}, // S_RINGSHOOTER_NIPPLES
	{SPR_RSHT,               FF_PAPERSPRITE|4,         -1,              {NULL}, 0, 0, S_NULL}, // S_RINGSHOOTER_SCREEN
	{SPR_RSHT, FF_FULLBRIGHT|FF_PAPERSPRITE|5,         -1,              {NULL}, 0, 0, S_NULL}, // S_RINGSHOOTER_NUMBERBACK
	{SPR_RSHT, FF_FULLBRIGHT|FF_PAPERSPRITE|9,         -1,              {NULL}, 0, 0, S_NULL}, // S_RINGSHOOTER_NUMBERFRONT
	{SPR_PLAY, FF_FULLBRIGHT|FF_PAPERSPRITE|SPR2_XTRA, -1, {A_RingShooterFace}, 0, 0, S_NULL}, // S_RINGSHOOTER_FACE

	{SPR_DEZL, FF_FULLBRIGHT|FF_PAPERSPRITE, 8, {NULL}, 0, 0, S_NULL}, // S_DEZLASER
	{SPR_DEZL, FF_FULLBRIGHT|1, 2, {NULL}, 0, 0, S_DEZLASER_TRAIL2}, // S_DEZLASER_TRAIL1
	{SPR_DEZL, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_DEZLASER_TRAIL3}, // S_DEZLASER_TRAIL2
	{SPR_DEZL, FF_FULLBRIGHT|FF_PAPERSPRITE|3, 4, {NULL}, 0, 0, S_DEZLASER_TRAIL4}, // S_DEZLASER_TRAIL3
	{SPR_DEZL, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_DEZLASER_TRAIL5}, // S_DEZLASER_TRAIL4
	{SPR_DEZL, FF_FULLBRIGHT|1, 2, {NULL}, 0, 0, S_NULL}, // S_DEZLASER_TRAIL5

	{SPR_FLAM,  FF_FULLBRIGHT|FF_ADD|0,  3, {NULL}, 0, 0, S_FLAYM2}, // S_FLAYM1,
	{SPR_FLAM,  FF_FULLBRIGHT|FF_ADD|1,  3, {NULL}, 0, 0, S_FLAYM3}, // S_FLAYM2,
	{SPR_FLAM,  FF_FULLBRIGHT|FF_ADD|2,  3, {NULL}, 0, 0, S_FLAYM4}, // S_FLAYM3,
	{SPR_FLAM,  FF_FULLBRIGHT|FF_ADD|3,  3, {NULL}, 0, 0, S_FLAYM1}, // S_FLAYM4,

	{SPR_SACO, 0, -1, {NULL}, 0, 0, S_NULL}, // S_PALMTREE2
	{SPR_SACO, 1, -1, {NULL}, 0, 0, S_NULL}, // S_PURPLEFLOWER1
	{SPR_SACO, 2, -1, {NULL}, 0, 0, S_NULL}, // S_PURPLEFLOWER2
	{SPR_SACO, 3, -1, {NULL}, 0, 0, S_NULL}, // S_YELLOWFLOWER1
	{SPR_SACO, 4, -1, {NULL}, 0, 0, S_NULL}, // S_YELLOWFLOWER2
	{SPR_SACO, 5, -1, {NULL}, 0, 0, S_NULL}, // S_PLANT2
	{SPR_SACO, 6, -1, {NULL}, 0, 0, S_NULL}, // S_PLANT3
	{SPR_SACO, 7, -1, {NULL}, 0, 0, S_NULL}, // S_PLANT4

	{SPR_BRNG, 0, 2, {NULL}, 0, 0, S_BIGRING02}, // S_BIGRING01
	{SPR_BRNG, 1, 2, {NULL}, 0, 0, S_BIGRING03}, // S_BIGRING02
	{SPR_BRNG, 2, 2, {NULL}, 0, 0, S_BIGRING04}, // S_BIGRING03
	{SPR_BRNG, 3, 2, {NULL}, 0, 0, S_BIGRING05}, // S_BIGRING04
	{SPR_BRNG, 4, 2, {NULL}, 0, 0, S_BIGRING06}, // S_BIGRING05
	{SPR_BRNG, 5, 2, {NULL}, 0, 0, S_BIGRING07}, // S_BIGRING06
	{SPR_BRNG, 6, 2, {NULL}, 0, 0, S_BIGRING08}, // S_BIGRING05
	{SPR_BRNG, 7, 2, {NULL}, 0, 0, S_BIGRING09}, // S_BIGRING05
	{SPR_BRNG, 8, 2, {NULL}, 0, 0, S_BIGRING10}, // S_BIGRING05
	{SPR_BRNG, 9, 2, {NULL}, 0, 0, S_BIGRING11}, // S_BIGRING10
	{SPR_BRNG, 10, 2, {NULL}, 0, 0, S_BIGRING12}, // S_BIGRING11
	{SPR_BRNG, 11, 2, {NULL}, 0, 0, S_BIGRING01}, // S_BIGRING12

	// Ark Arrows
	{SPR_SYM0, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_0
	{SPR_SYM1, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_1
	{SPR_SYM2, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_2
	{SPR_SYM3, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_3
	{SPR_SYM4, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_4
	{SPR_SYM5, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_5
	{SPR_SYM6, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_6
	{SPR_SYM7, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_7
	{SPR_SYM8, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_8
	{SPR_SYM9, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_9
	{SPR_SYMA, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_10
	{SPR_SYMB, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_11
	{SPR_SYMC, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_12
	{SPR_SYMD, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_13
	{SPR_SYME, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_14
	{SPR_SYMF, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_15
	{SPR_SYMG, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_16
	{SPR_SYMH, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_17
	{SPR_SYMI, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_18
	{SPR_SYMJ, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_19
	{SPR_SYMK, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_20
	{SPR_SYML, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_21
	{SPR_SYMM, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_22
	{SPR_SYMN, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_23
	{SPR_SYMO, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_24
	{SPR_SYMP, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_25
	{SPR_SYMQ, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_26
	{SPR_SYMR, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_27
	{SPR_SYMS, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_28
	{SPR_SYMT, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_29
	{SPR_SYMU, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_30
	{SPR_SYMV, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_31
	{SPR_SYMW, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_32
	{SPR_SYMX, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_33
	{SPR_SYMY, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_34
	{SPR_SYMZ, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_35
	{SPR_ARK0, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_36
	{SPR_ARK1, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_37
	{SPR_ARK2, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_38
	{SPR_ARK3, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_39
	{SPR_ARK4, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_40
	{SPR_ARK5, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 15, 2, S_NULL}, // S_ARKARROW_41

	{SPR_BUMP, FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_BUMP2}, // S_BUMP1
	{SPR_BUMP, FF_FULLBRIGHT|1, 3, {NULL}, 0, 0, S_BUMP3}, // S_BUMP2
	{SPR_BUMP, FF_FULLBRIGHT|2, 3, {NULL}, 0, 0, S_NULL}, // S_BUMP3

	{SPR_FLEN, FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_FLINGENERGY2}, // S_FLINGENERGY1,
	{SPR_FLEN, FF_FULLBRIGHT|1, 3, {NULL}, 0, 0, S_FLINGENERGY3}, // S_FLINGENERGY2,
	{SPR_FLEN, FF_FULLBRIGHT|2, 3, {NULL}, 0, 0, S_NULL}, // S_FLINGENERGY3,

	{SPR_CLAS, FF_FULLBRIGHT|FF_ADD, 2, {A_PlayActiveSound}, 0, 0, S_CLASH2}, // S_CLASH1
	{SPR_CLAS, FF_FULLBRIGHT|FF_ADD|1, 2, {NULL}, 0, 0, S_CLASH3}, // S_CLASH2
	{SPR_CLAS, FF_FULLBRIGHT|FF_ADD|2, 2, {NULL}, 0, 0, S_CLASH4}, // S_CLASH3
	{SPR_CLAS, FF_FULLBRIGHT|FF_ADD|3, 2, {NULL}, 0, 0, S_CLASH5}, // S_CLASH4
	{SPR_CLAS, FF_FULLBRIGHT|FF_ADD|4, 2, {NULL}, 0, 0, S_CLASH6}, // S_CLASH5
	{SPR_CLAS, FF_FULLBRIGHT|FF_ADD|5, 2, {NULL}, 0, 0, S_NULL}, // S_CLASH6

	{SPR_PSHW, FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_FIREDITEM2}, // S_FIREDITEM1
	{SPR_PSHW, FF_FULLBRIGHT|1, 3, {NULL}, 0, 0, S_FIREDITEM3}, // S_FIREDITEM2
	{SPR_PSHW, FF_FULLBRIGHT|2, 3, {NULL}, 0, 0, S_FIREDITEM4}, // S_FIREDITEM3
	{SPR_PSHW, FF_FULLBRIGHT|3, 3, {NULL}, 0, 0, S_NULL}, // S_FIREDITEM4

	{SPR_ISTA, FF_FULLBRIGHT|FF_TRANS30, 2, {NULL}, 0, 0, S_INSTASHIELDA2},	// S_INSTASHIELDA1
	{SPR_ISTA, FF_FULLBRIGHT|FF_TRANS30|1, 2, {NULL}, 0, 0, S_INSTASHIELDA3},	// S_INSTASHIELDA2
	{SPR_ISTA, FF_FULLBRIGHT|FF_TRANS30|2, 2, {NULL}, 0, 0, S_INSTASHIELDA4},	// S_INSTASHIELDA3
	{SPR_ISTA, FF_FULLBRIGHT|FF_TRANS30|3, 2, {NULL}, 0, 0, S_INSTASHIELDA5},	// S_INSTASHIELDA4
	{SPR_ISTA, FF_FULLBRIGHT|FF_TRANS30|4, 2, {NULL}, 0, 0, S_INSTASHIELDA6},	// S_INSTASHIELDA5
	{SPR_ISTA, FF_FULLBRIGHT|FF_TRANS30|5, 2, {NULL}, 0, 0, S_INSTASHIELDA7},	// S_INSTASHIELDA6
	{SPR_ISTA, FF_FULLBRIGHT|FF_TRANS30|6, 2, {NULL}, 0, 0, S_NULL},			// S_INSTASHIELDA7

	{SPR_ISTB, FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_INSTASHIELDB2},		// S_INSTASHIELDB1
	{SPR_ISTB, FF_FULLBRIGHT|1, 2, {NULL}, 0, 0, S_INSTASHIELDB3},	// S_INSTASHIELDB2
	{SPR_ISTB, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_INSTASHIELDB4},	// S_INSTASHIELDB3
	{SPR_ISTB, FF_FULLBRIGHT|3, 2, {NULL}, 0, 0, S_INSTASHIELDB5},	// S_INSTASHIELDB4
	{SPR_ISTB, FF_FULLBRIGHT|4, 2, {NULL}, 0, 0, S_INSTASHIELDB6},	// S_INSTASHIELDB5
	{SPR_ISTB, FF_FULLBRIGHT|5, 2, {NULL}, 0, 0, S_INSTASHIELDB7},	// S_INSTASHIELDB6
	{SPR_ISTB, FF_FULLBRIGHT|6, 2, {NULL}, 0, 0, S_NULL},				// S_INSTASHIELDB7

	{SPR_PWCL, FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE, 10, {NULL}, 9, 1, S_NULL}, // S_POWERCLASH
	{SPR_GBRK, FF_ADD|FF_FULLBRIGHT|FF_ANIMATE|FF_PAPERSPRITE, 24, {NULL}, 5, 4, S_NULL}, // S_GUARDBREAK

	{SPR_SPBM, 0, 1, {NULL}, 0, 0,  S_PLAYERBOMB2}, // S_PLAYERBOMB1
	{SPR_SPBM, 1, 1, {NULL}, 0, 0,  S_PLAYERBOMB3}, // S_PLAYERBOMB2
	{SPR_SPBM, 0, 1, {NULL}, 0, 0,  S_PLAYERBOMB4}, // S_PLAYERBOMB3
	{SPR_SPBM, 2, 1, {NULL}, 0, 0,  S_PLAYERBOMB5}, // S_PLAYERBOMB4
	{SPR_SPBM, 0, 1, {NULL}, 0, 0,  S_PLAYERBOMB6}, // S_PLAYERBOMB5
	{SPR_SPBM, 3, 1, {NULL}, 0, 0,  S_PLAYERBOMB7}, // S_PLAYERBOMB6
	{SPR_SPBM, 0, 1, {NULL}, 0, 0,  S_PLAYERBOMB8}, // S_PLAYERBOMB7
	{SPR_SPBM, 4, 1, {NULL}, 0, 0,  S_PLAYERBOMB9}, // S_PLAYERBOMB8
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_PLAYERBOMB10}, // S_PLAYERBOMB9
	{SPR_SPBM, 5, 1, {NULL}, 0, 0, S_PLAYERBOMB11}, // S_PLAYERBOMB10
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_PLAYERBOMB12}, // S_PLAYERBOMB11
	{SPR_SPBM, 6, 1, {NULL}, 0, 0, S_PLAYERBOMB13}, // S_PLAYERBOMB12
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_PLAYERBOMB14}, // S_PLAYERBOMB13
	{SPR_SPBM, 7, 1, {NULL}, 0, 0, S_PLAYERBOMB15}, // S_PLAYERBOMB14
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_PLAYERBOMB16}, // S_PLAYERBOMB15
	{SPR_SPBM, 8, 1, {NULL}, 0, 0, S_PLAYERBOMB17}, // S_PLAYERBOMB16
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_PLAYERBOMB18}, // S_PLAYERBOMB17
	{SPR_SPBM, 8, 1, {NULL}, 0, 0, S_PLAYERBOMB19}, // S_PLAYERBOMB18
	{SPR_SPBM, 0, 1, {NULL}, 0, 0, S_PLAYERBOMB20}, // S_PLAYERBOMB19
	{SPR_SPBM, 8, 1, {NULL}, 0, 0,  S_PLAYERBOMB1}, // S_PLAYERBOMB20

	{SPR_RNDM,    FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM2},	// S_PLAYERITEM1
	{SPR_RNDM,  2|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM3},	// S_PLAYERITEM2
	{SPR_RNDM,  4|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM4},	// S_PLAYERITEM3
	{SPR_RNDM,  6|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM5},	// S_PLAYERITEM4
	{SPR_RNDM,  8|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM6},	// S_PLAYERITEM5
	{SPR_RNDM, 10|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM7},	// S_PLAYERITEM6
	{SPR_RNDM, 12|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM8},	// S_PLAYERITEM7
	{SPR_RNDM, 14|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM9},	// S_PLAYERITEM8
	{SPR_RNDM, 16|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM10},	// S_PLAYERITEM9
	{SPR_RNDM, 18|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM11},	// S_PLAYERITEM10
	{SPR_RNDM, 20|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM12},	// S_PLAYERITEM11
	{SPR_RNDM, 22|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERITEM1},	// S_PLAYERITEM12

	{SPR_FITM,    FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE2},	// S_PLAYERFAKE1
	{SPR_FITM,  2|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE3},	// S_PLAYERFAKE2
	{SPR_FITM,  4|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE4},	// S_PLAYERFAKE3
	{SPR_FITM,  6|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE5},	// S_PLAYERFAKE4
	{SPR_FITM,  8|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE6},	// S_PLAYERFAKE5
	{SPR_FITM, 10|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE7},	// S_PLAYERFAKE6
	{SPR_FITM, 12|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE8},	// S_PLAYERFAKE7
	{SPR_FITM, 14|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE9},	// S_PLAYERFAKE8
	{SPR_FITM, 16|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE10},	// S_PLAYERFAKE9
	{SPR_FITM, 18|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE11},	// S_PLAYERFAKE10
	{SPR_FITM, 20|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE12},	// S_PLAYERFAKE11
	{SPR_FITM, 22|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, 4, {NULL}, 1, 1, S_PLAYERFAKE1},	// S_PLAYERFAKE12

	{SPR_PBOM, 0, -1, {NULL}, 0, 0, S_NULL}, // S_KARMAWHEEL

	{SPR_HIT1, FF_FULLBRIGHT, 7, {NULL}, 0, 0, S_BATTLEPOINT1B}, // S_BATTLEPOINT1A
	{SPR_HIT1, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_BATTLEPOINT1C}, // S_BATTLEPOINT1B
	{SPR_HIT1, FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_BATTLEPOINT1D}, // S_BATTLEPOINT1C
	{SPR_HIT1, FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_BATTLEPOINT1E}, // S_BATTLEPOINT1D
	{SPR_HIT1, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_BATTLEPOINT1F}, // S_BATTLEPOINT1E
	{SPR_HIT1, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_BATTLEPOINT1G}, // S_BATTLEPOINT1F
	{SPR_HIT1, FF_FULLBRIGHT|2, TICRATE, {NULL}, 0, 0, S_BATTLEPOINT1H}, // S_BATTLEPOINT1G
	{SPR_HIT1, FF_FULLBRIGHT|5, 1, {NULL}, 0, 0, S_BATTLEPOINT1I}, // S_BATTLEPOINT1H
	{SPR_HIT1, FF_FULLBRIGHT|6, 1, {NULL}, 0, 0, S_NULL}, // S_BATTLEPOINT1I

	{SPR_HIT2, FF_FULLBRIGHT, 7, {NULL}, 0, 0, S_BATTLEPOINT2B}, // S_BATTLEPOINT2A
	{SPR_HIT2, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_BATTLEPOINT2C}, // S_BATTLEPOINT2B
	{SPR_HIT2, FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_BATTLEPOINT2D}, // S_BATTLEPOINT2C
	{SPR_HIT2, FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_BATTLEPOINT2E}, // S_BATTLEPOINT2D
	{SPR_HIT2, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_BATTLEPOINT2F}, // S_BATTLEPOINT2E
	{SPR_HIT2, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_BATTLEPOINT2G}, // S_BATTLEPOINT2F
	{SPR_HIT2, FF_FULLBRIGHT|2, TICRATE, {NULL}, 0, 0, S_BATTLEPOINT2H}, // S_BATTLEPOINT2G
	{SPR_HIT2, FF_FULLBRIGHT|5, 1, {NULL}, 0, 0, S_BATTLEPOINT2I}, // S_BATTLEPOINT2H
	{SPR_HIT2, FF_FULLBRIGHT|6, 1, {NULL}, 0, 0, S_NULL}, // S_BATTLEPOINT2I

	{SPR_HIT3, FF_FULLBRIGHT, 7, {NULL}, 0, 0, S_BATTLEPOINT3B}, // S_BATTLEPOINT3A
	{SPR_HIT3, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_BATTLEPOINT3C}, // S_BATTLEPOINT3B
	{SPR_HIT3, FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_BATTLEPOINT3D}, // S_BATTLEPOINT3C
	{SPR_HIT3, FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_BATTLEPOINT3E}, // S_BATTLEPOINT3D
	{SPR_HIT3, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_BATTLEPOINT3F}, // S_BATTLEPOINT3E
	{SPR_HIT3, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_BATTLEPOINT3G}, // S_BATTLEPOINT3F
	{SPR_HIT3, FF_FULLBRIGHT|2, TICRATE, {NULL}, 0, 0, S_BATTLEPOINT3H}, // S_BATTLEPOINT3G
	{SPR_HIT3, FF_FULLBRIGHT|5, 1, {NULL}, 0, 0, S_BATTLEPOINT3I}, // S_BATTLEPOINT3H
	{SPR_HIT3, FF_FULLBRIGHT|6, 1, {NULL}, 0, 0, S_NULL}, // S_BATTLEPOINT3I

	// Oh no it's annoying lightning states.......
	// Lightning Sparks (it's the ones we'll use for the radius)
	{SPR_KSPK, FF_ADD|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_KSPARK2},	// S_KSPARK1
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 1, {A_LightningFollowPlayer}, 0, 0, S_KSPARK3},	// S_KSPARK2
	{SPR_KSPK, FF_ADD|FF_FULLBRIGHT|1, 2, {A_LightningFollowPlayer}, 0, 0, S_KSPARK4},	// S_KSPARK3
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 1, {A_LightningFollowPlayer}, 0, 0, S_KSPARK5},	// S_KSPARK4
	{SPR_KSPK, FF_ADD|FF_FULLBRIGHT|2, 2, {A_LightningFollowPlayer}, 0, 0, S_KSPARK6},	// S_KSPARK5
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 1, {A_LightningFollowPlayer}, 0, 0, S_KSPARK7},	// S_KSPARK6
	{SPR_KSPK, FF_ADD|FF_FULLBRIGHT|3, 2, {A_LightningFollowPlayer}, 0, 0, S_KSPARK8},	// S_KSPARK7
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 1, {A_LightningFollowPlayer}, 0, 0, S_KSPARK9},	// S_KSPARK8
	{SPR_KSPK, FF_ADD|FF_TRANS40|FF_FULLBRIGHT|4, 2, {A_LightningFollowPlayer}, 0, 0, S_KSPARK10},	// S_KSPARK9
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 1, {A_LightningFollowPlayer}, 0, 0, S_KSPARK11},	// S_KSPARK10
	{SPR_KSPK, FF_ADD|FF_TRANS50|FF_FULLBRIGHT|5, 2, {A_LightningFollowPlayer}, 0, 0, S_KSPARK12},	// S_KSPARK11
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 1, {A_LightningFollowPlayer}, 0, 0, S_KSPARK13},	// S_KSPARK12
	{SPR_KSPK, FF_ADD|FF_TRANS60|FF_FULLBRIGHT|6, 2, {A_LightningFollowPlayer}, 0, 0, S_NULL},	// S_KSPARK13

	// The straight bolt...
	{SPR_LZI1, FF_ADD|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO12},	// S_LZIO11
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO13},	// S_LZIO12
	{SPR_LZI1, FF_ADD|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO14},	// S_LZIO13
	{SPR_LZI1, FF_ADD|FF_FULLBRIGHT|1, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO15},	// S_LZIO14
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 4, {A_LightningFollowPlayer}, 0, 0, S_LZIO16},	// S_LZIO15
	{SPR_LZI1, FF_ADD|FF_FULLBRIGHT|1, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO17},	// S_LZIO16
	{SPR_NULL, 0, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO18}, // S_LZIO17
	{SPR_LZI1, FF_ADD|FF_TRANS50|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO19},	// S_LZIO18
	{SPR_LZI1, FF_ADD|FF_TRANS70|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_NULL},	// S_LZIO19

	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 6, {A_LightningFollowPlayer}, 0, 0, S_LZIO22},	// S_LZIO21
	{SPR_LZI2, FF_ADD|FF_FULLBRIGHT|1, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO23},	// S_LZIO22
	{SPR_LZI2, FF_ADD|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO24},	// S_LZIO23
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO25},	// S_LZIO24
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO26},	// S_LZIO25
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO27},	// S_LZIO26
	{SPR_LZI2, FF_ADD|FF_TRANS30|FF_FULLBRIGHT|2, 2, {A_LightningFollowPlayer}, 0, 0, S_LZIO28}, // S_LZIO27
	{SPR_NULL, 0, 4, {A_LightningFollowPlayer}, 0, 0, S_LZIO29}, // S_LZIO28
	{SPR_LZI2, FF_ADD|FF_TRANS70|FF_FULLBRIGHT, 2, {A_LightningFollowPlayer}, 0, 0, S_NULL}, // S_LZIO29

	// The slanted bolt. Man these states are boring as all heck to do.
	{SPR_KLIT, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT2},		// S_KLIT1
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT3},		// S_KLIT2
	{SPR_KLIT, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE|1, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT4},	// S_KLIT3
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT5},		// S_KLIT4
	{SPR_KLIT, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE|2, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT6},	// S_KLIT5
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT7},		// S_KLIT6
	{SPR_KLIT, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE|3, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT8},	// S_KLIT7
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT9},		// S_KLIT8
	{SPR_KLIT, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE|4, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT10},	// S_KLIT9
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT11},	    // S_KLIT10
	{SPR_KLIT, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE|5, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT12},	// S_KLIT11
	{SPR_NULL, FF_ADD|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {A_LightningFollowPlayer}, 0, 0, S_KLIT1},		// S_KLIT12

	{SPR_FZSM, 0, 4, {NULL}, 0, 0, S_FZEROSMOKE2}, // S_FZEROSMOKE1
	{SPR_FZSM, 1, 4, {NULL}, 0, 0, S_FZEROSMOKE3}, // S_FZEROSMOKE2
	{SPR_FZSM, 2, 4, {NULL}, 0, 0, S_FZEROSMOKE4}, // S_FZEROSMOKE3
	{SPR_FZSM, 3, 4, {NULL}, 0, 0, S_FZEROSMOKE5}, // S_FZEROSMOKE4
	{SPR_FZSM, 4, 4, {NULL}, 0, 0, S_NULL},        // S_FZEROSMOKE5

	{SPR_FZBM, FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_FZEROBOOM2},   // S_FZEROBOOM1
	{SPR_FZBM, FF_FULLBRIGHT|1, 2, {NULL}, 0, 0, S_FZEROBOOM3}, // S_FZEROBOOM2
	{SPR_FZBM, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_FZEROBOOM4}, // S_FZEROBOOM3
	{SPR_FZBM, FF_FULLBRIGHT|3, 2, {A_FZBoomFlash}, 0, 0, S_FZEROBOOM5}, // S_FZEROBOOM4
	{SPR_FZBM, FF_FULLBRIGHT|2, 1, {A_FZBoomSmoke}, 0, 0, S_FZEROBOOM6}, // S_FZEROBOOM5
	{SPR_FZBM, FF_FULLBRIGHT|1, 1, {A_FZBoomSmoke}, 0, 0, S_FZEROBOOM7}, // S_FZEROBOOM6
	{SPR_NULL, 0, 1, {A_FZBoomSmoke}, 1, 0, S_FZEROBOOM8},  // S_FZEROBOOM7
	{SPR_NULL, 0, 1, {A_FZBoomSmoke}, 2, 0, S_FZEROBOOM9},  // S_FZEROBOOM8
	{SPR_NULL, 0, 1, {A_FZBoomSmoke}, 3, 0, S_FZEROBOOM10}, // S_FZEROBOOM9
	{SPR_NULL, 0, 1, {A_FZBoomSmoke}, 2, 0, S_FZEROBOOM11}, // S_FZEROBOOM10
	{SPR_NULL, 0, 1, {A_FZBoomSmoke}, 1, 0, S_FZEROBOOM12}, // S_FZEROBOOM11
	{SPR_NULL, 0, 1, {A_FZBoomSmoke}, 0, 0, S_NULL},        // S_FZEROBOOM12

	{SPR_SMOK, 0, 30, {NULL}, 0, 0, S_FZSLOWSMOKE2},	// S_FZSLOWSMOKE1
	{SPR_SMOK, 1, 30, {NULL}, 0, 0, S_FZSLOWSMOKE3},	// S_FZSLOWSMOKE2
	{SPR_SMOK, 2, 30, {NULL}, 0, 0, S_FZSLOWSMOKE4},	// S_FZSLOWSMOKE3
	{SPR_SMOK, 3, 30, {NULL}, 0, 0, S_FZSLOWSMOKE5},	// S_FZSLOWSMOKE4
	{SPR_SMOK, 4, 30, {NULL}, 0, 0, S_NULL},			// S_FZSLOWSMOKE5

	// Dash Rings
	{SPR_RAIR, 0, -1, {NULL}, 0, 0, S_NULL}, // S_DASHRING_HORIZONTAL
	{SPR_RAIR, 1, -1, {NULL}, 0, 0, S_NULL}, // S_DASHRING_30DEGREES
	{SPR_RAIR, 2, -1, {NULL}, 0, 0, S_NULL}, // S_DASHRING_60DEGREES
	{SPR_RAIR, 3, -1, {NULL}, 0, 0, S_NULL}, // S_DASHRING_VERTICAL
	{SPR_NULL,        0, TICRATE/3 - 2, {NULL}, 0, 0, S_DASHRING_HORIZONTAL_FLASH2}, // S_DASHRING_HORIZONTAL_FLASH1
	{SPR_RAIR, FF_ADD|0,             2, {NULL}, 0, 0, S_DASHRING_HORIZONTAL_FLASH1}, // S_DASHRING_HORIZONTAL_FLASH2
	{SPR_NULL,        0, TICRATE/3 - 2, {NULL}, 0, 0, S_DASHRING_30DEGREES_FLASH2},  // S_DASHRING_30DEGREES_FLASH1
	{SPR_RAIR, FF_ADD|1,             2, {NULL}, 0, 0, S_DASHRING_30DEGREES_FLASH1},  // S_DASHRING_30DEGREES_FLASH2
	{SPR_NULL,        0, TICRATE/3 - 2, {NULL}, 0, 0, S_DASHRING_60DEGREES_FLASH2},  // S_DASHRING_60DEGREES_FLASH1
	{SPR_RAIR, FF_ADD|2,             2, {NULL}, 0, 0, S_DASHRING_60DEGREES_FLASH1},  // S_DASHRING_60DEGREES_FLASH2
	{SPR_NULL,        0, TICRATE/3 - 2, {NULL}, 0, 0, S_DASHRING_VERTICAL_FLASH2},   // S_DASHRING_VERTICAL_FLASH1
	{SPR_RAIR, FF_ADD|3,             2, {NULL}, 0, 0, S_DASHRING_VERTICAL_FLASH1},   // S_DASHRING_VERTICAL_FLASH2

	// Adventure Air Booster
	{SPR_ADVE, 13|FF_FULLBRIGHT|FF_ADD, 1, {A_RollAngle}, 8, 0, S_ADVENTUREAIRBOOSTER}, // S_ADVENTUREAIRBOOSTER
	{SPR_ADVE,  1|FF_FULLBRIGHT|FF_ADD|FF_ANIMATE|FF_PAPERSPRITE, 10, {NULL}, 4, 2, S_NULL}, // S_ADVENTUREAIRBOOSTER_EXHAUST1
	{SPR_ADVE,  7|FF_FULLBRIGHT|FF_ADD|FF_ANIMATE|FF_PAPERSPRITE, 10, {NULL}, 4, 2, S_NULL}, // S_ADVENTUREAIRBOOSTER_EXHAUST2
	{SPR_ADVR,  0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_ADVENTUREAIRBOOSTER_FRAME
	{SPR_ADVE,  0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_ADVENTUREAIRBOOSTER_ARROW

	// Sneaker Panels
	{SPR_BSTP, FF_ANIMATE|FF_GLOBALANIM|FF_FLOORSPRITE|FF_FULLBRIGHT, -1, {NULL}, 5, 2, S_SNEAKERPANEL},       // S_SNEAKERPANEL
	{SPR_BSTS, FF_ANIMATE|FF_GLOBALANIM|FF_FLOORSPRITE|FF_FULLBRIGHT, -1, {NULL}, 5, 2, S_SNEAKERPANEL_SMALL}, // S_SNEAKERPANEL_SMALL
	{SPR_BSTT, FF_ANIMATE|FF_GLOBALANIM|FF_FLOORSPRITE|FF_FULLBRIGHT, -1, {NULL}, 5, 2, S_SNEAKERPANEL_TINY},  // S_SNEAKERPANEL_TINY

	// Marble Zone
	{SPR_MARB, FF_FULLBRIGHT|FF_ANIMATE|5, TICRATE, {NULL}, 3, 3, S_NULL}, // S_MARBLEFLAMEPARTICLE
	{SPR_MARB, FF_FULLBRIGHT|FF_ANIMATE, 8*3, {A_FlameParticle}, 3, 3, S_MARBLETORCH}, // S_MARBLETORCH
	{SPR_MARB, FF_FULLBRIGHT|FF_TRANS80|4, -1, {NULL}, 1, 29, S_NULL}, // S_MARBLELIGHT
	{SPR_MARB, 9, -1, {NULL}, 0, 0, S_NULL}, // S_MARBLEBURNER

	// CD Special Stage
	{SPR_FUFO, 0, 1, {A_Thrust},     5, 2,          S_CDUFO},     // S_CDUFO
	{SPR_FUFO, 0, 4, {A_BossScream}, 0, MT_EXPLODE, S_CDUFO_DIE}, // S_CDUFO_DIE

	// Rusty Rig
	{SPR_RUST, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_RUSTYLAMP_ORANGE
	{SPR_RUST, 1,             -1, {NULL}, 0, 0, S_NULL}, // S_RUSTYCHAIN

	// Ports of gardens
	{SPR_PGTR, 0, -1, {NULL}, 0, 0, S_NULL}, // S_PGTREE

	// Daytona Speedway
	{SPR_PINE, 1, -1, {NULL}, 0, 0, S_NULL}, // S_DAYTONAPINETREE
	{SPR_PINE, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_DAYTONAPINETREE_SIDE

	// Egg Zeppelin
	{SPR_PPLR, 0, -1, {NULL}, 0, 0, S_EZZPROPELLER}, // S_EZZPROPELLER
	{SPR_PPLR, FF_PAPERSPRITE|1, -1, {NULL}, 0, 0, S_EZZPROPELLER_BLADE}, // S_EZZPROPELLER_BLADE

	// Desert Palace
	{SPR_DPPT, 0, -1, {NULL}, 0, 0, S_NULL}, // S_DP_PALMTREE

	// Aurora Atoll
	{SPR_AATR, 0, -1, {NULL}, 0, 0, S_AAZTREE_SEG}, // S_AAZTREE_SEG
	{SPR_COCO, 0, -1, {NULL}, 0, 0, S_AAZTREE_COCONUT}, // S_AAZTREE_COCONUT
	{SPR_AATR, FF_PAPERSPRITE|1, -1, {NULL}, 0, 0, S_AAZTREE_LEAF}, // S_AAZTREE_LEAF

	// Barren Badlands
	{SPR_BDST, FF_TRANS80, 35, {NULL}, 0, 0, S_BBZDUST2}, // S_BBZDUST1
	{SPR_BDST, FF_TRANS80|1, 12, {NULL}, 0, 0, S_BBZDUST3}, // S_BBZDUST2
	{SPR_BDST, FF_TRANS80|2, 11, {NULL}, 0, 0, S_BBZDUST4}, // S_BBZDUST3
	{SPR_BDST, FF_TRANS80|3, 10, {NULL}, 0, 0, S_NULL}, // S_BBZDUST4

	{SPR_FROG, 0, -1, {NULL}, 0, 0, S_FROGGER}, // S_FROGGER
	{SPR_FROG, 1, -1, {NULL}, 0, 0, S_FROGGER_ATTACK}, // S_FROGGER_ATTACK
	{SPR_FROG, 2, -1, {NULL}, 0, 0, S_FROGGER_JUMP}, // S_FROGGER_JUMP

	{SPR_FROG, 3, 7*TICRATE/2, {NULL}, 0, 0, S_NULL}, // S_FROGTONGUE
	{SPR_FROG, 4, 7*TICRATE/2, {NULL}, 0, 0, S_NULL}, // S_FROGTONGUE_JOINT

	{SPR_HOLE, 0, 1, {NULL}, 0, 0, S_ROBRA}, // S_ROBRA
	{SPR_CBRA, 0, 1, {NULL}, 0, 0, S_ROBRA_HEAD}, // S_ROBRA_HEAD
	{SPR_CBRA, 1, -1, {NULL}, 0, 0, S_ROBRA_JOINT}, // S_ROBRA_JOINT
	{SPR_CBRA, 2, -1, {NULL}, 1, 0, S_ROBRASHELL_INSIDE}, // S_ROBRASHELL_INSIDE
	{SPR_CBRA, 3, -1, {NULL}, 0, 0, S_ROBRASHELL_OUTSIDE}, // S_ROBRASHELL_OUTSIDE

	{SPR_HOLE, 0, 1, {NULL}, 0, 0, S_BLUEROBRA}, // S_BLUEROBRA
	{SPR_BBRA, 0, 1, {NULL}, 0, 0, S_BLUEROBRA_HEAD}, // S_BLUEROBRA_HEAD
	{SPR_BBRA, 1, -1, {NULL}, 0, 0, S_BLUEROBRA_JOINT}, // S_BLUEROBRA_JOINT

	// Eerie Grove
	{SPR_EGFG, FF_TRANS90|FF_FULLBRIGHT, 7, {A_SetRandomTics}, 5, 9, S_EERIEFOG2}, // S_EERIEFOG1
	{SPR_EGFG, FF_TRANS90|FF_FULLBRIGHT|1, 7, {A_SetRandomTics}, 5, 9, S_EERIEFOG3}, // S_EERIEFOG2
	{SPR_EGFG, FF_TRANS90|FF_FULLBRIGHT|2, 7, {A_SetRandomTics}, 5, 9, S_EERIEFOG4}, // S_EERIEFOG3
	{SPR_EGFG, FF_TRANS90|FF_FULLBRIGHT|3, 7, {A_SetRandomTics}, 5, 9, S_EERIEFOG5}, // S_EERIEFOG4
	{SPR_EGFG, FF_TRANS90|FF_FULLBRIGHT|4, 7, {A_SetRandomTics}, 5, 9, S_EERIEFOG1}, // S_EERIEFOG5

	// Chaos Chute
	{SPR_SARC,           FF_PAPERSPRITE|0, -1, {NULL}, 0, 0, S_NULL}, // S_SPECIALSTAGEARCH
	{SPR_SSBM, FF_GLOBALANIM|FF_ANIMATE|0,         -1, {NULL}, 3, 3, S_NULL}, // S_SPECIALSTAGEBOMB
	{SPR_SSBM,                          0,          1, {A_SetObjectFlags}, MF_NOCLIPTHING, 2, S_SPECIALSTAGEBOMB_EXPLODE}, // S_SPECIALSTAGEBOMB_DISARM
	{SPR_NULL,                          0,          0, {A_SpecialStageBombExplode}, 0, 0, S_SPECIALSTAGEBOMB_DISAPPEAR}, // S_SPECIALSTAGEBOMB_EXPLODE
	{SPR_NULL,                          0, 28*TICRATE, {A_Pain}, 0, 0, S_SPECIALSTAGEBOMB_FLICKER1}, // S_SPECIALSTAGEBOMB_DISAPPEAR
	{SPR_SSBM, FF_GLOBALANIM|FF_ANIMATE|0,          1, {NULL}, 3, 3, S_SPECIALSTAGEBOMB_FLICKER2}, // S_SPECIALSTAGEBOMB_FLICKER1
	{SPR_NULL,                          0,          1, {NULL}, 0, 0, S_SPECIALSTAGEBOMB_FLICKERLOOP}, // S_SPECIALSTAGEBOMB_FLICKER2
	{SPR_NULL,                          0,          0, {A_Repeat}, TICRATE, S_SPECIALSTAGEBOMB_FLICKER1, S_SPECIALSTAGEBOMB_RESET}, // S_SPECIALSTAGEBOMB_FLICKERLOOP
	{SPR_NULL,                          0,          0, {A_SetObjectFlags}, MF_NOCLIPTHING, 1, S_SPECIALSTAGEBOMB}, // S_SPECIALSTAGEBOMB_RESET

	// Hanagumi Hall
	{SPR_HGSP, FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 10, 1, S_NULL}, // S_HANAGUMIHALL_STEAM
	{SPR_HGC0, 0, -1, {NULL}, 0, 0, S_NULL}, // S_ALFONSO
	{SPR_HGCA, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SAKURA
	{SPR_HGCB, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SUMIRE
	{SPR_HGCC, 0, -1, {NULL}, 0, 0, S_NULL}, // S_MARIA
	{SPR_HGCD, 0, -1, {NULL}, 0, 0, S_NULL}, // S_IRIS
	{SPR_HGCE, 0, -1, {NULL}, 0, 0, S_NULL}, // S_KOHRAN
	{SPR_HGCF, 0, -1, {NULL}, 0, 0, S_NULL}, // S_KANNA
	{SPR_HGCG, 0, -1, {NULL}, 0, 0, S_NULL}, // S_OGAMI

	// Dimension Disaster
	{SPR_DVDD, 1, -1, {NULL}, 0, 0, S_NULL}, // S_DVDTRUMPET
	{SPR_SPRC, 1|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_DVDSHINE2}, // S_DVDSHINE1
	{SPR_SPRC, 2|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_DVDSHINE3}, // S_DVDSHINE2
	{SPR_SPRC, 3|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_DVDSHINE4}, // S_DVDSHINE3
	{SPR_SPRC, 4|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_DVDSHINE5}, // S_DVDSHINE4
	{SPR_SPRC, 5|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_DVDSHINE1}, // S_DVDSHINE5
	{SPR_SPRC,            FF_FULLBRIGHT|6, 3, {NULL}, 0, 0, S_DVDSPARK2}, // S_DVDSPARK1
	{SPR_SPRC, FF_TRANS20|FF_FULLBRIGHT|7, 3, {NULL}, 0, 0, S_DVDSPARK3}, // S_DVDSPARK2
	{SPR_SPRC, FF_TRANS40|FF_FULLBRIGHT|8, 3, {NULL}, 0, 0, S_NULL},      // S_DVDSPARK3

	{SPR_TUST, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_SUNBEAMPALM_STEM
	{SPR_TULE, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_SUNBEAMPALM_LEAF

	{SPR_FWRK,   FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_KARMAFIREWORK2}, // S_KARMAFIREWORK1
	{SPR_FWRK, 1|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_KARMAFIREWORK3}, // S_KARMAFIREWORK2
	{SPR_FWRK, 2|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_KARMAFIREWORK4}, // S_KARMAFIREWORK3
	{SPR_FWRK, 3|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_KARMAFIREWORK1}, // S_KARMAFIREWORK4
	{SPR_FWRK, 4|FF_FULLBRIGHT, TICRATE, {NULL}, 0, 0, S_NULL}, // S_KARMAFIREWORKTRAIL

	// Opaque smoke
	{SPR_SMOK, 0, 4, {NULL}, 0, 0, S_OPAQUESMOKE2}, // S_OPAQUESMOKE1
	{SPR_SMOK, 1, 5, {NULL}, 0, 0, S_OPAQUESMOKE3}, // S_OPAQUESMOKE2
	{SPR_SMOK, 2, 6, {NULL}, 0, 0, S_OPAQUESMOKE4}, // S_OPAQUESMOKE3
	{SPR_SMOK, 3, 7, {NULL}, 0, 0, S_OPAQUESMOKE5}, // S_OPAQUESMOKE4
	{SPR_SMOK, 4, 8, {NULL}, 0, 0, S_NULL},         // S_OPAQUESMOKE5


	// followers:

	// bubble
	{SPR_FBUB, 11|FF_ANIMATE|FF_TRANS70|FF_FULLBRIGHT, -1, {NULL}, 10, 3, S_FOLLOWERBUBBLE_FRONT},	// S_FOLLOWERBUBBLE_FRONT
	{SPR_FBUB, FF_ANIMATE|0|FF_FULLBRIGHT, -1, {NULL}, 10, 3, S_FOLLOWERBUBBLE_BACK},	// S_FOLLOWERBUBBLE_BACK

	// generic chao:
	{SPR_GCHA, FF_ANIMATE, -1, {NULL}, 1, 4, S_GCHAOIDLE},		//S_GCHAOIDLE
	{SPR_GCHA, 2|FF_ANIMATE, -1, {NULL}, 1, 2, S_GCHAOFLY},	//S_GCHAOFLY
	{SPR_GCHA, 7, 5, {NULL}, 0, 0, S_GCHAOSAD2},	//S_GCHAOSAD1
	{SPR_GCHA, 8, 3, {NULL}, 0, 0, S_GCHAOSAD3},	//S_GCHAOSAD2
	{SPR_GCHA, 9, 6, {NULL}, 0, 0, S_GCHAOSAD4},	//S_GCHAOSAD3
	{SPR_GCHA, 8, 3, {NULL}, 0, 0, S_GCHAOSAD1},	//S_GCHAOSAD4
	{SPR_GCHA, 4, 8, {NULL}, 0, 0, S_GCHAOHAPPY2},	//S_GCHAOHAPPY1
	{SPR_GCHA, 5, 4, {NULL}, 0, 0, S_GCHAOHAPPY3},	//S_GCHAOHAPPY2
	{SPR_GCHA, 6, 8, {NULL}, 0, 0, S_GCHAOHAPPY4},	//S_GCHAOHAPPY3
	{SPR_GCHA, 5, 4, {NULL}, 0, 0, S_GCHAOHAPPY1},	//S_GCHAOHAPPY4

	// cheese:
	{SPR_CHEZ, FF_ANIMATE, -1, {NULL}, 1, 4, S_CHEESEIDLE},		//S_CHEESEIDLE
	{SPR_CHEZ, 2|FF_ANIMATE, -1, {NULL}, 1, 2, S_CHEESEFLY},	//S_CHEESEFLY
	{SPR_CHEZ, 7, 5, {NULL}, 0, 0, S_CHEESESAD2},	//S_CHEESESAD1
	{SPR_CHEZ, 8, 3, {NULL}, 0, 0, S_CHEESESAD3},	//S_CHEESESAD2
	{SPR_CHEZ, 9, 6, {NULL}, 0, 0, S_CHEESESAD4},	//S_CHEESESAD3
	{SPR_CHEZ, 8, 3, {NULL}, 0, 0, S_CHEESESAD1},	//S_CHEESESAD4
	{SPR_CHEZ, 4, 8, {NULL}, 0, 0, S_CHEESEHAPPY2},	//S_CHEESEHAPPY1
	{SPR_CHEZ, 5, 4, {NULL}, 0, 0, S_CHEESEHAPPY3},	//S_CHEESEHAPPY2
	{SPR_CHEZ, 6, 8, {NULL}, 0, 0, S_CHEESEHAPPY4},	//S_CHEESEHAPPY3
	{SPR_CHEZ, 5, 4, {NULL}, 0, 0, S_CHEESEHAPPY1},	//S_CHEESEHAPPY4

	{SPR_MXCL, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_RINGDEBT

	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_RINGSPARKS2}, // S_RINGSPARKS1
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_RINGSPARKS3}, // S_RINGSPARKS2
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_RINGSPARKS4}, // S_RINGSPARKS3
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_RINGSPARKS5}, // S_RINGSPARKS4
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_RINGSPARKS6}, // S_RINGSPARKS5
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|5, 1, {NULL}, 0, 0, S_RINGSPARKS7}, // S_RINGSPARKS6
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|6, 1, {NULL}, 0, 0, S_RINGSPARKS8}, // S_RINGSPARKS7
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|7, 1, {NULL}, 0, 0, S_RINGSPARKS9}, // S_RINGSPARKS8
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|8, 1, {NULL}, 0, 0, S_RINGSPARKS10}, // S_RINGSPARKS9
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|9, 1, {NULL}, 0, 0, S_RINGSPARKS11}, // S_RINGSPARKS10
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|10, 1, {NULL}, 0, 0, S_RINGSPARKS12}, // S_RINGSPARKS11
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|11, 1, {NULL}, 0, 0, S_RINGSPARKS13}, // S_RINGSPARKS12
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|12, 1, {NULL}, 0, 0, S_RINGSPARKS14}, // S_RINGSPARKS13
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|13, 1, {NULL}, 0, 0, S_RINGSPARKS15}, // S_RINGSPARKS14
	{SPR_RGSP, FF_PAPERSPRITE|FF_FULLBRIGHT|14, 1, {NULL}, 0, 0, S_NULL}, // S_RINGSPARKS15

	{SPR_LENS, FF_FULLBRIGHT|FF_ADD|FF_TRANS10|FF_ANIMATE|11, -1, {NULL},  3, 1, S_NULL}, // S_GAINAX_TINY
	{SPR_LENS, FF_FULLBRIGHT|FF_ADD|FF_TRANS10|FF_ANIMATE,     5, {NULL},  5, 1, S_GAINAX_MID1}, // S_GAINAX_HUGE
	{SPR_LENS, FF_FULLBRIGHT|FF_ADD|FF_TRANS10|FF_ANIMATE|5,  14, {NULL}, 14, 1, S_GAINAX_MID2}, // S_GAINAX_MID1
	{SPR_LENS, FF_FULLBRIGHT|FF_ADD|FF_TRANS10|FF_ANIMATE|19, -1, {NULL},  1, 1, S_NULL}, // S_GAINAX_MID2

	{SPR_DRAF, 0, 2, {NULL}, 0, 0, S_DRAFTDUST2}, // S_DRAFTDUST1
	{SPR_DRAF, 1, 1, {NULL}, 0, 0, S_DRAFTDUST3}, // S_DRAFTDUST2
	{SPR_DRAF, 2, 1, {NULL}, 0, 0, S_DRAFTDUST4}, // S_DRAFTDUST3
	{SPR_DRAF, 3, 1, {NULL}, 0, 0, S_DRAFTDUST5}, // S_DRAFTDUST4
	{SPR_DRAF, 4, 1, {NULL}, 0, 0, S_NULL}, // S_DRAFTDUST5

	{SPR_GRES, FF_ANIMATE|FF_PAPERSPRITE, -1, {NULL}, 2, 4, S_NULL}, // S_TIREGREASE

	{SPR_OTBU, FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_NULL}, // S_OVERTIME_BULB1
	{SPR_OTBU, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_NULL}, // S_OVERTIME_BULB2
	{SPR_OTLS, FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_NULL}, // S_OVERTIME_LASER
	{SPR_OTCP, 0, -1, {NULL}, 0, 0, S_NULL}, // S_OVERTIME_CENTER

	{SPR_CAPS, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_BATTLECAPSULE_SIDE1
	{SPR_CAPS, FF_PAPERSPRITE|1, -1, {NULL}, 0, 0, S_NULL}, // S_BATTLECAPSULE_SIDE2
	{SPR_CAPS, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BATTLECAPSULE_TOP
	{SPR_CAPS, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BATTLECAPSULE_BUTTON
	{SPR_CAPS, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BATTLECAPSULE_SUPPORT
	{SPR_CAPS, FF_ANIMATE|5, -1, {NULL}, 3, 1, S_NULL}, // S_BATTLECAPSULE_SUPPORTFLY

	{SPR_WAYP, 0, 1, {NULL}, 0, 0, S_NULL}, // S_WAYPOINTORB
	{SPR_WAYP, 1|FF_FLOORSPRITE, 1, {NULL}, 0, 0, S_NULL}, // S_WAYPOINTSPLAT
	{SPR_EGOO, 0, 1, {NULL}, 0, 0, S_NULL}, // S_EGOORB

	{SPR_AMPA, FF_FULLBRIGHT|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 41, 1, S_NULL}, // S_AMPS
	{SPR_EXPC, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_EXP

	// Water Trail
	{SPR_WTRL, FF_PAPERSPRITE  , 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAIL1
	{SPR_WTRL, FF_PAPERSPRITE|1, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAIL2
	{SPR_WTRL, FF_PAPERSPRITE|2, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAIL3
	{SPR_WTRL, FF_PAPERSPRITE|3, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAIL4
	{SPR_WTRL, FF_PAPERSPRITE|4, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAIL5
	{SPR_WTRL, FF_PAPERSPRITE|5, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAIL6
	{SPR_WTRL, FF_PAPERSPRITE|6, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAIL7
	{SPR_WTRL, FF_PAPERSPRITE|7, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAIL8
	{SPR_WTRL, FF_TRANS50|FF_PAPERSPRITE|8,  2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAILUNDERLAY1
	{SPR_WTRL, FF_TRANS50|FF_PAPERSPRITE|9,  2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAILUNDERLAY2
	{SPR_WTRL, FF_TRANS50|FF_PAPERSPRITE|10, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAILUNDERLAY3
	{SPR_WTRL, FF_TRANS50|FF_PAPERSPRITE|11, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAILUNDERLAY4
	{SPR_WTRL, FF_TRANS50|FF_PAPERSPRITE|12, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAILUNDERLAY5
	{SPR_WTRL, FF_TRANS50|FF_PAPERSPRITE|13, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAILUNDERLAY6
	{SPR_WTRL, FF_TRANS50|FF_PAPERSPRITE|14, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAILUNDERLAY7
	{SPR_WTRL, FF_TRANS50|FF_PAPERSPRITE|15, 2, {NULL}, 0, 0, S_NULL}, // S_WATERTRAILUNDERLAY8

	{SPR_SDDS, FF_ANIMATE, 9, {NULL}, 9, 1, S_NULL}, // S_SPINDASHDUST
	{SPR_SDWN, FF_ANIMATE|FF_PAPERSPRITE, 18, {NULL}, 9, 2, S_NULL}, // S_SPINDASHWIND

	// Soft Landing
	{SPR_EBRK, 0|FF_ADD|FF_FLOORSPRITE|FF_FULLBRIGHT, 4, {NULL}, 0, 0, S_SOFTLANDING2}, // S_SOFTLANDING1
	{SPR_EBRK, 1|FF_ADD|FF_FLOORSPRITE|FF_FULLBRIGHT, 4, {NULL}, 0, 0, S_SOFTLANDING3}, // S_SOFTLANDING2
	{SPR_EBRK, 2|FF_ADD|FF_FLOORSPRITE|FF_FULLBRIGHT, 4, {NULL}, 0, 0, S_SOFTLANDING4}, // S_SOFTLANDING3
	{SPR_EBRK, 3|FF_ADD|FF_FLOORSPRITE|FF_FULLBRIGHT, 4, {NULL}, 0, 0, S_SOFTLANDING5}, // S_SOFTLANDING4
	{SPR_EBRK, 4|FF_ADD|FF_FLOORSPRITE|FF_FULLBRIGHT, 4, {NULL}, 0, 0, S_NULL}, 		// S_SOFTLANDING5

	// Downwards Lines
	{SPR_HMTR, 0|FF_ADD|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DOWNLINE2}, 	// S_DOWNLINE1
	{SPR_HMTR, 1|FF_ADD|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DOWNLINE3}, 	// S_DOWNLINE2
	{SPR_HMTR, 2|FF_ADD|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DOWNLINE4}, 	// S_DOWNLINE3
	{SPR_HMTR, 3|FF_ADD|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DOWNLINE5}, 	// S_DOWNLINE4
	{SPR_HMTR, 4|FF_ADD|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_NULL}, 		// S_DOWNLINE5

	// HOLD Bubble
	{SPR_HBUB, 0|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_HOLDBUBBLE}, 	// S_HOLDBUBBLE

	// Finish line beam
	{SPR_FLBM, FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_NULL}, // S_FINISHBEAM1
	{SPR_FLBM, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_NULL}, // S_FINISHBEAM2
	{SPR_FLBM, FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_NULL}, // S_FINISHBEAM3
	{SPR_FLBM, FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_NULL}, // S_FINISHBEAM4
	{SPR_FLBM, FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_NULL}, // S_FINISHBEAM5
	{SPR_FLBM, FF_PAPERSPRITE|5, 1, {NULL}, 0, 0, S_NULL}, // S_FINISHBEAMEND1
	{SPR_FLBM, FF_PAPERSPRITE|6, 1, {NULL}, 0, 0, S_NULL}, // S_FINISHBEAMEND2

	// Funny Spike
	{SPR_DEBT, 0|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKE2}, // S_DEBTSPIKE1
	{SPR_DEBT, 7|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKE3}, // S_DEBTSPIKE2
	{SPR_DEBT, 1|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKE4}, // S_DEBTSPIKE3
	{SPR_DEBT, 7|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKE5}, // S_DEBTSPIKE4
	{SPR_DEBT, 2|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKE6}, // S_DEBTSPIKE5
	{SPR_DEBT, 7|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKE7}, // S_DEBTSPIKE6
	{SPR_DEBT, 3|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKE8}, // S_DEBTSPIKE7
	{SPR_DEBT, 7|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKE9}, // S_DEBTSPIKE8
	{SPR_DEBT, 4|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKEA}, // S_DEBTSPIKE9
	{SPR_DEBT, 7|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKEB}, // S_DEBTSPIKEA
	{SPR_DEBT, 5|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKEC}, // S_DEBTSPIKEB
	{SPR_DEBT, 7|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKED}, // S_DEBTSPIKEC
	{SPR_DEBT, 6|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKEE}, // S_DEBTSPIKED
	{SPR_DEBT, 7|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_DEBTSPIKE1}, // S_DEBTSPIKEE

	// Sparks when driving on stairs
	{SPR_JANK, 0, 1, {NULL}, 0, 0, S_JANKSPARK2}, // S_JANKSPARK1
	{SPR_JANK, FF_PAPERSPRITE|FF_FULLBRIGHT|FF_ANIMATE, 4, {NULL}, 3, 1, S_JANKSPARK3}, // S_JANKSPARK2
	{SPR_JANK, 0, 0, {A_SetCustomValue}, -1, 5, S_JANKSPARK4}, // S_JANKSPARK3
	{SPR_JANK, 0, 0, {A_ChangeAngleRelative}, 180, 180, S_JANKSPARK2}, // S_JANKSPARK4

	{SPR_HFX1, FF_FULLBRIGHT|FF_PAPERSPRITE, 1, {NULL}, 0, 1, S_NULL}, // S_HITLAG_1
	{SPR_HFX2, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 2, {NULL}, 1, 1, S_NULL}, // S_HITLAG_2
	{SPR_HFX3, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 3, {NULL}, 2, 1, S_NULL}, // S_HITLAG_3
	{SPR_HFX4, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 4, {NULL}, 3, 1, S_NULL}, // S_HITLAG_4
	{SPR_HFX5, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 5, {NULL}, 4, 1, S_NULL}, // S_HITLAG_5
	{SPR_HFX6, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 6, {NULL}, 5, 1, S_NULL}, // S_HITLAG_6
	{SPR_HFX8, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 8, {NULL}, 7, 1, S_NULL}, // S_HITLAG_8
	{SPR_HFX9, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 9, {NULL}, 8, 1, S_NULL}, // S_HITLAG_9
	{SPR_HFXX, FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 10, {NULL}, 9, 1, S_NULL}, // S_HITLAG_10

	// Broly Ki Orb
	{SPR_LSSJ, FF_REVERSESUBTRACT|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_BROLY2}, // S_BROLY1
	{SPR_NULL, 0, 5*TICRATE, {A_SSMineFlash}, 0, 0, S_NULL}, // S_BROLY2

	{SPR_UFOB, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SPECIAL_UFO_POD
	{SPR_UFOB, 1|FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 1, 1, S_NULL}, // S_SPECIAL_UFO_OVERLAY
	{SPR_SSCA, 0,           -1, {NULL}, 0, 0, S_NULL}, // S_SPECIAL_UFO_GLASS
	{SPR_SSCB, FF_SUBTRACT, -1, {NULL}, 0, 0, S_NULL}, // S_SPECIAL_UFO_GLASS_UNDER
	{SPR_UFOA, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_SPECIAL_UFO_ARM
	{SPR_UFOS, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SPECIAL_UFO_STEM

	{SPR_GBOM, FF_ANIMATE, -1, {NULL}, 3, 1, S_NULL}, // S_GACHABOM
	{SPR_GBOM, FF_INVERT, 175, {NULL}, 0, 0, S_NULL}, // S_GACHABOM_DEAD

	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_GACHABOM_EXPLOSION_2},
	{SPR_GCHX, 0|FF_PAPERSPRITE|FF_ANIMATE, 14, {NULL}, 6, 2, S_GACHABOM_EXPLOSION_3A}, // S_GACHABOM_EXPLOSION_2
	{SPR_GCHX, 6|FF_PAPERSPRITE|FF_ANIMATE, 4, {NULL}, 1, 2, S_GACHABOM_EXPLOSION_3B}, // S_GACHABOM_EXPLOSION_3A
	{SPR_NULL, 0|FF_PAPERSPRITE, 0, {A_Repeat}, 8, S_GACHABOM_EXPLOSION_3A, S_GACHABOM_EXPLOSION_4}, // S_GACHABOM_EXPLOSION_3B
	{SPR_GCHX, 6|FF_PAPERSPRITE|FF_ANIMATE|FF_REVERSEANIM, 14, {NULL}, 6, 2, S_GACHABOM_WAITING}, // S_GACHABOM_EXPLOSION_4
	{SPR_GBOM, FF_INVERT, 8, {A_SetScale}, FRACUNIT, 0, S_GACHABOM_RETURNING}, // S_GACHABOM_WAITING
	{SPR_GBOM, FF_INVERT, -1, {A_SetScale}, FRACUNIT/2, 1, S_NULL}, // S_GACHABOM_RETURNING

	{SPR_3DFR, 1|FF_ANIMATE, -1, {NULL}, 2, 5, S_NULL}, // S_SUPER_FLICKY

	// Battle/Power-UP UFO
	{SPR_BUFO, 0, -1, {A_SetScale}, 3*FRACUNIT/2 , 0, S_NULL}, // S_BATTLEUFO
	{SPR_BUFO, 1, -1, {A_SetScale}, 2*FRACUNIT/2, 0, S_NULL}, // S_BATTLEUFO_LEG
	{SPR_BUFO, 0, 4, {A_BossScream}, 0, MT_EXPLODE, S_BATTLEUFO_DIE}, // S_BATTLEUFO_DIE
	{SPR_DEZL, 1|FF_ANIMATE|FF_FULLBRIGHT, 15, {NULL}, 2, 5, S_BATTLEUFO_BEAM2}, // S_BATTLEUFO_BEAM1
	{SPR_DEZL, 3|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_BATTLEUFO_BEAM2

	{SPR_RBOW, FF_PAPERSPRITE|FF_ADD|FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 14, 2, S_NULL}, // S_POWERUP_AURA

	{SPR_CPT3, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CHECKPOINT
	{SPR_CPT2, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_CHECKPOINT_ARM
	{SPR_CPT1, FF_ADD|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_CHECKPOINT_ORB_DEAD
	{SPR_CPT1, FF_ADD|FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 2, 1, S_NULL}, // S_CHECKPOINT_ORB_LIVE
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK2}, // S_CHECKPOINT_SPARK1
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK3}, // S_CHECKPOINT_SPARK2
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK4}, // S_CHECKPOINT_SPARK3
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK5}, // S_CHECKPOINT_SPARK4
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK6}, // S_CHECKPOINT_SPARK5
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|5, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK7}, // S_CHECKPOINT_SPARK6
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|6, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK8}, // S_CHECKPOINT_SPARK7
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|7, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK9}, // S_CHECKPOINT_SPARK8
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|8, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK10}, // S_CHECKPOINT_SPARK9
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|3, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK11}, // S_CHECKPOINT_SPARK10
	{SPR_SGNS, FF_ADD|FF_FULLBRIGHT|2, 1, {NULL}, 0, 0, S_CHECKPOINT_SPARK1}, // S_CHECKPOINT_SPARK11

	// Las Vegas
	{SPR_RDRD, 0, -1, {NULL}, 0, 0, S_RIDEROID},	// S_RIDEROID
	{SPR_RDRC, FF_ANIMATE|FF_FULLBRIGHT|FF_TRANS30, -1, {NULL}, 3, 2, S_RIDEROID_ICON},	// S_RIDEROID_ICON

	// Leaf Storm
	{SPR_LSZB, 0, -1, {NULL}, 0, 0, S_EGGBALL},	// S_EGGBALL

	// Dead Line
	{SPR_DLZH, 0, -1, {NULL}, 0, 0, S_DLZHOVER},	// S_DLZHOVER

	{SPR_DLZR, 0, -1, {NULL}, 0, 0, S_DLZROCKET_L},	// S_DLZROCKET_L
	{SPR_DLZR, 1, -1, {NULL}, 0, 0, S_DLZROCKET_R},	// S_DLZROCKET_R

	// Water Palace
	{SPR_WPZF, 0, -1, {NULL}, 0, 0, S_WPZFOUNTAIN},	// S_WPZFOUNTAIN
	{SPR_WPZF, 1|FF_ANIMATE, -1, {NULL}, 3, 2, S_WPZFOUNTAINANIM},	// S_WPZFOUNTAINANIM
	{SPR_WPZK, FF_ANIMATE, -1, {NULL}, 3, 12, S_KURAGEN},	// S_KURAGEN
	{SPR_WPZK, 4, -1, {NULL}, 0, 0, S_KURAGENBOMB},			// S_KURAGENBOMB

	{SPR_SA2S, FF_SEMIBRIGHT|3, -1, {NULL}, 0, 0, S_NULL}, // S_BALLSWITCH_BALL
	{SPR_SA2S, FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM|4, -1, {NULL}, 1, 1, S_NULL}, // S_BALLSWITCH_BALL_ACTIVE
	{SPR_SA2S, FF_FLOORSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_BALLSWITCH_PAD
	{SPR_SA2S, FF_FLOORSPRITE|FF_ANIMATE|FF_GLOBALANIM|1, -1, {NULL}, 1, 1, S_NULL}, // S_BALLSWITCH_PAD_ACTIVE

	{SPR_STRG, FF_ADD|FF_FLOORSPRITE, -1, {NULL}, 0, 0, S_NULL},	// S_SPIKEDTARGET,
	{SPR_STRG, FF_ADD,                -1, {NULL}, 0, 0, S_NULL},	// S_SPIKEDLENS,

	{SPR_BLEA, 0, -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_MAIN,
	{SPR_BLEA, 1, -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_MAIN_LAUNCHED,

	{SPR_BLEB, FF_PAPERSPRITE|FF_FULLBRIGHT,            -1, {NULL}, 0, 0, S_NULL},			// S_BLENDEYE_EYE,
	{SPR_BLEB, FF_PAPERSPRITE|FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 1, 1, S_BLENDEYE_EYE},	// S_BLENDEYE_EYE_FLASH,

	{SPR_BLEC,   FF_PAPERSPRITE|FF_ADD,           -1, {NULL}, 0, 0, S_NULL},			// S_BLENDEYE_GLASS,
	{SPR_BLEC, 1|FF_PAPERSPRITE|FF_ADD|FF_ANIMATE, 2, {NULL}, 1, 1, S_BLENDEYE_GLASS},	// S_BLENDEYE_GLASS_STRESS,

	{SPR_BLED,   FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_SHIELD,
	{SPR_BLED, 1|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_SHIELD_L,
	{SPR_BLED, 2|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_SHIELD_R,
	{SPR_BLED, 3|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_SHIELD_BUSTED,
	{SPR_BLED, 5|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_SHIELD_BUSTED_L,
	{SPR_BLED, 6|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_SHIELD_BUSTED_R,

	{SPR_BLEE, 0,             4, {NULL}, 0, 0, S_BLENDEYE_EGGBEATER_EXTEND_2},	// S_BLENDEYE_EGGBEATER_EXTEND_1,
	{SPR_BLEE, 1|FF_ANIMATE,  8, {NULL}, 1, 2, S_BLENDEYE_EGGBEATER},			// S_BLENDEYE_EGGBEATER_EXTEND_2,
	{SPR_BLEE, 2,            -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_EGGBEATER,
	{SPR_BLEE, 2|FF_ANIMATE, -1, {NULL}, 1, 2, S_NULL},	// S_BLENDEYE_EGGBEATER_SPIN,

	{SPR_BLEF, FF_ADD|FF_ANIMATE, -1, {NULL}, 1, 2, S_NULL},	// S_BLENDEYE_FLAME,

	{SPR_BLEG,   FF_PAPERSPRITE|FF_ANIMATE, -1, {NULL}, 1, 1, S_NULL},	// S_BLENDEYE_GENERATOR,
	{SPR_BLEG, 2|FF_PAPERSPRITE,            -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_GENERATOR_BUSTED_L,
	{SPR_BLEG, 3|FF_PAPERSPRITE,            -1, {NULL}, 0, 0, S_NULL},	// S_BLENDEYE_GENERATOR_BUSTED_R,

	{SPR_PUYA, 6,  2, {A_BlendEyePuyoHack}, 0, 0, S_BLENDEYE_PUYO_SPAWN_2},	// S_BLENDEYE_PUYO_SPAWN_1,
	{SPR_PUYA, 5,  2, {A_BlendEyePuyoHack}, 0, 0, S_BLENDEYE_PUYO_SPAWN_3},	// S_BLENDEYE_PUYO_SPAWN_2,
	{SPR_PUYA, 4,  2, {A_BlendEyePuyoHack}, 0, 0, S_BLENDEYE_PUYO},			// S_BLENDEYE_PUYO_SPAWN_3,
	{SPR_PUYA, 0, -1, {A_BlendEyePuyoHack}, 0, 0, S_NULL},					// S_BLENDEYE_PUYO,
	{SPR_PUYA, 2,  2, {A_BlendEyePuyoHack}, 0, 0, S_BLENDEYE_PUYO_LAND_2},	// S_BLENDEYE_PUYO_LAND_1,
	{SPR_PUYA, 0,  2, {A_BlendEyePuyoHack}, 0, 0, S_BLENDEYE_PUYO_LAND_3},	// S_BLENDEYE_PUYO_LAND_2,
	{SPR_PUYA, 1,  2, {A_BlendEyePuyoHack}, 0, 0, S_BLENDEYE_PUYO_LAND_4},	// S_BLENDEYE_PUYO_LAND_3,
	{SPR_PUYA, 0,  2, {A_BlendEyePuyoHack}, 1, S_BLENDEYE_PUYO_LAND_1, S_BLENDEYE_PUYO},	// S_BLENDEYE_PUYO_LAND_4,
	{SPR_PUYA, 3, -1, {A_BlendEyePuyoHack}, 0, 0, S_NULL},					// S_BLENDEYE_PUYO_SHOCK,
	{SPR_PUYA, 4|FF_ANIMATE,  5, {A_BlendEyePuyoHack}, 2, 2, S_NULL},				// S_BLENDEYE_PUYO_DIE,
	{SPR_PUYA, 5,             2, {A_BlendEyePuyoHack}, 0, 0, S_BLENDEYE_PUYO_DIE},	// S_BLENDEYE_PUYO_DUST,

	// Aerial Highlands
	{SPR_BCLD, FF_ANIMATE, -1, {NULL}, 3, 6, S_AHZCLOUD},  // S_AHZCLOUD

	// Avant Garden
	{SPR_AGTL, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_AGZBULB_BASE}, // S_AGZBULB_BASE
	{SPR_AGTU, 0, -1, {NULL}, 0, 0, S_AGZBULB_NEUTRAL}, // S_AGZBULB_NEUTRAL
	{SPR_AGTS, FF_ANIMATE, 8, {NULL}, 3, 2, S_AGZBULB_ANIM2}, // S_AGZBULB_ANIM1
	{SPR_AGTS, 4, 8, {NULL}, 0, 0, S_AGZBULB_ANIM3}, // S_AGZBULB_ANIM2
	{SPR_AGTS, FF_ANIMATE, 8, {NULL}, 3, 2, S_AGZBULB_NEUTRAL}, // S_AGZBULB_ANIM3
	{SPR_AGTR, 0, -1, {NULL}, 0, 0, S_AGTR}, // S_AGTR
	{SPR_AGFL, 0, -1, {NULL}, 0, 0, S_AGFL}, // S_AGFL
	{SPR_AGFF, 0, -1, {NULL}, 0, 0, S_AGFF}, // S_AGFF
	{SPR_AGCL, FF_ANIMATE, -1, {NULL}, 3, 6, S_AGZCLOUD}, // S_AGZCLOUD

	// Sky Sanctuary
	{SPR_SSCL, FF_ANIMATE, -1, {NULL}, 3, 6, S_SSZCLOUD}, // S_SSZCLOUD

	{SPR_MGSH, 2|FF_PAPERSPRITE|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_MEGABARRIER1,
	{SPR_MGSH, 1|FF_PAPERSPRITE|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_MEGABARRIER2,
	{SPR_MGSH, 0|FF_PAPERSPRITE|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_MEGABARRIER3,

	{SPR_GPTB, 0, -1, {A_SetScale}, 4*FRACUNIT, 0, S_NULL}, // S_GPZ_TREETHING_B,
	{SPR_GPTM, 0, -1, {A_SetScale}, 4*FRACUNIT, 0, S_NULL}, // S_GPZ_TREETHING_M,
	{SPR_GPTS, 0, -1, {A_SetScale}, 4*FRACUNIT, 0, S_NULL}, // S_GPZ_TREETHING_S,

	// MT_GGZFREEZETHRUSTER
	{SPR_GGZ6, 0, -1, {NULL}, 0, 0, S_GGZFREEZETHRUSTER},           // S_GGZFREEZETHRUSTER

	// MT_GGZICEDUST
	{SPR_GGZ7, 0|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZICEDUST2},    // S_GGZICEDUST1
	{SPR_GGZ7, 1|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZICEDUST3},    // S_GGZICEDUST2
	{SPR_GGZ7, 2|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZICEDUST4},    // S_GGZICEDUST3
	{SPR_GGZ7, 3|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZICEDUST5},    // S_GGZICEDUST4
	{SPR_GGZ7, 4|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZICEDUST6},    // S_GGZICEDUST5
	{SPR_GGZ7, 5|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZICEDUST7},    // S_GGZICEDUST6
	{SPR_GGZ7, 6|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZICEDUST8},    // S_GGZICEDUST7
	{SPR_GGZ7, 7|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZICEDUST9},    // S_GGZICEDUST8
	{SPR_GGZ7, 8|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZICEDUST10},   // S_GGZICEDUST9
	{SPR_GGZ7, 9|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZICEDUST11},   // S_GGZICEDUST10
	{SPR_GGZ7, 10|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_NULL},          // S_GGZICEDUST11
	{SPR_GGZ1, 0|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE12},  // S_GGZPARTICLE11
	{SPR_GGZ1, 1|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE13},  // S_GGZPARTICLE12
	{SPR_GGZ1, 2|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE14},  // S_GGZPARTICLE13
	{SPR_GGZ1, 3|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE15},  // S_GGZPARTICLE14
	{SPR_GGZ1, 4|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE16},  // S_GGZPARTICLE15
	{SPR_GGZ1, 5|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE17},  // S_GGZPARTICLE16
	{SPR_GGZ1, 6|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE18},  // S_GGZPARTICLE17
	{SPR_GGZ1, 7|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE11},  // S_GGZPARTICLE18
	{SPR_GGZ2, 0|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE22},  // S_GGZPARTICLE21
	{SPR_GGZ2, 1|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE23},  // S_GGZPARTICLE22
	{SPR_GGZ2, 2|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE24},  // S_GGZPARTICLE23
	{SPR_GGZ2, 3|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_GGZPARTICLE21},  // S_GGZPARTICLE24

	{SPR_GGZ8, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},  // S_GGZICECUBE

	// MT_THRUSTERPART
	{SPR_SFTR, 0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_THRUSTERPART},                 // S_THRUSTERPART

	// MT_IVOBALL
	{SPR_BSPH, 2|FF_SEMIBRIGHT, -1, {NULL}, 0, 0, S_NULL},        // S_IVOBALL

	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_RandomStateRange}, S_SA2_CRATE_DEBRIS_E, S_SA2_CRATE_DEBRIS_H, S_NULL}, // S_SA2_CRATE_DEBRIS
	{SPR_SABX, 4, 70, {NULL}, 0, 0, S_NULL},        // S_SA2_CRATE_DEBRIS_E
	{SPR_SABX, 5, 70, {NULL}, 0, 0, S_NULL},        // S_SA2_CRATE_DEBRIS_F
	{SPR_SABX, 6, 70, {NULL}, 0, 0, S_NULL},        // S_SA2_CRATE_DEBRIS_G
	{SPR_SABX, 7, 70, {NULL}, 0, 0, S_NULL},        // S_SA2_CRATE_DEBRIS_H
	{SPR_SABX, 12, 70, {NULL}, 0, 0, S_NULL},       // S_SA2_CRATE_DEBRIS_METAL

	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_RandomStateRange}, S_ICECAPBLOCK_DEBRIS_C, S_ICECAPBLOCK_DEBRIS_F, S_NULL}, // S_ICECAPBLOCK_DEBRIS
	{SPR_ICBL, 2, 70, {NULL}, 0, 0, S_NULL},        // S_ICECAPBLOCK_DEBRIS_C
	{SPR_ICBL, 3, 70, {NULL}, 0, 0, S_NULL},        // S_ICECAPBLOCK_DEBRIS_D
	{SPR_ICBL, 4, 70, {NULL}, 0, 0, S_NULL},        // S_ICECAPBLOCK_DEBRIS_E
	{SPR_ICBL, 5, 70, {NULL}, 0, 0, S_NULL},        // S_ICECAPBLOCK_DEBRIS_F

	// MT_SPEAR
	{SPR_BSSP, 1|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},        // S_SPEAR_ROD
	{SPR_BSSP, 2|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},        // S_SPEAR_TIP
	{SPR_BSPR, 0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},        // S_SPEAR_HILT_FRONT
	{SPR_BSPB, 0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},        // S_SPEAR_HILT_BACK
	{SPR_BSSR, 0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},        // S_SPEAR_WALL

	// MT_BSZLAMP_S
	{SPR_BLMS, 0|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL},          // S_BLMS
	{SPR_BLMM, 0|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL},          // S_BLMM
	{SPR_BLML, 0|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL},          // S_BLML

	// MT_BSZSLAMP
	{SPR_BSWL, 0|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL},          // S_BSWL
	{SPR_BSWC, 0|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL},          // S_BSWC

	{SPR_LCLA, 0|FF_FULLBRIGHT|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},           // S_BETA_PARTICLE_WHEEL
	{SPR_LCLA, 1|FF_FULLBRIGHT|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL},           // S_BETA_PARTICLE_ICON
	{SPR_LCLA, 2|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL},           // S_BETA_PARTICLE_EXPLOSION

	// MT_AIZ_REDFERN
	{SPR_AIZ1, 0, 1, {NULL}, 0, 0, S_AIZFL1},       // S_AIZFL1
	{SPR_AIZ2, 0, 1, {NULL}, 0, 0, S_AIZFR1},       // S_AIZFR1
	{SPR_AIZ3, 0, 1, {NULL}, 0, 0, S_AIZFR2},       // S_AIZFR2
	{SPR_AIZ4, 0, 1, {NULL}, 0, 0, S_AIZTRE},       // S_AIZTRE
	{SPR_AIZ5, 0, 1, {NULL}, 0, 0, S_AIZFR3},       // S_AIZFR3
	{SPR_AIZ6, 0, 10, {NULL}, 0, 0, S_AIZDB2},      // S_AIZDB1
	{SPR_AIZ6, 1, 8, {NULL}, 0, 0, S_AIZDB3},       // S_AIZDB2
	{SPR_AIZ6, 2, 6, {NULL}, 0, 0, S_AIZDB4},       // S_AIZDB3
	{SPR_AIZ6, 3, 6, {NULL}, 0, 0, S_AIZDB5},       // S_AIZDB4
	{SPR_AIZ6, 4, 6, {NULL}, 0, 0, S_AIZDB6},       // S_AIZDB5
	{SPR_AIZ6, 5, 8, {NULL}, 0, 0, S_AIZDB7},       // S_AIZDB6
	{SPR_AIZ6, 6, 8, {NULL}, 0, 0, S_AIZDB8},       // S_AIZDB7
	{SPR_AIZ6, 7, 10, {NULL}, 0, 0, S_AIZDB1},      // S_AIZDB8

	// MT_AZROCKS
	{SPR_AZR1, 0, -1, {NULL}, 0, 0, S_AZROCKS},     // S_AZROCKS
	{SPR_NULL, 0, -1, {NULL}, 0, 0, S_AZROCKS},     // S_AZROCKS_RESPAWN
	{SPR_AZR2, 0, 5*TICRATE, {NULL}, 0, 0, S_NULL},                 // S_AZROCKS_PARTICLE1

	// MT_EMROCKS
	{SPR_EMR1, 0, -1, {NULL}, 0, 0, S_EMROCKS},     // S_EMROCKS
	{SPR_NULL, 0, -1, {NULL}, 0, 0, S_EMROCKS},     // S_EMROCKS_RESPAWN
	{SPR_EMR2, 0, 5*TICRATE, {NULL}, 0, 0, S_NULL},                 // S_EMROCKS_PARTICLE1
	{SPR_EMR3, 0, 5*TICRATE, {NULL}, 0, 0, S_NULL},                 // S_EMROCKS_PARTICLE2

	// MT_EMFAUCET
	{SPR_EMFC, 0, -1, {NULL}, 0, 0, S_EMFAUCET},    // S_EMFAUCET

	// MT_EMFAUCET_DRIP
	{SPR_EMFC, 1, -1, {NULL}, 0, 0, S_EMROCKS_DRIP},                // S_EMROCKS_DRIP

	// MT_EMFAUCET_PARTICLE
	{SPR_EMFC, 2, -1, {NULL}, 0, 0, S_EMFAUCET_PARTICLE},           // S_EMFAUCET_PARTICLE

	// MT_TRICKBALLOON_RED
	{SPR_TKBR, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED2},			// S_TRICKBALLOON_RED1
	{SPR_TKBR, 1, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED1},
	{SPR_TKBR, 2, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED_POP2},		// S_TRICKBALLOON_RED_POP1
	{SPR_TKBR, 3, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED_POP3},
	{SPR_TKBR, 4, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED_GONE},
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED_INFLATE1},	// S_TRICKBALLOON_RED_GONE
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED_INFLATE2},	// S_TRICKBALLOON_RED_INFLATE1
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED_INFLATE3},
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED_INFLATE4},
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED_INFLATE5},
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_RED1},

	// MT_TRICKBALLOON_RED_POINT
	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_TRICKBALLOON_RED_POINT1},		// S_TRICKBALLOON_RED_POINT1

	// MT_TRICKBALLOON_YELLOW
	{SPR_TKBY, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW2},			// S_TRICKBALLOON_YELLOW1
	{SPR_TKBY, 1, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW1},
	{SPR_TKBY, 2, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW_POP2},		// S_TRICKBALLOON_YELLOW_POP1
	{SPR_TKBY, 3, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW_POP3},
	{SPR_TKBY, 4, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW_GONE},
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW_INFLATE1},	// S_TRICKBALLOON_YELLOW_GONE
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW_INFLATE2},	// S_TRICKBALLOON_YELLOW_INFLATE1
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW_INFLATE3},
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW_INFLATE4},
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW_INFLATE5},
	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW1},

	// MT_TRICKBALLOON_YELLOW_POINT
	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_TRICKBALLOON_YELLOW_POINT1},		// S_TRICKBALLOON_YELLOW_POINT1

	// MT_WATERFALLPARTICLESPAWNER
	{SPR_WTRP, FF_ANIMATE, 32, {NULL}, 31, 1, S_NULL},

	// MT_SSCANDLE
	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_SSCANDLE},						// S_SSCANDLE_INIT
	{SPR_NULL, 0, -1, {A_MakeSSCandle}, 0, 0, S_SSCANDLE},			// S_SSCANDLE

	// MT_SSCANDLE_SIDE
	{SPR_SCND, 0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_SSCANDLE_SIDE},	// S_SSCANDLE_SIDE

	// MT_SSCANDLE_FLAME
	{SPR_SCNF, FF_ANIMATE|FF_FULLBRIGHT|FF_ADD|FF_RANDOMANIM|0, -1, {NULL}, 15, 4, S_SSCANDLE_FLAME},	// S_SSCANDLE_FLAME

	// MT_SS_HOLOGRAM
	{SPR_SSBI, FF_PAPERSPRITE|FF_ANIMATE|FF_ADD|0, -1, {A_HologramRandomTranslucency}, 15, 1, S_NULL}, // S_HOLOGRAM_BIRD
	{SPR_SSCR, FF_PAPERSPRITE|FF_ANIMATE|FF_ADD|0, -1, {A_HologramRandomTranslucency}, 15, 1, S_NULL}, // S_HOLOGRAM_CRAB
	{SPR_SSFI, FF_PAPERSPRITE|FF_ANIMATE|FF_ADD|0, -1, {A_HologramRandomTranslucency}, 15, 1, S_NULL}, // S_HOLOGRAM_FISH
	{SPR_SSSQ, FF_PAPERSPRITE|FF_ANIMATE|FF_ADD|0, -1, {A_HologramRandomTranslucency}, 15, 1, S_NULL}, // S_HOLOGRAM_SQUID

	// MT_SS_COIN
	{SPR_SSCO, 0, 0, {NULL}, 0, 0, S_NULL}, // S_SS_COIN

	// MT_SS_GOBLET
	{SPR_SGOB, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SS_GOBLET

	// MT_SS_LAMP
	{SPR_SSLA, 0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_SS_LAMP

	// MT_SS_LAMP_BULB
	{SPR_SSLA, 1|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_SS_LAMP_BULB
	{SPR_SSLA, 3|FF_ANIMATE|FF_FULLBRIGHT|FF_ADD, -1, {NULL}, 1, 1, S_NULL}, // S_SS_LAMP_AURA

	// MT_SSWINDOW
	{SPR_SWIN, FF_PAPERSPRITE|FF_FULLBRIGHT|FF_TRANS30|0, 1, {A_ChangeAngleRelative}, -90, -90, S_SSWINDOW}, // S_SSWINDOW_INIT
	{SPR_SWIN, FF_PAPERSPRITE|FF_ANIMATE|FF_FULLBRIGHT|FF_TRANS30|0, -1, {NULL}, 31, 4, S_SSWINDOW}, // S_SSWINDOW

	// MT_SSWINDOW_SHINE
	{SPR_SWIS, FF_PAPERSPRITE|FF_ANIMATE|FF_FULLBRIGHT|FF_ADD|FF_RANDOMANIM|0, -1, {NULL}, 31, 3, S_SSWINDOW_SHINE}, // S_SSWINDOW_SHINE

	// MT_SSCHAINSOUND
	{SPR_NULL, 0, 16, {A_PlaySound}, sfx_ssthnk, 1, S_SSCHAINSOUND}, // S_SSCHAINSOUND

	// MT_SLSTMACE
	{SPR_S_SP, FF_ANIMATE|FF_SEMIBRIGHT, -1, {NULL}, 3, 2, S_NULL}, // S_SLSTMACE

	// MT_SEALEDSTAR_BUMPER
	{SPR_SBMP, 0|FF_FULLBRIGHT, -1, {A_GenericBumper}, 0, 56, S_SEALEDSTAR_BUMPER}, // S_SEALEDSTAR_BUMPER
	{SPR_SBMP, 1|FF_ANIMATE|FF_FULLBRIGHT, 8, {NULL}, 1, 2, S_SEALEDSTAR_BUMPER}, // S_SEALEDSTAR_BUMPERHIT

	// MT_SSCHAIN_SPAWNER
	{SPR_NULL, 0, 2, {A_SSChainShatter}, 0, 0, S_NULL}, // S_SSCHAIN_SPAWNER_SHATTER

	// MT_SSCHAIN
	{SPR_SSCH, 0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_SSCHAIN1

	// MT_GACHATARGET
	{SPR_GCTA, FF_PAPERSPRITE|FF_FULLBRIGHT|0, -1, {NULL}, 0, 0, S_NULL}, // S_GACHATARGET
	{SPR_GCTA, FF_PAPERSPRITE|FF_ANIMATE|FF_FULLBRIGHT|0, 12, {NULL}, 3, 4, S_GACHATARGETOK}, // S_GACHATARGETSPIN
	{SPR_GCTA, FF_PAPERSPRITE|FF_FULLBRIGHT|5, -1, {NULL}, 0, 0, S_NULL}, // S_GACHATARGETOK

	// MT_CABOTRON
	{SPR_SENB, 0|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_CABOTRON}, // S_CABOTRON

	// MT_CABOTRONSTAR
	{SPR_SENC, 0|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_CABOTRONSTAR}, // S_CABOTRONSTAR

	// MT_STARSTREAM
	{SPR_SEAS, FF_ANIMATE|0, 30, {NULL}, 29, 1, S_NULL}, // S_STARSTREAM

	// MT_SCRIPT_THING
	{SPR_TLKP, 0|FF_SEMIBRIGHT|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_TALKPOINT
	{SPR_TLKP, 1|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_TALKPOINT_ORB

	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_BADNIK_EXPLOSION_SHOCKWAVE2}, // S_BADNIK_EXPLOSION_SHOCKWAVE1
	{SPR_NULL, 0, 1, {A_PlaySound}, sfx_s3k3d, 1, S_BATTLEBUMPER_EXBLAST1}, // S_BADNIK_EXPLOSION_SHOCKWAVE2
	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_BADNIK_EXPLOSION2}, // S_BADNIK_EXPLOSION1
	{SPR_WIPD, FF_FULLBRIGHT|FF_RANDOMANIM|FF_ANIMATE, 30, {NULL}, 9, 3, S_NULL}, // S_BADNIK_EXPLOSION2

	// Flybot767 (stun)
	{SPR_STUN, FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 4, 4, S_NULL}, // S_FLYBOT767

	{SPR_STON, 0, -1, {NULL}, 0, 0, S_STON}, // S_STON
											 //
	{SPR_TOXA, 0, -1, {NULL}, 0, 0, S_TOXAA}, // S_TOXAA
	{SPR_TOXA, 0, 175, {NULL}, 0, 0, S_NULL}, // S_TOXAA_DEAD
	{SPR_TOXA, 1, -1, {NULL}, 0, 0, S_TOXAB}, // S_TOXAB
	{SPR_TOXB, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 6, 5, S_TOXBA}, // S_TOXBA

	{SPR_GEAR,                0, -1, {NULL}, 0, 0, S_NULL}, // S_ANCIENTGEAR
	{SPR_GEAR, FF_PAPERSPRITE|1, -1, {NULL}, 0, 0, S_NULL}, // S_ANCIENTGEAR_PART

	{SPR_MHPL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_MHPOLE
};

mobjinfo_t mobjinfo[NUMMOBJTYPES] =
{
	{           // MT_NULL
		-1,             // doomednum
		S_NULL,         // spawnstate
		0,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		0,              // radius
		0,              // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_RAY
		-1,             // doomednum
		S_NULL,         // spawnstate
		0,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		0,              // radius
		0,              // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_UNKNOWN
		-1,             // doomednum
		S_UNKNOWN,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_THOK
		-1,             // doomednum
		S_THOK,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SHADOW
		-1,             // doomednum
		S_SHADOW,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		-1,             // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_PLAYER
		-1,             // doomednum
		S_KART_STILL,   // spawnstate
		1,              // spawnhealth
		S_KART_FAST,    // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_thok,       // attacksound
		S_KART_SPINOUT, // painstate
		MT_THOK,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_KART_SPINOUT, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		MT_THOK,        // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		(statenum_t)MT_THOK // raisestate
	},

	{           // MT_KART_LEFTOVER
		4095,            // doomednum
		S_KART_LEFTOVER_NOTIRES, // spawnstate
		2,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_KART_LEFTOVER_NOTIRES, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		16*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		-1,             // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_KART_TIRE
		-1,             // doomednum
		S_KART_TIRE1,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		6*FRACUNIT,     // radius
		12*FRACUNIT,    // height
		-1,             // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_KART_PARTICLE
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		6*FRACUNIT,     // radius
		12*FRACUNIT,    // height
		-1,             // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_DONTENCOREMAP|MF_NOSQUISH, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSSEXPLODE
		-1,             // doomednum
		S_BOSSEXPLODE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SONIC3KBOSSEXPLODE
		-1,                      // doomednum
		S_SONIC3KBOSSEXPLOSION1, // spawnstate
		1000,                    // spawnhealth
		S_NULL,                  // seestate
		sfx_None,                // seesound
		8,                       // reactiontime
		sfx_None,                // attacksound
		S_NULL,                  // painstate
		0,                       // painchance
		sfx_None,                // painsound
		S_NULL,                  // meleestate
		S_NULL,                  // missilestate
		S_NULL,                  // deathstate
		S_NULL,                  // xdeathstate
		sfx_None,                // deathsound
		1,                       // speed
		8*FRACUNIT,              // radius
		16*FRACUNIT,             // height
		0,                       // display offset
		4,                       // mass
		0,                       // damage
		sfx_None,                // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL                   // raisestate
	},

	{           // MT_BOSSFLYPOINT
		290,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGTRAP
		291,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_pop,        // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_INVISIBLE,    // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSS3WAYPOINT
		292,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_RING
		300,            // doomednum
		S_RING,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGRING,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_RING,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		38*FRACUNIT,    // speed
		48*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_PICKUPFROMBELOW|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLINGRING
		-1,             // doomednum
		S_RING,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGRING,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_RING,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		38*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_PICKUPFROMBELOW|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_DEBTSPIKE
		-1,             // doomednum
		S_DEBTSPIKE1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		38*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUESPHERE
		-1,             // doomednum
		S_BLUESPHERE_SPAWN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGBLUESPHERE, // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k65,      // deathsound
		38*FRACUNIT,    // speed
		48*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_RUNSPAWNFUNC|MF_SPECIAL|MF_PICKUPFROMBELOW|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL // raisestate
	},

	{           // MT_FLINGBLUESPHERE
		-1,             // doomednum
		S_BLUESPHERE_SPAWN,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGBLUESPHERE,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_BLUESPHERE,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k65,     // deathsound
		38*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_PICKUPFROMBELOW, // flags
		S_NULL // raisestate
	},

	{           // MT_EMBLEM
		322,            // doomednum
		S_EMBLEM1,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		16*FRACUNIT,     // radius
		30*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPRAYCAN
		2807,           // doomednum
		S_SPRAYCAN,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		30*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ANCIENTSHRINE
		2256,           // doomednum
		S_ANCIENTSHRINE,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SOLID|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_EMERALD
		-1,             // doomednum
		S_CHAOSEMERALD1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_EMERALD,     // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k9c,      // deathsound
		0,              // speed
		80*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_PICKUPFROMBELOW|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_EMERALDSPARK
		-1,             // doomednum
		S_EMERALDSPARK1,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_EMERALDFLARE
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_PRISONEGGDROP
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k9c,      // deathsound
		0,              // speed
		65*FRACUNIT,    // radius
		130*FRACUNIT,   // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_PICKUPFROMBELOW|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_STEAM
		541,            // doomednum
		S_STEAM1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_steam2,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_steam1,     // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		1*FRACUNIT,     // height
		0,              // display offset
		20*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_BALLOON
		543,            // doomednum
		S_BALLOON,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		2,              // painchance
		sfx_s3k77,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BALLOONPOP2,  // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		20*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_BALLOONPOP1   // raisestate
	},

	{           // MT_YELLOWSPRING
		550,            // doomednum
		S_YELLOWSPRING1,// spawnstate
		1000,           // spawnhealth
		S_YELLOWSPRING2,// seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_YELLOW, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		40*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_YELLOWSPRING2 // raisestate
	},

	{           // MT_REDSPRING
		551,            // doomednum
		S_REDSPRING1,   // spawnstate
		1000,           // spawnhealth
		S_REDSPRING2,   // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_RASPBERRY, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		64*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_REDSPRING2    // raisestate
	},

	{           // MT_BLUESPRING
		552,            // doomednum
		S_BLUESPRING1,  // spawnstate
		1000,           // spawnhealth
		S_BLUESPRING2,  // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_PASTEL, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		102*FRACUNIT,   // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_BLUESPRING2   // raisestate
	},

	{           // MT_GREYSPRING
		553,            // doomednum
		S_GREYSPRING1,  // spawnstate
		1000,           // spawnhealth
		S_GREYSPRING2,  // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_POPCORN, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		25*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_GREYSPRING2   // raisestate
	},

	{           // MT_POGOSPRING
		2057,           // doomednum
		S_POGOSPRING1,  // spawnstate
		1000,           // spawnhealth
		S_POGOSPRING2B, // seestate
		sfx_eggspr,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_SUNSLAM, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		32*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_POGOSPRING2   // raisestate
	},

	{           // MT_YELLOWDIAG
		554,            // doomednum
		S_YDIAG1,       // spawnstate
		1,              // spawnhealth
		S_YDIAG2,       // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_YELLOW, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		40*FRACUNIT,    // mass
		40*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_YDIAG2        // raisestate
	},

	{           // MT_REDDIAG
		555,            // doomednum
		S_RDIAG1,       // spawnstate
		1,              // spawnhealth
		S_RDIAG2,       // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_RASPBERRY, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		64*FRACUNIT,    // mass
		64*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_RDIAG2        // raisestate
	},

	{           // MT_BLUEDIAG
		556,            // doomednum
		S_BDIAG1,       // spawnstate
		1,              // spawnhealth
		S_BDIAG2,       // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_PASTEL, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		102*FRACUNIT,   // mass
		102*FRACUNIT,   // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_BDIAG2        // raisestate
	},

	{           // MT_GREYDIAG
		557,            // doomednum
		S_GDIAG1,       // spawnstate
		1,              // spawnhealth
		S_GDIAG2,       // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_POPCORN, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		25*FRACUNIT,    // mass
		25*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_GDIAG2        // raisestate
	},

	{           // MT_YELLOWHORIZ
		558,            // doomednum
		S_YHORIZ1,      // spawnstate
		1,              // spawnhealth
		S_YHORIZ2,      // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_YELLOW, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		72*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_YHORIZ2       // raisestate
	},

	{           // MT_REDHORIZ
		559,            // doomednum
		S_RHORIZ1,      // spawnstate
		1,              // spawnhealth
		S_RHORIZ2,      // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_RASPBERRY, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		115*FRACUNIT,   // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_RHORIZ2       // raisestate
	},

	{           // MT_BLUEHORIZ
		560,            // doomednum
		S_BHORIZ1,      // spawnstate
		1,              // spawnhealth
		S_BHORIZ2,      // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_PASTEL, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		184*FRACUNIT,   // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_BHORIZ2       // raisestate
	},

	{           // MT_GREYHORIZ
		561,            // doomednum
		S_GHORIZ1,      // spawnstate
		1,              // spawnhealth
		S_GHORIZ2,      // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_POPCORN, // painchance
		sfx_s3kb1,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		45*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_GHORIZ2       // raisestate
	},

	{           // MT_BUBBLES
		500,            // doomednum
		S_BUBBLES1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_SIGN
		501,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3kb8,      // seesound
		8,              // reactiontime
		sfx_s3k7e,      // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		48*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SIGN_PIECE
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		48*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPIKEBALL
		521,            // doomednum
		S_SPIKEBALL1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		12*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		0,              // display offset
		DMG_NORMAL,      // mass
		1,              // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPINFIRE
		-1,             // doomednum
		S_SPINFIRE1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPIKE
		523,            // doomednum
		S_SPIKE1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_s3k64,      // painsound
		S_SPIKE4,       // meleestate
		S_NULL,         // missilestate
		S_SPIKED1,      // deathstate
		S_SPIKED2,      // xdeathstate
		sfx_mspogo,     // deathsound
		2*TICRATE,      // speed
		14*FRACUNIT,     // radius
		90*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY|MF_NOCLIPHEIGHT|MF_NOHITLAGFORME|MF_DONTENCOREMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_WALLSPIKE
		522,            // doomednum
		S_WALLSPIKE1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_s3k64,      // painsound
		S_WALLSPIKE4,   // meleestate
		S_NULL,         // missilestate
		S_WALLSPIKED1,  // deathstate
		S_WALLSPIKED2,  // xdeathstate
		sfx_mspogo,     // deathsound
		2*TICRATE,      // speed
		48*FRACUNIT,    // radius
		14*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		45*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SCENERY|MF_NOCLIPHEIGHT|MF_PAPERCOLLISION|MF_NOHITLAGFORME|MF_DONTENCOREMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_WALLSPIKEBASE
		-1,            // doomednum
		S_WALLSPIKEBASE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		7*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOHITLAGFORME|MF_DONTENCOREMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_CHEATCHECK
		502,            // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_INVISIBLE,    // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_INVISIBLE,    // painstate
		0,              // painchance
		sfx_strpst,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		64*FRACUNIT,    // radius
		128*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_BLASTEXECUTOR
		756,            // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SHOOTABLE|MF_NOGRAVITY|MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANNONLAUNCHER
		1123,           // doomednum
		S_CANNONLAUNCHER1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		2*TICRATE,      // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		TICRATE,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_pop,        // activesound
		MF_NOGRAVITY|MF_NOSECTOR|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANNONBALL
		-1,             // doomednum
		S_CANNONBALL1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_cannon,     // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cybdth,     // deathsound
		16*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANNONBALLDECOR
		1124,           // doomednum
		S_CANNONBALL1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		16*FRACUNIT,    // speed
		20*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_PUSHABLE|MF_SLIDEME, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFLOWER1
		800,            // doomednum
		S_GFZFLOWERA,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFLOWER2
		801,            // doomednum
		S_GFZFLOWERB,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFLOWER3
		802,            // doomednum
		S_GFZFLOWERC,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEBERRYBUSH
		803,            // doomednum
		S_BLUEBERRYBUSH, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BERRYBUSH
		804,            // doomednum
		S_BERRYBUSH,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUSH
		805,            // doomednum
		S_BUSH,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZTREE
		806,            // doomednum
		S_GFZTREE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		128*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZBERRYTREE
		807,            // doomednum
		S_GFZBERRYTREE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		128*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZCHERRYTREE
		808,            // doomednum
		S_GFZCHERRYTREE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		128*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHECKERTREE
		809,            // doomednum
		S_CHECKERTREE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHECKERSUNSETTREE
		810,            // doomednum
		S_CHECKERSUNSETTREE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FHZTREE
		2102,           // doomednum
		S_FHZTREE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FHZPINKTREE
		2103,           // doomednum
		S_FHZPINKTREE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_POLYGONTREE
		811,            // doomednum
		S_POLYGONTREE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUSHTREE
		812,            // doomednum
		S_BUSHTREE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUSHREDTREE
		813,            // doomednum
		S_BUSHREDTREE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPRINGTREE
		1600,           // doomednum
		S_SPRINGTREE,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZFLOWER1
		900,            // doomednum
		S_THZFLOWERA,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZFLOWER2
		902,            // doomednum
		S_THZFLOWERB,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZFLOWER3
		903,            // doomednum
		S_THZFLOWERC,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZTREE
		904,            // doomednum
		S_THZTREE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZTREEBRANCH
		-1,             // doomednum
		S_THZTREEBRANCH1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ALARM
		901,            // doomednum
		S_ALARM1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_alarm,      // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GARGOYLE
		1000,           // doomednum
		S_GARGOYLE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		21*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_statu2,     // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIGGARGOYLE
		1009,           // doomednum
		S_BIGGARGOYLE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		12*FRACUNIT,    // speed
		32*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_statu2,     // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_SEAWEED
		1001,           // doomednum
		S_SEAWEED1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WATERDRIP
		1002,           // doomednum
		S_DRIPA1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		15*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WATERDROP
		-1,             // doomednum
		S_DRIPB1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DRIPC1,       // deathstate
		S_NULL,         // xdeathstate
		sfx_wdrip1,     // deathsound
		0,              // speed
		2*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		8,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORAL1
		1003,           // doomednum
		S_CORAL1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		29*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORAL2
		1004,           // doomednum
		S_CORAL2,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		30*FRACUNIT,    // radius
		53*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORAL3
		1005,           // doomednum
		S_CORAL3,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		28*FRACUNIT,    // radius
		41*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORAL4
		1014,           // doomednum
		S_CORAL4,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		56*FRACUNIT,    // radius
		112*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORAL5
		1015,           // doomednum
		S_CORAL5,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		56*FRACUNIT,    // radius
		112*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUECRYSTAL
		1006,           // doomednum
		S_BLUECRYSTAL1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_KELP
		1007,           // doomednum
		S_KELP,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		292*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ANIMALGAETOP
		1013,            // doomednum
		S_ANIMALGAETOP1, // spawnstate
		1000,            // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		48*FRACUNIT,     // radius
		120*FRACUNIT,    // height
		0,               // display offset
		4,               // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOCLIP|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_ANIMALGAESEG
		-1,             // doomednum
		S_ANIMALGAESEG, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		120*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DSZSTALAGMITE
		1008,           // doomednum
		S_DSZSTALAGMITE,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		116*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SOLID, // flags
		S_NULL          // raisestate
	},

	{           // MT_DSZ2STALAGMITE
		1011,           // doomednum
		S_DSZ2STALAGMITE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		116*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SOLID, // flags
		S_NULL          // raisestate
	},

	{           // MT_LIGHTBEAM
		1010,           // doomednum
		S_LIGHTBEAM1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHAIN
		1100,           // doomednum
		S_CEZCHAIN,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		128*FRACUNIT,   // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAME
		1101,           // doomednum
		S_FLAME,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_FLAMEPARTICLE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,       // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEPARTICLE
		-1,             // doomednum
		S_FLAMEPARTICLE,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGSTATUE
		1102,           // doomednum
		S_EGGSTATUE1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		240*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_PUSHABLE|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MACEPOINT
		1104,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		10000,          // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHAINMACEPOINT
		1105,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		10000,          // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHAINPOINT
		1107,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		10000,          // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_HIDDEN_SLING
		1108,           // doomednum
		S_SLING1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FIREBARPOINT
		1109,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		200,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CUSTOMMACEPOINT
		1110,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		200,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{            // MT_SMALLMACECHAIN
		-1,               // doomednum
		S_SMALLMACECHAIN, // spawnstate
		1000,             // spawnhealth
		S_NULL,           // seestate
		sfx_None,         // seesound
		8,                // reactiontime
		sfx_None,         // attacksound
		S_NULL,           // painstate
		0,                // painchance
		sfx_None,         // painsound
		S_NULL,           // meleestate
		S_NULL,           // missilestate
		S_NULL,           // deathstate
		S_NULL,           // xdeathstate
		sfx_None,         // deathsound
		24*FRACUNIT,      // speed
		17*FRACUNIT,      // radius
		34*FRACUNIT,      // height
		0,                // display offset
		100,              // mass
		1,                // damage
		sfx_None,         // activesound
		MF_SCENERY|MF_NOGRAVITY, // flags
		S_NULL            // raisestate
	},

	{            // MT_BIGMACECHAIN
		-1,             // doomednum
		S_BIGMACECHAIN,	// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		48*FRACUNIT,    // speed
		34*FRACUNIT,    // radius
		68*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{            // MT_SMALLMACE
		1130,           // doomednum
		S_SMALLMACE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		24*FRACUNIT,    // speed
		17*FRACUNIT,    // radius
		34*FRACUNIT,    // height
		1,              // display offset
		0,              // mass
		1,              // damage
		sfx_s3kc9s, //sfx_mswing, -- activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{            // MT_BIGMACE
		1131,           // doomednum
		S_BIGMACE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		48*FRACUNIT,    // speed
		34*FRACUNIT,    // radius
		68*FRACUNIT,    // height
		1,              // display offset
		0,              // mass
		1,              // damage
		sfx_s3kc9s, //sfx_mswing, -- activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{            // MT_SMALLGRABCHAIN
		-1,               // doomednum
		S_SMALLGRABCHAIN, // spawnstate
		1000,             // spawnhealth
		S_NULL,           // seestate
		sfx_None,         // seesound
		8,                // reactiontime
		sfx_None,         // attacksound
		S_NULL,           // painstate
		0,                // painchance
		sfx_None,         // painsound
		S_NULL,           // meleestate
		S_NULL,           // missilestate
		S_NULL,           // deathstate
		S_NULL,           // xdeathstate
		sfx_None,         // deathsound
		24*FRACUNIT,      // speed
		17*FRACUNIT,      // radius
		34*FRACUNIT,      // height
		0,                // display offset
		100,              // mass
		1,                // damage
		sfx_None,         // activesound
		MF_SCENERY|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL            // raisestate
	},

	{            // MT_BIGGRABCHAIN
		-1,             // doomednum
		S_BIGGRABCHAIN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		48*FRACUNIT,    // speed
		34*FRACUNIT,    // radius
		68*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{            // MT_SMALLFIREBAR
		1136,           // doomednum
		S_SMALLFIREBAR1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_FLAMEPARTICLE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		24*FRACUNIT,    // speed
		17*FRACUNIT,    // radius
		34*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{            // MT_BIGFIREBAR
		1137,           // doomednum
		S_BIGFIREBAR1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_FLAMEPARTICLE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		48*FRACUNIT,    // speed
		34*FRACUNIT,    // radius
		68*FRACUNIT,    // height
		1,              // display offset
		DMG_NORMAL,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZFLOWER
		1103,           // doomednum
		S_CEZFLOWER,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZPOLE1
		1117,           // doomednum
		S_CEZPOLE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		224*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZPOLE2
		1118,           // doomednum
		S_CEZPOLE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		224*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZBANNER1
		-1,             // doomednum
		S_CEZBANNER1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		224*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZBANNER2
		-1,             // doomednum
		S_CEZBANNER2,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		224*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PINETREE
		1114,           // doomednum
		S_PINETREE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		628*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SOLID|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZBUSH1
		1115,           // doomednum
		S_CEZBUSH1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZBUSH2
		1116,           // doomednum
		S_CEZBUSH2,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		3*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANDLE
		1119,           // doomednum
		S_CANDLE,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANDLEPRICKET
		1120,           // doomednum
		S_CANDLEPRICKET, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		176*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEHOLDER
		1121,           // doomednum
		S_FLAMEHOLDER,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_FIRETORCH
		1122,           // doomednum
		S_FIRETORCH,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_FLAMEPARTICLE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WAVINGFLAG1
		1128,           // doomednum
		S_WAVINGFLAG,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		208*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_WAVINGFLAG2
		1129,           // doomednum
		S_WAVINGFLAG,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		208*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_WAVINGFLAGSEG1
		-1,             // doomednum
		S_WAVINGFLAGSEG1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		1,              // height -- this is not a typo
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WAVINGFLAGSEG2
		-1,             // doomednum
		S_WAVINGFLAGSEG2, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		1,              // height -- this is not a typo
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CRAWLASTATUE
		1111,           // doomednum
		S_CRAWLASTATUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_BRAMBLES
		1125,           // doomednum
		S_BRAMBLES,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIGTUMBLEWEED
		1200,           // doomednum
		S_BIGTUMBLEWEED,// spawnstate
		1000,           // spawnhealth
		S_BIGTUMBLEWEED_ROLL1, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_s3k64,      // activesound
		MF_SPECIAL,      // flags
		S_NULL          // raisestate
	},

	{           // MT_LITTLETUMBLEWEED
		1201,           // doomednum
		S_LITTLETUMBLEWEED,// spawnstate
		1000,           // spawnhealth
		S_LITTLETUMBLEWEED_ROLL1, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_s3k64,      // activesound
		MF_SPECIAL,      // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI1
		1203,           // doomednum
		S_CACTI1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		13*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI2
		1204,           // doomednum
		S_CACTI2,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		15*FRACUNIT,    // radius
		52*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI3
		1205,           // doomednum
		S_CACTI3,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		13*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI4
		1206,           // doomednum
		S_CACTI4,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		15*FRACUNIT,    // radius
		52*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI5
		1207,           // doomednum
		S_CACTI5,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI6
		1208,           // doomednum
		S_CACTI6,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		128*FRACUNIT,   // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI7
		1209,           // doomednum
		S_CACTI7,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		224*FRACUNIT,   // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI8
		1210,           // doomednum
		S_CACTI8,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		256*FRACUNIT,   // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI9
		1211,           // doomednum
		S_CACTI9,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI10
		1230,           // doomednum
		S_CACTI10,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		13*FRACUNIT,    // radius
		28*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI11
		1231,           // doomednum
		S_CACTI11,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		15*FRACUNIT,    // radius
		60*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTITINYSEG
		-1,             // doomednum
		S_CACTITINYSEG, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		13*FRACUNIT,    // radius
		28*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTISMALLSEG
		-1,              // doomednum
		S_CACTISMALLSEG, // spawnstate
		1000,            // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		15*FRACUNIT,     // radius
		60*FRACUNIT,     // height
		0,               // display offset
		DMG_NORMAL,       // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOTHINK|MF_SCENERY|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARIDSIGN_CAUTION
		1212,           // doomednum
		S_ARIDSIGN_CAUTION,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		22*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SOLID|MF_PAPERCOLLISION, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARIDSIGN_CACTI
		1213,           // doomednum
		S_ARIDSIGN_CACTI,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		22*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SOLID|MF_PAPERCOLLISION, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARIDSIGN_SHARPTURN
		1214,           // doomednum
		S_ARIDSIGN_SHARPTURN,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		192*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SOLID|MF_PAPERCOLLISION, // flags
		S_NULL          // raisestate
	},

	{           // MT_OILLAMP
		1215,           // doomednum
		S_OILLAMP,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		22*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k4b,      // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SPAWNCEILING, // flags
		S_NULL          // raisestate
	},

	{           // MT_TNTBARREL
		1216,           // doomednum
		S_TNTBARREL_STND1,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_bowl,       // attacksound
		S_TNTBARREL_EXPL1,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_TNTBARREL_FLYING,         // missilestate
		S_TNTBARREL_EXPL1,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k4e,      // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		63*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_TNTDUST
		-1,                // doomednum
		S_TNTDUST_1,       // spawnstate
		1,                 // spawnhealth
		S_NULL,            // seestate
		sfx_None,          // seesound
		0,                 // reactiontime
		sfx_None,          // attacksound
		S_NULL,            // painstate
		0,                 // painchance
		sfx_None,          // painsound
		S_NULL,            // meleestate
		S_NULL,            // missilestate
		S_NULL,            // deathstate
		S_NULL,            // xdeathstate
		sfx_None,          // deathsound
		20*FRACUNIT,       // speed
		16*FRACUNIT,       // radius
		32*FRACUNIT,       // height
		0,                 // display offset
		100,               // mass
		0,                 // damage
		sfx_None,          // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL             // raisestate
	},

	{           // MT_PROXIMITYTNT
		1217,           // doomednum
		S_PROXIMITY_TNT,    // spawnstate
		1,              // spawnhealth
		S_PROXIMITY_TNT_TRIGGER1,         // seestate
		sfx_s3k5c,      // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k4e,      // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k89,      // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_DUSTDEVIL
		1218,           // doomednum
		S_DUSTDEVIL,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2,              // speed
		80*FRACUNIT,    // radius
		416*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k4b,      // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DUSTLAYER
		-1,             // doomednum
		S_DUSTLAYER1,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		256*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{          // MT_ARIDDUST
		-1,             // doomednum
		S_ARIDDUST1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEJET
		1300,           // doomednum
		S_FLAMEJETSTND, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOSECTOR|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_VERTICALFLAMEJET
		1301,           // doomednum
		S_FLAMEJETSTND, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEJETFLAME
		-1,             // doomednum
		S_FLAMEJETFLAME1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		5*FRACUNIT,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,       // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_MISSILE|MF_PAIN|MF_NOSQUISH|MF_NOHITLAGFORME|MF_ELEMENTAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_FJSPINAXISA
		1302,           // doomednum
		S_FJSPINAXISA1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOCLIPTHING|MF_NOGRAVITY|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_FJSPINAXISB
		1303,           // doomednum
		S_FJSPINAXISB1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOCLIPTHING|MF_NOGRAVITY|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEJETFLAMEB
		-1,             // doomednum
		S_FLAMEJETFLAMEB1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_fire,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		18,             // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_MISSILE|MF_PAIN|MF_NOBLOCKMAP|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_LAVAFALL
		1304,           // doomednum
		S_LAVAFALL_DORMANT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_lvfal1,     // seesound
		8,              // reactiontime
		sfx_s3kd5l,     // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		3200*FRACUNIT,  // speed
		30*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPAWNCEILING, // flags
		S_NULL          // raisestate
	},

	{           // MT_LAVAFALL_LAVA
		-1,             // doomednum
		S_LAVAFALL_LAVA1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_s3k7e,      // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_LAVAFALL_LAVA3, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		30*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_PAIN|MF_NOGRAVITY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_LAVAFALLROCK
		-1,             // doomednum
		S_LAVAFALLROCK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_BIGFERNLEAF
		-1,             // doomednum
		S_BIGFERNLEAF,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIGFERN
		1306,           // doomednum
		S_BIGFERN1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_JUNGLEPALM
		1307,           // doomednum
		S_JUNGLEPALM,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_TORCHFLOWER
		1308,           // doomednum
		S_TORCHFLOWER,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		14*FRACUNIT,    // radius
		110*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_WALLVINE_LONG
		1309,           // doomednum
		S_WALLVINE_LONG, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,    // radius
		288*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_WALLVINE_SHORT
		1310,           // doomednum
		S_WALLVINE_SHORT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,    // radius
		288*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE0
		1900,           // doomednum
		S_STG0,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE1
		1901,           // doomednum
		S_STG1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE2
		1902,           // doomednum
		S_STG2,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE3
		1903,           // doomednum
		S_STG3,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE4
		1904,           // doomednum
		S_STG4,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE5
		1905,           // doomednum
		S_STG5,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE6
		1906,           // doomednum
		S_STG6,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE7
		1907,           // doomednum
		S_STG7,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE8
		1908,           // doomednum
		S_STG8,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE9
		1909,           // doomednum
		S_STG9,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_XMASPOLE
		1850,           // doomednum
		S_XMASPOLE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANDYCANE
		1851,           // doomednum
		S_CANDYCANE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNOWMAN
		1852,           // doomednum
		S_SNOWMAN,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		25*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNOWMANHAT
		1853,           // doomednum
		S_SNOWMANHAT,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		25*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_LAMPPOST1
		1854,           // doomednum
		S_LAMPPOST1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		120*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_LAMPPOST2
		1855,           // doomednum
		S_LAMPPOST2,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		120*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_HANGSTAR
		1856,           // doomednum
		S_HANGSTAR,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4*FRACUNIT,     // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MISTLETOE
		2105,           // doomednum
		S_MISTLETOE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		52*FRACUNIT,    // radius
		106*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_XMASBLUEBERRYBUSH
		1859,           // doomednum
		S_XMASBLUEBERRYBUSH, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_XMASBERRYBUSH
		1857,           // doomednum
		S_XMASBERRYBUSH, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_XMASBUSH
		1858,           // doomednum
		S_XMASBUSH,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FHZICE1
		2100,           // doomednum
		S_FHZICE1,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FHZICE2
		2101,           // doomednum
		S_FHZICE2,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_JACKO1
		3006,           // doomednum
		S_JACKO1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_JACKO1OVERLAY_1 // raisestate
	},

	{           // MT_JACKO2
		3007,           // doomednum
		S_JACKO2,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_JACKO2OVERLAY_1 // raisestate
	},

	{           // MT_JACKO3
		3008,           // doomednum
		S_JACKO3,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_JACKO3OVERLAY_1 // raisestate
	},

	{           // MT_HHZTREE_TOP
		3010,           // doomednum
		S_HHZTREE_TOP,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZTREE_PART
		-1,             // doomednum
		S_HHZTREE_TRUNK,// spawnstate
		1000,           // spawnhealth
		S_HHZTREE_LEAF, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZSHROOM
		3009,           // doomednum
		S_HHZSHROOM_1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZGRASS
		3001,           // doomednum
		S_HHZGRASS,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZTENTACLE1
		3002,           // doomednum
		S_HHZTENT1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZTENTACLE2
		3003,           // doomednum
		S_HHZTENT2,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZSTALAGMITE_TALL
		3004,           // doomednum
		S_HHZSTALAGMITE_TALL, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZSTALAGMITE_SHORT
		3005,           // doomednum
		S_HHZSTALAGMITE_SHORT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	// No, I did not do all of this by hand.
	// I made a script to make all of these for me.
	// Ha HA. ~Inuyasha
	{           // MT_BSZTALLFLOWER_RED
		1400,           // doomednum
		S_BSZTALLFLOWER_RED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTALLFLOWER_PURPLE
		1401,           // doomednum
		S_BSZTALLFLOWER_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTALLFLOWER_BLUE
		1402,           // doomednum
		S_BSZTALLFLOWER_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTALLFLOWER_CYAN
		1403,           // doomednum
		S_BSZTALLFLOWER_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTALLFLOWER_YELLOW
		1404,           // doomednum
		S_BSZTALLFLOWER_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTALLFLOWER_ORANGE
		1405,           // doomednum
		S_BSZTALLFLOWER_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_RED
		1410,           // doomednum
		S_BSZFLOWER_RED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_PURPLE
		1411,           // doomednum
		S_BSZFLOWER_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_BLUE
		1412,           // doomednum
		S_BSZFLOWER_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_CYAN
		1413,           // doomednum
		S_BSZFLOWER_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_YELLOW
		1414,           // doomednum
		S_BSZFLOWER_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_ORANGE
		1415,           // doomednum
		S_BSZFLOWER_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_RED
		1420,           // doomednum
		S_BSZSHORTFLOWER_RED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_PURPLE
		1421,           // doomednum
		S_BSZSHORTFLOWER_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_BLUE
		1422,           // doomednum
		S_BSZSHORTFLOWER_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_CYAN
		1423,           // doomednum
		S_BSZSHORTFLOWER_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_YELLOW
		1424,           // doomednum
		S_BSZSHORTFLOWER_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_ORANGE
		1425,           // doomednum
		S_BSZSHORTFLOWER_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_RED
		1430,           // doomednum
		S_BSZTULIP_RED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_PURPLE
		1431,           // doomednum
		S_BSZTULIP_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_BLUE
		1432,           // doomednum
		S_BSZTULIP_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_CYAN
		1433,           // doomednum
		S_BSZTULIP_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_YELLOW
		1434,           // doomednum
		S_BSZTULIP_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_ORANGE
		1435,           // doomednum
		S_BSZTULIP_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_RED
		1440,           // doomednum
		S_BSZCLUSTER_RED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_PURPLE
		1441,           // doomednum
		S_BSZCLUSTER_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_BLUE
		1442,           // doomednum
		S_BSZCLUSTER_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_CYAN
		1443,           // doomednum
		S_BSZCLUSTER_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_YELLOW
		1444,           // doomednum
		S_BSZCLUSTER_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_ORANGE
		1445,           // doomednum
		S_BSZCLUSTER_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_RED
		1450,           // doomednum
		S_BSZBUSH_RED,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_PURPLE
		1451,           // doomednum
		S_BSZBUSH_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_BLUE
		1452,           // doomednum
		S_BSZBUSH_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_CYAN
		1453,           // doomednum
		S_BSZBUSH_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_YELLOW
		1454,           // doomednum
		S_BSZBUSH_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_ORANGE
		1455,           // doomednum
		S_BSZBUSH_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_RED
		1460,           // doomednum
		S_BSZVINE_RED,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_PURPLE
		1461,           // doomednum
		S_BSZVINE_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_BLUE
		1462,           // doomednum
		S_BSZVINE_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_CYAN
		1463,           // doomednum
		S_BSZVINE_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_YELLOW
		1464,           // doomednum
		S_BSZVINE_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_ORANGE
		1465,           // doomednum
		S_BSZVINE_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHRUB
		1470,           // doomednum
		S_BSZSHRUB,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLOVER
		1471,           // doomednum
		S_BSZCLOVER,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIG_PALMTREE_TRUNK
		-1,             // doomednum
		S_BIG_PALMTREE_TRUNK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		160*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIG_PALMTREE_TOP
		1473,           // doomednum
		S_BIG_PALMTREE_TOP, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		160*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_RUNSPAWNFUNC|MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PALMTREE_TRUNK
		-1,             // doomednum
		S_PALMTREE_TRUNK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PALMTREE_TOP
		1475,           // doomednum
		S_PALMTREE_TOP, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_RUNSPAWNFUNC|MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DBALL
		1875,           // doomednum
		S_DBALL1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		54*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGSTATUE2
		1876,           // doomednum
		S_EGGSTATUE2,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_PUSHABLE|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SUPERSPARK
		-1,             // doomednum
		S_SSPK1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// Bluebird
	{           // MT_FLICKY_01
		-1,             // doomednum
		S_FLICKY_01_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_01_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_01_CENTER
		2200,             // doomednum
		S_FLICKY_01_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_02
		-1,             // doomednum
		S_FLICKY_02_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_02_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_02_CENTER
		2201,             // doomednum
		S_FLICKY_02_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_03
		-1,             // doomednum
		S_FLICKY_03_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_03_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_03_CENTER
		2202,             // doomednum
		S_FLICKY_03_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_04
		-1,             // doomednum
		S_FLICKY_04_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_04_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_FLICKY_04_SWIM1, // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLICKY_04_CENTER
		2203,             // doomednum
		S_FLICKY_04_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_05
		-1,             // doomednum
		S_FLICKY_05_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_05_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_05_CENTER
		2204,             // doomednum
		S_FLICKY_05_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_06
		-1,             // doomednum
		S_FLICKY_06_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_06_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_06_CENTER
		2205,             // doomednum
		S_FLICKY_06_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_07
		-1,             // doomednum
		S_FLICKY_07_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_07_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_FLICKY_07_SWIM1, // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLICKY_07_CENTER
		2206,             // doomednum
		S_FLICKY_07_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_08
		-1,             // doomednum
		S_FLICKY_08_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_08_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_FLICKY_08_SWIM1, // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLICKY_08_CENTER
		2207,             // doomednum
		S_FLICKY_08_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_09
		-1,             // doomednum
		S_FLICKY_09_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_09_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_09_CENTER
		2208,             // doomednum
		S_FLICKY_09_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_10
		-1,             // doomednum
		S_FLICKY_10_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_10_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_10_CENTER
		2209,             // doomednum
		S_FLICKY_10_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_11
		-1,             // doomednum
		S_FLICKY_11_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_11_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_11_CENTER
		2210,             // doomednum
		S_FLICKY_11_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_12
		-1,             // doomednum
		S_FLICKY_12_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_12_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_12_CENTER
		2211,             // doomednum
		S_FLICKY_12_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_13
		-1,             // doomednum
		S_FLICKY_13_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_13_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_13_CENTER
		2212,             // doomednum
		S_FLICKY_13_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_14
		-1,             // doomednum
		S_FLICKY_14_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_14_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_14_CENTER
		2213,             // doomednum
		S_FLICKY_14_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_15
		-1,             // doomednum
		S_FLICKY_15_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_15_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_15_CENTER
		2214,             // doomednum
		S_FLICKY_15_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_16
		-1,             // doomednum
		S_FLICKY_16_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_16_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_16_CENTER
		2215,             // doomednum
		S_FLICKY_16_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_SECRETFLICKY_01
		-1,             // doomednum
		S_SECRETFLICKY_01_OUT, // spawnstate
		1000,           // spawnhealth
		S_SECRETFLICKY_01_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_SECRETFLICKY_01_CENTER
		2216,             // doomednum
		S_SECRETFLICKY_01_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_SECRETFLICKY_02
		-1,             // doomednum
		S_SECRETFLICKY_02_OUT, // spawnstate
		1000,           // spawnhealth
		S_SECRETFLICKY_02_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_SECRETFLICKY_02_CENTER
		2217,             // doomednum
		S_SECRETFLICKY_02_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_SEED
		-1,             // doomednum
		S_SEED,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-2*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_RAIN
		-1,             // doomednum
		S_RAIN1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_rainin,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPLASH1,      // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		72*FRACUNIT,    // speed
		1*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		80,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_SNOWFLAKE
		-1,             // doomednum
		S_SNOW1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		2,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_BLIZZARDSNOW
		-1,             // doomednum
		S_BLIZZARDSNOW1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		24*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		2,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_SPLISH
		-1,             // doomednum
		S_SPLISH1,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		6*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_LAVASPLISH
		-1,             // doomednum
		S_LAVASPLISH,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		6*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SMOKE
		-1,             // doomednum
		S_SMOKE1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SMALLBUBBLE
		-1,             // doomednum
		S_SMALLBUBBLE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MEDIUMBUBBLE
		-1,             // doomednum
		S_MEDIUMBUBBLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EXTRALARGEBUBBLE
		-1,             // doomednum
		S_LARGEBUBBLE1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_POP1,         // deathstate
		S_NULL,         // xdeathstate
		sfx_gasp,       // deathsound
		8,              // speed
		23*FRACUNIT,    // radius
		43*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_SCENERY, // flags
		S_EXTRALARGEBUBBLE // raisestate
	},

	{           // MT_WATERZAP
		-1,             // doomednum
		S_WATERZAP,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPINDUST
		-1,             // doomednum
		S_SPINDUST1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_PARTICLE
		-1,             // doomednum
		S_PARTICLE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		1,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_PARTICLEGEN
		757,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DROWNNUMBERS
		-1,             // doomednum
		S_ZERO1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		113,            // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// ambient sound effect
	{           // MT_AMBIENT
		700,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORK
		-1,             // doomednum
		S_CORK,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_corkp,      // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SMOKE1,       // deathstate
		S_NULL,         // xdeathstate
		sfx_corkh,      // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_LHRT
		-1,             // doomednum
		S_LHRT,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_SPRK1,        // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXIS
		1700,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		256*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXISTRANSFER
		1701,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		16*FRACUNIT,    // radius
		1,              // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP,    // flags
		S_NULL          // raisestate
	},

	{           // MT_AXISTRANSFERLINE
		1702,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		32*FRACUNIT,    // radius
		1,              // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP,    // flags
		S_NULL          // raisestate
	},

	{           // MT_HOOP
		-1,             // doomednum
		S_HOOP,         // spawnstate
		1000,           // spawnhealth
		S_HOOP_XMASA,   // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_HOOPCOLLIDE
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_SPECIAL|MF_NOTHINK, // flags
		S_NULL          // raisestate
	},

	{           // MT_HOOPCENTER
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGCAPSULE
		1710,           // doomednum
		S_EGGCAPSULE,   // spawnstate
		20,             // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		72*FRACUNIT,    // radius
		144*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_SMASHINGSPIKEBALL
		3000,           // doomednum
		S_SMASHSPIKE_FLOAT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		18*FRACUNIT,    // radius
		28*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_TELEPORTMAN
		751,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ALTVIEWMAN
		752,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CRUMBLEOBJ
		-1,             // doomednum
		S_CRUMBLE1,     // spawnstate
		1000,           // spawnhealth
		S_CRUMBLE1,     // seestate
		0,              // seesound
		1,              // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		200,            // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_crumbl,     // deathsound
		3,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// Waypoint for zoom tubes
	{           // MT_TUBEWAYPOINT
		753,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		2*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT,    // flags
		S_NULL          // raisestate
	},

	// for use with wind and current effects
	{           // MT_PUSH
		754,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY,  // flags
		S_NULL          // raisestate
	},

	{           // MT_GHOST
		-1,             // doomednum
		S_THOK,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		-1,             // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_FAKESHADOW
		-1,             // doomednum
		S_THOK,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		-1,             // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_OVERLAY
		-1,             // doomednum
		S_NULL,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ANGLEMAN
		758,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_POLYANCHOR
		760,            // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_POLYSPAWN
		761,            // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_MINIMAPBOUND
		770,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SKYBOX
		780,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPARK
		-1,             // doomednum
		S_SPRK1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		2,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EXPLODE
		-1,             // doomednum
		S_XPLD1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_UWEXPLODE
		-1,             // doomednum
		S_WPLD1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1*FRACUNIT,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DUST
		-1,             // doomednum
		S_DUST1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		3*FRACUNIT,     // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKSPAWNER
		1202,           // doomednum
		S_ROCKSPAWN,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY,  // flags
		S_NULL          // raisestate
	},

	{           // MT_FALLINGROCK
		-1,             // doomednum
		S_ROCKCRUMBLEA, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		4,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_rocks1,     // activesound
		MF_SOLID, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE1
		-1,             // doomednum
		S_ROCKCRUMBLEA, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE2
		-1,             // doomednum
		S_ROCKCRUMBLEB, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE3
		-1,             // doomednum
		S_ROCKCRUMBLEC, //spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE4
		-1,             // doomednum
		S_ROCKCRUMBLED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE5
		-1,             // doomednum
		S_ROCKCRUMBLEE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE6
		-1,             // doomednum
		S_ROCKCRUMBLEF, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE7
		-1,             // doomednum
		S_ROCKCRUMBLEG, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE8
		-1,             // doomednum
		S_ROCKCRUMBLEH, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE9
		-1,             // doomednum
		S_ROCKCRUMBLEI, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE10
		-1,             // doomednum
		S_ROCKCRUMBLEJ, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE11
		-1,             // doomednum
		S_ROCKCRUMBLEK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE12
		-1,             // doomednum
		S_ROCKCRUMBLEL, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE13
		-1,             // doomednum
		S_ROCKCRUMBLEM, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE14
		-1,             // doomednum
		S_ROCKCRUMBLEN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE15
		-1,             // doomednum
		S_ROCKCRUMBLEO, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE16
		-1,             // doomednum
		S_ROCKCRUMBLEP, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZDEBRIS
		-1,             // doomednum
		S_GFZDEBRIS,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_RUNSPAWNFUNC|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BRICKDEBRIS
		-1,             // doomednum
		S_BRICKDEBRIS,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_RUNSPAWNFUNC|MF_NOCLIPHEIGHT|MF_SCENERY,  // flags
		S_NULL          // raisestate
	},

	{           // MT_WOODDEBRIS
		-1,             // doomednum
		S_WOODDEBRIS,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_wbreak,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_RUNSPAWNFUNC|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// SRB2kart MT's
	{           // MT_RANDOMITEM
		2000,             // doomednum
		S_RINGBOX1,    // spawnstate
		1000,             // spawnhealth
		S_NULL,           // seestate
		sfx_None,         // seesound
		0,                // reactiontime
		sfx_None,         // attacksound
		S_NULL,           // painstate
		0,                // painchance
		sfx_None,         // painsound
		S_NULL,           // meleestate
		S_NULL,           // missilestate
		S_NULL,           // deathstate
		S_NULL,           // xdeathstate
		sfx_kc2e,         // deathsound
		60*FRACUNIT,      // speed
		48*FRACUNIT,      // radius
		64*FRACUNIT,      // height
		0,                // display offset
		100,              // mass
		0,                // damage
		sfx_None,         // activesound
		MF_NOSQUISH|MF_PICKUPFROMBELOW|MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_RANDOMITEM1     // raisestate
	},

	{           // MT_SPHEREBOX
		-1,               // doomednum
		S_SPHEREBOX1,     // spawnstate
		1000,             // spawnhealth
		S_NULL,           // seestate
		sfx_None,         // seesound
		0,                // reactiontime
		sfx_None,         // attacksound
		S_NULL,           // painstate
		0,                // painchance
		sfx_None,         // painsound
		S_NULL,           // meleestate
		S_NULL,           // missilestate
		S_NULL,           // deathstate
		S_NULL,           // xdeathstate
		sfx_kc2e,         // deathsound
		60*FRACUNIT,      // speed
		48*FRACUNIT,      // radius
		64*FRACUNIT,      // height
		0,                // display offset
		100,              // mass
		0,                // damage
		sfx_None,         // activesound
		MF_NOSQUISH|MF_PICKUPFROMBELOW|MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL            // raisestate
	},

	{           // MT_FLOATINGITEM
		-1,             // doomednum
		S_ITEMICON,     // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_mbs54,      // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_PICKUPFROMBELOW|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_GOTPOWERUP
		-1,             // doomednum
		S_ITEMICON,     // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_mbs54,      // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOSQUISH|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ITEMCAPSULE
		2010,           // doomednum
		S_ITEMCAPSULE,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_INVISIBLE,    // deathstate
		S_NULL,         // xdeathstate
		sfx_itcaps,     // deathsound
		0,              // speed
		56*FRACUNIT,    // radius
		112*FRACUNIT,   // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ITEMCAPSULE_PART
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		26*FRACUNIT,    // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_MONITOR
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		FRACUNIT,       // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_MONITOR_DAMAGE, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_MONITOR_DEATH, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		112*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_SLIDEME|MF_DONTENCOREMAP|MF_NOHITLAGFORME, // flags
		S_NULL          // raisestate
	},

	{           // MT_MONITOR_PART
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		112*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOSQUISH|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_MONITOR_SHARD
		-1,             // doomednum
		S_MONITOR_BIG_SHARD, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOSQUISH|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_MAGICIANBOX
		-1,             // doomednum
		S_MAGICIANBOX,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		20*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_MINERADIUS
		-1,             // doomednum
		S_MINERADIUS,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		0*FRACUNIT,    // radius
		0*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_WAVEDASH
		-1,             // doomednum
		S_WAVEDASH,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		20*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_INSTAWHIP
		-1,             // doomednum
		S_INSTAWHIP,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		108*FRACUNIT,    // radius
		50*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_INSTAWHIP_RECHARGE
		-1,             // doomednum
		S_INSTAWHIP_RECHARGE1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		90*FRACUNIT,    // radius
		90*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOCLIPTHING|MF_DONTENCOREMAP|MF_NOSQUISH, // flags
		S_INSTAWHIP_RECHARGE3 // raisestate
	},

	{               // MT_INSTAWHIP_REJECT
		-1,             // doomednum
		S_INSTAWHIP_REJECT,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		90*FRACUNIT,    // radius
		90*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLOCKRING
		-1,             // doomednum
		S_BLOCKRING,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLOCKBODY
		-1,             // doomednum
		S_BLOCKBODY,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BAIL
		-1,             // doomednum
		S_BAIL,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BAILCHARGE
		-1,             // doomednum
		S_BAILCHARGE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BAILSPARKLE
		-1,             // doomednum
		S_BAIC,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_AMPRING
		-1,             // doomednum
		S_AMPRING,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		3,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_AMPBODY
		-1,             // doomednum
		S_AMPBODY,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		2,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_AMPAURA
		-1,             // doomednum
		S_AMPAURA,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_AMPBURST
		-1,             // doomednum
		S_AMPBURST,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SONICBOOM
		-1,             // doomednum
		S_SONICBOOM,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_TRIPWIREOK
		-1,             // doomednum
		S_TRIPWIREOK,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

{           // MT_TRIPWIRELOCKOUT
		-1,             // doomednum
		S_TRIPWIRELOCKOUT,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_GOTIT
		-1,             // doomednum
		S_GOTIT,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		2,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHARGEAURA
		-1,             // doomednum
		S_CHARGEAURA,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHARGEFALL
		-1,             // doomednum
		S_CHARGEFALL,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHARGEFLICKER
		-1,             // doomednum
		S_CHARGEFLICKER,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHARGESPARK
		-1,             // doomednum
		S_CHARGESPARK,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHARGERELEASE
		-1,             // doomednum
		S_CHARGERELEASE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHARGEEXTRA
		-1,             // doomednum
		S_CHARGEEXTRA,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		67*FRACUNIT,    // radius
		67*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SERVANTHAND
		-1,             // doomednum
		S_SERVANTHAND,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HORNCODE
		-1,             // doomednum
		S_HORNCODE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		36*FRACUNIT,    // radius
		36*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SIGNSPARKLE
		-1,              // doomednum
		S_SIGNSPARK1,    // spawnstate
		1,               // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		8,               // speed
		14*FRACUNIT,     // radius
		14*FRACUNIT,     // height
		0,               // display offset
		100,             // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL           // raisestate
	},

	{           // MT_FASTLINE
		-1,						// doomednum
		S_FASTLINE1,            // spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		14*FRACUNIT,			// radius
		14*FRACUNIT,			// height
		0,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_FASTDUST
		-1,						// doomednum
		S_FASTDUST1,            // spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		14*FRACUNIT,			// radius
		14*FRACUNIT,			// height
		0,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_DRIFTEXPLODE
		-1,						// doomednum
		S_DRIFTEXPLODE1,		// spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		32*FRACUNIT,			// radius
		64*FRACUNIT,			// height
		1,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_DRIFTCLIP
		-1,						// doomednum
		S_DRIFTCLIPA1,		// spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		105,						// speed
		32*FRACUNIT,			// radius
		64*FRACUNIT,			// height
		1,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_DONTENCOREMAP|MF_GRENADEBOUNCE, // flags
		S_NULL					// raisestate
	},

	{           // MT_DRIFTCLIPSPARK
		-1,						// doomednum
		S_DRIFTCLIPSPARK,		// spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		32*FRACUNIT,			// radius
		64*FRACUNIT,			// height
		1,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_BOOSTFLAME
		-1,						// doomednum
		S_BOOSTFLAME,	        // spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		32*FRACUNIT,			// radius
		64*FRACUNIT,			// height
		1,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_BOOSTSMOKE
		-1,						// doomednum
		S_BOOSTSMOKE1,	        // spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		8*FRACUNIT,				// radius
		8*FRACUNIT,				// height
		1,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_SNEAKERTRAIL
		-1,             // doomednum
		S_KARTFIRE1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		14*FRACUNIT,    // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_SLIDEME|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_AIZDRIFTSTRAT
		-1,             // doomednum
		S_KARTAIZDRIFTSTRAT,// spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		14*FRACUNIT,    // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPARKLETRAIL
		-1,						// doomednum
		S_NULL,	                // spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		14*FRACUNIT,			// radius
		14*FRACUNIT,			// height
		0,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_INVULNFLASH
		-1,						// doomednum
		S_INVULNFLASH1,	        // spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		8*FRACUNIT,				// radius
		8*FRACUNIT,				// height
		1,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_WIPEOUTTRAIL
		-1,              // doomednum
		S_WIPEOUTTRAIL1, // spawnstate
		1,               // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		8,               // speed
		14*FRACUNIT,     // radius
		14*FRACUNIT,     // height
		0,               // display offset
		100,             // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL           // raisestate
	},

	{           // MT_DRIFTSPARK
		-1,             // doomednum
		S_DRIFTSPARK_B1,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		12,             // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		14*FRACUNIT,    // radius
		14*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BRAKEDRIFT
		-1,						// doomednum
		S_BRAKEDRIFT,			// spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		8*FRACUNIT,				// radius
		8*FRACUNIT,				// height
		1,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_BRAKEDUST
		-1,						// doomednum
		S_BRAKEDUST1,			// spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		8*FRACUNIT,				// radius
		8*FRACUNIT,				// height
		1,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP|MF_NOSQUISH, // flags
		S_NULL					// raisestate
	},

	{           // MT_DRIFTDUST
		-1,             // doomednum
		S_DRIFTDUST1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		12,             // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		15*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_FLOAT|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ITEM_DEBRIS
		-1,             // doomednum
		S_ITEM_DEBRIS,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ITEM_DEBRIS_CLOUD_SPAWNER
		-1,             // doomednum
		S_ITEM_DEBRIS_CLOUD_SPAWNER1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_DRIFTELECTRICITY
		-1,             // doomednum
		S_DRIFTELECTRICITY, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOCLIP|MF_NOCLIPTHING|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_DRIFTELECTRICSPARK
		-1,             // doomednum
		S_DRIFTELECTRICSPARK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		9*FRACUNIT,     // radius
		37*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOCLIP|MF_NOCLIPTHING|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_JANKSPARK
		-1,             // doomednum
		S_JANKSPARK1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_FLOAT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HITLAG
		-1,             // doomednum
		S_HITLAG_1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKETSNEAKER
		-1,             // doomednum
		S_ROCKETSNEAKER_L, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k5d,      // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_s3kc0s,     // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMANITEM
		2052,           // doomednum
		S_EGGMANITEM1,  // spawnstate
		2,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_EGGMANITEM_DEAD, // deathstate
		S_NULL,         // xdeathstate
		sfx_kc2e,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_cdfm28,     // activesound
		MF_NOSQUISH|MF_PICKUPFROMBELOW|MF_SHOOTABLE|MF_DONTENCOREMAP|MF_APPLYTERRAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMANITEM_SHIELD
		-1,             // doomednum
		S_EGGMANITEM1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_EGGMANITEM_DEAD, // deathstate
		S_NULL,         // xdeathstate
		sfx_kc2e,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOSQUISH|MF_PICKUPFROMBELOW|MF_SHOOTABLE|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP|MF_APPLYTERRAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_BANANA
		2051,           // doomednum
		S_BANANA,       // spawnstate
		2,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BANANA_DEAD,  // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_peel,       // activesound
		MF_SHOOTABLE|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_BANANA_SHIELD
		-1,             // doomednum
		S_BANANA,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BANANA_DEAD,  // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,				// speed
		10*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SHOOTABLE|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_BANANA_SPARK
		-1,             // doomednum
		S_BANANA_SPARK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BANANA_SPARK2,// deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_DONTENCOREMAP|MF_NOCLIPTHING|MF_NOSQUISH, // flags
		S_NULL          // raisestate
	},

	{           // MT_ORBINAUT
		-1,             // doomednum
		S_ORBINAUT1,    // spawnstate
		7,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		8,              // reactiontime
		sfx_s3k49,      // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_ORBINAUT_DEAD,// deathstate
		S_NULL,         // xdeathstate
		sfx_s3k5d,      // deathsound
		28*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_s3k96,      // activesound
		MF_SHOOTABLE|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_ORBINAUT_SHIELD
		-1,             // doomednum
		S_ORBINAUT_SHIELD1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_ORBINAUT_SHIELDDEAD, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SHOOTABLE|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_JAWZ
		-1,             // doomednum
		S_JAWZ1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_JAWZ_DEAD1,   // deathstate
		S_JAWZ_DEAD2,   // xdeathstate
		sfx_s3k5d,      // deathsound
		28*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_s3kc0s,     // activesound
		MF_SHOOTABLE|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT,   // flags
		S_NULL          // raisestate
	},

	{           // MT_JAWZ_SHIELD
		-1,             // doomednum
		S_JAWZ_SHIELD1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_JAWZ_DEAD1,   // deathstate
		S_JAWZ_DEAD2,   // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SHOOTABLE|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_PLAYERRETICULE
		-1,             // doomednum
		S_PLAYERRETICULE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		2,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SSMINE
		2053,					// doomednum
		S_SSMINE_AIR1,			// spawnstate
		1,						// spawnhealth
		S_NULL,					// seestate
		sfx_tossed,				// seesound
		0,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		192*FRACUNIT,			// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_SSMINE_EXPLODE,		// deathstate
		S_NULL,					// xdeathstate
		sfx_gshc5,				// deathsound
		0,						// speed
		16*FRACUNIT,			// radius
		56*FRACUNIT,			// height
		0,						// display offset
		100,					// mass
		1,						// damage
		sfx_s3k5c,				// activesound
		MF_SHOOTABLE|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL					// raisestate
	},

	{           // MT_SSMINE_SHIELD
		-1,             // doomednum
		S_SSMINE_SHIELD1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		192*FRACUNIT,   // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SSMINE_EXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SHOOTABLE|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SMOLDERING
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		35*FRACUNIT,    // radius
		70*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOOMEXPLODE
		-1,             // doomednum
		S_SLOWBOOM1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOOMPARTICLE
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_LANDMINE
		2054,					// doomednum
		S_LANDMINE,				// spawnstate
		2,						// spawnhealth
		S_NULL,					// seestate
		sfx_tossed,				// seesound
		0,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_LANDMINE_EXPLODE,		// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		128*FRACUNIT,			// speed
		24*FRACUNIT,			// radius
		32*FRACUNIT,			// height
		0,						// display offset
		0,						// mass
		0,						// damage
		sfx_s3k5c,				// activesound
		MF_SHOOTABLE|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL					// raisestate
	},

	{           // MT_DROPTARGET
		2056,           // doomednum
		S_DROPTARGET,   // spawnstate
		3,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_DROPTARGET_SPIN, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DROPTARGET_SPIN, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		128*FRACUNIT,	// speed
		45*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_s3k96,      // activesound
		MF_SPECIAL|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_DROPTARGET_SHIELD
		-1,             // doomednum
		S_DROPTARGET,   // spawnstate
		3,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_DROPTARGET_SPIN, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DROPTARGET_SPIN, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,				// speed
		45*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_DROPTARGET_MORPH
		-1,             // doomednum
		S_DROPTARGET,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,				// speed
		45*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BALLHOG
		-1,             // doomednum
		S_BALLHOG1,     // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BALLHOG_DEAD, // deathstate
		S_NULL,         // xdeathstate
		sfx_gshdd,     // deathsound
		0,              // speed
		26*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SHOOTABLE|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_BALLHOGBOOM
		-1,             // doomednum
		S_BALLHOGBOOM, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		50*FRACUNIT,    // radius
		50*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BALLHOG_RETICULE
		-1,             // doomednum
		S_BALLHOG_RETICULE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		26*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BALLHOG_RETICULE_TEST
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		26*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING|MF_DONTENCOREMAP|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPB
		-1,             // doomednum
		S_SPB1,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		8,              // reactiontime
		sfx_kc57,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPB_DEAD,     // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k5d,      // deathsound
		80*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_kc64,       // activesound
		MF_SOLID|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPBEXPLOSION
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		192*FRACUNIT,   // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SSMINE_EXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_MANTARING
		-1,             // doomednum
		S_MANTA1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_LIGHTNINGSHIELD
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		28*FRACUNIT,    // radius
		0*FRACUNIT,    // height
		1,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_LIGHTNINGSHIELD_VISUAL
		-1,             // doomednum
		S_THNC1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		28*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		1,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_LIGHTNINGATTACK_VISUAL
		-1,             // doomednum
		S_THND,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		28*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		1,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUBBLESHIELD
		-1,             // doomednum
		S_BUBBLESHIELD1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		28*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		1,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUBBLESHIELD_VISUAL
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		28*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		1,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMESHIELD
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		28*FRACUNIT,    // radius
		0*FRACUNIT,    // height
		1,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMESHIELD_VISUAL
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		28*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		1,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMESHIELDUNDERLAY
	    -1,             // doomednum
	    S_FLAMESHIELDDASH2_UNDERLAY, // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    0<<FRACBITS,   // height
	    -1,             // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
	    S_NULL          // raisestate
	},

	{           // MT_FLAMESHIELDPAPER
	    -1,             // doomednum
	    S_FLAMESHIELDPAPER, // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    0<<FRACBITS,   // height
	    1,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
	    S_NULL          // raisestate
	},

	{           // MT_BUBBLESHIELDTRAP
		-1,             // doomednum
		S_BUBBLESHIELDTRAP1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		128*FRACUNIT,   // speed
		28*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		2,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_GARDENTOP
		-1,             // doomednum
		S_GARDENTOP_FLOATING, // spawnstate
		8,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		4,              // reactiontime
		sfx_s3k8b,      // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GARDENTOP_DEAD, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k7a,      // deathsound
		30*FRACUNIT,    // speed
		30*FRACUNIT,    // radius
		68*FRACUNIT,    // height
		-1,             // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING|MF_DONTENCOREMAP|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_GARDENTOPSPARK
		-1,             // doomednum
		S_GARDENTOPSPARK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP|MF_NOSQUISH, // flags
		S_NULL          // raisestate
	},

	{           // MT_GARDENTOPARROW
		-1,             // doomednum
		S_GARDENTOPARROW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		54*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP|MF_NOSQUISH, // flags
		S_NULL          // raisestate
	},

	{           // MT_HYUDORO
		-1,             // doomednum
		S_HYUDORO,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_NOSQUISH, // flags
		S_NULL          // raisestate
	},

	{           // MT_HYUDORO_CENTER
		2055,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_GROW_PARTICLE
		-1,             // doomednum
		S_GROW_PARTICLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SHRINK_POHBEE
		-1,             // doomednum
		S_SHRINK_POHBEE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP|MF_NOSQUISH, // flags
		S_NULL          // raisestate
	},

	{           // MT_SHRINK_GUN
		-1,             // doomednum
		S_SHRINK_GUN,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		52*FRACUNIT,    // radius
		120*FRACUNIT,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SHRINK_CHAIN
		-1,             // doomednum
		S_SHRINK_CHAIN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		26*FRACUNIT,    // radius
		26*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SHRINK_LASER
		-1,             // doomednum
		S_SHRINK_LASER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		33*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SHRINK_PARTICLE
		-1,             // doomednum
		S_SHRINK_PARTICLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		26*FRACUNIT,    // radius
		26*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SINK
		-1,             // doomednum
		S_SINK,         // spawnstate
		105,            // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,   		// painstate
		100,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k5d,      // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_s3k5c,      // activesound
		MF_SHOOTABLE|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SINK_SHIELD
		-1,             // doomednum
		S_SINK_SHIELD,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		100,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SINKTRAIL
		-1,             // doomednum
		S_SINKTRAIL1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_GACHABOM
		-1,             // doomednum
		S_GACHABOM,     // spawnstate
		7,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		8,              // reactiontime
		sfx_s3k49,      // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GACHABOM_DEAD, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k5d,      // deathsound
		28*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_s3k96,      // activesound
		MF_SHOOTABLE|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_SLOPE|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_GACHABOM_REBOUND
		-1,             // doomednum
		S_GACHABOM_EXPLOSION_1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GACHABOM_DEAD, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP|MF_NOSQUISH, // flags
		S_NULL          // raisestate
	},

	{           // MT_DUELBOMB
		2050,           // doomednum
		S_SPB1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		32*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_DONTENCOREMAP|MF_APPLYTERRAIN|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},

	{           // MT_BATTLEBUMPER
		-1,              // doomednum
		S_BATTLEBUMPER1,// spawnstate
		1000,            // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_BATTLEBUMPER1, // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		4*FRACUNIT,      // speed
		8*FRACUNIT,      // radius
		16*FRACUNIT,     // height
		0,               // display offset
		100,             // mass
		1,               // damage
		sfx_None,        // activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL           // raisestate
	},

	{           // MT_BATTLEBUMPER_DEBRIS
		-1,              // doomednum
		S_BATTLEBUMPER_EXDEBRIS1,// spawnstate
		1000,            // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		8*FRACUNIT,      // radius
		16*FRACUNIT,     // height
		0,               // display offset
		100,             // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL           // raisestate
	},

	{           // MT_BATTLEBUMPER_BLAST
		-1,              // doomednum
		S_BATTLEBUMPER_EXBLAST1, // spawnstate
		1000,            // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		8*FRACUNIT,      // radius
		16*FRACUNIT,     // height
		0,               // display offset
		100,             // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL           // raisestate
	},

	{           // MT_TRIPWIREBOOST
		-1,              // doomednum
		S_TRIPWIREBOOST_TOP, // spawnstate
		1000,            // spawnhealth
		S_TRIPWIREBOOST_BOTTOM, // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_TRIPWIREBOOST_BLAST_TOP,    // meleestate
		S_TRIPWIREBOOST_BLAST_BOTTOM, // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		8*FRACUNIT,      // radius
		16*FRACUNIT,     // height
		1,               // display offset
		100,             // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL           // raisestate
	},

	{           // MT_TRIPWIREAPPROACH
		-1,              // doomednum
		S_TRIPWIREAPPROACH, // spawnstate
		1000,            // spawnhealth
		S_NULL, // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,    // meleestate
		S_NULL, // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		8*FRACUNIT,      // radius
		16*FRACUNIT,     // height
		1,               // display offset
		100,             // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL           // raisestate
	},

	{           // MT_SMOOTHLANDING
		-1,              // doomednum
		S_SMOOTHLANDING, // spawnstate
		1000,            // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		8*FRACUNIT,      // radius
		16*FRACUNIT,     // height
		-1,              // display offset
		100,             // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL           // raisestate
	},

	{           // MT_TRICKINDICATOR
		-1,              // doomednum
		S_INVISIBLE,     // spawnstate
		1000,            // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		128*FRACUNIT,    // radius
		128*FRACUNIT,    // height
		-1,              // display offset
		100,             // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL           // raisestate
	},

	{           // MT_SIDETRICK
		-1,             // doomednum
		S_SIDETRICK,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		36*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_FORWARDTRICK
		-1,             // doomednum
		S_FORWARDTRICK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		60*FRACUNIT,    // radius
		86*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_TIREGRABBER
		-1,             // doomednum
		S_TIREGRABBER,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		36*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_RINGSHOOTER
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_s3ka7,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3kad,      // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_RINGSHOOTER_PART
		-1,             // doomednum
		S_RINGSHOOTER_SIDE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		6*FRACUNIT,     // radius
		70*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_RINGSHOOTER_SCREEN
		-1,             // doomednum
		S_RINGSHOOTER_SCREEN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		23*FRACUNIT,    // radius
		39*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_DEZLASER
		-1,             // doomednum
		S_DEZLASER,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		42*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		1,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP,   // flags
		S_NULL          // raisestate
	},

	{           // MT_WAYPOINT
		2001,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		100,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		2*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WAYPOINT_RISER
		2002,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		100,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		2*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WAYPOINT_ANCHOR
		2003,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		100,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		2*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOTHINT
		2004,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		100,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		2*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_RANDOMAUDIENCE
		1488,           // doomednum
		S_UNKNOWN,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAYM
		1479,           // doomednum
		S_FLAYM1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL|MF_PAIN, // flags
		S_NULL          // raisestate
	},


	{           // MT_PALMTREE2
		2016,           // doomednum
		S_PALMTREE2,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_PURPLEFLOWER1
		3000,           // doomednum
		S_PURPLEFLOWER1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PURPLEFLOWER2
		3001,           // doomednum
		S_PURPLEFLOWER2, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_YELLOWFLOWER1
		3002,           // doomednum
		S_YELLOWFLOWER1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_YELLOWFLOWER2
		3003,           // doomednum
		S_YELLOWFLOWER2, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PLANT2
		4022,           // doomednum
		S_PLANT2,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PLANT3
		4024,           // doomednum
		S_PLANT3,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_PLANT4
		4025,           // doomednum
		S_PLANT4,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIGRING
		2808,           // doomednum
		S_BIGRING01,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		26*FRACUNIT,    // radius
		62*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARKARROW
		4094,           // doomednum
		S_ARKARROW_0,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		43*FRACUNIT,    // radius
		100*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SCENERY|MF_NOCLIP|MF_DRAWFROMFARAWAY|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUMP
		-1,             // doomednum
		S_BUMP1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SCENERY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLINGENERGY
		-1,             // doomednum
		S_FLINGENERGY1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGENERGY, // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ITEMCLASH
		-1,             // doomednum
		S_CLASH1,       // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s259,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_RUNSPAWNFUNC|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_FIREDITEM
		-1,             // doomednum
		S_FIREDITEM1,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_INSTASHIELDA
		-1,						// doomednum
		S_INSTASHIELDA1,	    // spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		8*FRACUNIT,				// radius
		8*FRACUNIT,				// height
		1,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_INSTASHIELDB
		-1,						// doomednum
		S_INSTASHIELDB1,	    // spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		8*FRACUNIT,				// radius
		8*FRACUNIT,				// height
		2,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_POWERCLASH
		-1,						// doomednum
		S_POWERCLASH,	    	// spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		8*FRACUNIT,				// radius
		8*FRACUNIT,				// height
		2,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_GUARDBREAK
		-1,						// doomednum
		S_GUARDBREAK,	    	// spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		8*FRACUNIT,				// radius
		8*FRACUNIT,				// height
		2,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_KARMAHITBOX
		-1,             // doomednum
		S_PLAYERBOMB1,  // spawnstate
		1000,           // spawnhealth
		S_PLAYERITEM1,  // seestate
		sfx_kc2e,       // seesound
		8,              // reactiontime
		sfx_s3k4e,      // attacksound
		S_PLAYERFAKE1,  // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		52*FRACUNIT,    // height
		-1,             // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_KARMAWHEEL
		-1,             // doomednum
		S_KARMAWHEEL,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		-1,             // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BATTLEPOINT
		-1,						// doomednum
		S_INVISIBLE,			// spawnstate
		1000,					// spawnhealth
		S_NULL,					// seestate
		sfx_None,				// seesound
		8,						// reactiontime
		sfx_None,				// attacksound
		S_NULL,					// painstate
		0,						// painchance
		sfx_None,				// painsound
		S_NULL,					// meleestate
		S_NULL,					// missilestate
		S_NULL,					// deathstate
		S_NULL,					// xdeathstate
		sfx_None,				// deathsound
		8,						// speed
		8*FRACUNIT,				// radius
		8*FRACUNIT,				// height
		-1,						// display offset
		100,					// mass
		0,						// damage
		sfx_None,				// activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL					// raisestate
	},

	{           // MT_FZEROBOOM
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_kc31,       // seesound
		8,              // reactiontime
		sfx_kc51,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_DASHRING
		3441,           // doomednum
		S_DASHRING_HORIZONTAL, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_dashr,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		112*FRACUNIT,   // radius
		192*FRACUNIT,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_RAINBOWDASHRING
		3442,           // doomednum
		S_DASHRING_HORIZONTAL, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_rainbr,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		112*FRACUNIT,   // radius
		192*FRACUNIT,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ADVENTUREAIRBOOSTER
		3500,        // doomednum
		S_ADVENTUREAIRBOOSTER, // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_spdpad,  // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		50*FRACUNIT, // radius
		2*FRACUNIT,  // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_supert,  // activesound
		MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL       // raisestate
	},

	{           // MT_ADVENTUREAIRBOOSTER_HITBOX
		-1,          // doomednum
		S_INVISIBLE, // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		127*FRACUNIT, // radius
		256*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_NOCLIPHEIGHT|MF_SPECIAL|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL       // raisestate
	},

	{           // MT_ADVENTUREAIRBOOSTER_PART
		-1,          // doomednum
		S_ADVENTUREAIRBOOSTER_FRAME, // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		32*FRACUNIT, // radius
		64*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL       // raisestate
	},

	{           // MT_SNEAKERPANEL
		510,         // doomednum
		S_SNEAKERPANEL,  // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		91*FRACUNIT, // radius
		16*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_SPECIAL|MF_ENEMY|MF_DONTENCOREMAP, // flags -- NOTE: IIRC MF_ENEMY was added by mappers to make conveyor belt setups more convenient
		S_NULL       // raisestate
	},

	{           // MT_SNEAKERPANELSPAWNER
		511,         // doomednum
		S_INVISIBLE, // spawnstate
		0,           // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		65,          // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		32*FRACUNIT, // radius
		60*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIPTHING, // flags
		S_NULL       // raisestate
	},

	{           // MT_MARBLEFLAMEPARTICLE
		-1,             // doomednum
		S_MARBLEFLAMEPARTICLE,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SCENERY|MF_NOCLIPTHING|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_MARBLETORCH
		1969,           // doomednum
		S_MARBLETORCH,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_MARBLEFLAMEPARTICLE,// painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		45*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOGRAVITY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_MARBLELIGHT
		-1,             // doomednum
		S_MARBLELIGHT,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SCENERY|MF_NOCLIPTHING|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_MARBLEBURNER
		1970,           // doomednum
		S_MARBLEBURNER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SOLID, // flags
		S_NULL          // raisestate
	},

	{           // MT_CDUFO
		4050,           // doomednum
		S_CDUFO,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_CDUFO_DIE,    // deathstate
		S_NULL,         // xdeathstate
		sfx_cdfm19,     // deathsound
		0,              // speed
		55*FRACUNIT,    // radius
		95*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SPECIAL|MF_SHOOTABLE|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_RUSTYLAMP_ORANGE
		1988,           // doomednum
		S_RUSTYLAMP_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		45*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOGRAVITY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_RUSTYCHAIN
		1989,           // doomednum
		S_RUSTYCHAIN,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		45*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_PGTREE
		711,            // doomednum
		S_PGTREE,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		30*FRACUNIT,    // radius
		504*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DAYTONAPINETREE
		3204,           // doomednum
		S_DAYTONAPINETREE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32<<FRACBITS,   // radius
		192<<FRACBITS,  // height
		0,              // display offset
		8,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DAYTONAPINETREE_SIDE
		-1,             // doomednum
		S_DAYTONAPINETREE_SIDE,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32<<FRACBITS,   // radius
		192<<FRACBITS,  // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EZZPROPELLER
		2311,           // doomednum
		S_EZZPROPELLER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3kbds,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		30<<FRACBITS,   // speed
		32<<FRACBITS,   // radius
		48<<FRACBITS,   // height
		0,              // display offset
		3,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY,   // flags
		S_NULL          // raisestate
	},

	{           // MT_EZZPROPELLER_BLADE
		-1,             // doomednum
		S_EZZPROPELLER_BLADE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32<<FRACBITS,   // radius
		48<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOTHINK, // flags
		S_NULL          // raisestate
	},

	{           // MT_DP_PALMTREE
		3742,           // doomednum
		S_DP_PALMTREE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16<<FRACBITS,   // radius
		560<<FRACBITS,  // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_AAZTREE_HELPER
		1950,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		160<<FRACBITS,  // radius
		256<<FRACBITS,  // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOTHINK, // flags
		S_NULL          // raisestate
	},

	{           // MT_AAZTREE_SEG
		-1,             // doomednum
		S_AAZTREE_SEG,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16<<FRACBITS,   // radius
		128<<FRACBITS,  // height
		-1,             // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOTHINK, // flags
		S_NULL          // raisestate
	},

	{           // MT_AAZTREE_COCONUT
		-1,             // doomednum
		S_AAZTREE_COCONUT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16<<FRACBITS,   // radius
		32<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOTHINK, // flags
		S_NULL          // raisestate
	},

	{           // MT_AAZTREE_LEAF
		-1,             // doomednum
		S_AAZTREE_LEAF, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16<<FRACBITS,   // radius
		32<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOTHINK, // flags
		S_NULL          // raisestate
	},

	{           // MT_BBZDUST
		-1,             // doomednum
		S_BBZDUST1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4<<FRACBITS,    // radius
		8<<FRACBITS,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FROGGER
		2005,           // doomednum
		S_FROGGER,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		TICRATE/2,      // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		28<<FRACBITS,   // radius
		72<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FROGTONGUE
		-1,             // doomednum
		S_FROGTONGUE,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3k8c,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		24<FRACBITS,    // speed
		6<<FRACBITS,    // radius
		8<<FRACBITS,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FROGTONGUE_JOINT
		-1,             // doomednum
		S_FROGTONGUE_JOINT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		6<<FRACBITS,    // radius
		6<<FRACBITS,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROBRA
		2006,           // doomednum
		S_ROBRA,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		TICRATE/2,      // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32<<FRACBITS,   // radius
		72<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROBRA_HEAD
		-1,             // doomednum
		S_ROBRA_HEAD,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		TICRATE,        // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		FRACUNIT/4,     // speed
		16<<FRACBITS,   // radius
		32<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_ENEMY|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROBRA_JOINT
		-1,             // doomednum
		S_ROBRA_JOINT,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		14<<FRACBITS,   // radius
		28<<FRACBITS,   // height
		-1,             // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEROBRA
		2007,           // doomednum
		S_BLUEROBRA,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		TICRATE/2,      // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48<<FRACBITS,   // radius
		72<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEROBRA_HEAD
		-1,             // doomednum
		S_BLUEROBRA_HEAD, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		TICRATE,        // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		FRACUNIT/4,     // speed
		32<<FRACBITS,   // radius
		32<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_ENEMY|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEROBRA_JOINT
		-1,             // doomednum
		S_BLUEROBRA_JOINT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32<<FRACBITS,   // radius
		28<<FRACBITS,   // height
		-1,             // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EERIEFOG
		-1,             // doomednum
		S_EERIEFOG1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		128<<FRACBITS,  // radius
		32<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EERIEFOGGEN
		2679,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		TICRATE,        // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8<<FRACBITS,    // radius
		8<<FRACBITS,    // height
		0,              // display offset
		8192,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_NOTHINK, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPECIALSTAGEARCH
		3889,        // doomednum
		S_SPECIALSTAGEARCH, // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		16*FRACUNIT, // radius
		16*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_SCENERY|MF_NOGRAVITY, // flags
		S_NULL       // raisestate
	},

	{           // MT_SPECIALSTAGEBOMB
		3890,        // doomednum
		S_SPECIALSTAGEBOMB, // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_SPECIALSTAGEBOMB_DISARM, // painstate
		0,           // painchance
		sfx_s24b,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		60*FRACUNIT, // radius
		100*FRACUNIT, // height
		0,           // dispoffset
		7,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_SPECIAL,  // flags
		S_SPECIALSTAGEBOMB_EXPLODE // raisestate
	},

	{           // MT_HANAGUMIHALL_STEAM
		2023,        // doomednum
		S_HANAGUMIHALL_STEAM, // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		24*FRACUNIT, // radius
		48*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_NOGRAVITY|MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIPTHING, // flags
		S_NULL       // raisestate
	},

	{           // MT_HANAGUMIHALL_NPC
		2024,        // doomednum
		S_ALFONSO,   // spawnstate
		1,           // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		64*FRACUNIT, // radius
		128*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_DONTENCOREMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY, // flags
		S_NULL       // raisestate
	},

	{           // MT_DVDTRUMPET
		3181,        // doomednum
		S_DVDTRUMPET, // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		8*FRACUNIT,  // radius
		14*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_NOTHINK|MF_SCENERY, // flags
		S_NULL       // raisestate
	},

	{           // MT_DVDPARTICLE
		-1,          // doomednum
		S_DVDSPARK1, // spawnstate
		1,           // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		8,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		2*FRACUNIT,  // speed
		8*FRACUNIT,  // radius
		14*FRACUNIT, // height
		0,           // dispoffset
		100,         // mass
		62*FRACUNIT, // damage
		sfx_None,    // activesound
		MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_NOBLOCKMAP, // flags
		S_NULL       // raisestate
	},

	{           // MT_SUNBEAMPALM_STEM
	    2697,           // doomednum
	    S_SUNBEAMPALM_STEM, // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    60<<FRACBITS,   // radius
	    420<<FRACBITS,  // height
	    0,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPTHING|MF_DRAWFROMFARAWAY, // flags
	    S_NULL          // raisestate
	},

	{           // MT_SUNBEAMPALM_LEAF
	    -1,             // doomednum
	    S_SUNBEAMPALM_LEAF, // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    96<<FRACBITS,   // radius
	    128<<FRACBITS,  // height
	    0,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_DRAWFROMFARAWAY, // flags
	    S_NULL          // raisestate
	},

	{           // MT_KARMAFIREWORK
	    -1,             // doomednum
	    S_KARMAFIREWORK1, // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    16<<FRACBITS,   // height
	    1,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOCLIPTHING|MF_GRENADEBOUNCE, // flags
	    S_NULL          // raisestate
	},


	{           // MT_RINGSPARKS
	    -1,             // doomednum
	    S_RINGSPARKS1,  // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    16<<FRACBITS,   // height
	    1,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
	    S_NULL          // raisestate
	},
	{           // MT_GAINAX
	    -1,             // doomednum
	    S_INVISIBLE,    // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    16<<FRACBITS,   // height
	    1,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
	    S_NULL          // raisestate
	},

	{           // MT_DRAFTDUST
	    -1,             // doomednum
	    S_DRAFTDUST1,   // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    16<<FRACBITS,   // height
	    1,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOBLOCKMAP|MF_DONTENCOREMAP, // flags
	    S_NULL          // raisestate
	},

	{           // MT_SPBDUST
	    -1,             // doomednum
	    S_DRAFTDUST1,   // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    16<<FRACBITS,   // radius
	    32<<FRACBITS,   // height
	    1,              // display offset
	    DMG_NORMAL,     // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOGRAVITY|MF_PAIN|MF_NOHITLAGFORME|MF_DONTENCOREMAP, // flags
	    S_NULL          // raisestate
	},

	{           // MT_TIREGREASE
	    -1,             // doomednum
	    S_TIREGREASE,   // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    16<<FRACBITS,   // height
	    0,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
	    S_NULL          // raisestate
	},

	{           // MT_OVERTIME_PARTICLE
	    -1,             // doomednum
	    S_INVISIBLE,    // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    16<<FRACBITS,   // radius
	    24<<FRACBITS,   // height
	    0,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DRAWFROMFARAWAY|MF_SCENERY|MF_DONTENCOREMAP, // flags
	    S_NULL          // raisestate
	},

	{           // MT_OVERTIME_CENTER
	    3775,           // doomednum
	    S_OVERTIME_CENTER, // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    12<<FRACBITS,   // radius
	    72<<FRACBITS,   // height
	    0,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_SOLID|MF_DONTENCOREMAP, // flags
	    S_NULL          // raisestate
	},

	{           // MT_BATTLECAPSULE
	    2333,           // doomednum
	    S_SHADOW,       // spawnstate
	    1,              // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_INVISIBLE,    // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    64<<FRACBITS,   // radius
	    144<<FRACBITS,  // height
	    0,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_SOLID|MF_SHOOTABLE|MF_DONTENCOREMAP|MF_DONTPUNT, // flags
	    S_NULL          // raisestate
	},

	{           // MT_BATTLECAPSULE_PIECE
	    -1,             // doomednum
	    S_INVISIBLE,    // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    8<<FRACBITS,    // height
	    0,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound
	    MF_SCENERY|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOBLOCKMAP|MF_DONTENCOREMAP, // flags
	    S_NULL          // raisestate
	},

	{           // MT_FOLLOWER
	    -1,             // doomednum
	    S_INVISIBLE, // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    16<<FRACBITS,   // height
	    1,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound

	    MF_NOCLIPTHING|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL,			// raisestate
	},

	{           // MT_FOLLOWERBUBBLE_FRONT
	    -1,             // doomednum
	    S_FOLLOWERBUBBLE_FRONT, // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    16<<FRACBITS,   // height
	    2,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound

	    MF_NOCLIPTHING|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL,			// raisestate
	},

	{           // MT_FOLLOWERBUBBLE_BACK
	    -1,             // doomednum
	    S_FOLLOWERBUBBLE_BACK, // spawnstate
	    1000,           // spawnhealth
	    S_NULL,         // seestate
	    sfx_None,       // seesound
	    8,              // reactiontime
	    sfx_None,       // attacksound
	    S_NULL,         // painstate
	    0,              // painchance
	    sfx_None,       // painsound
	    S_NULL,         // meleestate
	    S_NULL,         // missilestate
	    S_NULL,         // deathstate
	    S_NULL,         // xdeathstate
	    sfx_None,       // deathsound
	    0,              // speed
	    8<<FRACBITS,    // radius
	    16<<FRACBITS,   // height
	    -2,              // display offset
	    100,            // mass
	    0,              // damage
	    sfx_None,       // activesound

	    MF_NOCLIPTHING|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL,			// raisestate
	},

	{           // MT_WATERTRAIL
		-1,             // doomednum
		S_WATERTRAIL1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_WATERTRAILUNDERLAY
		-1,             // doomednum
		S_WATERTRAILUNDERLAY1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPINDASHDUST
		-1,             // doomednum
		S_SPINDASHDUST, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPINDASHWIND
		-1,             // doomednum
		S_SPINDASHWIND, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SOFTLANDING
		-1,             // doomednum
		S_SOFTLANDING1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_DOWNLINE
		-1,             // doomednum
		S_DOWNLINE1, 	// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HOLDBUBBLE
		-1,             // doomednum
		S_HOLDBUBBLE, 	// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_PAPERITEMSPOT
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BEAMPOINT
		2424,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIPTHING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BROLY
		-1,             // doomednum
		S_BROLY1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPECIAL_UFO
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		101,            // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		108*FRACUNIT,   // radius
		180*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SHOOTABLE|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPECIAL_UFO_PIECE
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY|MF_DONTENCOREMAP|MF_NOSQUISH, // flags
		S_NULL          // raisestate
	},

	{           // MT_LOOPENDPOINT
		2020,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		MAXRADIUS,      // radius
		2*MAXRADIUS,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_LOOPCENTERPOINT
		2021,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SUPER_FLICKY
		-1,             // doomednum
		S_SUPER_FLICKY, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_DONTPUNT|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SUPER_FLICKY_CONTROLLER
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BATTLEUFO_SPAWNER
		3786,           // doomednum
		S_INVISIBLE,        // spawnstate
		1000,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,    // deathstate
		S_NULL,         // xdeathstate
		sfx_None,     // deathsound
		0,              // speed
		55*FRACUNIT,    // radius
		95*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BATTLEUFO
		-1,           // doomednum
		S_BATTLEUFO,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BATTLEUFO_DIE,    // deathstate
		S_NULL,         // xdeathstate
		sfx_cdfm19,     // deathsound
		0,              // speed
		60*FRACUNIT,    // radius
		156*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SOLID|MF_SHOOTABLE|MF_DONTENCOREMAP|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_BATTLEUFO_LEG
		-1,           // doomednum
		S_BATTLEUFO_LEG,        // spawnstate
		1000,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,    // deathstate
		S_NULL,         // xdeathstate
		sfx_None,     // deathsound
		-4*FRACUNIT,              // speed
		64*FRACUNIT,    // radius
		55*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

		{           // MT_BATTLEUFO_BEAM
		-1,           // doomednum
		S_BATTLEUFO_BEAM1,        // spawnstate
		1000,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,    // deathstate
		S_NULL,         // xdeathstate
		sfx_None,     // deathsound
		-(FRACUNIT/2),              // speed
		64*FRACUNIT,    // radius
		55*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOGRAVITY|MF_NOCLIP|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_POWERUP_AURA
		-1,             // doomednum
		S_POWERUP_AURA, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		106*FRACUNIT,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHECKPOINT_END
		2030,           // doomednum
		S_CHECKPOINT,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		19*FRACUNIT,    // radius
		75*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SCRIPT_THING
		4096,           // doomednum
		S_TALKPOINT,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SCRIPT_THING_ORB
		-1,           // doomednum
		S_TALKPOINT_ORB, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_RIDEROID
		-1,           // doomednum
		S_RIDEROID,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		30*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_RIDEROIDNODE
		3711,           // doomednum
		S_RIDEROID_ICON,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		80*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_LSZ_BUNGEE
		3440,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		127*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_LSZ_EGGBALLSPAWNER
		3443,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,    // radius
		8*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_LSZ_EGGBALL
		-1,             // doomednum
		S_EGGBALL,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		DMG_TUMBLE,     // mass
		0,              // damage
		sfx_None,       // activesound
		MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_DLZ_HOVER,
		3430,           // doomednum
		S_DLZHOVER,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		100*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,     // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SOLID, // flags
		S_NULL          // raisestate
	},

	{           // MT_DLZ_ROCKET,
		3431,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		100*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		0,     // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_DLZ_RINGVACCUM,
		3433,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		96*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,     			// mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID, 		// flags
		S_NULL          // raisestate
	},

	{           // MT_DLZ_SUCKEDRING,
		-1,           	// doomednum
		S_RING,    		// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,     			// mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY, 	// flags
		S_NULL          // raisestate
	},

	{           // MT_WATERPALACETURBINE
		3400,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,     			// mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP, 	// flags
		S_NULL          // raisestate
	},

	{           // MT_WATERPALACEBUBBLE
		-1,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,     			// mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP, 	// flags
		S_NULL          // raisestate
	},

	{           // MT_WATERPALACEFOUNTAIN
		3401,           // doomednum
		S_WPZFOUNTAIN,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		0,     			// mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID, 		// flags
		S_NULL          // raisestate
	},

	{           // MT_KURAGEN
		3402,           // doomednum
		S_KURAGEN,  	// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		0,     			// mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY, 	// flags
		S_NULL          // raisestate
	},

	{           // MT_KURAGENBOMB
		-1,           	// doomednum
		S_KURAGENBOMB,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		0,     			// mass
		DMG_EXPLODE,    // damage
		sfx_None,       // activesound
		MF_PAIN, 		// flags
		S_NULL          // raisestate
	},

	{           // MT_BALLSWITCH_BALL
		5000,           // doomednum
		S_BALLSWITCH_BALL, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		36*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BALLSWITCH_PAD
		-1,           // doomednum
		S_BALLSWITCH_PAD, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSSARENACENTER
		2130,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPIKEDTARGET
		-1,             // doomednum
		S_SPIKEDTARGET, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLENDEYE_MAIN
		2131,           // doomednum
		S_BLENDEYE_MAIN, // spawnstate
		3,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_BLENDEYE_MAIN, // painstate
		0,              // painchance
		sfx_mbs60,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BLENDEYE_MAIN, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		42*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		DMG_NORMAL,     // mass
		0,              // damage
		sfx_None,       // activesound
		MF_BOSS|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPTHING|MF_SOLID, // flags
		S_BLENDEYE_MAIN_LAUNCHED // raisestate
	},

	{           // MT_BLENDEYE_EYE
		-1,             // doomednum
		S_BLENDEYE_EYE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		14*FRACUNIT,    // radius
		28*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_BLENDEYE_EYE_FLASH // raisestate
	},

	{           // MT_BLENDEYE_GLASS
		-1,             // doomednum
		S_BLENDEYE_GLASS, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_BLENDEYE_GLASS_STRESS, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_PAPERCOLLISION|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLENDEYE_SHIELD
		-1,             // doomednum
		S_BLENDEYE_SHIELD, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		17*FRACUNIT,    // radius
		42*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_PAPERCOLLISION|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLENDEYE_EGGBEATER
		-1,             // doomednum
		S_BLENDEYE_EGGBEATER_EXTEND_1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		26*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		-1,             // display offset
		DMG_TUMBLE,     // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_PAIN, // flags
		S_BLENDEYE_EGGBEATER_SPIN // raisestate
	},

	{           // MT_BLENDEYE_GENERATOR
		-1,             // doomednum
		S_BLENDEYE_GENERATOR, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BLENDEYE_GENERATOR_BUSTED_L, // deathstate
		S_BLENDEYE_GENERATOR_BUSTED_R, // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		26*FRACUNIT,    // radius
		42*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SHOOTABLE|MF_PAPERCOLLISION|MF_SOLID, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLENDEYE_PUYO
		-1,             // doomednum
		S_BLENDEYE_PUYO_SPAWN_1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BLENDEYE_PUYO_DIE, // deathstate
		S_NULL,         // xdeathstate
		sfx_mbs4c,      // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		12*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOHITLAGFORME, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLENDEYE_PUYO_DUST
		-1,             // doomednum
		S_BLENDEYE_PUYO_DUST, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLENDEYE_PUYO_DUST_COFFEE
		-1,             // doomednum
		S_BLENDEYE_PUYO_DUST, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_AHZ_CLOUD
		-1,             // doomednum
		S_AHZCLOUD,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64<<FRACBITS,   // radius
		64<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL,   // flags
		S_NULL          // raisestate
	},

	{           // MT_AHZ_CLOUDCLUSTER
		3486,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64<<FRACBITS,   // radius
		64<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY,   // flags
		S_NULL          // raisestate
	},

	{           // MT_AGZ_BULB
		3445,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48<<FRACBITS,   // radius
		64<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,   // flags
		S_NULL          // raisestate
	},

	{           // MT_AGZ_BULB_PART
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48<<FRACBITS,   // radius
		64<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY,   // flags
		S_NULL          // raisestate
	},

	{           // MT_AGZ_TREE
		3447,           // doomednum
		S_AGTR,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32<<FRACBITS,   // radius
		32<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		0,              // flags
		S_NULL          // raisestate
	},

	{           // MT_AGZ_AGFL
		3448,           // doomednum
		S_AGFL,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32<<FRACBITS,   // radius
		32<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		0,              // flags
		S_NULL          // raisestate
	},

	{           // MT_AGZ_AGFF
		3449,           // doomednum
		S_AGFF,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32<<FRACBITS,   // radius
		32<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		0,              // flags
		S_NULL          // raisestate
	},

	{           // MT_AGZ_CLOUD
		-1,             // doomednum
		S_AGZCLOUD,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64<<FRACBITS,   // radius
		64<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL,   // flags
		S_NULL          // raisestate
	},

	{           // MT_AGZ_CLOUDCLUSTER
		3446,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64<<FRACBITS,   // radius
		64<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY,   // flags
		S_NULL          // raisestate
	},

	{           // MT_SSZ_CLOUD
		-1,             // doomednum
		S_SSZCLOUD,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64<<FRACBITS,   // radius
		64<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL,   // flags
		S_NULL          // raisestate
	},

	{           // MT_SSZ_CLOUDCLUSTER
		3456,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64<<FRACBITS,   // radius
		64<<FRACBITS,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY,   // flags
		S_NULL          // raisestate
	},

	{           // MT_MEGABARRIER
		-1,             // doomednum
		S_MEGABARRIER1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		0,              // radius
		0,              // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOSQUISH, // flags
		S_NULL          // raisestate
	},

	{           // MT_SEASAW_VISUAL,
		-1,           	// doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		0,     			// mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT, 	// flags
		S_NULL          // raisestate
	},

	{           // MT_DLZ_SEASAW_SPAWN,
		3432,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,     // mass
		0,              // damage
		sfx_None,       // activesound
		0, // flags
		S_NULL          // raisestate
	},

	{           // MT_DLZ_SEASAW_HITBOX,
		-1,          	// doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		0,    	 		// mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID, 		// flags
		S_NULL          // raisestate
	},

	{           // MT_GPZ_SEASAW_SPAWN,
		3435,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,     // mass
		0,              // damage
		sfx_None,       // activesound
		0, // flags
		S_NULL          // raisestate
	},

	{           // MT_GPZ_SEASAW_HITBOX,
		-1,          	// doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		0,    	 		// mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID, 		// flags
		S_NULL          // raisestate
	},

	{           // MT_GPZ_TREETHING_B,
		3436,           // doomednum
		S_GPZ_TREETHING_B, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_GPZ_TREETHING_M,
		3437,           // doomednum
		S_GPZ_TREETHING_M, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_GPZ_TREETHING_S,
		3438,           // doomednum
		S_GPZ_TREETHING_M, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_GGZFREEZETHRUSTER
		691,         // doomednum
		S_GGZFREEZETHRUSTER, // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		32*FRACUNIT, // radius
		48*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL       // raisestate
	},
	{           // MT_GGZICEDUST
		-1,          // doomednum
		S_GGZICEDUST1, // spawnstate
		2,           // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		32*FRACUNIT, // radius
		16*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_SPECIAL|MF_NOSQUISH|MF_NOGRAVITY, // flags
		S_NULL       // raisestate
	},
	{           // MT_GGZICECUBE
		-1,          // doomednum
		S_GGZICECUBE, // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		16*FRACUNIT, // radius
		16*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOSQUISH, // flags
		S_NULL       // raisestate
	},
	{           // MT_GGZICESHATTER
		-1,          // doomednum
		S_INVISIBLE, // spawnstate
		1000,        // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		32*FRACUNIT, // radius
		64*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOSQUISH, // flags
		S_NULL       // raisestate
	},
	{           // MT_SIDEWAYSFREEZETHRUSTER
		693,         // doomednum
		S_INVISIBLE, // spawnstate
		1,           // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		16*FRACUNIT, // radius
		32*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_NOGRAVITY, // flags
		S_NULL       // raisestate
	},
	{           // MT_THRUSTERPART
		-1,          // doomednum
		S_THRUSTERPART, // spawnstate
		1,           // spawnhealth
		S_NULL,      // seestate
		sfx_None,    // seesound
		0,           // reactiontime
		sfx_None,    // attacksound
		S_NULL,      // painstate
		0,           // painchance
		sfx_None,    // painsound
		S_NULL,      // meleestate
		S_NULL,      // missilestate
		S_NULL,      // deathstate
		S_NULL,      // xdeathstate
		sfx_None,    // deathsound
		0,           // speed
		16*FRACUNIT, // radius
		32*FRACUNIT, // height
		0,           // dispoffset
		0,           // mass
		0,           // damage
		sfx_None,    // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOCLIPTHING, // flags
		S_NULL       // raisestate
	},

	{           // MT_IVOBALL
		3792,         // doomednum
		S_IVOBALL,    // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		40*FRACUNIT,  // radius
		128*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOCLIP|MF_SCENERY|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_PATROLIVOBALL
		3808,         // doomednum
		S_IVOBALL,    // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		28*FRACUNIT,  // speed
		40*FRACUNIT,  // radius
		128*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_ENEMY|MF_NOBLOCKMAP, // flags
		S_NULL        // raisestate
	},
	{           // MT_AIRIVOBALL
		3811,         // doomednum
		S_IVOBALL,    // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		28*FRACUNIT,  // speed
		50*FRACUNIT,  // radius
		100*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_BOX_SIDE
		-1,           // doomednum
		S_INVISIBLE,  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		40*FRACUNIT,  // radius
		80*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOCLIPTHING, // flags
		S_NULL        // raisestate
	},
	{           // MT_BOX_DEBRIS
		-1,           // doomednum
		S_INVISIBLE,  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		0*FRACUNIT,   // radius
		0*FRACUNIT,   // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOCLIPTHING, // flags
		S_NULL        // raisestate
	},
	{           // MT_SA2_CRATE
		2529,         // doomednum
		S_INVISIBLE,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		40*FRACUNIT,  // radius
		80*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SPECIAL|MF_SOLID|MF_SHOOTABLE|MF_SCENERY|MF_DONTPUNT, // flags
		S_NULL        // raisestate
	},
	{           // MT_ICECAPBLOCK
		3750,         // doomednum
		S_INVISIBLE,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		40*FRACUNIT,  // radius
		80*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SPECIAL|MF_SOLID|MF_SHOOTABLE|MF_SCENERY|MF_DONTPUNT, // flags
		S_NULL        // raisestate
	},

	{           // MT_SPEAR
		3450,         // doomednum
		S_SPEAR_ROD,  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		64*FRACUNIT,  // radius
		80*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_PAIN|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_PAPERCOLLISION|MF_SCENERY|MF_NOHITLAGFORME, // flags
		S_NULL        // raisestate
	},
	{           // MT_SPEARVISUAL
		-1,           // doomednum
		S_UNKNOWN,    // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		1*FRACUNIT,   // radius
		1*FRACUNIT,   // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOCLIPHEIGHT|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_NOBLOCKMAP, // flags
		S_NULL        // raisestate
	},
	{           // MT_BSZLAMP_S
		3452,         // doomednum
		S_BLMS,       // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		32*FRACUNIT,  // radius
		32*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		0,            // flags
		S_NULL        // raisestate
	},
	{           // MT_BSZLAMP_M
		3453,         // doomednum
		S_BLMM,       // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		32*FRACUNIT,  // radius
		32*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		0,            // flags
		S_NULL        // raisestate
	},
	{           // MT_BSZLAMP_L
		3454,         // doomednum
		S_BLML,       // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		32*FRACUNIT,  // radius
		32*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		0,            // flags
		S_NULL        // raisestate
	},
	{           // MT_BSZSLAMP
		3469,         // doomednum
		S_BSWL,       // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		96*FRACUNIT,  // radius
		128*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		0,            // flags
		S_NULL        // raisestate
	},
	{           // MT_BSZSLCHA
		3470,         // doomednum
		S_BSWC,       // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		128*FRACUNIT, // radius
		128*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		0,            // flags
		S_NULL        // raisestate
	},
	{           // MT_BETA_EMITTER
		2699,         // doomednum
		S_INVISIBLE,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		0,            // radius
		0,            // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOSECTOR|MF_SCENERY, // flags
		S_NULL        // raisestate
	},
	{           // MT_BETA_PARTICLE_PHYSICAL
		-1,           // doomednum
		S_INVISIBLE,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		24*FRACUNIT,  // radius
		128*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_BETA_PARTICLE_VISUAL
		-1,           // doomednum
		S_NULL,       // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		0,            // radius
		0,            // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_BETA_PARTICLE_EXPLOSION
		-1,             // doomednum
		S_BETA_PARTICLE_EXPLOSION, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY|MF_DONTENCOREMAP|MF_NOHITLAGFORME|MF_SPECIAL|MF_DONTPUNT, // flags
		S_NULL          // raisestate
	},
	{           // MT_AIZ_REDFERN
		2910,         // doomednum
		S_AIZFL1,     // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		12*FRACUNIT,  // radius
		64*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY,   // flags
		S_NULL        // raisestate
	},
	{           // MT_AIZ_FERN1
		2911,         // doomednum
		S_AIZFR1,     // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		16*FRACUNIT,  // radius
		72*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY,   // flags
		S_NULL        // raisestate
	},
	{           // MT_AIZ_FERN2
		2912,         // doomednum
		S_AIZFR2,     // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		16*FRACUNIT,  // radius
		64*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY,   // flags
		S_NULL        // raisestate
	},
	{           // MT_AIZ_TREE
		2913,         // doomednum
		S_AIZTRE,     // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		9*FRACUNIT,   // radius
		115*FRACUNIT, // height
		0,            // dispoffset
		100,          // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_SOLID, // flags
		S_NULL        // raisestate
	},
	{           // MT_AIZ_FERN3
		2914,         // doomednum
		S_AIZFR3,     // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		16*FRACUNIT,  // radius
		24*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY,   // flags
		S_NULL        // raisestate
	},
	{           // MT_AIZ_DDB
		2915,         // doomednum
		S_AIZDB1,     // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		8*FRACUNIT,   // radius
		20*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY,   // flags
		S_NULL        // raisestate
	},
	{           // MT_AZROCKS
		470,          // doomednum
		S_AZROCKS,    // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_s3k59,    // deathsound
		0,            // speed
		48*FRACUNIT,  // radius
		96*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_AZROCKS_PARTICLE
		-1,           // doomednum
		S_AZROCKS_PARTICLE1, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		16*FRACUNIT,  // radius
		32*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL        // raisestate
	},
	{           // MT_EMROCKS
		467,          // doomednum
		S_EMROCKS,    // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_s3k59,    // deathsound
		0,            // speed
		48*FRACUNIT,  // radius
		96*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_EMROCKS_PARTICLE
		-1,           // doomednum
		S_EMROCKS_PARTICLE1, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		16*FRACUNIT,  // radius
		32*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL        // raisestate
	},
	{           // MT_EMFAUCET
		468,          // doomednum
		S_EMFAUCET,   // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		24*FRACUNIT,  // radius
		24*FRACUNIT,  // height
		1,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_NULL        // raisestate
	},
	{           // MT_EMFAUCET_DRIP
		-1,           // doomednum
		S_EMROCKS_DRIP, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		4*FRACUNIT,   // radius
		8*FRACUNIT,   // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL        // raisestate
	},
	{           // MT_EMFAUCET_PARTICLE
		-1,           // doomednum
		S_EMFAUCET_PARTICLE, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		4*FRACUNIT,   // radius
		8*FRACUNIT,   // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOCLIPHEIGHT|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL        // raisestate
	},
	{           // MT_EMRAINGEN
		469,          // doomednum
		S_INVISIBLE,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		0,            // radius
		0,            // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_NULL        // raisestate
	},
	{           // MT_TRICKBALLOON_RED
		2764,                 // doomednum
		S_TRICKBALLOON_RED1,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_s3k77,    // deathsound
		0,            // speed
		96*FRACUNIT,  // radius
		128*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_TRICKBALLOON_RED_POINT
		-1,           // doomednum
		S_TRICKBALLOON_RED_POINT1,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		32*FRACUNIT,  // radius
		64*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_TRICKBALLOON_YELLOW
		2765,                 // doomednum
		S_TRICKBALLOON_YELLOW1,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_s3k77,    // deathsound
		0,            // speed
		96*FRACUNIT,  // radius
		128*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_TRICKBALLOON_YELLOW_POINT
		-1,           // doomednum
		S_TRICKBALLOON_YELLOW_POINT1,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		32*FRACUNIT,  // radius
		64*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_WATERFALLPARTICLESPAWNER
		3422,           // doomednum
		S_INVISIBLE, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		1*FRACUNIT,   // radius
		1*FRACUNIT,   // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SSCANDLE
		3492,         // doomednum
		S_SSCANDLE_INIT,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		16*FRACUNIT,  // radius
		256*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_RUNSPAWNFUNC|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SSCANDLE_SIDE
		-1,           // doomednum
		S_SSCANDLE_SIDE,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		16*FRACUNIT,  // radius
		256*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_NOGRAVITY|MF_NOTHINK|MF_DRAWFROMFARAWAY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SSCANDLE_FLAME
		-1,           // doomednum
		S_SSCANDLE_FLAME,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		32*FRACUNIT,  // radius
		32*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_NOGRAVITY|MF_DRAWFROMFARAWAY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SS_HOLOGRAM
		3475,           // doomednum
		S_HOLOGRAM_CRAB,  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		3,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		128*FRACUNIT, // radius
		256*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SS_HOLOGRAM_ROTATOR
		3476,         // doomednum
		S_INVISIBLE,  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		0,            // radius
		0,            // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SS_COIN
		-1,           // doomednum
		S_SS_COIN,  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		18*FRACUNIT,  // radius
		37*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SS_COIN_CLOUD
		3474,         // doomednum
		S_INVISIBLE,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		0,            // radius
		0,            // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SS_GOBLET
		-1,           // doomednum
		S_SS_GOBLET,  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		20*FRACUNIT,  // radius
		64*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SS_GOBLET_CLOUD
		3478,         // doomednum
		S_INVISIBLE,  // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		0,            // radius
		0,            // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SS_LAMP
		3477,         // doomednum
		S_SS_LAMP,    // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		12*FRACUNIT,  // radius
		369*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DRAWFROMFARAWAY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SS_LAMP_BULB
		-1,           // doomednum
		S_SS_LAMP_BULB, // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		16*FRACUNIT,  // radius
		85*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DRAWFROMFARAWAY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SSWINDOW
		3493,         // doomednum
		S_SSWINDOW_INIT, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		192*FRACUNIT, // radius
		512*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_RUNSPAWNFUNC|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL        // raisestate
	},
	{           // MT_SSWINDOW_SHINE
		-1,           // doomednum
		S_SSWINDOW_SHINE, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		192*FRACUNIT, // radius
		512*FRACUNIT, // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT, // flags
		S_NULL        // raisestate
	},
	{           // MT_SSCHAINSOUND
		3494,         // doomednum
		S_SSCHAINSOUND, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		1*FRACUNIT,   // radius
		1*FRACUNIT,   // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL        // raisestate
	},
	{           // MT_SLSTMACE
		-1,           // doomednum
		S_SLSTMACE,   // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		40*FRACUNIT,  // radius
		96*FRACUNIT,  // height
		0,            // dispoffset
		DMG_WIPEOUT,  // mass
		0,            // damage
		sfx_None,     // activesound
		MF_PAIN|MF_NOHITLAGFORME, // flags
		S_NULL        // raisestate
	},
	{           // MT_SEALEDSTAR_BUMPER
		2461,         // doomednum
		S_SEALEDSTAR_BUMPER, // spawnstate
		1,            // spawnhealth
		S_SEALEDSTAR_BUMPERHIT,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_ssbmpr,     // deathsound
		0,            // speed
		24*FRACUNIT,   // radius
		32*FRACUNIT,   // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY|MF_SOLID, // flags
		S_NULL        // raisestate
	},
	{           // MT_SSCHAIN_SPAWNER
		3479,         // doomednum
		S_INVISIBLE, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		1*FRACUNIT,   // radius
		1*FRACUNIT,   // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_SSCHAIN
		-1,         // doomednum
		S_SSCHAIN1, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		1*FRACUNIT,   // radius
		1*FRACUNIT,   // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOSQUISH, // flags
		S_NULL        // raisestate
	},
	{           // MT_GACHATARGET
		3739,         // doomednum
		S_GACHATARGET, // spawnstate
		1,            // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_GACHATARGETSPIN, // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		32*FRACUNIT,// radius
		64*FRACUNIT,// height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_ENEMY|MF_SHOOTABLE|MF_NOHITLAGFORME, // flags
		S_NULL        // raisestate
	},
	{           // MT_CABOTRON
		3738,         // doomednum
		S_CABOTRON, // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		MT_CABOTRONSTAR, // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		2,            // speed
		32*FRACUNIT,   // radius
		64*FRACUNIT,   // height
		0,            // dispoffset
		8*FRACUNIT,   // mass
		5,            // damage
		sfx_None,     // activesound
		MF_SPECIAL|MF_ENEMY|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_CABOTRONSTAR
		-1,         // doomednum
		S_CABOTRONSTAR, // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		32*FRACUNIT,  // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		4*FRACUNIT,   // speed
		26*FRACUNIT,  // radius
		52*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		8,            // damage
		sfx_None,     // activesound
		MF_PAIN|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT, // flags
		S_NULL        // raisestate
	},
	{           // MT_STARSTREAM
		-1,         // doomednum
		S_STARSTREAM,  // spawnstate
		10000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		4*FRACUNIT,  // radius
		4*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		8,            // damage
		sfx_None,     // activesound
		MF_SCENERY|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL        // raisestate
	},
	{           // MT_IPULLUP
		3444,         // doomednum
		S_INVISIBLE,  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		32*FRACUNIT,  // radius
		32*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL        // raisestate
	},
	{           // MT_PULLUPHOOK
		3444,         // doomednum
		S_INVISIBLE,  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		0,            // speed
		64*FRACUNIT,  // radius
		64*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOCLIPHEIGHT|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL        // raisestate
	},
	{           // MT_AMPS
		-1,         // doomednum
		S_AMPS,  	  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_None,     // deathsound
		1,            // speed
		32*FRACUNIT,  // radius
		32*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL        // raisestate
	},
	{           // MT_EXP
		-1,         // doomednum
		S_EXP,  	  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_exp,     // deathsound
		1,            // speed
		32*FRACUNIT,  // radius
		32*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL        // raisestate
	},
	{           // MT_FLYBOT767
		-1,           // doomednum
		S_FLYBOT767,  // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_pop,      // deathsound
		4*FRACUNIT,   // speed
		32*FRACUNIT,  // radius
		15*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP|MF_NOCLIPTHING|MF_DONTENCOREMAP, // flags
		S_NULL        // raisestate
	},
	{           // MT_STONESHOE
		-1,           // doomednum
		S_STON,       // spawnstate
		1000,         // spawnhealth
		S_NULL,       // seestate
		sfx_None,     // seesound
		0,            // reactiontime
		sfx_None,     // attacksound
		S_NULL,       // painstate
		0,            // painchance
		sfx_None,     // painsound
		S_NULL,       // meleestate
		S_NULL,       // missilestate
		S_NULL,       // deathstate
		S_NULL,       // xdeathstate
		sfx_pop,      // deathsound
		4*FRACUNIT,   // speed
		64*FRACUNIT,  // radius
		64*FRACUNIT,  // height
		0,            // dispoffset
		0,            // mass
		0,            // damage
		sfx_None,     // activesound
		MF_SOLID|MF_NOSQUISH|MF_NOHITLAGFORME|MF_DONTENCOREMAP, // flags
		S_NULL        // raisestate
	},
	{           // MT_STONESHOE_CHAIN
		-1,             // doomednum
		S_SHRINK_CHAIN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SCENERY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_PICKUPFROMBELOW|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},
	{           // MT_TOXOMISTER_POLE
		-1,             // doomednum
		S_TOXAA,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_tossed,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_TOXAA_DEAD,   // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SHOOTABLE|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},
	{           // MT_TOXOMISTER_EYE
		-1,             // doomednum
		S_TOXAB,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_DONTENCOREMAP, // flags
		S_NULL          // raisestate
	},
	{           // MT_TOXOMISTER_CLOUD
		-1,             // doomednum
		S_TOXBA,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		35*FRACUNIT,    // radius
		70*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_DONTENCOREMAP|MF_ELEMENTAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_ANCIENTGEAR
		323,            // doomednum
		S_ANCIENTGEAR,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_gotgea,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_ANCIENTGEAR,  // deathstate
		S_NULL,         // xdeathstate
		sfx_gshc4,      // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		128*FRACUNIT,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_DONTENCOREMAP|MF_NOSQUISH|MF_DRAWFROMFARAWAY, // flags
		S_NULL          // raisestate
	},
	{           // MT_ANCIENTGEAR_PART
		-1,             // doomednum
		S_ANCIENTGEAR_PART, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPTHING|MF_NOCLIPHEIGHT|MF_DONTENCOREMAP|MF_NOSQUISH|MF_DRAWFROMFARAWAY, // flags
		S_NULL          // raisestate
	},
	{           // MT_MHPOLE
		3850,           // doomednum
		S_MHPOLE,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3k4a,      // seesound
		15,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8*FRACUNIT,     // speed
		20*FRACUNIT,    // radius
		128*FRACUNIT,   // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_s3kb6,      // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},
};


skincolor_t skincolors[MAXSKINCOLORS] = {
	{"Default",        {  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, SKINCOLOR_NONE,             0, 0,             false, UINT16_MAX}, // SKINCOLOR_NONE

	{"White",          {  0,   0,   0,   0,   1,   2,   5,   8,   9,  11,  14,  17,  20,  22,  25,  28}, SKINCOLOR_BLACK,            8, 0,              true, UINT16_MAX}, // SKINCOLOR_WHITE
	{"Silver",         {  0,   1,   2,   3,   5,   7,   9,  12,  13,  15,  18,  20,  23,  25,  27,  30}, SKINCOLOR_NICKEL,           8, 0,              true, UINT16_MAX}, // SKINCOLOR_SILVER
	{"Grey",           {  1,   3,   5,   7,   9,  11,  13,  15,  17,  19,  21,  23,  25,  27,  29,  31}, SKINCOLOR_GREY,             8, V_LAVENDERMAP,  true, UINT16_MAX}, // SKINCOLOR_GREY
	{"Nickel",         {  3,   5,   8,  11,  15,  17,  19,  21,  23,  24,  25,  26,  27,  29,  30,  31}, SKINCOLOR_SILVER,           8, V_GRAYMAP,      true, UINT16_MAX}, // SKINCOLOR_NICKEL
	{"Black",          {  4,   7,  11,  15,  20,  22,  24,  27,  28,  28,  28,  29,  29,  30,  30,  31}, SKINCOLOR_WHITE,            8, V_GRAYMAP,      true, UINT16_MAX}, // SKINCOLOR_BLACK
	{"Skunk",          {  0,   1,   2,   3,   4,  10,  16,  21,  23,  24,  25,  26,  27,  28,  29,  31}, SKINCOLOR_VOMIT,            8, V_GRAYMAP,      true, UINT16_MAX}, // SKINCOLOR_SKUNK
	{"Fairy",          {  0,   0, 252, 252, 200, 201, 211,  14,  16,  18,  20,  22,  24,  26,  28,  31}, SKINCOLOR_ARTICHOKE,       12, V_PINKMAP,      true, UINT16_MAX}, // SKINCOLOR_FAIRY
	{"Popcorn",        {  0,  80,  80,  81,  82, 218, 240,  11,  13,  16,  18,  21,  23,  26,  28,  31}, SKINCOLOR_PIGEON,          12, V_TANMAP,       true, UINT16_MAX}, // SKINCOLOR_POPCORN
	{"Artichoke",      { 80,  88,  89,  98,  99,  91,  12,  14,  16,  18,  20,  22,  24,  26,  28,  31}, SKINCOLOR_FAIRY,           12, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_ARTICHOKE
	{"Pigeon",         {  0, 128, 129, 130, 146, 170,  14,  15,  17,  19,  21,  23,  25,  27,  29,  31}, SKINCOLOR_POPCORN,         12, V_SKYMAP,       true, UINT16_MAX}, // SKINCOLOR_PIGEON
	{"Sepia",          {  0,   1,   3,   5,   7,   9, 241, 242, 243, 245, 247, 249, 236, 237, 238, 239}, SKINCOLOR_LEATHER,          6, V_TANMAP,       true, UINT16_MAX}, // SKINCOLOR_SEPIA
	{"Beige",          {  0, 208, 216, 217, 240, 241, 242, 243, 245, 247, 249, 250, 251, 237, 238, 239}, SKINCOLOR_BROWN,            2, V_BROWNMAP,     true, UINT16_MAX}, // SKINCOLOR_BEIGE
	{"Caramel",        {208,  48, 216, 217, 218, 220, 221, 223, 224, 226, 228, 230, 232, 234, 236, 239}, SKINCOLOR_CERULEAN,         8, V_TANMAP,       true, UINT16_MAX}, // SKINCOLOR_CARAMEL
	{"Peach",          {  0, 208,  48, 216, 218, 221, 212, 213, 214, 215, 206, 207, 197, 198, 199, 254}, SKINCOLOR_CYAN,             8, V_TANMAP,       true, UINT16_MAX}, // SKINCOLOR_PEACH
	{"Brown",          {216, 217, 219, 221, 224, 225, 227, 229, 230, 232, 234, 235, 237, 239,  29,  30}, SKINCOLOR_BEIGE,            8, V_BROWNMAP,     true, UINT16_MAX}, // SKINCOLOR_BROWN
	{"Leather",        {218, 221, 224, 227, 229, 231, 233, 235, 237, 239,  28,  28,  29,  29,  30,  31}, SKINCOLOR_SEPIA,            8, V_BROWNMAP,     true, UINT16_MAX}, // SKINCOLOR_LEATHER
	{"Pink",           {  0, 208, 208, 209, 209, 210, 211, 211, 212, 213, 214, 215,  41,  43,  45,  46}, SKINCOLOR_PISTACHIO,        8, V_PINKMAP,      true, UINT16_MAX}, // SKINCOLOR_PINK
	{"Rose",           {209, 210, 211, 211, 212, 213, 214, 215,  41,  42,  43,  44,  45,  71,  46,  47}, SKINCOLOR_MOSS,             8, V_PINKMAP,      true, UINT16_MAX}, // SKINCOLOR_ROSE
	{"Cinnamon",       {216, 221, 224, 226, 228,  60,  61,  43,  44,  45,  71,  46,  47,  29,  30,  31}, SKINCOLOR_WRISTWATCH,       6, V_REDMAP,       true, UINT16_MAX}, // SKINCOLOR_CINNAMON
	{"Ruby",           {  0, 208, 209, 210, 211, 213,  39,  40,  41,  43, 186, 186, 169, 169, 253, 254}, SKINCOLOR_SAPPHIRE,         8, V_REDMAP,       true, UINT16_MAX}, // SKINCOLOR_RUBY
	{"Raspberry",      {  0, 208, 209, 210,  32,  33,  34,  35,  37,  39,  41,  43,  44,  45,  46,  47}, SKINCOLOR_MINT,             8, V_REDMAP,       true, UINT16_MAX}, // SKINCOLOR_RASPBERRY
	{"Red",            {209, 210,  32,  34,  36,  38,  39,  40,  41,  42,  43,  44 , 45,  71,  46,  47}, SKINCOLOR_GREEN,            6, V_REDMAP,       true, UINT16_MAX}, // SKINCOLOR_RED
	{"Crimson",        {210,  33,  35,  38,  40,  42,  43,  45,  71,  71,  46,  46,  47,  47,  30,  31}, SKINCOLOR_PINETREE,         6, V_REDMAP,       true, UINT16_MAX}, // SKINCOLOR_CRIMSON
	{"Maroon",         { 32,  33,  35,  37,  39,  41,  43, 237,  26,  26,  27,  27,  28,  29,  30,  31}, SKINCOLOR_TOXIC,            8, V_REDMAP,       true, UINT16_MAX}, // SKINCOLOR_MAROON
	{"Lemonade",       {  0,  80,  81,  82,  83, 216, 210, 211, 212, 213, 214, 215,  43,  44,  71,  47}, SKINCOLOR_THUNDER,          8, V_PINKMAP,      true, UINT16_MAX}, // SKINCOLOR_LEMONADE
	{"Scarlet",        { 48,  49,  50,  51,  53,  34,  36,  38, 184, 185, 168, 168, 169, 169, 254,  31}, SKINCOLOR_ALGAE,           10, V_REDMAP,       true, UINT16_MAX}, // SKINCOLOR_SCARLET
	{"Ketchup",        { 72,  73,  64,  51,  52,  54,  34,  36,  38,  40,  42,  43,  44,  71,  46,  47}, SKINCOLOR_MUSTARD,         10, V_REDMAP,       true, UINT16_MAX}, // SKINCOLOR_KETCHUP
	{"Dawn",           {  0, 208, 216, 209, 210, 211, 212,  57,  58,  59,  60,  61,  63,  71,  47,  31}, SKINCOLOR_DUSK,             8, V_ORANGEMAP,    true, UINT16_MAX}, // SKINCOLOR_DAWN
	{"Sunslam",        { 82,  72,  73,  64,  51,  53,  55, 213, 214, 195, 195, 173, 174, 175, 253, 254}, SKINCOLOR_MOONSET,          8, V_ORANGEMAP,    true, UINT16_MAX}, // SKINCOLOR_SUNSLAM
	{"Creamsicle",     {  0,   0, 208, 208,  48,  49,  50,  52,  53,  54,  56,  57,  58,  60,  61,  63}, SKINCOLOR_PERIWINKLE,       8, V_ORANGEMAP,    true, UINT16_MAX}, // SKINCOLOR_CREAMSICLE
	{"Orange",         {208,  48,  49,  50,  51,  52,  53,  54,  55,  57,  59,  60,  62,  44,  71,  47}, SKINCOLOR_BLUE,             8, V_ORANGEMAP,    true, UINT16_MAX}, // SKINCOLOR_ORANGE
	{"Rosewood",       { 50,  52,  55,  56,  58,  59,  60,  61,  62,  63,  44,  45,  71,  46,  47,  30}, SKINCOLOR_MIDNIGHT,         6, V_ORANGEMAP,    true, UINT16_MAX}, // SKINCOLOR_ROSEWOOD
	{"Tangerine",      { 80,  81,  82,  83,  64,  51,  52,  54,  55,  57,  58,  60,  61,  63,  71,  47}, SKINCOLOR_LIME,             8, V_ORANGEMAP,    true, UINT16_MAX}, // SKINCOLOR_TANGERINE
	{"Tan",            {  0,  80,  81,  82,  83,  84,  85,  86,  87, 245, 246, 248, 249, 251, 237, 239}, SKINCOLOR_RUST,             8, V_TANMAP,       true, UINT16_MAX}, // SKINCOLOR_TAN
	{"Cream",          {  0,  80,  80,  81,  81,  49,  51, 222, 224, 227, 230, 233, 236, 239,  29,  31}, SKINCOLOR_COPPER,          10, V_TANMAP,       true, UINT16_MAX}, // SKINCOLOR_CREAM
	{"Gold",           {  0,  80,  81,  83,  64,  65,  66,  67,  68, 215,  69,  70,  44,  71,  46,  47}, SKINCOLOR_SLATE,            8, V_GOLDMAP,      true, UINT16_MAX}, // SKINCOLOR_GOLD
	{"Royal",          { 80,  81,  83,  64,  65, 223, 229, 196, 196, 197, 197, 198, 199,  29,  30,  31}, SKINCOLOR_PLATINUM,         6, V_GOLDMAP,      true, UINT16_MAX}, // SKINCOLOR_ROYAL
	{"Bronze",         { 83,  64,  65,  66,  67, 215,  69,  70,  44,  44,  45,  71,  46,  47,  29,  31}, SKINCOLOR_STEEL,            8, V_GOLDMAP,      true, UINT16_MAX}, // SKINCOLOR_BRONZE
	{"Copper",         {  0,  82,  64,  65,  67,  68,  70, 237, 239,  28,  28,  29,  29,  30,  30,  31}, SKINCOLOR_CREAM,            6, V_GOLDMAP,      true, UINT16_MAX}, // SKINCOLOR_COPPER
	{"Yellow",         {  0,  80,  81,  82,  83,  73,  84,  74,  64,  65,  66,  67,  68,  69,  70,  71}, SKINCOLOR_AQUAMARINE,       8, V_YELLOWMAP,    true, UINT16_MAX}, // SKINCOLOR_YELLOW
	{"Mustard",        { 80,  81,  82,  83,  64,  65,  65,  76,  76,  77,  77,  78,  79, 237, 239,  29}, SKINCOLOR_KETCHUP,          8, V_YELLOWMAP,    true, UINT16_MAX}, // SKINCOLOR_MUSTARD
	{"Banana",         { 80,  81,  83,  72,  73,  74,  75,  76,  77,  78,  79, 236, 237, 238, 239,  30}, SKINCOLOR_EMERALD,          8, V_YELLOWMAP,    true, UINT16_MAX}, // SKINCOLOR_BANANA
	{"Olive",          { 80,  82,  73,  74,  75,  76,  77,  78,  79, 236, 237, 238, 239,  28,  29,  31}, SKINCOLOR_TEAL,             8, V_YELLOWMAP,    true, UINT16_MAX}, // SKINCOLOR_OLIVE
	{"Crocodile",      {  0,  80,  81,  88,  88, 188, 189,  76,  76,  77,  78,  79, 236, 237, 238, 239}, SKINCOLOR_VIOLET,           8, V_YELLOWMAP,    true, UINT16_MAX}, // SKINCOLOR_CROCODILE
	{"Peridot",        {  0,  80,  81,  88, 188, 189, 190, 191,  94,  94,  95,  95, 109, 110, 111,  31}, SKINCOLOR_NAVY,             6, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_PERIDOT
	{"Vomit",          {  0, 208, 216, 209, 218,  51,  65,  76, 191, 191, 126, 143, 138, 175, 169, 254}, SKINCOLOR_SKUNK,            8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_VOMIT
	{"Garden",         { 81,  82,  83,  73,  64,  65,  66,  92,  92,  93,  93,  94,  95, 109, 110, 111}, SKINCOLOR_LAVENDER,         6, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_GARDEN
	{"Lime",           {  0,  80,  81,  88, 188, 189, 114, 114, 115, 115, 116, 116, 117, 118, 119, 111}, SKINCOLOR_TANGERINE,        8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_LIME
	{"Handheld",       { 83,  72,  73,  74,  75,  76, 102, 104, 105, 106, 107, 108, 109, 110, 111,  31}, SKINCOLOR_ULTRAMARINE,      8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_HANDHELD
	{"Tea",            {  0,  80,  80,  81,  88,  89,  90,  91,  92,  93,  94,  95, 109, 110, 111,  31}, SKINCOLOR_BLOSSOM,          8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_TEA
	{"Pistachio",      {  0,  80,  88,  88,  89,  90,  91, 102, 103, 104, 105, 106, 107, 108, 109, 110}, SKINCOLOR_PINK,             6, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_PISTACHIO
	{"Moss",           { 88,  89,  90,  91,  91,  92,  93,  94, 107, 107, 108, 108, 109, 109, 110, 111}, SKINCOLOR_ROSE,             8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_MOSS
	{"Camouflage",     {208,  84,  85, 240, 241, 243, 245,  94, 107, 108, 108, 109, 109, 110, 110, 111}, SKINCOLOR_CAMOUFLAGE,       8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_CAMOUFLAGE
	{"Mint",           {  0,  88,  88,  89,  89, 100, 101, 102, 125, 126, 143, 143, 138, 175, 169, 254}, SKINCOLOR_RASPBERRY,        8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_MINT
	{"Green",          { 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111}, SKINCOLOR_RED,              8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_GREEN
	{"Pinetree",       { 97,  99, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,  30,  30,  31}, SKINCOLOR_CRIMSON,          8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_PINETREE
	{"Turtle",         { 96, 112, 112, 113, 113, 114, 114, 115, 115, 116, 116, 117, 117, 118, 119, 111}, SKINCOLOR_MAGENTA,          8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_TURTLE
	{"Swamp",          { 96, 112, 113, 114, 115, 116, 117, 118, 119, 119,  29,  29,  30,  30,  31,  31}, SKINCOLOR_BYZANTIUM,        8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_SWAMP
	{"Dream",          {  0,   0, 208, 208,  48,  89,  98, 100, 148, 148, 172, 172, 173, 173, 174, 175}, SKINCOLOR_POMEGRANATE,      8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_DREAM
	{"Plague",         { 80,  88,  96, 112, 113, 124, 142, 149, 149, 173, 174, 175, 169, 253, 254,  31}, SKINCOLOR_NOVA,             8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_PLAGUE
	{"Emerald",        {  0, 120, 121, 112, 113, 114, 115, 125, 125, 126, 126, 127, 138, 175, 253, 254}, SKINCOLOR_BANANA,           8, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_EMERALD
	{"Algae",          {128, 129, 130, 131, 132, 133, 134, 115, 115, 116, 116, 117, 118, 119, 110, 111}, SKINCOLOR_SCARLET,         10, V_GREENMAP,     true, UINT16_MAX}, // SKINCOLOR_ALGAE
	{"Aquamarine",     {  0, 128, 120, 121, 122, 123, 124, 125, 126, 126, 127, 127, 118, 118, 119, 111}, SKINCOLOR_YELLOW,           8, V_AQUAMAP,      true, UINT16_MAX}, // SKINCOLOR_AQUAMARINE
	{"Turquoise",      {128, 120, 121, 122, 123, 141, 141, 142, 142, 143, 143, 138, 138, 139, 139,  31}, SKINCOLOR_MAUVE,           10, V_AQUAMAP,      true, UINT16_MAX}, // SKINCOLOR_TURQUOISE
	{"Teal",           {  0, 120, 120, 121, 140, 141, 142, 143, 143, 138, 138, 139, 139, 254, 254,  31}, SKINCOLOR_OLIVE,            8, V_AQUAMAP,      true, UINT16_MAX}, // SKINCOLOR_TEAL
	{"Robin",          {  0,  80,  81,  82,  83,  88, 121, 140, 133, 133, 134, 135, 136, 137, 138, 139}, SKINCOLOR_THISTLE,          8, V_SKYMAP,       true, UINT16_MAX}, // SKINCOLOR_ROBIN
	{"Cyan",           {  0,   0, 128, 128, 255, 131, 132, 134, 142, 142, 143, 127, 118, 119, 110, 111}, SKINCOLOR_PEACH,            8, V_SKYMAP,       true, UINT16_MAX}, // SKINCOLOR_CYAN
	{"Jawz",           {  0,   0, 128, 128, 129, 146, 133, 134, 135, 149, 149, 173, 173, 174, 175,  31}, SKINCOLOR_LILAC,           10, V_SKYMAP,       true, UINT16_MAX}, // SKINCOLOR_JAWZ
	{"Cerulean",       {  0, 128, 129, 130, 131, 132, 133, 135, 136, 136, 137, 137, 138, 138, 139,  31}, SKINCOLOR_CARAMEL,          8, V_SKYMAP,       true, UINT16_MAX}, // SKINCOLOR_CERULEAN
	{"Navy",           {128, 129, 130, 132, 134, 135, 136, 137, 137, 138, 138, 139, 139,  29,  30,  31}, SKINCOLOR_PERIDOT,          8, V_SKYMAP,       true, UINT16_MAX}, // SKINCOLOR_NAVY
	{"Platinum",       {  0,   0,   0, 144, 144, 145,   9,  11,  14, 142, 136, 137, 138, 138, 139,  31}, SKINCOLOR_ROYAL,            8, V_GRAYMAP,      true, UINT16_MAX}, // SKINCOLOR_PLATINUM
	{"Slate",          {  0,   0, 144, 144, 144, 145, 145, 145, 170, 170, 171, 171, 172, 173, 174, 175}, SKINCOLOR_GOLD,            10, 0,              true, UINT16_MAX}, // SKINCOLOR_SLATE
	{"Steel",          {  0, 144, 144, 145, 145, 170, 170, 171, 171, 172, 172, 173, 173, 174, 175,  31}, SKINCOLOR_BRONZE,          10, V_GRAYMAP,      true, UINT16_MAX}, // SKINCOLOR_STEEL
	{"Thunder",        { 80,  81,  82,  83,  64,  65,  11, 171, 172, 173, 173, 157, 158, 159, 254,  31}, SKINCOLOR_LEMONADE,         8, V_GOLDMAP,      true, UINT16_MAX}, // SKINCOLOR_THUNDER
	{"Nova",           {  0,  83,  49,  50,  51,  32, 192, 148, 148, 172, 173, 174, 175,  29,  30,  31}, SKINCOLOR_PLAGUE,          10, V_BLUEMAP,      true, UINT16_MAX}, // SKINCOLOR_NOVA
	{"Rust",           {208,  48, 216, 217, 240, 241, 242, 171, 172, 173,  24,  25,  26,  28,  29,  31}, SKINCOLOR_TAN,              8, V_BROWNMAP,     true, UINT16_MAX}, // SKINCOLOR_RUST
	{"Wristwatch",     { 48, 218, 221, 224, 227, 231, 196, 173, 173, 174, 159, 159, 253, 253, 254,  31}, SKINCOLOR_CINNAMON,         8, V_BROWNMAP,     true, UINT16_MAX}, // SKINCOLOR_WRISTWATCH
	{"Jet",            {145, 146, 147, 148, 149, 173, 173, 174, 175, 175,  28,  28,  29,  29,  30,  31}, SKINCOLOR_TAFFY,            8, V_GRAYMAP,      true, UINT16_MAX}, // SKINCOLOR_JET
	{"Sapphire",       {  0, 128, 129, 131, 133, 135, 149, 150, 152, 154, 156, 158, 159, 253, 254,  31}, SKINCOLOR_RUBY,             6, V_SKYMAP,       true, UINT16_MAX}, // SKINCOLOR_SAPPHIRE
	{"Ultramarine",    {  0,   0, 120, 120, 121, 133, 135, 149, 149, 166, 166, 167, 168, 169, 254,  31}, SKINCOLOR_HANDHELD,        10, V_SKYMAP,       true, UINT16_MAX}, // SKINCOLOR_ULTRAMARINE
	{"Periwinkle",     {  0,   0, 144, 144, 145, 146, 147, 149, 150, 152, 154, 155, 157, 159, 253, 254}, SKINCOLOR_CREAMSICLE,       8, V_BLUEMAP,      true, UINT16_MAX}, // SKINCOLOR_PERIWINKLE
	{"Blue",           {144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 155, 156, 158, 253, 254,  31}, SKINCOLOR_ORANGE,           8, V_BLUEMAP,      true, UINT16_MAX}, // SKINCOLOR_BLUE
	{"Midnight",       {146, 148, 149, 150, 152, 153, 155, 157, 159, 253, 253, 254, 254,  31,  31,  31}, SKINCOLOR_ROSEWOOD,         8, V_BLUEMAP,      true, UINT16_MAX}, // SKINCOLOR_MIDNIGHT
	{"Blueberry",      {  0, 144, 145, 146, 147, 171, 172, 166, 166, 167, 167, 168, 168, 175, 169, 253}, SKINCOLOR_PURPLE,           8, V_BLUEMAP,      true, UINT16_MAX}, // SKINCOLOR_BLUEBERRY
	{"Thistle",        {  0,   0,   0, 252, 252, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 254}, SKINCOLOR_ROBIN,            8, V_PURPLEMAP,    true, UINT16_MAX}, // SKINCOLOR_THISTLE
	{"Purple",         {  0, 252, 160, 161, 162, 163, 164, 165, 166, 167, 168, 168, 169, 169, 253, 254}, SKINCOLOR_BLUEBERRY,       10, V_PURPLEMAP,    true, UINT16_MAX}, // SKINCOLOR_PURPLE
	{"Pastel",         {  0, 128, 128, 129, 129, 146, 170, 162, 163, 164, 165, 166, 167, 168, 169, 254}, SKINCOLOR_FUCHSIA,         11, V_PURPLEMAP,    true, UINT16_MAX}, // SKINCOLOR_PASTEL
	{"Moonset",        {  0, 144, 145, 146, 170, 162, 163, 184, 184, 207, 207,  44,  45,  46,  47,  31}, SKINCOLOR_SUNSLAM,         10, V_MAGENTAMAP,   true, UINT16_MAX}, // SKINCOLOR_MOONSET
	{"Dusk",           {252, 200, 201, 192, 193, 194, 172, 172, 173, 173, 174, 174, 175, 169, 253, 254}, SKINCOLOR_DAWN,             6, V_MAGENTAMAP,   true, UINT16_MAX}, // SKINCOLOR_DUSK
	{"Violet",         {176, 177, 178, 179, 180, 181, 182, 183, 184, 165, 165, 166, 167, 168, 169, 254}, SKINCOLOR_CROCODILE,        8, V_MAGENTAMAP,   true, UINT16_MAX}, // SKINCOLOR_VIOLET
	{"Magenta",        {252, 200, 177, 177, 178, 179, 180, 181, 182, 183, 183, 184, 185, 186, 187,  31}, SKINCOLOR_TURTLE,           8, V_MAGENTAMAP,   true, UINT16_MAX}, // SKINCOLOR_MAGENTA
	{"Fuchsia",        {208, 209, 209,  32,  33, 182, 183, 184, 185, 185, 186, 186, 187, 253, 254,  31}, SKINCOLOR_PASTEL,          11, V_MAGENTAMAP,   true, UINT16_MAX}, // SKINCOLOR_FUCHSIA
	{"Toxic",          {  0,   0,  88,  88,  89,   6,   8,  10, 193, 194, 195, 184, 185, 186, 187,  31}, SKINCOLOR_MAROON,           8, V_LAVENDERMAP,  true, UINT16_MAX}, // SKINCOLOR_TOXIC
	{"Mauve",          { 80,  81,  82,  83,  64,  50, 201, 192, 193, 194, 195, 173, 174, 175, 253, 254}, SKINCOLOR_TURQUOISE,        8, V_LAVENDERMAP,  true, UINT16_MAX}, // SKINCOLOR_MAUVE
	{"Lavender",       {252, 177, 179, 192, 193, 194, 195, 196, 196, 197, 197, 198, 198, 199,  30,  31}, SKINCOLOR_GARDEN,           6, V_LAVENDERMAP,  true, UINT16_MAX}, // SKINCOLOR_LAVENDER
	{"Byzantium",      {145, 192, 193, 194, 195, 196, 197, 198, 199, 199,  29,  29,  30,  30,  31,  31}, SKINCOLOR_SWAMP,            8, V_LAVENDERMAP,  true, UINT16_MAX}, // SKINCOLOR_BYZANTIUM
	{"Pomegranate",    {208, 209, 210, 211, 212, 213, 214, 195, 195, 196, 196, 197, 198, 199,  29,  30}, SKINCOLOR_DREAM,            8, V_LAVENDERMAP,  true, UINT16_MAX}, // SKINCOLOR_POMEGRANATE
	{"Lilac",          {  0,   0,   0, 252, 252, 176, 200, 201, 179, 192, 193, 194, 195, 196, 197, 198}, SKINCOLOR_JAWZ,             6, V_PINKMAP,      true, UINT16_MAX}, // SKINCOLOR_LILAC
	{"Blossom",        {  0, 252, 252, 176, 200, 177, 201, 202, 202,  34,  36,  38,  40,  42,  45,  46}, SKINCOLOR_TEA,              8, V_PINKMAP,      true, UINT16_MAX}, // SKINCOLOR_BLOSSOM
	{"Taffy",          {  0, 252, 252, 200, 200, 201, 202, 203, 204, 204, 205, 206, 207,  43,  45,  47}, SKINCOLOR_JET,              8, V_PINKMAP,      true, UINT16_MAX}, // SKINCOLOR_TAFFY

	// super (todo: replace these with the kart ones)
	{"Super Silver 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03}, SKINCOLOR_BLACK, 15, 0,         false, UINT16_MAX}, // SKINCOLOR_SUPERSILVER1
	{"Super Silver 2", {0x00, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07}, SKINCOLOR_BLACK, 6,  0,         false, UINT16_MAX}, // SKINCOLOR_SUPERSILVER2
	{"Super Silver 3", {0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07, 0x09, 0x0b}, SKINCOLOR_BLACK, 5,  0,         false, UINT16_MAX}, // SKINCOLOR_SUPERSILVER3
	{"Super Silver 4", {0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f, 0x11}, SKINCOLOR_BLACK, 5,  V_GRAYMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERSILVER4
	{"Super Silver 5", {0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f, 0x11, 0x13}, SKINCOLOR_BLACK, 5,  V_GRAYMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERSILVER5

	{"Super Red 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2}, SKINCOLOR_CYAN, 15, 0,         false, UINT16_MAX}, // SKINCOLOR_SUPERRED1
	{"Super Red 2", {0x00, 0x00, 0x00, 0xd0, 0xd0, 0xd0, 0xd1, 0xd1, 0xd1, 0xd2, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21}, SKINCOLOR_CYAN, 14, V_PINKMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERRED2
	{"Super Red 3", {0x00, 0x00, 0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23}, SKINCOLOR_CYAN, 13, V_REDMAP,  false, UINT16_MAX}, // SKINCOLOR_SUPERRED3
	{"Super Red 4", {0x00, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24}, SKINCOLOR_CYAN, 11, V_REDMAP,  false, UINT16_MAX}, // SKINCOLOR_SUPERRED4
	{"Super Red 5", {0xd0, 0xd1, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25}, SKINCOLOR_CYAN, 10, V_REDMAP,  false, UINT16_MAX}, // SKINCOLOR_SUPERRED5

	{"Super Orange 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x30, 0x31, 0x32, 0x33, 0x34}, SKINCOLOR_SAPPHIRE, 15, 0,           false, UINT16_MAX}, // SKINCOLOR_SUPERORANGE1
	{"Super Orange 2", {0x00, 0x00, 0x00, 0x00, 0xd0, 0xd0, 0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34}, SKINCOLOR_SAPPHIRE, 12, V_ORANGEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERORANGE2
	{"Super Orange 3", {0x00, 0x00, 0xd0, 0xd0, 0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x35}, SKINCOLOR_SAPPHIRE, 9,  V_ORANGEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERORANGE3
	{"Super Orange 4", {0x00, 0xd0, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x44, 0x45, 0x46}, SKINCOLOR_SAPPHIRE, 4,  V_ORANGEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERORANGE4
	{"Super Orange 5", {0xd0, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x44, 0x45, 0x46, 0x47}, SKINCOLOR_SAPPHIRE, 3,  V_ORANGEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERORANGE5

	{"Super Gold 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x50, 0x51, 0x52, 0x53, 0x48}, SKINCOLOR_PERIWINKLE, 15, 0,           false, UINT16_MAX}, // SKINCOLOR_SUPERGOLD1
	{"Super Gold 2", {0x00, 0x50, 0x51, 0x52, 0x53, 0x53, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41}, SKINCOLOR_PERIWINKLE, 9,  V_YELLOWMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERGOLD2
	{"Super Gold 3", {0x51, 0x52, 0x53, 0x53, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41, 0x42, 0x43}, SKINCOLOR_PERIWINKLE, 8,  V_YELLOWMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERGOLD3
	{"Super Gold 4", {0x53, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46}, SKINCOLOR_PERIWINKLE, 8,  V_YELLOWMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERGOLD4
	{"Super Gold 5", {0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47}, SKINCOLOR_PERIWINKLE, 8,  V_YELLOWMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERGOLD5

	{"Super Peridot 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x58, 0x58, 0xbc, 0xbc, 0xbc}, SKINCOLOR_BLUEBERRY, 15, 0,            false, UINT16_MAX}, // SKINCOLOR_SUPERPERIDOT1
	{"Super Peridot 2", {0x00, 0x58, 0x58, 0x58, 0xbc, 0xbc, 0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe}, SKINCOLOR_BLUEBERRY, 4,  V_GREENMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERPERIDOT2
	{"Super Peridot 3", {0x58, 0x58, 0xbc, 0xbc, 0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe, 0xbf, 0xbf}, SKINCOLOR_BLUEBERRY, 3,  V_GREENMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERPERIDOT3
	{"Super Peridot 4", {0x58, 0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe, 0xbf, 0xbf, 0x5e, 0x5e, 0x5f}, SKINCOLOR_BLUEBERRY, 3,  V_GREENMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERPERIDOT4
	{"Super Peridot 5", {0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe, 0xbf, 0xbf, 0x5e, 0x5e, 0x5f, 0x77}, SKINCOLOR_BLUEBERRY, 3,  V_GREENMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERPERIDOT5

	{"Super Sky 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x81, 0x82, 0x83, 0x84}, SKINCOLOR_RUST, 15, 0,        false, UINT16_MAX}, // SKINCOLOR_SUPERSKY1
	{"Super Sky 2", {0x00, 0x80, 0x81, 0x82, 0x83, 0x83, 0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86}, SKINCOLOR_RUST, 4,  V_SKYMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERSKY2
	{"Super Sky 3", {0x81, 0x82, 0x83, 0x83, 0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87}, SKINCOLOR_RUST, 3,  V_SKYMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERSKY3
	{"Super Sky 4", {0x83, 0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87, 0x88, 0x89, 0x8a}, SKINCOLOR_RUST, 3,  V_SKYMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERSKY4
	{"Super Sky 5", {0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87, 0x88, 0x89, 0x8a, 0x8b}, SKINCOLOR_RUST, 3,  V_SKYMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERSKY5

	{"Super Purple 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0xa0, 0xa0, 0xa1, 0xa2}, SKINCOLOR_EMERALD, 15, 0,           false, UINT16_MAX}, // SKINCOLOR_SUPERPURPLE1
	{"Super Purple 2", {0x00, 0x90, 0xa0, 0xa0, 0xa1, 0xa1, 0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5}, SKINCOLOR_EMERALD, 4,  V_PURPLEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERPURPLE2
	{"Super Purple 3", {0xa0, 0xa0, 0xa1, 0xa1, 0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa6, 0xa6}, SKINCOLOR_EMERALD, 0,  V_PURPLEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERPURPLE3
	{"Super Purple 4", {0xa1, 0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa6, 0xa6, 0xa7, 0xa8, 0xa9}, SKINCOLOR_EMERALD, 0,  V_PURPLEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERPURPLE4
	{"Super Purple 5", {0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa6, 0xa6, 0xa7, 0xa8, 0xa9, 0xfd}, SKINCOLOR_EMERALD, 0,  V_PURPLEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERPURPLE5

	{"Super Rust 1", {0x00, 0xd0, 0xd0, 0xd0, 0x30, 0x30, 0x31, 0x32, 0x33, 0x37, 0x3a, 0x44, 0x45, 0x46, 0x47, 0x2e}, SKINCOLOR_CYAN, 14, V_ORANGEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERRUST1
	{"Super Rust 2", {0x30, 0x31, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x38, 0x3a, 0x44, 0x45, 0x46, 0x47, 0x47, 0x2e}, SKINCOLOR_CYAN, 10, V_ORANGEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERRUST2
	{"Super Rust 3", {0x31, 0x32, 0x33, 0x34, 0x36, 0x37, 0x38, 0x3a, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x2e, 0x2e}, SKINCOLOR_CYAN, 9,  V_ORANGEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERRUST3
	{"Super Rust 4", {0x48, 0x40, 0x41, 0x42, 0x43, 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x47, 0x2e, 0x2e, 0x2e}, SKINCOLOR_CYAN, 8,  V_ORANGEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERRUST4
	{"Super Rust 5", {0x41, 0x42, 0x43, 0x43, 0x44, 0x44, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xed, 0xee, 0xee, 0xef, 0xef}, SKINCOLOR_CYAN, 8,  V_ORANGEMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERRUST5

	{"Super Tan 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x50, 0x51, 0x51, 0x52, 0x52}, SKINCOLOR_BROWN, 14, 0,          false, UINT16_MAX}, // SKINCOLOR_SUPERTAN1
	{"Super Tan 2", {0x00, 0x50, 0x50, 0x51, 0x51, 0x52, 0x52, 0x52, 0x54, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5}, SKINCOLOR_BROWN, 13, V_BROWNMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERTAN2
	{"Super Tan 3", {0x50, 0x51, 0x51, 0x52, 0x52, 0x52, 0x54, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5, 0xf7, 0xf9}, SKINCOLOR_BROWN, 12, V_BROWNMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERTAN3
	{"Super Tan 4", {0x51, 0x52, 0x52, 0x52, 0x52, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5, 0xf7, 0xf9, 0xfb, 0xed}, SKINCOLOR_BROWN, 11, V_BROWNMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERTAN4
	{"Super Tan 5", {0x52, 0x52, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5, 0xf7, 0xf9, 0xfb, 0xed, 0xee, 0xef, 0xef}, SKINCOLOR_BROWN, 10, V_BROWNMAP, false, UINT16_MAX}, // SKINCOLOR_SUPERTAN5

	{"Chaos Emerald 1", {  0,  88, 188,  98, 114, 116, 117, 119,   0,   0,   0,   0,   0,   0,   0,   0}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_CHAOSEMERALD1
	{"Chaos Emerald 2", {  0,  80,  82,  74,  65,  52,  56,  60,   0,   0,   0,   0,   0,   0,   0,   0}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_CHAOSEMERALD2
	{"Chaos Emerald 3", {  0, 252, 201, 179, 182, 183, 185, 187,   0,   0,   0,   0,   0,   0,   0,   0}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_CHAOSEMERALD3
	{"Chaos Emerald 4", {  0, 144, 146, 147, 149, 165, 167, 169,   0,   0,   0,   0,   0,   0,   0,   0}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_CHAOSEMERALD4
	{"Chaos Emerald 5", {  0,   1, 144,   4,   9, 170,  14,  21,   0,   0,   0,   0,   0,   0,   0,   0}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_CHAOSEMERALD5
	{"Chaos Emerald 6", {  0, 208,  50,  32,  34,  37,  40,  44,   0,   0,   0,   0,   0,   0,   0,   0}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_CHAOSEMERALD6
	{"Chaos Emerald 7", {  0, 120, 121, 140, 133, 135, 149, 156,   0,   0,   0,   0,   0,   0,   0,   0}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_CHAOSEMERALD7

	{"Invinc Flash", {  0,   0,   0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_INVINCFLASH

	{"Position", {  8,   9,  11,  12,  14,  15,  17,  18,  20,  21,  23,  24,  26,  27,  29,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM
	{"Position Win 1", {152, 152, 153, 153, 154, 154, 155, 155, 156, 156, 157, 158, 159, 253, 254,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_WIN1
	{"Position Win 2", {134, 134, 135, 135, 135, 136, 136, 136, 137, 137, 138, 138, 139, 139, 254,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_WIN2
	{"Position Win 3", {255, 255, 122, 122, 123, 123, 141, 141, 142, 142, 143, 143, 138, 139, 254,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_WIN3
	{"Position Lose 1", { 35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  71,  46,  47,  29,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_LOSE1
	{"Position Lose 2", { 52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  63,  44,  45,  46,  47,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_LOSE2
	{"Position Lose 3", { 73,  74,  75,  76,  76,  77,  77,  78,  78,  79,  79, 236, 237, 238, 239,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_LOSE3
	{"Position Best 1", { 35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  71,  46,  47,  29,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_BEST1
	{"Position Best 2", { 73,  74,  75,  76,  76,  77,  77,  78,  78,  79,  79, 236, 237, 238, 239,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_BEST2
	{"Position Best 3", {112, 112, 113, 114, 115, 115, 116, 116, 117, 117, 118, 118, 119, 110, 111,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_BEST3
	{"Position Best 4", {255, 255, 122, 122, 123, 123, 141, 141, 142, 142, 143, 143, 138, 139, 254,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_BEST4
	{"Position Best 5", {152, 152, 153, 153, 154, 154, 155, 155, 156, 156, 157, 158, 159, 253, 254,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_BEST5
	{"Position Best 6", {181, 181, 182, 182, 183, 183, 184, 184, 185, 185, 186, 186, 187, 187,  29,  30}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_POSNUM_BEST6

	{"Intermission 1", {  0,  80,  80,  80,  81,  81,  81,  84,  84,  85,  86,  86,  87,  87, 246, 248}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_INTERMISSION1
	{"Intermission 2", {  0,  81,  81,  81,  88,  88,  88,  89,  89, 140, 140, 141, 141, 142, 142, 142}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_INTERMISSION2
	{"Intermission 3", {  0, 144, 144, 144, 129, 129, 129, 130, 130, 146, 147, 147,  14,  16,  17,  19}, SKINCOLOR_NONE, 0, 0, false, UINT16_MAX}, // SKINCOLOR_INTERMISSION3
};

/** Patches the mobjinfo, state, and skincolor tables.
  * Free slots are emptied out and set to initial values.
  */
void P_PatchInfoTables(void)
{
	INT32 i;
	char *tempname;

#if NUMSPRITEFREESLOTS > 9999 //tempname numbering actually starts at SPR_FIRSTFREESLOT, so the limit is actually 9999 + SPR_FIRSTFREESLOT-1, but the preprocessor doesn't understand enums, so its left at 9999 for safety
#error "Update P_PatchInfoTables, you big dumb head"
#endif

	// empty out free slots
	for (i = SPR_FIRSTFREESLOT; i <= SPR_LASTFREESLOT; i++)
	{
		tempname = sprnames[i];
		tempname[0] = (char)('0' + (char)((i-SPR_FIRSTFREESLOT+1)/1000));
		tempname[1] = (char)('0' + (char)(((i-SPR_FIRSTFREESLOT+1)/100)%10));
		tempname[2] = (char)('0' + (char)(((i-SPR_FIRSTFREESLOT+1)/10)%10));
		tempname[3] = (char)('0' + (char)((i-SPR_FIRSTFREESLOT+1)%10));
		tempname[4] = '\0';
	}
	sprnames[i][0] = '\0'; // i == NUMSPRITES
	memset(&states[S_FIRSTFREESLOT], 0, sizeof (state_t) * NUMSTATEFREESLOTS);
	memset(&mobjinfo[MT_FIRSTFREESLOT], 0, sizeof (mobjinfo_t) * NUMMOBJFREESLOTS);
	memset(&skincolors[SKINCOLOR_FIRSTFREESLOT], 0, sizeof (skincolor_t) * NUMCOLORFREESLOTS);
	for (i = SKINCOLOR_FIRSTFREESLOT; i <= SKINCOLOR_LASTFREESLOT; i++) {
		skincolors[i].accessible = false;
		skincolors[i].name[0] = '\0';
	}
	for (i = MT_FIRSTFREESLOT; i <= MT_LASTFREESLOT; i++)
		mobjinfo[i].doomednum = -1;
}

#ifdef ALLOW_RESETDATA
static char *sprnamesbackup;
static state_t *statesbackup;
static mobjinfo_t *mobjinfobackup;
static skincolor_t *skincolorsbackup;
static size_t sprnamesbackupsize, statesbackupsize, mobjinfobackupsize, skincolorsbackupsize;
#endif

void P_BackupTables(void)
{
#ifdef ALLOW_RESETDATA
	// Allocate buffers in size equal to that of the uncompressed data to begin with
	sprnamesbackup = Z_Malloc(sizeof(sprnames), PU_STATIC, NULL);
	statesbackup = Z_Malloc(sizeof(states), PU_STATIC, NULL);
	mobjinfobackup = Z_Malloc(sizeof(mobjinfo), PU_STATIC, NULL);
	skincolorsbackup = Z_Malloc(sizeof(skincolors), PU_STATIC, NULL);

	// Sprite names
	sprnamesbackupsize = lzf_compress(sprnames, sizeof(sprnames), sprnamesbackup, sizeof(sprnames));
	if (sprnamesbackupsize > 0)
		sprnamesbackup = Z_Realloc(sprnamesbackup, sprnamesbackupsize, PU_STATIC, NULL);
	else
		M_Memcpy(sprnamesbackup, sprnames, sizeof(sprnames));

	// States
	statesbackupsize = lzf_compress(states, sizeof(states), statesbackup, sizeof(states));
	if (statesbackupsize > 0)
		statesbackup = Z_Realloc(statesbackup, statesbackupsize, PU_STATIC, NULL);
	else
		M_Memcpy(statesbackup, states, sizeof(states));

	// Mobj info
	mobjinfobackupsize = lzf_compress(mobjinfo, sizeof(mobjinfo), mobjinfobackup, sizeof(mobjinfo));
	if (mobjinfobackupsize > 0)
		mobjinfobackup = Z_Realloc(mobjinfobackup, mobjinfobackupsize, PU_STATIC, NULL);
	else
		M_Memcpy(mobjinfobackup, mobjinfo, sizeof(mobjinfo));

	//Skincolor info
	skincolorsbackupsize = lzf_compress(skincolors, sizeof(skincolors), skincolorsbackup, sizeof(skincolors));
	if (skincolorsbackupsize > 0)
		skincolorsbackup = Z_Realloc(skincolorsbackup, skincolorsbackupsize, PU_STATIC, NULL);
	else
		M_Memcpy(skincolorsbackup, skincolors, sizeof(skincolors));
#endif
}

void P_ResetData(INT32 flags)
{
#ifndef ALLOW_RESETDATA
	(void)flags;
	CONS_Alert(CONS_NOTICE, M_GetText("P_ResetData(): not supported in this build.\n"));
#else
	if (flags & 1)
	{
		if (sprnamesbackupsize > 0)
			lzf_decompress(sprnamesbackup, sprnamesbackupsize, sprnames, sizeof(sprnames));
		else
			M_Memcpy(sprnames, sprnamesbackup, sizeof(sprnamesbackup));
	}

	if (flags & 2)
	{
		if (statesbackupsize > 0)
			lzf_decompress(statesbackup, statesbackupsize, states, sizeof(states));
		else
			M_Memcpy(states, statesbackup, sizeof(statesbackup));
	}

	if (flags & 4)
	{
		if (mobjinfobackupsize > 0)
			lzf_decompress(mobjinfobackup, mobjinfobackupsize, mobjinfo, sizeof(mobjinfo));
		else
			M_Memcpy(mobjinfo, mobjinfobackup, sizeof(mobjinfobackup));
	}

	if (flags & 8)
	{
		if (skincolorsbackupsize > 0)
			lzf_decompress(skincolorsbackup, skincolorsbackupsize, skincolors, sizeof(skincolors));
		else
			M_Memcpy(skincolors, skincolorsbackup, sizeof(skincolorsbackup));
	}
#endif
}
