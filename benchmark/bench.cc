// Modified sajson benchmark
// Copyright (c) 2012-2017 Chad Austin
// https://github.com/chadaustin/sajson

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cfloat>
#include <algorithm>
#include <vector>
#include <time.h>
#include <cmath>
#include <chrono>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <windows.h>
#endif

#include "bench_json1.h"
#include "bench_rapidjson.h"

#define REVERSE_ORDER 1

struct TestFile {
    const char* name;
    size_t length;
    const unsigned char* data;
};

class ZeroTerminatedCopy {
public:
    ZeroTerminatedCopy(const TestFile& file) {
        data = new char[1 + file.length];
        memcpy(data, file.data, file.length);
        data[file.length] = 0;
    }

    ZeroTerminatedCopy(size_t length, const void* original_data) {
        data = new char[1 + length];
        memcpy(data, original_data, length);
        data[length] = 0;
    }

    ~ZeroTerminatedCopy() {
        delete[] data;
    }

    char* get() const {
        return data;
    }

private:
    char* data;
};

class Copy {
public:
    Copy(const TestFile& file) {
        data = new char[file.length];
        memcpy(data, file.data, file.length);
    }

    Copy(size_t length, const void* original_data) {
        data = new char[length];
        memcpy(data, original_data, length);
    }

    ~Copy() {
        delete[] data;
    }

    char* get() const {
        return data;
    }

private:
    char* data;
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

//namespace nlohmann_sax_test {
//    void test(jsonstats& stats, const TestFile& file) {
//        if (!nlohmann_sax_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
//            fprintf(stderr, "rapidjson sax parse error\n");
//            abort();
//        }
//    }
//}
//
//namespace nlohmann_dom_test {
//    void test(jsonstats& stats, const TestFile& file) {
//        if (!nlohmann_dom_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
//            fprintf(stderr, "rapidjson dom parse error\n");
//            abort();
//        }
//    }
//}

struct TestImplementation {
    typedef void (*TestFunction)(jsonstats&, const TestFile&);

    const char* name;
    TestFunction func;
};

TestImplementation test_implementations[] = {
#if 1
#if REVERSE_ORDER
    { "json1 sax", &json1_sax_test::test },
    { "rapidjson sax", &rapidjson_sax_test::test },
#else
    { "json1 sax", &json1_sax_test::test },
    { "rapidjson sax", &rapidjson_sax_test::test },
#endif
#else
#if REVERSE_ORDER
    { "json1 dom", &json1_dom_test::test },
    { "rapidjson dom", &rapidjson_dom_test::test },
#else
    { "rapidjson dom", &rapidjson_dom_test::test },
    { "json1 dom", &json1_dom_test::test },
#endif
#endif
};

const char* benchmark_files[] = {
// bench other:
    //"test_data/truenull.json",
    //"test_data/whitespace.json",

// strings:
    "test_data/apache_builds.json",
    "test_data/allthethings.json",
    "test_data/twitter.json",
    "test_data/sample.json",
    "test_data/update-center.json",

// numbers:
    //"test_data/floats.json",
    //"test_data/signed_ints.json",
    //"test_data/unsigned_ints.json",
    "test_data/canada.json",
    "test_data/mesh.json",

// strings + numbers:
    "test_data/citm_catalog.json",
    "test_data/instruments.json",
    "test_data/github_events.json",

// small files:
    //"test_data/svg_menu.json",
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

void benchmark(const char* filename) {
    FILE* fh = fopen(filename, "rb");
    if (!fh) {
        fprintf(stderr, "failed to open file: %s\n", filename);
        exit(1);
    }
    fseek(fh, 0, SEEK_END);
    auto const slength = ftell(fh);
    assert(slength >= 0);
    size_t length = static_cast<size_t>(slength);

    std::vector<char> contents(length);

    fseek(fh, 0, SEEK_SET);
    if (length != fread(&contents[0], 1, length, fh)) {
        fprintf(stderr, "Failed to read file\n");
        abort();
    }
    fclose(fh);

    contents.push_back('\0');

    TestFile file = { filename, length, reinterpret_cast<unsigned char*>(&contents[0]) };

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

        bool warmup = true;

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

        printf("%-20s%-40s %8.2f ms (%8.2f MB/sec)", implementation.name,  filename, 1000.0*mean, MBPerSec);
        if (first) {
            printf("\n");
        } else {
#if REVERSE_ORDER
            printf(" --- x%.1f\n", mean / reference);
#else
            printf(" --- x%.1f\n", reference / mean);
#endif
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
