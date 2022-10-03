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

#ifndef ACSVM__CAPI__Floats_H__
#define ACSVM__CAPI__Floats_H__

#include "Types.h"

#ifdef __cplusplus
#include "../Util/Floats.hpp"
#endif


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

bool ACSVM_CF_AddF_W1(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACSVM_CF_AddF_W2(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACSVM_CF_DivF_W1(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACSVM_CF_DivF_W2(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACSVM_CF_MulF_W1(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACSVM_CF_MulF_W2(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACSVM_CF_SubF_W1(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACSVM_CF_SubF_W2(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);

bool ACSVM_CF_PrintDouble(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACSVM_CF_PrintFloat(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);

double ACSVM_DWordToDouble(ACSVM_DWord w);

ACSVM_DWord ACSVM_DoubleToDWord(double f);

ACSVM_Word ACSVM_FloatToWord(float f);

float ACSVM_WordToFloat(ACSVM_Word w);

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__Floats_H__

