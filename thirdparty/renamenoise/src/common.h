/*
   Copyright (c) 2024, The Mumble Developers
   Copyright (c) 2017, Jean-Marc Valin

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef COMMON_H
#define COMMON_H

#include "stdlib.h"
#include "string.h"

#define RENAMENOISE_INLINE inline

/** Copy n elements from src to dst. The 0* term provides compile-time type checking  */
#ifndef OVERRIDE_RENAMENOISE_COPY
#	define RENAMENOISE_COPY(dst, src, n) (memcpy((dst), (src), (n) * sizeof(*(dst)) + 0 * ((dst) - (src))))
#endif

/** Copy n elements from src to dst, allowing overlapping regions. The 0* term provides compile-time type checking */
#ifndef OVERRIDE_RENAMENOISE_MOVE
#	define RENAMENOISE_MOVE(dst, src, n) (memmove((dst), (src), (n) * sizeof(*(dst)) + 0 * ((dst) - (src))))
#endif

/** Set n elements of dst to zero */
#ifndef OVERRIDE_RENAMENOISE_CLEAR
#	define RENAMENOISE_CLEAR(dst, n) (memset((dst), 0, (n) * sizeof(*(dst))))
#endif

#endif /* COMMON_H */
