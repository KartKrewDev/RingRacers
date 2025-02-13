// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2025 by James Robert Roman.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_threads.h
/// \brief Multithreading abstraction

#ifdef HAVE_THREADS

#ifndef I_THREADS_H
#define I_THREADS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*I_thread_fn)(void *userdata);

typedef void * I_mutex;
typedef void * I_cond;

void      I_start_threads (void);
void      I_stop_threads  (void);

void      I_spawn_thread (const char *name, I_thread_fn, void *userdata);

/* check in your thread whether to return early */
int       I_thread_is_stopped (void);

void      I_lock_mutex      (I_mutex *);
void      I_unlock_mutex    (I_mutex);

void      I_hold_cond       (I_cond *, I_mutex);

void      I_wake_one_cond   (I_cond *);
void      I_wake_all_cond   (I_cond *);

#ifdef __cplusplus
} // extern "C"
#endif

#endif/*I_THREADS_H*/
#endif/*HAVE_THREADS*/
