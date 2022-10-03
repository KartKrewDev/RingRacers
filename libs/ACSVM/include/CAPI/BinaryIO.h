//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Binary data reading/writing.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__CAPI__BinaryIO_H__
#define ACSVM__CAPI__BinaryIO_H__

#include "Types.h"

#include <stdio.h>

#ifdef __cplusplus
#include "../ACSVM/BinaryIO.hpp"

#include <istream>
#include <ostream>
#endif


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Types                                                                      |
//

#ifdef __cplusplus
//
// ACSVM_Buffer
//
struct ACSVM_Buffer : std::streambuf
{
public:
   ACSVM_Buffer(FILE *stream);

   FILE *stream;

protected:
   virtual int overflow(int c);

   virtual int uflow();
};

//
// ACSVM_IStream
//
struct ACSVM_IStream : std::istream
{
public:
   ACSVM_IStream(FILE *stream);

   ACSVM_Buffer buf;
};

//
// ACSVM_OStream
//
struct ACSVM_OStream : std::ostream
{
public:
   ACSVM_OStream(FILE *stream);

   ACSVM_Buffer buf;
};
#endif


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

ACSVM_IStream *ACSVM_AllocIStream_File(FILE *stream);
void ACSVM_FreeIStream(ACSVM_IStream *stream);

ACSVM_OStream *ACSVM_AllocOStream_File(FILE *stream);
void ACSVM_FreeOStream(ACSVM_OStream *stream);

uintmax_t ACSVM_ReadVLN(ACSVM_IStream *in);

void ACSVM_WriteVLN(ACSVM_OStream *out, uintmax_t in);

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__BinaryIO_H__

