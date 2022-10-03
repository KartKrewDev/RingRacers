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

#include "PrintBuf.h"

#include <new>


extern "C"
{

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// ACSVM_AllocPrintBuf
//
ACSVM_PrintBuf *ACSVM_AllocPrintBuf(void)
{
   return reinterpret_cast<ACSVM_PrintBuf *>(new(std::nothrow) ACSVM::PrintBuf);
}

//
// ACSVM_FreePrintBuf
//
void ACSVM_FreePrintBuf(ACSVM_PrintBuf *buf)
{
   delete reinterpret_cast<ACSVM::PrintBuf *>(buf);
}

//
// ACSVM_PrintBuf_Clear
//
void ACSVM_PrintBuf_Clear(ACSVM_PrintBuf *buf)
{
   reinterpret_cast<ACSVM::PrintBuf *>(buf)->clear();
}

//
// ACSVM_PrintBuf_Drop
//
void ACSVM_PrintBuf_Drop(ACSVM_PrintBuf *buf)
{
   reinterpret_cast<ACSVM::PrintBuf *>(buf)->drop();
}

//
// ACSVM_PrintBuf_Format
//
void ACSVM_PrintBuf_Format(ACSVM_PrintBuf *buf, char const *fmt, ...)
{
   va_list arg;
   va_start(arg, fmt);
   reinterpret_cast<ACSVM::PrintBuf *>(buf)->formatv(fmt, arg);
   va_end(arg);
}

//
// ACSVM_PrintBuf_FormatV
//
void ACSVM_PrintBuf_FormatV(ACSVM_PrintBuf *buf, char const *fmt, va_list arg)
{
   reinterpret_cast<ACSVM::PrintBuf *>(buf)->formatv(fmt, arg);
}

//
// ACSVM_PrintBuf_GetBuf
//
char *ACSVM_PrintBuf_GetBuf(ACSVM_PrintBuf *buf, size_t count)
{
   return reinterpret_cast<ACSVM::PrintBuf *>(buf)->getBuf(count);
}

//
// ACSVM_PrintBuf_GetData
//
char const *ACSVM_PrintBuf_GetData(ACSVM_PrintBuf const *buf)
{
   return reinterpret_cast<ACSVM::PrintBuf const *>(buf)->data();
}

//
// ACSVM_PrintBuf_GetDataFull
//
char const *ACSVM_PrintBuf_GetDataFull(ACSVM_PrintBuf const *buf)
{
   return reinterpret_cast<ACSVM::PrintBuf const *>(buf)->dataFull();
}

//
// ACSVM_PrintBuf_GetLoadBuf
//
char *ACSVM_PrintBuf_GetLoadBuf(ACSVM_PrintBuf *buf, size_t countFull, size_t count)
{
   return reinterpret_cast<ACSVM::PrintBuf *>(buf)->getLoadBuf(countFull, count);
}

//
// ACSVM_PrintBuf_Push
//
void ACSVM_PrintBuf_Push(ACSVM_PrintBuf *buf)
{
   reinterpret_cast<ACSVM::PrintBuf *>(buf)->push();
}

//
// ACSVM_PrintBuf_PutC
//
void ACSVM_PrintBuf_PutC(ACSVM_PrintBuf *buf, char c)
{
   reinterpret_cast<ACSVM::PrintBuf *>(buf)->put(c);
}

//
// ACSVM_PrintBuf_PutS
//
void ACSVM_PrintBuf_PutS(ACSVM_PrintBuf *buf, char const *s, size_t n)
{
   reinterpret_cast<ACSVM::PrintBuf *>(buf)->put(s, n);
}

//
// ACSVM_PrintBuf_Reserve
//
void ACSVM_PrintBuf_Reserve(ACSVM_PrintBuf *buf, size_t count)
{
   reinterpret_cast<ACSVM::PrintBuf *>(buf)->reserve(count);
}

//
// ACSVM_PrintBuf_Size
//
size_t ACSVM_PrintBuf_Size(ACSVM_PrintBuf const *buf)
{
   return reinterpret_cast<ACSVM::PrintBuf const *>(buf)->size();
}

//
// ACSVM_PrintBuf_SizeFull
//
size_t ACSVM_PrintBuf_SizeFull(ACSVM_PrintBuf const *buf)
{
   return reinterpret_cast<ACSVM::PrintBuf const *>(buf)->sizeFull();
}

}

// EOF

