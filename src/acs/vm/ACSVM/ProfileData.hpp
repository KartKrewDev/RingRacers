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

#ifndef ACSVM__ProfileData_H__
#define ACSVM__ProfileData_H__

#include "Types.hpp"


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // ProfileData
   //
   class ProfileData
   {
   public:
      ProfileData() :
         callMax{0}, callMin{0}, callTot{0},
         timeMax{0}, timeMin{0}, timeTot{0},
         callNum{0}, timeNum{0}
      {
      }

      explicit operator bool () const {return timeNum != 0;}

      void addCall();

      void addTime(ProfileTime pt);

      void loadState(Serial &in);

      //
      // reset
      //
      void reset()
      {
         callMax = callMin = callTot = 0;
         timeMax = timeMin = timeTot = 0;
         callNum = timeNum = 0;
      }

      void saveState(Serial &out) const;

      ProfileTime callMax;
      ProfileTime callMin;
      ProfileTime callTot;

      ProfileTime timeMax;
      ProfileTime timeMin;
      ProfileTime timeTot;

      std::size_t callNum;
      std::size_t timeNum;
   };
}


#endif//ACSVM__ProfileData_H__

