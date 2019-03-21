#include "bench_simdjson.h"

//#if defined(__GNUC__)
//#pragma GCC diagnostic ignored "-Wsign-conversion"
//#pragma GCC diagnostic ignored "-Wold-style-cast"
//#pragma GCC diagnostic ignored "-Wpedantic"
//#pragma GCC diagnostic ignored "-Wconversion"
//#pragma GCC diagnostic ignored "-Woverflow"
//#endif

#define SAJSON_UNSORTED_OBJECT_KEYS 1
#include "../ext/sajson/sajson.h"

static void GenStats(jsonstats& stat, const sajson::value& v)
{
    using namespace sajson;

    switch (v.get_type())
    {
    case TYPE_ARRAY:
        {
            size_t size = v.get_length();
            for (size_t i = 0; i < size; i++)
                GenStats(stat, v.get_array_element(i));
            stat.array_count++;
            stat.total_array_length += size;
        }
        break;

    case TYPE_OBJECT:
        {
            size_t size = v.get_length();
            for (size_t i = 0; i < size; i++) {
                GenStats(stat, v.get_object_value(i));
                stat.total_key_length += v.get_object_key(i).length();
            }
            stat.object_count++;
            stat.total_object_length += size;
            stat.key_count += size;
        }
        break;

    case TYPE_STRING:
        stat.string_count++;
        stat.total_string_length += v.get_string_length();
        break;

    case TYPE_INTEGER:
        stat.number_count++;
        stat.total_number_value.Add(v.get_integer_value());
        break;

    case TYPE_DOUBLE:
        stat.number_count++;
        stat.total_number_value.Add(v.get_double_value());
        break;

    case TYPE_TRUE:
        stat.true_count++;
        break;

    case TYPE_FALSE:
        stat.false_count++;
        break;

    case TYPE_NULL:
        stat.null_count++;
        break;
    }
}

bool sajson_sax_stats(jsonstats& /*stats*/, char const* /*first*/, char const* /*last*/)
{
    return false;
}

bool sajson_dom_stats(jsonstats& stats, char const* first, char const* last)
{
    const auto doc = sajson::parse(sajson::single_allocation(), sajson::string(first, static_cast<size_t>(last - first)));
    if (!doc.is_valid())
        return false;

#if BENCH_COLLECT_STATS
    GenStats(stats, doc.get_root());
#endif
    return true;
}
