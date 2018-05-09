// Copyright 2018 Alexander Bolz
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

#include "json_options.h"
#include "json_parse.h" // XXX: For NumberClass

namespace json {
namespace numbers {

// Convert the double-precision number `value` to a decimal floating-point
// number.
// The buffer must be large enough! (size >= 32 is sufficient.)
char* NumberToString(char* next, char* last, double value, bool emit_trailing_dot_zero = true);

// Convert the string `[first, last)` to a double-precision value.
// The string must be valid according to the JSON grammar and match the number
// class defined by `nc` (which must not be `NumberClass::invalid`).
double StringToNumber(char const* first, char const* last, NumberClass nc);

// Convert the string `[first, last)` to a double-precision value.
// Returns true if the string is a valid number according to the JSON grammar.
// Otherwise returns false and stores 'NaN' in `result`.
bool StringToNumber(double& result, char const* first, char const* last, Options const& options = {});

constexpr double kMaxSafeInteger =  9007199254740991.0;
constexpr double kMinSafeInteger = -9007199254740991.0;

// https://tc39.github.io/ecma262/#sec-parsefloat-string
//
// The parseFloat function produces a Number value dictated by interpretation of
// the contents of the string argument as a decimal literal.
//
// NOTE
// parseFloat may interpret only a leading portion of string as a Number value;
// it ignores any code units that cannot be interpreted as part of the notation
// of a decimal literal, and no indication is given that any such code units
// were ignored.
double ParseFloat(char const* first, char const* last);

// https://tc39.github.io/ecma262/#sec-parseint-string-radix
//
// The parseInt function produces an integer value dictated by interpretation of
// the contents of the string argument according to the specified radix. Leading
// white space in string is ignored. If radix is undefined or 0, it is assumed
// to be 10 except when the number begins with the code unit pairs 0x or 0X, in
// which case a radix of 16 is assumed. If radix is 16, the number may also
// optionally begin with the code unit pairs 0x or 0X.
//
// NOTE
// parseInt may interpret only a leading portion of string as an integer value;
// it ignores any code units that cannot be interpreted as part of the notation
// of an integer, and no indication is given that any such code units were
// ignored.
double ParseInt(char const* first, char const* last, int radix = 0);

} // namespace numbers
} // namespace json
