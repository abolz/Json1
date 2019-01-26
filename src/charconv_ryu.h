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

#include "charconv_common.h"
#include "charconv_pow10.h"

namespace charconv {
namespace ryu {

//==================================================================================================
// Implements the Ryu algorithm for (IEEE) binary to decimal floating-point conversion.
//
// This implementation is a slightly modified version of the
// implementation by Ulf Adams which can be obtained from
// https://github.com/ulfjack/ryu
//
// The license can be found below.
//
// References:
//
// [1]  Adams, "Ryu: fast float-to-string conversion",
//      PLDI 2018 Proceedings of the 39th ACM SIGPLAN Conference on Programming Language Design and Implementation Pages 270-282
//      https://dl.acm.org/citation.cfm?id=3192369
//==================================================================================================

/*
Copyright 2018 Ulf Adams

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

inline int BitLength(uint64_t x) // DEBUG only
{
    int n = 0;
    for ( ; x != 0; x >>= 1, ++n)
    {
    }
    return n;
}

// e == 0 ? 1 : ceil(log_2(5^e))
inline int Pow5BitLength(int e)
{
    CC_ASSERT(e >= 0);
    CC_ASSERT(e <= 1500); // Only tested for e <= 1500
    return static_cast<int>((static_cast<uint32_t>(e) * 1217359) >> 19) + 1;
}

// floor(log_10(2^e))
inline int Log10Pow2(int e)
{
    CC_ASSERT(e >= 0);
    CC_ASSERT(e <= 1500); // Only tested for e <= 1500
    return static_cast<int>((static_cast<uint32_t>(e) * 78913) >> 18);
}

// floor(log_10(5^e))
inline int Log10Pow5(int e)
{
    CC_ASSERT(e >= 0);
    CC_ASSERT(e <= 1500); // Only tested for e <= 1500
    return static_cast<int>((static_cast<uint32_t>(e) * 732923) >> 20);
}

//==================================================================================================
// IEEE double-precision implementation
//==================================================================================================

constexpr int kDoublePow5InvBitLength = 128;
constexpr int kDoublePow5BitLength = 128;

inline uint64_t MulShift(uint64_t m, Uint64x2 const* mul, int j)
{
    CC_ASSERT((m >> 55) == 0); // m is maximum 55 bits
    CC_ASSERT((BitLength(mul->hi) + 64 + 55) - j <= 64);

#if CC_HAS_UINT128
    __extension__ using uint128_t = unsigned __int128;

    uint128_t const b0 = uint128_t{m} * mul->lo;
    uint128_t const b2 = uint128_t{m} * mul->hi;

    return static_cast<uint64_t>(((b0 >> 64) + b2) >> (j - 64));
#else
    auto b0 = Mul128(m, mul->lo);
    auto b2 = Mul128(m, mul->hi);

    b2.lo += b0.hi;
    b2.hi += (b2.lo < b0.hi);

    return ShiftRight128(b2, j - 64);
#endif
}

inline void MulShiftAll(uint64_t mv, uint64_t mp, uint64_t mm, Uint64x2 const* mul, int j, uint64_t* vr, uint64_t* vp, uint64_t* vm)
{
    //
    // TODO:
    //
    // Optimize (at least for 32-bit platforms)?!?!
    // As in https://github.com/ulfjack/ryu/blob/2ec9ead2400afd7ff91a563fbc46ec62984fa3d0/ryu/d2s.c#L172-L200
    //

    *vr = MulShift(mv, mul, j);
    *vp = MulShift(mp, mul, j);
    *vm = MulShift(mm, mul, j);
}

#if CC_32_BIT_PLATFORM

// On x86 platforms, compilers typically generate calls to library
// functions for 64-bit divisions, even if the divisor is a constant.
//
// E.g.:
// https://bugs.llvm.org/show_bug.cgi?id=37932
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=17958
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37443
//
// The functions here perform division-by-constant using multiplications
// in the same way as 64-bit compilers would do.
//
// NB:
// The multipliers and shift values are the ones generated by clang x64
// for expressions like x/5, x/10, etc.

inline uint64_t Div5(uint64_t x) {
    return Mul128(x, 0xCCCCCCCCCCCCCCCDu).hi >> 2;
}

inline uint64_t Div10(uint64_t x) {
    return Mul128(x, 0xCCCCCCCCCCCCCCCDu).hi >> 3;
}

inline uint64_t Div100(uint64_t x) {
    return Mul128(x >> 2, 0x28F5C28F5C28F5C3u).hi >> 2;
}

inline uint64_t Div1e8(uint64_t x) {
    return Mul128(x, 0xABCC77118461CEFDu).hi >> 26;
}

#else // CC_32_BIT_PLATFORM

inline uint64_t Div5(uint64_t x) {
    return x / 5;
}

inline uint64_t Div10(uint64_t x) {
    return x / 10;
}

inline uint64_t Div100(uint64_t x) {
    return x / 100;
}

inline uint64_t Div1e8(uint64_t x) {
    return x / 100000000;
}

#endif // CC_32_BIT_PLATFORM

inline int Pow5Factor(uint64_t value)
{
    // For 64-bit integers: result <= 27
    // Since value here has at most 55-bits: result <= 23

    int factor = 0;
    for (;;) {
        CC_ASSERT(value != 0);
        CC_ASSERT(factor <= 23);

        uint64_t const q = Div5(value);
        uint32_t const r = static_cast<uint32_t>(value - 5 * q);
        if (r != 0)
            return factor;
        value = q;
        ++factor;
    }
}

inline bool MultipleOfPow5(uint64_t value, int p)
{
    return Pow5Factor(value) >= p;
}

inline bool MultipleOfPow2(uint64_t value, int p)
{
    CC_ASSERT(p >= 0);
    CC_ASSERT(p <= 63);

    //return (value << (64 - p)) == 0;
    return (value & ((uint64_t{1} << p) - 1)) == 0;
}

struct DoubleToDecimalResult {
    uint64_t digits;
    int exponent;
};

inline DoubleToDecimalResult DoubleToDecimal(double value)
{
    CC_ASSERT(Double(value).IsFinite());
    CC_ASSERT(value >= 0);

    //
    // Step 1:
    // Decode the floating point number, and unify normalized and subnormal cases.
    //

    Double const ieee_value(value);

    // Decode bits into mantissa, and exponent.
    uint64_t const ieeeMantissa = ieee_value.PhysicalSignificand();
    uint64_t const ieeeExponent = ieee_value.PhysicalExponent();

    uint64_t m2;
    int e2;
    if (ieeeExponent == 0) {
        if (ieeeMantissa == 0) // +/- 0.0
            return {0, 0};
        m2 = ieeeMantissa;
        e2 = 1;
    } else {
        m2 = Double::HiddenBit | ieeeMantissa;
        e2 = static_cast<int>(ieeeExponent);
    }

    bool const even = (m2 & 1) == 0;
    bool const acceptBounds = even;

    //
    // Step 2:
    // Determine the interval of legal decimal representations.
    //

    // We subtract 2 so that the bounds computation has 2 additional bits.
    e2 -= Double::ExponentBias + 2;

    uint64_t const mv = 4 * m2;
    uint64_t const mp = mv + 2;
    uint32_t const mmShift = (ieeeMantissa != 0 || ieeeExponent <= 1) ? 1 : 0;
    uint64_t const mm = mv - 1 - mmShift;

    //
    // Step 3:
    // Convert to a decimal power base using 128-bit arithmetic.
    //

    int e10;

    uint64_t vm;
    uint64_t vr;
    uint64_t vp;

    bool vmIsTrailingZeros = false;
    bool vrIsTrailingZeros = false;
    bool vpIsTrailingZeros = false;

    if (e2 >= 0)
    {
        // I tried special-casing q == 0, but there was no effect on performance.
        // q = max(0, log_10(2^e2))
        int const q = Log10Pow2(e2) - (e2 > 3); // exponent
        CC_ASSERT(q >= 0);
        int const k = kDoublePow5InvBitLength + Pow5BitLength(q) - 1;
        int const j = -e2 + q + k - (q == 0); // shift
        CC_ASSERT(j >= 115);

        e10 = q;

		// mul = 5^-q
        auto const mul = ComputePow10Significand(-q);
        MulShiftAll(mv, mp, mm, &mul, j, &vr, &vp, &vm);

        // 22 = floor(log_5(2^53))
        // 23 = floor(log_5(2^(53+2)))
        if (q <= 22)
        {
            // This should use q <= 22, but I think 21 is also safe. Smaller values
            // may still be safe, but it's more difficult to reason about them.
            // Only one of mp, mv, and mm can be a multiple of 5, if any.
            if (mv - 5 * Div5(mv) == 0)
            {
                vrIsTrailingZeros = MultipleOfPow5(mv, q);
            }
            else if (acceptBounds)
            {
                // Same as min(e2 + (~mm & 1), Pow5Factor(mm)) >= q
                // <=> e2 + (~mm & 1) >= q && Pow5Factor(mm) >= q
                // <=> true && Pow5Factor(mm) >= q, since e2 >= q.
                vmIsTrailingZeros = MultipleOfPow5(mm, q);
            }
            else
            {
                // Same as min(e2 + 1, Pow5Factor(mp)) >= q.
                vpIsTrailingZeros = MultipleOfPow5(mp, q);
            }
        }
    }
    else
    {
        // q = max(0, log_10(5^-e2))
        int const q = Log10Pow5(-e2) - (-e2 > 1);
        CC_ASSERT(q >= 0);
        int const i = -e2 - q; // -exponent
        CC_ASSERT(i > 0);
        int const k = Pow5BitLength(i) - kDoublePow5BitLength;
        int const j = q - k; // shift
        CC_ASSERT(j >= 114);

        e10 = -i;

        // mul = 5^i
        auto const mul = ComputePow10Significand(i);
        MulShiftAll(mv, mp, mm, &mul, j, &vr, &vp, &vm);

        if (q <= 1)
        {
            // {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
            // mv = 4 * m2, so it always has at least two trailing 0 bits.
            vrIsTrailingZeros = true;

            if (acceptBounds)
            {
                // mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
                vmIsTrailingZeros = (mmShift == 1);
            }
            else
            {
                // mp = mv + 2, so it always has at least one trailing 0 bit.
                vpIsTrailingZeros = true;
            }
        }
        else if (q <= Double::SignificandSize + 2)
        {
            // TODO(ulfjack): Use a tighter bound here.

            // We need to compute min(ntz(mv), Pow5Factor(mv) - e2) >= q-1
            // <=> ntz(mv) >= q-1  &&  Pow5Factor(mv) - e2 >= q-1
            // <=> ntz(mv) >= q-1
            // <=> mv & ((1 << (q-1)) - 1) == 0
            // We also need to make sure that the left shift does not overflow.
            vrIsTrailingZeros = MultipleOfPow2(mv, q - 1);
        }
    }

    //
    // Step 4:
    // Find the shortest decimal representation in the interval of legal representations.
    //

    // On average, we remove ~2 digits.

    vp -= vpIsTrailingZeros;

    uint64_t output;

    if (vmIsTrailingZeros || vrIsTrailingZeros)
    {
        // General case, which happens rarely (<1%).

        uint32_t lastRemovedDigit = 0;

        bool vrPrevIsTrailingZeros = vrIsTrailingZeros;

        for (;;)
        {
            uint64_t const vmDiv10 = Div10(vm);
            uint64_t const vpDiv10 = Div10(vp);
            if (vmDiv10 >= vpDiv10)
                break;

            uint32_t const vmMod10 = static_cast<uint32_t>(vm - 10 * vmDiv10);
            vmIsTrailingZeros &= vmMod10 == 0;
            vrPrevIsTrailingZeros &= lastRemovedDigit == 0;

            uint64_t const vrDiv10 = Div10(vr);
            uint32_t const vrMod10 = static_cast<uint32_t>(vr - 10 * vrDiv10);
            lastRemovedDigit = vrMod10;

            vm = vmDiv10;
            vr = vrDiv10;
            vp = vpDiv10;
            ++e10;
        }

        if (vmIsTrailingZeros)
        {
            for (;;)
            {
                uint64_t const vmDiv10 = Div10(vm);
                uint32_t const vmMod10 = static_cast<uint32_t>(vm - 10 * vmDiv10);
                if (vmMod10 != 0)
                    break;

                vrPrevIsTrailingZeros &= lastRemovedDigit == 0;

                uint64_t const vrDiv10 = Div10(vr);
                uint32_t const vrMod10 = static_cast<uint32_t>(vr - 10 * vrDiv10);
                lastRemovedDigit = vrMod10;

                vm = vmDiv10;
                vr = vrDiv10;
                //vp = Div10(vp);
                ++e10;
            }
        }

        bool roundUp = lastRemovedDigit >= 5;
        if (lastRemovedDigit == 5 && vrPrevIsTrailingZeros)
        {
            // Halfway case: The number ends in ...500...00.
            roundUp = static_cast<uint32_t>(vr) % 2 != 0;
        }

        // We need to take vr+1 if vr is outside bounds...
        // or we need to round up.
        bool const inc = (vr == vm && !(acceptBounds && vmIsTrailingZeros)) || roundUp;

        output = vr + (inc ? 1 : 0);
    }
    else
    {
        // Specialized for the common case (>99%).

        bool roundUp = false;

#if CC_32_BIT_PLATFORM
        uint64_t const vmDiv100 = Div100(vm);
        uint64_t const vpDiv100 = Div100(vp);
        if (vmDiv100 < vpDiv100)
        {
            uint64_t const vrDiv100 = Div100(vr);
            uint32_t const vrMod100 = static_cast<uint32_t>(vr - 100 * vrDiv100);
            roundUp = vrMod100 >= 50;

            vm = vmDiv100;
            vr = vrDiv100;
            vp = vpDiv100;
            e10 += 2;
        }
#endif

        for (;;)
        {
            uint64_t const vmDiv10 = Div10(vm);
            uint64_t const vpDiv10 = Div10(vp);
            if (vmDiv10 >= vpDiv10)
                break;

            uint64_t const vrDiv10 = Div10(vr);
            uint32_t const vrMod10 = static_cast<uint32_t>(vr - 10 * vrDiv10);
            roundUp = vrMod10 >= 5;

            vm = vmDiv10;
            vr = vrDiv10;
            vp = vpDiv10;
            ++e10;
        }

        // We need to take vr+1 if vr is outside bounds...
        // or we need to round up.
        bool const inc = vr == vm || roundUp;

        output = vr + (inc ? 1 : 0);
    }

    return {output, e10};
}

} // namespace ryu
} // namespace charconv
