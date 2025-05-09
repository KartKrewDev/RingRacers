// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2020 by Spaddlewit Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2.
*/

#ifndef _U_LIST_H_
#define _U_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct listitem_s
{
	struct listitem_s *next;
	struct listitem_s *prev;
} listitem_t;

void ListAdd(void *pItem, listitem_t **itemHead);
void ListAddFront(void *pItem, listitem_t **itemHead);
void ListAddBefore(void *pItem, void *pSpot, listitem_t **itemHead);
void ListAddAfter(void *pItem, void *pSpot, listitem_t **itemHead);
void ListRemove(void *pItem, listitem_t **itemHead);
void ListRemoveAll(listitem_t **itemHead);
void ListRemoveNoFree(void *pItem, listitem_t **itemHead);
unsigned int ListGetCount(void *itemHead);
listitem_t *ListGetByIndex(void *itemHead, unsigned int index);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
