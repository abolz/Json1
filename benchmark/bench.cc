// Modified sajson benchmark
// Copyright (c) 2012-2017 Chad Austin
// https://github.com/chadaustin/sajson

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cfloat>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <time.h>
#include <cmath>
#include <chrono>
#include <memory>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <windows.h>
#include <intrin.h>
#endif

#include "bench_json1.h"
#include "bench_nlohmann.h"
#include "bench_rapidjson.h"
#include "bench_simdjson.h"
#include "bench_sajson.h"

#define REVERSE_ORDER 1

struct TestFile {
    const char* name;
    size_t length;
    const unsigned char* data;
};

namespace json1_sax_test {
    void test(jsonstats& stats, const TestFile& file) {
        if (!json1_sax_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
            fprintf(stderr, "json1 sax parse error\n");
            abort();
        }
    }
}

namespace json1_dom_test {
    void test(jsonstats& stats, const TestFile& file) {
        if (!json1_dom_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
            fprintf(stderr, "json1 dom parse error\n");
            abort();
        }
    }
}

namespace rapidjson_sax_test {
    void test(jsonstats& stats, const TestFile& file) {
        if (!rapidjson_sax_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
            fprintf(stderr, "rapidjson sax parse error\n");
            abort();
        }
    }
}

namespace rapidjson_dom_test {
    void test(jsonstats& stats, const TestFile& file) {
        if (!rapidjson_dom_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
            fprintf(stderr, "rapidjson dom parse error\n");
            abort();
        }
    }
}

namespace nlohmann_sax_test {
    void test(jsonstats& stats, const TestFile& file) {
        if (!nlohmann_sax_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
            fprintf(stderr, "nlohmann sax parse error\n");
            abort();
        }
    }
}

namespace nlohmann_dom_test {
    void test(jsonstats& stats, const TestFile& file) {
        if (!nlohmann_dom_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
            fprintf(stderr, "nlohmann dom parse error\n");
            abort();
        }
    }
}

//namespace simdjson_sax_test {
//    void test(jsonstats& stats, const TestFile& file) {
//    }
//}

namespace simdjson_dom_test {
    void test(jsonstats& stats, const TestFile& file) {
        if (!simdjson_dom_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
            fprintf(stderr, "simdjson dom parse error\n");
            abort();
        }
    }
}

//namespace sajson_sax_test {
//    void test(jsonstats& stats, const TestFile& file) {
//    }
//}

namespace sajson_dom_test {
    void test(jsonstats& stats, const TestFile& file) {
        if (!sajson_dom_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
            fprintf(stderr, "sajson dom parse error\n");
            abort();
        }
    }
}

struct TestImplementation {
    typedef void (*TestFunction)(jsonstats&, const TestFile&);

    const char* name;
    TestFunction func;
};

const auto SECONDS_PER_WARMUP = std::chrono::seconds(2);
const auto SECONDS_PER_TEST = std::chrono::seconds(2);
//const auto SECONDS_PER_WARMUP = std::chrono::seconds(0);
//const auto SECONDS_PER_TEST = std::chrono::seconds(0);

struct TestResult {
    std::string name;
    int64_t nanoseconds_min;
    int64_t nanoseconds_avg;
    int64_t cycles_min;
    int64_t cycles_avg;
};

TestImplementation test_implementations[] = {
    { "Json1", &json1_dom_test::test },
#if !defined(__MINGW32__) || !defined(_WIN32)
    { "simdjson", &simdjson_dom_test::test },
#endif
    { "sajson", &sajson_dom_test::test },
    { "RapidJSON", &rapidjson_dom_test::test },
    //{ "nlohmann", &nlohmann_dom_test::test },
};

const char* benchmark_files[] = {
#if 0
    "test_data/examples/apache_builds.json",
    "test_data/examples/canada.json",
    "test_data/examples/citm_catalog.json",
    "test_data/examples/github_events.json",
    "test_data/examples/gsoc-2018.json",
    "test_data/examples/instruments.json",
    "test_data/examples/marine_ik.json",
    "test_data/examples/mesh.json",
    "test_data/examples/mesh.pretty.json",
    "test_data/examples/numbers.json",
    "test_data/examples/random.json",
    "test_data/examples/twitter.json",
    "test_data/examples/twitterescaped.json",
    "test_data/examples/update-center.json",
#endif
#if 1
    "test_data/examples/minified/apache_builds.json",
    "test_data/examples/minified/canada.json",
    "test_data/examples/minified/citm_catalog.json",
    "test_data/examples/minified/github_events.json",
    "test_data/examples/minified/gsoc-2018.json",
    "test_data/examples/minified/instruments.json",
    "test_data/examples/minified/marine_ik.json",
    "test_data/examples/minified/mesh.json",
    "test_data/examples/minified/numbers.json",
    "test_data/examples/minified/random.json",
    "test_data/examples/minified/twitter.json",
    "test_data/examples/minified/update-center.json",
#endif
};

template<typename T, size_t L>
constexpr size_t array_length(T(&)[L]) {
    return L;
}

using Clock = std::chrono::steady_clock;

//struct Timing
//{
//    int n = 0;
//    double mean = 0.0;
//    double var2 = 0.0;
//    double minx = 0.0;
//    double maxx = 0.0;
//};
//
//static inline void Update(Timing& t, double nanoseconds)
//{
//    if (t.n <= 0)
//    {
//        t.n = 1;
//        t.mean = nanoseconds;
//        t.var2 = 0;
//        t.minx = nanoseconds;
//        t.maxx = nanoseconds;
//        return;
//    }
//
//    const int n = t.n + 1;
//
//    // x_n - mu_{n-1}
//    const double diff = nanoseconds - t.mean;
//
//    // mu_n = mu_{n-1} + (x_n - mu_{n-1}) / n
//    const double mean = t.mean + diff / n;
//
//    // S_n = S_{n-1} + (x_n - mu_{n-1}) * (x_n - mu_n)
//    const double var2 = t.var2 + diff * (nanoseconds - mean);
//
//    t.n = n;
//    t.mean = mean;
//    t.var2 = var2;
//
//    if (t.minx > nanoseconds)
//        t.minx = nanoseconds;
//    if (t.maxx < nanoseconds)
//        t.maxx = nanoseconds;
//}
//
//static inline double Mean(Timing const& t)
//{
//    return t.mean;
//}
//
//static inline double Var2(Timing const& t)
//{
//    return t.var2 / (t.n - 1);
////  return t.var2 / t.n;
//}
//
//static inline double StdDev(Timing const& t)
//{
//    return std::sqrt(Var2(t));
//}

// the input buf should be readable up to buf + SIMDJSON_PADDING
static constexpr size_t SIMDJSON_PADDING = 64; // (256 / 8);

// PRE: align == 2^n
static inline void* mem_aligned_alloc(size_t num_bytes, size_t align)
{
    return ::_mm_malloc(num_bytes, align);
}

static inline void mem_aligned_free(void* ptr)
{
    assert(ptr != nullptr);
    ::_mm_free(ptr);
}

struct AlignedDeleter {
    void operator()(void* ptr) const { mem_aligned_free(ptr); }
};

using AlignedPointer = std::unique_ptr<char, AlignedDeleter>;

//static inline void Serialize() {
//    int t[4];
//    __cpuid(t, 0);
//    //volatile int dont_skip = t[0]; // Prevent the compiler from optimizing away the whole Serialize function:
//}
static inline uint64_t ReadTSC() {
    return __rdtsc();
}

void benchmark(const char* filename) {
    FILE* fh = fopen(filename, "rb");
    if (!fh) {
        fprintf(stderr, "failed to open file: %s\n", filename);
        exit(1);
    }
    fseek(fh, 0, SEEK_END);
    auto const slength = ftell(fh);
    assert(slength >= 0);
    const size_t length = static_cast<size_t>(slength);

    printf("File: %s\n", filename);
    printf("Size: %llu\n", (uint64_t)length);
    //printf("Size: %llu B = %.2f KB = %.2f MB\n", (uint64_t)length, length / 1024.0, length / (1024.0 * 1024.0));

    AlignedPointer contents(reinterpret_cast<char*>(mem_aligned_alloc(length + SIMDJSON_PADDING, 64)));

    fseek(fh, 0, SEEK_SET);
    if (length != fread(contents.get(), 1, length, fh)) {
        fprintf(stderr, "Failed to read file\n");
        abort();
    }
    fclose(fh);

    std::memset(contents.get() + length, 0, SIMDJSON_PADDING);
    //contents.get()[length] = '\0';

    TestFile file = { filename, length, reinterpret_cast<unsigned char*>(contents.get()) };

    constexpr size_t NumTests = array_length(test_implementations);
    TestResult results[NumTests];

    fprintf(stderr, "Benchmarking...\n");

    jsonstats expected_stats;
    double reference_ms = 0;
    bool first = true;
    for (size_t i = 0; i < NumTests; ++i) {
        auto& implementation = test_implementations[i];
        //fprintf(stderr, "Benchmarking %s...\n", implementation.name);

        jsonstats this_stats;
        if (first) {
            implementation.func(expected_stats, file);
            expected_stats.total_number_value.Finish();
            //expected_stats.print(stderr);
        } else {
            implementation.func(this_stats, file);
            this_stats.total_number_value.Finish();
            //this_stats.print(stderr);
            if (this_stats != expected_stats) {
                fprintf(stderr, "Test: %s\n", implementation.name);
                fprintf(stderr, "> Parse results did not match.\n");
                fprintf(stderr, "> Expected:\n");
                expected_stats.print(stderr);
                fprintf(stderr, "> Actual:\n");
                this_stats.print(stderr);
                fprintf(stderr, "> Parse results did not match.\n");
            }
        }
        first = false;

        bool warmup = SECONDS_PER_WARMUP != std::chrono::seconds{0};
        //Timing timing;
        int parses = 0;

        int64_t min_nanoseconds = INT64_MAX;
        int64_t min_cycles = INT64_MAX;
        int64_t avg_nanoseconds = 0;
        int64_t avg_cycles = 0;

        auto test_start = Clock::now();
        for (;;) {
            //Serialize();
            const auto start = Clock::now();
            const auto cy_start = ReadTSC();

            implementation.func(this_stats, file);

            //Serialize();
            const auto cy_end = ReadTSC();
            const auto end = Clock::now();

            if (warmup) {
                if (end - test_start > SECONDS_PER_WARMUP) {
                    warmup = false;
                    test_start = end;
                }
            } else {
                ++parses;
                avg_nanoseconds += (end - start).count();
                avg_cycles += cy_end - cy_start;
                //Update(timing, static_cast<double>((end - start).count()));
                const int64_t ns = (end - start).count();
                if (min_nanoseconds > ns)
                    min_nanoseconds = ns;
                const auto cy = (cy_end - cy_start);
                if (min_cycles > cy)
                    min_cycles = cy;
                if (end - test_start > SECONDS_PER_TEST)
                    break;
            }
        }

        avg_nanoseconds /= parses;
        avg_cycles /= parses;

        results[i].name = implementation.name;
        results[i].nanoseconds_min = min_nanoseconds;
        results[i].nanoseconds_avg = avg_nanoseconds;
        results[i].cycles_min = min_cycles;
        results[i].cycles_avg = avg_cycles;
        /*
        //auto const time_ns = timing.minx;
        //auto const time_ns = Mean(timing);
        auto const time_ns = min_nanoseconds;
        auto const time_ms = time_ns / 1e6;

        const double GBPerSec = (length / ( (1024.0 * 1024.0 * 1024.0) / 1e9 )) / time_ns;

        //const double Frequency = 2.0; // GHz
        //const double CPB = Frequency * time_ns / length;
        const double CPB = (double)min_cycles / length;

        if (first) {
            reference_ms = time_ms;
        }

        printf("Test: %-20s %8.2f ms --- %8.6f GB/sec --- %5.1f cycles/byte", implementation.name, time_ms, GBPerSec, CPB);
        if (first) {
            printf("\n");
        } else {
            printf(" --- x %.2f = 1 / %.2f\n", reference_ms / time_ms, time_ms / reference_ms);
        }
        fflush(stdout);

        first = false;
        //reference = mean;
        */
    }

    //std::sort(std::begin(results), std::end(results), [](TestResult const& lhs, TestResult const& rhs) {
    //    return lhs.nanoseconds_min < rhs.nanoseconds_min;
    //});
    //size_t const fastest_i = 0;
    auto const fastest_it = std::min_element(std::begin(results), std::end(results), [](TestResult const& lhs, TestResult const& rhs) {
        return lhs.nanoseconds_min < rhs.nanoseconds_min;
    });
    size_t const fastest_i = fastest_it - std::begin(results);

    int64_t nanoseconds_ref = results[0].nanoseconds_min;
    for (size_t i = 0; i < NumTests; ++i)
    {
        const auto& r = results[i];
        const double ms = r.nanoseconds_min / 1e6;
        const double gbps = length / (1024.0 * 1024.0 * 1024.0 / 1e9) / r.nanoseconds_min;
        const double cpb = (double)r.cycles_min / length;
        fprintf(stderr, " %s %-20s %8.2f ms --- %8.6f GB/sec --- %6.2f cycles/byte", i == fastest_i ? "*" : " ", r.name.c_str(), ms, gbps, cpb);
        if (i == 0) {
            fprintf(stderr, "\n");
        } else {
            fprintf(stderr, " --- x %.2f = 1 / %.2f\n", (double)nanoseconds_ref / r.nanoseconds_min, (double)r.nanoseconds_min / nanoseconds_ref);
        }
    }
}

int main(int argc, const char** argv)
{
#ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 1);
#endif

    if (argc <= 1) {
        for (size_t i = 0; i < array_length(benchmark_files); ++i) {
            fprintf(stderr, "---\n");
            auto& filename = benchmark_files[i];
            benchmark(filename);
        }
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        benchmark(argv[i]);
    }
}
