// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_memcpy.c
/// \brief X86 optimized implementations of M_Memcpy

#include "doomdef.h"
#include "m_misc.h"

#if defined (__GNUC__) && defined (__i386__) // from libkwave, under GPL
// Alam: note libkwave memcpy code comes from mplayer's libvo/aclib_template.c, r699

/* for small memory blocks (<256 bytes) this version is faster */
#define small_memcpy(dest,src,n)\
{\
register unsigned long int dummy;\
__asm__ __volatile__(\
	"cld\n\t"\
	"rep; movsb"\
	:"=&D"(dest), "=&S"(src), "=&c"(dummy)\
	:"0" (dest), "1" (src),"2" (n)\
	: "memory", "cc");\
}

/* linux kernel __memcpy (from: /include/asm/string.h) */
ATTRINLINE static FUNCINLINE void *__memcpy (void *dest, const void * src, size_t n)
{
	int d0, d1, d2;

	if ( n < 4 )
	{
		small_memcpy(dest, src, n);
	}
	else
	{
		__asm__ __volatile__ (
			"rep ; movsl;"
			"testb $2,%b4;"
			"je 1f;"
			"movsw;"
			"1:\ttestb $1,%b4;"
			"je 2f;"
			"movsb;"
			"2:"
		: "=&c" (d0), "=&D" (d1), "=&S" (d2)
		:"0" (n/4), "q" (n),"1" ((long) dest),"2" ((long) src)
		: "memory");
	}

	return dest;
}

#define SSE_MMREG_SIZE 16
#define MMX_MMREG_SIZE 8

#define MMX1_MIN_LEN 0x800  /* 2K blocks */
#define MIN_LEN 0x40  /* 64-byte blocks */

/* SSE note: i tried to move 128 bytes a time instead of 64 but it
didn't make any measureable difference. i'm using 64 for the sake of
simplicity. [MF] */
static /*FUNCTARGET("sse2")*/ void *sse_cpy(void * dest, const void * src, size_t n)
{
	void *retval = dest;
	size_t i;

	/* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
		"prefetchnta (%0);"
		"prefetchnta 32(%0);"
		"prefetchnta 64(%0);"
		"prefetchnta 96(%0);"
		"prefetchnta 128(%0);"
		"prefetchnta 160(%0);"
		"prefetchnta 192(%0);"
		"prefetchnta 224(%0);"
		"prefetchnta 256(%0);"
		"prefetchnta 288(%0);"
		: : "r" (src) );

	if (n >= MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(SSE_MMREG_SIZE-1);
		if (delta)
		{
			delta=SSE_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		if (((unsigned long)src) & 15)
		/* if SRC is misaligned */
		 for (; i>0; i--)
		 {
			__asm__ __volatile__ (
				"prefetchnta 320(%0);"
				"prefetchnta 352(%0);"
				"movups (%0), %%xmm0;"
				"movups 16(%0), %%xmm1;"
				"movups 32(%0), %%xmm2;"
				"movups 48(%0), %%xmm3;"
				"movntps %%xmm0, (%1);"
				"movntps %%xmm1, 16(%1);"
				"movntps %%xmm2, 32(%1);"
				"movntps %%xmm3, 48(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = (const unsigned char *)src + 64;
			dest = (unsigned char *)dest + 64;
		}
		else
			/*
			   Only if SRC is aligned on 16-byte boundary.
			   It allows to use movaps instead of movups, which required data
			   to be aligned or a general-protection exception (#GP) is generated.
			*/
		 for (; i>0; i--)
		 {
			__asm__ __volatile__ (
				"prefetchnta 320(%0);"
				"prefetchnta 352(%0);"
				"movaps (%0), %%xmm0;"
				"movaps 16(%0), %%xmm1;"
				"movaps 32(%0), %%xmm2;"
				"movaps 48(%0), %%xmm3;"
				"movntps %%xmm0, (%1);"
				"movntps %%xmm1, 16(%1);"
				"movntps %%xmm2, 32(%1);"
				"movntps %%xmm3, 48(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		/* since movntq is weakly-ordered, a "sfence"
		 * is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
		/* enables to use FPU */
		__asm__ __volatile__ ("emms":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
}

static FUNCTARGET("mmx") void *mmx2_cpy(void *dest, const void *src, size_t n)
{
	void *retval = dest;
	size_t i;

	/* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
		"prefetchnta (%0);"
		"prefetchnta 32(%0);"
		"prefetchnta 64(%0);"
		"prefetchnta 96(%0);"
		"prefetchnta 128(%0);"
		"prefetchnta 160(%0);"
		"prefetchnta 192(%0);"
		"prefetchnta 224(%0);"
		"prefetchnta 256(%0);"
		"prefetchnta 288(%0);"
	: : "r" (src));

	if (n >= MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(MMX_MMREG_SIZE-1);
		if (delta)
		{
			delta=MMX_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		for (; i>0; i--)
		{
			__asm__ __volatile__ (
				"prefetchnta 320(%0);"
				"prefetchnta 352(%0);"
				"movq (%0), %%mm0;"
				"movq 8(%0), %%mm1;"
				"movq 16(%0), %%mm2;"
				"movq 24(%0), %%mm3;"
				"movq 32(%0), %%mm4;"
				"movq 40(%0), %%mm5;"
				"movq 48(%0), %%mm6;"
				"movq 56(%0), %%mm7;"
				"movntq %%mm0, (%1);"
				"movntq %%mm1, 8(%1);"
				"movntq %%mm2, 16(%1);"
				"movntq %%mm3, 24(%1);"
				"movntq %%mm4, 32(%1);"
				"movntq %%mm5, 40(%1);"
				"movntq %%mm6, 48(%1);"
				"movntq %%mm7, 56(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		/* since movntq is weakly-ordered, a "sfence"
		* is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
		__asm__ __volatile__ ("emms":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
}

static FUNCTARGET("mmx") void *mmx1_cpy(void *dest, const void *src, size_t n) //3DNOW
{
	void *retval = dest;
	size_t i;

	/* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
		"prefetch (%0);"
		"prefetch 32(%0);"
		"prefetch 64(%0);"
		"prefetch 96(%0);"
		"prefetch 128(%0);"
		"prefetch 160(%0);"
		"prefetch 192(%0);"
		"prefetch 224(%0);"
		"prefetch 256(%0);"
		"prefetch 288(%0);"
	: : "r" (src));

	if (n >= MMX1_MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(MMX_MMREG_SIZE-1);
		if (delta)
		{
			delta=MMX_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		for (; i>0; i--)
		{
			__asm__ __volatile__ (
				"prefetch 320(%0);"
				"prefetch 352(%0);"
				"movq (%0), %%mm0;"
				"movq 8(%0), %%mm1;"
				"movq 16(%0), %%mm2;"
				"movq 24(%0), %%mm3;"
				"movq 32(%0), %%mm4;"
				"movq 40(%0), %%mm5;"
				"movq 48(%0), %%mm6;"
				"movq 56(%0), %%mm7;"
				"movq %%mm0, (%1);"
				"movq %%mm1, 8(%1);"
				"movq %%mm2, 16(%1);"
				"movq %%mm3, 24(%1);"
				"movq %%mm4, 32(%1);"
				"movq %%mm5, 40(%1);"
				"movq %%mm6, 48(%1);"
				"movq %%mm7, 56(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		__asm__ __volatile__ ("femms":::"memory"); // same as mmx_cpy() but with a femms
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
}
#endif

// Alam: why? memcpy may be __cdecl/_System and our code may be not the same type
static void *cpu_cpy(void *dest, const void *src, size_t n)
{
	if (src == NULL)
	{
		CONS_Debug(DBG_MEMORY, "Memcpy from 0x0?!: %p %p %s\n", dest, src, sizeu1(n));
		return dest;
	}

	if(dest == NULL)
	{
		CONS_Debug(DBG_MEMORY, "Memcpy to 0x0?!: %p %p %s\n", dest, src, sizeu1(n));
		return dest;
	}

	return memcpy(dest, src, n);
}

static /*FUNCTARGET("mmx")*/ void *mmx_cpy(void *dest, const void *src, size_t n)
{
#if defined (_MSC_VER) && defined (_X86_)
	_asm
	{
		mov ecx, [n]
		mov esi, [src]
		mov edi, [dest]
		shr ecx, 6 // mit mmx: 64bytes per iteration
		jz lower_64 // if lower than 64 bytes
		loop_64: // MMX transfers multiples of 64bytes
		movq mm0,  0[ESI] // read sources
		movq mm1,  8[ESI]
		movq mm2, 16[ESI]
		movq mm3, 24[ESI]
		movq mm4, 32[ESI]
		movq mm5, 40[ESI]
		movq mm6, 48[ESI]
		movq mm7, 56[ESI]

		movq  0[EDI], mm0 // write destination
		movq  8[EDI], mm1
		movq 16[EDI], mm2
		movq 24[EDI], mm3
		movq 32[EDI], mm4
		movq 40[EDI], mm5
		movq 48[EDI], mm6
		movq 56[EDI], mm7

		add esi, 64
		add edi, 64
		dec ecx
		jnz loop_64
		emms // close mmx operation
		lower_64:// transfer rest of buffer
		mov ebx,esi
		sub ebx,src
		mov ecx,[n]
		sub ecx,ebx
		shr ecx, 3 // multiples of 8 bytes
		jz lower_8
		loop_8:
		movq  mm0, [esi] // read source
		movq [edi], mm0 // write destination
		add esi, 8
		add edi, 8
		dec ecx
		jnz loop_8
		emms // close mmx operation
		lower_8:
		mov ebx,esi
		sub ebx,src
		mov ecx,[n]
		sub ecx,ebx
		rep movsb
		mov eax, [dest] // return dest
	}
#elif defined (__GNUC__) && defined (__i386__)
	void *retval = dest;
	size_t i;

	if (n >= MMX1_MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(MMX_MMREG_SIZE-1);
		if (delta)
		{
			delta=MMX_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		for (; i>0; i--)
		{
			__asm__ __volatile__ (
				"movq (%0), %%mm0;"
				"movq 8(%0), %%mm1;"
				"movq 16(%0), %%mm2;"
				"movq 24(%0), %%mm3;"
				"movq 32(%0), %%mm4;"
				"movq 40(%0), %%mm5;"
				"movq 48(%0), %%mm6;"
				"movq 56(%0), %%mm7;"
				"movq %%mm0, (%1);"
				"movq %%mm1, 8(%1);"
				"movq %%mm2, 16(%1);"
				"movq %%mm3, 24(%1);"
				"movq %%mm4, 32(%1);"
				"movq %%mm5, 40(%1);"
				"movq %%mm6, 48(%1);"
				"movq %%mm7, 56(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		__asm__ __volatile__ ("emms":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
#else
	return cpu_cpy(dest, src, n);
#endif
}

void *(*M_Memcpy)(void* dest, const void* src, size_t n) = cpu_cpy;

/** Memcpy that uses MMX, 3DNow, MMXExt or even SSE
  * Do not use on overlapped memory, use memmove for that
  */
void M_SetupMemcpy(void)
{
#if defined (__GNUC__) && defined (__i386__)
	if (R_SSE2)
		M_Memcpy = sse_cpy;
	else if (R_MMXExt)
		M_Memcpy = mmx2_cpy;
	else if (R_3DNow)
		M_Memcpy = mmx1_cpy;
	else
#endif
	if (R_MMX)
		M_Memcpy = mmx_cpy;
#if 0
	M_Memcpy = cpu_cpy;
#endif
}
