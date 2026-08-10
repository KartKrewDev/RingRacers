// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  stun.cpp
/// \brief RFC 5389 client implementation to fetch external IP address.

/* https://tools.ietf.org/html/rfc5389 */

#if defined (__linux__) || defined (__FreeBSD__)
#include <sys/random.h>
#elif defined (_WIN32)
#define _CRT_RAND_S
#include <cstdlib>
#elif defined (__APPLE__)
#include <CommonCrypto/CommonRandom.h>
#else
#error "Need CSPRNG."
#endif

#include <vector>

#include "doomdef.h"
#include "d_clisrv.h"
#include "command.h"
#include "i_net.h"
#include "stun.h"

static std::vector<stun_callback_t> stun_callbacks;

/* 18.4 STUN UDP and TCP Port Numbers */

#define STUN_PORT "3478"

/* 6. STUN Message Structure */

#define BIND_REQUEST  0x0001
#define BIND_RESPONSE 0x0101

static const uint32_t MAGIC_COOKIE = MSBF_LONG (0x2112A442);

static char transaction_id[12];

/* 18.2 STUN Attribute Registry */

#define XOR_MAPPED_ADDRESS 0x0020

/* 15.1 MAPPED-ADDRESS */

#define STUN_IPV4 0x01

static int8_t
STUN_node (void)
{
	int8_t node;

	char * const colon = strchr(cv_stunserver.zstring, ':');

	const char * const host = cv_stunserver.zstring;
	const char * const port = &colon[1];

	I_Assert(I_NetMakeNodewPort != NULL);

	if (colon != NULL)
	{
		*colon = '\0';

		node = I_NetMakeNodewPort(host, port);

		*colon = ':';
	}
	else
	{
		node = I_NetMakeNodewPort(host, STUN_PORT);
	}

	return node;
}

void
csprng
(
		void * const buffer,
		const size_t size
){
#if defined (_WIN32)
	size_t o;

	for (o = 0; o < size; o += sizeof (unsigned int))
	{
		rand_s((unsigned int *)&((char *)buffer)[o]);
	}
#elif defined (__linux__)
	getrandom(buffer, size, 0U);
#elif defined (__APPLE__)
	CCRandomGenerateBytes(buffer, size);
#elif defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__)
	arc4random_buf(buffer, size);
#endif
}

void
STUN_bind (stun_callback_t callback)
{
	/* 6. STUN Message Structure */

	const uint16_t type = MSBF_SHORT (BIND_REQUEST);

	const int8_t node = STUN_node();

	doomcom->remotenode = node;
	doomcom->datalength = 20;

	csprng(transaction_id, 12U);

	memcpy(&doomcom->data[0], &type,           2U);
	memset(&doomcom->data[2], 0,               2U);
	memcpy(&doomcom->data[4], &MAGIC_COOKIE,   4U);
	memcpy(&doomcom->data[8], transaction_id, 12U);

	stun_callbacks.push_back(callback);

	I_NetSend();
	Net_CloseConnection(node);/* will handle response at I_NetGet */
}

static size_t
STUN_xor_mapped_address (const char * const value)
{
	const uint32_t xaddr = *(const uint32_t *)&value[4];
	const uint32_t  addr = xaddr ^ MAGIC_COOKIE;

	for (auto &callback : stun_callbacks)
	{
		callback(addr);
	}

	return 0U;
}

static size_t
align4 (size_t n)
{
	return n + n % 4U;
}

static size_t
STUN_parse_attribute (const char * const attribute)
{
	/* 15. STUN Attributes */
	const uint16_t type   = MSBF_SHORT (*(const uint16_t *)&attribute[0]);
	const uint16_t length = MSBF_SHORT (*(const uint16_t *)&attribute[2]);

	/* 15.2 XOR-MAPPED-ADDRESS */
	if (
			type   == XOR_MAPPED_ADDRESS &&
			length == 8U &&
			(unsigned char)attribute[5] == STUN_IPV4
	){
		return STUN_xor_mapped_address(&attribute[4]);
	}

	return align4(4U + length);
}

dboolean
STUN_got_response
(
		const char * const buffer,
		const size_t       size
){
	const char * const end = &buffer[size];

	const char * p = &buffer[20];

	uint16_t type;
	uint16_t length;

	/*
	Check for STUN response.

	Header is 20 bytes.
	XOR-MAPPED-ADDRESS attribute is required.
	Each attribute has a 2 byte header.
	The XOR-MAPPED-ADDRESS attribute also has a 8 byte value.
	This totals 10 bytes for the attribute.
	*/

	if (size < 30U || stun_callbacks.empty())
	{
		return false;
	}

	/* 6. STUN Message Structure */

	if (
			*(const uint32_t *)&buffer[4] == MAGIC_COOKIE &&
			memcmp(&buffer[8], transaction_id, 12U) == 0
	){
		type   = MSBF_SHORT (*(const uint16_t *)&buffer[0]);
		length = MSBF_SHORT (*(const uint16_t *)&buffer[2]);

		if (
				(type >> 14)    == 0U &&
				(length & 0x02) == 0U &&
				(20U + length)  <= size
		){
			if (type == BIND_RESPONSE)
			{
				do
				{
					length = STUN_parse_attribute(p);

					if (length == 0U)
					{
						break;
					}

					p += length;
				}
				while (p < end) ;
			}

			stun_callbacks = {};

			return true;
		}
	}

	return false;
}
