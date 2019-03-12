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

TestImplementation test_implementations[] = {
#if 0
    { "json1 sax", &json1_sax_test::test },
    { "rapidjson sax", &rapidjson_sax_test::test },
    //{ "rapidjson sax", &rapidjson_sax_test::test },
    //{ "nlohmann sax", &nlohmann_sax_test::test },
    { "nlohmann sax", &nlohmann_sax_test::test },
#else
    { "json1 dom", &json1_dom_test::test },
    { "sajson dom", &sajson_dom_test::test },
    //{ "rapidjson dom", &rapidjson_dom_test::test },
    //{ "nlohmann dom", &nlohmann_dom_test::test },
    { "simdjson dom", &simdjson_dom_test::test },
#endif
};

const char* benchmark_files[] = {
#if 0
// bench other:
    //"test_data/truenull.json",
    //"test_data/whitespace.json",

// strings:
    //"test_data/apache_builds.json",
    "test_data/allthethings.json",
    "test_data/twitter.json",
    "test_data/sample.json",
    //"test_data/update-center.json",

// numbers:
    //"test_data/floats.json",
    ////"test_data/signed_ints.json",
    ////"test_data/unsigned_ints.json",
    //"test_data/canada.json",
    //"test_data/mesh.json",

// strings + numbers:
    //"test_data/citm_catalog.json",
    //"test_data/instruments.json",
    //"test_data/github_events.json",

//// small files:
//    //"test_data/svg_menu.json",
#endif

#if 0
    "test_data/jsonexamples/tiny0.json",
    "test_data/jsonexamples/tiny1.json",
    "test_data/jsonexamples/tiny2.json",
    "test_data/jsonexamples/tiny3.json",
    "test_data/jsonexamples/tiny4.json",
    "test_data/jsonexamples/tiny5.json",
    "test_data/jsonexamples/tiny6.json",
#endif
#if 0
    "test_data/jsonexamples/apache_builds.json",
    "test_data/jsonexamples/canada.json",
    "test_data/jsonexamples/citm_catalog.json",
    "test_data/jsonexamples/github_events.json",
    "test_data/jsonexamples/gsoc-2018.json",
    "test_data/jsonexamples/instruments.json",
    "test_data/jsonexamples/marine_ik.json",
    "test_data/jsonexamples/mesh.json",
    "test_data/jsonexamples/mesh.pretty.json",
    "test_data/jsonexamples/numbers.json",
    "test_data/jsonexamples/random.json",
    "test_data/jsonexamples/twitter.json",
    "test_data/jsonexamples/twitterescaped.json",
    "test_data/jsonexamples/update-center.json",
#endif
#if 1
    "test_data/jsonexamples/mini/apache_builds.json",
    "test_data/jsonexamples/mini/canada.json",
    "test_data/jsonexamples/mini/citm_catalog.json",
    "test_data/jsonexamples/mini/github_events.json",
    "test_data/jsonexamples/mini/gsoc-2018.json",
    "test_data/jsonexamples/mini/instruments.json",
    "test_data/jsonexamples/mini/marine_ik.json",
    "test_data/jsonexamples/mini/mesh.json",
    "test_data/jsonexamples/mini/numbers.json",
    "test_data/jsonexamples/mini/random.json",
    "test_data/jsonexamples/mini/twitter.json",
    "test_data/jsonexamples/mini/update-center.json",
#endif
};

template<typename T, size_t L>
size_t array_length(T(&)[L]) {
    return L;
}

using Clock = std::chrono::steady_clock;

const auto SECONDS_PER_WARMUP = std::chrono::seconds(4);
const auto SECONDS_PER_TEST = std::chrono::seconds(4);

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

        const double MB = static_cast<double>(length) / 1024.0 / 1024.0;
        const double MBPerSec = MB / mean;

        if (first) {
            reference = mean;
        }

        printf("%-20s%-60s %8.3f MB %8.2f ms (%10.6f GB/sec)", implementation.name, filename, MB/1.0, 1000.0*mean, MBPerSec/1000.0);
        if (first) {
            printf("\n");
        } else {
            printf(" --- x%.2f\n", reference / mean);
        }
        fflush(stdout);

        first = false;
    }
}

int main(int argc, const char** argv) {
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
