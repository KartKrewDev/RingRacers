// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
// Copyright (C) 2020 by Sonic Team Junior
// Copyright (C) 2003 by James Haley
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_queue.h
/// \brief General queue code

#ifndef M_QUEUE_H
#define M_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

struct mqueueitem_t
{
	mqueueitem_t *next;
};

struct mqueue_t
{
	mqueueitem_t head;
	mqueueitem_t *tail;
	mqueueitem_t *rover;
};

void M_QueueInit(mqueue_t *queue);
void M_QueueInsert(mqueueitem_t *item, mqueue_t *queue);
mqueueitem_t *M_QueueIterator(mqueue_t *queue);
void M_QueueResetIterator(mqueue_t *queue);
void M_QueueFree(mqueue_t *queue);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

// EOF
