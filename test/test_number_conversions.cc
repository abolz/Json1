#define __STDC_FORMAT_MACROS
#include "catch.hpp"
#include "../src/json.h"
#include "../src/json_numbers.h"

#include <algorithm>
#include <limits>
#include <cmath>

struct Test {
    double   value;
    int32_t  i32;
    uint32_t u32;
    int16_t  i16;
    uint16_t u16;
    int8_t   i8;
    uint8_t  u8;
    uint8_t  u8_clamped;
};

static constexpr double Infinity = std::numeric_limits<double>::infinity();
static constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

static const Test tests[] = {
//                          value           i32            u32           i16            u16            i8             u8     u8_clamped
    {         -0.0000000000000000,            0,            0u,            0,            0u,            0,            0u,            0u},
    {          0.0000000000000000,            0,            0u,            0,            0u,            0,            0u,            0u},
    {         -126.00000000000000,         -126,   4294967170u,         -126,        65410u,         -126,          130u,            0u},
    {          126.00000000000000,          126,          126u,          126,          126u,          126,          126u,          126u},
    {         -126.50000000000000,         -126,   4294967170u,         -126,        65410u,         -126,          130u,            0u},
    {          126.50000000000000,          126,          126u,          126,          126u,          126,          126u,          126u},
    {         -127.00000000000000,         -127,   4294967169u,         -127,        65409u,         -127,          129u,            0u},
    {          127.00000000000000,          127,          127u,          127,          127u,          127,          127u,          127u},
    {         -127.50000000000000,         -127,   4294967169u,         -127,        65409u,         -127,          129u,            0u},
    {          127.50000000000000,          127,          127u,          127,          127u,          127,          127u,          128u},
    {         -128.00000000000000,         -128,   4294967168u,         -128,        65408u,         -128,          128u,            0u},
    {          128.00000000000000,          128,          128u,          128,          128u,         -128,          128u,          128u},
    {         -128.50000000000000,         -128,   4294967168u,         -128,        65408u,         -128,          128u,            0u},
    {          128.50000000000000,          128,          128u,          128,          128u,         -128,          128u,          128u},
    {         -129.00000000000000,         -129,   4294967167u,         -129,        65407u,          127,          127u,            0u},
    {          129.00000000000000,          129,          129u,          129,          129u,         -127,          129u,          129u},
    {         -254.00000000000000,         -254,   4294967042u,         -254,        65282u,            2,            2u,            0u},
    {          254.00000000000000,          254,          254u,          254,          254u,           -2,          254u,          254u},
    {         -254.50000000000000,         -254,   4294967042u,         -254,        65282u,            2,            2u,            0u},
    {          254.50000000000000,          254,          254u,          254,          254u,           -2,          254u,          254u},
    {         -255.00000000000000,         -255,   4294967041u,         -255,        65281u,            1,            1u,            0u},
    {          255.00000000000000,          255,          255u,          255,          255u,           -1,          255u,          255u},
    {         -255.50000000000000,         -255,   4294967041u,         -255,        65281u,            1,            1u,            0u},
    {          255.50000000000000,          255,          255u,          255,          255u,           -1,          255u,          255u},
    {         -256.00000000000000,         -256,   4294967040u,         -256,        65280u,            0,            0u,            0u},
    {          256.00000000000000,          256,          256u,          256,          256u,            0,            0u,          255u},
    {         -256.50000000000000,         -256,   4294967040u,         -256,        65280u,            0,            0u,            0u},
    {          256.50000000000000,          256,          256u,          256,          256u,            0,            0u,          255u},
    {         -257.00000000000000,         -257,   4294967039u,         -257,        65279u,           -1,          255u,            0u},
    {          257.00000000000000,          257,          257u,          257,          257u,            1,            1u,          255u},
    {         -32766.000000000000,       -32766,   4294934530u,       -32766,        32770u,            2,            2u,            0u},
    {          32766.000000000000,        32766,        32766u,        32766,        32766u,           -2,          254u,          255u},
    {         -32766.500000000000,       -32766,   4294934530u,       -32766,        32770u,            2,            2u,            0u},
    {          32766.500000000000,        32766,        32766u,        32766,        32766u,           -2,          254u,          255u},
    {         -32767.000000000000,       -32767,   4294934529u,       -32767,        32769u,            1,            1u,            0u},
    {          32767.000000000000,        32767,        32767u,        32767,        32767u,           -1,          255u,          255u},
    {         -32767.500000000000,       -32767,   4294934529u,       -32767,        32769u,            1,            1u,            0u},
    {          32767.500000000000,        32767,        32767u,        32767,        32767u,           -1,          255u,          255u},
    {         -32768.000000000000,       -32768,   4294934528u,       -32768,        32768u,            0,            0u,            0u},
    {          32768.000000000000,        32768,        32768u,       -32768,        32768u,            0,            0u,          255u},
    {         -32768.500000000000,       -32768,   4294934528u,       -32768,        32768u,            0,            0u,            0u},
    {          32768.500000000000,        32768,        32768u,       -32768,        32768u,            0,            0u,          255u},
    {         -32769.000000000000,       -32769,   4294934527u,        32767,        32767u,           -1,          255u,            0u},
    {          32769.000000000000,        32769,        32769u,       -32767,        32769u,            1,            1u,          255u},
    {         -65534.000000000000,       -65534,   4294901762u,            2,            2u,            2,            2u,            0u},
    {          65534.000000000000,        65534,        65534u,           -2,        65534u,           -2,          254u,          255u},
    {         -65534.500000000000,       -65534,   4294901762u,            2,            2u,            2,            2u,            0u},
    {          65534.500000000000,        65534,        65534u,           -2,        65534u,           -2,          254u,          255u},
    {         -65535.000000000000,       -65535,   4294901761u,            1,            1u,            1,            1u,            0u},
    {          65535.000000000000,        65535,        65535u,           -1,        65535u,           -1,          255u,          255u},
    {         -65535.500000000000,       -65535,   4294901761u,            1,            1u,            1,            1u,            0u},
    {          65535.500000000000,        65535,        65535u,           -1,        65535u,           -1,          255u,          255u},
    {         -65536.000000000000,       -65536,   4294901760u,            0,            0u,            0,            0u,            0u},
    {          65536.000000000000,        65536,        65536u,            0,            0u,            0,            0u,          255u},
    {         -65536.500000000000,       -65536,   4294901760u,            0,            0u,            0,            0u,            0u},
    {          65536.500000000000,        65536,        65536u,            0,            0u,            0,            0u,          255u},
    {         -65537.000000000000,       -65537,   4294901759u,           -1,        65535u,           -1,          255u,            0u},
    {          65537.000000000000,        65537,        65537u,            1,            1u,            1,            1u,          255u},
    {         -2147483646.0000000,  -2147483646,   2147483650u,            2,            2u,            2,            2u,            0u},
    {          2147483646.0000000,   2147483646,   2147483646u,           -2,        65534u,           -2,          254u,          255u},
    {         -2147483646.5000000,  -2147483646,   2147483650u,            2,            2u,            2,            2u,            0u},
    {          2147483646.5000000,   2147483646,   2147483646u,           -2,        65534u,           -2,          254u,          255u},
    {         -2147483647.0000000,  -2147483647,   2147483649u,            1,            1u,            1,            1u,            0u},
    {          2147483647.0000000,   2147483647,   2147483647u,           -1,        65535u,           -1,          255u,          255u},
    {         -2147483647.5000000,  -2147483647,   2147483649u,            1,            1u,            1,            1u,            0u},
    {          2147483647.5000000,   2147483647,   2147483647u,           -1,        65535u,           -1,          255u,          255u},
    {         -2147483648.0000000,    INT32_MIN,   2147483648u,            0,            0u,            0,            0u,            0u},
    {          2147483648.0000000,    INT32_MIN,   2147483648u,            0,            0u,            0,            0u,          255u},
    {         -2147483648.5000000,    INT32_MIN,   2147483648u,            0,            0u,            0,            0u,            0u},
    {          2147483648.5000000,    INT32_MIN,   2147483648u,            0,            0u,            0,            0u,          255u},
    {         -2147483649.0000000,   2147483647,   2147483647u,           -1,        65535u,           -1,          255u,            0u},
    {          2147483649.0000000,  -2147483647,   2147483649u,            1,            1u,            1,            1u,          255u},
    {         -4294967296.0000000,            0,            0u,            0,            0u,            0,            0u,            0u},
    {          4294967296.0000000,            0,            0u,            0,            0u,            0,            0u,          255u},
    {         -4294971391.9999990,        -4095,   4294963201u,        -4095,        61441u,            1,            1u,            0u},
    {          4294971391.9999990,         4095,         4095u,         4095,         4095u,           -1,          255u,          255u},
    {         -4294969343.9999990,        -2047,   4294965249u,        -2047,        63489u,            1,            1u,            0u},
    {          4294969343.9999990,         2047,         2047u,         2047,         2047u,           -1,          255u,          255u},
    {         -4294969344.0000000,        -2048,   4294965248u,        -2048,        63488u,            0,            0u,            0u},
    {          4294969344.0000000,         2048,         2048u,         2048,         2048u,            0,            0u,          255u},
    {        -0.50000000000000000,            0,            0u,            0,            0u,            0,            0u,            0u},
    {         0.50000000000000000,            0,            0u,            0,            0u,            0,            0u,            0u},
    {        -0.99999999999999989,            0,            0u,            0,            0u,            0,            0u,            0u},
    {         0.99999999999999989,            0,            0u,            0,            0u,            0,            0u,            1u},
    {         -1.0000000000000000,           -1,   4294967295u,           -1,        65535u,           -1,          255u,            0u},
    {          1.0000000000000000,            1,            1u,            1,            1u,            1,            1u,            1u},
    {         -1.9999999999999998,           -1,   4294967295u,           -1,        65535u,           -1,          255u,            0u},
    {          1.9999999999999998,            1,            1u,            1,            1u,            1,            1u,            2u},
    {         -1.0000009536743162,           -1,   4294967295u,           -1,        65535u,           -1,          255u,            0u},
    {          1.0000009536743162,            1,            1u,            1,            1u,            1,            1u,            1u},
    {         -1.0000004768371580,           -1,   4294967295u,           -1,        65535u,           -1,          255u,            0u},
    {          1.0000004768371580,            1,            1u,            1,            1u,            1,            1u,            1u},
    {         -1.0000000000145517,           -1,   4294967295u,           -1,        65535u,           -1,          255u,            0u},
    {          1.0000000000145517,            1,            1u,            1,            1u,            1,            1u,            1u},
    {         -1.0000000000072757,           -1,   4294967295u,           -1,        65535u,           -1,          255u,            0u},
    {          1.0000000000072757,            1,            1u,            1,            1u,            1,            1u,            1u},
    {         -2251799813685248.0,            0,            0u,            0,            0u,            0,            0u,            0u},
    {          2251799813685248.0,            0,            0u,            0,            0u,            0,            0u,          255u},
    {         -4503599627370496.0,            0,            0u,            0,            0u,            0,            0u,            0u},
    {          4503599627370496.0,            0,            0u,            0,            0u,            0,            0u,          255u},
    {         -4503599627370497.0,           -1,   4294967295u,           -1,        65535u,           -1,          255u,            0u},
    {          4503599627370497.0,            1,            1u,            1,            1u,            1,            1u,          255u},
    {         -4503599627370751.0,         -255,   4294967041u,         -255,        65281u,            1,            1u,            0u},
    {          4503599627370751.0,          255,          255u,          255,          255u,           -1,          255u,          255u},
    {         -4503599627370623.0,         -127,   4294967169u,         -127,        65409u,         -127,          129u,            0u},
    {          4503599627370623.0,          127,          127u,          127,          127u,          127,          127u,          255u},
    {         -4503599627370624.0,         -128,   4294967168u,         -128,        65408u,         -128,          128u,            0u},
    {          4503599627370624.0,          128,          128u,          128,          128u,         -128,          128u,          255u},
    {         -4503599627436031.0,       -65535,   4294901761u,            1,            1u,            1,            1u,            0u},
    {          4503599627436031.0,        65535,        65535u,           -1,        65535u,           -1,          255u,          255u},
    {         -4503599627403263.0,       -32767,   4294934529u,       -32767,        32769u,            1,            1u,            0u},
    {          4503599627403263.0,        32767,        32767u,        32767,        32767u,           -1,          255u,          255u},
    {         -4503599627403264.0,       -32768,   4294934528u,       -32768,        32768u,            0,            0u,            0u},
    {          4503599627403264.0,        32768,        32768u,       -32768,        32768u,            0,            0u,          255u},
    {         -4503603922337791.0,            1,            1u,            1,            1u,            1,            1u,            0u},
    {          4503603922337791.0,           -1,   4294967295u,           -1,        65535u,           -1,          255u,          255u},
    {         -4503601774854143.0,  -2147483647,   2147483649u,            1,            1u,            1,            1u,            0u},
    {          4503601774854143.0,   2147483647,   2147483647u,           -1,        65535u,           -1,          255u,          255u},
    {         -4503601774854144.0,    INT32_MIN,   2147483648u,            0,            0u,            0,            0u,            0u},
    {          4503601774854144.0,    INT32_MIN,   2147483648u,            0,            0u,            0,            0u,          255u},
    {         -9007199254740992.0,            0,            0u,            0,            0u,            0,            0u,            0u},
    {          9007199254740992.0,            0,            0u,            0,            0u,            0,            0u,          255u},
    {        -18014398509481984.0,            0,            0u,            0,            0u,            0,            0u,            0u},
    {         18014398509481984.0,            0,            0u,            0,            0u,            0,            0u,          255u},
    {     -9.6714065569170334e+24,            0,            0u,            0,            0u,            0,            0u,            0u},
    {      9.6714065569170334e+24,            0,            0u,            0,            0u,            0,            0u,          255u},
    {     -1.9342813113834067e+25,            0,            0u,            0,            0u,            0,            0u,            0u},
    {      1.9342813113834067e+25,            0,            0u,            0,            0u,            0,            0u,          255u},
    {                   -Infinity,            0,            0u,            0,            0u,            0,            0u,            0u},
    {                    Infinity,            0,            0u,            0,            0u,            0,            0u,          255u},
    {                         NaN,            0,            0u,            0,            0u,            0,            0u,            0u},
};

TEST_CASE("ToInt32")
{
    int i = 0;
    for (auto const& test : tests)
    {
        const double value = test.value;
        CAPTURE(i);
        CAPTURE(value);
        const int32_t expected = test.i32;
        const int32_t actual = json::numbers::ToInt32(value);
        CHECK(expected == actual);
        ++i;
    }
}

TEST_CASE("ToUint32")
{
    int i = 0;
    for (auto const& test : tests)
    {
        const double value = test.value;
        CAPTURE(i);
        CAPTURE(value);
        const uint32_t expected = test.u32;
        const uint32_t actual = json::numbers::ToUint32(value);
        CHECK(expected == actual);
        ++i;
    }
}

TEST_CASE("ToInt16")
{
    int i = 0;
    for (auto const& test : tests)
    {
        const double value = test.value;
        CAPTURE(i);
        CAPTURE(value);
        const int32_t expected = test.i16;
        const int32_t actual = json::numbers::ToInt16(value);
        CHECK(expected == actual);
        ++i;
    }
}

TEST_CASE("ToUint16")
{
    int i = 0;
    for (auto const& test : tests)
    {
        const double value = test.value;
        CAPTURE(i);
        CAPTURE(value);
        const uint32_t expected = test.u16;
        const uint32_t actual = json::numbers::ToUint16(value);
        CHECK(expected == actual);
        ++i;
    }
}

TEST_CASE("ToInt8")
{
    int i = 0;
    for (auto const& test : tests)
    {
        const double value = test.value;
        CAPTURE(i);
        CAPTURE(value);
        const int32_t expected = test.i8;
        const int32_t actual = json::numbers::ToInt8(value);
        CHECK(expected == actual);
        ++i;
    }
}

TEST_CASE("ToUint8")
{
    int i = 0;
    for (auto const& test : tests)
    {
        const double value = test.value;
        CAPTURE(i);
        CAPTURE(value);
        const uint32_t expected = test.u8;
        const uint32_t actual = json::numbers::ToUint8(value);
        CHECK(expected == actual);
        ++i;
    }
}

TEST_CASE("ToUint8Clamp")
{
    int i = 0;
    for (auto const& test : tests)
    {
        const double value = test.value;
        CAPTURE(i);
        CAPTURE(value);
        const uint32_t expected = test.u8_clamped;
        const uint32_t actual = json::numbers::ToUint8Clamp(value);
        CHECK(expected == actual);
        ++i;
    }
}

static inline double NextUp(double d) {
    return std::nextafter(d, std::numeric_limits<double>::infinity());
}

static inline double NextDown(double d) {
    return std::nextafter(d, -std::numeric_limits<double>::infinity());
}

TEST_CASE("ToUint8Clamp boundaries")
{
    for (int32_t i = 0; i <= 255; ++i)
    {
        const double v = static_cast<double>(i);
        const double vDown = NextDown(v);
        const double vUp = NextUp(v);
        CAPTURE(v);
        CAPTURE(vDown);
        CAPTURE(vUp);
        const int32_t expected     = i;
        const int32_t expectedDown = i;
        const int32_t expectedUp   = i;
        const int32_t actual       = json::numbers::ToUint8Clamp(v);
        const int32_t actualDown   = json::numbers::ToUint8Clamp(vDown);
        const int32_t actualUp     = json::numbers::ToUint8Clamp(vUp);
        CHECK(expected == actual);
        CHECK(expectedDown == actualDown);
        CHECK(expectedUp == actualUp);
    }

    for (int32_t i = 0; i <= 255; ++i)
    {
        const double v = static_cast<double>(i) + 0.5; // exact
        const double vDown = NextDown(v);
        const double vUp = NextUp(v);
        CAPTURE(v);
        CAPTURE(vDown);
        CAPTURE(vUp);
        const int32_t expected     = std::min(255, (i + 1) & ~1);
        const int32_t expectedDown = std::min(255, (i + 0)     );
        const int32_t expectedUp   = std::min(255, (i + 1)     );
        const int32_t actual       = json::numbers::ToUint8Clamp(v);
        const int32_t actualDown   = json::numbers::ToUint8Clamp(vDown);
        const int32_t actualUp     = json::numbers::ToUint8Clamp(vUp);
        CHECK(expected == actual);
        CHECK(expectedDown == actualDown);
        CHECK(expectedUp == actualUp);
    }
}

using json::int53_t;
using json::uint53_t;

struct TestCase53 {
    double value;
    int53_t i53;
    uint53_t u53;
};

// 4503599627370496 = 2^52
// 9007199254740992 = 2^53
static const TestCase53 tests53[] = {
    {                                0.0, int53_t{                0ll}, uint53_t{               0ull} },
    {                               -1.0, int53_t{               -1ll}, uint53_t{9007199254740991ull} },
    {                                1.0, int53_t{                1ll}, uint53_t{               1ull} },
    {                -4503599627370494.0, int53_t{-4503599627370494ll}, uint53_t{4503599627370498ull} },
    {                 4503599627370494.0, int53_t{ 4503599627370494ll}, uint53_t{4503599627370494ull} },
    {                -4503599627370494.5, int53_t{-4503599627370494ll}, uint53_t{4503599627370498ull} },
    {                 4503599627370494.5, int53_t{ 4503599627370494ll}, uint53_t{4503599627370494ull} },
    {                -4503599627370495.0, int53_t{-4503599627370495ll}, uint53_t{4503599627370497ull} },
    {                 4503599627370495.0, int53_t{ 4503599627370495ll}, uint53_t{4503599627370495ull} },
    {                -4503599627370495.5, int53_t{-4503599627370495ll}, uint53_t{4503599627370497ull} },
    {                 4503599627370495.5, int53_t{ 4503599627370495ll}, uint53_t{4503599627370495ull} },
    {                -4503599627370496.0, int53_t{-4503599627370496ll}, uint53_t{4503599627370496ull} },
    {                 4503599627370496.0, int53_t{-4503599627370496ll}, uint53_t{4503599627370496ull} },
    {                -4503599627370496.5, int53_t{-4503599627370496ll}, uint53_t{4503599627370496ull} }, // == -4503599627370496.0
    {                 4503599627370496.5, int53_t{-4503599627370496ll}, uint53_t{4503599627370496ull} }, // ==  4503599627370496.0
    {                -4503599627370497.0, int53_t{ 4503599627370495ll}, uint53_t{4503599627370495ull} },
    {                 4503599627370497.0, int53_t{-4503599627370495ll}, uint53_t{4503599627370497ull} },
    {                -4503599627370497.5, int53_t{ 4503599627370494ll}, uint53_t{4503599627370494ull} }, // == -4503599627370498.0
    {                 4503599627370497.5, int53_t{-4503599627370494ll}, uint53_t{4503599627370498ull} }, // ==  4503599627370498.0
    {                -4503599627370498.0, int53_t{ 4503599627370494ll}, uint53_t{4503599627370494ull} },
    {                 4503599627370498.0, int53_t{-4503599627370494ll}, uint53_t{4503599627370498ull} },
    {                -9007199254740990.0, int53_t{                2ll}, uint53_t{               2ull} },
    {                 9007199254740990.0, int53_t{               -2ll}, uint53_t{9007199254740990ull} },
    {                -9007199254740991.0, int53_t{                1ll}, uint53_t{               1ull} },
    {                 9007199254740991.0, int53_t{               -1ll}, uint53_t{9007199254740991ull} },
    {                -9007199254740992.0, int53_t{                0ll}, uint53_t{               0ull} },
    {                 9007199254740992.0, int53_t{                0ll}, uint53_t{               0ull} },
    {                -9007199254740993.0, int53_t{                0ll}, uint53_t{               0ull} }, // == -9007199254740992.0
    {                 9007199254740993.0, int53_t{                0ll}, uint53_t{               0ull} }, // ==  9007199254740992.0
    {                -9007199254740994.0, int53_t{               -2ll}, uint53_t{9007199254740990ull} },
    {                 9007199254740994.0, int53_t{                2ll}, uint53_t{               2ull} },
    {                -9007199254740995.0, int53_t{               -4ll}, uint53_t{9007199254740988ull} }, // == -9007199254740996.0
    {                 9007199254740995.0, int53_t{                4ll}, uint53_t{               4ull} }, // ==  9007199254740996.0
    {                -9007199254740996.0, int53_t{               -4ll}, uint53_t{9007199254740988ull} },
    {                 9007199254740996.0, int53_t{                4ll}, uint53_t{               4ull} },
    {                -9007199254740997.0, int53_t{               -4ll}, uint53_t{9007199254740988ull} }, // == -9007199254740996.0
    {                 9007199254740997.0, int53_t{                4ll}, uint53_t{               4ull} }, // ==  9007199254740996.0
    {                -9007199254740998.0, int53_t{               -6ll}, uint53_t{9007199254740986ull} },
    {                 9007199254740998.0, int53_t{                6ll}, uint53_t{               6ull} },
    {               -18014398509481984.0, int53_t{                0ll}, uint53_t{               0ull} }, // == -2^54
    {                18014398509481984.0, int53_t{                0ll}, uint53_t{               0ull} }, // ==  2^54
    {-40564819207303336344294875201536.0, int53_t{-4503599627370496ll}, uint53_t{4503599627370496ull} }, // == -2^52 * (2^53 - 1)
    { 40564819207303336344294875201536.0, int53_t{-4503599627370496ll}, uint53_t{4503599627370496ull} }, // ==  2^52 * (2^53 - 1)
    {-81129638414606672688589750403072.0, int53_t{                0ll}, uint53_t{               0ull} }, // == -2^53 * (2^53 - 1)
    { 81129638414606672688589750403072.0, int53_t{                0ll}, uint53_t{               0ull} }, // ==  2^53 * (2^53 - 1)
};

TEST_CASE("ToInt53")
{
    int i = 0;
    for (auto const& test : tests53)
    {
        const double value = test.value;
        CAPTURE(i);
        CAPTURE(value);
        const int64_t expected = test.i53;
        const int64_t actual = json::numbers::ToInt53(value);
        CHECK(expected == actual);
        ++i;
    }
}

TEST_CASE("ToUint53")
{
    int i = 0;
    for (auto const& test : tests53)
    {
        const double value = test.value;
        CAPTURE(i);
        CAPTURE(value);
        const uint64_t expected = test.u53;
        const uint64_t actual = json::numbers::ToUint53(value);
        CHECK(expected == actual);
        ++i;
    }
}
