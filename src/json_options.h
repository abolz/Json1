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

#include <cstdint>

namespace json {

struct Options
{
    // If true, skip line comments (introduced with "//") and block
    // comments like "/* hello */".
    // Default is false.
    bool strip_comments = false;

    // If true, parses "NaN" and "Infinity" (without the quotes) as numbers.
    // Default is true.
    bool allow_nan_inf = true;

    // If true, skip UTF-8 byte order mark - if any.
    // Default is true.
    bool skip_bom = true;

    // If true, allows trailing commas in arrays or objects.
    // Default is false.
    bool allow_trailing_comma = false;

    // If true, parse numbers as raw strings.
    // Default is false.
    bool parse_numbers_as_strings = false;

    // If true, allow characters after value.
    // Might be used to parse strings like "[1,2,3]{"hello":"world"}" into
    // different values by repeatedly calling parse.
    // Default is false.
    bool allow_trailing_characters = false;

    // If >= 0, pretty-print the JSON.
    // Default is < 0, that is the JSON is rendered as the shortest string possible.
    int8_t indent_width = -1;
};

} // namespace json
