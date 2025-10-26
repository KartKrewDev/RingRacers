// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_tcp.c
/// \brief TCP driver, socket code.
///        This is not really OS-dependent because all OSes have the same socket API.
///        Just use ifdef for OS-dependent parts.

#include "i_tcp_detail.h"
#include "i_system.h"
#include "i_time.h"
#include "i_net.h"
#include "d_net.h"
#include "d_netfil.h"
#include "m_argv.h"
#include "stun.h"
#include "z_zone.h"

#include "doomstat.h"

// win32 or djgpp
#ifdef USE_WINSOCK
	// winsock stuff (in winsock a socket is not a file)
	#define ioctl ioctlsocket
	#define close closesocket
#endif

#include "i_addrinfo.h"

#define SELECTTEST

#define DEFAULTPORT "5029"

#ifdef USE_WINSOCK
	typedef SOCKET SOCKET_TYPE;
	#define ERRSOCKET (SOCKET_ERROR)
#else
	#if (defined (__unix__) && !defined (MSDOS)) || defined (__APPLE__) || defined (__HAIKU__)
		typedef int SOCKET_TYPE;
	#else
		typedef unsigned long SOCKET_TYPE;
	#endif
	#define ERRSOCKET (-1)
#endif

// define socklen_t in Windows if it is not already defined
#ifdef USE_WINSOCK1
	typedef int socklen_t;
#endif

typedef struct
{
	mysockaddr_t address;
	UINT8 mask;
	char *username;
	char *reason;
	time_t timestamp;
} banned_t;

static SOCKET_TYPE mysockets[MAXNETNODES+1] = {ERRSOCKET};
static size_t mysocketses = 0;
static int myfamily[MAXNETNODES+1] = {0};
static SOCKET_TYPE nodesocket[MAXNETNODES+1] = {ERRSOCKET};
mysockaddr_t clientaddress[MAXNETNODES+1];
static mysockaddr_t broadcastaddress[MAXNETNODES+1];
static size_t broadcastaddresses = 0;
static boolean nodeconnected[MAXNETNODES+1];
static const INT32 hole_punch_magic = MSBF_LONG (0x52eb11);

static bannednode_t SOCK_bannednode[MAXNETNODES+1]; /// \note do we really need the +1?
static boolean init_tcp_driver = false;

static const char *serverport_name = DEFAULTPORT;
static const char *clientport_name;/* any port */

#ifdef USE_WINSOCK
// stupid microsoft makes things complicated
static char *get_WSAErrorStr(int e)
{
	static char buf[256]; // allow up to 255 bytes

	buf[0] = '\0';

	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		(DWORD)e,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)buf,
		sizeof (buf),
		NULL);

	if (!buf[0]) // provide a fallback error message if no message is available for some reason
		sprintf(buf, "Unknown error");

	return buf;
}
#undef strerror
#define strerror get_WSAErrorStr
#endif

#ifdef USE_WINSOCK2
#define inet_ntop inet_ntopA
#define HAVE_NTOP
static const char* inet_ntopA(short af, const void *cp, char *buf, socklen_t len)
{
	DWORD Dlen = len, AFlen = 0;
	SOCKADDR_STORAGE any;
	LPSOCKADDR anyp = (LPSOCKADDR)&any;
	LPSOCKADDR_IN any4 = (LPSOCKADDR_IN)&any;
	LPSOCKADDR_IN6 any6 = (LPSOCKADDR_IN6)&any;

	if (!buf)
	{
		WSASetLastError(STATUS_INVALID_PARAMETER);
		return NULL;
	}

	if (af != AF_INET && af != AF_INET6)
	{
		WSASetLastError(WSAEAFNOSUPPORT);
		return NULL;
	}

	ZeroMemory(&any, sizeof(SOCKADDR_STORAGE));
	any.ss_family = af;

	switch (af)
	{
		case AF_INET:
		{
			CopyMemory(&any4->sin_addr, cp, sizeof(IN_ADDR));
			AFlen = sizeof(SOCKADDR_IN);
			break;
		}
		case AF_INET6:
		{
			CopyMemory(&any6->sin6_addr, cp, sizeof(IN6_ADDR));
			AFlen = sizeof(SOCKADDR_IN6);
			break;
		}
	}

	if (WSAAddressToStringA(anyp, AFlen, NULL, buf, &Dlen) == SOCKET_ERROR)
		return NULL;
	return buf;
}
#elif !defined (USE_WINSOCK1)
#define HAVE_NTOP
#endif

#ifdef HAVE_MINIUPNPC // based on old XChat patch
static struct UPNPUrls urls;
static struct IGDdatas data;
static char lanaddr[64];

static void I_ShutdownUPnP(void)
{
	FreeUPNPUrls(&urls);
}

static inline void I_InitUPnP(void)
{
	struct UPNPDev * devlist = NULL;
	int upnp_error = -2;
	CONS_Printf(M_GetText("Looking for UPnP Internet Gateway Device\n"));
	devlist = upnpDiscover(2000, NULL, NULL, 0, false, &upnp_error);
	if (devlist)
	{
		struct UPNPDev *dev = devlist;
		char * descXML;
		int descXMLsize = 0;
		while (dev)
		{
			if (strstr (dev->st, "InternetGatewayDevice"))
				break;
			dev = dev->pNext;
		}
		if (!dev)
			dev = devlist; /* defaulting to first device */

		CONS_Printf(M_GetText("Found UPnP device:\n desc: %s\n st: %s\n"),
		           dev->descURL, dev->st);

		UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
		CONS_Printf(M_GetText("Local LAN IP address: %s\n"), lanaddr);
		descXML = miniwget(dev->descURL, &descXMLsize);
		if (descXML)
		{
			parserootdesc(descXML, descXMLsize, &data);
			free(descXML);
			descXML = NULL;
			memset(&urls, 0, sizeof(struct UPNPUrls));
			memset(&data, 0, sizeof(struct IGDdatas));
			GetUPNPUrls(&urls, &data, dev->descURL);
			I_AddExitFunc(I_ShutdownUPnP);
		}
		freeUPNPDevlist(devlist);
	}
	else if (upnp_error == UPNPDISCOVER_SOCKET_ERROR)
	{
		CONS_Printf(M_GetText("No UPnP devices discovered\n"));
	}
}

static inline void I_UPnP_add(const char * addr, const char *port, const char * servicetype)
{
	if (addr == NULL)
		addr = lanaddr;
	if (!urls.controlURL || urls.controlURL[0] == '\0')
		return;
	UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
	                    port, port, addr, "SRB2", servicetype, NULL, NULL);
}

static inline void I_UPnP_rem(const char *port, const char * servicetype)
{
	if (!urls.controlURL || urls.controlURL[0] == '\0')
		return;
	UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype,
	                       port, servicetype, NULL);
}
#endif

// I spent 3 hours trying to work out why I couldn't sensibly access this from anywhere,
// even when extern'd. I am not the person who should be doing this. -Tyron, 2023-10-08
mysockaddr_t SOCK_DirectNodeToAddr(UINT8 node)
{
	return clientaddress[node];
}

const char *SOCK_AddrToStr(mysockaddr_t *sk)
{
	static char s[64]; // 255.255.255.255:65535 or IPv6:65535
#ifdef HAVE_NTOP
	void *addr;

	if(sk->any.sa_family == AF_INET)
		addr = &sk->ip4.sin_addr;
#ifdef HAVE_IPV6
	else if(sk->any.sa_family == AF_INET6)
		addr = &sk->ip6.sin6_addr;
#endif
	else
		addr = NULL;

	if(addr == NULL)
		sprintf(s, "No address");
	else if(inet_ntop(sk->any.sa_family, addr, s, sizeof (s)) == NULL)
		sprintf(s, "Unknown family type, error #%u", errno);
#ifdef HAVE_IPV6
	else if(sk->any.sa_family == AF_INET6 && sk->ip6.sin6_port != 0)
		strcat(s, va(":%d", ntohs(sk->ip6.sin6_port)));
#endif
	else if(sk->any.sa_family == AF_INET  && sk->ip4.sin_port  != 0)
		strcat(s, va(":%d", ntohs(sk->ip4.sin_port)));
#else
	if (sk->any.sa_family == AF_INET)
	{
		strcpy(s, inet_ntoa(sk->ip4.sin_addr));
		if (sk->ip4.sin_port != 0) strcat(s, va(":%d", ntohs(sk->ip4.sin_port)));
	}
	else
		sprintf(s, "Unknown type");
#endif
	return s;
}

static const char *SOCK_GetNodeAddress(INT32 node)
{
	if (node == 0)
		return "self";
	if (!nodeconnected[node])
		return NULL;
	return SOCK_AddrToStr(&clientaddress[node]);
}

static UINT32 SOCK_GetNodeAddressInt(INT32 node)
{
    if (nodeconnected[node] && clientaddress[node].any.sa_family == AF_INET)
    {
        return clientaddress[node].ip4.sin_addr.s_addr;
    }
    else
    {
        I_Error("SOCK_GetNodeAddressInt: Node %d is not IPv4!\n", node);
    }

    return 0;
}

boolean SOCK_cmpaddr(mysockaddr_t *a, mysockaddr_t *b, UINT8 mask)
{
	UINT32 bitmask = INADDR_NONE;

	if (mask && mask < 32)
		bitmask = htonl((UINT32)(-1) << (32 - mask));

	if (b->any.sa_family == AF_INET)
		return (a->ip4.sin_addr.s_addr & bitmask) == (b->ip4.sin_addr.s_addr & bitmask)
			&& (b->ip4.sin_port == 0 || (a->ip4.sin_port == b->ip4.sin_port));
#ifdef HAVE_IPV6
	else if (b->any.sa_family == AF_INET6)
		return memcmp(&a->ip6.sin6_addr, &b->ip6.sin6_addr, sizeof(b->ip6.sin6_addr))
			&& (b->ip6.sin6_port == 0 || (a->ip6.sin6_port == b->ip6.sin6_port));
#endif
	else
		return false;
}

// This is a hack. For some reason, nodes aren't being freed properly.
// This goes through and cleans up what nodes were supposed to be freed.
/** \warning This function causes the file downloading to stop if someone joins.
  *          How? Because it removes nodes that are connected but not in game,
  *          which is exactly what clients downloading a file are.
  */
static void cleanupnodes(void)
{
	SINT8 j;

	if (!Playing())
		return;

	// Why can't I start at zero?
	for (j = 1; j < MAXNETNODES; j++)
		if (!(nodeingame[j] || nodeneedsauth[j] || SendingFile(j)))
			nodeconnected[j] = false;
}

static SINT8 getfreenode(void)
{
	SINT8 j;

	cleanupnodes();

	for (j = 0; j < MAXNETNODES; j++)
		if (!nodeconnected[j])
		{
			nodeconnected[j] = true;
			return j;
		}

	/** \warning No free node? Just in case a node might not have been freed properly,
	  *          look if there are connected nodes that aren't in game, and forget them.
	  *          It's dirty, and might result in a poor guy having to restart
	  *          downloading a needed wad, but it's better than not letting anyone join...
	  */
	/*I_Error("No more free nodes!!1!11!11!!1111\n");
	for (j = 1; j < MAXNETNODES; j++)
		if (!nodeingame[j])
			return j;*/

	return -1;
}

void Command_Numnodes(void)
{
	INT32 connected = 0;
	INT32 ingame = 0;
	INT32 i;

	for (i = 1; i < MAXNETNODES; i++)
	{
		if (!(nodeconnected[i] || nodeingame[i]))
			continue;

		if (nodeconnected[i])
			connected++;
		if (nodeingame[i])
			ingame++;

		CONS_Printf("%2d - ", i);
		if (nodetoplayer[i] != -1)
			CONS_Printf("player %.2d", nodetoplayer[i]);
		else
			CONS_Printf("         ");
		if (nodeconnected[i])
			CONS_Printf(" - connected");
		else
			CONS_Printf(" -          ");
		if (nodeingame[i])
			CONS_Printf(" - ingame");
		else
			CONS_Printf(" -       ");
		CONS_Printf(" - %s\n", I_GetNodeAddress(i));
	}

	CONS_Printf("\n"
				"Connected: %d\n"
				"Ingame:    %d\n",
				connected, ingame);
}

static boolean hole_punch(ptrdiff_t c)
{
	if (c == 10 && holepunchpacket->magic == hole_punch_magic)
	{
		mysockaddr_t addr;
		addr.ip4.sin_family      = AF_INET;
		addr.ip4.sin_addr.s_addr = holepunchpacket->addr;
		addr.ip4.sin_port        = holepunchpacket->port;
		sendto(mysockets[0], NULL, 0, 0, &addr.any, sizeof addr.ip4);

		CONS_Debug(DBG_NETPLAY,
				"hole punching request from %s\n", SOCK_AddrToStr(&addr));

		return true;
	}
	else
	{
		return false;
	}
}

// Returns true if a packet was received from a new node, false in all other cases
static boolean SOCK_Get(void)
{
	size_t n;
	int j;
	ptrdiff_t c;
	mysockaddr_t fromaddress;
	socklen_t fromlen;

	for (n = 0; n < mysocketses; n++)
	{
		fromlen = (socklen_t)sizeof(fromaddress);
		c = recvfrom(mysockets[n], (char *)&doomcom->data, MAXPACKETLENGTH, 0,
			(void *)&fromaddress, &fromlen);
		if (c > 0)
		{
#ifdef USE_STUN
			if (STUN_got_response(doomcom->data, c))
			{
				break;
			}
#endif

			if (hole_punch(c))
			{
				break;
			}

			// find remote node number
			for (j = 1; j <= MAXNETNODES; j++) //include LAN
			{
				if (SOCK_cmpaddr(&fromaddress, &clientaddress[j], 0))
				{
					doomcom->remotenode = (INT16)j; // good packet from a game player
					doomcom->datalength = (INT16)c;
					nodesocket[j] = mysockets[n];
					return false;
				}
			}
			// not found

			// find a free slot
			j = getfreenode();
			if (j > 0)
			{
				M_Memcpy(&clientaddress[j], &fromaddress, fromlen);
				nodesocket[j] = mysockets[n];
				DEBFILE(va("New node detected: node:%d address:%s\n", j,
						SOCK_GetNodeAddress(j)));
				doomcom->remotenode = (INT16)j; // good packet from a game player
				doomcom->datalength = (INT16)c;

				return true;
			}
			else
				DEBFILE("New node detected: No more free slots\n");
		}
	}

	doomcom->remotenode = -1; // no packet
	return false;
}

// check if we can send (do not go over the buffer)

static fd_set masterset;

#ifdef SELECTTEST
static boolean FD_CPY(fd_set *src, fd_set *dst, SOCKET_TYPE *fd, size_t len)
{
	size_t i;
	boolean testset = false;
	FD_ZERO(dst);
	for (i = 0; i < len;i++)
	{
		if(fd[i] != (SOCKET_TYPE)ERRSOCKET &&
		   FD_ISSET(fd[i], src) && !FD_ISSET(fd[i], dst)) // no checking for dups
		{
			FD_SET(fd[i], dst);
			testset = true;
		}
	}
	return testset;
}

static boolean SOCK_CanSend(void)
{
	struct timeval timeval_for_select = {0, 0};
	fd_set tset;
	int wselect;

	if(!FD_CPY(&masterset, &tset, mysockets, mysocketses))
		return false;
	wselect = select(255, NULL, &tset, NULL, &timeval_for_select);
	if (wselect >= 1)
		return true;
	return false;
}

static boolean SOCK_CanGet(void)
{
	struct timeval timeval_for_select = {0, 0};
	fd_set tset;
	int rselect;

	if(!FD_CPY(&masterset, &tset, mysockets, mysocketses))
		return false;
	rselect = select(255, &tset, NULL, NULL, &timeval_for_select);
	if (rselect >= 1)
		return true;
	return false;
}
#endif

static inline ptrdiff_t SOCK_SendToAddr(SOCKET_TYPE socket, mysockaddr_t* sockaddr)
{
	socklen_t d4 = (socklen_t)sizeof(struct sockaddr_in);
#ifdef HAVE_IPV6
	socklen_t d6 = (socklen_t)sizeof(struct sockaddr_in6);
#endif
	socklen_t d, da = (socklen_t)sizeof(mysockaddr_t);

	switch (sockaddr->any.sa_family)
	{
		case AF_INET:  d = d4; break;
#ifdef HAVE_IPV6
		case AF_INET6: d = d6; break;
#endif
		default:       d = da; break;
	}

	return sendto(socket, (char *)&doomcom->data, doomcom->datalength, 0, &sockaddr->any, d);
}

static void SOCK_Send(void)
{
	ptrdiff_t c = ERRSOCKET;
	size_t i, j;

	if (!nodeconnected[doomcom->remotenode])
		return;

	if (doomcom->remotenode == BROADCASTADDR)
	{
		for (i = 0; i < mysocketses; i++)
		{
			for (j = 0; j < broadcastaddresses; j++)
			{
				if (myfamily[i] == broadcastaddress[j].any.sa_family)
					SOCK_SendToAddr(mysockets[i], &broadcastaddress[j]);
			}
		}
		return;
	}
	else if (nodesocket[doomcom->remotenode] == (SOCKET_TYPE)ERRSOCKET)
	{
		for (i = 0; i < mysocketses; i++)
		{
			if (myfamily[i] == clientaddress[doomcom->remotenode].any.sa_family)
				SOCK_SendToAddr(mysockets[i], &clientaddress[doomcom->remotenode]);
		}
		return;
	}
	else
	{
		c = SOCK_SendToAddr(nodesocket[doomcom->remotenode], &clientaddress[doomcom->remotenode]);
	}

	if (c == ERRSOCKET)
	{
		int e = errno; // save error code so it can't be modified later
		if (e != ECONNREFUSED && e != EWOULDBLOCK)
			I_Error("SOCK_Send, error sending to node %d (%s) #%u: %s", doomcom->remotenode,
				SOCK_GetNodeAddress(doomcom->remotenode), e, strerror(e));
	}
}

static void SOCK_FreeNodenum(INT32 numnode)
{
	// can't disconnect from self :)
	if (!numnode || numnode > MAXNETNODES)
		return;

	DEBFILE(va("Free node %d (%s)\n", numnode, SOCK_GetNodeAddress(numnode)));

	nodeconnected[numnode] = false;
	nodesocket[numnode] = ERRSOCKET;

	// put invalid address
	memset(&clientaddress[numnode], 0, sizeof (clientaddress[numnode]));
}

//
// UDPsocket
//

// allocate a socket
static SOCKET_TYPE UDP_Bind(int family, struct sockaddr *addr, socklen_t addrlen)
{
	SOCKET_TYPE s = socket(family, SOCK_DGRAM, IPPROTO_UDP);
	int opt;
	socklen_t opts;
#ifdef FIONBIO
	unsigned long trueval = true;
#endif
	mysockaddr_t straddr;
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);

	if (s == (SOCKET_TYPE)ERRSOCKET)
		return (SOCKET_TYPE)ERRSOCKET;
#ifdef USE_WINSOCK
	{ // Alam_GBC: disable the new UDP connection reset behavior for Win2k and up
#ifdef USE_WINSOCK2
		DWORD dwBytesReturned = 0;
		BOOL bfalse = FALSE;
		WSAIoctl(s, SIO_UDP_CONNRESET, &bfalse, sizeof(bfalse),
		         NULL, 0, &dwBytesReturned, NULL, NULL);
#else
		unsigned long falseval = false;
		ioctl(s, SIO_UDP_CONNRESET, &falseval);
#endif
	}
#endif

	straddr.any = *addr;
	I_OutputMsg("Binding to %s\n", SOCK_AddrToStr(&straddr));

	if (family == AF_INET)
	{
		mysockaddr_t tmpaddr;
		tmpaddr.any = *addr ;
		if (tmpaddr.ip4.sin_addr.s_addr == htonl(INADDR_ANY))
		{
			opt = true;
			opts = (socklen_t)sizeof(opt);
			setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, opts);
		}
		// make it broadcastable
		opt = true;
		opts = (socklen_t)sizeof(opt);
		if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&opt, opts))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Could not get broadcast rights\n")); // I do not care anymore
		}
	}
#ifdef HAVE_IPV6
	else if (family == AF_INET6)
	{
		if (memcmp(addr, &in6addr_any, sizeof(in6addr_any)) == 0) //IN6_ARE_ADDR_EQUAL
		{
			opt = true;
			opts = (socklen_t)sizeof(opt);
			setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, opts);
		}
#ifdef IPV6_V6ONLY
		// make it IPv6 ony
		opt = true;
		opts = (socklen_t)sizeof(opt);
		if (setsockopt(s, SOL_SOCKET, IPV6_V6ONLY, (char *)&opt, opts))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Could not limit IPv6 bind\n")); // I do not care anymore
		}
#endif
	}
#endif

	if (bind(s, addr, addrlen) == ERRSOCKET)
	{
		close(s);
		I_OutputMsg("Binding failed\n");
		return (SOCKET_TYPE)ERRSOCKET;
	}

#ifdef FIONBIO
	// make it non blocking
	opt = true;
	if (ioctl(s, FIONBIO, &trueval) != 0)
	{
		close(s);
		I_OutputMsg("Seting FIOBIO on failed\n");
		return (SOCKET_TYPE)ERRSOCKET;
	}
#endif

	opts = (socklen_t)sizeof(opt);
	getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&opt, &opts);
	CONS_Printf(M_GetText("Network system buffer: %dKb\n"), opt>>10);

	if (opt < 64<<10) // 64k
	{
		opt = 64<<10;
		opts = (socklen_t)sizeof(opt);
		setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&opt, opts);
		getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&opt, &opts);
		if (opt < 64<<10)
			CONS_Alert(CONS_WARNING, M_GetText("Can't set buffer length to 64k, file transfer will be bad\n"));
		else
			CONS_Printf(M_GetText("Network system buffer set to: %dKb\n"), opt>>10);
	}

	if (getsockname(s, (struct sockaddr *)&sin, &len) == -1)
		CONS_Alert(CONS_WARNING, M_GetText("Failed to get port number\n"));
	else
		current_port = (UINT16)ntohs(sin.sin_port);

	return s;
}

static boolean UDP_Socket(void)
{
	size_t s;
	struct my_addrinfo *ai, *runp, hints;
	int gaie;
#ifdef HAVE_IPV6
	const INT32 b_ipv6 = M_CheckParm("-ipv6");
#endif
	const char *serv;


	for (s = 0; s < mysocketses; s++)
		mysockets[s] = ERRSOCKET;
	for (s = 0; s < MAXNETNODES+1; s++)
		nodesocket[s] = ERRSOCKET;
	FD_ZERO(&masterset);
	s = 0;

	memset(&hints, 0x00, sizeof (hints));
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (serverrunning)
		serv = serverport_name;
	else
		serv = clientport_name;

	if (M_CheckParm("-bindaddr"))
	{
		while (M_IsNextParm())
		{
			gaie = I_getaddrinfo(M_GetNextParm(), serv, &hints, &ai);
			if (gaie == 0)
			{
				runp = ai;
				while (runp != NULL && s < MAXNETNODES+1)
				{
					mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
					if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
					{
						FD_SET(mysockets[s], &masterset);
						myfamily[s] = hints.ai_family;
						s++;
					}
					runp = runp->ai_next;
				}
				I_freeaddrinfo(ai);
			}
		}
	}
	else
	{
		gaie = I_getaddrinfo("0.0.0.0", serv, &hints, &ai);
		if (gaie == 0)
		{
			runp = ai;
			while (runp != NULL && s < MAXNETNODES+1)
			{
				mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
				if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
				{
					FD_SET(mysockets[s], &masterset);
					myfamily[s] = hints.ai_family;
					s++;
#ifdef HAVE_MINIUPNPC
					if (UPNP_support)
					{
						I_UPnP_rem(serverport_name, "UDP");
						I_UPnP_add(NULL, serverport_name, "UDP");
					}
#endif
				}
				runp = runp->ai_next;
			}
			I_freeaddrinfo(ai);
		}
	}
#ifdef HAVE_IPV6
	if (b_ipv6)
	{
		hints.ai_family = AF_INET6;
		if (M_CheckParm("-bindaddr6"))
		{
			while (M_IsNextParm())
			{
				gaie = I_getaddrinfo(M_GetNextParm(), serv, &hints, &ai);
				if (gaie == 0)
				{
					runp = ai;
					while (runp != NULL && s < MAXNETNODES+1)
					{
						mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
						if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
						{
							FD_SET(mysockets[s], &masterset);
							myfamily[s] = hints.ai_family;
							s++;
						}
						runp = runp->ai_next;
					}
					I_freeaddrinfo(ai);
				}
			}
		}
		else
		{
			gaie = I_getaddrinfo("::", serv, &hints, &ai);
			if (gaie == 0)
			{
				runp = ai;
				while (runp != NULL && s < MAXNETNODES+1)
				{
					mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
					if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
					{
						FD_SET(mysockets[s], &masterset);
						myfamily[s] = hints.ai_family;
						s++;
					}
					runp = runp->ai_next;
				}
				I_freeaddrinfo(ai);
			}
		}
	}
#endif

	mysocketses = s;
	if (s == 0) // no sockets?
		return false;

	s = 0;

	// ip + udp
	packetheaderlength = 20 + 8; // for stats

	hints.ai_family = AF_INET;
	gaie = I_getaddrinfo("127.0.0.1", "0", &hints, &ai);
	if (gaie == 0)
	{
		runp = ai;
		while (runp != NULL && s < MAXNETNODES+1)
		{
			memcpy(&clientaddress[s], runp->ai_addr, runp->ai_addrlen);
			s++;
			runp = runp->ai_next;
		}
		I_freeaddrinfo(ai);
	}
	else
	{
		clientaddress[s].any.sa_family = AF_INET;
		clientaddress[s].ip4.sin_port = htons(0);
		clientaddress[s].ip4.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //GetLocalAddress(); // my own ip
		s++;
	}

	s = 0;

	// setup broadcast adress to BROADCASTADDR entry
	gaie = I_getaddrinfo("255.255.255.255", DEFAULTPORT, &hints, &ai);
	if (gaie == 0)
	{
		runp = ai;
		while (runp != NULL && s < MAXNETNODES+1)
		{
			memcpy(&broadcastaddress[s], runp->ai_addr, runp->ai_addrlen);
			s++;
			runp = runp->ai_next;
		}
		I_freeaddrinfo(ai);
	}
	else
	{
		broadcastaddress[s].any.sa_family = AF_INET;
		broadcastaddress[s].ip4.sin_port = htons(atoi(DEFAULTPORT));
		broadcastaddress[s].ip4.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		s++;
	}
#ifdef HAVE_IPV6
	if (b_ipv6)
	{
		hints.ai_family = AF_INET6;
		gaie = I_getaddrinfo("ff02::1", DEFAULTPORT, &hints, &ai);
		if (gaie == 0)
		{
			runp = ai;
			while (runp != NULL && s < MAXNETNODES+1)
			{
				memcpy(&broadcastaddress[s], runp->ai_addr, runp->ai_addrlen);
				s++;
				runp = runp->ai_next;
			}
			I_freeaddrinfo(ai);
		}
	}
#endif

	broadcastaddresses = s;

	doomcom->extratics = 1; // internet is very high ping

	return true;
}

boolean I_InitTcpDriver(void)
{
	boolean tcp_was_up = init_tcp_driver;
	if (!init_tcp_driver)
	{
#ifdef USE_WINSOCK
#ifdef USE_WINSOCK2
		const WORD VerNeed = MAKEWORD(2,2);
#else
		const WORD VerNeed = MAKEWORD(1,1);
#endif
		WSADATA WSAData;
		const int WSAresult = WSAStartup(VerNeed, &WSAData);
		if (WSAresult != 0)
		{
			LPCSTR WSError = NULL;
			switch (WSAresult)
			{
				case WSASYSNOTREADY:
					WSError = "The underlying network subsystem is not ready for network communication";
					break;
				case WSAEINPROGRESS:
					WSError = "A blocking Windows Sockets 1.1 operation is in progress";
					break;
				case WSAEPROCLIM:
					WSError = "Limit on the number of tasks supported by the Windows Sockets implementation has been reached";
					break;
				case WSAEFAULT:
					WSError = "WSAData is not a valid pointer? What kind of setup do you have?";
					break;
				default:
					WSError = va("Error code %u",WSAresult);
					break;
			}
			if (WSAresult != WSAVERNOTSUPPORTED)
				CONS_Debug(DBG_NETPLAY, "WinSock(TCP/IP) error: %s\n",WSError);
		}
#ifdef USE_WINSOCK2
		if(LOBYTE(WSAData.wVersion) != 2 ||
			HIBYTE(WSAData.wVersion) != 2)
		{
			WSACleanup();
			CONS_Debug(DBG_NETPLAY, "No WinSock(TCP/IP) 2.2 driver detected\n");
		}
#else
		if (LOBYTE(WSAData.wVersion) != 1 ||
			HIBYTE(WSAData.wVersion) != 1)
		{
			WSACleanup();
			CONS_Debug(DBG_NETPLAY, "No WinSock(TCP/IP) 1.1 driver detected\n");
		}
#endif
		CONS_Debug(DBG_NETPLAY, "WinSock description: %s\n",WSAData.szDescription);
		CONS_Debug(DBG_NETPLAY, "WinSock System Status: %s\n",WSAData.szSystemStatus);
#endif
		init_tcp_driver = true;
	}
	if (!tcp_was_up && init_tcp_driver)
	{
		I_AddExitFunc(I_ShutdownTcpDriver);
#ifdef HAVE_MINIUPNPC
		if (M_CheckParm("-useUPnP"))
			I_InitUPnP();
		else
			UPNP_support = false;
#endif
	}
	return init_tcp_driver;
}

static void SOCK_CloseSocket(void)
{
	size_t i;
	for (i=0; i < MAXNETNODES+1; i++)
	{
		if (mysockets[i] != (SOCKET_TYPE)ERRSOCKET
		 && FD_ISSET(mysockets[i], &masterset))
		{
			FD_CLR(mysockets[i], &masterset);
			close(mysockets[i]);
		}
		mysockets[i] = ERRSOCKET;
	}
}

void I_ShutdownTcpDriver(void)
{
	SOCK_CloseSocket();

	CONS_Printf("I_ShutdownTcpDriver: ");
#ifdef USE_WINSOCK
	WS_addrinfocleanup();
	WSACleanup();
#endif
	CONS_Printf("shut down\n");
	init_tcp_driver = false;
}

static boolean SOCK_GetAddr(struct sockaddr_in *sin, const char *address, const char *port, boolean test)
{
	struct my_addrinfo *ai = NULL, *runp, hints;
	int gaie;

	if (!port || !port[0])
		port = DEFAULTPORT;

	memset (&hints, 0x00, sizeof (hints));
	hints.ai_flags = 0;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	gaie = I_getaddrinfo(address, port, &hints, &ai);

	if (gaie != 0)
	{
		I_freeaddrinfo(ai);
		return false;
	}

	runp = ai;

	if (test)
	{
		while (runp != NULL)
		{
			if (sendto(mysockets[0], NULL, 0, 0, runp->ai_addr, runp->ai_addrlen) == 0)
				break;

			runp = runp->ai_next;
		}
	}

	if (runp != NULL)
		memcpy(sin, runp->ai_addr, runp->ai_addrlen);

	I_freeaddrinfo(ai);

	return (runp != NULL);
}

static SINT8 SOCK_NetMakeNodewPort(const char *address, const char *port)
{
	SINT8 newnode = getfreenode();

	DEBFILE(va("Creating new node: %s@%s\n", address, port));

	if (newnode != -1)
	{
		if (!SOCK_GetAddr(&clientaddress[newnode].ip4, address, port, true))
		{
			nodeconnected[newnode] = false;
			return -1;
		}
	}

	return newnode;
}

static void rendezvous(int size)
{
	char *addrs = strdup(cv_rendezvousserver.string);

	char *host = strtok(addrs, ":");
	char *port = strtok(NULL,  ":");

	static mysockaddr_t rzv;
	static tic_t refreshtic = (tic_t)-1;

	tic_t tic = I_GetTime();

	if (tic != refreshtic)
	{
		if (SOCK_GetAddr(&rzv.ip4, host, (port ? port : "7777"), false))
		{
			refreshtic = tic;
		}
		else
		{
			CONS_Alert(CONS_ERROR, "Failed to contact rendezvous server (%s).\n",
					cv_rendezvousserver.string);
		}
	}

	if (tic == refreshtic)
	{
		holepunchpacket->magic = hole_punch_magic;
		sendto(mysockets[0], doomcom->data, size, 0, &rzv.any, sizeof rzv.ip4);
	}

	free(addrs);
}

static void SOCK_RequestHolePunch(INT32 node)
{
	mysockaddr_t * addr = &clientaddress[node];

	holepunchpacket->addr = addr->ip4.sin_addr.s_addr;
	holepunchpacket->port = addr->ip4.sin_port;

	CONS_Debug(DBG_NETPLAY,
			"requesting hole punch to node %s\n", SOCK_AddrToStr(addr));

	rendezvous(10);
}

static void SOCK_RegisterHolePunch(void)
{
	rendezvous(4);
}

static boolean SOCK_OpenSocket(void)
{
	size_t i;

	memset(clientaddress, 0, sizeof (clientaddress));

	nodeconnected[0] = true; // always connected to self
	for (i = 1; i < MAXNETNODES; i++)
		nodeconnected[i] = false;
	nodeconnected[BROADCASTADDR] = true;
	I_NetSend = SOCK_Send;
	I_NetGet = SOCK_Get;
	I_NetCloseSocket = SOCK_CloseSocket;
	I_NetFreeNodenum = SOCK_FreeNodenum;
	I_NetMakeNodewPort = SOCK_NetMakeNodewPort;

#ifdef SELECTTEST
	// seem like not work with libsocket : (
	I_NetCanSend = SOCK_CanSend;
	I_NetCanGet = SOCK_CanGet;
#endif

	I_NetRequestHolePunch = SOCK_RequestHolePunch;
	I_NetRegisterHolePunch = SOCK_RegisterHolePunch;

	// build the socket but close it first
	SOCK_CloseSocket();
	return UDP_Socket();
}

// https://github.com/jameds/holepunch/blob/master/holepunch.c#L75
static int SOCK_IsExternalAddress (const void *p)
{
	const int a = ((const unsigned char*)p)[0];
	const int b = ((const unsigned char*)p)[1];

	if (*(const UINT32*)p == (UINT32)~0)/* 255.255.255.255 */
		return 0;

	switch (a)
	{
		case 0:
		case 10:
		case 127:
			return 0;
		case 172:
			return (b & ~15) != 16;/* 16 - 31 */
		case 192:
			return b != 168;
		default:
			return 1;
	}
}


boolean I_InitTcpNetwork(void)
{
	char serverhostname[255];
	const char *urlparam = NULL;
	boolean ret = false;
	// initilize the OS's TCP/IP stack
	if (!I_InitTcpDriver())
		return false;

	if (M_CheckParm("-port") || M_CheckParm("-serverport"))
	// Combined -udpport and -clientport into -port
	// As it was really redundant having two seperate parms that does the same thing
	/* Sorry Steel, I'm adding these back. But -udpport is a stupid name. */
	{
		/*
		If it's NULL, that's okay! Because then
		we'll get a random port from getaddrinfo.
		*/
		serverport_name = M_GetNextParm();
	}
	if (M_CheckParm("-clientport"))
		clientport_name = M_GetNextParm();

	// parse network game options,
	if (M_CheckParm("-server") || dedicated)
	{
		server = true;
		connectedtodedicated = dedicated;

		// If a number of clients (i.e. nodes) is specified, the server will wait for the clients
		// to connect before starting.
		// If no number is specified here, the server starts with 1 client, and others can join
		// in-game.
		// Since Boris has implemented join in-game, there is no actual need for specifying a
		// particular number here.
		// FIXME: for dedicated server, numnodes needs to be set to 0 upon start
		if (dedicated)
			doomcom->numnodes = 0;
/*		else if (M_IsNextParm())
			doomcom->numnodes = (INT16)atoi(M_GetNextParm());*/
		else
			doomcom->numnodes = 1;

		if (doomcom->numnodes < 0)
			doomcom->numnodes = 0;
		if (doomcom->numnodes > MAXNETNODES)
			doomcom->numnodes = MAXNETNODES;

		// server
		servernode = 0;
		// FIXME:
		// ??? and now ?
		// server on a big modem ??? 4*isdn
		net_bandwidth = 16000;
		hardware_MAXPACKETLENGTH = INETPACKETLENGTH;

		ret = true;
	}
	else if ((urlparam = M_GetUrlProtocolArg()) != NULL || M_CheckParm("-connect"))
	{
		if (urlparam != NULL)
			strlcpy(serverhostname, urlparam, sizeof(serverhostname));
		else if (M_IsNextParm())
			strlcpy(serverhostname, M_GetNextParm(), sizeof(serverhostname));
		else
			serverhostname[0] = 0; // assuming server in the LAN, use broadcast to detect it

		// server address only in ip
		if (serverhostname[0])
		{
			COM_BufAddText("connect \"");
			COM_BufAddText(serverhostname);
			COM_BufAddText("\"\n");

			// probably modem
			hardware_MAXPACKETLENGTH = INETPACKETLENGTH;
		}
		else
		{
			// so we're on a LAN
			COM_BufAddText("connect any\n");

			net_bandwidth = 800000;
			hardware_MAXPACKETLENGTH = MAXPACKETLENGTH;
		}
	}

	I_NetOpenSocket = SOCK_OpenSocket;
	I_GetNodeAddress = SOCK_GetNodeAddress;
	I_GetNodeAddressInt = SOCK_GetNodeAddressInt;
	I_IsExternalAddress = SOCK_IsExternalAddress;

	bannednode = SOCK_bannednode;

	return ret;
}

#include "i_addrinfo.c"
