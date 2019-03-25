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

#include "json_numbers.h"
#include "json_strings.h" // FIXME: string_view

#include <cstdio>
#include <string>
#include <string_view>

namespace json {

// Stringifier is a simple class that helps to generate JSON documents.
//
// ```C++
// {                                                            // BeginObject();
//     "Image": {                                               //     BeginObject("Image");
//         "Width": 800,                                        //         Number("Width", 800);
//         "Height": 600,                                       //         Number("Height", 600);
//         "Title": "View from 15th Floor",                     //         String("Title", "View from 15th Floor");
//         "Thumbnail": {                                       //         BeginObject("Thumbnail");
//             "Url": "http://www.example.com/image/481989943", //             String("Url", "http://www.example.com/image/481989943");
//             "Height": 125,                                   //             Number("Height", 125);
//             "Width": 100                                     //             Number("Width", 100);
//         },                                                   //         EndObject();
//         "Animated": false,                                   //         Boolean("Animated", false);
//         "IDs": [                                             //         BeginArray("IDs");
//             116,                                             //             Number(116);
//             943,                                             //             Number(943);
//             234,                                             //             Number(234);
//             38793                                            //             Number(38793);
//         ]                                                    //         EndArray();
//     }                                                        //     EndObject();
// }                                                            // EndObject();
// ```
template <typename Writer/* = std::string*/>
class Stringifier
{
    static constexpr uint32_t kMaxDepth = 500;

    enum class StructureKind : uint8_t {
        object,
        array,
    };

    struct StackElement {
        StructureKind kind; // DEBUG-only actually
        bool non_empty;
    };

    Writer& writer;
    StackElement stack[kMaxDepth];
    int depth = 0;
    int indent = 0;

public:
//  using StringParam = std::string const&; // FIXME
    using StringParam = std::string_view;

    explicit Stringifier(Writer& writer_, int indent_ = 0)
        : writer(writer_)
        , indent(indent_ < 10 ? indent_ : 10) // This is what JS does!?
    {
    }

    // Start a member object.
    // This function must only called between calls to BeginObject() and EndObject().
    void BeginObject(StringParam key)
    {
        JSON_ASSERT(stack[depth].kind == StructureKind::object);

        BeginStructured(StructureKind::object, key);
    }

    // Start an object.
    void BeginObject()
    {
        JSON_ASSERT(depth == 0 || stack[depth].kind == StructureKind::array);

        BeginStructured(StructureKind::object);
    }

    // End an object.
    void EndObject()
    {
        JSON_ASSERT(depth > 0 && stack[depth].kind == StructureKind::object);

        EndStructured(StructureKind::object);
    }

    // Start a member array.
    // This function must only called between calls to BeginObject() and EndObject().
    void BeginArray(StringParam key)
    {
        JSON_ASSERT(stack[depth].kind == StructureKind::object);

        BeginStructured(StructureKind::array, key);
    }

    // Start an array.
    void BeginArray()
    {
        JSON_ASSERT(depth == 0 || stack[depth].kind == StructureKind::array);

        BeginStructured(StructureKind::array);
    }

    // End an array.
    void EndArray()
    {
        JSON_ASSERT(depth > 0 && stack[depth].kind == StructureKind::array);

        EndStructured(StructureKind::array);
    }

    // Append a string to the current object.
    // This function must only called between calls to BeginObject() and EndObject().
    void String(StringParam key, StringParam value)
    {
        JSON_ASSERT(stack[depth].kind == StructureKind::object);

        AppendCommaIfNeeded();
        AppendKeyAndColon(key);
        AppendEscapedString(value);
    }

    // Append a string to the current array.
    void String(StringParam value)
    {
        JSON_ASSERT(depth == 0 || stack[depth].kind == StructureKind::array);

        AppendCommaIfNeeded();
        AppendEscapedString(value);
    }

    // Append a number to the current object.
    // This function must only called between calls to BeginObject() and EndObject().
    void Number(StringParam key, double value)
    {
        JSON_ASSERT(stack[depth].kind == StructureKind::object);

        AppendCommaIfNeeded();
        AppendKeyAndColon(key);
        AppendNumber(value);
    }

    // Append a number to the current array.
    void Number(double value)
    {
        JSON_ASSERT(depth == 0 || stack[depth].kind == StructureKind::array);

        AppendCommaIfNeeded();
        AppendNumber(value);
    }

    // Append a boolean to the current object.
    // This function must only called between calls to BeginObject() and EndObject().
    void Boolean(StringParam key, bool value)
    {
        JSON_ASSERT(stack[depth].kind == StructureKind::object);

        AppendCommaIfNeeded();
        AppendKeyAndColon(key);
        AppendBoolean(value);
    }

    // Append a boolean to the current array.
    void Boolean(bool value)
    {
        JSON_ASSERT(depth == 0 || stack[depth].kind == StructureKind::array);

        AppendCommaIfNeeded();
        AppendBoolean(value);
    }

    // Append a null value to the current object.
    // This function must only called between calls to BeginObject() and EndObject().
    void Null(StringParam key)
    {
        JSON_ASSERT(stack[depth].kind == StructureKind::object);

        AppendCommaIfNeeded();
        AppendKeyAndColon(key);
        AppendNull();
    }

    // Append a null value to the current array.
    void Null()
    {
        JSON_ASSERT(depth == 0 || stack[depth].kind == StructureKind::array);

        AppendCommaIfNeeded();
        AppendNull();
    }

    // Append a custom value to the current object.
    // The value must be a valid string representation of a JSON value.
    // This function must only called between calls to BeginObject() and EndObject().
    void Custom(StringParam key, StringParam value)
    {
        JSON_ASSERT(stack[depth].kind == StructureKind::object);
        JSON_ASSERT(!value.empty());
//      JSON_ASSERT(IsJsonValue(value));

        AppendCommaIfNeeded();
        AppendKeyAndColon(key);
        AppendRawString(value);
    }

    // Append a custom value to the current array.
    // The value must be a valid string representation of a JSON value.
    void Custom(StringParam value)
    {
        JSON_ASSERT(depth == 0 || stack[depth].kind == StructureKind::array);
        JSON_ASSERT(!value.empty());
//      JSON_ASSERT(IsJsonValue(value));

        AppendCommaIfNeeded();
        AppendRawString(value);
    }

private:
    void AppendCommaIfNeeded()
    {
        const bool need_comma = stack[depth].non_empty;
        stack[depth].non_empty = true;

        if (need_comma)
            writer.push_back(',');

        if (indent > 0 && depth > 0)
        {
            writer.push_back('\n');
            writer.append(static_cast<size_t>(indent) * depth, ' ');
        }
    }

    void BeginStructured(StructureKind kind, StringParam key)
    {
        AppendCommaIfNeeded();
        AppendKeyAndColon(key);

        writer.push_back((kind == StructureKind::object) ? '{' : '[');

        ++depth;
        stack[depth] = {kind, /*non_empty*/ false};
    }

    void BeginStructured(StructureKind kind)
    {
        AppendCommaIfNeeded();

        writer.push_back((kind == StructureKind::object) ? '{' : '[');

        ++depth;
        stack[depth] = {kind, /*non_empty*/ false};
    }

    void EndStructured(StructureKind kind)
    {
        JSON_ASSERT(stack[depth].kind == kind);

        const bool want_line_break = stack[depth].non_empty;
        --depth;

        if (indent > 0 && want_line_break)
        {
            writer.push_back('\n');
            writer.append(static_cast<size_t>(indent) * depth, ' ');
        }

        writer.push_back((kind == StructureKind::object) ? '}' : ']');
    }

    void AppendKeyAndColon(StringParam key)
    {
        AppendEscapedString(key);

        writer.push_back(':');
        if (indent > 0)
            writer.push_back(' ');
    }

    void AppendRawString(StringParam value)
    {
        writer.append(value);
    }

    void AppendEscapedString(StringParam value)
    {
        writer.push_back('"');

        auto* const next = value.data();
        auto* const last = value.data() + value.size();
        json::strings::EscapeString(next, last, /*allow_invalid_unicode*/ true, [&](char ch) { writer.push_back(ch); });

        writer.push_back('"');
    }

    void AppendNumber(double value)
    {
        if (!std::isfinite(value))
        {
            writer.append("null", 4);
            return;
        }

        char buf[32];
        char* end = json::numbers::NumberToString(buf, 32, value, /*force_trailing_dot_zero*/ true);
        writer.append(buf, static_cast<size_t>(end - buf));
    }

    void AppendBoolean(bool value)
    {
        if (value)
            writer.append("true", 4);
        else
            writer.append("false", 5);
    }

    void AppendNull()
    {
        writer.append("null", 4);
    }
};

} // namespace json
