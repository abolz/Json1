#include "src/json.h"

#include "catch.hpp"

#include <tuple>

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

TEST_CASE("Value - implicit constructors")
{
    SECTION("null")
    {
        json::Value j = nullptr;
        CHECK(j.is_null());
    }

    SECTION("boolean")
    {
        json::Value j0 = true;
        CHECK(j0.is_boolean());
        CHECK(j0.as_boolean() == true);

        json::Value j1 = false;
        CHECK(j1.is_boolean());
        CHECK(j1.as_boolean() == false);
    }

    SECTION("number")
    {
        json::Value j0 = static_cast<signed char>(-1);
        CHECK(j0.is_number());
        CHECK(j0.as_number() == -1.0);

        json::Value j1 = static_cast<signed short>(-1);
        CHECK(j1.is_number());
        CHECK(j1.as_number() == -1.0);

        json::Value j2 = -1;
        CHECK(j2.is_number());
        CHECK(j2.as_number() == -1.0);

        json::Value j3 = -1l; // (warning expected)
        CHECK(j3.is_number());
        CHECK(j3.as_number() == -1.0);

        json::Value j4 = -1ll; // warning expected
        CHECK(j4.is_number());
        CHECK(j4.as_number() == -1.0);

        json::Value j4_1(-1ll); // no warning expected
        CHECK(j4_1.is_number());
        CHECK(j4_1.as_number() == -1.0);

        json::Value j5 = static_cast<unsigned char>(1);
        CHECK(j5.is_number());
        CHECK(j5.as_number() == 1.0);

        json::Value j6 = static_cast<unsigned short>(1);
        CHECK(j6.is_number());
        CHECK(j6.as_number() == 1.0);

        json::Value j7 = 1u;
        CHECK(j7.is_number());
        CHECK(j7.as_number() == 1.0);

        json::Value j8 = 1ul; // (warning expected)
        CHECK(j8.is_number());
        CHECK(j8.as_number() == 1.0);

        json::Value j9 = 1ull; // warning expected
        CHECK(j9.is_number());
        CHECK(j9.as_number() == 1.0);

        json::Value j10 = 1.2;
        CHECK(j10.is_number());
        CHECK(j10.as_number() == 1.2);

#if JSON_VALUE_HAS_EXPLICIT_OPERATOR_T
        int i0 = static_cast<int>(j10); // warning expected (double -> int)
        CHECK(i0 == 1);
        int i1 = static_cast<float>(j10); // two warnings expected! (double -> float, float -> int)
        CHECK(i1 == 1);
        int i2 = static_cast<int>(static_cast<double>(j10)); // no warning
        CHECK(i2 == 1);
#endif

        int i3 = j10.cast<int>(); // warning expected (double -> int)
        CHECK(i3 == 1);
        int i4 = j10.cast<float>(); // two warnings expected! (double -> float, float -> int)
        CHECK(i4 == 1);
        int i5 = static_cast<int>(j10.cast<double>()); // no warning
        CHECK(i5 == 1);

        json::Value j11 = 1.234f;
        CHECK(j11.is_number());
        CHECK(j11.as_number() == static_cast<double>(1.234f));
    }

    SECTION("string")
    {
        json::Value j0 = std::string("hello");
        CHECK(j0.is_string());
        CHECK(j0.as_string() == "hello");
#if 0
        // Should not compile!
        auto s0 = std::move(j1).cast<char const*>();
#endif
#if 0
        // Should not compile!
        auto s0 = std::move(j1).cast<cxx::string_view>();
#endif

        json::Value j1 = "hello";
        CHECK(j1.is_string());
        CHECK(j1.as_string() == "hello");

        //auto s1 = j1.to<char const*>();
        //CHECK(s1 == std::string("hello"));
        //auto s2 = j1.to<cxx::string_view>();
        //CHECK(s2 == "hello");

        json::Value j2 = const_cast<char*>("hello");
        CHECK(j2.is_string());
        CHECK(j2.as_string() == "hello");

        json::Value j3 = "hello";
        CHECK(j3.is_string());
        CHECK(j3.as_string() == "hello");

        json::Value j4 = cxx::string_view("hello");
        CHECK(j4.is_string());
        CHECK(j4.as_string() == "hello");

        //auto s3 = j4.to<char*>();
        //CHECK(s3 == std::string("hello"));
        //s3[0] = 'H';
        //CHECK(s3 == std::string("Hello"));
    }

    SECTION("array")
    {
    }

    SECTION("object")
    {
    }
}

TEST_CASE("Value - construct JSON")
{
    std::string const input = R"({
        "empty_array": [],
        "empty_object" : {},
        "FirstName": "John",
        "LastName": "Doe",
        "Age": 43,
        "Address": {
            "Street": "Downing \"Street\" 10",
            "City": "London",
            "Country": "Great Britain"
        },
        "Phone numbers": [
            "+44 1234567",
            "+44 2345678"
        ]
    })";

    json::Value val;
    json::parse(val, input.data(), input.data() + input.size());

    std::string str;
    json::stringify(str, val);

    json::Value val2;
    json::parse(val2, str.data(), str.data() + str.size());

    CHECK(val == val2);
    CHECK(val.hash() == val2.hash());

    json::Value val3 = json::Object{
        {"empty_array", json::Array{}},
        {"empty_object" , json::Object{}},
        {"FirstName", "John"},
        {"LastName", "Doe"},
        {"Age", 43ll},
        {"Address", json::Object{
            {"Street", "Downing \"Street\" 10"},
            {"City", "London"},
            {"Country", "Great Britain"}
        }},
        {"Phone numbers", json::Array{
            "+44 1234567",
            "+44 2345678"
        }},
    };

    CHECK(val == val3);
    CHECK(val.hash() == val3.hash());
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

#include <forward_list>
#include <iterator>
#include <list>
#include <set>

namespace json
{
    template <typename T, typename Alloc>
    struct Traits<std::list<T, Alloc>>
    {
        using tag = Tag_array;
        template <typename V> static decltype(auto) to_json(V&& in) { return Array(in.begin(), in.end()); }
        template <typename V> static decltype(auto) from_json(V&& in) = delete;
    };

    template <typename T, typename Compare, typename Alloc>
    struct Traits<std::set<T, Compare, Alloc>>
    {
        using tag = Tag_array;
        template <typename V> static decltype(auto) to_json(V&& in) { return Array(in.begin(), in.end()); }
        template <typename V> static decltype(auto) from_json(V&&) = delete;
    };
}

TEST_CASE("arrays")
{
    SECTION("json::Array")
    {
        json::Array a {1, "two", 3.3, true};
        json::Value j = a;
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 4);
        REQUIRE(j[0].is_number());
        REQUIRE(j[0].as_number() == 1);
        REQUIRE(j[1].is_string());
        REQUIRE(j[1].as_string() == "two");
        REQUIRE(j[2].is_number());
        REQUIRE(j[2].as_number() == 3.3);
        REQUIRE(j[3].is_boolean());
        REQUIRE(j[3].as_boolean() == true);
    }

    SECTION("std::list")
    {
        std::list<json::Value> a {1, "two", 3.3, true};
        json::Value j = a;//Value::from_array(a);
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 4);
        REQUIRE(j[0].is_number());
        REQUIRE(j[0].as_number() == 1);
        REQUIRE(j[1].is_string());
        REQUIRE(j[1].as_string() == "two");
        REQUIRE(j[2].is_number());
        REQUIRE(j[2].as_number() == 3.3);
        REQUIRE(j[3].is_boolean());
        REQUIRE(j[3].as_boolean() == true);
    }

    SECTION("std::forward_list")
    {
        std::forward_list<json::Value> a {1, "two", 3.3, true};
        json::Value j = json::Value::from_array(a);
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 4);
        REQUIRE(j[0].is_number());
        REQUIRE(j[0].as_number() == 1);
        REQUIRE(j[1].is_string());
        REQUIRE(j[1].as_string() == "two");
        REQUIRE(j[2].is_number());
        REQUIRE(j[2].as_number() == 3.3);
        REQUIRE(j[3].is_boolean());
        REQUIRE(j[3].as_boolean() == true);
    }

    SECTION("std::set")
    {
        std::set<json::Value> a {1, "two", 3.3, true};
        json::Value j = a;// Value::from_array(a);
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 4);
        // bool < number < string
        REQUIRE(j[0].is_boolean());
        REQUIRE(j[0].as_boolean() == true);
        REQUIRE(j[1].is_number());
        REQUIRE(j[1].as_number() == 1);
        REQUIRE(j[2].is_number());
        REQUIRE(j[2].as_number() == 3.3);
        REQUIRE(j[3].is_string());
        REQUIRE(j[3].as_string() == "two");
    }
}

namespace json
{
    template <typename ...Tn>
    struct Traits<std::tuple<Tn...>>
    {
        using tag = Tag_array;
        template <typename V> static decltype(auto) to_json(V&& in) // V = std::tuple<Tn...> [const][&]
        {
            static_cast<void>(in); // unused for empty tuples - fix warning
            return Array{std::get<Tn>(std::forward<V>(in))...};
        }
        template <typename V> static decltype(auto) from_json(V&& in) // V = Value [const&]
        {
            assert(in.as_array().size() == sizeof...(Tn));
            auto I = std::make_move_iterator(in.as_array().begin()); // Does _not_ move for V = Value const&
            return std::tuple<Tn...>{json::cast<Tn>(*I++)...};
            //                                       ^~~
            // Very small performance penalty here.
            // Could provide a specialization for empty/non-empty tuples using ++I instead of I++
        }
    };
}

TEST_CASE("tuple")
{
    SECTION("0-tuple")
    {
        using Tup = std::tuple<>;

        Tup tup;
        json::Value j = tup;
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 0);
        REQUIRE(j.empty());

        //auto t = j.cast<Tup>();
    }

    SECTION("1-tuple")
    {
        using Tup = std::tuple<double>;

        Tup tup = {1.2};
        json::Value j = tup;
        REQUIRE(j.is_array());
        REQUIRE(j[0].is_number());
        REQUIRE(j[0] == 1.2);

        auto t = j.cast<Tup>();
        REQUIRE(std::get<0>(t) == 1.2);
    }

    SECTION("2-tuple")
    {
        using Tup = std::tuple<double, std::string>;

        Tup tup = {2.34, "hello hello hello hello hello hello hello hello "};
        json::Value j = tup;
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 2);
        REQUIRE(j[0].is_number());
        REQUIRE(j[0] == 2.34);
        REQUIRE(j[1].is_string());
        REQUIRE(j[1] == "hello hello hello hello hello hello hello hello ");

        auto t1 = json::cast<Tup>(j);
        REQUIRE(std::get<0>(t1) == 2.34);
        REQUIRE(std::get<1>(t1) == "hello hello hello hello hello hello hello hello ");
        REQUIRE(j[1].is_string());
        REQUIRE(j[1].as_string() == "hello hello hello hello hello hello hello hello ");

        auto t2 = json::cast<Tup>(std::move(j));
        REQUIRE(std::get<0>(t2) == 2.34);
        REQUIRE(std::get<1>(t2) == "hello hello hello hello hello hello hello hello ");
#if 0//_MSC_VER // These tests probably work... XXX: Add a test type which has a distinguished moved-from state...
        REQUIRE(j[1].is_string());
        REQUIRE(j[1].as_string().empty());
#endif
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

#if __cplusplus >= 201703 || (_MSC_VER >= 1910 && _HAS_CXX17)
#define JSON_HAS_OPTIONAL 1
#endif

#if JSON_HAS_OPTIONAL

#include <optional>

namespace json
{
    template <typename T>
    struct Traits<std::optional<T>>
    {
        using tag = Tag_t<T>;

        template <typename V>
        static Value to_json(V&& in) // V = std::optional<T> [const][&]
        {
            if (!in.has_value())
                return {}; // null
            return Traits_t<T>::to_json(std::forward<V>(in).value());
        }

        template <typename V>
        static std::optional<T> from_json(V&& in) // V = Value [const][&]
        {
            if (in.type() != tag::value)
                return {}; // nullopt
#if 1
            return Traits_t<T>::from_json(std::forward<V>(in));
#else
            return static_cast<T>( Traits_t<TargetType_t<T>>::from_json(std::forward<V>(in)) );
#endif
        }
    };
}

TEST_CASE("optional")
{
    SECTION("optional -> Value")
    {
        std::optional<int> oi0;
        CHECK(!oi0.has_value());

        json::Value j0 = oi0;
        CHECK(j0.is_null());

        std::optional<int> oi1 = 123;
        CHECK(oi1.has_value());
        CHECK(oi1.value() == 123);

        json::Value j1 = oi1;
        CHECK(j1.is_number());
        CHECK(j1.as_number() == 123);
    }

    SECTION("Value -> optional")
    {
        json::Value j0 = 123;
        CHECK(j0.is_number());
        CHECK(j0.as_number() == 123);

        auto oi0 = j0.cast<std::optional<int>>(); // warning expected -- XXX: need a way to silence this warning...
        CHECK(oi0.has_value());
        CHECK(oi0.value() == 123);

        auto oi1 = j0.cast<std::optional<std::string>>();
        CHECK(!oi1.has_value());

#if JSON_VALUE_HAS_EXPLICIT_OPERATOR_T
        // This is the tricky part...
        // optional<T>'s _implicit_ constructor is preferred over Value's _explicit_ conversion operator.
        // The implicit constructor then calls 'explicit operator T' (not 'explicit operator std::optional<T>').

        // This works,
        // since Value's 'explicit operator int' is called, which calls as_number() and j0 actually contains a number.
        auto oi4 = std::optional<int>(j0); // warning expected
        CHECK(oi4.has_value());
        CHECK(oi4.value() == 123);

        // This does not work.
        // Calls 'explicit operator std::string' and j0 does not contain a string.
        // It could be made to work if the json::impl::DefaultTraits could return a default value in case the
        // JSON value does not contain a number,string,etc...
        // DefaultTraits<T>::from_json could return an optional<T>...
#if 0
        auto oi5 = std::optional<std::string>(j0);
        CHECK(!oi5.has_value());
#endif
#endif
    }
}

#endif // JSON_HAS_OPTIONAL

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

// Test suite from:
// http://www.json.org/JSON_checker/

static std::string const kJsonCheckerPass[] = {
    //1
    R"([
    "JSON Test Pattern pass1",
    {"object with 1 member":["array with 1 element"]},
    {},
    [],
    -42,
    true,
    false,
    null,
    {
        "integer": 1234567890,
        "real": -9876.543210,
        "e": 0.123456789e-12,
        "E": 1.234567890E+34,
        "":  23456789012E66,
        "zero": 0,
        "one": 1,
        "space": " ",
        "quote": "\"",
        "backslash": "\\",
        "controls": "\b\f\n\r\t",
        "slash": "/ & \/",
        "alpha": "abcdefghijklmnopqrstuvwyz",
        "ALPHA": "ABCDEFGHIJKLMNOPQRSTUVWYZ",
        "digit": "0123456789",
        "0123456789": "digit",
        "special": "`1~!@#$%^&*()_+-={':[,]}|;.</>?",
        "hex": "\u0123\u4567\u89AB\uCDEF\uabcd\uef4A",
        "true": true,
        "false": false,
        "null": null,
        "array":[  ],
        "object":{  },
        "address": "50 St. James Street",
        "url": "http://www.JSON.org/",
        "comment": "// /* <!-- --",
        "# -- --> */": " ",
        " s p a c e d " :[1,2 , 3

,

4 , 5        ,          6           ,7        ],"compact":[1,2,3,4,5,6,7],
        "jsontext": "{\"object with 1 member\":[\"array with 1 element\"]}",
        "quotes": "&#34; \u0022 %22 0x22 034 &#x22;",
        "\/\\\"\uCAFE\uBABE\uAB98\uFCDE\ubcda\uef4A\b\f\n\r\t`1~!@#$%^&*()_+-=[]{}|;:',./<>?"
: "A key can be any string"
    },
    0.5 ,98.6
,
99.44
,

1066,
1e1,
0.1e1,
1e-1,
1e00,2e+00,2e-00
,"rosebud"])",
    //2
    R"([[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]])",
    //3
    R"({
    "JSON Test Pattern pass3": {
        "The outermost value": "must be an object or array.",
        "In this test": "It is an object."
    }
}
)",
};

static std::string const kJsonCheckerFail[] = {
// Test suite from:
// http://www.json.org/JSON_checker/

    //R"("A JSON payload should be an object or array, not a string.")",
    R"(["Unclosed array")",
    R"({unquoted_key: "keys must be quoted"})",
    R"(["extra comma",])",
    R"(["double extra comma",,])",
    R"([   , "<-- missing value"])",
    R"(["Comma after the close"],)",
    R"(["Extra close"]])",
    R"({"Extra comma": true,})",
    R"({"Extra value after close": true} "misplaced quoted value")",
    R"({"Illegal expression": 1 + 2})",
    R"({"Illegal invocation": alert()})",
    R"({"Numbers cannot have leading zeroes": 013})",
    R"({"Numbers cannot be hex": 0x14})",
    R"(["Illegal backslash escape: \x15"])",
    R"([\naked])",
    R"(["Illegal backslash escape: \017"])",
    //R"([[[[[[[[[[[[[[[[[[[["Too deep"]]]]]]]]]]]]]]]]]]]])",
    R"({"Missing colon" null})",
    R"({"Double colon":: null})",
    R"({"Comma instead of colon", null})",
    R"(["Colon instead of comma": false])",
    R"(["Bad value", truth])",
    R"(['single quote'])",
    R"(["	tab	character	in	string	"])",
    R"(["tab\   character\   in\  string\  "])",
    R"(["line
break"])",
    R"(["line\
break"])",
    R"([0e])",
    R"([0e+])",
    R"([0e+-1])",
    R"({"Comma instead if closing brace": true,)",
    R"(["mismatch"})",
};

TEST_CASE("JSON_checker")
{
    SECTION("pass")
    {
        for (auto const& inp : kJsonCheckerPass)
        {
            CAPTURE(inp);

            json::Value val;
            auto const res = json::parse(val, inp.data(), inp.data() + inp.size());
            CHECK(res.ec == json::ErrorCode::success);

            std::string s;
            json::StringifyOptions options;
            //options.indent_width = INT8_MIN;
            //options.indent_width = INT8_MAX;
            options.indent_width = 4;
            auto const str_ok = json::stringify(s, val, options);
            CHECK(str_ok);

            //printf("%s\n", s.c_str());

            json::Value val2;
            auto const res2 = json::parse(val2, s.data(), s.data() + s.size());
            CHECK(res2.ec == json::ErrorCode::success);

            CHECK(val == val2);
        }
    }

    SECTION("fail")
    {
        for (auto const& inp : kJsonCheckerFail)
        {
            CAPTURE(inp);

            json::Value val;
            auto const res = json::parse(val, inp.data(), inp.data() + inp.size());
            CHECK(res.ec != json::ErrorCode::success);
        }
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

// From:
// https://github.com/nst/JSONTestSuite
//
// MIT License
//
// Copyright (c) 2016 Nicolas Seriot
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

struct JSONTestSuiteTest {
    std::string input;
    std::string name;
};

static const JSONTestSuiteTest kJSONTestSuitePass[] = {
    {std::string("[[]   ]", 7), "y_array_arraysWithSpaces"},
    {std::string("[\"\"]", 4), "y_array_empty-string"},
    {std::string("[]", 2), "y_array_empty"},
    {std::string("[\"a\"]", 5), "y_array_ending_with_newline"},
    {std::string("[false]", 7), "y_array_false"},
    {std::string("[null, 1, \"1\", {}]", 18), "y_array_heterogeneous"},
    {std::string("[null]", 6), "y_array_null"},
    {std::string("[1\n]", 4), "y_array_with_1_and_newline"},
    {std::string(" [1]", 4), "y_array_with_leading_space"},
    {std::string("[1,null,null,null,2]", 20), "y_array_with_several_null"},
    {std::string("[2] ", 4), "y_array_with_trailing_space"},
    {std::string("[123e65]", 8), "y_number"},
    {std::string("[0e+1]", 6), "y_number_0e+1"},
    {std::string("[0e1]", 5), "y_number_0e1"},
    {std::string("[ 4]", 4), "y_number_after_space"},
    {std::string("[-0.000000000000000000000000000000000000000000000000000000000000000000000000000001]\n", 84), "y_number_double_close_to_zero"},
    {std::string("[20e1]", 6), "y_number_int_with_exp"},
    {std::string("[-0]", 4), "y_number_minus_zero"},
    {std::string("[-123]", 6), "y_number_negative_int"},
    {std::string("[-1]", 4), "y_number_negative_one"},
    {std::string("[-0]", 4), "y_number_negative_zero"},
    {std::string("[1E22]", 6), "y_number_real_capital_e"},
    {std::string("[1E-2]", 6), "y_number_real_capital_e_neg_exp"},
    {std::string("[1E+2]", 6), "y_number_real_capital_e_pos_exp"},
    {std::string("[123e45]", 8), "y_number_real_exponent"},
    {std::string("[123.456e78]", 12), "y_number_real_fraction_exponent"},
    {std::string("[1e-2]", 6), "y_number_real_neg_exp"},
    {std::string("[1e+2]", 6), "y_number_real_pos_exponent"},
    {std::string("[123]", 5), "y_number_simple_int"},
    {std::string("[123.456789]", 12), "y_number_simple_real"},
    {std::string("{\"asd\":\"sdf\", \"dfg\":\"fgh\"}", 26), "y_object"},
    {std::string("{\"asd\":\"sdf\"}", 13), "y_object_basic"},
    {std::string("{\"a\":\"b\",\"a\":\"c\"}", 17), "y_object_duplicated_key"},
    {std::string("{\"a\":\"b\",\"a\":\"b\"}", 17), "y_object_duplicated_key_and_value"},
    {std::string("{}", 2), "y_object_empty"},
    {std::string("{\"\":0}", 6), "y_object_empty_key"},
    {std::string("{\"foo\\u0000bar\": 42}", 20), "y_object_escaped_null_in_key"},
    {std::string("{ \"min\": -1.0e+28, \"max\": 1.0e+28 }", 35), "y_object_extreme_numbers"},
    {std::string("{\"x\":[{\"id\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}], \"id\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}", 108), "y_object_long_strings"},
    {std::string("{\"a\":[]}", 8), "y_object_simple"},
    {std::string("{\"title\":\"\\u041f\\u043e\\u043b\\u0442\\u043e\\u0440\\u0430 \\u0417\\u0435\\u043c\\u043b\\u0435\\u043a\\u043e\\u043f\\u0430\" }", 110), "y_object_string_unicode"},
    {std::string("{\n\"a\": \"b\"\n}", 12), "y_object_with_newlines"},
    {std::string("[\"\\u0060\\u012a\\u12AB\"]", 22), "y_string_1_2_3_bytes_UTF-8_sequences"},
    {std::string("[\"\\uD801\\udc37\"]", 16), "y_string_accepted_surrogate_pair"},
    {std::string("[\"\\ud83d\\ude39\\ud83d\\udc8d\"]", 28), "y_string_accepted_surrogate_pairs"},
    {std::string("[\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"]", 20), "y_string_allowed_escapes"},
    {std::string("[\"\\\\u0000\"]", 11), "y_string_backslash_and_u_escaped_zero"},
    {std::string("[\"\\\"\"]", 6), "y_string_backslash_doublequotes"},
    {std::string("[\"a/*b*/c/*d//e\"]", 17), "y_string_comments"},
    {std::string("[\"\\\\a\"]", 7), "y_string_double_escape_a"},
    {std::string("[\"\\\\n\"]", 7), "y_string_double_escape_n"},
    {std::string("[\"\\u0012\"]", 10), "y_string_escaped_control_character"},
    {std::string("[\"\\uFFFF\"]", 10), "y_string_escaped_noncharacter"},
    {std::string("[\"asd\"]", 7), "y_string_in_array"},
    {std::string("[ \"asd\"]", 8), "y_string_in_array_with_leading_space"},
    {std::string("[\"\\uDBFF\\uDFFF\"]", 16), "y_string_last_surrogates_1_and_2"},
    {std::string("[\"new\\u00A0line\"]", 17), "y_string_nbsp_uescaped"},
    {std::string("[\"\364\217\277\277\"]", 8), "y_string_nonCharacterInUTF-8_U+10FFFF"},
    {std::string("[\"\360\233\277\277\"]", 8), "y_string_nonCharacterInUTF-8_U+1FFFF"},
    {std::string("[\"\357\277\277\"]", 7), "y_string_nonCharacterInUTF-8_U+FFFF"},
    {std::string("[\"\\u0000\"]", 10), "y_string_null_escape"},
    {std::string("[\"\\u002c\"]", 10), "y_string_one-byte-utf-8"},
    {std::string("[\"\317\200\"]", 6), "y_string_pi"},
    {std::string("[\"asd \"]", 8), "y_string_simple_ascii"},
    {std::string("\" \"", 3), "y_string_space"},
    {std::string("[\"\\uD834\\uDd1e\"]", 16), "y_string_surrogates_U+1D11E_MUSICAL_SYMBOL_G_CLEF"},
    {std::string("[\"\\u0821\"]", 10), "y_string_three-byte-utf-8"},
    {std::string("[\"\\u0123\"]", 10), "y_string_two-byte-utf-8"},
    {std::string("[\"\342\200\250\"]", 7), "y_string_u+2028_line_sep"},
    {std::string("[\"\342\200\251\"]", 7), "y_string_u+2029_par_sep"},
    {std::string("[\"\\u0061\\u30af\\u30EA\\u30b9\"]", 28), "y_string_uEscape"},
    {std::string("[\"new\\u000Aline\"]", 17), "y_string_uescaped_newline"},
    {std::string("[\"\177\"]", 5), "y_string_unescaped_char_delete"},
    {std::string("[\"\\uA66D\"]", 10), "y_string_unicode"},
    {std::string("[\"\\u005C\"]", 10), "y_string_unicodeEscapedBackslash"},
    {std::string("[\"\342\215\202\343\210\264\342\215\202\"]", 13), "y_string_unicode_2"},
    {std::string("[\"\\u0022\"]", 10), "y_string_unicode_escaped_double_quote"},
    {std::string("[\"\\uDBFF\\uDFFE\"]", 16), "y_string_unicode_U+10FFFE_nonchar"},
    {std::string("[\"\\uD83F\\uDFFE\"]", 16), "y_string_unicode_U+1FFFE_nonchar"},
    {std::string("[\"\\u200B\"]", 10), "y_string_unicode_U+200B_ZERO_WIDTH_SPACE"},
    {std::string("[\"\\u2064\"]", 10), "y_string_unicode_U+2064_invisible_plus"},
    {std::string("[\"\\uFDD0\"]", 10), "y_string_unicode_U+FDD0_nonchar"},
    {std::string("[\"\\uFFFE\"]", 10), "y_string_unicode_U+FFFE_nonchar"},
    {std::string("[\"\342\202\254\360\235\204\236\"]", 11), "y_string_utf8"},
    {std::string("[\"a\177a\"]", 7), "y_string_with_del_character"},
    {std::string("false", 5), "y_structure_lonely_false"},
    {std::string("42", 2), "y_structure_lonely_int"},
    {std::string("-0.1", 4), "y_structure_lonely_negative_real"},
    {std::string("null", 4), "y_structure_lonely_null"},
    {std::string("\"asd\"", 5), "y_structure_lonely_string"},
    {std::string("true", 4), "y_structure_lonely_true"},
    {std::string("\"\"", 2), "y_structure_string_empty"},
    {std::string("[\"a\"]\n", 6), "y_structure_trailing_newline"},
    {std::string("[true]", 6), "y_structure_true_in_array"},
    {std::string(" [] ", 4), "y_structure_whitespace_array"},
};

static const JSONTestSuiteTest kJSONTestSuiteFail[] = {
    {std::string("[1 true]", 8), "n_array_1_true_without_comma"},
    {std::string("[a\345]", 4), "n_array_a_invalid_utf8"},
    {std::string("[\"\": 1]", 7), "n_array_colon_instead_of_comma"},
    {std::string("[\"\"],", 5), "n_array_comma_after_close"},
    {std::string("[,1]", 4), "n_array_comma_and_number"},
    {std::string("[1,,2]", 6), "n_array_double_comma"},
    {std::string("[\"x\",,]", 7), "n_array_double_extra_comma"},
    {std::string("[\"x\"]]", 6), "n_array_extra_close"},
    {std::string("[\"\",]", 5), "n_array_extra_comma"},
    {std::string("[\"x\"", 4), "n_array_incomplete"},
    {std::string("[x", 2), "n_array_incomplete_invalid_value"},
    {std::string("[3[4]]", 6), "n_array_inner_array_no_comma"},
    {std::string("[\377]", 3), "n_array_invalid_utf8"},
    {std::string("[1:2]", 5), "n_array_items_separated_by_semicolon"},
    {std::string("[,]", 3), "n_array_just_comma"},
    {std::string("[-]", 3), "n_array_just_minus"},
    {std::string("[   , \"\"]", 9), "n_array_missing_value"},
    {std::string("[\"a\",\n4\n,1,", 11), "n_array_newlines_unclosed"},
    {std::string("[1,]", 4), "n_array_number_and_comma"},
    {std::string("[1,,]", 5), "n_array_number_and_several_commas"},
    {std::string("[\"\va\"\\f]", 8), "n_array_spaces_vertical_tab_formfeed"},
    {std::string("[*]", 3), "n_array_star_inside"},
    {std::string("[\"\"", 3), "n_array_unclosed"},
    {std::string("[1,", 3), "n_array_unclosed_trailing_comma"},
    {std::string("[1,\n1\n,1", 8), "n_array_unclosed_with_new_lines"},
    {std::string("[{}", 3), "n_array_unclosed_with_object_inside"},
    {std::string("[fals]", 6), "n_incomplete_false"},
    {std::string("[nul]", 5), "n_incomplete_null"},
    {std::string("[tru]", 5), "n_incomplete_true"},
    {std::string("123\000", 4), "n_multidigit_number_then_00"},
    {std::string("[++1234]", 8), "n_number_++"},
    {std::string("[+1]", 4), "n_number_+1"},
    {std::string("[-01]", 5), "n_number_-01"},
    {std::string("[-1.0.]", 7), "n_number_-1.0."},
    {std::string("[-2.]", 5), "n_number_-2."},
    {std::string("[.-1]", 5), "n_number_.-1"},
    {std::string("[.2e-3]", 7), "n_number_.2e-3"},
    {std::string("[0.1.2]", 7), "n_number_0.1.2"},
    {std::string("[0.3e+]", 7), "n_number_0.3e+"},
    {std::string("[0.3e]", 6), "n_number_0.3e"},
    {std::string("[0.e1]", 6), "n_number_0.e1"},
    {std::string("[0e+]", 5), "n_number_0e+"},
    {std::string("[0e]", 4), "n_number_0e"},
    {std::string("[0E+]", 5), "n_number_0_capital_E+"},
    {std::string("[0E]", 4), "n_number_0_capital_E"},
    {std::string("[1.0e+]", 7), "n_number_1.0e+"},
    {std::string("[1.0e-]", 7), "n_number_1.0e-"},
    {std::string("[1.0e]", 6), "n_number_1.0e"},
    {std::string("[1eE2]", 6), "n_number_1eE2"},
    {std::string("[1 000.0]", 9), "n_number_1_000"},
    {std::string("[2.e+3]", 7), "n_number_2.e+3"},
    {std::string("[2.e-3]", 7), "n_number_2.e-3"},
    {std::string("[2.e3]", 6), "n_number_2.e3"},
    {std::string("[9.e+]", 6), "n_number_9.e+"},
    {std::string("[1+2]", 5), "n_number_expression"},
    {std::string("[0x1]", 5), "n_number_hex_1_digit"},
    {std::string("[0x42]", 6), "n_number_hex_2_digits"},
    {std::string("[0e+-1]", 7), "n_number_invalid+-"},
    {std::string("[-123.123foo]", 13), "n_number_invalid-negative-real"},
    {std::string("[123\345]", 6), "n_number_invalid-utf-8-in-bigger-int"},
    {std::string("[1e1\345]", 6), "n_number_invalid-utf-8-in-exponent"},
    {std::string("[0\345]\n", 5), "n_number_invalid-utf-8-in-int"},
    {std::string("[-foo]", 6), "n_number_minus_sign_with_trailing_garbage"},
    {std::string("[- 1]", 5), "n_number_minus_space_1"},
    {std::string("[-012]", 6), "n_number_neg_int_starting_with_zero"},
    {std::string("[-.123]", 7), "n_number_neg_real_without_int_part"},
    {std::string("[-1x]", 5), "n_number_neg_with_garbage_at_end"},
    {std::string("[1ea]", 5), "n_number_real_garbage_after_e"},
    {std::string("[1.]", 4), "n_number_real_without_fractional_part"},
    {std::string("[1e\345]", 5), "n_number_real_with_invalid_utf8_after_e"},
    {std::string("[.123]", 6), "n_number_starting_with_dot"},
    {std::string("[\357\274\221]", 5), "n_number_U+FF11_fullwidth_digit_one"},
    {std::string("[1.2a-3]", 8), "n_number_with_alpha"},
    {std::string("[1.8011670033376514H-308]", 25), "n_number_with_alpha_char"},
    {std::string("[012]", 5), "n_number_with_leading_zero"},
    {std::string("[\"x\", truth]", 12), "n_object_bad_value"},
    {std::string("{[: \"x\"}\n", 9), "n_object_bracket_key"},
    {std::string("{\"x\", null}", 11), "n_object_comma_instead_of_colon"},
    {std::string("{\"x\"::\"b\"}", 10), "n_object_double_colon"},
    {std::string("{\360\237\207\250\360\237\207\255}", 10), "n_object_emoji"},
    {std::string("{\"a\":\"a\" 123}", 13), "n_object_garbage_at_end"},
    {std::string("{key: \'value\'}", 14), "n_object_key_with_single_quotes"},
    {std::string("{\"a\" b}", 7), "n_object_missing_colon"},
    {std::string("{:\"b\"}", 6), "n_object_missing_key"},
    {std::string("{\"a\" \"b\"}", 9), "n_object_missing_semicolon"},
    {std::string("{\"a\":", 5), "n_object_missing_value"},
    {std::string("{\"a\"", 4), "n_object_no-colon"},
    {std::string("{1:1}", 5), "n_object_non_string_key"},
    {std::string("{9999E9999:1}", 13), "n_object_non_string_key_but_huge_number_instead"},
    {std::string("{\"\271\":\"0\",}", 10), "n_object_pi_in_key_and_trailing_comma"},
    {std::string("{null:null,null:null}", 21), "n_object_repeated_null_null"},
    {std::string("{\"id\":0,,,,,}", 13), "n_object_several_trailing_commas"},
    {std::string("{\'a\':0}", 7), "n_object_single_quote"},
    {std::string("{\"id\":0,}", 9), "n_object_trailing_comma"},
    {std::string("{\"a\":\"b\"}/**/", 13), "n_object_trailing_comment"},
    {std::string("{\"a\":\"b\"}/**//", 14), "n_object_trailing_comment_open"},
    {std::string("{\"a\":\"b\"}//", 11), "n_object_trailing_comment_slash_open"},
    {std::string("{\"a\":\"b\"}/", 10), "n_object_trailing_comment_slash_open_incomplete"},
    {std::string("{\"a\":\"b\",,\"c\":\"d\"}", 18), "n_object_two_commas_in_a_row"},
    {std::string("{a: \"b\"}", 8), "n_object_unquoted_key"},
    {std::string("{\"a\":\"a", 7), "n_object_unterminated-value"},
    {std::string("{ \"foo\" : \"bar\", \"a\" }", 22), "n_object_with_single_string"},
    {std::string("{\"a\":\"b\"}#", 10), "n_object_with_trailing_garbage"},
    {std::string(" ", 1), "n_single_space"},
    {std::string("[\"\\uD800\\\"]", 11), "n_string_1_surrogate_then_escape"},
    {std::string("[\"\\uD800\\u\"]", 12), "n_string_1_surrogate_then_escape_u"},
    {std::string("[\"\\uD800\\u1\"]", 13), "n_string_1_surrogate_then_escape_u1"},
    {std::string("[\"\\uD800\\u1x\"]", 14), "n_string_1_surrogate_then_escape_u1x"},
    {std::string("[\303\251]", 4), "n_string_accentuated_char_no_quotes"},
    {std::string("[\"\\\000\"]", 6), "n_string_backslash_00"},
    {std::string("[\"\\\\\\\"]", 7), "n_string_escaped_backslash_bad"},
    {std::string("[\"\\\t\"]", 6), "n_string_escaped_ctrl_char_tab"},
    {std::string("[\"\\\360\237\214\200\"]", 9), "n_string_escaped_emoji"},
    {std::string("[\"\\x00\"]", 8), "n_string_escape_x"},
    {std::string("[\"\\\"]", 5), "n_string_incomplete_escape"},
    {std::string("[\"\\u00A\"]", 9), "n_string_incomplete_escaped_character"},
    {std::string("[\"\\uD834\\uDd\"]", 14), "n_string_incomplete_surrogate"},
    {std::string("[\"\\uD800\\uD800\\x\"]", 18), "n_string_incomplete_surrogate_escape_invalid"},
    {std::string("[\"\\u\345\"]", 7), "n_string_invalid-utf-8-in-escape"},
    {std::string("[\"\\a\"]", 6), "n_string_invalid_backslash_esc"},
    {std::string("[\"\\uqqqq\"]", 10), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\\345\"]", 6), "n_string_invalid_utf8_after_escape"},
    {std::string("[\\u0020\"asd\"]", 13), "n_string_leading_uescaped_thinspace"},
    {std::string("[\\n]", 4), "n_string_no_quotes_with_bad_escape"},
    {std::string("\"", 1), "n_string_single_doublequote"},
    {std::string("[\'single quote\']", 16), "n_string_single_quote"},
    {std::string("abc", 3), "n_string_single_string_no_double_quotes"},
    {std::string("[\"\\", 3), "n_string_start_escape_unclosed"},
    {std::string("[\"a\000a\"]", 7), "n_string_unescaped_crtl_char"},
    {std::string("[\"new\nline\"]", 12), "n_string_unescaped_newline"},
    {std::string("[\"\t\"]", 5), "n_string_unescaped_tab"},
    {std::string("\"\\UA66D\"", 8), "n_string_unicode_CapitalU"},
    {std::string("\"\"x", 3), "n_string_with_trailing_garbage"},
    {std::string("<.>", 3), "n_structure_angle_bracket_."},
    {std::string("[<null>]", 8), "n_structure_angle_bracket_null"},
    {std::string("[1]x", 4), "n_structure_array_trailing_garbage"},
    {std::string("[1]]", 4), "n_structure_array_with_extra_array_close"},
    {std::string("[\"asd]", 6), "n_structure_array_with_unclosed_string"},
    {std::string("a\303\245", 3), "n_structure_ascii-unicode-identifier"},
    {std::string("[True]", 6), "n_structure_capitalized_True"},
    {std::string("1]", 2), "n_structure_close_unopened_array"},
    {std::string("{\"x\": true,", 11), "n_structure_comma_instead_of_closing_brace"},
    {std::string("[][]", 4), "n_structure_double_array"},
    {std::string("]", 1), "n_structure_end_array"},
    {std::string("\357\273{}", 4), "n_structure_incomplete_UTF8_BOM"},
    {std::string("\345", 1), "n_structure_lone-invalid-utf-8"},
    {std::string("[", 1), "n_structure_lone-open-bracket"},
    {std::string("", 0), "n_structure_no_data"},
    {std::string("[\000]", 3), "n_structure_null-byte-outside-string"},
    {std::string("2@", 2), "n_structure_number_with_trailing_garbage"},
    {std::string("{}}", 3), "n_structure_object_followed_by_closing_object"},
    {std::string("{\"\":", 4), "n_structure_object_unclosed_no_value"},
    {std::string("{\"a\":/*comment*/\"b\"}", 20), "n_structure_object_with_comment"},
    {std::string("{\"a\": true} \"x\"", 15), "n_structure_object_with_trailing_garbage"},
    {std::string("[\'", 2), "n_structure_open_array_apostrophe"},
    {std::string("[,", 2), "n_structure_open_array_comma"},
    {std::string("[{", 2), "n_structure_open_array_open_object"},
    {std::string("[\"a", 3), "n_structure_open_array_open_string"},
    {std::string("[\"a\"", 4), "n_structure_open_array_string"},
    {std::string("{", 1), "n_structure_open_object"},
    {std::string("{]", 2), "n_structure_open_object_close_array"},
    {std::string("{,", 2), "n_structure_open_object_comma"},
    {std::string("{[", 2), "n_structure_open_object_open_array"},
    {std::string("{\"a", 3), "n_structure_open_object_open_string"},
    {std::string("{\'a\'", 4), "n_structure_open_object_string_with_apostrophes"},
    {std::string("[\"\\{[\"\\{[\"\\{[\"\\{", 16), "n_structure_open_open"},
    {std::string("\351", 1), "n_structure_single_eacute"},
    {std::string("*", 1), "n_structure_single_star"},
    {std::string("{\"a\":\"b\"}#{}", 12), "n_structure_trailing_#"},
    {std::string("[\342\201\240]", 5), "n_structure_U+2060_word_joined"},
    {std::string("[\\u000A\"\"]", 10), "n_structure_uescaped_LF_before_string"},
    {std::string("[1", 2), "n_structure_unclosed_array"},
    {std::string("[ false, nul", 12), "n_structure_unclosed_array_partial_null"},
    {std::string("[ true, fals", 12), "n_structure_unclosed_array_unfinished_false"},
    {std::string("[ false, tru", 12), "n_structure_unclosed_array_unfinished_true"},
    {std::string("{\"asd\":\"asd\"", 12), "n_structure_unclosed_object"},
    {std::string("\303\245", 2), "n_structure_unicode-identifier"},
    {std::string("\357\273\277", 3), "n_structure_UTF8_BOM_no_data"},
    {std::string("[\f]", 3), "n_structure_whitespace_formfeed"},
    {std::string("[\342\201\240]", 5), "n_structure_whitespace_U+2060_word_joiner"},
};

TEST_CASE("JSONTestSuite")
{
    SECTION("pass")
    {
        for (auto const& test : kJSONTestSuitePass)
        {
            CAPTURE(test.name);

            json::Value val;
            auto const res = json::parse(val, test.input.data(), test.input.data() + test.input.size());
            CHECK(res.ec == json::ErrorCode::success);
        }
    }

    SECTION("fail")
    {
        for (auto const& test : kJSONTestSuiteFail)
        {
            CAPTURE(test.name);

            json::ParseOptions options;
            //options.allow_comments = true;

            json::Value val;
            auto const res = json::parse(val, test.input.data(), test.input.data() + test.input.size(), options);
            CHECK(res.ec != json::ErrorCode::success);
        }
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

struct StringTest
{
    std::string inp;
    std::string expected;
};

static const StringTest kTestStrings[] = {
    {"[\"\"]", ""},
    {"[\"Hello\"]", "Hello"},
    {"[\"Hello\\nWorld\"]", "Hello\nWorld"},
    {std::string("[\"Hello\\u0000World\"]", 20), std::string("Hello\0World", 11)},
    {"[\"\\\"\\\\/\\b\\f\\n\\r\\t\"]", "\"\\/\b\f\n\r\t"},
    {"[\"\\u0024\"]", "\x24"},
    {"[\"\\u00A2\"]", "\xC2\xA2"},
    {"[\"\\u20AC\"]", "\xE2\x82\xAC"},
    {"[\"\\uD834\\uDD1E\"]", "\xF0\x9D\x84\x9E"},
};

TEST_CASE("Parse_string")
{
    for (auto const& test : kTestStrings)
    {
        json::Value val1;
        auto const res1 = json::parse(val1, test.inp.data(), test.inp.data() + test.inp.size());
        CHECK(res1.ec == json::ErrorCode::success);
        CHECK(val1.is_array());
        CHECK(val1.as_array().size() == 1);
        CHECK(val1[0].is_string());
        CHECK(val1[0].as_string() == test.expected);

        std::string s;
        auto const str_ok = json::stringify(s, val1);
        CHECK(str_ok);

        json::Value val2;
        auto const res2 = json::parse(val2, s.data(), s.data() + s.size());
        CHECK(res2.ec == json::ErrorCode::success);
        CHECK(val2.is_array());
        CHECK(val2.as_array().size() == 1);
        CHECK(val2[0].is_string());
        CHECK(val2[0].as_string() == test.expected);
    }
}

TEST_CASE("Stringify")
{
    std::string const input = R"({
    "empty_array": [],
    "empty_object" : {},
    "FirstName": "John",
    "LastName": "Doe",
    "Age": 43,
    "Address": {
        "Street": "Downing \"Street\" 10",
        "City": "London",
        "Country": "Great Britain"
    },
    "Phone numbers": [
        "+44 1234567",
        "+44 2345678"
    ]
})";

    json::Value val;
    json::parse(val, input.data(), input.data() + input.size());

    json::StringifyOptions options;
    options.indent_width = 4;

    std::string str;
    json::stringify(str, val, options);
    //printf("%s\n", str.c_str());

    json::Value val2;
    json::parse(val2, str.data(), str.data() + str.size());

    CHECK(val == val2);
}

