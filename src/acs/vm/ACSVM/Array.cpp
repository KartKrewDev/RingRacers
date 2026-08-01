//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2025 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Array class.
//
//-----------------------------------------------------------------------------

#include "Array.hpp"

#include "BinaryIO.hpp"
#include "Environment.hpp"
#include "Serial.hpp"


//----------------------------------------------------------------------------|
// Static Functions                                                           |
//

namespace ACSVM
{
   //
   // FreeData (Word)
   //
   static void FreeData(Word &)
   {
   }

   //
   // FreeData
   //
   template<typename T>
   static void FreeData(T *&data)
   {
      if(!data) return;

      for(auto &itr : *data)
         FreeData(itr);

      delete[] data;
      data = nullptr;
   }

   //
   // RefStringsData (Word)
   //
   static void RefStringsData(Environment *env, Word const &data, void (*ref)(String *))
   {
      ref(env->getString(data));
   }

   //
   // RefStringsData
   //
   template<typename T>
   static void RefStringsData(Environment *env, T *data, void (*ref)(String *))
   {
      if(data) for(auto &itr : *data)
         RefStringsData(env, itr, ref);
   }

   //
   // WritePage
   //
   static void WritePage(Serial &out, Word const *page, std::size_t pageC, std::size_t idx)
   {
      std::size_t pageEnd = pageC, pageIdx = 0;

      // Skip trailing 0 elements.
      while(pageEnd && !page[pageEnd - 1])
         --pageEnd;

      if(!pageEnd) return;

      // Skip leading 0 elements.
      while(!page[pageIdx])
         ++pageIdx;

      WriteVLN(out, idx + pageIdx);
      WriteVLN(out, pageEnd - pageIdx);
      while(pageIdx != pageEnd)
         WriteVLN(out, page[pageIdx++]);
   }
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // Array::operator [Word]
   //
   Word &Array::operator [] (Word idx)
   {
      if(!data) data = new Data[1]{};
      Bank *&bank = (*data)[idx / (BankSize * SegmSize * PageSize)];

      if(!bank) bank = new Bank[1]{};
      Segm *&segm = (*bank)[idx / (SegmSize * PageSize) % BankSize];

      if(!segm) segm = new Segm[1]{};
      Page *&page = (*segm)[idx / PageSize % SegmSize];

      if(!page) page = new Page[1]{};
      return (*page)[idx % PageSize];
   }

   //
   // Array::find
   //
   Word Array::find(Word idx) const
   {
      if(!data) return 0;
      Bank *&bank = (*data)[idx / (BankSize * SegmSize * PageSize)];

      if(!bank) return 0;
      Segm *&segm = (*bank)[idx / (SegmSize * PageSize) % BankSize];

      if(!segm) return 0;
      Page *&page = (*segm)[idx / PageSize % SegmSize];

      if(!page) return 0;
      return (*page)[idx % PageSize];
   }

   //
   // Array::clear
   //
   void Array::clear()
   {
      FreeData(data);
   }

   //
   // Array::empty
   //
   bool Array::empty() const
   {
      return data;
   }

   //
   // Array::loadState
   //
   void Array::loadState(Serial &in)
   {
      clear();

      in.readSign(Signature::Array);

      if(in.version == 0)
      {
         if(in.readByte()) for(std::size_t bankIdx = 0; bankIdx != 256; ++bankIdx)
         {
            if(in.readByte()) for(std::size_t segmIdx = 0; segmIdx != 256; ++segmIdx)
            {
               if(in.readByte()) for(std::size_t pageIdx = 0; pageIdx != 256; ++pageIdx)
               {
                  if(in.readByte())
                  {
                     std::size_t idx = bankIdx * 256*256*256 + segmIdx * 256*256 + pageIdx * 256;
                     std::size_t len = 256;

                     while(len--)
                        (*this)[idx++] = ReadVLN<Word>(in);
                  }
               }
            }
         }
      }
      else
      {
         while(true)
         {
            std::size_t idx = ReadVLN<std::size_t>(in);
            std::size_t len = ReadVLN<std::size_t>(in);

            if(idx == 0 && len == 0)
               break;

            while(len--)
               (*this)[idx++] = ReadVLN<Word>(in);
         }
      }

      in.readSign(~Signature::Array);
   }

   //
   // Array::lockStrings
   //
   void Array::lockStrings(Environment *env) const
   {
      RefStringsData(env, data, [](String *s){++s->lock;});
   }

   //
   // Array::refStrings
   //
   void Array::refStrings(Environment *env) const
   {
      RefStringsData(env, data, [](String *s){s->ref = true;});
   }

   //
   // Array::saveState
   //
   void Array::saveState(Serial &out) const
   {
      out.writeSign(Signature::Array);

      std::size_t idx = 0;
      if(data) for(auto const &bank : *data)
      {
         if(bank) for(auto const &segm : *bank)
         {
            if(segm) for(auto const &page : *segm)
            {
               if(page)
                  WritePage(out, *page, PageSize, idx);

               idx += PageSize;
            }
            else
               idx += SegmSize * PageSize;
         }
         else
            idx += BankSize * SegmSize * PageSize;
      }

      out.writeByte(0);
      out.writeByte(0);

      out.writeSign(~Signature::Array);
   }

   //
   // Array::unlockStrings
   //
   void Array::unlockStrings(Environment *env) const
   {
      RefStringsData(env, data, [](String *s){--s->lock;});
   }
}

// EOF

