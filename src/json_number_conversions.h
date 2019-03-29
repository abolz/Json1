// Copyright 2018 Alexander Bolz
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

#include "charconv/double.h"

#ifndef JSON_ASSERT
#define JSON_ASSERT(X) assert(X)
#endif

//==================================================================================================
//
//==================================================================================================

namespace json {

#if defined(__GNUC__) // && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif

struct int53_t;
struct uint53_t;

struct int53_t
{
    union {
        int64_t s : 53;
        uint64_t u : 53;
    };

    int53_t(signed int v) : s(v) {}
#if LONG_MAX == INT_MAX
    int53_t(signed long v) : s(v) {}
#else
    explicit int53_t(signed long v) : s(v) {}
#endif
    explicit int53_t(signed long long v) : s(v) {}
    explicit int53_t(uint53_t v);

#if LONG_MAX == LLONG_MAX
    operator signed long() const { return s; }
#endif
    operator signed long long() const { return s; }
};

struct uint53_t
{
    union {
        int64_t s : 53;
        uint64_t u : 53;
    };

    uint53_t(unsigned int v) : u(v) {}
#if LONG_MAX == INT_MAX
    uint53_t(unsigned long v) : u(v) {}
#else
    explicit uint53_t(unsigned long v) : u(v) {}
#endif
    explicit uint53_t(unsigned long long v) : u(v) {}
    explicit uint53_t(int53_t v);

#if LONG_MAX == LLONG_MAX
    operator unsigned long() const { return u; }
#endif
    operator unsigned long long() const { return u; }
};

inline int53_t::int53_t(uint53_t v) : u(v.u) {}
inline uint53_t::uint53_t(int53_t v) : s(v.s) {}

#if defined(__GNUC__) // && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

} // namespace json

//==================================================================================================
// Number conversions
//==================================================================================================

namespace json {
namespace numbers {

//
// https://tc39.github.io/ecma262/#sec-touint32
//
// The abstract operation ToUint32 converts x to one of 2^32 integer values in the range
// 0 through 2^32 - 1, inclusive.
// This abstract operation functions as follows:
//
//  1.  [...]
//  2.  If x is NaN, +0, -0, +Infinity, or -Infinity, return +0.
//  3.  Let int be the mathematical value that is the same sign as x and whose magnitude is
//      floor(abs(x)).
//  4.  Let int32bit be int modulo 2^32.
//  5.  Return int32bit.
//
inline uint32_t ToUint32(double x)
{
    using charconv::Double;
    Double const d(x);

    // Assume that x is a normalized floating-point number.
    // The special cases subnormal/zero and nan/inf are actually handled below
    // in the branches 'e <= -p' and 'e >= 32'.

    auto const f = d.NormalizedSignificand();
    auto const e = d.NormalizedExponent();

    uint32_t bits32; // = floor(abs(x)) mod 2^32
    if (e <= -Double::SignificandSize)
    {
        // x = f / 2^-e < 1.
        return 0u;
    }
    else if (e < 0)
    {
        // x = f / 2^-e.
        // Discard the fractional bits (floor).
        // Since 0 < -e < 53, the (64-bit) right shift is well defined.
        bits32 = static_cast<uint32_t>(f >> -e);
    }
    else if (e < 32)
    {
        // x = f * 2^e.
        // Since 0 <= e < 32, the (32-bit) left shift is well defined.
        bits32 = static_cast<uint32_t>(f) << e;
    }
    else
    {
        // x = f * 2^e.
        // The lower 32 bits of x are definitely 0.
        return 0u;
    }

    return d.SignBit() ? 0 - bits32 : bits32;
}

// FIXME(?)
// Technically, casting unsigned to signed integers is implementation-defined (if out of range).

//
// https://tc39.github.io/ecma262/#sec-toint32
//
// The abstract operation ToInt32 converts x to one of 2^32 integer values in the range
// -2^31 through 2^31 - 1, inclusive.
// This abstract operation functions as follows:
//
//  1.  [...]
//  2.  If x is NaN, +0, -0, +Infinity, or -Infinity, return +0.
//  3.  Let int be the mathematical value that is the same sign as x and whose magnitude is
//      floor(abs(x)).
//  4.  Let int32bit be int modulo 2^32.
//  5.  If int32bit >= 2^31, return int32bit - 2^32; otherwise return int32bit.
//
inline int32_t ToInt32(double x)
{
    return static_cast<int32_t>(ToUint32(x));
}

inline uint16_t ToUint16(double x)
{
    return static_cast<uint16_t>(ToUint32(x));
}

inline int16_t ToInt16(double x)
{
    return static_cast<int16_t>(ToUint16(x));
}

inline uint8_t ToUint8(double x)
{
    return static_cast<uint8_t>(ToUint32(x));
}

inline int8_t ToInt8(double x)
{
    return static_cast<int8_t>(ToUint8(x));
}

//
// https://tc39.github.io/ecma262/#sec-touint8clamp
//
// The abstract operation ToUint8Clamp converts x to one of 2^8 integer values in the range
// 0 through 255, inclusive.
// This abstract operation functions as follows:
//
//  1.  [...]
//  2.  If x is NaN, return +0.
//  3.  If x <= 0, return +0.
//  4.  If x >= 255, return 255.
//  5.  Let f be floor(x).
//  6.  If f + 0.5 < x, return f + 1.
//  7.  If x < f + 0.5, return f.
//  8.  If f is odd, return f + 1.
//  9.  Return f.
//
inline uint8_t ToUint8Clamp(double x)
{
#if 1
    // The following test is equivalent to (std::isnan(x) || x <= 0.5),
    // i.e. it catches NaN's and subnormal numbers.
    if (!(x > 0.5))
        return 0;
    if (x > 254.5)
        return 255;

    // Truncate to integer.
    int32_t t = static_cast<int32_t>(x);

    // Then do the rounding.
    // Note: (x - t) is exact here (as would be (t + 0.5)).
    double const fraction = x - t;

    // Round towards +Inf
    t += (fraction >= 0.5);
    if (fraction == 0.5)
    {
        // Round to nearest-even
        t &= ~1;
    }

    JSON_ASSERT(t >= 0);
    JSON_ASSERT(t <= 255);
    return static_cast<uint8_t>(static_cast<uint32_t>(t));
#else
    using Double = charconv::Double;
    Double const d(x);

    if (d.bits <= 0x3FE0000000000000ull) // +0.0 <= x <= 1/2
    {
        return 0;
    }
    else if (d.bits <= 0x406FD00000000000ull) // 0.5 < x <= 254.5
    {
        // x = f * 2^e is a normalized floating-point number.
        auto const f = d.NormalizedSignificand();
        auto const e = d.NormalizedExponent();
        JSON_ASSERT(e >= -1 - (Double::SignificandSize - 1));
        JSON_ASSERT(e <   8 - (Double::SignificandSize - 1));
        // 44 < -e <= 53

        uint64_t const high = uint64_t{1} << -e;
        uint64_t const mask = high - 1;
        uint64_t const half = high >> 1;
        uint64_t const fraction = f & mask;

        uint32_t t = static_cast<uint32_t>(f >> -e);

        // Round towards +Inf
        t += (fraction >= half);
        if (fraction == half)
        {
            // Round towards nearest-even.
            t &= ~1u;
        }

        JSON_ASSERT(t <= 255);
        return static_cast<uint8_t>(t);
    }
    else if (d.bits <= 0x7FF0000000000000ull) // 254.5 < x <= +Inf
    {
        return 255;
    }
    else // x <= -0.0 or NaN(x)
    {
        return 0;
    }
#endif
}

//
// The abstract operation ToUint53 converts x to one of 2^53 integer values in the range
// 0 through 2^53 - 1, inclusive.
// This abstract operation functions as follows:
//
//  1.  [...]
//  2.  If x is NaN, +0, -0, +Infinity, or -Infinity, return +0.
//  3.  Let int be the mathematical value that is the same sign as x and whose magnitude is
//      floor(abs(x)).
//  4.  Let int53bit be int modulo 2^53.
//  5.  Return int53bit.
//
inline uint53_t ToUint53(double x)
{
    using charconv::Double;
    Double const d(x);

    auto const f = d.NormalizedSignificand();
    auto const e = d.NormalizedExponent();

    uint64_t bits64;
    if (e <= -Double::SignificandSize)
    {
        return 0u;
    }
    else if (e < 0)
    {
        bits64 = f >> -e;
    }
    else if (e < 53)
    {
        bits64 = f << e;
    }
    else
    {
        return 0u;
    }

    return static_cast<uint53_t>(d.SignBit() ? (0 - bits64) : bits64);
}

inline int53_t ToInt53(double x)
{
    return static_cast<int53_t>(ToUint53(x));
}

// Returns whether x is an integer and in the range [-2^53+1, 2^53-1]
// and stores the integral value of x in result.
inline int64_t DoubleToInt53(double x, bool& is_int53)
{
    using charconv::Double;

    constexpr double kMinInt = -9007199254740991.0; // -2^53 + 1
    constexpr double kMaxInt =  9007199254740991.0; //  2^53 - 1

    if (x < kMinInt || x > kMaxInt || Double(x).IsMinusZero())
    {
        is_int53 = false;
        return 0;
    }

    int64_t const i = static_cast<int64_t>(x);
    is_int53 = (x == static_cast<double>(i));
    return i;
}

} // namespace numbers
} // namespace json
