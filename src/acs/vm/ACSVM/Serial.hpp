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

#ifndef ACSVM__Serial_H__
#define ACSVM__Serial_H__

#include "ID.hpp"

#include <climits>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // Signature
   //
   enum class Signature : std::uint32_t
   {
      Array       = MakeID("ARAY"),
      Environment = MakeID("ENVI"),
      GlobalScope = MakeID("GBLs"),
      HubScope    = MakeID("HUBs"),
      MapScope    = MakeID("MAPs"),
      ModuleScope = MakeID("MODs"),
      Serial      = MakeID("SERI"),
      Thread      = MakeID("THRD"),
   };

   //
   // Serial
   //
   // Read/write functions must either read/write the given number of bytes, or
   // throw an exception.
   //
   class Serial
   {
   public:
      Serial() : version{CurrentVersion}, signs{false} {}

      void loadHead();
      void loadTail();

      virtual void read(char *data, std::size_t size) = 0;
      virtual char readByte();
      double readFloat();
      void readSign(Signature sign);
      template<typename T> T readVLN();

      void saveHead();
      void saveTail();

      virtual void write(char const *data, std::size_t size) = 0;
      virtual void writeByte(char data);
      void writeFloat(double data);
      void writeSign(Signature sign);
      template<typename T> void writeVLN(T in);

      unsigned int version;
      bool         signs;


      static constexpr unsigned int CurrentVersion = 2;
   };
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   constexpr Signature operator ~ (Signature sign)
      {return static_cast<Signature>(~static_cast<std::uint32_t>(sign));}

   //
   // Serial::readVLN
   //
   template<typename T>
   T Serial::readVLN()
   {
      T out{0};

      unsigned char c;
      while(((c = readByte()) & 0x80))
         out = (out << 7) + (c & 0x7F);
      out = (out << 7) + c;

      return out;
   }
   template<typename T>
   T ReadVLN(Serial &in) {return in.readVLN<T>();}

   //
   // Serial::WriteVLN
   //
   template<typename T>
   void Serial::writeVLN(T in)
   {
      constexpr std::size_t len = (sizeof(T) * CHAR_BIT + 6) / 7;
      char buf[len], *ptr = buf + len;

      *--ptr = static_cast<char>(in & 0x7F);
      while((in >>= 7))
         *--ptr = static_cast<char>(in & 0x7F) | 0x80;

      write(ptr, (buf + len) - ptr);
   }
   template<typename T>
   void WriteVLN(Serial &out, T in) {out.writeVLN<T>(in);}
}

#endif//ACSVM__Serial_H__

