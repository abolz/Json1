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
const auto SECONDS_PER_TEST = std::chrono::seconds(6);

TestImplementation test_implementations[] = {
    { "Json1", &json1_dom_test::test },
    { "simdjson", &simdjson_dom_test::test },
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
size_t array_length(T(&)[L]) {
    return L;
}

using Clock = std::chrono::steady_clock;

#if 0
struct Timing
{
    int n = 0;
    double mean = 0.0;
    double var2 = 0.0;
};

static inline void Update(Timing& t, double x)
{
    const int n = t.n + 1;

    // x_n - mu_{n-1}
    const double diff = x - t.mean;

    // mu_n = mu_{n-1} + (x_n - mu_{n-1}) / n
    const double mean = t.mean + diff / n;

    // S_n = S_{n-1} + (x_n - mu_{n-1}) * (x_n - mu_n)
    const double var2 = t.var2 + diff * (x - mean);

    t.n = n;
    t.mean = mean;
    t.var2 = var2;
}

static inline double Mean(Timing const& t)
{
    return t.mean;
}

static inline double Var2(Timing const& t)
{
    return t.var2 / static_cast<double>(t.n - 1);
//  return t.var2 / static_cast<double>(t.n);
}
#endif

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

    jsonstats expected_stats;
    double reference = 0;
    bool first = true;
    for (size_t i = 0; i < array_length(test_implementations); ++i) {
        auto& implementation = test_implementations[i];
        jsonstats this_stats;
        if (first) {
            implementation.func(expected_stats, file);
        } else {
            implementation.func(this_stats, file);
            if (this_stats != expected_stats) {
                fprintf(stderr, "parse results did not match.\nexpected:\n");
                expected_stats.print(stderr);
                fprintf(stderr, "actual:\n");
                this_stats.print(stderr);
            }
        }

        auto start = Clock::now();
        auto end = start;

        bool warmup = SECONDS_PER_WARMUP != std::chrono::seconds{0};

        int parses = 0;
        for (;;) {
            implementation.func(this_stats, file);
            end = Clock::now();
            if (warmup) {
                if (end - start > SECONDS_PER_WARMUP) {
                    warmup = false;
                    start = end;
                }
            } else {
                ++parses;
                if (end - start > SECONDS_PER_TEST)
                    break;
            }
        }

        auto const mean = std::chrono::duration<double>(end - start).count() / static_cast<double>(parses);

        //const double MB = static_cast<double>(length) / 1024.0 / 1024.0;
        //const double MBPerSec = MB / mean;
        const double GBPerSec = (length / (1024.0 * 1024.0 * 1024.0)) / mean;

        if (first) {
            reference = mean;
        }

        printf("Test: %-20s %8.2f ms --- %8.6f GB/sec", implementation.name, 1000.0*mean, GBPerSec);
        if (first) {
            printf("\n");
        } else {
            printf(" --- x %.2f = 1 / %.2f\n", reference / mean, mean / reference);
        }
        fflush(stdout);

        first = false;
        //reference = mean;
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
