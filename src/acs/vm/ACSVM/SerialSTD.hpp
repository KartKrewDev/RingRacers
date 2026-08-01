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

#ifndef ACSVM__SerialSTD_H__
#define ACSVM__SerialSTD_H__

#include "Serial.hpp"

#include <istream>
#include <ostream>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // SerialSTD
   //
   class SerialSTD : public Serial
   {
   public:
      SerialSTD(std::istream &in_) : in{&in_} {}
      SerialSTD(std::ostream &out_) : out{&out_} {}

      virtual void read(char *data, std::size_t size);
      virtual char readByte();

      virtual void write(char const *data, std::size_t size);
      virtual void writeByte(char data);

      union
      {
         std::istream *const in;
         std::ostream *const out;
      };
   };
}

#endif//ACSVM__SerialSTD_H__

