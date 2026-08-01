//-----------------------------------------------------------------------------
//
// Copyright (C) 2025 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Serialization using std::iostream.
//
//-----------------------------------------------------------------------------

#include "SerialSTD.hpp"

#include "Error.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // SerialSTD::read
   //
   void SerialSTD::read(char *data, std::size_t size)
   {
      if(!in->read(data, size))
         throw SerialError("failed read");
   }

   //
   // SerialSTD::readByte
   //
   char SerialSTD::readByte()
   {
      auto data = in->get();
      if(!*in)
         throw SerialError("failed read");
      return data;
   }

   //
   // SerialSTD::write
   //
   void SerialSTD::write(char const *data, std::size_t size)
   {
      if(!out->write(data, size))
         throw SerialError("failed write");
   }

   //
   // SerialSTD::writeByte
   //
   void SerialSTD::writeByte(char data)
   {
      if(!out->put(data))
         throw SerialError("failed write");
   }
}

// EOF

