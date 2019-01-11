// Copyright 2017 Alexander Bolz
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

#include "json_numbers.h"
#include "json_parser.h"
#include "json_strings.h"

//#include <rapidjson/document.h>
//#include <rapidjson/stringbuffer.h>

#include <cmath>

namespace json {

//==================================================================================================
// Parse
//==================================================================================================

namespace impl {

struct RapidjsonDocumentReader
{
    static constexpr bool kCopyCleanStrings = true;

    rapidjson::Document* doc;

    RapidjsonDocumentReader(rapidjson::Document* doc_)
        : doc(doc_)
    {
    }

    ParseStatus HandleNull()
    {
        doc->Null();
        return {};
    }

    ParseStatus HandleTrue()
    {
        doc->Bool(true);
        return {};
    }

    ParseStatus HandleFalse()
    {
        doc->Bool(false);
        return {};
    }

    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc)
    {
        // TODO:
        // nc == NumberClass::integer

        doc->Double(json::numbers::StringToNumber(first, last, nc));
        return {};
    }

    ParseStatus HandleString(char const* first, char const* last, StringClass sc)
    {
        if (sc == StringClass::needs_cleaning)
        {
            std::string str;
            str.reserve(static_cast<size_t>(last - first));

            auto const res = json::strings::UnescapeString(first, last, [&](char ch) { str.push_back(ch); });
            if (res.ec != json::strings::Status::success) {
                return ParseStatus::invalid_string;
            }

            doc->String(str.data(), static_cast<rapidjson::SizeType>(str.size()), /*copy*/ true);
        }
        else
        {
            doc->String(first, static_cast<rapidjson::SizeType>(last - first), /*copy*/ kCopyCleanStrings);
        }
        return {};
    }

    ParseStatus HandleBeginArray()
    {
        doc->StartArray();
        return {};
    }

    ParseStatus HandleEndArray(size_t count)
    {
        doc->EndArray(static_cast<rapidjson::SizeType>(count));
        return {};
    }

    ParseStatus HandleEndElement(size_t& /*count*/)
    {
        return {};
    }

    ParseStatus HandleBeginObject()
    {
        doc->StartObject();
        return {};
    }

    ParseStatus HandleEndObject(size_t count)
    {
        doc->EndObject(static_cast<rapidjson::SizeType>(count));
        return {};
    }

    ParseStatus HandleEndMember(size_t& /*count*/)
    {
        return {};
    }

    ParseStatus HandleKey(char const* first, char const* last, StringClass sc)
    {
        if (sc == StringClass::needs_cleaning)
        {
            std::string str;
            str.reserve(static_cast<size_t>(last - first));

            auto const res = json::strings::UnescapeString(first, last, [&](char ch) { str.push_back(ch); });
            if (res.ec != json::strings::Status::success) {
                return ParseStatus::invalid_string;
            }

            doc->Key(str.data(), static_cast<rapidjson::SizeType>(str.size()), /*copy*/ true);
        }
        else
        {
            doc->Key(first, static_cast<rapidjson::SizeType>(last - first), /*copy*/ kCopyCleanStrings);
        }
        return {};
    }
};

} // namespace impl

inline ParseResult ParseRapidjsonDocument(rapidjson::Document& document, char const* next, char const* last)
{
    ParseResult res;

    auto gen = [&](rapidjson::Document& doc)
    {
        json::impl::RapidjsonDocumentReader cb(&doc);
        res = json::ParseSAX(cb, next, last);
        return res.ec == json::ParseStatus::success;
    };

    document.Populate(gen);
    return res;
}

//==================================================================================================
// Stringify
//==================================================================================================

struct RapidjsonStringifyOptions
{
    // If true, prints NaN and Infinity values as "NaN" or "Infinity", resp.
    // If false, prints these special values as "null".
    // Default is false.
    bool allow_nan_inf = false;

    // If > 0, pretty-print the JSON.
    // Default is <= 0, that is the JSON is rendered as the shortest string possible.
    int indent_width = -1;
};

namespace impl {

template <typename OutputStream>
inline void WriteString(OutputStream& out, char const* str, size_t len)
{
    out.Reserve(len);
    for (size_t i = 0; i < len; ++i) {
        out.PutUnsafe(str[i]);
    }
}

template <typename OutputStream>
inline void WriteChars(OutputStream& out, char ch, size_t count)
{
    out.Reserve(count);
    for (size_t i = 0; i < count; ++i) {
        out.PutUnsafe(ch);
    }
}

template <typename OutputStream>
inline void WriteNumber(OutputStream& out, double num)
{
    char buf[32];
    char* const end = json::numbers::NumberToString(buf, 32, num, /*force_trailing_dot_zero*/ true);
    json::impl::WriteString(out, buf, static_cast<size_t>(end - buf));
}

inline void WriteString(rapidjson::StringBuffer& out, char const* str, size_t len)
{
    char* const buf = out.Push(len);
    std::memcpy(buf, str, len);
}

inline void WriteChars(rapidjson::StringBuffer& out, char ch, size_t count)
{
    char* const buf = out.Push(count);
    std::memset(buf, static_cast<unsigned char>(ch), count);
}

inline void WriteNumber(rapidjson::StringBuffer& out, double num)
{
    char* const buf = out.Push(32);
    char* const end = json::numbers::NumberToString(buf, 32, num, /*force_trailing_dot_zero*/ true);
    out.Pop(32 - static_cast<size_t>(end - buf));
}

template <typename OutputStream>
inline bool StringifyValue(OutputStream& out, rapidjson::Value const& value, RapidjsonStringifyOptions const& options, int curr_indent);

template <typename OutputStream>
inline bool StringifyNumber(OutputStream& out, rapidjson::Value const& value, RapidjsonStringifyOptions const& options)
{
    double num;
    if (value.IsDouble())
        num = value.GetDouble();
    else if (value.IsInt())
        num = value.GetInt();
    else if (value.IsUint())
        num = value.GetUint();
    else if (value.IsInt64())
        num = static_cast<double>(value.GetInt64());
    else if (value.IsUint64())
        num = static_cast<double>(value.GetUint64());
    else
        return false;

    if (!options.allow_nan_inf && !std::isfinite(num))
    {
        json::impl::WriteString(out, "null", 4);
        return true;
    }

    json::impl::WriteNumber(out, num);
    return true;
}

template <typename OutputStream>
inline bool StringifyString(OutputStream& out, rapidjson::Value const& value, RapidjsonStringifyOptions const& /*options*/)
{
    JSON_ASSERT(value.IsString());

    auto const str = value.GetString();
    auto const len = value.GetStringLength();

    out.Reserve(len + 2);
    out.Put('"');

    if (len != 0)
    {
        auto const res = json::strings::EscapeString(str, str + len, [&](char ch) { out.Put(ch); }, /*allow_invalid_unicode*/ true);
        if (res.ec != json::strings::Status::success) {
            return false;
        }
    }

    out.Put('"');

    return true;
}

template <typename OutputStream>
inline bool StringifyArray(OutputStream& out, rapidjson::Value const& value, RapidjsonStringifyOptions const& options, int curr_indent)
{
    out.Put('[');

    auto       I = value.Begin();
    auto const E = value.End();
    if (I != E)
    {
        if (options.indent_width > 0)
        {
            // Prevent overflow in curr_indent + options.indent_width
            int const indent_width = (curr_indent <= INT_MAX - options.indent_width) ? options.indent_width : 0;
            curr_indent += indent_width;

            for (;;)
            {
                out.Put('\n');
                json::impl::WriteChars(out, ' ', static_cast<size_t>(curr_indent));

                if (!json::impl::StringifyValue(out, *I, options, curr_indent))
                    return false;
                if (++I == E)
                    break;
                out.Put(',');
            }

            curr_indent -= indent_width;

            out.Put('\n');
            json::impl::WriteChars(out, ' ', static_cast<size_t>(curr_indent));
        }
        else
        {
            for (;;)
            {
                if (!json::impl::StringifyValue(out, *I, options, curr_indent))
                    return false;
                if (++I == E)
                    break;
                out.Put(',');
            }
        }
    }

    out.Put(']');

    return true;
}

template <typename OutputStream>
static bool StringifyObject(OutputStream& out, rapidjson::Value const& value, RapidjsonStringifyOptions const& options, int curr_indent)
{
    out.Put('{');

    auto       I = value.MemberBegin();
    auto const E = value.MemberEnd();
    if (I != E)
    {
        if (options.indent_width > 0)
        {
            // Prevent overflow in curr_indent + options.indent_width
            int const indent_width = (curr_indent <= INT_MAX - options.indent_width) ? options.indent_width : 0;
            curr_indent += indent_width;

            for (;;)
            {
                out.Put('\n');
                json::impl::WriteChars(out, ' ', static_cast<size_t>(curr_indent));

                if (!json::impl::StringifyString(out, I->name, options))
                    return false;
                out.Put(':');
                out.Put(' ');
                if (!json::impl::StringifyValue(out, I->value, options, curr_indent))
                    return false;
                if (++I == E)
                    break;
                out.Put(',');
            }

            curr_indent -= indent_width;

            out.Put('\n');
            json::impl::WriteChars(out, ' ', static_cast<size_t>(curr_indent));
        }
        else
        {
            for (;;)
            {
                if (!json::impl::StringifyString(out, I->name, options))
                    return false;
                out.Put(':');
                if (!json::impl::StringifyValue(out, I->value, options, curr_indent))
                    return false;
                if (++I == E)
                    break;
                out.Put(',');
            }
        }
    }

    out.Put('}');

    return true;
}

template <typename OutputStream>
inline bool StringifyValue(OutputStream& out, rapidjson::Value const& value, RapidjsonStringifyOptions const& options, int curr_indent)
{
    switch (value.GetType())
    {
    case rapidjson::kNullType:
        json::impl::WriteString(out, "null", 4);
        return true;
    case rapidjson::kTrueType:
        json::impl::WriteString(out, "true", 4);
        return true;
    case rapidjson::kFalseType:
        json::impl::WriteString(out, "false", 5);
        return true;
    case rapidjson::kNumberType:
        return json::impl::StringifyNumber(out, value, options);
    case rapidjson::kStringType:
        return json::impl::StringifyString(out, value, options);
    case rapidjson::kArrayType:
        return json::impl::StringifyArray(out, value, options, curr_indent);
    case rapidjson::kObjectType:
        return json::impl::StringifyObject(out, value, options, curr_indent);
    default:
        JSON_ASSERT(false && "invalid type");
        return false;
    }
}

} // namespace impl

template <typename OutputStream>
inline bool StringifyRapidjsonValue(OutputStream& out, rapidjson::Value const& value, RapidjsonStringifyOptions const& options = {})
{
    return json::impl::StringifyValue(out, value, options, 0);
}

} // namespace json
