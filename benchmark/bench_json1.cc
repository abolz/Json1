#include "bench_json1.h"

#include <memory>

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

#if JSON_PARSER_CONVERT_NUMBERS_FAST
    ParseStatus HandleNumber(double value, NumberClass nc)
    {
        if (nc == NumberClass::invalid)
            return ParseStatus::invalid_number;

        ++stats.number_count;
        stats.total_number_value += value;
        return {};
    }
#else
    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc)
    {
        if (nc == NumberClass::invalid)
            return ParseStatus::invalid_number;

        ++stats.number_count;
        stats.total_number_value += json::numbers::StringToNumber(first, last, nc);
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
    static constexpr bool kCopyCleanStrings = true;

    RapidjsonDocument* doc;
    char* string_buffer;

    RapidjsonDocumentReader(RapidjsonDocument* doc_, char* string_buffer_)
        : doc(doc_)
        , string_buffer(string_buffer_)
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

#if JSON_PARSER_CONVERT_NUMBERS_FAST
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
        if (sc != StringClass::clean)
        {
            char* str = string_buffer; // static_cast<char*>(doc->GetAllocator().Malloc(static_cast<size_t>(last - first)));

            intptr_t len = 0;
            auto const res = json::strings::UnescapeString(first, last, /*allow_invalid_unicode*/ false,
                [&](char ch) { str[len++] =ch; },
                [&](char const* p, intptr_t n) { std::memcpy(str + len, p, static_cast<size_t>(n)); len += n; });

            if (res.ec != json::strings::Status::success)
                return ParseStatus::invalid_string;

            string_buffer += len;

            doc->String(str, static_cast<rapidjson::SizeType>(len), /*copy*/ false);
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
        if (sc != StringClass::clean)
        {
            char* str = string_buffer; // static_cast<char*>(doc->GetAllocator().Malloc(static_cast<size_t>(last - first)));

            intptr_t len = 0;
            auto const res = json::strings::UnescapeString(first, last, /*allow_invalid_unicode*/ false,
                [&](char ch) { str[len++] =ch; },
                [&](char const* p, intptr_t n) { std::memcpy(str + len, p, static_cast<size_t>(n)); len += n; });

            if (res.ec != json::strings::Status::success) {
                return ParseStatus::invalid_string;
            }

            string_buffer += len;

            doc->String(str, static_cast<rapidjson::SizeType>(len), /*copy*/ false);
        }
        else
        {
            doc->String(first, static_cast<rapidjson::SizeType>(last - first), /*copy*/ kCopyCleanStrings);
        }
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
        stats.total_number_value += static_cast<double>(value);
        return true;
    }

    bool Uint(unsigned value)
    {
        ++stats.number_count;
        stats.total_number_value += static_cast<double>(value);
        return true;
    }

    bool Int64(int64_t value)
    {
        ++stats.number_count;
        stats.total_number_value += static_cast<double>(value);
        return true;
    }

    bool Uint64(uint64_t value)
    {
        ++stats.number_count;
        stats.total_number_value += static_cast<double>(value);
        return true;
    }

    bool Double(double value)
    {
        ++stats.number_count;
        stats.total_number_value += value;
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

struct FreeDeleter {
    void operator()(void* p) { free(p); }
};

bool json1_dom_stats(jsonstats& stats, char const* first, char const* last)
{
#if BENCH_RAPIDJSON_DOCUMENT
    size_t const doc_len = static_cast<size_t>(last - first);
    std::unique_ptr<char, FreeDeleter> string_buffer( (char*)malloc(doc_len), FreeDeleter{} );

    RapidjsonDocument doc;

    auto const res = ParseRapidjsonDocument(doc, string_buffer.get(), first, last);
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
