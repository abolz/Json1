# Translated from:
# https://github.com/ulfjack/ryu/blob/c9c3fb19791c44fbe35701ad3b8ca4dc0977eb08/src/main/java/info/adams/ryu/analysis/ComputeRequiredBitSizes.java

# Copyright 2018 Ulf Adams
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from EuclidMinMax import EuclidMin, EuclidMax

def FloorLog10Pow2(e):
    return (e * 315653) >> 20

def FloorLog10Pow5(e):
    return (e * 732923) >> 20

# Computes appropriate values for B_0 and B_1 for a given floating point type.
def ComputeRequiredBitSizes(totalBits, exponentBits, mantissaBits):
    bias = 2**(exponentBits - 1) - 1
    mbits = mantissaBits + 3

    # max(w) = 4 * ((1 << format.mantissaBits()) * 2 - 1) + 2
    #        = (1 << (format.mantissaBits() + 3)) - 2
    max_w = 2**mbits - 2

    minE2 = 0
    maxE2 = (2**exponentBits - 2) - bias - mantissaBits - 2
    print "minE2 = {}, maxE2 = {}".format(minE2, maxE2)

    b0 = 0
    for e2 in range(minE2, maxE2 + 1):
        if (e2 % 100 == 0):
            print "e2 = {}, b0 = {}".format(e2, b0)

        q = max(0, FloorLog10Pow2(e2) - 1)
        pow5 = 5**q
        pow2 = 2**(e2 - q)

        euclid_max = EuclidMax(pow2, pow5, max_w - 1)
        bits = ((max_w * pow5 * pow2) / (pow5 - euclid_max)).bit_length()
        reqn = bits - pow5.bit_length()

        b0 = max(b0, reqn)

    print(b0)

    minE2 = 0
    maxE2 = -(1 - bias - mantissaBits - 2)
    print "minE2 = {}, maxE2 = {}".format(minE2, maxE2)

    b1 = 0
    for e2 in range(minE2, maxE2 + 1):
        if (e2 % 100 == 0):
            print "e2 = {}, b1 = {}".format(-e2, b1)

        q = max(0, FloorLog10Pow5(e2) - 1)
        pow5 = 5**(e2 - q)
        pow2 = 2**q

        euclid_min = EuclidMin(pow5, pow2, max_w - 1)
        bits = (euclid_min / max_w).bit_length()
        reqn = pow5.bit_length() - bits

        b1 = max(b1, reqn)

    print(b1)

    return [b0, b1]

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------

# # float16: [15, 21]
# b = ComputeRequiredBitSizes(16, 5, 10)
# print(b)

# # float32: [60, 63]
# b = ComputeRequiredBitSizes(32, 8, 23)
# print(b)

# # float64: [124, 124]
# b = ComputeRequiredBitSizes(64, 11, 52)
# print(b)

# # float80: [150, 152]
# b = ComputeRequiredBitSizes(80, 15, 63)
# print(b)

# # float128: [249, 246]
# b = ComputeRequiredBitSizes(128, 15, 112)
# print(b)

# # float256:
# b = ComputeRequiredBitSizes(256, 19, 236)
# print(b)
