#include "bench_json1.h"

#include <memory>

#define JSON_STRICT 1
#if !BENCH_FULLPREC
//#define JSON_CONVERT_NUMBERS 0
#endif
#include "../src/json_parser.h"
#include "../src/json_strings.h"
#include "../src/json_numbers.h"

#if BENCH_RAPIDJSON_DOCUMENT
#include "../ext/rapidjson/document.h"
#include "../ext/rapidjson/stringbuffer.h"
//#include "../src/json_rapidjson_interop.h"
#else
#include "../src/json.h"
#include "traverse.h"
#endif

#include <cstdio>

#define USE_STRING_BUFFER 0

using namespace json;

namespace {

struct GenStatsCallbacks
{
    jsonstats& stats;

    GenStatsCallbacks(jsonstats& s) : stats(s) {}

    ParseStatus HandleNull()
    {
        ++stats.null_count;
        return {};
    }

    ParseStatus HandleTrue()
    {
        ++stats.true_count;
        return {};
    }

    ParseStatus HandleFalse()
    {
        ++stats.false_count;
        return {};
    }

#if JSON_CONVERT_NUMBERS
    ParseStatus HandleNumber(double value, NumberClass nc)
    {
        if (nc == NumberClass::invalid)
            return ParseStatus::invalid_number;

        ++stats.number_count;
        stats.total_number_value.Add(value);
        return {};
    }
#else
    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc)
    {
        if (nc == NumberClass::invalid)
            return ParseStatus::invalid_number;

        ++stats.number_count;
        stats.total_number_value.Add(json::numbers::StringToNumber(first, last, nc));
        return {};
    }
#endif

    ParseStatus HandleString(char const* first, char const* last, StringClass sc)
    {
        ++stats.string_count;
        return AddStringLength(stats.total_string_length, first, last, sc);
    }

    ParseStatus HandleBeginArray()
    {
        ++stats.array_count;
        return {};
    }

    ParseStatus HandleEndArray(size_t count)
    {
        stats.total_array_length += count;
        return {};
    }

    ParseStatus HandleEndElement(size_t /*count*/)
    {
        return {};
    }

    ParseStatus HandleBeginObject()
    {
        ++stats.object_count;
        return {};
    }

    ParseStatus HandleEndObject(size_t count)
    {
        stats.total_object_length += count;
        return {};
    }

    ParseStatus HandleEndMember(size_t /*count*/)
    {
        return {};
    }

    ParseStatus HandleKey(char const* first, char const* last, StringClass sc)
    {
        ++stats.key_count;
        return AddStringLength(stats.total_key_length, first, last, sc);
    }

private:
    static ParseStatus AddStringLength(size_t& dest, char const* first, char const* last, StringClass sc)
    {
        intptr_t len = 0;
        if (sc != StringClass::clean)
        {
            auto const res = json::strings::UnescapeString(first, last, /*allow_invalid_unicode*/ false, [&](char) { ++len; });
            if (res.ec != json::strings::Status::success) {
                return ParseStatus::invalid_string;
            }
        }
        else
        {
            len = last - first;
        }

        dest += static_cast<size_t>(len);
        return {};
    }
};

#if BENCH_RAPIDJSON_DOCUMENT
using RapidjsonDocument = rapidjson::Document;

struct RapidjsonDocumentReader
{
    RapidjsonDocument* doc;
    char* string_buffer;

    RapidjsonDocumentReader(RapidjsonDocument* doc_, char* string_buffer_)
        : doc(doc_)
        , string_buffer(string_buffer_)
    {
        static_cast<void>(string_buffer_);
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

#if JSON_CONVERT_NUMBERS
    ParseStatus HandleNumber(double value, NumberClass nc)
    {
        if (nc == NumberClass::invalid)
            return ParseStatus::invalid_number;

        doc->Double(value);
        return {};
    }
#else
    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc)
    {
        if (nc == NumberClass::invalid)
            return ParseStatus::invalid_number;

        doc->Double(json::numbers::StringToNumber(first, last, nc));
        return {};
    }
#endif

    ParseStatus HandleString(char const* first, char const* last, StringClass sc)
    {
        return HandleStringImpl(first, last, sc);
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
        return HandleStringImpl(first, last, sc);
    }

    inline ParseStatus HandleStringImpl(char const* first, char const* last, StringClass sc)
    {
#if USE_STRING_BUFFER
        if (sc != StringClass::clean)
        {
            char* str = string_buffer;

            intptr_t len = 0;
            auto const res = json::strings::UnescapeString(first, last, /*allow_invalid_unicode*/ false,
                [&](char ch) {
                    str[len++] = ch;
                }/*,
                [&](char const* p, intptr_t n) {
                    switch (n) {
                    case 0: break;
                    case 1: std::memcpy(str + len, p, 1); break;
                    case 2: std::memcpy(str + len, p, 2); break;
                    case 3: std::memcpy(str + len, p, 3); break;
                    case 4: std::memcpy(str + len, p, 4); break;
                    //case 5: std::memcpy(str + len, p, 5); break;
                    //case 6: std::memcpy(str + len, p, 6); break;
                    //case 7: std::memcpy(str + len, p, 7); break;
                    //case 8: std::memcpy(str + len, p, 8); break;
                    default:
                        std::memcpy(str + len, p, static_cast<size_t>(n));
                        break;
                    }
                    len += n;
                }*/);

            if (res.ec != json::strings::Status::success) {
                return ParseStatus::invalid_string;
            }

            string_buffer += len;

            doc->String(str, static_cast<rapidjson::SizeType>(len), /*copy*/ false);
        }
        else
        {
            const size_t len = static_cast<size_t>(last - first);
            std::memcpy(string_buffer, first, len);

            doc->String(string_buffer, static_cast<rapidjson::SizeType>(len), /*copy*/ false);

            string_buffer += len;
        }
#else // ^^^ USE_STRING_BUFFER ^^^
        if (sc != StringClass::clean)
        {
            const size_t max_len = static_cast<size_t>(last - first);       // allow_invalid_unicode == false
//          const size_t max_len = static_cast<size_t>(last - first) * 6;   // allow_invalid_unicode == true

            char* str = static_cast<char*>(doc->GetAllocator().Malloc(max_len));

            intptr_t len = 0;
            auto const res = json::strings::UnescapeString(first, last, /*allow_invalid_unicode*/ false,
                [&](char ch) {
                    str[len++] = ch;
                },
                [&](char const* p, intptr_t n) {
                    switch (n) {
                    case 0: break;
                    case 1: std::memcpy(str + len, p, 1); break;
                    case 2: std::memcpy(str + len, p, 2); break;
                    case 3: std::memcpy(str + len, p, 3); break;
                    case 4: std::memcpy(str + len, p, 4); break;
                    //case 5: std::memcpy(str + len, p, 5); break;
                    //case 6: std::memcpy(str + len, p, 6); break;
                    //case 7: std::memcpy(str + len, p, 7); break;
                    //case 8: std::memcpy(str + len, p, 8); break;
                    default:
                        std::memcpy(str + len, p, static_cast<size_t>(n));
                        break;
                    }
                    //std::memcpy(str + len, p, static_cast<size_t>(n));
                    len += n;
                });

            if (res.ec != json::strings::Status::success) {
                return ParseStatus::invalid_string;
            }

            doc->String(str, static_cast<rapidjson::SizeType>(len), /*copy*/ false);
        }
        else
        {
            doc->String(first, static_cast<rapidjson::SizeType>(last - first), /*copy*/ true);
        }
#endif // ^^^ ! USE_STRING_BUFFER ^^^
        return {};
    }
};

inline ParseResult ParseRapidjsonDocument(RapidjsonDocument& document, char* string_buffer, char const* next, char const* last)
{
    ParseResult res;

    auto gen = [&](RapidjsonDocument& doc)
    {
        RapidjsonDocumentReader cb(&doc, string_buffer);
        res = ParseSAX(cb, next, last, Mode::strict);
        return res.ec == ParseStatus::success;
    };

    document.Populate(gen);
    return res;
}

struct RapidjsonStatsHandler
{
    jsonstats& stats;

    RapidjsonStatsHandler(jsonstats& s) : stats(s) {}

    bool Null()
    {
        ++stats.null_count;
        return true;
    }

    bool Bool(bool value)
    {
        if (value)
            ++stats.true_count;
        else
            ++stats.false_count;
        return true;
    }

    bool Int(int value)
    {
        ++stats.number_count;
        stats.total_number_value.Add(static_cast<double>(value));
        return true;
    }

    bool Uint(unsigned value)
    {
        ++stats.number_count;
        stats.total_number_value.Add(static_cast<double>(value));
        return true;
    }

    bool Int64(int64_t value)
    {
        ++stats.number_count;
        stats.total_number_value.Add(static_cast<double>(value));
        return true;
    }

    bool Uint64(uint64_t value)
    {
        ++stats.number_count;
        stats.total_number_value.Add(static_cast<double>(value));
        return true;
    }

    bool Double(double value)
    {
        ++stats.number_count;
        stats.total_number_value.Add(value);
        return true;
    }

    bool String(char const* /*ptr*/, size_t length, bool /*copy*/)
    {
        ++stats.string_count;
        stats.total_string_length += length;
        return true;
    }

    bool StartObject()
    {
        ++stats.object_count;
        return true;
    }

    bool Key(char const* /*ptr*/, size_t length, bool /*copy*/)
    {
        ++stats.key_count;
        stats.total_key_length += length;
        return true;
    }

    bool EndObject(size_t memberCount)
    {
        stats.total_object_length += memberCount;
        return true;
    }

    bool StartArray()
    {
        ++stats.array_count;
        return true;
    }

    bool EndArray(size_t elementCount)
    {
        stats.total_array_length += elementCount;
        return true;
    }
};
#endif

} // namespace

bool json1_sax_stats(jsonstats& stats, char const* first, char const* last)
{
    GenStatsCallbacks cb(stats);

    auto const res = ParseSAX(cb, first, last, Mode::strict);
    return res.ec == ParseStatus::success;
}

bool json1_dom_stats(jsonstats& stats, char const* first, char const* last)
{
#if BENCH_RAPIDJSON_DOCUMENT
    RapidjsonDocument doc;

#if USE_STRING_BUFFER
    const size_t doc_len = static_cast<size_t>(last - first);
    const size_t max_len = doc_len;     // allow_invalid_unicode == false
//  const size_t max_len = doc_len * 6; // allow_invalid_unicode == true

#if 1
    char* string_buffer = static_cast<char*>(doc.GetAllocator().Malloc(max_len));
#else
    std::unique_ptr<char, decltype(&::free)> string_buffer_holder(static_cast<char*>(malloc(max_len)), &::free);
    char* string_buffer = string_buffer_holder.get();
#endif
#else
    char* string_buffer = nullptr;
#endif

    auto const res = ParseRapidjsonDocument(doc, string_buffer, first, last);
    if (res.ec != ParseStatus::success) {
        return false;
    }

#if BENCH_COLLECT_STATS
    RapidjsonStatsHandler handler(stats);
    doc.Accept(handler);
#endif

    return true;
#else
    json::Value value;

    auto const res = json::parse(value, first, last);
    if (res.ec != json::ParseStatus::success) {
        return false;
    }

#if BENCH_COLLECT_STATS
    traverse(stats, value);
#endif
    return true;
#endif
}
