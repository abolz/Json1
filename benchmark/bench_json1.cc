#include "bench_json1.h"
#include "bench.h"
#include "jsonstats.h"
#include "traverse.h"

#include "../src/json.h"
#include "../src/json_parser.h"
#include "../src/json_strings.h"
#include "../src/json_numbers.h"

#include "../ext/rapidjson/document.h"

using namespace json;

namespace {

struct GenStatsCallbacks
{
    jsonstats& stats;

    GenStatsCallbacks(jsonstats& s) : stats(s) {}

    ParseStatus HandleNull(char const* /*first*/, char const* /*last*/)
    {
        ++stats.null_count;
        return {};
    }

    ParseStatus HandleTrue(char const* /*first*/, char const* /*last*/)
    {
        ++stats.true_count;
        return {};
    }

    ParseStatus HandleFalse(char const* /*first*/, char const* /*last*/)
    {
        ++stats.false_count;
        return {};
    }

    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc)
    {
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

    ParseStatus HandleEndArray(size_t /*count*/)
    {
        //stats.total_array_length += count;
        return {};
    }

    ParseStatus HandleEndElement(size_t& /*count*/)
    {
        return {};
    }

    ParseStatus HandleBeginObject()
    {
        ++stats.object_count;
        return {};
    }

    ParseStatus HandleEndObject(size_t /*count*/)
    {
        //stats.total_object_length += count;
        return {};
    }

    ParseStatus HandleEndMember(size_t& /*count*/)
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
        if (sc == StringClass::needs_cleaning)
        {
            auto const res = json::strings::UnescapeString(first, last, [&](char) { ++len; });
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

struct RapidjsonDocumentCallbacks
{
    static constexpr bool kCopyCleanStrings = true;

    rapidjson::Document* doc;

    RapidjsonDocumentCallbacks(rapidjson::Document* doc_)
        : doc(doc_)
    {
    }

    ParseStatus HandleNull(char const* /*first*/, char const* /*last*/)
    {
        doc->Null();
        return {};
    }

    ParseStatus HandleTrue(char const* /*first*/, char const* /*last*/)
    {
        doc->Bool(true);
        return {};
    }

    ParseStatus HandleFalse(char const* /*first*/, char const* /*last*/)
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

            auto yield = [&](char ch) { str.push_back(ch); };
            auto const res = json::strings::UnescapeString(first, last, yield);
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

            auto yield = [&](char ch) { str.push_back(ch); };
            auto const res = json::strings::UnescapeString(first, last, yield);
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

inline json::ParseResult ParseRapidjsonDocument(rapidjson::Document& document, char const* next, char const* last, json::Options const& options = {})
{
    json::ParseResult res;

    auto gen = [&](rapidjson::Document& doc)
    {
        RapidjsonDocumentCallbacks cb(&doc);
        res = json::ParseSAX(cb, next, last, options);
        return res.ec == json::ParseStatus::success;
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

    bool EndObject(size_t /*memberCount*/)
    {
        //stats.total_object_length += memberCount;
        return true;
    }

    bool StartArray()
    {
        ++stats.array_count;
        return true;
    }

    bool EndArray(size_t /*elementCount*/)
    {
        //stats.total_array_length += elementCount;
        return true;
    }
};

} // namespace

bool json1_sax_stats(jsonstats& stats, char const* first, char const* last)
{
    GenStatsCallbacks cb(stats);

    auto const res = json::ParseSAX(cb, first, last);
    return res.ec == json::ParseStatus::success;
}

bool json1_dom_stats(jsonstats& stats, char const* first, char const* last)
{
#if BENCH_USE_RAPIDJSON_DOCUMENT
    rapidjson::Document doc;

    auto const res = ParseRapidjsonDocument(doc, first, last);
    if (res.ec != json::ParseStatus::success) {
        return false;
    }

    RapidjsonStatsHandler handler(stats);
    doc.Accept(handler);

    return true;
#else
    json::Value value;

    auto const res = json::parse(value, first, last);
    if (res.ec != json::ParseStatus::success) {
        return false;
    }

    traverse(stats, value);
    return true;
#endif
}
