#pragma once

#define BENCH_RAPIDJSON_DOCUMENT 1
#define BENCH_COLLECT_STATS 0

#include <cstddef>
#include <cstdio>
#include <cmath>

struct jsonstats {
    jsonstats()
        : null_count(0)
        , false_count(0)
        , true_count(0)
        , number_count(0)
        , object_count(0)
        , key_count(0)
        , array_count(0)
        , string_count(0)
        , total_string_length(0)
        , total_array_length(0)
        , total_object_length(0)
        , total_key_length(0)
        , total_number_value(0)
    {}

    size_t null_count;
    size_t false_count;
    size_t true_count;
    size_t number_count;
    size_t object_count;
    size_t key_count;
    size_t array_count;
    size_t string_count;

    size_t total_string_length;
    size_t total_array_length;
    size_t total_object_length;
    size_t total_key_length;
    double total_number_value;

    void print(FILE* file) const {
        fprintf(
            file,
            "    # null=%zu, false=%zu, true=%zu, numbers=%zu, strings=%zu, arrays=%zu, objects=%zu, keys=%zu\n",
            null_count, false_count, true_count,
            number_count, string_count,
            array_count, object_count, key_count);
        fprintf(
            file,
            "    # total_string_length=%zu, total_array_length=%zu, total_object_length=%zu, total_key_length=%zu, total_number_value=%f\n",
            total_string_length,
            total_array_length,
            total_object_length,
            total_key_length,
            total_number_value);
    }

    bool operator==(const jsonstats& rhs) const {
        return null_count == rhs.null_count
            && false_count == rhs.false_count
            && true_count == rhs.true_count
            && number_count == rhs.number_count
            && object_count == rhs.object_count
            && key_count == rhs.key_count
            && array_count == rhs.array_count
            && string_count == rhs.string_count
            && total_string_length == rhs.total_string_length
            //&& total_array_length == rhs.total_array_length
            //&& total_object_length == rhs.total_object_length
            && total_key_length == rhs.total_key_length
            // TODO: figure this out
            // && fabs(total_number_value - rhs.total_number_value) <= 0.01
            && std::round(total_number_value) == std::round(rhs.total_number_value)
            ;
    }

    bool operator!=(const jsonstats& rhs) const {
        return !(*this == rhs);
    }
};
