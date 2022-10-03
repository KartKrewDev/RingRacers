//----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//----------------------------------------------------------------------------
//
// Floating-point utilities.
//
//----------------------------------------------------------------------------

#include "Floats.h"

#include "Thread.h"


extern "C"
{

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// ACSVM_CF_*
//

bool ACSVM_CF_AddF_W1(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
   {return ACSVM::CF_AddF_W1(thread, argV, argC);}
bool ACSVM_CF_AddF_W2(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
   {return ACSVM::CF_AddF_W2(thread, argV, argC);}
bool ACSVM_CF_DivF_W1(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
   {return ACSVM::CF_DivF_W1(thread, argV, argC);}
bool ACSVM_CF_DivF_W2(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
   {return ACSVM::CF_DivF_W2(thread, argV, argC);}
bool ACSVM_CF_MulF_W1(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
   {return ACSVM::CF_MulF_W1(thread, argV, argC);}
bool ACSVM_CF_MulF_W2(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
   {return ACSVM::CF_MulF_W2(thread, argV, argC);}
bool ACSVM_CF_SubF_W1(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
   {return ACSVM::CF_SubF_W1(thread, argV, argC);}
bool ACSVM_CF_SubF_W2(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
   {return ACSVM::CF_SubF_W2(thread, argV, argC);}

bool ACSVM_CF_PrintDouble(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
   {return ACSVM::CF_PrintDouble(thread, argV, argC);}
bool ACSVM_CF_PrintFloat(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
   {return ACSVM::CF_PrintFloat(thread, argV, argC);}

//
// ACSVM_DWordToDouble
//
double ACSVM_DWordToDouble(ACSVM_DWord w)
{
   return ACSVM::WordsToFloat<double, 2>(
      {{static_cast<ACSVM_Word>(w), static_cast<ACSVM_Word>(w >> 32)}});
}

//
// ACSVM_DoubleToDWord
//
ACSVM_DWord ACSBM_DoubleToDWord(double f)
{
   auto w = ACSVM::FloatToWords<2>(f);
   return (static_cast<ACSVM_DWord>(w[1]) << 32) | w[0];
}

//
// ACSVM_FloatToWord
//
ACSVM_Word ACSVM_FloatToWord(float f)
{
   return ACSVM::FloatToWords<1>(f)[0];
}

//
// ACSVM_WordToFloat
//
float ACSVM_WordToFloat(ACSVM_Word w)
{
   return ACSVM::WordsToFloat<float, 1>({{w}});
}

}

// EOF

