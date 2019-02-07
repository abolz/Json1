# Translated from:
# https://github.com/ulfjack/ryu/blob/c9c3fb19791c44fbe35701ad3b8ca4dc0977eb08/src/main/java/info/adams/ryu/analysis/ComputeTableSizes.java

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

def FloorLog10Pow2(e):
    return (e * 315653) >> 20

def FloorLog10Pow5(e):
    return (e * 732923) >> 20

def ComputeTableSizes(exponentBits, mantissaBits):
    bias = 2**(exponentBits - 1) - 1

    minE2 = 1 - bias - mantissaBits - 2
    maxE2 = (2**exponentBits - 2) - bias - mantissaBits - 2

    posE2maxQ = max(0, FloorLog10Pow2(maxE2) - 1)
    negE2maxQ = max(0, FloorLog10Pow5(-minE2) - 1)

    maxNegExp = posE2maxQ
    maxPosExp = -minE2 - negE2maxQ

    return [maxNegExp, maxPosExp]

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------

# float16: [0, 9]
b = ComputeTableSizes(5, 10)
print(b)

# float32: [29, 47]
b = ComputeTableSizes(8, 23)
print(b)

# float64: [290, 325]
b = ComputeTableSizes(11, 52)
print(b)

# float80: [4911, 4953]
b = ComputeTableSizes(15, 63)
print(b)

# float128: [4896, 4967]
b = ComputeTableSizes(15, 112)
print(b)

# float256: [78840, 78986]
b = ComputeTableSizes(19, 236)
print(b)
