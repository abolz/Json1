#include "bench_json1.h"

#include "../src/json.h"
#include "../src/json_strings.h"
#include "../src/json_numbers.h"

#include "traverse.h"

using namespace json;

namespace {

struct SaxHandler
{
    jsonstats& stats;

    SaxHandler(jsonstats& s) : stats(s) {}

    ParseStatus HandleNull(char const* /*first*/, char const* /*last*/, Options const& /*options*/)
    {
        ++stats.null_count;
        return {};
    }

    ParseStatus HandleTrue(char const* /*first*/, char const* /*last*/, Options const& /*options*/)
    {
        ++stats.true_count;
        return {};
    }

    ParseStatus HandleFalse(char const* /*first*/, char const* /*last*/, Options const& /*options*/)
    {
        ++stats.false_count;
        return {};
    }

    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc, Options const& /*options*/)
    {
        ++stats.number_count;
        stats.total_number_value += json::numbers::StringToNumber(first, last, nc);
        return {};
    }

    ParseStatus HandleString(char const* first, char const* last, StringClass sc, Options const& /*options*/)
    {
        ++stats.string_count;
        intptr_t len = 0;
        if (sc == StringClass::needs_cleaning)
        {
            auto const res = json::strings::UnescapeString(first, last, [&](char) { ++len; });
            if (res.status != json::strings::Status::success)
                return ParseStatus::invalid_string;
        }
        else
        {
            len = last - first;
        }
        stats.total_string_length += static_cast<size_t>(len);
        return {};
    }

    ParseStatus HandleBeginArray(Options const& /*options*/)
    {
        ++stats.array_count;
        return {};
    }

    ParseStatus HandleEndArray(size_t /*count*/, Options const& /*options*/)
    {
        //stats.total_array_length += count;
        return {};
    }

    ParseStatus HandleEndElement(size_t& /*count*/, Options const& /*options*/)
    {
        return {};
    }

    ParseStatus HandleBeginObject(Options const& /*options*/)
    {
        ++stats.object_count;
        return {};
    }

    ParseStatus HandleEndObject(size_t /*count*/, Options const& /*options*/)
    {
        //stats.total_object_length += count;
        return {};
    }

    ParseStatus HandleEndMember(size_t& /*count*/, Options const& /*options*/)
    {
        return {};
    }

    ParseStatus HandleKey(char const* first, char const* last, StringClass sc, Options const& /*options*/)
    {
        ++stats.key_count;
        size_t len = 0;
        if (sc == StringClass::needs_cleaning)
        {
            auto const res = json::strings::UnescapeString(first, last, [&](char) { ++len; });

            if (res.status != json::strings::Status::success)
                return ParseStatus::invalid_string;
        }
        else
        {
            len = static_cast<size_t>(last - first);
        }
        stats.total_key_length += len;
        return {};
    }
};

} // namespace

bool json1_sax_stats(jsonstats& stats, char const* first, char const* last)
{
    SaxHandler handler(stats);

    auto const res = json::ParseSAX(handler, first, last);
    return res.ec == json::ParseStatus::success;
}

bool json1_dom_stats(jsonstats& stats, char const* first, char const* last)
{
    json::Value value;
    auto const res = json::parse(value, first, last);

    if (res.ec != json::ParseStatus::success)
        return false;

    traverse(stats, value);
    return true;
}
