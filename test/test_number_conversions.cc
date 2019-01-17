#define __STDC_FORMAT_MACROS
#include "catch.hpp"
#include "../src/json.h"
#include "../src/json_numbers.h"

#include <algorithm>
#include <limits>
#include <stdint.h>
#include <inttypes.h>

struct DoubleParts {
    bool S;
    uint64_t E;
    uint64_t F;
};

static constexpr int      SignificandSize         = 53;                                    // = p   (includes the hidden bit)
static constexpr int      PhysicalSignificandSize = SignificandSize - 1;                   // = p-1 (excludes the hidden bit)
static constexpr uint64_t HiddenBit               = uint64_t{1} << (SignificandSize - 1);  // = 2^(p-1)
static constexpr uint64_t SignificandMask         = HiddenBit - 1;                         // = 2^(p-1) - 1
static constexpr uint64_t ExponentMask            = uint64_t{2 * 1024 - 1} << PhysicalSignificandSize;
static constexpr uint64_t SignMask                = ~(~uint64_t{0} >> 1);

static inline DoubleParts GetDoubleParts(double d)
{
    static_assert(sizeof(double) == sizeof(uint64_t), "size mismatch");

    uint64_t bits;
    std::memcpy(&bits, &d, sizeof(double));

    DoubleParts parts;
    parts.S = (bits & SignMask) != 0 ? 1 : 0;
    parts.E = (bits & ExponentMask) >> PhysicalSignificandSize;
    parts.F = (bits & SignificandMask);
    return parts;
}

static inline double DoubleFromParts(DoubleParts const& parts)
{
    uint64_t bits = 0;
    if (parts.S)
        bits |= SignMask;
    bits |= parts.E << PhysicalSignificandSize;
    bits |= parts.F;

    double d;
    std::memcpy(&d, &bits, sizeof(double));
    return d;
}

static inline bool operator==(DoubleParts const& lhs, DoubleParts const& rhs) {
    return lhs.S == rhs.S && lhs.E == rhs.E && lhs.F == rhs.F;
}
static inline bool operator!=(DoubleParts const& lhs, DoubleParts const& rhs) {
    return !(lhs == rhs);
}
static inline std::ostream& operator<<(std::ostream& os, DoubleParts const& parts) {
    char buf[128];
    snprintf(buf, 64, "%g (%d,%d,0x%016" PRIX64 ")", DoubleFromParts(parts), static_cast<int>(parts.S), static_cast<int>(parts.E), parts.F);
    os << buf;
    return os;
}

static const double Arr[] = {
    -0.0,
    0.0,
    -1.0,
    1.0,

    -126.0,
    126.0,
    -126.5,
    126.5,
    -127.0,
    127.0,
    -127.5,
    127.5,
    -128.0, // 2^7
    128.0, // 2^7
    -128.5,
    128.5,
    -129.0,
    129.0,

    -254.0,
    254.0,
    -254.5,
    254.5,
    -255.0,
    255.0,
    -255.5,
    255.5,
    -256.0, // 2^8
    256.0, // 2^8
    -256.5,
    256.5,
    -257.0,
    257.0,

    -32766.0,
    32766.0,
    -32766.5,
    32766.5,
    -32767.0,
    32767.0,
    -32767.5,
    32767.5,
    -32768.0, // 2^15
    32768.0, // 2^15
    -32768.5,
    32768.5,
    -32769.0,
    32769.0,

    -65534.0,
    65534.0,
    -65534.5,
    65534.5,
    -65535.0,
    65535.0,
    -65535.5,
    65535.5,
    -65536.0, // 2^16
    65536.0, // 2^16
    -65536.5,
    65536.5,
    -65537.0,
    65537.0,

    -2147483646.0,
    2147483646.0,
    -2147483646.5,
    2147483646.5,
    -2147483647.0,
    2147483647.0,
    -2147483647.5,
    2147483647.5,
    -2147483648.0, // 2^31
    2147483648.0, // 2^31
    -2147483648.5,
    2147483648.5,
    -2147483649.0,
    2147483649.0,

    -4294967294.0,
    4294967294.0,
    -4294967294.5,
    4294967294.5,
    -4294967295.0,
    4294967295.0,
    -4294967295.5,
    4294967295.5,
    -4294967296.0, // 2^32
    4294967296.0, // 2^32
    -4294967296.5,
    4294967296.5,
    -4294967297.0,
    4294967297.0,

    -std::numeric_limits<double>::infinity(),
    std::numeric_limits<double>::infinity(),
    std::numeric_limits<double>::quiet_NaN(),
};

TEST_CASE("ToInt32")
{
    static const double Expected[] = {
        0.0,
        0.0,
        -1.0,
        1.0,
        -126.0,
        126.0,
        -126.0,
        126.0,
        -127.0,
        127.0,
        -127.0,
        127.0,
        -128.0,
        128.0,
        -128.0,
        128.0,
        -129.0,
        129.0,
        -254.0,
        254.0,
        -254.0,
        254.0,
        -255.0,
        255.0,
        -255.0,
        255.0,
        -256.0,
        256.0,
        -256.0,
        256.0,
        -257.0,
        257.0,
        -32766.0,
        32766.0,
        -32766.0,
        32766.0,
        -32767.0,
        32767.0,
        -32767.0,
        32767.0,
        -32768.0,
        32768.0,
        -32768.0,
        32768.0,
        -32769.0,
        32769.0,
        -65534.0,
        65534.0,
        -65534.0,
        65534.0,
        -65535.0,
        65535.0,
        -65535.0,
        65535.0,
        -65536.0,
        65536.0,
        -65536.0,
        65536.0,
        -65537.0,
        65537.0,
        -2147483646.0,
        2147483646.0,
        -2147483646.0,
        2147483646.0,
        -2147483647.0,
        2147483647.0,
        -2147483647.0,
        2147483647.0,
        -2147483648.0,
        -2147483648.0,
        -2147483648.0,
        -2147483648.0,
        2147483647.0,
        -2147483647.0,
        2.0,
        -2.0,
        2.0,
        -2.0,
        1.0,
        -1.0,
        1.0,
        -1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        -1.0,
        1.0,
        0.0,
        0.0,
        0.0,
    };
    static_assert(sizeof(Arr) == sizeof(Expected), "size mismatch");

    size_t i = 0;
    for (double d : Arr)
    {
        double expected = Expected[i];
        CAPTURE(d);
        json::Value j = d;
        double actual = static_cast<double>(j.to_int32());
        const auto expected_parts = GetDoubleParts(expected);
        const auto actual_parts = GetDoubleParts(actual);
        CHECK(expected_parts == actual_parts);
        ++i;
    }
}

TEST_CASE("ToUint32")
{
    static const double Expected[] = {
        0.0,
        0.0,
        4294967295.0,
        1.0,
        4294967170.0,
        126.0,
        4294967170.0,
        126.0,
        4294967169.0,
        127.0,
        4294967169.0,
        127.0,
        4294967168.0,
        128.0,
        4294967168.0,
        128.0,
        4294967167.0,
        129.0,
        4294967042.0,
        254.0,
        4294967042.0,
        254.0,
        4294967041.0,
        255.0,
        4294967041.0,
        255.0,
        4294967040.0,
        256.0,
        4294967040.0,
        256.0,
        4294967039.0,
        257.0,
        4294934530.0,
        32766.0,
        4294934530.0,
        32766.0,
        4294934529.0,
        32767.0,
        4294934529.0,
        32767.0,
        4294934528.0,
        32768.0,
        4294934528.0,
        32768.0,
        4294934527.0,
        32769.0,
        4294901762.0,
        65534.0,
        4294901762.0,
        65534.0,
        4294901761.0,
        65535.0,
        4294901761.0,
        65535.0,
        4294901760.0,
        65536.0,
        4294901760.0,
        65536.0,
        4294901759.0,
        65537.0,
        2147483650.0,
        2147483646.0,
        2147483650.0,
        2147483646.0,
        2147483649.0,
        2147483647.0,
        2147483649.0,
        2147483647.0,
        2147483648.0,
        2147483648.0,
        2147483648.0,
        2147483648.0,
        2147483647.0,
        2147483649.0,
        2.0,
        4294967294.0,
        2.0,
        4294967294.0,
        1.0,
        4294967295.0,
        1.0,
        4294967295.0,
        0.0,
        0.0,
        0.0,
        0.0,
        4294967295.0,
        1.0,
        0.0,
        0.0,
        0.0,
    };
    static_assert(sizeof(Arr) == sizeof(Expected), "size mismatch");

    size_t i = 0;
    for (double d : Arr)
    {
        double expected = Expected[i];
        CAPTURE(d);
        json::Value j = d;
        double actual = static_cast<double>(j.to_uint32());
        const auto expected_parts = GetDoubleParts(expected);
        const auto actual_parts = GetDoubleParts(actual);
        CHECK(expected_parts == actual_parts);
        ++i;

        // The ToUint32 abstract operation is idempotent: if applied to a result
        // that it produced, the second application leaves that value unchanged.
        j = actual;
        double actual2 = static_cast<double>(j.to_uint32());
        const auto actual2_parts = GetDoubleParts(actual2);
        CHECK(expected_parts == actual2_parts);

        // ToUint32(ToInt32(x)) is equal to ToUint32(x) for all values of x.
        // (It is to preserve this latter property that +Infinity and -Infinity are mapped to +0.)
        j = d;
        double i32 = static_cast<double>(j.to_int32()); // exact
        j = i32;
        double u32 = static_cast<double>(j.to_uint32());
        const auto u32_parts = GetDoubleParts(u32);
        CHECK(expected_parts == u32_parts);
    }
}

TEST_CASE("ToInt16")
{
    static const double Expected[] = {
        0.0,
        0.0,
        -1.0,
        1.0,
        -126.0,
        126.0,
        -126.0,
        126.0,
        -127.0,
        127.0,
        -127.0,
        127.0,
        -128.0,
        128.0,
        -128.0,
        128.0,
        -129.0,
        129.0,
        -254.0,
        254.0,
        -254.0,
        254.0,
        -255.0,
        255.0,
        -255.0,
        255.0,
        -256.0,
        256.0,
        -256.0,
        256.0,
        -257.0,
        257.0,
        -32766.0,
        32766.0,
        -32766.0,
        32766.0,
        -32767.0,
        32767.0,
        -32767.0,
        32767.0,
        -32768.0,
        -32768.0,
        -32768.0,
        -32768.0,
        32767.0,
        -32767.0,
        2.0,
        -2.0,
        2.0,
        -2.0,
        1.0,
        -1.0,
        1.0,
        -1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        -1.0,
        1.0,
        2.0,
        -2.0,
        2.0,
        -2.0,
        1.0,
        -1.0,
        1.0,
        -1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        -1.0,
        1.0,
        2.0,
        -2.0,
        2.0,
        -2.0,
        1.0,
        -1.0,
        1.0,
        -1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        -1.0,
        1.0,
        0.0,
        0.0,
        0.0,
    };
    static_assert(sizeof(Arr) == sizeof(Expected), "size mismatch");

    size_t i = 0;
    for (double d : Arr)
    {
        double expected = Expected[i];
        CAPTURE(d);
        json::Value j = d;
        double actual = static_cast<double>(j.to_int16());
        const auto expected_parts = GetDoubleParts(expected);
        const auto actual_parts = GetDoubleParts(actual);
        CHECK(expected_parts == actual_parts);
        ++i;
    }
}

TEST_CASE("ToUint16")
{
    static const double Expected[] = {
        0.0,
        0.0,
        65535.0,
        1.0,
        65410.0,
        126.0,
        65410.0,
        126.0,
        65409.0,
        127.0,
        65409.0,
        127.0,
        65408.0,
        128.0,
        65408.0,
        128.0,
        65407.0,
        129.0,
        65282.0,
        254.0,
        65282.0,
        254.0,
        65281.0,
        255.0,
        65281.0,
        255.0,
        65280.0,
        256.0,
        65280.0,
        256.0,
        65279.0,
        257.0,
        32770.0,
        32766.0,
        32770.0,
        32766.0,
        32769.0,
        32767.0,
        32769.0,
        32767.0,
        32768.0,
        32768.0,
        32768.0,
        32768.0,
        32767.0,
        32769.0,
        2.0,
        65534.0,
        2.0,
        65534.0,
        1.0,
        65535.0,
        1.0,
        65535.0,
        0.0,
        0.0,
        0.0,
        0.0,
        65535.0,
        1.0,
        2.0,
        65534.0,
        2.0,
        65534.0,
        1.0,
        65535.0,
        1.0,
        65535.0,
        0.0,
        0.0,
        0.0,
        0.0,
        65535.0,
        1.0,
        2.0,
        65534.0,
        2.0,
        65534.0,
        1.0,
        65535.0,
        1.0,
        65535.0,
        0.0,
        0.0,
        0.0,
        0.0,
        65535.0,
        1.0,
        0.0,
        0.0,
        0.0,
    };
    static_assert(sizeof(Arr) == sizeof(Expected), "size mismatch");

    size_t i = 0;
    for (double d : Arr)
    {
        double expected = Expected[i];
        CAPTURE(d);
        json::Value j = d;
        double actual = static_cast<double>(j.to_uint16());
        const auto expected_parts = GetDoubleParts(expected);
        const auto actual_parts = GetDoubleParts(actual);
        CHECK(expected_parts == actual_parts);
        ++i;
    }
}

TEST_CASE("ToInt8")
{
    static const double Expected[] = {
        0,
        0,
        -1,
        1,
        -126,
        126,
        -126,
        126,
        -127,
        127,
        -127,
        127,
        -128,
        -128,
        -128,
        -128,
        127,
        -127,
        2,
        -2,
        2,
        -2,
        1,
        -1,
        1,
        -1,
        0,
        0,
        0,
        0,
        -1,
        1,
        2,
        -2,
        2,
        -2,
        1,
        -1,
        1,
        -1,
        0,
        0,
        0,
        0,
        -1,
        1,
        2,
        -2,
        2,
        -2,
        1,
        -1,
        1,
        -1,
        0,
        0,
        0,
        0,
        -1,
        1,
        2,
        -2,
        2,
        -2,
        1,
        -1,
        1,
        -1,
        0,
        0,
        0,
        0,
        -1,
        1,
        2,
        -2,
        2,
        -2,
        1,
        -1,
        1,
        -1,
        0,
        0,
        0,
        0,
        -1,
        1,
        0.0,
        0.0,
        0.0,
    };
    static_assert(sizeof(Arr) == sizeof(Expected), "size mismatch");

    size_t i = 0;
    for (double d : Arr)
    {
        double expected = Expected[i];
        CAPTURE(d);
        json::Value j = d;
        double actual = static_cast<double>(j.to_int8());
        const auto expected_parts = GetDoubleParts(expected);
        const auto actual_parts = GetDoubleParts(actual);
        CHECK(expected_parts == actual_parts);
        ++i;
    }
}

TEST_CASE("ToUint8")
{
    static const double Expected[] = {
        0.0,
        0.0,
        255.0,
        1.0,

        130.0,
        126.0,
        130.0,
        126.0,
        129.0,
        127.0,
        129.0,
        127.0,
        128.0,
        128.0,
        128.0,
        128.0,
        127.0,
        129.0,

        2.0,
        254.0,
        2.0,
        254.0,
        1.0,
        255.0,
        1.0,
        255.0,
        0.0,
        0.0,
        0.0,
        0.0,
        255.0,
        1.0,
        2.0,
        254.0,
        2.0,
        254.0,
        1.0,
        255.0,
        1.0,
        255.0,
        0.0,
        0.0,
        0.0,
        0.0,
        255.0,
        1.0,
        2.0,
        254.0,
        2.0,
        254.0,
        1.0,
        255.0,
        1.0,
        255.0,
        0.0,
        0.0,
        0.0,
        0.0,
        255.0,
        1.0,
        2.0,
        254.0,
        2.0,
        254.0,
        1.0,
        255.0,
        1.0,
        255.0,
        0.0,
        0.0,
        0.0,
        0.0,
        255.0,
        1.0,
        2.0,
        254.0,
        2.0,
        254.0,
        1.0,
        255.0,
        1.0,
        255.0,
        0.0,
        0.0,
        0.0,
        0.0,
        255.0,
        1.0,
        0.0,
        0.0,
        0.0,
    };
    static_assert(sizeof(Arr) == sizeof(Expected), "size mismatch");

    size_t i = 0;
    for (double d : Arr)
    {
        double expected = Expected[i];
        CAPTURE(d);
        json::Value j = d;
        double actual = static_cast<double>(j.to_uint8());
        const auto expected_parts = GetDoubleParts(expected);
        const auto actual_parts = GetDoubleParts(actual);
        CHECK(expected_parts == actual_parts);
        ++i;
    }
}

TEST_CASE("ToInt8Clamp")
{
    static const double Expected[] = {
        0.0,
        0.0,
        0.0,
        1.0,
        0.0,
        126.0,
        0.0,
        126.0,
        0.0,
        127.0,
        0.0,
        128.0,
        0.0,
        128.0,
        0.0,
        128.0,
        0.0,
        129.0,
        0.0,
        254.0,
        0.0,
        254.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0,
        255.0,
        0.0, // -Inf
        255.0, // +Inf
        0.0, // NaN
    };
    static_assert(sizeof(Arr) == sizeof(Expected), "size mismatch");

    size_t i = 0;
    for (double d : Arr)
    {
        double expected = Expected[i];
        CAPTURE(d);
        json::Value j = d;
        double actual = static_cast<double>(j.to_uint8_clamped());
        const auto expected_parts = GetDoubleParts(expected);
        const auto actual_parts = GetDoubleParts(actual);
        CHECK(expected_parts == actual_parts);
        ++i;
    }
}

static inline double NextUp(double d) {
    return std::nextafter(d, std::numeric_limits<double>::infinity());
}

static inline double NextDown(double d) {
    return std::nextafter(d, -std::numeric_limits<double>::infinity());
}

TEST_CASE("ToUint8Clamp boundary")
{
    for (int i = 0; i <= 255; ++i)
    {
        const double v = static_cast<double>(i);
        const int32_t expected     = i;
        const int32_t expectedDown = i;
        const int32_t expectedUp   = i;
        const int32_t actual       = json::numbers::ToUint8Clamp(v);
        const int32_t actualDown   = json::numbers::ToUint8Clamp(NextDown(v));
        const int32_t actualUp     = json::numbers::ToUint8Clamp(NextUp(v));
        CHECK(expected == actual);
        CHECK(expectedDown == actualDown);
        CHECK(expectedUp == actualUp);
    }

    for (int i = 0; i <= 255; ++i)
    {
        const double v = static_cast<double>(2 * i + 1) / 2.0;
        const int32_t expected     = std::min(255, (i + 1) & ~1);
        const int32_t expectedDown = std::min(255, (i + 0)     );
        const int32_t expectedUp   = std::min(255, (i + 1)     );
        const int32_t actual       = json::numbers::ToUint8Clamp(v);
        const int32_t actualDown   = json::numbers::ToUint8Clamp(NextDown(v));
        const int32_t actualUp     = json::numbers::ToUint8Clamp(NextUp(v));
        CHECK(expected == actual);
        CHECK(expectedDown == actualDown);
        CHECK(expectedUp == actualUp);
    }
}
