/// \file  menus/transient/manual.c
/// \brief Manual

#include "../../k_menu.h"

menuitem_t MISC_Manual[] = {
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL00", NULL, NULL, {.routine = M_HandleImageDef}, 0, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL01", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL02", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL03", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL04", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL05", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL06", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL07", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL08", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL09", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL10", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL11", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL12", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL99", NULL, NULL, {.routine = M_HandleImageDef}, 0, 0},
};

menu_t MISC_ManualDef = IMAGEDEF(MISC_Manual);
