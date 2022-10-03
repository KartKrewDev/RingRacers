//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// PrintBuf class.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__CAPI__PrintBuf_H__
#define ACSVM__CAPI__PrintBuf_H__

#include "Types.h"

#ifdef __cplusplus
#include "../ACSVM/PrintBuf.hpp"
#endif


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

ACSVM_PrintBuf *ACSVM_AllocPrintBuf(void);
void ACSVM_FreePrintBuf(ACSVM_PrintBuf *buf);

void ACSVM_PrintBuf_Clear(ACSVM_PrintBuf *buf);

void ACSVM_PrintBuf_Drop(ACSVM_PrintBuf *buf);

void ACSVM_PrintBuf_Format(ACSVM_PrintBuf *buf, char const *fmt, ...);
void ACSVM_PrintBuf_FormatV(ACSVM_PrintBuf *buf, char const *fmt, va_list arg);

char *ACSVM_PrintBuf_GetBuf(ACSVM_PrintBuf *buf, size_t count);

char const *ACSVM_PrintBuf_GetData(ACSVM_PrintBuf const *buf);
char const *ACSVM_PrintBuf_GetDataFull(ACSVM_PrintBuf const *buf);

char *ACSVM_PrintBuf_GetLoadBuf(ACSVM_PrintBuf *buf, size_t countFull, size_t count);

void ACSVM_PrintBuf_Push(ACSVM_PrintBuf *buf);

void ACSVM_PrintBuf_PutC(ACSVM_PrintBuf *buf, char c);
void ACSVM_PrintBuf_PutS(ACSVM_PrintBuf *buf, char const *s, size_t n);

void ACSVM_PrintBuf_Reserve(ACSVM_PrintBuf *buf, size_t count);

size_t ACSVM_PrintBuf_Size(ACSVM_PrintBuf const *buf);
size_t ACSVM_PrintBuf_SizeFull(ACSVM_PrintBuf const *buf);

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__PrintBuf_H__

