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
#include "clib_aux.h"

#ifdef _WIN32
#include <windows.h>
#endif


/** find # consecutive trailing or leading zeros in a word */
static __inline uint32_t  t_zeros(uint32_t v)
{
#ifdef _MSC_VER
	unsigned long c = 0;
	if (_BitScanForward(&c, v)){
		return c;
	}
	else{
		return 32;
	}
#else
    return (v)?__builtin_ctz(v):32;
#endif // _MSC_VER
}
static __inline uint32_t  l_zeros(uint32_t v)
{
#ifdef _MSC_VER
	unsigned long c = 0;
	if (_BitScanReverse(&c, v)){
		return c;
	}
	else{
		return 32;
	}
#else
    return (v)?__builtin_clz(v):32;
#endif // _MSC_VER
}

static __inline uint32_t t_zerosll(uint64_t v)
{
#ifdef _MSC_VER
	unsigned long c = 0;
	if (_BitScanForward64(&c, v)){
		return c;
	}
	else{
		return 64;
	}
#else
    return (v)?__builtin_ctzll(v):64;
#endif // _MSC_VER
}

static __inline uint32_t l_zerosll(uint64_t v)
{
#ifdef _MSC_VER
	unsigned long c = 0;
	if (_BitScanReverse64(&c, v)){
		return c;
	}
	else{
		return 64;
	}
#else
    return (v)?__builtin_clzll(v):64;
#endif // _MSC_VER
}


/** convert ascii number to unsigned int */
uint32_t to_uint32(char *s)
{
    uint32_t x = 0;
    for( ; ((unsigned)*s - 0x30) < 10; s++){
        x = 10*x + ((unsigned)*s - 0x30);
    }
    return x;
}

uint64_t to_uint64(char *s)
{
    uint64_t x = 0;
    for( ; ((unsigned)*s - 0x30) < 10; s++){
        x = 10*x + ((unsigned)*s - 0x30);
    }
    return x;
}

/** find position of '\0' byte in the UTF-8 string */
int astrlen_32(const char *s)
{
    uint32_t *str = (uint32_t *)s;
    const char* beg = s;
    while(!haszero(*str)){
        str++;
    }
    s = (char *)str;
    for (int i = 0; i < 4; i ++){
        if(!*s) return (int)(s - beg);
        s++;
    }
    return -1;
}


int astrlen_64(const char *s)
{
    uint64_t *str = (uint64_t *)s;
    const char* beg = s;
    while(!haszeroll(*str)){
        str++;
    }
    s = (char *)str;
    for (int i = 0; i < 8; i ++){
        if(!*s) return (int)(s - beg);
        s++;
    }
    return -1;
}

/** compare 2 zero terminated byte arrays */
int astrcmp_32(const char *s1, const char *s2)
{
    uint32_t *str1 = (uint32_t *)s1;
    uint32_t *str2 = (uint32_t *)s2;
    while((!haszero(*str1))&&(!haszero(*str2))){
        if(*str1 != *str2) return ~0;
        str1++;
        str2++;
    }
    s1 = (char *)str1;
    s2 = (char *)str2;
    for (int i = 0; i < 4; i ++){
        if((*s1 == *s2)&&(*s1 == 0)) return 0;
        if(*s1 != *s2) return ~0;
        s1++;
        s2++;
    }
    return ~0;
}


int astrcmp_64(const char *s1, const char *s2)
{
    uint64_t *str1 = (uint64_t *)s1;
    uint64_t *str2 = (uint64_t *)s2;
    while((!haszeroll(*str1))&&(!haszeroll(*str2))){
        if(*str1 != *str2) return ~0;
        str1++;
        str2++;
    }
    s1 = (char *)str1;
    s2 = (char *)str2;
    for (int i = 0; i < 8; i ++){
        if((*s1 == *s2)&&(*s1 == 0)) return 0;
        if(*s1 != *s2) return ~0;
        s1++;
        s2++;
    }
    return ~0;
}


int memcmpeq_32(const char *a, const char *b, size_t len)
{
    uint32_t *aa = (uint32_t *)a;
    uint32_t *bb = (uint32_t *)b;
    while(len >= 4){
        if((*aa)^(*bb)){
            return 0;
        }
        aa++;
        bb++;
        len -= 4;
    }
    a = (char *)aa;
    b = (char *)bb;
    while(len){
        if((*a)^(*b)){
            return 0;
        }
        a++;
        b++;
        len--;
    }
    return ~0;
}


int memcmpeq_64(const char *a, const char *b, size_t len)
{
    uint64_t *aa = (uint64_t *)a;
    uint64_t *bb = (uint64_t *)b;
    while(len >= 8){
        if((*aa) ^ (*bb)){
            return 0;
        }
        aa++;
        bb++;
        len -= 8;
    }
    a = (char *)aa;
    b = (char *)bb;
    while(len){
        if((*a)^(*b)){
            return 0;
        }
        a++;
        b++;
        len--;
    }
    return ~0;
}

/** find first byte location in the byte array */
char *find_charptr_32(const char *s, const char ch, size_t len)
{
    uint32_t *str = (uint32_t *)s;
    uint32_t test;
    uint32_t cx4;
    REPLICATE4(cx4, ch);
    while(len >= 4){
        test = *str ^ cx4;
        if(haszero(test)){
            break;
        }
        len -= 4;
        str++;
    }
    s = (char *)str;
    while(len){
        if(*s == ch){
            return (char *)s;
        }
        s++;
        len--;
    }
    return NULL;
}


char *find_charptr_64(const char *s, const char ch, size_t len)
{
    uint64_t *str = (uint64_t *)s;
    uint64_t test;
    uint64_t cx8;
    /* replicate ch */
    REPLICATE8(cx8, ch);
    while(len >= 8){
        test = *str ^ cx8;
        if(haszeroll(test)){
            break;
        }
        len -= 8;
        str++;
    }
    s = (char *)str;
    while(len){
        if(*s == ch){
            return (char *)s;
        }
        s++;
        len--;
    }
    return NULL;
}


int find_charpos_32(const char *s, const char ch, size_t len)
{
    char *ptr = (char *)s;
    uint32_t test;
    uint32_t cx4;
    REPLICATE4(cx4, ch);
    while(len >= 4){
        test = *((uint32_t *)ptr) ^ cx4;
        if(haszero(test)){
            break;
        }
        len -= 4;
        ptr += 4;
    }
    while(len){
        if(*ptr == ch){
            return (int)(ptr-s);
        }
        ptr++;
        len--;
    }
    return -1;
}


int find_charpos_64(const char *s, const char ch, size_t len)
{
    char *ptr = (char *)s;
    uint64_t test;
    uint64_t cx8;
    REPLICATE8(cx8, ch);
    while(len >= 8){
        test = *((uint64_t *)ptr) ^ cx8;
        if(haszeroll(test)){
            break;
        }
        len -= 8;
        ptr += 8;
    }
    while(len){
        if(*ptr == ch){
            return (int)(ptr-s);
        }
        ptr++;
        len--;
    }
    return -1;
}


/** find_ptrnpos() functions scan memory buffer searching for a pattern and
*  return its position. They're deemed as replacement for strstr()
*  for x86-64 architectures with AVX\SSE support
*  ( -02\-O3 compiler optimization flag should be applied for noticeable effect )
*/
int32_t find_ptrnpos_32(const char *s, size_t slen, const char *ptrn, size_t ptlen)
{
    uint32_t block_first;
    uint32_t block_last;
    uint32_t first;
    uint32_t last;
    uint32_t mask;
    uint32_t test;
    uint32_t bytepos;
    if (slen < ptlen){
        return -1;
    }
    REPLICATE4(first, ptrn[0]);
    REPLICATE4(last, ptrn[ptlen - 1]);
    for (uint32_t i = 0; i < slen; i += 4) {
        block_first = *((uint32_t *)s);
        block_last  = *((uint32_t *)(s + ptlen - 1));
        mask = (first ^ block_first)|(last ^ block_last);
        /* checking if mask has a byte equal to 0 */
        test = (mask - 0x01010101U) & 0x80808080U & ~mask;
        while(test){
            bytepos = t_zeros(test)>>3;
            if (memcmpeq_32(s + bytepos + 1, ptrn + 1, ptlen - 2)) {
                return i + bytepos;
            }
            test &= (test-1);
        }
        s += 4;
    }
    return -1;
}

int64_t find_ptrnpos_64(const char *s, size_t slen, const char *ptrn, size_t ptlen)
{
    uint64_t block_first;
    uint64_t block_last;
    uint64_t first;
    uint64_t last;
    uint64_t mask;
    uint64_t test;
    uint32_t bytepos;
    if (slen < ptlen){
        return -1;
    }
    REPLICATE8(first, ptrn[0]);
    REPLICATE8(last, ptrn[ptlen - 1]);
    for (size_t i = 0; i < slen; i += 8) {
        block_first = *((uint64_t *)s);
        block_last  = *((uint64_t *)(s + ptlen - 1));
        mask = (first ^ block_first)|(last ^ block_last);
        /* checking if mask has a byte equal to 0 */
        test = (mask - 0x0101010101010101ULL) & 0x8080808080808080ULL & ~mask;
        while(test){
            bytepos = t_zerosll(test)>>3;
            if (memcmpeq_32(s + bytepos + 1, ptrn + 1, ptlen - 2)) {
                return i + bytepos;
            }
            test &= (test-1);
        }
        s += 8;
    }
    return -1;
}

#ifdef USE_INTRINSICS_SSE
char *find_charptr_sse(const char *s, const char ch, size_t len)
{
    const __m128i ch16 = _mm_set1_epi8(ch); /* ch replicated 16 times */
    __m128i x;
    uint32_t mask;
    while(len >= 16){
        x = _mm_loadu_si128((__m128i *)s);
        mask = _mm_movemask_epi8(_mm_cmpeq_epi8(ch16, x));
        if(mask) return (char *)s + t_zeros(mask);
        s += 16;
        len -=16;
    }
    while(len){
        if(*s == ch){
            return (char *)s;
        }
        s++;
        len--;
    }
    return NULL;
}
int find_charpos_sse(const char *s, const char ch, size_t len)
{
    const __m128i ch16 = _mm_set1_epi8(ch); /* ch replicated 16 times */
    __m128i x;
    char *ptr = (char *)s;
    uint32_t mask;
    while(len >= 16){
        x = _mm_loadu_si128((__m128i *)ptr);
        mask = _mm_movemask_epi8(_mm_cmpeq_epi8(ch16, x));
        if(mask) return ((int)(ptr  - s) + t_zeros(mask));
        ptr += 16;
        len -=16;
    }
    while(len){
        if(*ptr == ch){
            return (uintptr_t)(ptr-s);
        }
        ptr++;
        len--;
    }
    return -1;
}
int64_t find_ptrnpos_sse(const char *s, size_t slen, const char *ptrn, size_t ptlen)
{
    __m128i block_first;
    __m128i block_last;
    __m128i eq_first;
    __m128i eq_last;
    if (slen < ptlen){
        return -1;
    }
    const __m128i first = _mm_set1_epi8(ptrn[0]);
    const __m128i last  = _mm_set1_epi8(ptrn[ptlen - 1]);
    for (size_t i = 0; i < slen; i += 16) {
        block_first = _mm_loadu_si128((const __m128i*)(s));
        block_last  = _mm_loadu_si128((const __m128i*)(s + ptlen - 1));
        eq_first = _mm_cmpeq_epi8(first, block_first);
        eq_last  = _mm_cmpeq_epi8(last, block_last);
        uint32_t mask = _mm_movemask_epi8(_mm_and_si128(eq_first, eq_last));
        while (mask) {
            uint32_t bitpos = t_zeros(mask);
            if (memcmpeq_32(s + bitpos + 1, ptrn + 1, ptlen - 2)) {
                return i + bitpos;
            }
            mask &= (mask-1); /* clear lowest set bit */
        }
        s += 16;
    }
    return -1;
}
#endif // USE_INTRINSICS_SSE

#ifdef USE_INTRINSICS_AVX
char *find_charptr_avx(const char *s, const char ch, size_t len)
{
    __m256i cx32 = _mm256_set1_epi8(ch); /* ch replicated 32 times */
    __m256i x;
    uint32_t mask;
    while(len >= 32){
        x = _mm256_loadu_si256((__m256i *)s);
        mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(cx32, x));
        if(mask) return (char*)(s + t_zeros(mask));
        s += 32;
        len -=32;
    }
    while(len){
        if(*s == ch){
            return (char *)s;
        }
        s++;
        len--;
    }
    return NULL;
}
int find_charpos_avx(const char *s, const char ch, size_t len)
{
    const __m256i cx32 = _mm256_set1_epi8(ch); /* ch replicated 32 times */
    __m256i x;
    char *ptr = (char *)s;
    uint32_t mask;
    while(len >= 32){
        x = _mm256_loadu_si256((__m256i *)ptr);
        mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(cx32, x));
        if(mask) return (int)((ptr - s) + t_zeros(mask));
        ptr += 32;
        len -=32;
    }
    while(len){
        if(*ptr == ch){
            return (int)(ptr-s);
        }
        ptr++;
        len--;
    }
    return -1;
}
int64_t find_ptrnpos_avx(const char *s, size_t slen, const char *ptrn, size_t ptlen)
{
    if (slen < ptlen){
        return -1;
    }
    const __m256i first = _mm256_set1_epi8(ptrn[0]);
    const __m256i last = _mm256_set1_epi8(ptrn[ptlen - 1]);
    for (size_t i = 0; i < slen; i += 32) {
        const __m256i block_first = _mm256_loadu_si256((const __m256i*)(s));
        const __m256i block_last  = _mm256_loadu_si256((const __m256i*)(s + ptlen - 1));
        const __m256i eq_first = _mm256_cmpeq_epi8(first, block_first);
        const __m256i eq_last  = _mm256_cmpeq_epi8(last, block_last);
        uint32_t mask = _mm256_movemask_epi8(_mm256_and_si256(eq_first, eq_last));
        while (mask) {
            uint32_t bitpos = t_zeros(mask);
            if (memcmpeq_32(s + bitpos + 1, ptrn + 1, ptlen - 2)) {
                return i + bitpos;
            }
            mask &= (mask-1); /* clear lowest set bit */
        }
        s += 32;
    }
    return -1;
}

char *ascii_tolower_32(char *d, const char *s, size_t len){
    char *in, *out;
    uint32_t *str = (uint32_t *)s;
    uint32_t *dst = (uint32_t *)d;
    uint32_t mask1;
    uint32_t mask2;
    while(len >= 4){
            *str &= 0x7f7f7f7fu; /* make sure we are not GT 127 */
            mask1 = ((~(*str + 0x25252525u))&0x80808080u); /* test if it's lower than 91 */
            mask2 = (*str + 0x3f3f3f3fu)&0x80808080u; /* test if it's greater than 64 */
            mask1 = (mask1&mask2)>>2;
            *dst = *str | mask1; /* add 0x20 where uppercase detected */
            str++;
            dst++;
            len -=4;
        }
    in = (char *)str;
    out = (char *)dst;
    while(len > 0){
        if((unsigned)*in - 'A' < 26){
            *out = *in | 0x20;
        } else{
            *out = *in;
        }
        in++;
        out++;
        len--;
    }
    return d;
}
#endif // USE_INTRINSICS_AVX

char *ascii_tolower_64(char *d, const char *s, size_t len){
    char *in, *out;
    uint64_t *str = (uint64_t *)s;
    uint64_t *dst = (uint64_t *)d;
    uint64_t mask1;
    uint64_t mask2;
    while(len >= 8){
            *str &= 0x7f7f7f7f7f7f7f7fULL; /* make sure we are not GT 127 */
            mask1 = ((~(*str + 0x2525252525252525ULL))&0x8080808080808080ULL); /* test if it's lower than 91 */
            mask2 = (*str + 0x3f3f3f3f3f3f3f3fULL)&0x8080808080808080ULL; /* test if it's greater than 64 */
            mask1 = (mask1&mask2)>>2;
            *dst = *str | mask1; /* add 0x20 where uppercase detected */
            str++;
            dst++;
            len -=8;
        }
    in = (char *)str;
    out = (char *)dst;
    while(len > 0){
        if((unsigned)*in - 'A' < 26){
            *out = *in | 0x20;
        } else{
            *out = *in;
        }
        in++;
        out++;
        len--;
    }
    return d;
}

int itoa_aux(long long n, char* buf, int len)
{
    int i = 0;
    int sign = 0;
    char tmp;
    if (n < 0){
        sign = ~0;
        n = -n;
    }
    while(i < len){
        buf[i++] =  n % 10 + '0';
        n = n/10;
        if (n == 0) break;
    }
    if(sign){
        if(((len - 1) <= 0)&&(n != 0)) return -1;
        buf[i++] = '-';
    } else{
        if((len <= 0)&&(n != 0)) return -1;
    }
    /* reverse for the output */
    int start = 0;
    int end = i - 1;
    while(end > start){
        tmp = buf[end];
        buf[end--] = buf[start];
        buf[start++] = tmp;
    }
    return i;
}

/* Below follows an implementation of grisu3 algorithm for converting double to ascii.
    For details see:
	"Printing Floating-Point Numbers Quickly And Accurately with Integers"
	by Florian Loitsch, available at
	http://www.cs.tufts.edu/~nr/cs257/archive/florian-loitsch/printf.pdf
*/
#define U64_SIGN         0x8000000000000000ULL
#define U64_EXP_MASK     0x7FF0000000000000ULL
#define U64_FRACT_MASK   0x000FFFFFFFFFFFFFULL
#define U64_IMPLICIT_ONE 0x0010000000000000ULL
#define U64_EXP_POS      52
#define U64_EXP_BIAS     1075
#define FP_FRACT_SIZE    64
#define MIN_TARGET_EXP   -60
#define MASK32           0xFFFFFFFFULL

#define MIN(x,y) ((x) <= (y) ? (x) : (y))
#define MAX(x,y) ((x) >= (y) ? (x) : (y))

#define MIN_CACHED_EXP -348
#define CACHED_EXP_STEP 8

typedef struct fp_t
{
	uint64_t f;
	int e;
} fp_t;

typedef union{
    double d;
    uint64_t u64;
} dblhex;

/* fractions */
static const uint64_t pow_frac[] ={
    0xfa8fd5a0081c0288ULL, 0xbaaee17fa23ebf76ULL, 0x8b16fb203055ac76ULL,
    0xcf42894a5dce35eaULL, 0x9a6bb0aa55653b2dULL, 0xe61acf033d1a45dfULL,
    0xab70fe17c79ac6caULL, 0xff77b1fcbebcdc4fULL, 0xbe5691ef416bd60cULL,
    0x8dd01fad907ffc3cULL, 0xd3515c2831559a83ULL, 0x9d71ac8fada6c9b5ULL,
    0xea9c227723ee8bcbULL, 0xaecc49914078536dULL, 0x823c12795db6ce57ULL,
	0xc21094364dfb5637ULL, 0x9096ea6f3848984fULL, 0xd77485cb25823ac7ULL,
    0xa086cfcd97bf97f4ULL, 0xef340a98172aace5ULL, 0xb23867fb2a35b28eULL,
    0x84c8d4dfd2c63f3bULL, 0xc5dd44271ad3cdbaULL, 0x936b9fcebb25c996ULL,
    0xdbac6c247d62a584ULL, 0xa3ab66580d5fdaf6ULL, 0xf3e2f893dec3f126ULL,
    0xb5b5ada8aaff80b8ULL, 0x87625f056c7c4a8bULL, 0xc9bcff6034c13053ULL,
    0x964e858c91ba2655ULL, 0xdff9772470297ebdULL, 0xa6dfbd9fb8e5b88fULL,
    0xf8a95fcf88747d94ULL, 0xb94470938fa89bcfULL, 0x8a08f0f8bf0f156bULL,
    0xcdb02555653131b6ULL, 0x993fe2c6d07b7facULL, 0xe45c10c42a2b3b06ULL,
    0xaa242499697392d3ULL, 0xfd87b5f28300ca0eULL, 0xbce5086492111aebULL,
    0x8cbccc096f5088ccULL, 0xd1b71758e219652cULL, 0x9c40000000000000ULL,
    0xe8d4a51000000000ULL, 0xad78ebc5ac620000ULL, 0x813f3978f8940984ULL,
    0xc097ce7bc90715b3ULL, 0x8f7e32ce7bea5c70ULL, 0xd5d238a4abe98068ULL,
    0x9f4f2726179a2245ULL, 0xed63a231d4c4fb27ULL, 0xb0de65388cc8ada8ULL,
    0x83c7088e1aab65dbULL, 0xc45d1df942711d9aULL, 0x924d692ca61be758ULL,
    0xda01ee641a708deaULL, 0xa26da3999aef774aULL, 0xf209787bb47d6b85ULL,
    0xb454e4a179dd1877ULL, 0x865b86925b9bc5c2ULL, 0xc83553c5c8965d3dULL,
    0x952ab45cfa97a0b3ULL, 0xde469fbd99a05fe3ULL, 0xa59bc234db398c25ULL,
    0xf6c69a72a3989f5cULL, 0xb7dcbf5354e9beceULL, 0x88fcf317f22241e2ULL,
    0xcc20ce9bd35c78a5ULL, 0x98165af37b2153dfULL, 0xe2a0b5dc971f303aULL,
    0xa8d9d1535ce3b396ULL, 0xfb9b7cd9a4a7443cULL, 0xbb764c4ca7a44410ULL,
    0x8bab8eefb6409c1aULL, 0xd01fef10a657842cULL, 0x9b10a4e5e9913129ULL,
    0xe7109bfba19c0c9dULL, 0xac2820d9623bf429ULL, 0x80444b5e7aa7cf85ULL,
    0xbf21e44003acdd2dULL, 0x8e679c2f5e44ff8fULL, 0xd433179d9c8cb841ULL,
    0x9e19db92b4e31ba9ULL, 0xeb96bf6ebadf77d9ULL, 0xaf87023b9bf0ee6bULL
};

/* binary exponents */
static const int16_t exp_bin[] ={
    -1220, -1193, -1166, -1140, -1113, -1087, -1060, -1034, -1007, -980, -954, -927,
    -901, -874, -847, -821, -794, -768, -741, -715, -688, -661, -635, -608,
    -582, -555, -529, -502, -475, -449, -422, -396, -369, -343, -316, -289,
    -263, -236, -210, -183, -157, -130, -103, -77, -50, -24, 3, 30,
    56, 83, 109, 136, 162, 189, 216, 242, 269, 295, 322, 348,
    375, 402, 428, 455, 481, 508, 534, 561, 588, 614, 641, 667,
    694, 720, 747, 774, 800, 827, 853, 880, 907, 933, 960, 986,
    1013, 1039, 1066
};

/* decimal exponents */
static const int16_t exp_dec[] ={
    -348, -340, -332, -324, -316, -308, -300, -292, -284, -276, -268, -260,
    -252, -244, -236, -228, -220, -212, -204, -196, -188, -180, -172, -164,
    -156, -148, -140, -132, -124, -116, -108, -100, -92, -84, -76, -68,
    -60, -52, -44, -36, -28, -20, -12, -4,  4, 12,  20,  28,
    36, 44, 52, 60, 68, 76,  84,  92, 100, 108, 116, 124,
    132, 140, 148, 156, 164, 172, 180, 188, 196, 204, 212, 220,
    228, 236, 244, 252, 260, 268, 276, 284, 292, 300, 308, 316,
    324, 332, 340
};

static __inline int k_comp(int n)
{
    /*  The approximation to log10(2) ~ 1233/(2^12).
        It gives the correct results for our context  */
 	int tmp = n + 63;
 	int k = (tmp * 1233) / (1 << 12);
 	return tmp > 0 ? k + 1 : k;
}

static __inline int cached_pow(int exp, fp_t *p)
{
    int k = k_comp(exp);
	int i = (k-MIN_CACHED_EXP-1) / CACHED_EXP_STEP + 1;
	p->f = pow_frac[i];
	p->e = exp_bin[i];
	return exp_dec[i];
}

static __inline fp_t minus(fp_t x, fp_t y)
{
	fp_t d; d.f = x.f - y.f; d.e = x.e;
	return d;
}

static __inline fp_t multiply(fp_t x, fp_t y)
{
	uint64_t a, b, c, d, ac, bc, ad, bd, tmp;
	fp_t r;
	a = x.f >> 32; b = x.f & MASK32;
	c = y.f >> 32; d = y.f & MASK32;
	ac = a*c; bc = b*c;
	ad = a*d; bd = b*d;
	tmp = (bd >> 32) + (ad & MASK32) + (bc & MASK32);
	tmp += 1U << 31; /* round */
	r.f = ac + (ad >> 32) + (bc >> 32) + (tmp >> 32);
	r.e = x.e + y.e + 64;
	return r;
}

static __inline fp_t normalize_fp(fp_t n)
{
	while(!(n.f & 0xFFC0000000000000ULL)) { n.f <<= 10; n.e -= 10; }
	while(!(n.f & U64_SIGN)) { n.f <<= 1; --n.e; }
	return n;
}

static __inline fp_t double2fp(double d)
{
	fp_t fp;
	dblhex h = {d};
	if (!(h.u64 & U64_EXP_MASK)){
        fp.f = h.u64 & U64_FRACT_MASK; fp.e = 1 - U64_EXP_BIAS;
    }
	else{
        fp.f = (h.u64 & U64_FRACT_MASK) + U64_IMPLICIT_ONE;
        fp.e = (int)((h.u64 & U64_EXP_MASK) >> U64_EXP_POS) - U64_EXP_BIAS;
    }
	return fp;
}

/* pow10_cache[i] = 10^(i-1) */
static const unsigned int pow10_cache[] = { 0, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

static __inline int largest_pow10(uint32_t n, int n_bits, uint32_t *power)
{
	int guess = ((n_bits + 1) * 1233 >> 12) + 1/*skip first entry*/;
	if (n < pow10_cache[guess]) --guess; /* We don't have any guarantees that 2^n_bits <= n */
	*power = pow10_cache[guess];
	return guess;
}

static __inline int round_weed(char *buffer, int len, uint64_t wp_W,
	uint64_t delta, uint64_t rest, uint64_t ten_kappa, uint64_t ulp)
{
	uint64_t wp_Wup = wp_W - ulp;
	uint64_t wp_Wdown = wp_W + ulp;
	while(rest < wp_Wup && delta - rest >= ten_kappa
		&& (rest + ten_kappa < wp_Wup || wp_Wup - rest >= rest + ten_kappa - wp_Wup)){
		--buffer[len-1];
		rest += ten_kappa;
	}
	if (rest < wp_Wdown && delta - rest >= ten_kappa
		&& (rest + ten_kappa < wp_Wdown || wp_Wdown - rest > rest + ten_kappa - wp_Wdown))
		return 0;
	return 2*ulp <= rest && rest <= delta - 4*ulp;
}

static __inline int digit_gen(fp_t low, fp_t w, fp_t high, char *buffer, int *length, int *kappa)
{
	uint64_t unit = 1;
	fp_t too_low = { low.f - unit, low.e };
	fp_t too_high = { high.f + unit, high.e };
	fp_t unsafe_interval = minus(too_high, too_low);
	fp_t one = { 1ULL << -w.e, w.e };
	uint32_t p1 = (uint32_t)(too_high.f >> -one.e);
	uint64_t p2 = too_high.f & (one.f - 1);
	uint32_t div;
	*kappa = largest_pow10(p1, FP_FRACT_SIZE + one.e, &div);
	*length = 0;

	while(*kappa > 0){
		uint64_t rest;
		int digit = p1 / div;
		buffer[*length] = (char)('0' + digit);
		++*length;
		p1 %= div;
		--*kappa;
		rest = ((uint64_t)p1 << -one.e) + p2;
		if (rest < unsafe_interval.f) return round_weed(buffer, *length, minus(too_high, w).f,
                    unsafe_interval.f, rest, (uint64_t)div << -one.e, unit);
		div /= 10;
	}
	for(;;){
		int digit;
		p2 *= 10;
		unit *= 10;
		unsafe_interval.f *= 10;
		/* Integer division by one */
		digit = (int)(p2 >> -one.e);
		buffer[*length] = (char)('0' + digit);
		++*length;
		p2 &= one.f - 1;  /* Modulo by one */
		--*kappa;
		if (p2 < unsafe_interval.f) return round_weed(buffer, *length,
                    minus(too_high, w).f * unit, unsafe_interval.f, p2, one.f, unit);
	}
}

static int grisu3(double v, char *buffer, int *length, int *d_exp)
{
	int mk, kappa, success;
	fp_t f = double2fp(v);
	fp_t w = normalize_fp(f);
	/* normalize boundaries */
	fp_t t = { (f.f << 1) + 1, f.e - 1 };
	fp_t b_plus = normalize_fp(t);
	fp_t b_minus;
	fp_t c_mk; /* Cached power of ten: 10^-k */
    dblhex d = {v};
	if (!(d.u64 & U64_FRACT_MASK) && (d.u64 & U64_EXP_MASK) != 0){
        b_minus.f = (f.f << 2) - 1; b_minus.e =  f.e - 2;
    }
	else{
        /* lower boundary is closer */
        b_minus.f = (f.f << 1) - 1; b_minus.e = f.e - 1;
    }
	b_minus.f = b_minus.f << (b_minus.e - b_plus.e);
	b_minus.e = b_plus.e;

	mk = cached_pow(MIN_TARGET_EXP - FP_FRACT_SIZE - w.e, &c_mk);

	w = multiply(w, c_mk);
	b_minus = multiply(b_minus, c_mk);
	b_plus  = multiply(b_plus,  c_mk);

	success = digit_gen(b_minus, w, b_plus, buffer, length, &kappa);
	*d_exp = kappa - mk;
	return success;
}

static int write_exp(int e, char* buf, int rlen)
{
    int i = 0;
    if(rlen < 2) return -1;
    buf[i++] = 'e';
    rlen--;
    if(e < 0){
        rlen--;
        buf[i++] = '-';
        e = -e;
    }
    if(e >= 100){
        if(rlen < 3) return -1;
        buf[i++] = '0' + e/100;
        e %= 100;
        buf[i++] = '0' + e/10;
        e %= 10;
        buf[i++] = '0' + e;
    }
    else if (e >= 10){
        if(rlen < 2) return -1;
        buf[i++] = '0' + e/10;
        e %= 10;
        buf[i++] = '0' + e;
    }
    else{
        if(rlen < 1) return -1;
        buf[i++] = '0' + e;
    }
    return i;
}

int dtoa_aux(double v, char *dst, int rlen)
{
	int d_exp, len, success, decimals, i, rc;
    dblhex d = {v};
	char *s2 = dst;
	/* check for NaN or Inf*/
	if (((d.u64 << 1) > 0xFFE0000000000000ULL)||(d.u64 == U64_EXP_MASK))
        return -1; /* not allowed in JSON */
	/* Handle zero */
	if (!d.u64){
        if(rlen < 1) return -1;
        *dst = '0';
        return 1;
    }
	/* Handle negative values */
	if ((d.u64 & U64_SIGN) != 0){
        if(rlen < 2) return -1;
        rlen--;
        *s2++ = '-';
        v = -v;
        d.u64 ^= U64_SIGN;
    }
    if(rlen < 19) return -1; /* it may not be enough space - just exit */
	success = grisu3(v, s2, &len, &d_exp);
	/*  If grisu3 was not able to convert the number to a string (rare case),
        then use old snprintf */
	if (!success){
#ifdef _WIN32
        rc = _snprintf(s2, len, "%.17g", v) + (int)(s2 - dst);
#else
        rc = snprintf(s2, len, "%.17g", v) + (int)(s2 - dst);
#endif // _WIN32

        if (rc < 0) return -1;
        return rc + (int)(s2 - dst);
	}
    rlen -= len;
    /*  We now have an integer string of form "212354335" and a base-10 exponent for that number.
        Let's decide the best presentation for that string by whether to use a decimal point,
        or the scientific exponent notation 'e' */
    decimals = MIN(-d_exp, MAX(1, len-1));
	if (d_exp < 0 && len > 1){
		/* Add decimal point */
        if(rlen < 1) return -1;
        rlen--;
		for(i = 0; i < decimals; ++i)
            s2[len-i] = s2[len-i-1];
		s2[len++ - decimals] = '.';
		d_exp += decimals;
		/* Need scientific notation as well? */
		if(d_exp != 0){
            rc = write_exp(d_exp, s2+len, rlen);
            if(rc < 0) return -1;
            len += rc;
        }
	}
	else if(d_exp < 0 && d_exp >= -3){
        if(rlen < -d_exp + 1) return -1;
		/* Add decimal point for numbers of form 0.000x where it's shorter */
		for(i = 0; i < len; ++i)
            s2[len-d_exp-1-i] = s2[len-i-1];
		s2[0] = '.';
		for(i = 1; i < -d_exp; ++i)
            s2[i] = '0';
		len += -d_exp;
	}
	else if (d_exp < 0 || d_exp > 2){
        /* Add scientific notation */
        rc = write_exp(d_exp, s2+len, rlen);
        len += rc;
    }
    else if (d_exp > 0){
       /* Add zeros instead of scientific notation */
        if(rlen < d_exp) return -1;
        while(d_exp-- > 0) s2[len++] = '0';
        rlen -= d_exp;
    }
	return (int)(s2+len-dst);
}



