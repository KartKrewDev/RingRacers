//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Profiling data handling.
//
//-----------------------------------------------------------------------------

#include "ProfileData.hpp"

#include "Serial.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // ProfileData::addCall
   //
   void ProfileData::addCall()
   {
      if(callNum++)
      {
         if(callMax < callTot) callMax = callTot;
         if(callMin > callTot) callMin = callTot;
      }
      else
      {
         callMax = callMin = callTot;
      }

      callTot = 0;
   }

   //
   // ProfileData::addTime
   //
   void ProfileData::addTime(ProfileTime pt)
   {
      if(timeNum++)
      {
         if(timeMax < pt) timeMax = pt;
         if(timeMin > pt) timeMin = pt;
      }
      else
      {
         timeMax = timeMin = pt;
      }

      callTot += pt;
      timeTot += pt;
   }

   //
   // ProfileData::loadState
   //
   void ProfileData::loadState(Serial &in)
   {
      callMax = in.readFloat();
      callMin = in.readFloat();
      callTot = in.readFloat();

      timeMax = in.readFloat();
      timeMin = in.readFloat();
      timeTot = in.readFloat();

      callNum = in.readVLN<decltype(callNum)>();
      timeNum = in.readVLN<decltype(timeNum)>();
   }

   //
   // ProfileData::saveState
   //
   void ProfileData::saveState(Serial &out) const
   {
      out.writeFloat(callMax);
      out.writeFloat(callMin);
      out.writeFloat(callTot);

      out.writeFloat(timeMax);
      out.writeFloat(timeMin);
      out.writeFloat(timeTot);

      out.writeVLN(callNum);
      out.writeVLN(timeNum);
   }
}

// EOF

