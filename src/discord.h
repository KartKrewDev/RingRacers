// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  discord.h
/// \brief Discord Rich Presence handling

#ifndef __DISCORD__
#define __DISCORD__

#ifdef HAVE_DISCORDRPC

#include <discord_rpc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern consvar_t cv_discordrp;
extern consvar_t cv_discordstreamer;
extern consvar_t cv_discordasks;

extern struct discordInfo_s {
	UINT8 maxPlayers;
	boolean joinsAllowed;
	boolean everyoneCanInvite;
} discordInfo;

struct discordRequest_t {
	char *username; // Discord user name.
#if 0 // Good night, sweet prince...
	char *discriminator; // Discord discriminator (The little hashtag thing after the username). Separated for a "hide discriminators" cvar.
#endif
	char *userID; // The ID of the Discord user, gets used with Discord_Respond()

	// HAHAHA, no.
	// *Maybe* if it was only PNG I would boot up curl just to get AND convert this to Doom GFX,
	// but it can *also* be a JEPG, WebP, or GIF :)
	// Hey, wanna add ImageMagick as a dependency? :dying:
	//patch_t *avatar;

	discordRequest_t *next; // Next request in the list.
	discordRequest_t *prev; // Previous request in the list. Not used normally, but just in case something funky happens, this should repair the list.
};

extern discordRequest_t *discordRequestList;

extern size_t g_discord_skins;

/*--------------------------------------------------
	const char *DRPC_HideUsername(const char *input);

		Handle usernames while cv_discordstreamer is activated.
		(The loss of discriminators is still a dumbass regression
		that I will never forgive the Discord developers for.)
--------------------------------------------------*/

const char *DRPC_HideUsername(const char *input);


/*--------------------------------------------------
	void DRPC_RemoveRequest(void);

		Removes an invite from the list.
--------------------------------------------------*/

void DRPC_RemoveRequest(discordRequest_t *removeRequest);


/*--------------------------------------------------
	void DRPC_Init(void);

		Initalizes Discord Rich Presence by linking the Application ID
		and setting the callback functions.
--------------------------------------------------*/

void DRPC_Init(void);


/*--------------------------------------------------
	void DRPC_UpdatePresence(void);

		Updates what is displayed by Rich Presence on the user's profile.
		Should be called whenever something that is displayed is
		changed in-game.
--------------------------------------------------*/

void DRPC_UpdatePresence(void);


#endif // HAVE_DISCORDRPC

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __DISCORD__
