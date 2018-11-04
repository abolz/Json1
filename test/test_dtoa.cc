#include "catch.hpp"
#include "../src/json_numbers.h"

#include <cmath>
#include <limits>

#define CHECK_EQ(EXPECTED, ACTUAL) CHECK(EXPECTED == ACTUAL)
#define CHECK_TRUE(EXPECTED) CHECK(EXPECTED == true)

static double DoubleFromBits(uint64_t ieeeBits)
{
    double value;
    std::memcpy(&value, &ieeeBits, sizeof(double));
    return value;
}

static double DoubleFromBits(int biased_exponent, uint64_t significand)
{
    assert(biased_exponent <= 0x7FF);
    assert(significand <= 0x000FFFFFFFFFFFFFull);

    return DoubleFromBits((static_cast<uint64_t>(biased_exponent) << 52) | significand);
}

// Ldexp = f * 2^e
static double Ldexp(uint64_t f, int e)
{
    static const uint64_t HiddenBit                = 0x0010000000000000ull;
    static const uint64_t SignificandMask          = 0x000FFFFFFFFFFFFFull;
    static const int      PhysicalSignificandSize  = 52; // Excludes the hidden bit.
    static const int      ExponentBias             = 0x3FF + PhysicalSignificandSize;
    static const int      DenormalExponent         = 1 - ExponentBias;
    static const int      MaxExponent              = 0x7FF - ExponentBias;

    assert(f <= HiddenBit + SignificandMask);

    if (e >= MaxExponent)
        return std::numeric_limits<double>::infinity();
    if (e < DenormalExponent)
        return 0;

    while (e > DenormalExponent && (f & HiddenBit) == 0) {
        f <<= 1;
        e--;
    }

    uint64_t biased_exponent;
    if (e == DenormalExponent && (f & HiddenBit) == 0)
        biased_exponent = 0;
    else
        biased_exponent = static_cast<uint64_t>(e + ExponentBias);

    return DoubleFromBits((f & SignificandMask) | (biased_exponent << PhysicalSignificandSize));
}

static charconv_ryu::DoubleToDecimalResult NonNegativeDoubleToDecimal(double value)
{
    return charconv_ryu::DoubleToDecimal(value);
}

TEST_CASE("Ryu_Regression")
{
    charconv_ryu::DoubleToDecimalResult res;

    res = NonNegativeDoubleToDecimal(0.0); // +0
    CHECK_EQ(0ull, res.digits);
    CHECK_EQ(0, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(0, 0x0000000000000001L)); // min denormal
    CHECK_EQ(5ull, res.digits);
    CHECK_EQ(-324, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(0, 0x000FFFFFFFFFFFFFL)); // max denormal
    CHECK_EQ(2225073858507201ull, res.digits);
    CHECK_EQ(-323, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(1, 0x0000000000000000L)); // min normal
    CHECK_EQ(22250738585072014ull, res.digits);
    CHECK_EQ(-324, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(1, 0x0000000000000001L));
    CHECK_EQ(2225073858507202ull, res.digits);
    CHECK_EQ(-323, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(1, 0x000FFFFFFFFFFFFFL));
    CHECK_EQ(44501477170144023ull, res.digits);
    CHECK_EQ(-324, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(2, 0x0000000000000000L));
    CHECK_EQ(4450147717014403ull, res.digits);
    CHECK_EQ(-323, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(2, 0x0000000000000001L));
    CHECK_EQ(4450147717014404ull, res.digits);
    CHECK_EQ(-323, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(4, 0x0000000000000000L));
    CHECK_EQ(17800590868057611ull, res.digits);
    CHECK_EQ(-323, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(5, 0x0000000000000000L));
    CHECK_EQ(35601181736115222ull, res.digits);
    CHECK_EQ(-323, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(6, 0x0000000000000000L));
    CHECK_EQ(7120236347223045ull, res.digits);
    CHECK_EQ(-322, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(10, 0x0000000000000000L));
    CHECK_EQ(11392378155556871ull, res.digits);
    CHECK_EQ(-321, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(2046, 0x000FFFFFFFFFFFFEL));
    CHECK_EQ(17976931348623155ull, res.digits);
    CHECK_EQ(292, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(2046, 0x000FFFFFFFFFFFFFL)); // max normal
    CHECK_EQ(17976931348623157ull, res.digits);
    CHECK_EQ(292, res.exponent);
}

TEST_CASE("Ryu_Paxson_Kahan")
{
    charconv_ryu::DoubleToDecimalResult res;

    //
    // V. Paxson and W. Kahan, "A Program for Testing IEEE Binary-Decimal Conversion", manuscript, May 1991,
    // ftp://ftp.ee.lbl.gov/testbase-report.ps.Z    (report)
    // ftp://ftp.ee.lbl.gov/testbase.tar.Z          (program)
    //

    //
    // Table 3:
    // Stress Inputs for Converting 53-bit Binary to Decimal, < 1/2 ULP
    //

    res = NonNegativeDoubleToDecimal(Ldexp(8511030020275656L, -342));
    CHECK_EQ(95ull, res.digits);
    CHECK_EQ(-89, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(5201988407066741L, -824));
    CHECK_EQ(465ull, res.digits);
    CHECK_EQ(-235, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6406892948269899L, 237));
    CHECK_EQ(1415ull, res.digits);
    CHECK_EQ(84, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8431154198732492L, 72));
    CHECK_EQ(39815ull, res.digits);
    CHECK_EQ(33, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6475049196144587L, 99));
    CHECK_EQ(410405ull, res.digits);
    CHECK_EQ(40, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8274307542972842L, 726));
    CHECK_EQ(2920845ull, res.digits);
    CHECK_EQ(228, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(5381065484265332L, -456));
    CHECK_EQ(28919465ull, res.digits);
    CHECK_EQ(-129, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6761728585499734L, -1057));
    CHECK_EQ(437877185ull, res.digits);
    CHECK_EQ(-311, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(7976538478610756L, 376));
    CHECK_EQ(1227701635ull, res.digits);
    CHECK_EQ(120, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(5982403858958067L, 377));
    CHECK_EQ(18415524525ull, res.digits);
    CHECK_EQ(119, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(5536995190630837L, 93));
    CHECK_EQ(548357443505ull, res.digits);
    CHECK_EQ(32, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(7225450889282194L, 710));
    CHECK_EQ(3891901811465ull, res.digits);
    CHECK_EQ(217, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(7225450889282194L, 709));
    CHECK_EQ(19459509057325ull, res.digits);
    CHECK_EQ(216, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8703372741147379L, 117));
    CHECK_EQ(144609583816055ull, res.digits);
    CHECK_EQ(37, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8944262675275217L, -1001));
    CHECK_EQ(4173677474585315ull, res.digits);
    CHECK_EQ(-301, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(7459803696087692L, -707));
    CHECK_EQ(11079507728788885ull, res.digits);
    CHECK_EQ(-213, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6080469016670379L, -381));
    CHECK_EQ(1234550136632744ull, res.digits);
    CHECK_EQ(-114, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8385515147034757L, 721));
    CHECK_EQ(925031711960365ull, res.digits);
    CHECK_EQ(218, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(7514216811389786L, -828));
    CHECK_EQ(419804715028489ull, res.digits);
    CHECK_EQ(-248, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8397297803260511L, -345));
    CHECK_EQ(11716315319786511ull, res.digits);
    CHECK_EQ(-104, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6733459239310543L, 202));
    CHECK_EQ(4328100728446125ull, res.digits);
    CHECK_EQ(61, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8091450587292794L, -473));
    CHECK_EQ(3317710118160031ull, res.digits);
    CHECK_EQ(-142, res.exponent);

    //
    // Table 4:
    // Stress Inputs for Converting 53-bit Binary to Decimal, > 1/2 ULP
    //

    res = NonNegativeDoubleToDecimal(Ldexp(6567258882077402L, 952));
    CHECK_EQ(25ull, res.digits);
    CHECK_EQ(301, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6712731423444934L, 535));
    CHECK_EQ(755ull, res.digits);
    CHECK_EQ(174, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6712731423444934L, 534));
    CHECK_EQ(3775ull, res.digits);
    CHECK_EQ(173, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(5298405411573037L, -957));
    CHECK_EQ(43495ull, res.digits);
    CHECK_EQ(-277, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(5137311167659507L, -144));
    CHECK_EQ(230365ull, res.digits);
    CHECK_EQ(-33, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6722280709661868L, 363));
    CHECK_EQ(1263005ull, res.digits);
    CHECK_EQ(119, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(5344436398034927L, -169));
    CHECK_EQ(71422105ull, res.digits);
    CHECK_EQ(-43, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8369123604277281L, -853));
    CHECK_EQ(139345735ull, res.digits);
    CHECK_EQ(-249, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8995822108487663L, -780));
    CHECK_EQ(1414634485ull, res.digits);
    CHECK_EQ(-228, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8942832835564782L, -383));
    CHECK_EQ(45392779195ull, res.digits);
    CHECK_EQ(-110, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8942832835564782L, -384));
    CHECK_EQ(226963895975ull, res.digits);
    CHECK_EQ(-111, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8942832835564782L, -385));
    CHECK_EQ(1134819479875ull, res.digits);
    CHECK_EQ(-112, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6965949469487146L, -249));
    CHECK_EQ(77003665618895ull, res.digits);
    CHECK_EQ(-73, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6965949469487146L, -250));
    CHECK_EQ(385018328094475ull, res.digits);
    CHECK_EQ(-74, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6965949469487146L, -251));
    CHECK_EQ(1925091640472375ull, res.digits);
    CHECK_EQ(-75, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(7487252720986826L, 548));
    CHECK_EQ(68985865317742005ull, res.digits);
    CHECK_EQ(164, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(5592117679628511L, 164));
    CHECK_EQ(13076622631878654ull, res.digits);
    CHECK_EQ(49, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8887055249355788L, 665));
    CHECK_EQ(13605202075612124ull, res.digits);
    CHECK_EQ(200, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(6994187472632449L, 690));
    CHECK_EQ(35928102174759597ull, res.digits);
    CHECK_EQ(207, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8797576579012143L, 588));
    CHECK_EQ(8912519771248455ull, res.digits);
    CHECK_EQ(177, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(7363326733505337L, 272));
    CHECK_EQ(55876975736230114ull, res.digits);
    CHECK_EQ(81, res.exponent);
    res = NonNegativeDoubleToDecimal(Ldexp(8549497411294502L, -448));
    CHECK_EQ(11762578307285404ull, res.digits);
    CHECK_EQ(-135, res.exponent);
}

// The D2s_X tests taken from:
// https://github.com/ulfjack/ryu/blob/master/ryu/tests/d2s_test.cc

TEST_CASE("D2s_LotsOfTrailingZeros")
{
    charconv_ryu::DoubleToDecimalResult res;

    res = NonNegativeDoubleToDecimal(2.98023223876953125E-8);
    CHECK_EQ(29802322387695312ull, res.digits);
    CHECK_EQ(-8-16, res.exponent);
}

TEST_CASE("D2s_Regression")
{
    charconv_ryu::DoubleToDecimalResult res;

    res = NonNegativeDoubleToDecimal(2.109808898695963E16);
    CHECK_EQ(2109808898695963ull, res.digits);
    CHECK_EQ(16-15, res.exponent);
    res = NonNegativeDoubleToDecimal(4.940656E-318);
    CHECK_EQ(4940656ull, res.digits);
    CHECK_EQ(-318-6, res.exponent);
    res = NonNegativeDoubleToDecimal(1.18575755E-316);
    CHECK_EQ(118575755ull, res.digits);
    CHECK_EQ(-316-8, res.exponent);
    res = NonNegativeDoubleToDecimal(2.989102097996E-312);
    CHECK_EQ(2989102097996ull, res.digits);
    CHECK_EQ(-312-12, res.exponent);
    res = NonNegativeDoubleToDecimal(9.0608011534336E15);
    CHECK_EQ(90608011534336ull, res.digits);
    CHECK_EQ(15-13, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(4845900000000000001L));
    CHECK_EQ(9060801153433602ull, res.digits);
    CHECK_EQ(0, res.exponent);
    res = NonNegativeDoubleToDecimal(4.708356024711512E18);
    CHECK_EQ(4708356024711512ull, res.digits);
    CHECK_EQ(18-15, res.exponent);
    res = NonNegativeDoubleToDecimal(9.409340012568248E18);
    CHECK_EQ(9409340012568248ull, res.digits);
    CHECK_EQ(18-15, res.exponent);
    res = NonNegativeDoubleToDecimal(1.2345678);
    CHECK_EQ(12345678ull, res.digits);
    CHECK_EQ(0-7, res.exponent);
}

TEST_CASE("D2s_LooksLikePow5")
{
    charconv_ryu::DoubleToDecimalResult res;

    // These numbers have a mantissa that is a multiple of the largest power of 5 that fits,
    // and an exponent that causes the computation for q to result in 22, which is a corner
    // case for Ryu.

    res = NonNegativeDoubleToDecimal(DoubleFromBits(0x4830F0CF064DD592L));
    CHECK_EQ(5764607523034235ull, res.digits);
    CHECK_EQ(39-15, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(0x4840F0CF064DD592L));
    CHECK_EQ(1152921504606847ull, res.digits);
    CHECK_EQ(40-15, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(0x4850F0CF064DD592L));
    CHECK_EQ(2305843009213694ull, res.digits);
    CHECK_EQ(40-15, res.exponent);
}

TEST_CASE("D2s_Q22")
{
    charconv_ryu::DoubleToDecimalResult res;

    // These numbers have a mantissa that is a multiple of the largest power of 5 that fits,
    // and an exponent that causes the computation for q to result in 22, which is a corner
    // case for Ryu.

    res = NonNegativeDoubleToDecimal(DoubleFromBits(0x4830F0CF064DD591L));
    CHECK_EQ(5764607523034234ull, res.digits);
    CHECK_EQ(39-15, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(0x4840F0CF064DD591L));
    CHECK_EQ(11529215046068467ull, res.digits);
    CHECK_EQ(40-16, res.exponent);
    res = NonNegativeDoubleToDecimal(DoubleFromBits(0x4850F0CF064DD591L));
    CHECK_EQ(23058430092136935ull, res.digits);
    CHECK_EQ(40-16, res.exponent);
}

static std::string Dtoa(double value, bool force_trailing_dot_zero = true)
{
    char buf[32];
    char* end = json::numbers::NumberToString(buf, 32, value, force_trailing_dot_zero);
    return std::string(buf, end);
}

TEST_CASE("Dtoa")
{
    CHECK_EQ("Infinity", Dtoa(std::numeric_limits<double>::infinity()));
    CHECK_EQ("-Infinity", Dtoa(-std::numeric_limits<double>::infinity()));
    CHECK_EQ("NaN", Dtoa(std::numeric_limits<double>::quiet_NaN()));
    CHECK_EQ("NaN", Dtoa(-std::numeric_limits<double>::quiet_NaN()));

    CHECK_EQ("-1.2345e-22"             , Dtoa(-1.2345e-22));
    CHECK_EQ("-1.2345e-21"             , Dtoa(-1.2345e-21));
    CHECK_EQ("-1.2345e-20"             , Dtoa(-1.2345e-20));
    CHECK_EQ("-1.2345e-19"             , Dtoa(-1.2345e-19));
    CHECK_EQ("-1.2345e-18"             , Dtoa(-1.2345e-18));
    CHECK_EQ("-1.2345e-17"             , Dtoa(-1.2345e-17));
    CHECK_EQ("-1.2345e-16"             , Dtoa(-1.2345e-16));
    CHECK_EQ("-1.2345e-15"             , Dtoa(-1.2345e-15));
    CHECK_EQ("-1.2345e-14"             , Dtoa(-1.2345e-14));
    CHECK_EQ("-1.2345e-13"             , Dtoa(-1.2345e-13));
    CHECK_EQ("-1.2345e-12"             , Dtoa(-1.2345e-12));
    CHECK_EQ("-1.2345e-11"             , Dtoa(-1.2345e-11));
    CHECK_EQ("-1.2345e-10"             , Dtoa(-1.2345e-10));
    CHECK_EQ("-1.2345e-9"              , Dtoa(-1.2345e-9 ));
    CHECK_EQ("-1.2345e-8"              , Dtoa(-1.2345e-8 ));
    CHECK_EQ("-1.2345e-7"              , Dtoa(-1.2345e-7 ));
    CHECK_EQ("-0.0000012345"           , Dtoa(-1.2345e-6 ));
    CHECK_EQ("-0.000012345"            , Dtoa(-1.2345e-5 ));
    CHECK_EQ("-0.00012345"             , Dtoa(-1.2345e-4 ));
    CHECK_EQ("-0.0012345"              , Dtoa(-1.2345e-3 ));
    CHECK_EQ("-0.012345"               , Dtoa(-1.2345e-2 ));
    CHECK_EQ("-0.12345"                , Dtoa(-1.2345e-1 ));
    CHECK_EQ("-0.0"                    , Dtoa(-0.0       )); // -0 != +0
    CHECK_EQ( "0"                      , Dtoa( 0.0       ));
    CHECK_EQ( "1.2345"                 , Dtoa( 1.2345e+0 ));
    CHECK_EQ( "12.345"                 , Dtoa( 1.2345e+1 ));
    CHECK_EQ( "123.45"                 , Dtoa( 1.2345e+2 ));
    CHECK_EQ( "1234.5"                 , Dtoa( 1.2345e+3 ));
    CHECK_EQ( "12345"                  , Dtoa( 1.2345e+4 ));
    CHECK_EQ( "123450"                 , Dtoa( 1.2345e+5 ));
    CHECK_EQ( "1234500"                , Dtoa( 1.2345e+6 ));
    CHECK_EQ( "12345000"               , Dtoa( 1.2345e+7 ));
    CHECK_EQ( "123450000"              , Dtoa( 1.2345e+8 ));
    CHECK_EQ( "1234500000"             , Dtoa( 1.2345e+9 ));
    CHECK_EQ( "12345000000"            , Dtoa( 1.2345e+10));
    CHECK_EQ( "123450000000"           , Dtoa( 1.2345e+11));
    CHECK_EQ( "1234500000000"          , Dtoa( 1.2345e+12));
    CHECK_EQ( "12345000000000"         , Dtoa( 1.2345e+13));
    CHECK_EQ( "123450000000000"        , Dtoa( 1.2345e+14));
    CHECK_EQ( "1234500000000000"       , Dtoa( 1.2345e+15));
    CHECK_EQ( "12345000000000000.0"    , Dtoa( 1.2345e+16)); // not exactly representable as integer => force trailing ".0"
    CHECK_EQ( "123450000000000000.0"   , Dtoa( 1.2345e+17)); // not exactly representable as integer => force trailing ".0"
    CHECK_EQ( "1234500000000000000.0"  , Dtoa( 1.2345e+18)); // not exactly representable as integer => force trailing ".0"
    CHECK_EQ( "12345000000000000000.0" , Dtoa( 1.2345e+19)); // not exactly representable as integer => force trailing ".0"
    CHECK_EQ( "123450000000000000000.0", Dtoa( 1.2345e+20)); // not exactly representable as integer => force trailing ".0"
    CHECK_EQ( "1.2345e+21"             , Dtoa( 1.2345e+21));
    CHECK_EQ( "1.2345e+22"             , Dtoa( 1.2345e+22));

    CHECK_EQ("1e+308",   Dtoa(  1e+308));
    CHECK_EQ("1e-308",   Dtoa(  1e-308));
    CHECK_EQ("1.1e+308", Dtoa(1.1e+308));
    CHECK_EQ("1.1e-308", Dtoa(1.1e-308));
}
