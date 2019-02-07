DOUBLE_POW5_INV_BITCOUNT = 128 # 124
DOUBLE_POW5_BITCOUNT = 128 # 124

MIN_EXPONENT = -348
MAX_EXPONENT =  326

def FloorLog2Pow5(e):
    return (e * 1217359) >> 19

def CeilLog2Pow5(e):
    return (e * 1217359 + (2**19 - 1)) >> 19

print "static const int DOUBLE_POW5_INV_BITCOUNT = {};".format(DOUBLE_POW5_INV_BITCOUNT)
print "static const int DOUBLE_POW5_INV_MAX_EXPONENT = {};".format(-MIN_EXPONENT)
print ""
print "// Stores 5^-i in the form:"
print "//   k = ceil(log_2 5^i) - 1 + DOUBLE_POW5_INV_BITCOUNT"
print "//   pow[i] = ceil(2^k / 5^i)"
print "static const uint64_t DOUBLE_POW5_INV[][2] = {"

# e2 >= 0
for q in range(0, -MIN_EXPONENT + 1):
    k = CeilLog2Pow5(q) - 1
    j = k + DOUBLE_POW5_INV_BITCOUNT
    num = 2**j
    den = 5**q
    p, r = divmod(num, den)
    if r > 0:
        p += 1 # ceil
    assert p < 2**DOUBLE_POW5_INV_BITCOUNT
    assert p >= 2**(DOUBLE_POW5_INV_BITCOUNT - 1)
    pHi, pLo = divmod(p, 2**64)
    print "    {{ 0x{:016X}, 0x{:016X} }},".format(pHi, pLo)

print "};"
print ""
print "static const int DOUBLE_POW5_BITCOUNT = {};".format(DOUBLE_POW5_BITCOUNT)
print "static const int DOUBLE_POW5_MAX_EXPONENT = {};".format(MAX_EXPONENT)
print ""
print "// Stores 5^i in the form:"
print "//   k = floor(log_2 5^i) + 1 - DOUBLE_POW5_INV_BITCOUNT"
print "//   pow[i] = floor(5^i / 2^k)"
print "static const uint64_t FLOAT_POW5[][2] = {"

# e2 <= 0
for i in range(0, MAX_EXPONENT + 1):
    k = FloorLog2Pow5(i) + 1
    j = k - DOUBLE_POW5_BITCOUNT
    if j >= 0:
        p = 5**i // 2**j
    else:
        p = 5**i * 2**(-j)
    assert p < 2**DOUBLE_POW5_BITCOUNT
    assert p >= 2**(DOUBLE_POW5_BITCOUNT - 1)
    pHi, pLo = divmod(p, 2**64)
    print "    {{ 0x{:016X}, 0x{:016X} }},".format(pHi, pLo)

print "};"
