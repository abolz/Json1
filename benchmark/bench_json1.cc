#include "bench_json1.h"

#include "../src/json_parser.h"
#include "../src/json_strings.h"
#include "../src/json_numbers.h"

#if BENCH_RAPIDJSON_DOCUMENT
#include "../ext/rapidjson/document.h"
#include "../ext/rapidjson/stringbuffer.h"
#include "../src/json_rapidjson_interop.h"
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

    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc)
    {
        if (nc == NumberClass::invalid)
            return ParseStatus::invalid_number;

        ++stats.number_count;
        stats.total_number_value += json::numbers::StringToNumber(first, last, nc);
        return {};
    }

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
            auto const res = json::strings::UnescapeString(first, last, [&](char) { ++len; }, /*allow_invalid_unicode*/ false);
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

    auto const res = json::ParseSAX(cb, first, last, json::Mode::strict);
    return res.ec == json::ParseStatus::success;
}

bool json1_dom_stats(jsonstats& stats, char const* first, char const* last)
{
#if BENCH_RAPIDJSON_DOCUMENT
    rapidjson::Document doc;

    auto const res = ParseRapidjsonDocument(doc, first, last);
    if (res.ec != json::ParseStatus::success) {
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
