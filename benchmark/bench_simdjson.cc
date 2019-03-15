#include "bench_simdjson.h"

#if defined(__MINGW32__) && defined(_WIN32)

bool simdjson_sax_stats(jsonstats& /*stats*/, char const* /*first*/, char const* /*last*/)
{
    return false;
}

bool simdjson_dom_stats(jsonstats& /*stats*/, char const* /*first*/, char const* /*last*/)
{
    return true;
}

#else

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Woverflow"
#endif

//#ifdef __AVX__
//#error "AVX"
//#endif
//#ifdef __AVX2__
//#error "AVX2"
//#endif

#include "../ext/simdjson/jsonioutil.h"
#include "../ext/simdjson/jsonioutil.cpp"
#include "../ext/simdjson/parsedjson.h"
#include "../ext/simdjson/parsedjson.cpp"
#include "../ext/simdjson/parsedjsoniterator.cpp"
#include "../ext/simdjson/jsonparser.h"
#include "../ext/simdjson/jsonparser.cpp"
#include "../ext/simdjson/simdjson.h"
#include "../ext/simdjson/simdjson.cpp"
#include "../ext/simdjson/stage1_find_marks.cpp"
#include "../ext/simdjson/stage2_build_tape.cpp"

bool simdjson_sax_stats(jsonstats& /*stats*/, char const* /*first*/, char const* /*last*/)
{
    return false;
}

static void GenStats(jsonstats& stats, ParsedJson::iterator& it)
{
    switch (it.get_type())
    {
    case 'n':
        ++stats.null_count;
        return;
    case 't':
        ++stats.true_count;
        return;
    case 'f':
        ++stats.false_count;
        return;
    case 'd':
        ++stats.number_count;
        stats.total_number_value.Add(it.get_double());
        return;
    case 'l':
        ++stats.number_count;
        stats.total_number_value.Add(static_cast<double>(it.get_integer()));
        return;
    case '"':
        ++stats.string_count;
        stats.total_string_length += it.get_string_length();
        return;
    case '[':
        ++stats.array_count;
        if (it.down()) {
            ++stats.total_array_length;
            GenStats(stats, it);
            while (it.next()) {
                ++stats.total_array_length;
                GenStats(stats, it);
            }
            it.up();
        }
        return;
    case '{':
        ++stats.object_count;
        if (it.down()) {
            ++stats.total_object_length;
            ++stats.key_count;
            stats.total_key_length += it.get_string_length();
            it.next();
            GenStats(stats, it);
            while (it.next()) {
                ++stats.total_object_length;
                ++stats.key_count;
                stats.total_key_length += it.get_string_length();
                it.next();
                GenStats(stats, it);
            }
            it.up();
        }
        return;
    }
}

bool simdjson_dom_stats(jsonstats& stats, char const* first, char const* last)
{
    ParsedJson pj = build_parsed_json(first, static_cast<size_t>(last - first)); // do the parsing
    if (!pj.isValid())
        return false;

#if BENCH_COLLECT_STATS
    ParsedJson::iterator it(pj);
    if (!it.isOk())
        return false;
    GenStats(stats, it);
#endif

    return true;
}

#endif
