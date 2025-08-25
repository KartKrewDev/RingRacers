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
/// \file  d_netfil.h
/// \brief File transferring related structs and functions.

#ifndef __D_NETFIL__
#define __D_NETFIL__

#include "d_net.h"
#include "d_clisrv.h"
#include "w_wad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	SF_FILE,
	SF_Z_RAM,
	SF_RAM,
	SF_NOFREERAM
} freemethod_t;

typedef enum
{
	FS_NOTCHECKED,
	FS_NOTFOUND,
	FS_FOUND,
	FS_REQUESTED,
	FS_DOWNLOADING,
	FS_OPEN, // Is opened and used in w_wad
	FS_MD5SUMBAD,
	FS_FALLBACK, // HTTP failed
} filestatus_t;

struct fileneeded_t
{
	UINT8 willsend; // Is the server willing to send it?
	char filename[MAX_WADPATH];
	UINT8 md5sum[16];
	filestatus_t status; // The value returned by recsearch
	boolean justdownloaded; // To prevent late fragments from causing an I_Error

	// Used only for download
	FILE *file;
	boolean *receivedfragments;
	UINT32 fragmentsize;
	UINT8 iteration;
	fileack_pak *ackpacket;
	UINT32 currentsize;
	UINT32 totalsize;
	UINT32 ackresendposition; // Used when resuming downloads
};

extern INT32 fileneedednum;
extern fileneeded_t fileneeded[MAX_WADFILES];
#define DOWNLOADDIR_PART "downloads"
extern char downloaddir[];

extern INT32 lastfilenum;
extern INT32 downloadcompletednum;
extern UINT32 downloadcompletedsize;
extern INT32 totalfilesrequestednum;
extern UINT32 totalfilesrequestedsize;

#ifdef HAVE_CURL
extern boolean curl_failedwebdownload;
extern boolean curl_running;
extern INT32 curl_transfers;

extern struct HTTP_login
{
	char       * url;
	char       * auth;
	HTTP_login * next;
}
*curl_logins;
#endif

UINT8 *PutFileNeeded(UINT16 firstfile);
void D_ParseFileneeded(INT32 fileneedednum_parm, UINT8 *fileneededstr, UINT16 firstfile);
void CL_PrepareDownloadSaveGame(const char *tmpsave);

INT32 CL_CheckFiles(void);
boolean CL_LoadServerFiles(void);
void AddRamToSendQueue(INT32 node, void *data, size_t size, freemethod_t freemethod,
	UINT8 fileid);

void FileSendTicker(void);
void PT_FileAck(void);
void PT_FileReceived(void);
boolean SendingFile(INT32 node);

void FileReceiveTicker(void);
void PT_FileFragment(void);

boolean CL_CheckDownloadable(void);
boolean CL_SendFileRequest(void);
boolean PT_RequestFile(INT32 node);

void PT_ClientKey(INT32 node);

typedef enum
{
	LFTNS_NONE,    // This node is not connected
	LFTNS_WAITING, // This node is waiting for the server to send the file
	LFTNS_ASKED,   // The server has told the node they're ready to send the file
	LFTNS_SENDING, // The server is sending the file to this node
	LFTNS_SENT     // The node already has the file
} luafiletransfernodestatus_t;

struct luafiletransfer_t
{
	char *filename;
	char *realfilename;
	char mode[4]; // rb+/wb+/ab+ + null character
	INT32 id; // Callback ID
	boolean ongoing;
	luafiletransfernodestatus_t nodestatus[MAXNETNODES];
	tic_t nodetimeouts[MAXNETNODES];
	luafiletransfer_t *next;
};

extern luafiletransfer_t *luafiletransfers;
extern boolean waitingforluafiletransfer;
extern boolean waitingforluafilecommand;
extern char luafiledir[256 + 16];

void AddLuaFileTransfer(const char *filename, const char *mode);
void SV_PrepareSendLuaFile(void);
boolean AddLuaFileToSendQueue(INT32 node, const char *filename);
void SV_HandleLuaFileSent(UINT8 node);
void RemoveLuaFileTransfer(void);
void RemoveAllLuaFileTransfers(void);
void SV_AbortLuaFileTransfer(INT32 node);
void CL_PrepareDownloadLuaFile(void);
void Got_LuaFile(const UINT8 **cp, INT32 playernum);
void StoreLuaFileCallback(INT32 id);
void RemoveLuaFileCallback(INT32 id);
void MakePathDirs(char *path);

void SV_AbortSendFiles(INT32 node);
void CloseNetFile(void);
void CL_AbortDownloadResume(void);

void Command_Downloads_f(void);

boolean fileexist(char *filename, time_t ptime);

// Search a file in the wadpath, return FS_FOUND when found
filestatus_t findfile(char *filename, const char *suggestedfolder, const UINT8 *wantedmd5sum,
	boolean completepath);
filestatus_t checkfilemd5(char *filename, const UINT8 *wantedmd5sum);

void nameonly(char *s);
size_t nameonlylength(const char *s);

#ifdef HAVE_CURL
void CURLPrepareFile(const char* url, int dfilenum);
void CURLAbortFile(void);
void CURLGetFile(void);
HTTP_login * CURLGetLogin (const char *url, HTTP_login ***return_prev_next);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __D_NETFIL__
