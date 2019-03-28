// Copyright 2017 Alexander Bolz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "double.h"

#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>
#include <limits>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

#ifndef CC_ASSERT
#define CC_ASSERT(X) assert(X)
#endif

#ifndef CC_NEVER_INLINE
#if defined(__GNUC__)
#define CC_NEVER_INLINE __attribute__((noinline)) inline
#elif defined(_MSC_VER)
#define CC_NEVER_INLINE __declspec(noinline) inline
#else
#define CC_NEVER_INLINE inline
#endif
#endif

#if defined(_M_IX86) || defined(_M_ARM) || defined(__i386__) || defined(__arm__)
#define CC_32_BIT_PLATFORM 1
#endif

#if defined(__SIZEOF_INT128__)
#define CC_HAS_UINT128 1
#elif defined(_MSC_VER) && defined(_M_X64)
#define CC_HAS_64_BIT_INTRINSICS 1
#endif

namespace charconv {

//==================================================================================================
//
//==================================================================================================

struct Uint64x2 {
    uint64_t hi;
    uint64_t lo;
};

inline Uint64x2 Mul128(uint64_t a, uint64_t b)
{
#if CC_HAS_UINT128
    __extension__ using uint128_t = unsigned __int128;

    uint128_t const product = uint128_t{a} * b;

    uint64_t const lo = static_cast<uint64_t>(product);
    uint64_t const hi = static_cast<uint64_t>(product >> 64);
    return {hi, lo};
#elif CC_HAS_64_BIT_INTRINSICS
    uint64_t hi;
    uint64_t lo = _umul128(a, b, &hi);
    return {hi, lo};
#else
    uint32_t const aLo = static_cast<uint32_t>(a);
    uint32_t const aHi = static_cast<uint32_t>(a >> 32);
    uint32_t const bLo = static_cast<uint32_t>(b);
    uint32_t const bHi = static_cast<uint32_t>(b >> 32);

    uint64_t const b00 = uint64_t{aLo} * bLo;
    uint64_t const b01 = uint64_t{aLo} * bHi;
    uint64_t const b10 = uint64_t{aHi} * bLo;
    uint64_t const b11 = uint64_t{aHi} * bHi;

    uint32_t const b00Lo = static_cast<uint32_t>(b00);
    uint32_t const b00Hi = static_cast<uint32_t>(b00 >> 32);

    uint64_t const mid1 = b10 + b00Hi;
    uint32_t const mid1Lo = static_cast<uint32_t>(mid1);
    uint32_t const mid1Hi = static_cast<uint32_t>(mid1 >> 32);

    uint64_t const mid2 = b01 + mid1Lo;
    uint32_t const mid2Lo = static_cast<uint32_t>(mid2);
    uint32_t const mid2Hi = static_cast<uint32_t>(mid2 >> 32);

    uint64_t const hi = b11 + mid1Hi + mid2Hi;
    uint64_t const lo = (uint64_t{mid2Lo} << 32) | b00Lo;
    return {hi, lo};
#endif
}

inline uint64_t ShiftRight128(uint64_t lo, uint64_t hi, unsigned char dist)
{
    // TODO:
    // Move into charconv_ryu? The 32-bit path actually requires dist >= 32...

    // For the __shiftright128 intrinsic, the shift value is always modulo 64.
    // In the current implementation of the double-precision version of Ryu, the
    // shift value is always < 64.
    // Check this here in case a future change requires larger shift values. In
    // this case this function needs to be adjusted.
    CC_ASSERT(dist >= 56); // 56: MulShiftAll fallback, 57: otherwise.
    CC_ASSERT(dist <= 63);

#if CC_HAS_UINT128
    __extension__ using uint128_t = unsigned __int128;

    return static_cast<uint64_t>(((uint128_t{hi} << 64) | lo) >> dist);
#elif CC_HAS_64_BIT_INTRINSICS
    return __shiftright128(lo, hi, dist);
#else
#if CC_32_BIT_PLATFORM
    // Avoid a 64-bit shift by taking advantage of the range of shift values.
    // We know that 0 <= 64 - dist < 32 and 0 <= dist - 32 < 32.
    int const lshift = -dist & 31; // == (64 - dist) % 32
    int const rshift =  dist & 31; // == (dist - 32) % 32
#if defined(_MSC_VER) && !defined(__clang__)
    return __ll_lshift(hi, lshift) | (static_cast<uint32_t>(lo >> 32) >> rshift);
#else
    return (hi << lshift) | (static_cast<uint32_t>(lo >> 32) >> rshift);
#endif
#else
    int const lshift = -dist & 63;
    int const rshift =  dist & 63;
    return (hi << lshift) | (lo >> rshift);
#endif
#endif
}

// Returns the number of leading 0-bits in x, starting at the most significant bit position.
// If x is 0, the result is undefined.
inline int CountLeadingZeros64(uint64_t x)
{
    CC_ASSERT(x != 0);

#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clzll(x);
#elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
    return static_cast<int>(_CountLeadingZeros64(x));
#elif defined(_MSC_VER) && defined(_M_X64)
    return static_cast<int>(__lzcnt64(x));
#elif defined(_MSC_VER) && defined(_M_IX86)
    int lz = static_cast<int>( __lzcnt(static_cast<uint32_t>(x >> 32)) );
    if (lz == 32) {
        lz += static_cast<int>( __lzcnt(static_cast<uint32_t>(x)) );
    }
    return lz;
#else
    int lz = 0;
    while ((x >> 63) == 0) {
        x <<= 1;
        ++lz;
    }
    return lz;
#endif
}

// FIXME(?)
// Technically, right shifting negative integers is implementation-defined.

// Returns: floor(log_2(5^e))
inline int FloorLog2Pow5(int e)
{
    CC_ASSERT(e >= -1764);
    CC_ASSERT(e <=  1763);
    return (e * 1217359) >> 19;
}

// Returns: ceil(log_2(5^e))
inline int CeilLog2Pow5(int e)
{
    CC_ASSERT(e >= -1764);
    CC_ASSERT(e <=  1763);
    return (e * 1217359 + ((1 << 19) - 1)) >> 19;
}

// Returns: floor(log_2(10^e))
// Equivalent to: e + FloorLog2Pow5(e)
inline int FloorLog2Pow10(int e)
{
    //CC_ASSERT(e >= -1233);
    //CC_ASSERT(e <=  1232);
    //return (e * 1741647) >> 19;
    return e + FloorLog2Pow5(e);
}

// Returns: ceil(log_2(10^e))
// Equivalent to: e + CeilLog2Pow5(e)
inline int CeilLog2Pow10(int e)
{
    //CC_ASSERT(e >= -1233);
    //CC_ASSERT(e <=  1232);
    //return (e * 1741647 + ((1 << 19) - 1)) >> 19;
    return e + CeilLog2Pow5(e);
}

// Returns: floor(log_5(2^e))
inline int FloorLog5Pow2(int e)
{
    CC_ASSERT(e >= -1831);
    CC_ASSERT(e <=  1831);
    return (e * 225799) >> 19;
}

// Returns: ceil(log_5(2^e))
inline int CeilLog5Pow2(int e)
{
    CC_ASSERT(e >= -1831);
    CC_ASSERT(e <=  1831);
    return (e * 225799 + ((1 << 19) - 1)) >> 19;
}

// Returns: floor(log_5(10^e))
// Equivalent to: e + FloorLog5Pow2(e)
inline int FloorLog5Pow10(int e)
{
    CC_ASSERT(e >= -1831);
    CC_ASSERT(e <=  1831);
    return (e * 750087) >> 19;
}

// Returns: ceil(log_5(10^e))
// Equivalent to: e + CeilLog5Pow2(e)
inline int CeilLog5Pow10(int e)
{
    CC_ASSERT(e >= -1831);
    CC_ASSERT(e <=  1831);
    return (e * 750087 + ((1 << 19) - 1)) >> 19;
}

// Returns: floor(log_10(2^e))
inline int FloorLog10Pow2(int e)
{
    CC_ASSERT(e >= -2620);
    CC_ASSERT(e <=  2620);
    return (e * 315653) >> 20;
}

// Returns: ceil(log_10(2^e))
inline int CeilLog10Pow2(int e)
{
    CC_ASSERT(e >= -2620);
    CC_ASSERT(e <=  2620);
    return (e * 315653 + ((1 << 20) - 1)) >> 20;
}

// Returns: floor(log_10(5^e))
inline int FloorLog10Pow5(int e)
{
    CC_ASSERT(e >= -2620);
    CC_ASSERT(e <=  2620);
    return (e * 732923) >> 20;
}

// Returns: ceil(log_10(5^e))
inline int CeilLog10Pow5(int e)
{
    CC_ASSERT(e >= -2620);
    CC_ASSERT(e <=  2620);
    return (e * 732923 + ((1 << 20) - 1)) >> 20;
}

} // namespace charconv
