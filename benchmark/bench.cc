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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <windows.h>
#endif

#include "bench_json1.h"
#include "bench_rapidjson.h"
#include "bench_nlohmann.h"

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

namespace nlohmann_sax_test {
    void test(jsonstats& stats, const TestFile& file) {
        if (!nlohmann_sax_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
            fprintf(stderr, "rapidjson sax parse error\n");
            abort();
        }
    }
}

namespace nlohmann_dom_test {
    void test(jsonstats& stats, const TestFile& file) {
        if (!nlohmann_dom_stats(stats, reinterpret_cast<char const*>(file.data), reinterpret_cast<char const*>(file.data) + file.length)) {
            fprintf(stderr, "rapidjson dom parse error\n");
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
    { "nlohmann sax", &nlohmann_sax_test::test },
#else
    { "json1 dom", &json1_dom_test::test },
    { "rapidjson dom", &rapidjson_dom_test::test },
    { "nlohmann dom", &nlohmann_dom_test::test },
#endif
};

const char* benchmark_files[] = {
    "test_data/floats.json",
    "test_data/signed_ints.json",
    "test_data/unsigned_ints.json",
    "test_data/allthethings.json",
    "test_data/apache_builds.json",
    "test_data/canada.json",
    "test_data/citm_catalog.json",
    "test_data/github_events.json",
    "test_data/instruments.json",
    "test_data/mesh.json",
    //"test_data/sample.json", // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX stack overflow
    "test_data/svg_menu.json",
    "test_data/twitter.json",
    "test_data/update-center.json",
};

const double SECONDS_PER_TEST = 3.0;

template<typename T, size_t L>
size_t array_length(T(&)[L]) {
    return L;
}

#ifdef _WIN32

struct QPCFrequency {
    QPCFrequency() {
        LARGE_INTEGER li;
        li.QuadPart = 1;
        QueryPerformanceFrequency(&li);
        frequency = double(li.QuadPart);

        QueryPerformanceCounter(&li);
        start = li.QuadPart;
    }

    __int64 start;
    double frequency;
} QPC;

static double get_time() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart - QPC.start) / QPC.frequency;
}

#else

static double get_time() {
    timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return double(ts.tv_sec) + double(ts.tv_nsec) / 1000000000.0;
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

    TestFile file = { filename, length, reinterpret_cast<unsigned char*>(&contents[0]) };

    jsonstats expected_stats;
    double reference_time = 0;
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

        double start = get_time();
        double until = start + SECONDS_PER_TEST;
        double end = start;
        double fastest = DBL_MAX;
        int parses = 0;
        do {
            implementation.func(this_stats, file);
            ++parses;
            double next_end = get_time();
            fastest = std::min(fastest, next_end - end);
            end = next_end;
        } while (end < until);

        if (first) {
            reference_time = fastest;
        }

        printf("%-20s%-50s%10.2f MB/sec", implementation.name, filename, static_cast<double>(length) / fastest / 1000000.0);
        if (first) {
            printf("\n");
        } else {
            printf(" x%.4f\n", fastest / reference_time);
        }
        fflush(stdout);

        first = false;
    }
}

int main(int argc, const char** argv) {
    printf("Implementation,File,MB/s\n");

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
