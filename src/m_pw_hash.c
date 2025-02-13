// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2025 by James Robert Roman.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "modp_b64/modp_b64.h"
#include "monocypher/monocypher.h"

#include "doomdef.h"
#include "m_pw_hash.h"

UINT8 *M_HashPassword(UINT8 hash[M_PW_HASH_SIZE], const char *key, const UINT8 salt[M_PW_SALT_SIZE])
{
	size_t memory = 8*1024*1024;
	void *int_buf = malloc(memory);
	if (!int_buf)
		return NULL;

	// https://monocypher.org/manual/argon2
	crypto_argon2_config config = {
		CRYPTO_ARGON2_ID, // algorithm
		memory / 1024, // nb_blocks
		2, // nb_passes
		1, // nb_lanes
	};

	crypto_argon2_inputs inputs = {
		(const UINT8*)key, // pass
		salt, // salt
		strlen(key), // pass_size
		M_PW_SALT_SIZE, // salt_size
	};

	crypto_argon2_extras extras = {
		NULL, // key
		NULL, // ad
		0, // key_size
		0, // ad_size
	};

	crypto_argon2(hash, M_PW_HASH_SIZE, int_buf, config, inputs, extras);
	free(int_buf);
	return hash;
}
