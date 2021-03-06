#pragma once

#define BENCH_RAPIDJSON_DOCUMENT 1
#define BENCH_COLLECT_STATS 0
#define BENCH_FULLPREC 0

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>

class FloatSum
{
    // Jonathan Richard Shewchuk,
    //  "Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates",
    //      Discrete & Computational Geometry 18:305-363, 1997
    //
    // https://www.cs.cmu.edu/~quake/robust.html
    // https://www.cs.cmu.edu/afs/cs/project/quake/public/code/predicates.c

    //----------------------------------------------------------------------------------------------
    // WARNING:
    //
    // This code will not work correctly on a processor that uses extended precision internal
    // floating-point registers, such as the Intel 80486 and Pentium, unless the processor is
    // configured to round internally to double precision.
    //----------------------------------------------------------------------------------------------

    // Max length is 11 for the minified examples
    static constexpr int MaxExpansionLength = 16;

    double sum = 0.0;
    double expansion[MaxExpansionLength];
    int expansion_length = 0;

    static void TwoSum(double x, double y, double& hi, double& lo)
    {
        hi = x + y;
        const double y_virt = hi - x;
        const double x_virt = hi - y_virt;
        const double y_round = y - y_virt;
        const double x_round = x - x_virt;
        lo = x_round + y_round;
    }

public:
    void Assign(double x)
    {
        expansion[0] = x;
        expansion_length = 1;
    }

    void Add(double x)
    {
        // expansion += x
        // Add a scalar to an expansion, eliminating zero components from the output expansion.

        int i = 0;

        for (int j = 0; j < expansion_length; ++j)
        {
            const double y = expansion[j];

            double hi;
            double lo;
            TwoSum(x, y, hi, lo);
            if (lo != 0.0)
            {
                expansion[i++] = lo;
            }

            x = hi;
        }

        if (x != 0.0 /*|| i == 0*/)
        {
            if (i >= MaxExpansionLength) {
                fprintf(stderr, "MaxExpansionLength too small\n");
                abort();
            }

            expansion[i++] = x;
        }

        expansion_length = i;
    }

    double Finish()
    {
        sum = 0.0;
        for (int j = 0; j < expansion_length; ++j) {
            sum += expansion[j];
        }
        return sum;
    }

    double Result() const
    {
//      Finish();
        return sum;
    }
};

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
    FloatSum total_number_value;

    void print(FILE* file) const {
        fprintf(file, "    # null     = %10zu\n", null_count);
        fprintf(file, "    # false    = %10zu\n", false_count);
        fprintf(file, "    # true     = %10zu\n", true_count);
        //fprintf(file, "    # keywords = %10zu (null %zu, false %zu, true %zu)\n", null_count + false_count + true_count, null_count, false_count, true_count);
        fprintf(file, "    # numbers  = %10zu --- total_number_value  = %.17g\n", number_count, total_number_value.Result());
        fprintf(file, "    # strings  = %10zu --- total_string_length = %zu\n", string_count, total_string_length);
        fprintf(file, "    # arrays   = %10zu --- total_array_length  = %zu\n", array_count, total_array_length);
        fprintf(file, "    # objects  = %10zu --- total_object_length = %zu\n", object_count, total_object_length);
        fprintf(file, "    # keys     = %10zu --- total_key_length    = %zu\n", key_count, total_key_length);
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
            && total_array_length == rhs.total_array_length
            && total_object_length == rhs.total_object_length
            && total_key_length == rhs.total_key_length
            && total_number_value.Result() == rhs.total_number_value.Result()
            ;
    }

    bool operator!=(const jsonstats& rhs) const {
        return !(*this == rhs);
    }
};
