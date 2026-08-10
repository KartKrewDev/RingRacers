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

#ifndef D_NETFIL_H
#define D_NETFIL_H

#include <time.h>

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
	uint8_t willsend; // Is the server willing to send it?
	char filename[MAX_WADPATH];
	uint8_t md5sum[16];
	filestatus_t status; // The value returned by recsearch
	dboolean justdownloaded; // To prevent late fragments from causing an I_Error

	// Used only for download
	FILE *file;
	dboolean *receivedfragments;
	uint32_t fragmentsize;
	uint8_t iteration;
	fileack_pak *ackpacket;
	uint32_t currentsize;
	uint32_t totalsize;
	uint32_t ackresendposition; // Used when resuming downloads
};

extern int32_t fileneedednum;
extern fileneeded_t fileneeded[MAX_WADFILES];
#define DOWNLOADDIR_PART "downloads"
extern char downloaddir[];

extern int32_t lastfilenum;
extern int32_t downloadcompletednum;
extern uint32_t downloadcompletedsize;
extern int32_t totalfilesrequestednum;
extern uint32_t totalfilesrequestedsize;

#ifdef HAVE_CURL
extern dboolean curl_failedwebdownload;
extern dboolean curl_running;
extern int32_t curl_transfers;

extern struct HTTP_login
{
	char       * url;
	char       * auth;
	HTTP_login * next;
}
*curl_logins;
#endif

uint8_t *PutFileNeeded(uint16_t firstfile);
void D_ParseFileneeded(int32_t fileneedednum_parm, uint8_t *fileneededstr, uint16_t firstfile);
void CL_PrepareDownloadSaveGame(const char *tmpsave);

int32_t CL_CheckFiles(void);
dboolean CL_LoadServerFiles(void);
void AddRamToSendQueue(int32_t node, void *data, size_t size, freemethod_t freemethod,
	uint8_t fileid);

void FileSendTicker(void);
void PT_FileAck(void);
void PT_FileReceived(void);
dboolean SendingFile(int32_t node);

void FileReceiveTicker(void);
void PT_FileFragment(void);

dboolean CL_CheckDownloadable(void);
dboolean CL_SendFileRequest(void);
dboolean PT_RequestFile(int32_t node);

void PT_ClientKey(int32_t node);

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
	int32_t id; // Callback ID
	dboolean ongoing;
	luafiletransfernodestatus_t nodestatus[MAXNETNODES];
	tic_t nodetimeouts[MAXNETNODES];
	luafiletransfer_t *next;
};

extern luafiletransfer_t *luafiletransfers;
extern dboolean waitingforluafiletransfer;
extern dboolean waitingforluafilecommand;
extern char luafiledir[256 + 16];

void AddLuaFileTransfer(const char *filename, const char *mode);
void SV_PrepareSendLuaFile(void);
dboolean AddLuaFileToSendQueue(int32_t node, const char *filename);
void SV_HandleLuaFileSent(uint8_t node);
void RemoveLuaFileTransfer(void);
void RemoveAllLuaFileTransfers(void);
void SV_AbortLuaFileTransfer(int32_t node);
void CL_PrepareDownloadLuaFile(void);
void Got_LuaFile(const uint8_t **cp, int32_t playernum);
void StoreLuaFileCallback(int32_t id);
void RemoveLuaFileCallback(int32_t id);
void MakePathDirs(char *path);

void SV_AbortSendFiles(int32_t node);
void CloseNetFile(void);
void CL_AbortDownloadResume(void);

void Command_Downloads_f(void);

dboolean fileexist(char *filename, time_t ptime);

// Search a file in the wadpath, return FS_FOUND when found
filestatus_t findfile(char *filename, const char *suggestedfolder, const uint8_t *wantedmd5sum,
	dboolean completepath);
filestatus_t checkfilemd5(char *filename, const uint8_t *wantedmd5sum);

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

#endif // D_NETFIL_H
