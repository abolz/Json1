FLOAT_POW5_INV_BITCOUNT = 64 # 60
FLOAT_POW5_BITCOUNT = 64 # 63

# MIN_EXPONENT = -29
# MAX_EXPONENT =  47
MIN_EXPONENT = -30
MAX_EXPONENT =  46

def FloorLog2Pow5(e):
    return (e * 1217359) >> 19

def CeilLog2Pow5(e):
    return (e * 1217359 + (2**19 - 1)) >> 19

def BitLengthPow5(e):
    assert e >= 0
    return 1 + FloorLog2Pow5(e)

print "static const int FLOAT_POW5_INV_BITCOUNT = {};".format(FLOAT_POW5_INV_BITCOUNT)
print "static const int FLOAT_POW5_INV_MAX_EXPONENT = {};".format(-MIN_EXPONENT)
print ""
print "// Stores 5^-i in the form:"
print "//   k = ceil(log_2 5^i) - 1 + FLOAT_POW5_INV_BITCOUNT"
print "//   pow[i] = ceil(2^k / 5^i)"
print "static const uint64_t FLOAT_POW5_INV[] = {"

# e2 >= 0
for q in range(0, -MIN_EXPONENT + 1):
    k = CeilLog2Pow5(q) - 1
    j = k + FLOAT_POW5_INV_BITCOUNT
    p, r = divmod(2**j, 5**q)
    if r > 0:
        p += 1 # ceil
    assert p < 2**FLOAT_POW5_INV_BITCOUNT
    assert p >= 2**(FLOAT_POW5_INV_BITCOUNT - 1)
    print "    0x{:016X},".format(p)
    # print "  0x{:016X}, // k = {}".format(p, k)
    # print "  0x{:064b},".format(p)
    # print "  0x{:016X}, // ceil(2^({:3d} + {}) / 5^{:<3d})".format(p, k, FLOAT_POW5_INV_BITCOUNT, q)
    # print "  {:19d}u,".format(p)

print "};"
print ""
print "static const int FLOAT_POW5_BITCOUNT = {};".format(FLOAT_POW5_BITCOUNT)
print "static const int FLOAT_POW5_MAX_EXPONENT = {};".format(MAX_EXPONENT)
print ""
print "// Stores 5^i in the form:"
print "//   k = floor(log_2 5^i) + 1 - FLOAT_POW5_INV_BITCOUNT"
print "//   pow[i] = floor(5^i / 2^k)"
print "static const uint64_t FLOAT_POW5[] = {"

# e2 < 0
for i in range(0, MAX_EXPONENT + 1):
    k = FloorLog2Pow5(i) + 1
    j = k - FLOAT_POW5_BITCOUNT
    if j >= 0:
        p = 5**i // 2**j
    else:
        p = 5**i * 2**(-j)
    assert p < 2**FLOAT_POW5_BITCOUNT
    assert p >= 2**(FLOAT_POW5_BITCOUNT - 1)
    print "    0x{:016X},".format(p)
    # print "  0x{:016X}, // k = {}".format(p, k)
    # print "  0x{:064b},".format(p)
    # print "  0x{:016X}, // floor(5^{:<3d} / 2^({:3d} - {}))".format(p, i, k, FLOAT_POW5_BITCOUNT)
    # print "  {:19d}u,".format(p)

print "};"
