#include "bench_json1.h"

#include "../src/json.h"
#include "../src/json_strings.h"
#include "../src/json_numbers.h"

#include "traverse.h"

using namespace json;

namespace {

struct SaxHandler : public ParseCallbacks
{
    jsonstats& stats;

    SaxHandler(jsonstats& s) : stats(s) {}

    ParseStatus HandleNull(Options const& /*options*/) override
    {
        ++stats.null_count;
        return {};
    }

    ParseStatus HandleBoolean(bool value, Options const& /*options*/) override
    {
        if (value)
            ++stats.true_count;
        else
            ++stats.false_count;
        return {};
    }

    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc, Options const& /*options*/) override
    {
        ++stats.number_count;
        stats.total_number_value += json::numbers::StringToNumber(first, last, nc);
        return {};
    }

    ParseStatus HandleString(char const* first, char const* last, bool needs_cleaning, Options const& /*options*/) override
    {
        ++stats.string_count;
        size_t len = 0;
        if (needs_cleaning)
        {
            auto const res = json::strings::UnescapeString(first, last, [&](char) { ++len; });

            if (res.status != json::strings::UnescapeStringStatus::success)
                return ParseStatus::invalid_string;
        }
        else
        {
            len = static_cast<size_t>(last - first);
        }
        stats.total_string_length += len;
        return {};
    }

    ParseStatus HandleBeginArray(Options const& /*options*/) override
    {
        ++stats.array_count;
        return {};
    }

    ParseStatus HandleEndArray(size_t /*count*/, Options const& /*options*/) override
    {
        //stats.total_array_length += count;
        return {};
    }

    ParseStatus HandleEndElement(size_t& /*count*/, Options const& /*options*/) override
    {
        return {};
    }

    ParseStatus HandleBeginObject(Options const& /*options*/) override
    {
        ++stats.object_count;
        return {};
    }

    ParseStatus HandleEndObject(size_t /*count*/, Options const& /*options*/) override
    {
        //stats.total_object_length += count;
        return {};
    }

    ParseStatus HandleEndMember(size_t& /*count*/, Options const& /*options*/) override
    {
        return {};
    }

    ParseStatus HandleKey(char const* first, char const* last, bool needs_cleaning, Options const& /*options*/) override
    {
        ++stats.key_count;
        size_t len = 0;
        if (needs_cleaning)
        {
            auto const res = json::strings::UnescapeString(first, last, [&](char) { ++len; });

            if (res.status != json::strings::UnescapeStringStatus::success)
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

    auto const res = json::parse(handler, first, last);
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
