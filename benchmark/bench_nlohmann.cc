#include "bench_nlohmann.h"
#include "jsonstats.h"
#include "traverse.h"

#include "../ext/nlohmann/json.hpp"

#define USE_RAPIDJSON_DOCUMENT 1

#if USE_RAPIDJSON_DOCUMENT
#include "../ext/rapidjson/document.h"
#include "../ext/rapidjson/stringbuffer.h"
#endif

namespace {

class SaxHandler //: public nlohmann::json_sax<nlohmann::json>
{
    jsonstats& stats;
    bool errored;

public:
    using BasicJsonType = nlohmann::json;
    using number_integer_t = BasicJsonType::number_integer_t;
    using number_unsigned_t = BasicJsonType::number_unsigned_t;
    using number_float_t = BasicJsonType::number_float_t;
    using string_t = BasicJsonType::string_t;

    SaxHandler(jsonstats& s)
        : stats(s)
        , errored(false)
    {
    }

    bool null() /*override*/
    {
        ++stats.null_count;
        return true;
    }

    bool boolean(bool val) /*override*/
    {
        if (val)
            ++stats.true_count;
        else
            ++stats.false_count;
        return true;
    }

    bool number_integer(number_integer_t val) /*override*/
    {
        ++stats.number_count;
        stats.total_number_value += static_cast<double>(val);
        return true;
    }

    bool number_unsigned(number_unsigned_t val) /*override*/
    {
        ++stats.number_count;
        stats.total_number_value += static_cast<double>(val);
        return true;
    }

    bool number_float(number_float_t val, const string_t&) /*override*/
    {
        ++stats.number_count;
        stats.total_number_value += val;
        return true;
    }

    bool string(string_t& val) /*override*/
    {
        ++stats.string_count;
        stats.total_string_length += val.size();
        return true;
    }

    bool start_object(std::size_t /*len*/) /*override*/
    {
        ++stats.object_count;
        return true;
    }

    bool key(string_t& val) /*override*/
    {
        ++stats.key_count;
        stats.total_key_length += val.size();
        return true;
    }

    bool end_object(std::size_t count) /*override*/
    {
        stats.total_object_length += count;
        return true;
    }

    bool start_array(std::size_t /*len*/) /*override*/
    {
        ++stats.array_count;
        return true;
    }

    bool end_array(std::size_t count) /*override*/
    {
        stats.total_array_length += count;
        return true;
    }

    bool parse_error(std::size_t, const std::string&, const nlohmann::detail::exception& /*ex*/) /*override*/
    {
        errored = true;
        return false;
    }

    constexpr bool is_errored() const
    {
        return errored;
    }
};

} // namespace

bool nlohmann_sax_stats(jsonstats& stats, char const* next, char const* last)
{
    SaxHandler handler(stats);

    nlohmann::json::sax_parse(next, last, &handler);

    return !handler.is_errored();
}

//bool nlohmann_dom_stats(jsonstats& stats, char const* next, char const* last)
//{
//    json::Value value;
//    DomHandler handler(value);
//
//    nlohmann::json::sax_parse(next, last, &handler);
//
//    if (handler.is_errored())
//        return false;
//
//    traverse(stats, value);
//    return true;
//}
