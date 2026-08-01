//-----------------------------------------------------------------------------
//
// Copyright (C) 2017-2026 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Serialization.
//
//-----------------------------------------------------------------------------

#include "Serial.hpp"

#include "BinaryIO.hpp"
#include "Error.hpp"

#include <cmath>
#include <cstring>


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // Serial::loadHead
   //
   void Serial::loadHead()
   {
      char buf[6] = {};
      read(buf, 6);

      if(std::memcmp(buf, "ACSVM\0", 6))
         throw SerialError{"invalid file signature"};

      version = ReadVLN<unsigned int>(*this);
      if(version > CurrentVersion)
         throw SerialError("unknown version");

      auto flags = ReadVLN<std::uint_fast32_t>(*this);
      signs = flags & 0x0001;
   }

   //
   // Serial::loadTail
   //
   void Serial::loadTail()
   {
      readSign(~Signature::Serial);
   }

   //
   // Serial::readByte
   //
   char Serial::readByte()
   {
      char buf[1];
      read(buf, 1);
      return buf[0];
   }

   //
   // Serial::readFloat
   //
   double Serial::readFloat()
   {
      bool negExp, sign;

      // Read float type.
      switch(readByte())
      {
      case 0: return +0.0;
      case 1: return -0.0;
      case 2: negExp = false; sign = false; break;
      case 3: negExp = false; sign = true;  break;
      case 4: negExp = true;  sign = false; break;
      case 5: negExp = true;  sign = true;  break;
      case 6: return +INFINITY;
      case 7: return -INFINITY;
      case 8: return +NAN;
      case 9: return -NAN;
      default:
         throw SerialError{"invalid float type"};
      }

      int    exp = readVLN<unsigned>();
      double man = readVLN<std::uint64_t>();

      man = ldexp(man, negExp ? -exp : exp);
      return sign ? -man : man;
   }

   //
   // Serial::readSign
   //
   void Serial::readSign(Signature sign)
   {
      if(!signs) return;

      unsigned char buf[4];
      read(reinterpret_cast<char *>(buf), 4);
      auto got = static_cast<Signature>(ReadLE4(buf));

      if(sign != got)
         throw SerialSignError{sign, got};
   }

   //
   // Serial::saveHead
   //
   void Serial::saveHead()
   {
      write("ACSVM\0", 6);
      WriteVLN(*this, CurrentVersion);

      std::uint_fast32_t flags = 0;
      if(signs) flags |= 0x0001;
      WriteVLN(*this, flags);
   }

   //
   // Serial::saveTail
   //
   void Serial::saveTail()
   {
      writeSign(~Signature::Serial);
   }

   //
   // Serial::writeByte
   //
   void Serial::writeByte(char data)
   {
      write(&data, 1);
   }

   //
   // Serial::writeFloat
   //
   void Serial::writeFloat(double data)
   {
      bool sign = std::signbit(data);

      switch(std::fpclassify(data))
      {
      case FP_INFINITE:  writeByte(sign ?  9 :  8); return;
      case FP_NAN:       writeByte(sign ? 11 : 10); return;
      case FP_NORMAL:    break;
      case FP_SUBNORMAL: break;
      case FP_ZERO:      writeByte(sign ?  1 :  0); return;
      }

      int    exp;
      double man = std::ldexp(std::frexp(sign ? -data : data, &exp), 53);
      exp       -= 53;

      writeByte(exp < 0 ? (sign ? 5 : 4) : (sign ? 3 : 2));
      writeVLN(static_cast<unsigned>(std::abs(exp)));
      writeVLN(static_cast<std::uint64_t>(man));
   }

   //
   // Serial::writeSign
   //
   void Serial::writeSign(Signature sign)
   {
      if(!signs) return;

      unsigned char buf[4];
      WriteLE4(buf, static_cast<std::uint32_t>(sign));
      write(reinterpret_cast<char *>(buf), 4);
   }
}

// EOF

