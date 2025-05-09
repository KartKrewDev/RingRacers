// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iterator>

#include "core/static_vec.hpp"
#include "cxxutil.hpp"

#include "d_clisrv.h" // playerconsole
#include "doomdef.h" // MAXPLAYERS
#include "doomstat.h" // consoleplayer
#include "g_game.h" // G_FixCamera
#include "g_party.h"
#include "g_state.h"
#include "p_local.h"
#include "r_fps.h"
#include "r_main.h" // R_ExecuteSetViewSize

namespace
{

using playernum_t = uint8_t;

class Party
{
public:
	class Console
	{
	public:
		// The Console class is basically analogous to
		// a playernum except local splitscreen players only
		// resolve to one playernum.
		//
		// Local splitscreen players are always joined with
		// each other, so this lets just one party to refer to
		// that group.

		Console(playernum_t player)
		{
			SRB2_ASSERT(player >= 0 && player < MAXPLAYERS);

			console_ = playerconsole[player];

			SRB2_ASSERT(console_ >= 0 && console_ < MAXPLAYERS);
		}

		operator playernum_t() const { return console_; }

	private:
		playernum_t console_;
	};

	//
	//        Write Access Methods
	//

	// Add a single player.
	void add(playernum_t player) { vec_.push_back(player); }

	// Add every player from another party.
	void add(const Party& party) { std::copy(party.vec_.begin(), party.vec_.end(), std::back_inserter(vec_)); }

	// Remove every player whose console is the same.
	void remove(Console console)
	{
		auto it = std::remove_if(vec_.begin(), vec_.end(), [console](Console other) { return other == console; });

		while (it < vec_.end())
		{
			vec_.pop_back();
		}
	}

	//
	//        Read Access Methods
	//

	std::size_t size() const { return vec_.size(); }

	// The player at this position in the party.
	playernum_t at(std::size_t i) const { return vec_[i]; }
	playernum_t operator[](std::size_t i) const { return at(i); }

	// C array access to the raw player numbers.
	const playernum_t* data() const { return &vec_[0]; }

	// True if the player is a member of this party.
	bool contains(playernum_t player) const { return std::find(vec_.begin(), vec_.end(), player) != vec_.end(); }

	// True if the consoleplayer is a member of this party.
	bool local() const
	{
		// consoleplayer is not valid yet.
		if (!addedtogame && !demo.playback)
		{
			return false;
		}

		return contains(consoleplayer);
	}

	// Returns a party composed of only the unique consoles
	// from this party.
	Party consoles() const
	{
		Party party;

		std::unique_copy(vec_.begin(), vec_.end(), std::back_inserter(party.vec_), std::equal_to<Console>());

		return party;
	}

	// If the party is local, set the correct viewports.
	void rebuild_displayplayers() const
	{
		if (!local())
		{
			return;
		}

		for (std::size_t i = 0; i < size(); ++i)
		{
			displayplayers[i] = at(i);

			// Camera is not valid outside of levels.
			if (G_GamestateUsesLevel())
			{
				G_FixCamera(1 + i);
			}
		}

		r_splitscreen = size() - 1;

		R_ExecuteSetViewSize(); // present viewport
	}

	//
	//        Iterators
	//

	// Returns an iterator to the player within this party if
	// they are a member. Else returns the end() iterator.
	auto find(playernum_t player) const
	{
		return std::find(vec_.begin(), vec_.end(), player);
	}

	// Iterator to the beginning of the party.
	auto begin() const { return vec_.begin(); }

	// Iterator to the end of the party.
	auto end() const { return vec_.end(); }

private:
	srb2::StaticVec<playernum_t, MAXSPLITSCREENPLAYERS> vec_;
};

class PartyManager
{
public:
	// To avoid copying the same party to each local
	// splitscreen player, all lookups will use the
	// consoleplayer.
	Party& operator [](Party::Console console) { return pool_[console]; }

	// Clears a single player's local party. This method
	// accesses the playernum directly, instead of the
	// consoleplayer.
	void reset(playernum_t player) { pool_[player] = {}; }

protected:
	std::array<Party, MAXPLAYERS> pool_;
}
local_party;

class FinalPartyManager : public PartyManager
{
public:
	// Adds guest's entire local splitscreen party to the
	// host's party. If the operation succeeds, host and guest
	// parties are guaranteed to be identical and the
	// viewports are updated for every player involved.
	bool join(Party::Console host, Party::Console guest)
	{
		Party &party = pool_[host];

		// Already in the same party.
		if (party.contains(guest))
		{
			return false;
		}

		// Parties do not fit when merged.
		if (party.size() + local_party[guest].size() > MAXSPLITSCREENPLAYERS)
		{
			return false;
		}

		// If the host party includes players from a local
		// party, iterating the unique consoles avoids
		// duplicate insertions of the guest.
		for (Party::Console other : party.consoles())
		{
			pool_[other].add(local_party[guest]);
		}

		reset(guest, party); // assign new party to guest

		return true;
	}

	// Removes a player from another party and assigns a new
	// party. Viewports are updated for all players involved.
	void reset(Party::Console player, const Party &party)
	{
		SRB2_ASSERT(party.size() > 0);

		remove(player);

		pool_[player] = party;

		party.rebuild_displayplayers();
	}

private:
	// Removes a player from every party they're in. Updates
	// viewports for the players left behind.
	void remove(Party::Console player)
	{
		Party &party = pool_[player];

		// Iterate a COPY of party because this very party
		// will be modified.
		for (Party::Console member : Party(party))
		{
			pool_[member].remove(player);
		}

		party.rebuild_displayplayers(); // restore viewports for left behind party
	}
}
final_party;

}; // namespace

INT32 splitscreen_invitations[MAXPLAYERS];

void G_ObliterateParties(void)
{
	final_party = {};
	local_party = {};
}

void G_DestroyParty(UINT8 player)
{
	local_party.reset(player);
	final_party[player] = {};
}

void G_BuildLocalSplitscreenParty(UINT8 player)
{
	local_party[player].add(player);
	final_party[player] = local_party[player];
}

void G_JoinParty(UINT8 host, UINT8 guest)
{
	final_party.join(host, guest);
}

void G_LeaveParty(UINT8 player)
{
	final_party.reset(player, local_party[player]);
}

UINT8 G_LocalSplitscreenPartySize(UINT8 player)
{
	return local_party[player].size();
}

UINT8 G_PartySize(UINT8 player)
{
	return final_party[player].size();
}

boolean G_IsPartyLocal(UINT8 player)
{
	return final_party[player].local();
}

UINT8 G_PartyMember(UINT8 player, UINT8 index)
{
	SRB2_ASSERT(index < final_party[player].size());

	return final_party[player][index];
}

const UINT8* G_PartyArray(UINT8 player)
{
	return final_party[player].data();
}

UINT8 G_PartyPosition(UINT8 player)
{
	const Party& party = final_party[player];

	return party.find(player) - party.begin();
}

UINT8 G_LocalSplitscreenPartyPosition(UINT8 player)
{
	const Party& party = local_party[player];

	return party.find(player) - party.begin();
}

UINT8 G_LocalSplitscreenPartyMember(UINT8 player, UINT8 index)
{
	SRB2_ASSERT(index < local_party[player].size());

	return local_party[player][index];
}
