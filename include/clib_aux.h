/*
MIT License Copyright (c) 2018 Roman Marin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __CLIB_AUX_H
#define __CLIB_AUX_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h> /* for snprintf() */
#include <stdint.h>

//#define USE_INTRINSICS_SSE

#if UINTPTR_MAX == 0xffffffffffffffff
/* we're on 64-bit */
#define USE_64BIT_TARGET
#endif // UINTPTR_MAX

#if defined (USE_INTRINSICS_SSE) || defined (USE_INTRINSICS_AVX)
#include <intrin.h>
#endif // defined

#ifdef USE_64BIT_TARGET
#define astrlen(a) astrlen_64(a)
#define astrcmp(a,b) astrcmp_64(a,b)
#define memcmpeq(a,b,c) memcmpeq_64(a,b,c)
#define ascii_tolower(a,b,c) ascii_tolower_64(a,b,c)
#else
#define astrlen(a) astrlen_32(a)
#define astrcmp(a,b) astrcmp_32(a,b)
#define memcmpeq(a,b,c) memcmpeq_32(a,b,c)
#define ascii_tolower(a,b,c) ascii_tolower_32(a,b,c)
#endif // TARGET_IS_64BIT

#if defined(USE_INTRINSICS_AVX)
#define find_charpos(a,b,c) find_charpos_avx(a,b,c)
#define find_charptr(a,b,c) find_charptr_avx(a,b,c)
#define find_ptrnpos(a,b,c,d) find_ptrnpos_avx(a,b,c,d)
#elif defined(USE_INTRINSICS_SSE)
#define find_charpos(a,b,c) find_charpos_sse(a,b,c)
#define find_charptr(a,b,c) find_charptr_sse(a,b,c)
#define find_ptrnpos(a,b,c,d) find_ptrnpos_sse(a,b,c,d)
#elif defined(USE_64BIT_TARGET)
#define find_charpos(a,b,c) find_charpos_64(a,b,c)
#define find_charptr(a,b,c) find_charptr_64(a,b,c)
#define find_ptrnpos(a,b,c,d) find_ptrnpos_64(a,b,c,d)
#else
#define find_charpos(a,b,c) find_charpos_32(a,b,c)
#define find_charptr(a,b,c) find_charptr_32(a,b,c)
#define find_ptrnpos(a,b,c,d) find_ptrnpos_32(a,b,c,d)
#endif // USE_INTRINSICS_AVX

/*  See: Sean Eron Anderson's trick to find out if there's a zero byte
*   https://graphics.stanford.edu/~seander/bithacks.html */
#define haszero(v) (((v) - 0x01010101UL) & ~(v) & 0x80808080UL)
#define haszeroll(v) (((v) - 0x0101010101010101ULL) & ~(v) & 0x8080808080808080ULL)

#define REPLICATE4(a, b) (a=(~0UL/0xff*(b)))
#define REPLICATE8(a, b) (a=(~0ULL/0xff*(b)))

/** Return NULL position in ascii string - a replacement for strlen() */
int astrlen_32(const char *s);
int astrlen_64(const char *s);

/** compare two null terminated strings - if returned 0 the strings are equal*/
int astrcmp_32(const char *s1, const char *s2);
int astrcmp_64(const char *s1, const char *s2);

/** memory comparison helper functions
*   Return: not 0 - are equal
*           0 - not equal
*/
int memcmpeq_32(const char *a, const char *b, size_t len);
int memcmpeq_64(const char *a, const char *b, size_t len);

/** The following functions search for a byte in a memory buffer of len bytes
*   Replacement for standard strchr() or memchr()
*/
char *find_charptr_32(const char *s, const char ch, size_t len);
char *find_charptr_64(const char *s, const char ch, size_t len);
/** same but return byte's position in the buffer */
int find_charpos_32(const char *s, const char ch, size_t len);
int find_charpos_64(const char *s, const char ch, size_t len);

/** same but use processor intrinsics */
char *find_charptr_sse(const char *s, char ch, size_t len);
int find_charpos_sse(const char *s, char ch, size_t len);
char *find_charptr_avx(const char *s, char ch, size_t len);
int find_charpos_avx(const char *s, char ch, size_t len);

/** The functions search in a buffer for a pattern (the pattern must be more than 1 byte long),
*   Return: pattern's first byte's offset in the buffer or
*           -1 if pattern was not found
*/
int32_t find_ptrnpos_32(const char *s, size_t slen, const char *ptrn, size_t ptlen);
int64_t find_ptrnpos_64(const char *s, size_t slen, const char *ptrn, size_t ptlen);
/** same but use processor intrinsics */
int64_t find_ptrnpos_sse(const char *s, size_t slen, const char *ptrn, size_t ptlen);
int64_t find_ptrnpos_avx(const char *s, size_t slen, const char *ptrn, size_t ptlen);


/** Replace ASCII characters in the string with lowercase
*   Remark: input string is modified
*/
char *ascii_tolower_32(char *d, const char *s, size_t len);
char *ascii_tolower_64(char *d, const char *s, size_t len);

/** ascii to unsigned int */
uint32_t to_uint32(char *s);
uint64_t to_uint64(char *s);

/**
*   Convert integer number to ascii string (base 10 only)
*   Input: buf - output buffer
*           len - maximum number of bytes to write
*           n - number to convert
*   Return - # bytes (ascii digits) written
*          -1 - error (not enough space)
*   Remark: null terminator is not appended
*/
int itoa_aux(long long n, char* buf, int len);

/**
*   Convert double to ascii string (base 10 only)
*   Input: dst - output buffer
*           len - maximum number of bytes to write
*           n - number to convert
*   Return - # bytes (ascii digits) written
*          -1 - error (e.g. not enough space)
*   Remark: null terminator is not appended
*           Inf, NaN are treated as invalid json numbers, -1 is returned
*/
int dtoa_aux(double n, char* buf, int len);

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // __CLIB_AUX_H
