//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2025 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Binary data reading/writing primitives.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__BinaryIO_H__
#define ACSVM__BinaryIO_H__

#include "Types.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   std::uint_fast8_t  ReadLE1(Byte const *data);
   std::uint_fast16_t ReadLE2(Byte const *data);
   std::uint_fast32_t ReadLE4(Byte const *data);

   void WriteLE4(Byte *out, std::uint_fast32_t in);

   //
   // ReadLE1
   //
   inline std::uint_fast8_t ReadLE1(Byte const *data)
   {
      return static_cast<std::uint_fast8_t>(data[0]);
   }

   //
   // ReadLE2
   //
   inline std::uint_fast16_t ReadLE2(Byte const *data)
   {
      return
         (static_cast<std::uint_fast16_t>(data[0]) << 0) |
         (static_cast<std::uint_fast16_t>(data[1]) << 8);
   }

   //
   // ReadLE4
   //
   inline std::uint_fast32_t ReadLE4(Byte const *data)
   {
      return
         (static_cast<std::uint_fast32_t>(data[0]) <<  0) |
         (static_cast<std::uint_fast32_t>(data[1]) <<  8) |
         (static_cast<std::uint_fast32_t>(data[2]) << 16) |
         (static_cast<std::uint_fast32_t>(data[3]) << 24);
   }

   //
   // WriteLE4
   //
   inline void WriteLE4(Byte *out, std::uint_fast32_t in)
   {
      out[0] = (in >>  0) & 0xFF;
      out[1] = (in >>  8) & 0xFF;
      out[2] = (in >> 16) & 0xFF;
      out[3] = (in >> 24) & 0xFF;
   }
}

#endif//ACSVM__BinaryIO_H__

