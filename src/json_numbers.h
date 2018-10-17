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

#define JSON_NUMBERS_USE_GRISU2 1

#include "json_parser.h" // NumberClass, Options

#include <cstring>

namespace json {
namespace numbers {

// Convert the double-precision number `value` to a decimal floating-point
// number.
// The buffer must be large enough! (size >= 32 is sufficient.)
char* NumberToString(char* buffer, int buffer_length, double value, bool force_trailing_dot_zero = true);

// Convert the string `[first, last)` to a double-precision value.
// The string must be valid according to the JSON grammar and match the number
// class defined by `nc` (which must not be `NumberClass::invalid`).
double StringToNumber(char const* first, char const* last, NumberClass nc);

// Convert the string `[first, last)` to a double-precision value.
// Returns true if the string is a valid number according to the JSON grammar.
// Otherwise returns false and stores 'NaN' in `result`.
bool StringToNumber(double& result, char const* first, char const* last, Options const& options = {});

} // namespace numbers
} // namespace json
