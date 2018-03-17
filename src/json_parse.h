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

#include <cstddef>

namespace json {

enum class ParseStatus {
    success = 0,
    duplicate_key,
    expected_colon_after_key,
    expected_comma_or_closing_brace,
    expected_comma_or_closing_bracket,
    expected_eof,
    expected_key,
    invalid_key,
    invalid_number,
    invalid_string,
    invalid_value,
    max_depth_reached,
    unexpected_eof,
    unexpected_token,
    unrecognized_identifier,
};

enum class NumberClass : unsigned char {
    invalid,
    neg_integer,
    pos_integer,
    floating_point,
    pos_nan,
    neg_nan,
    pos_infinity,
    neg_infinity,
};

struct ParseCallbacks
{
    virtual ~ParseCallbacks() {}

    virtual ParseStatus HandleNull(Options const& options) = 0;
    virtual ParseStatus HandleBoolean(bool value, Options const& options) = 0;
    virtual ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc, Options const& options) = 0;
    virtual ParseStatus HandleString(char const* first, char const* last, bool needs_cleaning, Options const& options) = 0;
    virtual ParseStatus HandleBeginArray(Options const& options) = 0;
    virtual ParseStatus HandleEndArray(size_t count, Options const& options) = 0;
    virtual ParseStatus HandleEndElement(size_t& count, Options const& options) = 0;
    virtual ParseStatus HandleBeginObject(Options const& options) = 0;
    virtual ParseStatus HandleEndObject(size_t count, Options const& options) = 0;
    virtual ParseStatus HandleEndMember(size_t& count, Options const& options) = 0;
    virtual ParseStatus HandleKey(char const* first, char const* last, bool needs_cleaning, Options const& options) = 0;
};

struct ParseResult
{
    ParseStatus ec;
    // On return, PTR denotes the position after the parsed value, or if an
    // error occurred, denotes the position of the invalid token.
    char const* ptr;
    // If an error occurred, END denotes the position after the invalid token.
    // This field is unused on success.
    char const* end;
};

// Parse the JSON stored in the string [first, last).
ParseResult parse(ParseCallbacks& cb, char const* first, char const* last, Options const& options = {});

} // namespace json
