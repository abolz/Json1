#include "catch.hpp"
#include "../src/json_numbers.h"

#include <cmath>

#define EXPECT_EQUALS(EXPECTED, ACTUAL) CHECK(EXPECTED == ACTUAL)
#define EXPECT_TRUE(EXPECTED) CHECK(EXPECTED == true)

struct ScanNumberTest
{
    std::string str;
    json::NumberClass nc;
    int pos;

    ScanNumberTest(std::string const& str_, json::NumberClass nc_)
        : str(str_)
        , nc(nc_)
        , pos(static_cast<int>(str_.size()))
    {
    }

    ScanNumberTest(std::string const& str_, json::NumberClass nc_, int pos_)
        : str(str_)
        , nc(nc_)
        , pos(pos_)
    {
    }
};

TEST_CASE("ScanNumber")
{
    static const ScanNumberTest tests[] = {
        //
        // Valid inputs.
        //

        {"0", json::NumberClass::integer},
        {"-0", json::NumberClass::integer},
        {"123e65", json::NumberClass::integer_with_exponent},
        {"0e+1", json::NumberClass::integer_with_exponent},
        {"0e1", json::NumberClass::integer_with_exponent},
        {"4", json::NumberClass::integer},
        {"-0.0000000000000000000000000000001", json::NumberClass::decimal},
        {"20e1", json::NumberClass::integer_with_exponent},
        {"-123", json::NumberClass::integer},
        {"-1", json::NumberClass::integer},
        {"1E22", json::NumberClass::integer_with_exponent},
        {"1E-2", json::NumberClass::integer_with_exponent},
        {"1E+2", json::NumberClass::integer_with_exponent},
        {"123e45", json::NumberClass::integer_with_exponent},
        {"123.456e78", json::NumberClass::decimal_with_exponent},
        {"1e-2", json::NumberClass::integer_with_exponent},
        {"1e+2", json::NumberClass::integer_with_exponent},
        {"123", json::NumberClass::integer},
        {"123.456789", json::NumberClass::decimal},
        {"123.456e-789", json::NumberClass::decimal_with_exponent},
        {"0.4e0066999999999999999999999999999999999999999999999999999999999999999999999999999006", json::NumberClass::decimal_with_exponent},
        {"-1e+9999", json::NumberClass::integer_with_exponent},
        {"1.5e+9999", json::NumberClass::decimal_with_exponent},
        {"-123123e100000", json::NumberClass::integer_with_exponent},
        {"123123e100000", json::NumberClass::integer_with_exponent},
        {"123e-10000000", json::NumberClass::integer_with_exponent},
        {"-123123123123123123123123123123", json::NumberClass::integer},
        {"-1.0e+123123123123123123123123123123", json::NumberClass::decimal_with_exponent},
        {"1.0e-123123123123123123123123123123", json::NumberClass::decimal_with_exponent},
        {"100000000000000000000", json::NumberClass::integer},
        {"-237462374673276894279832749832423479823246327846", json::NumberClass::integer},

        //
        // Invalid inputs.
        //

        {"", json::NumberClass::invalid, 0},
        {"-", json::NumberClass::invalid, 1},
        {"++1234", json::NumberClass::invalid, 0},
        {"+1", json::NumberClass::invalid, 0},
        {"+Inf", json::NumberClass::invalid, 0},
        {"+Infinity", json::NumberClass::invalid, 0},
        {"+NaN", json::NumberClass::invalid, 0},
        {"-01", json::NumberClass::invalid, 2},
        {"-2.", json::NumberClass::invalid, 3},
        {".-1", json::NumberClass::invalid, 0},
        {".2e-3", json::NumberClass::invalid, 0},
        {"0.3e+", json::NumberClass::invalid, 5},
        {"0.3e", json::NumberClass::invalid, 4},
        {"0.e1", json::NumberClass::invalid, 2},
        {"0e+", json::NumberClass::invalid, 3},
        {"0e", json::NumberClass::invalid, 2},
        {"0E+", json::NumberClass::invalid, 3},
        {"0E", json::NumberClass::invalid, 2},
        {"1.0e+", json::NumberClass::invalid, 5},
        {"1.0e-", json::NumberClass::invalid, 5},
        {"1.0e", json::NumberClass::invalid, 4},
        {"1eE2", json::NumberClass::invalid, 2},
        {"2.e+3", json::NumberClass::invalid, 2},
        {"2.e-3", json::NumberClass::invalid, 2},
        {"2.e3", json::NumberClass::invalid, 2},
        {"9.e+", json::NumberClass::invalid, 2},
        {"Inf", json::NumberClass::invalid, 0},
        {"0e+-1", json::NumberClass::invalid, 3},
        {"-foo", json::NumberClass::invalid, 1},
        {"- 1", json::NumberClass::invalid, 1},
        {"-012", json::NumberClass::invalid, 2},
        {"-.123", json::NumberClass::invalid, 1},
        {"1ea", json::NumberClass::invalid, 2},
        {"1.", json::NumberClass::invalid, 2},
        {"1e\345", json::NumberClass::invalid, 2},
        {".123", json::NumberClass::invalid, 0},
        {"\357\274\221", json::NumberClass::invalid, 0},
        {"012", json::NumberClass::invalid, 1},

        {"Infinity", json::NumberClass::invalid, 0},
        {"-Infinity", json::NumberClass::invalid, 1},
        {"+Infinity", json::NumberClass::invalid, 0},
        {"NaN", json::NumberClass::invalid, 0},
        {"-NaN", json::NumberClass::invalid, 1},
        {"+NaN", json::NumberClass::invalid, 0},
        {"Infinity1234", json::NumberClass::invalid, 0},
        {"-Infinity1234", json::NumberClass::invalid, 1},
        {"+Infinity1234", json::NumberClass::invalid, 0},
        {"NaN1234", json::NumberClass::invalid, 0},
        {"-NaN1234", json::NumberClass::invalid, 1},
        {"+NaN1234", json::NumberClass::invalid, 0},

        //
        // Valid inputs.
        // But not all characters from the input string have been consumed.
        //

        {"-1.0.", json::NumberClass::decimal, 4},
        {"0.1.2", json::NumberClass::decimal, 3},
        {"1 000.0", json::NumberClass::integer, 1},
        {"1+2", json::NumberClass::integer, 1},
        {"0x1", json::NumberClass::integer, 1},
        {"0x42", json::NumberClass::integer, 1},
        {"-123.123foo", json::NumberClass::decimal, 8},
        {"123\345", json::NumberClass::integer, 3},
        {"1e1\345", json::NumberClass::integer_with_exponent, 3},
        {"1.1e1\345", json::NumberClass::decimal_with_exponent, 5},
        {"0\345", json::NumberClass::integer, 1},
        {"-1x", json::NumberClass::integer, 2},
        {"1.2a-3", json::NumberClass::decimal, 3},
        {"1.8011670033376514H-308", json::NumberClass::decimal, 18},
    };

    for (auto const& test : tests)
    {
        CAPTURE(test.str);
        char const* const next = test.str.data();
        char const* const last = test.str.data() + test.str.size();

        auto const snr = json::ScanNumber(next, last);
        EXPECT_EQUALS(test.nc, snr.number_class);
        EXPECT_EQUALS(test.pos, snr.next - next);
    }
}
