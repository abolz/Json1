import fractions
import math
import sys
import decimal
import textwrap

Precision, ExponentBits = 53, 11
# Precision, ExponentBits = 24, 8

HiddenBit = 2**(Precision - 1)
Bias = 2**(ExponentBits - 1) - 1 + (Precision - 1)
MinExponent = 1 - Bias
MaxExponent = 2**ExponentBits - 2 - Bias

#===================================================================================================
# Decimal to binary
#===================================================================================================

def BinaryFromFraction(num, den):
    """Converts x = num / den to IEEE binary floating-point x' = f * 2^e"""

    assert num != 0
    assert den != 0

    f = 0
    e = 0
    p = Precision
    isExact = True

    # g = fractions.gcd(num, den)
    # num //= g
    # den //= g

    # Scale into [2^(p-1), 2^p)
    while num >= 2**p * den:
        den *= 2
        e += 1
    while num < 2**(p - 1) * den:
        num *= 2
        e -= 1

    # For subnormal numbers, try to reduce the precision of the
    # significand to get the exponent into the valid range.
    while e < MinExponent and p > 1:
        den *= 2
        e += 1
        p -= 1

    # Divide and round
    f, r = divmod(num, den)
    assert f >= 0
    assert f < 2**Precision
    isExact = (r == 0)
    if not isExact:
        # Round to nearest-even
        if 2 * r > den or (2 * r == den and (f % 2) != 0):
            f += 1
            if f == 2**Precision:
                # Overflow.
                # Move a trailing zero into the exponent.
                f = 2**(Precision - 1)
                e += 1

    assert f > 0
    assert f < 2**Precision

    return f, e, p, isExact

def BinaryFromDecimal(d, k):
    """Converts x = d * 10^k to IEEE binary floating-point x' = f * 2^e"""

    if k >= 0:
        num = d * 10**k
        den = 1
    else:
        num = d
        den = 10**(-k)

    return BinaryFromFraction(num, den)

def BinaryFromDecimalString(s):
    """Converts the input string s to [f, e, p, isExact]"""

    _, digits, exponent = decimal.Decimal(s).as_tuple()
    d = reduce(lambda s, d: s * 10 + d, digits)
    return BinaryFromDecimal(d, exponent)

def BinaryFromFloat(v):
    """Converts v to [f, e, p, isExact=True]"""

    f = fractions.Fraction(v)
    return BinaryFromFraction(f.numerator, f.denominator)

#===================================================================================================
# Binary to decimal
#===================================================================================================

# Burger, Dybvig, "Printing Floating-Point Numbers Quickly and Accurately",
# Proceedings of the ACM SIGPLAN 1996 Conference on Programming Language
# Design and Implementation, PLDI 1996

def EffectivePrecision(f):
    """Returns the effective precision of the significand, aka. f.bit_length()"""

    assert f > 0
    assert f < 2**Precision

    p = Precision
    while f < 2**(Precision - 1):
        f *= 2
        p -= 1

    return p

def FloorLog10Pow2(e):
    """Returns floor(log_10(2^e))"""

    assert e >= -1650
    assert e <=  1650
    return (int(e) * 78913) // 2**18 # floor-division (SAR)

def CeilLog10Pow2(e):
    """Returns ceil(log_10(2^e))"""

    assert e >= -1650
    assert e <=  1650
    return (int(e) * 78913 + (2**18 - 1)) // 2**18 # floor-division (SAR)

def BurgerDybvig(f, e, p):
    assert f > 0
    assert f < 2**Precision
    assert e >= MinExponent
    assert e <= MaxExponent

    #
    # Init
    #

    isEven = (f % 2 == 0)
    acceptBounds = isEven
    lowerBoundaryIsCloser = e != MinExponent and f == HiddenBit

    if e >= 0:
        r, s, mp, mm = f * 2 * 2**e, 2, 2**e, 2**e
    else:
        r, s, mp, mm = f * 2, 2**(-e) * 2, 1, 1

    # Could do after scaling to keep the numbers a tiny bit smaller?!?!
    if lowerBoundaryIsCloser:
        r, s, mp = r * 2, s * 2, mp * 2

    #
    # Scale into the range [0.1, 1.0)
    #   aka: Find the smallest integer k, such that (r + m+) / s <= 10^k
    #   aka: k = ceil(log_10((r + m+) / s))
    #

    if False:
        k = 0
        while True:
            rp = r + mp
            if (rp >= s) if acceptBounds else (rp > s):
                s, k = s * 10, k + 1
            elif (rp * 10 < s) if acceptBounds else (rp * 10 <= s):
                r, mp, mm, k = r * 10, mp * 10, mm * 10, k - 1
            else:
                break
    else:
        p = f.bit_length() # Effective precision

        # Estimate:
        k = CeilLog10Pow2(e + (p - 1))
        if k >= 0:
            s *= 10**k
        else:
            r, mp, mm = r * 10**(-k), mp * 10**(-k), mm * 10**(-k)

        # Fixup:
        if (r + mp >= s) if acceptBounds else (r + mp > s):
            s, k = s * 10, k + 1

    assert (r + mp < s) if acceptBounds else (r + mp <= s)
    assert ((r + mp) * 10 >= s) if acceptBounds else ((r + mp) * 10 > s)

    #
    # Generate
    #

    d = []
    while True:
        assert r > 0

        r, mp, mm = r * 10, mp * 10, mm * 10
        q, r = divmod(r, s)
        assert q <= 9

        tc1 = (r <= mm) if acceptBounds else (r < mm)
        tc2 = (r + mp >= s) if acceptBounds else (r + mp > s)

        if tc1 and tc2:
            # Return the number closer to v. If the two are equidistant
            # from v, use **some** strategy to break the tie.
            if (r * 2 > s) or (r * 2 == s and q % 2 != 0):
                q += 1
        elif not tc1 and tc2:
            q += 1

        d.append(q) # d = 10 * d + q
        k -= 1

        if tc1 or tc2:
            break

    return d, k # result = d * 10^k

def ShortestDecimalStringFromBinary(f, e, p):
    d, k = BurgerDybvig(f, e, p)
    return ''.join(map(str, d)), k

def ShortestDecimalFromBinary(f, e, p):
    d, k = BurgerDybvig(f, e, p)
    return int(''.join(map(str, d))), k

#===================================================================================================
#
#===================================================================================================

def PrintBinary(f, e, p, isExact):
    print "Precision: {}".format(p)
    print "Exact: {}".format(isExact)
    print ""
    print "Decimal times power-of-2:"
    print ""
    print "   {} * 2^{}".format(f, e)
    print ""
    print "Shortest decimal times power-of-10:"
    print ""
    if f == 0:
        print "   0 * 10^{}".format(e)
    elif e < MinExponent:
        print "   0 [underflow: e = {}]".format(e)
    elif e > MaxExponent:
        print "   Infinity [overflow: e = {}]".format(e)
    else:
        print "   {} * 10^{}".format(*ShortestDecimalStringFromBinary(f, e, p))
    print ""

def TestDecimalString(s):
    print "========================================================================"
    print "Input:"
    print ""
    for line in textwrap.wrap(s, 72-4):
        print "    {}".format(line)
    print ""
    PrintBinary(*BinaryFromDecimalString(s))

TestDecimalString('12345678e-7')
TestDecimalString('12345677999999999929769955997471697628498077392578125e-52')

TestDecimalString('5e-324')
# TestDecimalString( # denorm_min [precision = 1, isExact = True]
#     '4940656458412465441765687928682213723650598026143247644255856825006'
#     '7550727020875186529983636163599237979656469544571773092665671035593'
#     '9796398774796010781878126300713190311404527845817167848982103688718'
#     '6360569987307230500063874091535649843873124733972731696151400317153'
#     '8539807412623856559117102665855668676818703956031062493194527159149'
#     '2455329305456544401127480129709999541931989409080416563324524757147'
#     '8690147267801593552386115501348035264934720193790268107107491703332'
#     '2268447533357208324319360923828934583680601060115061698097530783422'
#     '7731832924790498252473077637592724787465608477820373446969953364701'
#     '7972677717585125660551199131504891101451037862738167250955837389733'
#     '5989936648099411642057026370902792427675445652290875386825064197182'
#     '65533447265625e-1074')
# TestDecimalString( # denorm_min * 10 [precision = 4, isExact = False]
#     '4940656458412465441765687928682213723650598026143247644255856825006'
#     '7550727020875186529983636163599237979656469544571773092665671035593'
#     '9796398774796010781878126300713190311404527845817167848982103688718'
#     '6360569987307230500063874091535649843873124733972731696151400317153'
#     '8539807412623856559117102665855668676818703956031062493194527159149'
#     '2455329305456544401127480129709999541931989409080416563324524757147'
#     '8690147267801593552386115501348035264934720193790268107107491703332'
#     '2268447533357208324319360923828934583680601060115061698097530783422'
#     '7731832924790498252473077637592724787465608477820373446969953364701'
#     '7972677717585125660551199131504891101451037862738167250955837389733'
#     '5989936648099411642057026370902792427675445652290875386825064197182'
#     '65533447265625e-1073')

TestDecimalString('17976931348623157e292') # max
TestDecimalString('22250738585072014e-324') # min

# Half way between max-normal and infinity
# Should round to infinity in nearest-even mode.
TestDecimalString(
    '1797693134862315807937289714053034150799341327100378269361737789804'
    '4496829276475094664901797758720709633028641669288791094655554785194'
    '0402630657488671505820681908902000708383676273854845817711531764475'
    '7302700698555713669596228429148198608349364752927190741684443655107'
    '04342711559699508093042880177904174497792')
# Just below.
# Should round to max
TestDecimalString(
    '1797693134862315807937289714053034150799341327100378269361737789804'
    '4496829276475094664901797758720709633028641669288791094655554785194'
    '0402630657488671505820681908902000708383676273854845817711531764475'
    '7302700698555713669596228429148198608349364752927190741684443655107'
    '04342711559699508093042880177904174497791')

# def TestFloat(v):
#     print "------------------------------------------------------------------------"
#     print "Input: {0:.16e} = {0}".format(v)
#     print "------------------------------------------------------------------------"
#     PrintBinary(*BinaryFromFloat(v))
# TestFloat(0.3)
# TestFloat(0.1 + 0.2)
