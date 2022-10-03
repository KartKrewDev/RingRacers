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

#include "BinaryIO.h"


extern "C"
{

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// ACSVM_Buffer constructor
//
ACSVM_Buffer::ACSVM_Buffer(FILE *stream_) :
   stream{stream_}
{
}

//
// ACSVM_Buffer::overflow
//
int ACSVM_Buffer::overflow(int c)
{
   return std::fputc(c, stream);
}

//
// ACSVM_Buffer::uflow
//
int ACSVM_Buffer::uflow()
{
   return std::fgetc(stream);
}

//
// ACSVM_IStream constructor
//
ACSVM_IStream::ACSVM_IStream(FILE *stream) :
   buf{stream}
{
}

//
// ACSVM_OStream constructor
//
ACSVM_OStream::ACSVM_OStream(FILE *stream) :
   buf{stream}
{
}

//
// ACSVM_AllocIStream
//
ACSVM_IStream *ACSVM_AllocIStream_File(FILE *stream)
{
   return reinterpret_cast<ACSVM_IStream *>(
      static_cast<std::istream *>(new(std::nothrow) ACSVM_IStream(stream)));
}

//
// ACSVM_AllocOStream
//
ACSVM_OStream *ACSVM_AllocOStream_File(FILE *stream)
{
   return reinterpret_cast<ACSVM_OStream *>(
      static_cast<std::ostream *>(new(std::nothrow) ACSVM_OStream(stream)));
}

//
// ACSVM_ReadVLN
//
uintmax_t ACSVM_ReadVLN(ACSVM_IStream *in)
{
   return ACSVM::ReadVLN<std::uintmax_t>(reinterpret_cast<std::istream &>(*in));
}

//
// ACSVM_WriteVLN
//
void ACSVM_WriteVLN(ACSVM_OStream *out, uintmax_t in)
{
   ACSVM::WriteVLN(reinterpret_cast<std::ostream &>(*out), in);
}

}

// EOF

