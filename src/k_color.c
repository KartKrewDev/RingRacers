// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_color.c
/// \brief Waypoint handling from the relevant mobjs
///        Setup and interfacing with waypoints for the main game

#include "k_color.h"

#include "doomdef.h"
#include "doomtype.h"
#include "r_draw.h"
#include "r_things.h"
#include "v_video.h"

// These should be within 14 characters to fit on the character select screen
const char *KartColor_Names[MAXSKINCOLORS] =
{
	"None",           // SKINCOLOR_NONE
	"White",          // SKINCOLOR_WHITE
	"Silver",         // SKINCOLOR_SILVER
	"Grey",           // SKINCOLOR_GREY
	"Nickel",         // SKINCOLOR_NICKEL
	"Black",          // SKINCOLOR_BLACK
	"Skunk",          // SKINCOLOR_SKUNK
	"Fairy",          // SKINCOLOR_FAIRY
	"Popcorn",        // SKINCOLOR_POPCORN
	"Artichoke",      // SKINCOLOR_ARTICHOKE
	"Pigeon",         // SKINCOLOR_PIGEON
	"Sepia",          // SKINCOLOR_SEPIA
	"Beige",          // SKINCOLOR_BEIGE
	"Caramel",        // SKINCOLOR_CARAMEL
	"Peach",          // SKINCOLOR_PEACH
	"Brown",          // SKINCOLOR_BROWN
	"Leather",        // SKINCOLOR_LEATHER
	"Pink",           // SKINCOLOR_PINK
	"Rose",           // SKINCOLOR_ROSE
	"Cinnamon",       // SKINCOLOR_CINNAMON
	"Ruby",           // SKINCOLOR_RUBY
	"Raspberry",      // SKINCOLOR_RASPBERRY
	"Red",            // SKINCOLOR_RED
	"Crimson",        // SKINCOLOR_CRIMSON
	"Maroon",         // SKINCOLOR_MAROON
	"Lemonade",       // SKINCOLOR_LEMONADE
	"Scarlet",        // SKINCOLOR_SCARLET
	"Ketchup",        // SKINCOLOR_KETCHUP
	"Dawn",           // SKINCOLOR_DAWN
	"Sunslam",        // SKINCOLOR_SUNSLAM
	"Creamsicle",     // SKINCOLOR_CREAMSICLE
	"Orange",         // SKINCOLOR_ORANGE
	"Rosewood",       // SKINCOLOR_ROSEWOOD
	"Tangerine",      // SKINCOLOR_TANGERINE
	"Tan",            // SKINCOLOR_TAN
	"Cream",          // SKINCOLOR_CREAM
	"Gold",           // SKINCOLOR_GOLD
	"Royal",          // SKINCOLOR_ROYAL
	"Bronze",         // SKINCOLOR_BRONZE
	"Copper",         // SKINCOLOR_COPPER
	"Yellow",         // SKINCOLOR_YELLOW
	"Mustard",        // SKINCOLOR_MUSTARD
	"Banana",         // SKINCOLOR_BANANA
	"Olive",          // SKINCOLOR_OLIVE
	"Crocodile",      // SKINCOLOR_CROCODILE
	"Peridot",        // SKINCOLOR_PERIDOT
	"Vomit",          // SKINCOLOR_VOMIT
	"Garden",         // SKINCOLOR_GARDEN
	"Lime",           // SKINCOLOR_LIME
	"Handheld",       // SKINCOLOR_HANDHELD
	"Tea",            // SKINCOLOR_TEA
	"Pistachio",      // SKINCOLOR_PISTACHIO
	"Moss",           // SKINCOLOR_MOSS
	"Camouflage",     // SKINCOLOR_CAMOUFLAGE
	"Mint",           // SKINCOLOR_MINT
	"Green",          // SKINCOLOR_GREEN
	"Pinetree",       // SKINCOLOR_PINETREE
	"Turtle",         // SKINCOLOR_TURTLE
	"Swamp",          // SKINCOLOR_SWAMP
	"Dream",          // SKINCOLOR_DREAM
	"Plague",         // SKINCOLOR_PLAGUE
	"Emerald",        // SKINCOLOR_EMERALD
	"Algae",          // SKINCOLOR_ALGAE
	"Aquamarine",     // SKINCOLOR_AQUAMARINE
	"Turquoise",      // SKINCOLOR_TURQUOISE
	"Teal",           // SKINCOLOR_TEAL
	"Robin",          // SKINCOLOR_ROBIN
	"Cyan",           // SKINCOLOR_CYAN
	"Jawz",           // SKINCOLOR_JAWZ
	"Cerulean",       // SKINCOLOR_CERULEAN
	"Navy",           // SKINCOLOR_NAVY
	"Platinum",       // SKINCOLOR_PLATINUM
	"Slate",          // SKINCOLOR_SLATE
	"Steel",          // SKINCOLOR_STEEL
	"Thunder",        // SKINCOLOR_THUNDER
	"Nova",           // SKINCOLOR_NOVA
	"Rust",           // SKINCOLOR_RUST
	"Wristwatch",     // SKINCOLOR_WRISTWATCH
	"Jet",            // SKINCOLOR_JET
	"Sapphire",       // SKINCOLOR_SAPPHIRE
	"Ultramarine",    // SKINCOLOR_ULTRAMARINE
	"Periwinkle",     // SKINCOLOR_PERIWINKLE
	"Blue",           // SKINCOLOR_BLUE
	"Midnight",       // SKINCOLOR_MIDNIGHT
	"Blueberry",      // SKINCOLOR_BLUEBERRY
	"Thistle",        // SKINCOLOR_THISTLE
	"Purple",         // SKINCOLOR_PURPLE
	"Pastel",         // SKINCOLOR_PASTEL
	"Moonset",        // SKINCOLOR_MOONSET
	"Dusk",           // SKINCOLOR_DUSK
	"Violet",         // SKINCOLOR_VIOLET
	"Magenta",        // SKINCOLOR_MAGENTA
	"Fuchsia",        // SKINCOLOR_FUCHSIA
	"Toxic",          // SKINCOLOR_TOXIC
	"Mauve",          // SKINCOLOR_MAUVE
	"Lavender",       // SKINCOLOR_LAVENDER
	"Byzantium",      // SKINCOLOR_BYZANTIUM
	"Pomegranate",    // SKINCOLOR_POMEGRANATE
	"Lilac",          // SKINCOLOR_LILAC
	"Blossom",        // SKINCOLOR_BLOSSOM
	"Taffy"           // SKINCOLOR_TAFFY
};

// Color_Opposite replacement; frame setting has not been changed from 8 for most, should be done later
const UINT8 KartColor_Opposite[MAXSKINCOLORS*2] =
{
	SKINCOLOR_NONE,8,         // SKINCOLOR_NONE
	SKINCOLOR_BLACK,8,        // SKINCOLOR_WHITE
	SKINCOLOR_NICKEL,8,       // SKINCOLOR_SILVER
	SKINCOLOR_GREY,8,         // SKINCOLOR_GREY
	SKINCOLOR_SILVER,8,       // SKINCOLOR_NICKEL
	SKINCOLOR_WHITE,8,        // SKINCOLOR_BLACK
	SKINCOLOR_VOMIT,8,        // SKINCOLOR_SKUNK
	SKINCOLOR_ARTICHOKE,12,   // SKINCOLOR_FAIRY
	SKINCOLOR_PIGEON,12,      // SKINCOLOR_POPCORN
	SKINCOLOR_FAIRY,12,       // SKINCOLOR_ARTICHOKE
	SKINCOLOR_POPCORN,12,     // SKINCOLOR_PIGEON
	SKINCOLOR_LEATHER,6,      // SKINCOLOR_SEPIA
	SKINCOLOR_BROWN,2,        // SKINCOLOR_BEIGE
	SKINCOLOR_CERULEAN,8,     // SKINCOLOR_CARAMEL
	SKINCOLOR_CYAN,8,         // SKINCOLOR_PEACH
	SKINCOLOR_BEIGE,8,        // SKINCOLOR_BROWN
	SKINCOLOR_SEPIA,8,        // SKINCOLOR_LEATHER
	SKINCOLOR_PISTACHIO,8,    // SKINCOLOR_PINK
	SKINCOLOR_MOSS,8,         // SKINCOLOR_ROSE
	SKINCOLOR_WRISTWATCH,6,   // SKINCOLOR_CINNAMON
	SKINCOLOR_SAPPHIRE,8,     // SKINCOLOR_RUBY
	SKINCOLOR_MINT,8,         // SKINCOLOR_RASPBERRY
	SKINCOLOR_GREEN,6,        // SKINCOLOR_RED
	SKINCOLOR_PINETREE,6,     // SKINCOLOR_CRIMSON
	SKINCOLOR_TOXIC,8,        // SKINCOLOR_MAROON
	SKINCOLOR_THUNDER,8,      // SKINCOLOR_LEMONADE
	SKINCOLOR_ALGAE,10,       // SKINCOLOR_SCARLET
	SKINCOLOR_MUSTARD,10,     // SKINCOLOR_KETCHUP
	SKINCOLOR_DUSK,8,         // SKINCOLOR_DAWN
	SKINCOLOR_MOONSET,8,      // SKINCOLOR_SUNSLAM
	SKINCOLOR_PERIWINKLE,8,   // SKINCOLOR_CREAMSICLE
	SKINCOLOR_BLUE,8,         // SKINCOLOR_ORANGE
	SKINCOLOR_BLUEBERRY,6,    // SKINCOLOR_ROSEWOOD
	SKINCOLOR_LIME,8,         // SKINCOLOR_TANGERINE
	SKINCOLOR_RUST,8,         // SKINCOLOR_TAN
	SKINCOLOR_COPPER,10,      // SKINCOLOR_CREAM
	SKINCOLOR_SLATE,8,        // SKINCOLOR_GOLD
	SKINCOLOR_PLATINUM,6,     // SKINCOLOR_ROYAL
	SKINCOLOR_STEEL,8,        // SKINCOLOR_BRONZE
	SKINCOLOR_CREAM,6,        // SKINCOLOR_COPPER
	SKINCOLOR_AQUAMARINE,8,   // SKINCOLOR_YELLOW
	SKINCOLOR_KETCHUP,8,      // SKINCOLOR_MUSTARD
	SKINCOLOR_EMERALD,8,      // SKINCOLOR_BANANA
	SKINCOLOR_TEAL,8,         // SKINCOLOR_OLIVE
	SKINCOLOR_VIOLET,8,       // SKINCOLOR_CROCODILE
	SKINCOLOR_NAVY,6,         // SKINCOLOR_PERIDOT
	SKINCOLOR_SKUNK,8,        // SKINCOLOR_VOMIT
	SKINCOLOR_LAVENDER,6,     // SKINCOLOR_GARDEN
	SKINCOLOR_TANGERINE,8,    // SKINCOLOR_LIME
	SKINCOLOR_ULTRAMARINE,8,  // SKINCOLOR_HANDHELD
	SKINCOLOR_BLOSSOM,8,      // SKINCOLOR_TEA
	SKINCOLOR_PINK,6,         // SKINCOLOR_PISTACHIO
	SKINCOLOR_ROSE,8,         // SKINCOLOR_MOSS
	SKINCOLOR_CAMOUFLAGE,8,   // SKINCOLOR_CAMOUFLAGE
	SKINCOLOR_RASPBERRY,8,    // SKINCOLOR_MINT
	SKINCOLOR_RED,8,          // SKINCOLOR_GREEN
	SKINCOLOR_CRIMSON,8,      // SKINCOLOR_PINETREE
	SKINCOLOR_MAGENTA,8,      // SKINCOLOR_TURTLE
	SKINCOLOR_BYZANTIUM,8,    // SKINCOLOR_SWAMP
	SKINCOLOR_POMEGRANATE,8,  // SKINCOLOR_DREAM
	SKINCOLOR_NOVA,8,         // SKINCOLOR_PLAGUE
	SKINCOLOR_BANANA,8,       // SKINCOLOR_EMERALD
	SKINCOLOR_SCARLET,10,     // SKINCOLOR_ALGAE
	SKINCOLOR_YELLOW,8,       // SKINCOLOR_AQUAMARINE
	SKINCOLOR_MAUVE,10,       // SKINCOLOR_TURQUOISE
	SKINCOLOR_OLIVE,8,        // SKINCOLOR_TEAL
	SKINCOLOR_THISTLE,8,      // SKINCOLOR_ROBIN
	SKINCOLOR_PEACH,8,        // SKINCOLOR_CYAN
	SKINCOLOR_LILAC,10,       // SKINCOLOR_JAWZ
	SKINCOLOR_CARAMEL,8,      // SKINCOLOR_CERULEAN
	SKINCOLOR_PERIDOT,8,      // SKINCOLOR_NAVY
	SKINCOLOR_ROYAL,8,        // SKINCOLOR_PLATINUM
	SKINCOLOR_GOLD,10,        // SKINCOLOR_SLATE
	SKINCOLOR_BRONZE,10,      // SKINCOLOR_STEEL
	SKINCOLOR_LEMONADE,8,     // SKINCOLOR_THUNDER
	SKINCOLOR_PLAGUE,10,      // SKINCOLOR_NOVA
	SKINCOLOR_TAN,8,          // SKINCOLOR_RUST
	SKINCOLOR_CINNAMON,8,     // SKINCOLOR_WRISTWATCH
	SKINCOLOR_TAFFY,8,        // SKINCOLOR_JET
	SKINCOLOR_RUBY,6,         // SKINCOLOR_SAPPHIRE
	SKINCOLOR_HANDHELD,10,    // SKINCOLOR_ULTRAMARINE
	SKINCOLOR_CREAMSICLE,8,   // SKINCOLOR_PERIWINKLE
	SKINCOLOR_ORANGE,8,       // SKINCOLOR_BLUE
	SKINCOLOR_ROSEWOOD,8,     // SKINCOLOR_MIDNIGHT
	SKINCOLOR_PURPLE,8,       // SKINCOLOR_BLUEBERRY
	SKINCOLOR_ROBIN,8,        // SKINCOLOR_THISTLE
	SKINCOLOR_MIDNIGHT,10,    // SKINCOLOR_PURPLE
	SKINCOLOR_FUCHSIA,11,     // SKINCOLOR_PASTEL
	SKINCOLOR_SUNSLAM,10,     // SKINCOLOR_MOONSET
	SKINCOLOR_DAWN,6,         // SKINCOLOR_DUSK
	SKINCOLOR_CROCODILE,8,    // SKINCOLOR_VIOLET
	SKINCOLOR_TURTLE,8,       // SKINCOLOR_MAGENTA
	SKINCOLOR_PASTEL,11,      // SKINCOLOR_FUCHSIA
	SKINCOLOR_MAROON,8,       // SKINCOLOR_TOXIC
	SKINCOLOR_TURQUOISE,8,    // SKINCOLOR_MAUVE
	SKINCOLOR_GARDEN,6,       // SKINCOLOR_LAVENDER
	SKINCOLOR_SWAMP,8,        // SKINCOLOR_BYZANTIUM
	SKINCOLOR_DREAM,8,        // SKINCOLOR_POMEGRANATE
	SKINCOLOR_JAWZ,6,         // SKINCOLOR_LILAC
	SKINCOLOR_TEA,8,          // SKINCOLOR_BLOSSOM
	SKINCOLOR_JET,8           // SKINCOLOR_TAFFY
};

UINT8 colortranslations[MAXTRANSLATIONS][16] = {
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, // SKINCOLOR_NONE
	{  0,   0,   0,   0,   1,   2,   5,   8,   9,  11,  14,  17,  20,  22,  25,  28}, // SKINCOLOR_WHITE
	{  0,   1,   2,   3,   5,   7,   9,  12,  13,  15,  18,  20,  23,  25,  27,  30}, // SKINCOLOR_SILVER
	{  1,   3,   5,   7,   9,  11,  13,  15,  17,  19,  21,  23,  25,  27,  29,  31}, // SKINCOLOR_GREY
	{  3,   5,   8,  11,  15,  17,  19,  21,  23,  24,  25,  26,  27,  29,  30,  31}, // SKINCOLOR_NICKEL
	{  4,   7,  11,  15,  20,  22,  24,  27,  28,  28,  28,  29,  29,  30,  30,  31}, // SKINCOLOR_BLACK
	{  0,   1,   2,   3,   4,  10,  16,  21,  23,  24,  25,  26,  27,  28,  29,  31}, // SKINCOLOR_SKUNK
	{  0,   0, 252, 252, 200, 201, 211,  14,  16,  18,  20,  22,  24,  26,  28,  31}, // SKINCOLOR_FAIRY
	{  0,  80,  80,  81,  82, 218, 240,  11,  13,  16,  18,  21,  23,  26,  28,  31}, // SKINCOLOR_POPCORN
	{ 80,  88,  89,  98,  99,  91,  12,  14,  16,  18,  20,  22,  24,  26,  28,  31}, // SKINCOLOR_ARTICHOKE
	{  0, 128, 129, 130, 146, 170,  14,  15,  17,  19,  21,  23,  25,  27,  29,  31}, // SKINCOLOR_PIGEON
	{  0,   1,   3,   5,   7,   9, 241, 242, 243, 245, 247, 249, 236, 237, 238, 239}, // SKINCOLOR_SEPIA
	{  0, 208, 216, 217, 240, 241, 242, 243, 245, 247, 249, 250, 251, 237, 238, 239}, // SKINCOLOR_BEIGE
	{208,  48, 216, 217, 218, 220, 221, 223, 224, 226, 228, 230, 232, 234, 236, 239}, // SKINCOLOR_CARAMEL
	{  0, 208,  48, 216, 218, 221, 212, 213, 214, 215, 206, 207, 197, 198, 199, 254}, // SKINCOLOR_PEACH
	{216, 217, 219, 221, 224, 225, 227, 229, 230, 232, 234, 235, 237, 239,  29,  30}, // SKINCOLOR_BROWN
	{218, 221, 224, 227, 229, 231, 233, 235, 237, 239,  28,  28,  29,  29,  30,  31}, // SKINCOLOR_LEATHER
	{  0, 208, 208, 209, 209, 210, 211, 211, 212, 213, 214, 215,  41,  43,  45,  46}, // SKINCOLOR_PINK
	{209, 210, 211, 211, 212, 213, 214, 215,  41,  42,  43,  44,  45,  71,  46,  47}, // SKINCOLOR_ROSE
	{216, 221, 224, 226, 228,  60,  61,  43,  44,  45,  71,  46,  47,  29,  30,  31}, // SKINCOLOR_CINNAMON
	{  0, 208, 209, 210, 211, 213,  39,  40,  41,  43, 186, 186, 169, 169, 253, 254}, // SKINCOLOR_RUBY
	{  0, 208, 209, 210,  32,  33,  34,  35,  37,  39,  41,  43,  44,  45,  46,  47}, // SKINCOLOR_RASPBERRY
	{209, 210,  32,  34,  36,  38,  39,  40,  41,  42,  43,  44 , 45,  71,  46,  47}, // SKINCOLOR_RED
	{210,  33,  35,  38,  40,  42,  43,  45,  71,  71,  46,  46,  47,  47,  30,  31}, // SKINCOLOR_CRIMSON
	{ 32,  33,  35,  37,  39,  41,  43, 237,  26,  26,  27,  27,  28,  29,  30,  31}, // SKINCOLOR_MAROON
	{  0,  80,  81,  82,  83, 216, 210, 211, 212, 213, 214, 215,  43,  44,  71,  47}, // SKINCOLOR_LEMONADE
	{ 48,  49,  50,  51,  53,  34,  36,  38, 184, 185, 168, 168, 169, 169, 254,  31}, // SKINCOLOR_SCARLET
	{ 72,  73,  64,  51,  52,  54,  34,  36,  38,  40,  42,  43,  44,  71,  46,  47}, // SKINCOLOR_KETCHUP
	{  0, 208, 216, 209, 210, 211, 212,  57,  58,  59,  60,  61,  63,  71,  47,  31}, // SKINCOLOR_DAWN
	{ 82,  72,  73,  64,  51,  53,  55, 213, 214, 195, 195, 173, 174, 175, 253, 254}, // SKINCOLOR_SUNSLAM
	{  0,   0, 208, 208,  48,  49,  50,  52,  53,  54,  56,  57,  58,  60,  61,  63}, // SKINCOLOR_CREAMSICLE
	{208,  48,  49,  50,  51,  52,  53,  54,  55,  57,  59,  60,  62,  44,  71,  47}, // SKINCOLOR_ORANGE
	{ 50,  52,  55,  56,  58,  59,  60,  61,  62,  63,  44,  45,  71,  46,  47,  30}, // SKINCOLOR_ROSEWOOD
	{ 80,  81,  82,  83,  64,  51,  52,  54,  55,  57,  58,  60,  61,  63,  71,  47}, // SKINCOLOR_TANGERINE
	{  0,  80,  81,  82,  83,  84,  85,  86,  87, 245, 246, 248, 249, 251, 237, 239}, // SKINCOLOR_TAN
	{  0,  80,  80,  81,  81,  49,  51, 222, 224, 227, 230, 233, 236, 239,  29,  31}, // SKINCOLOR_CREAM
	{  0,  80,  81,  83,  64,  65,  66,  67,  68, 215,  69,  70,  44,  71,  46,  47}, // SKINCOLOR_GOLD
	{ 80,  81,  83,  64,  65, 223, 229, 196, 196, 197, 197, 198, 199,  29,  30,  31}, // SKINCOLOR_ROYAL
	{ 83,  64,  65,  66,  67, 215,  69,  70,  44,  44,  45,  71,  46,  47,  29,  31}, // SKINCOLOR_BRONZE
	{  0,  82,  64,  65,  67,  68,  70, 237, 239,  28,  28,  29,  29,  30,  30,  31}, // SKINCOLOR_COPPER
	{  0,  80,  81,  82,  83,  73,  84,  74,  64,  65,  66,  67,  68,  69,  70,  71}, // SKINCOLOR_YELLOW
	{ 80,  81,  82,  83,  64,  65,  65,  76,  76,  77,  77,  78,  79, 237, 239,  29}, // SKINCOLOR_MUSTARD
	{ 80,  81,  83,  72,  73,  74,  75,  76,  77,  78,  79, 236, 237, 238, 239,  30}, // SKINCOLOR_BANANA
	{ 80,  82,  73,  74,  75,  76,  77,  78,  79, 236, 237, 238, 239,  28,  29,  31}, // SKINCOLOR_OLIVE
	{  0,  80,  81,  88,  88, 188, 189,  76,  76,  77,  78,  79, 236, 237, 238, 239}, // SKINCOLOR_CROCODILE
	{  0,  80,  81,  88, 188, 189, 190, 191,  94,  94,  95,  95, 109, 110, 111,  31}, // SKINCOLOR_PERIDOT
	{  0, 208, 216, 209, 218,  51,  65,  76, 191, 191, 126, 143, 138, 175, 169, 254}, // SKINCOLOR_VOMIT
	{ 81,  82,  83,  73,  64,  65,  66,  92,  92,  93,  93,  94,  95, 109, 110, 111}, // SKINCOLOR_GARDEN
	{  0,  80,  81,  88, 188, 189, 114, 114, 115, 115, 116, 116, 117, 118, 119, 111}, // SKINCOLOR_LIME
	{ 83,  72,  73,  74,  75,  76, 102, 104, 105, 106, 107, 108, 109, 110, 111,  31}, // SKINCOLOR_HANDHELD
	{  0,  80,  80,  81,  88,  89,  90,  91,  92,  93,  94,  95, 109, 110, 111,  31}, // SKINCOLOR_TEA
	{  0,  80,  88,  88,  89,  90,  91, 102, 103, 104, 105, 106, 107, 108, 109, 110}, // SKINCOLOR_PISTACHIO
	{ 88,  89,  90,  91,  91,  92,  93,  94, 107, 107, 108, 108, 109, 109, 110, 111}, // SKINCOLOR_MOSS
	{208,  84,  85, 240, 241, 243, 245,  94, 107, 108, 108, 109, 109, 110, 110, 111}, // SKINCOLOR_CAMOUFLAGE
	{  0,  88,  88,  89,  89, 100, 101, 102, 125, 126, 143, 143, 138, 175, 169, 254}, // SKINCOLOR_MINT
	{ 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111}, // SKINCOLOR_GREEN
	{ 97,  99, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,  30,  30,  31}, // SKINCOLOR_PINETREE
	{ 96, 112, 112, 113, 113, 114, 114, 115, 115, 116, 116, 117, 117, 118, 119, 111}, // SKINCOLOR_TURTLE
	{ 96, 112, 113, 114, 115, 116, 117, 118, 119, 119,  29,  29,  30,  30,  31,  31}, // SKINCOLOR_SWAMP
	{  0,   0, 208, 208,  48,  89,  98, 100, 148, 148, 172, 172, 173, 173, 174, 175}, // SKINCOLOR_DREAM
	{ 80,  88,  96, 112, 113, 124, 142, 149, 149, 173, 174, 175, 169, 253, 254,  31}, // SKINCOLOR_PLAGUE
	{  0, 120, 121, 112, 113, 114, 115, 125, 125, 126, 126, 127, 138, 175, 253, 254}, // SKINCOLOR_EMERALD
	{128, 129, 130, 131, 132, 133, 134, 115, 115, 116, 116, 117, 118, 119, 110, 111}, // SKINCOLOR_ALGAE
	{  0, 128, 120, 121, 122, 123, 124, 125, 126, 126, 127, 127, 118, 118, 119, 111}, // SKINCOLOR_AQUAMARINE
	{128, 120, 121, 122, 123, 141, 141, 142, 142, 143, 143, 138, 138, 139, 139,  31}, // SKINCOLOR_TURQUOISE
	{  0, 120, 120, 121, 140, 141, 142, 143, 143, 138, 138, 139, 139, 254, 254,  31}, // SKINCOLOR_TEAL
	{  0,  80,  81,  82,  83,  88, 121, 140, 133, 133, 134, 135, 136, 137, 138, 139}, // SKINCOLOR_ROBIN
	{  0,   0, 128, 128, 255, 131, 132, 134, 142, 142, 143, 127, 118, 119, 110, 111}, // SKINCOLOR_CYAN
	{  0,   0, 128, 128, 129, 146, 133, 134, 135, 149, 149, 173, 173, 174, 175,  31}, // SKINCOLOR_JAWZ
	{  0, 128, 129, 130, 131, 132, 133, 135, 136, 136, 137, 137, 138, 138, 139,  31}, // SKINCOLOR_CERULEAN
	{128, 129, 130, 132, 134, 135, 136, 137, 137, 138, 138, 139, 139,  29,  30,  31}, // SKINCOLOR_NAVY
	{  0,   0,   0, 144, 144, 145,   9,  11,  14, 142, 136, 137, 138, 138, 139,  31}, // SKINCOLOR_PLATINUM
	{  0,   0, 144, 144, 144, 145, 145, 145, 170, 170, 171, 171, 172, 173, 174, 175}, // SKINCOLOR_SLATE
	{  0, 144, 144, 145, 145, 170, 170, 171, 171, 172, 172, 173, 173, 174, 175,  31}, // SKINCOLOR_STEEL
	{ 80,  81,  82,  83,  64,  65,  11, 171, 172, 173, 173, 157, 158, 159, 254,  31}, // SKINCOLOR_THUNDER
	{  0,  83,  49,  50,  51,  32, 192, 148, 148, 172, 173, 174, 175,  29,  30,  31}, // SKINCOLOR_NOVA
	{208,  48, 216, 217, 240, 241, 242, 171, 172, 173,  24,  25,  26,  28,  29,  31}, // SKINCOLOR_RUST
	{ 48, 218, 221, 224, 227, 231, 196, 173, 173, 174, 159, 159, 253, 253, 254,  31}, // SKINCOLOR_WRISTWATCH
	{145, 146, 147, 148, 149, 173, 173, 174, 175, 175,  28,  28,  29,  29,  30,  31}, // SKINCOLOR_JET
	{  0, 128, 129, 131, 133, 135, 149, 150, 152, 154, 156, 158, 159, 253, 254,  31}, // SKINCOLOR_SAPPHIRE
	{  0,   0, 120, 120, 121, 133, 135, 149, 149, 166, 166, 167, 168, 169, 254,  31}, // SKINCOLOR_ULTRAMARINE
	{  0,   0, 144, 144, 145, 146, 147, 149, 150, 152, 154, 155, 157, 159, 253, 254}, // SKINCOLOR_PERIWINKLE
	{144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 155, 156, 158, 253, 254,  31}, // SKINCOLOR_BLUE
	{146, 148, 149, 150, 152, 153, 155, 157, 159, 253, 253, 254, 254,  31,  31,  31}, // SKINCOLOR_MIDNIGHT
	{  0, 144, 145, 146, 147, 171, 172, 166, 166, 167, 167, 168, 168, 175, 169, 253}, // SKINCOLOR_BLUEBERRY
	{  0,   0,   0, 252, 252, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 254}, // SKINCOLOR_THISTLE
	{  0, 252, 160, 161, 162, 163, 164, 165, 166, 167, 168, 168, 169, 169, 253, 254}, // SKINCOLOR_PURPLE
	{  0, 128, 128, 129, 129, 146, 170, 162, 163, 164, 165, 166, 167, 168, 169, 254}, // SKINCOLOR_PASTEL
	{  0, 144, 145, 146, 170, 162, 163, 184, 184, 207, 207,  44,  45,  46,  47,  31}, // SKINCOLOR_MOONSET
	{252, 200, 201, 192, 193, 194, 172, 172, 173, 173, 174, 174, 175, 169, 253, 254}, // SKINCOLOR_DUSK
	{176, 177, 178, 179, 180, 181, 182, 183, 184, 165, 165, 166, 167, 168, 169, 254}, // SKINCOLOR_VIOLET
	{252, 200, 177, 177, 178, 179, 180, 181, 182, 183, 183, 184, 185, 186, 187,  31}, // SKINCOLOR_MAGENTA
	{208, 209, 209,  32,  33, 182, 183, 184, 185, 185, 186, 186, 187, 253, 254,  31}, // SKINCOLOR_FUCHSIA
	{  0,   0,  88,  88,  89,   6,   8,  10, 193, 194, 195, 184, 185, 186, 187,  31}, // SKINCOLOR_TOXIC
	{ 80,  81,  82,  83,  64,  50, 201, 192, 193, 194, 195, 173, 174, 175, 253, 254}, // SKINCOLOR_MAUVE
	{252, 177, 179, 192, 193, 194, 195, 196, 196, 197, 197, 198, 198, 199,  30,  31}, // SKINCOLOR_LAVENDER
	{145, 192, 193, 194, 195, 196, 197, 198, 199, 199,  29,  29,  30,  30,  31,  31}, // SKINCOLOR_BYZANTIUM
	{208, 209, 210, 211, 212, 213, 214, 195, 195, 196, 196, 197, 198, 199,  29,  30}, // SKINCOLOR_POMEGRANATE
	{  0,   0,   0, 252, 252, 176, 200, 201, 179, 192, 193, 194, 195, 196, 197, 198}, // SKINCOLOR_LILAC
	{  0, 252, 252, 176, 200, 177, 201, 202, 202,  34,  36,  38,  40,  42,  45,  46}, // SKINCOLOR_BLOSSOM
	{  0, 252, 252, 200, 200, 201, 202, 203, 204, 204, 205, 206, 207,  43,  45,  47}, // SKINCOLOR_TAFFY

	// THESE STILL NEED CONVERTED!!!
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  96, 100, 104, 113, 116, 119}, // SKINCOLOR_SUPER1
	{  0,   0,   0,   0,   0,   0,   0,   0,  96,  98, 101, 104, 113, 115, 117, 119}, // SKINCOLOR_SUPER2
	{  0,   0,   0,   0,   0,   0,  96,  98, 100, 102, 104, 113, 114, 116, 117, 119}, // SKINCOLOR_SUPER3
	{  0,   0,   0,   0,  96,  97,  99, 100, 102, 104, 113, 114, 115, 116, 117, 119}, // SKINCOLOR_SUPER4
	{  0,   0,  96,   0,   0,   0,   0,   0, 104, 113, 114, 115, 116, 117, 118, 119}, // SKINCOLOR_SUPER5
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  80,  82,  85, 115, 117, 119}, // SKINCOLOR_TSUPER1
	{  0,   0,   0,   0,   0,   0,   0,   0,  80,  81,  83,  85, 115, 116, 117, 119}, // SKINCOLOR_TSUPER2
	{  0,   0,   0,   0,   0,   0,  80,  81,  82,  83,  85, 115, 116, 117, 118, 119}, // SKINCOLOR_TSUPER3
	{  0,   0,   0,   0,  80,  81,  82,  83,  84,  85, 115, 115, 116, 117, 118, 119}, // SKINCOLOR_TSUPER4
	{  0,   0,  80,  80,  81,  82,  83,  84,  85, 115, 115, 116, 117, 117, 118, 119}, // SKINCOLOR_TSUPER5
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 121, 123, 125, 127, 129, 132}, // SKINCOLOR_KSUPER1
	{  0,   0,   0,   0,   0,   0,   0,   0, 121, 122, 124, 125, 127, 128, 130, 132}, // SKINCOLOR_KSUPER2
	{  0,   0,   0,   0,   0,   0, 121, 122, 123, 124, 125, 127, 128, 129, 130, 132}, // SKINCOLOR_KSUPER3
	{  0,   0,   0,   0, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132}, // SKINCOLOR_KSUPER4
	{  0,   0, 121, 121, 122, 123, 124, 125, 126, 126, 127, 128, 129, 130, 131, 132}, // SKINCOLOR_KSUPER5
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1, 122, 124, 248, 251, 255}, // SKINCOLOR_PSUPER1
	{  0,   0,   0,   0,   0,   0,   0,   0,   1, 121, 122, 124, 248, 250, 252, 255}, // SKINCOLOR_PSUPER2
	{  0,   0,   0,   0,   0,   0,   1, 121, 122, 123, 124, 248, 249, 251, 253, 255}, // SKINCOLOR_PSUPER3
	{  0,   0,   0,   0,   1, 121, 122, 123, 124, 248, 249, 250, 251, 252, 253, 255}, // SKINCOLOR_PSUPER4
	{  0,   0,   1, 121, 122, 123, 124, 248, 248, 249, 250, 251, 252, 253, 254, 255}, // SKINCOLOR_PSUPER5
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 224, 225, 227, 228, 230, 232}, // SKINCOLOR_BSUPER1
	{  0,   0,   0,   0,   0,   0,   0,   0, 224, 225, 226, 227, 228, 229, 230, 232}, // SKINCOLOR_BSUPER2
	{  0,   0,   0,   0,   0,   0, 224, 224, 225, 226, 227, 228, 229, 230, 231, 232}, // SKINCOLOR_BSUPER3
	{  0,   0,   0,   0, 224, 224, 225, 226, 226, 227, 228, 229, 229, 230, 231, 232}, // SKINCOLOR_BSUPER4
	{  0,   0, 224, 224, 225, 225, 226, 227, 227, 228, 228, 229, 230, 230, 231, 232}, // SKINCOLOR_BSUPER5
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 208, 210, 212, 215, 220, 222}, // SKINCOLOR_ASUPER1
	{  0,   0,   0,   0,   0,   0,   0,   0, 208, 209, 211, 213, 215, 220, 221, 223}, // SKINCOLOR_ASUPER2
	{  0,   0,   0,   0,   0,   0, 208, 209, 210, 211, 212, 213, 215, 220, 221, 223}, // SKINCOLOR_ASUPER3
	{  0,   0,   0,   0, 208, 209, 210, 211, 212, 213, 214, 215, 220, 221, 222, 223}, // SKINCOLOR_ASUPER4
	{  0,   0, 208, 208, 209, 210, 211, 211, 212, 213, 214, 215, 220, 221, 222, 223}, // SKINCOLOR_ASUPER5
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 176, 160, 163, 167, 171, 175}, // SKINCOLOR_GSUPER1
	{  0,   0,   0,   0,   0,   0,   0,   0, 176, 176, 160, 163, 166, 169, 172, 175}, // SKINCOLOR_GSUPER2
	{  0,   0,   0,   0,   0,   0, 176, 176, 160, 162, 164, 166, 168, 170, 172, 175}, // SKINCOLOR_GSUPER3
	{  0,   0,   0,   0, 176, 176, 176, 160, 161, 163, 165, 167, 169, 171, 173, 175}, // SKINCOLOR_GSUPER4
	{  0,   0, 176, 176, 176, 160, 161, 163, 164, 166, 167, 169, 170, 172, 173, 175}, // SKINCOLOR_GSUPER5
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, // SKINCOLOR_WSUPER1
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   4,   9}, // SKINCOLOR_WSUPER2
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   2,   4,   6,   8,  11}, // SKINCOLOR_WSUPER3
	{  0,   0,   0,   0,   0,   0,   0,   1,   1,   3,   4,   6,   8,   9,  11,  13}, // SKINCOLOR_WSUPER4
	{  0,   0,   0,   0,   1,   1,   2,   4,   5,   6,   8,   9,  10,  12,  13,  15}, // SKINCOLOR_WSUPER5
	{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  96,  98,  99,  81,  73,  79}, // SKINCOLOR_CSUPER1
	{  0,   0,   0,   0,   0,   0,   0,   0,  96,  97,  98,  81,  81,  71,  75,  79}, // SKINCOLOR_CSUPER2
	{  0,   0,   0,   0,   0,   0,  96,  97,  98,  99,  81,  81,  70,  73,  76,  79}, // SKINCOLOR_CSUPER3
	{  0,   0,   0,   0,  96,  96,  97,  98,  99,  81,  81,  70,  72,  74,  76,  79}, // SKINCOLOR_CSUPER4
	{  0,   0,  96,  96,  97,  98,  98,  99,  81,  81,  69,  71,  73,  75,  77,  79}, // SKINCOLOR_CSUPER5
};

/*--------------------------------------------------
	UINT8 K_ColorRelativeLuminance(UINT8 r, UINT8 g, UINT8 b)

		See header file for description.
--------------------------------------------------*/

UINT8 K_ColorRelativeLuminance(UINT8 r, UINT8 g, UINT8 b)
{
	UINT32 redweight = 1063 * r;
	UINT32 greenweight = 3576 * g;
	UINT32 blueweight = 361 * b;
	UINT32 brightness = (redweight + greenweight + blueweight) / 5000;
	return min(brightness, UINT8_MAX);
}

/*--------------------------------------------------
	void K_RainbowColormap(UINT8 *dest_colormap, UINT8 skincolor)

		See header file for description.
--------------------------------------------------*/

void K_RainbowColormap(UINT8 *dest_colormap, UINT8 skincolor)
{
	INT32 i;
	RGBA_t color;
	UINT8 brightness;
	INT32 j;
	UINT8 colorbrightnesses[16];
	UINT16 brightdif;
	INT32 temp;

	// first generate the brightness of all the colours of that skincolour
	for (i = 0; i < 16; i++)
	{
		color = V_GetColor(colortranslations[skincolor][i]);
		colorbrightnesses[i] = K_ColorRelativeLuminance(color.s.red, color.s.green, color.s.blue);
	}

	// next, for every colour in the palette, choose the transcolor that has the closest brightness
	for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
	{
		if (i == 0 || i == 31) // pure black and pure white don't change
		{
			dest_colormap[i] = (UINT8)i;
			continue;
		}

		color = V_GetColor(i);
		brightness = K_ColorRelativeLuminance(color.s.red, color.s.green, color.s.blue);
		brightdif = 256;

		for (j = 0; j < 16; j++)
		{
			temp = abs((INT16)brightness - (INT16)colorbrightnesses[j]);

			if (temp < brightdif)
			{
				brightdif = (UINT16)temp;
				dest_colormap[i] = colortranslations[skincolor][j];
			}
		}
	}
}

/**	\brief	Generates a translation colormap for Kart, to replace R_GenerateTranslationColormap in r_draw.c

	\param	dest_colormap	colormap to populate
	\param	skinnum			number of skin, TC_DEFAULT or TC_BOSS
	\param	color			translation color

	\return	void
*/
void K_GenerateKartColormap(UINT8 *dest_colormap, INT32 skinnum, UINT8 color)
{
	INT32 i;
	INT32 starttranscolor;

	// Handle a couple of simple special cases
	if (skinnum == TC_BOSS
		|| skinnum == TC_ALLWHITE
		|| skinnum == TC_METALSONIC
		|| skinnum == TC_BLINK
		|| color == SKINCOLOR_NONE)
	{
		for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
		{
			if (skinnum == TC_ALLWHITE)
				dest_colormap[i] = 0;
			else if (skinnum == TC_BLINK)
				dest_colormap[i] = colortranslations[color][3];
			else
				dest_colormap[i] = (UINT8)i;
		}

		// White!
		if (skinnum == TC_BOSS)
			dest_colormap[31] = 0;
		else if (skinnum == TC_METALSONIC)
			dest_colormap[143] = 0;

		return;
	}
	else if (skinnum == TC_RAINBOW)
	{
		K_RainbowColormap(dest_colormap, color);
		return;
	}

	starttranscolor = (skinnum != TC_DEFAULT) ? skins[skinnum].starttranscolor : DEFAULT_STARTTRANSCOLOR;

	// Fill in the entries of the palette that are fixed
	for (i = 0; i < starttranscolor; i++)
		dest_colormap[i] = (UINT8)i;

	for (i = (UINT8)(starttranscolor + 16); i < NUM_PALETTE_ENTRIES; i++)
		dest_colormap[i] = (UINT8)i;

	// Build the translated ramp
	for (i = 0; i < SKIN_RAMP_LENGTH; i++)
	{
		// Sryder 2017-10-26: What was here before was most definitely not particularly readable, check above for new color translation table
		dest_colormap[starttranscolor + i] = colortranslations[color][i];
	}
}

/**	\brief	Pulls kart color by name, to replace R_GetColorByName in r_draw.c

	\param	name	color name

	\return	0
*/
UINT8 K_GetKartColorByName(const char *name)
{
	UINT8 color = (UINT8)atoi(name);
	if (color > 0 && color < MAXSKINCOLORS)
		return color;
	for (color = 1; color < MAXSKINCOLORS; color++)
		if (!stricmp(KartColor_Names[color], name))
			return color;
	return 0;
}

//}
