#pragma once

#include "bench.h"
#include "../src/json.h"

inline void traverse(jsonstats& stats, const json::Value& v)
{
    switch (v.type()) {
    case json::Type::undefined:
        assert(false);
        break;
    case json::Type::null:
        ++stats.null_count;
        break;
    case json::Type::boolean:
        if (v.get_boolean())
            ++stats.true_count;
        else
            ++stats.false_count;
        break;
    case json::Type::number:
        ++stats.number_count;
        stats.total_number_value += v.get_number();
        break;
    case json::Type::string:
        ++stats.string_count;
        stats.total_string_length += v.size();
        break;
    case json::Type::array:
        ++stats.array_count;
        stats.total_array_length += v.size();
        for (auto const& e : v.get_array()) {
            traverse(stats, e);
        }
        break;
    case json::Type::object:
        ++stats.object_count;
        stats.total_object_length += v.size();
        for (auto const& e : v.get_object()) {
            ++stats.key_count;
            stats.total_key_length += e.first.size();
            traverse(stats, e.second);
        }
        break;
    }
}
