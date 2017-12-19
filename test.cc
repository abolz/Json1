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
            // Could provide a specialization for empty/non-empty tuples...
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
            json::Value val;
            auto const res = json::parse(val, inp.data(), inp.data() + inp.size());
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

