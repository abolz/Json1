#if 1
#ifdef _MSC_VER
#pragma warning(disable: 4244)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif
#endif

#include "../src/json.h"
#include "../src/json_numbers.h"

#include "catch.hpp"

#include <tuple>
#include <limits>
#include <cstring>
#include <cmath>

template <typename T> void Unused(T&& /*unused*/) {}

namespace json {
    inline std::ostream& operator<<(std::ostream& os, ParseStatus ec) {
        return os << static_cast<int>(ec);
    }
}

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
        CHECK(j0.get_boolean() == true);

        json::Value j1 = false;
        CHECK(j1.is_boolean());
        CHECK(j1.get_boolean() == false);
    }

    SECTION("number")
    {
        json::Value j0 = static_cast<signed char>(-1);
        CHECK(j0.is_number());
        CHECK(j0.get_number() == -1.0);

        json::Value j1 = static_cast<signed short>(-1);
        CHECK(j1.is_number());
        CHECK(j1.get_number() == -1.0);

        json::Value j2 = -1;
        CHECK(j2.is_number());
        CHECK(j2.get_number() == -1.0);

        json::Value j3 = -1l; // (warning expected)
        CHECK(j3.is_number());
        CHECK(j3.get_number() == -1.0);

        json::Value j4 = -1ll; // warning expected
        CHECK(j4.is_number());
        CHECK(j4.get_number() == -1.0);

        json::Value j4_1(-1ll); // no warning expected
        CHECK(j4_1.is_number());
        CHECK(j4_1.get_number() == -1.0);

        json::Value j5 = static_cast<unsigned char>(1);
        CHECK(j5.is_number());
        CHECK(j5.get_number() == 1.0);

        json::Value j6 = static_cast<unsigned short>(1);
        CHECK(j6.is_number());
        CHECK(j6.get_number() == 1.0);

        json::Value j7 = 1u;
        CHECK(j7.is_number());
        CHECK(j7.get_number() == 1.0);

        json::Value j8 = 1ul; // (warning expected)
        CHECK(j8.is_number());
        CHECK(j8.get_number() == 1.0);

        json::Value j9 = 1ull; // warning expected
        CHECK(j9.is_number());
        CHECK(j9.get_number() == 1.0);

        json::Value j10 = 1.2;
        CHECK(j10.is_number());
        CHECK(j10.get_number() == 1.2);

#if JSON_VALUE_HAS_EXPLICIT_OPERATOR_T
        int i0 = static_cast<int>(j10); // warning expected (double -> int)
        CHECK(i0 == 1);
        int i1 = static_cast<float>(j10); // two warnings expected! (double -> float, float -> int)
        CHECK(i1 == 1);
        int i2 = static_cast<int>(static_cast<double>(j10)); // no warning
        CHECK(i2 == 1);
#endif

        int i3 = j10.as<int>(); // warning expected (double -> int)
        CHECK(i3 == 1);
        int i4 = j10.as<float>(); // two warnings expected! (double -> float, float -> int)
        CHECK(i4 == 1);
        int i5 = static_cast<int>(j10.as<double>()); // no warning
        CHECK(i5 == 1);

        json::Value j11 = 1.234f;
        CHECK(j11.is_number());
        CHECK(j11.get_number() == static_cast<double>(1.234f));
    }

    SECTION("string")
    {
        json::Value j0 = std::string("hello");
        CHECK(j0.is_string());
        CHECK(j0.get_string() == "hello");
#if 0
        // Should not compile!
        auto s0 = std::move(j0).as<char const*>();
#endif

        json::Value j1 = "hello";
        CHECK(j1.is_string());
        CHECK(j1.get_string() == "hello");

        //auto s1 = j1.to<char const*>();
        //CHECK(s1 == std::string("hello"));
        //auto s2 = j1.to<cxx::string_view>();
        //CHECK(s2 == "hello");

        json::Value j2 = const_cast<char*>("hello");
        CHECK(j2.is_string());
        CHECK(j2.get_string() == "hello");

        json::Value j3 = "hello";
        CHECK(j3.is_string());
        CHECK(j3.get_string() == "hello");

        //json::Value j4 = cxx::string_view("hello");
        //CHECK(j4.is_string());
        //CHECK(j4.get_string() == "hello");

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
    auto const res = json::parse(val, input.data(), input.data() + input.size());
    CHECK(res.ec == json::ParseStatus{});

    json::StringifyOptions sopt;
    sopt.indent_width = 4;

    std::string str;
    json::stringify(str, val, sopt);

    std::string str_expected = R"({
    "Address": {
        "City": "London",
        "Country": "Great Britain",
        "Street": "Downing \"Street\" 10"
    },
    "Age": 43,
    "FirstName": "John",
    "LastName": "Doe",
    "Phone numbers": [
        "+44 1234567",
        "+44 2345678"
    ],
    "empty_array": [],
    "empty_object": {}
})";
    CHECK(str == str_expected);

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

TEST_CASE("Parse")
{
    std::string const input = R"({
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

    CHECK(val.is_object());
    CHECK(val.get_object().size() == 5);
    CHECK(val.size() == 5);
    CHECK(val.has_member("FirstName"));
    CHECK(val["FirstName"].is_string());
    CHECK(val["FirstName"].get_string().size() == 4);
    CHECK(val["FirstName"].get_string() == "John");
    CHECK(val["FirstName"].size() == 4);
    CHECK(val["FirstName"] == "John");
    CHECK(val.has_member("LastName"));
    CHECK(val["LastName"].is_string());
    CHECK(val["LastName"].get_string().size() == 3);
    CHECK(val["LastName"].get_string() == "Doe");
    CHECK(val["LastName"].size() == 3);
    CHECK(val["LastName"] == "Doe");
    CHECK(val.has_member("Age"));
    CHECK(val["Age"].is_number());
    CHECK(val["Age"].get_number() == 43);
    CHECK(val["Age"] == 43);
    CHECK(43 == val["Age"]);
    CHECK(val["Age"] < 44);
    CHECK(42 < val["Age"]);
    CHECK(val.has_member("Address"));
    CHECK(val["Address"].is_object());
    CHECK(val["Address"].get_object().size() == 3);
    CHECK(val["Address"].size() == 3);
    CHECK(val["Address"].has_member("Street"));
    CHECK(val["Address"]["Street"].is_string());
    CHECK(val["Address"]["Street"].get_string().size() == 19);
    CHECK(val["Address"]["Street"].get_string() == "Downing \"Street\" 10");
    CHECK(val["Address"]["Street"].size() == 19);
    CHECK(val["Address"]["Street"] == "Downing \"Street\" 10");
    CHECK(val.has_member("Phone numbers"));
    CHECK(val["Phone numbers"].is_array());
    CHECK(val["Phone numbers"].get_array().size() == 2);
    CHECK(val["Phone numbers"][0].is_string());
    CHECK(val["Phone numbers"][0] == "+44 1234567");
    CHECK(val["Phone numbers"][1].is_string());
    CHECK(val["Phone numbers"][1] == "+44 2345678");

    std::string const input2 = R"({
    "LastName": "Doe",
    "Phone numbers": [
        "+44 1234567",
        "+44 2345678"
    ]
    "Age": 43,
    "Address": {
        "Country": "Great Britain"
        "City": "London",
        "Street": "Downing \"Street\" 10",
    },
    "FirstName": "John",
})";

    json::Value val2;
    json::parse(val2, input.data(), input.data() + input.size());

    CHECK(val == val2);
    CHECK(!(val != val2));
    CHECK(val.hash() == val2.hash());

    json::Value val3;
    val3 = val2;
    CHECK(val3 == val2);

    val3 = std::move(val2);
    CHECK(val3 == val);
    CHECK(val2.is_undefined()); //

    *(val.get_ptr("Address")->get_ptr("Street")) = "Hello World";
    CHECK(val["Address"]["Street"] == "Hello World");
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

#include <forward_list>
#include <iterator>
#include <list>
#include <set>

//namespace xxx {
//    struct S {};
//    template <> struct ::json::Traits<S> {};
//};

TEST_CASE("arrays")
{
    SECTION("json::Array")
    {
        json::Array a {1, "two", 3.3, true};
        json::Value j = a;
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 4);
        REQUIRE(j[0].is_number());
        REQUIRE(j[0].get_number() == 1);
        REQUIRE(j[1].is_string());
        REQUIRE(j[1].get_string() == "two");
        REQUIRE(j[2].is_number());
        REQUIRE(j[2].get_number() == 3.3);
        REQUIRE(j[3].is_boolean());
        REQUIRE(j[3].get_boolean() == true);
    }

    SECTION("std::list")
    {
        std::list<json::Value> a {1, "two", 3.3, true};
        json::TestConversionToJson<decltype(a)>{};
        json::Value j = a;//Value::from_array(a);
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 4);
        REQUIRE(j[0].is_number());
        REQUIRE(j[0].get_number() == 1);
        REQUIRE(j[1].is_string());
        REQUIRE(j[1].get_string() == "two");
        REQUIRE(j[2].is_number());
        REQUIRE(j[2].get_number() == 3.3);
        REQUIRE(j[3].is_boolean());
        REQUIRE(j[3].get_boolean() == true);
    }

    SECTION("std::forward_list")
    {
        std::forward_list<json::Value> a {1, "two", 3.3, true};
        //json::Value j = json::Value(json::Tag_array{}, a.begin(), a.end());
        json::Value j = {json::array_tag, a.begin(), a.end()};
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 4);
        REQUIRE(j[0].is_number());
        REQUIRE(j[0].get_number() == 1);
        REQUIRE(j[1].is_string());
        REQUIRE(j[1].get_string() == "two");
        REQUIRE(j[2].is_number());
        REQUIRE(j[2].get_number() == 3.3);
        REQUIRE(j[3].is_boolean());
        REQUIRE(j[3].get_boolean() == true);
    }

    SECTION("std::set")
    {
        std::set<json::Value> a {1, "two", 3.3, true};
        json::Value j = a;// Value::from_array(a);
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 4);
        // bool < number < string
        REQUIRE(j[0].is_boolean());
        REQUIRE(j[0].get_boolean() == true);
        REQUIRE(j[1].is_number());
        REQUIRE(j[1].get_number() == 1);
        REQUIRE(j[2].is_number());
        REQUIRE(j[2].get_number() == 3.3);
        REQUIRE(j[3].is_string());
        REQUIRE(j[3].get_string() == "two");
    }
}

#ifndef __clang__

namespace json
{
    template <>
    struct Traits<std::tuple<>>
    {
        using tag = Tag_array;
        template <typename V> static decltype(auto) to_json(V&&) { return Array(); }
        template <typename V> static decltype(auto) from_json(V&& in) {
            assert(in.get_array().empty());
            static_cast<void>(in); // may be unused
            return std::tuple<>{};
        }
    };
    template <typename T1, typename ...Tn>
    struct Traits<std::tuple<T1, Tn...>>
    {
        using tag = Tag_array;
        template <typename V> static decltype(auto) to_json(V&& in) // V = std::tuple<Tn...> [const][&]
        {
//          return Array{std::get<T1>(std::forward<V>(in)), std::get<Tn>(std::forward<V>(in))...};
            return Array{std::get<T1>(in), std::get<Tn>(in)...};
        }
        template <typename V> static decltype(auto) from_json(V&& in) // V = Value [const&]
        {
            assert(in.get_array().size() == 1 + sizeof...(Tn));
            auto I = std::make_move_iterator(in.get_array().begin()); // Does _not_ move for V = Value const&
            return std::tuple<T1, Tn...>{json::cast<T1>(*I), json::cast<Tn>(*++I)...};
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

        auto t = j.as<Tup>();
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
        REQUIRE(j[1].get_string() == "hello hello hello hello hello hello hello hello ");

        auto t2 = json::cast<Tup>(std::move(j));
        REQUIRE(std::get<0>(t2) == 2.34);
        REQUIRE(std::get<1>(t2) == "hello hello hello hello hello hello hello hello ");
#if 0//_MSC_VER // These tests probably work... XXX: Add a test type which has a distinguished moved-from state...
        REQUIRE(j[1].is_string());
        REQUIRE(j[1].get_string().empty());
#endif
    }
}

#endif

struct Point {
    double x;
    double y;
};
inline bool operator==(Point const& lhs, Point const& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}
inline bool operator<(Point const& lhs, Point const& rhs) {
    return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
}
namespace json
{
    template <>
    struct Traits<Point>
    {
        using tag = Tag_object;
        static decltype(auto) to_json(Point const& in) {
            Object out;
            out["x"] = in.x;
            out["y"] = in.y;
            return out;
        }
        static decltype(auto) from_json(Value const& in) {
            Point out;
            out.x = in["x"].get_number();
            out.y = in["y"].get_number();
            return out;
        }
    };
}
TEST_CASE("Value - custom")
{
    SECTION("val")
    {
        json::Value j = Point{1.0, 2.0};
        CHECK(j.is_object());
        CHECK(j.size() == 2);
        CHECK(j.has_member("x"));
        CHECK(j["x"].is_number());
        CHECK(j["x"] == 1.0);
        CHECK(j.has_member("y"));
        CHECK(j["y"].is_number());
        CHECK(j["y"] == 2.0);
    }

    //SECTION("array_x")
    //{
    //    std::vector<int*> v;
    //    json::TestConversionToJson<decltype(v)>{};
    //    json::Value j = v;
    //}

    SECTION("array")
    {
        std::vector<Point> points = {
            {1.0, 2.0},
            {3.0, 4.0},
        };
        json::Value j = points;
        CHECK(j.is_array());
        //CHECK(j.isa<std::vector<Point>>());
        CHECK(j.size() == 2);
        CHECK(j[0].is_object());
        //CHECK(j[0].isa<Point>());
        CHECK(j[0].size() == 2);
        CHECK(j[0].has_member("x"));
        CHECK(j[0]["x"] == 1.0);
        CHECK(j[0].has_member("y"));
        CHECK(j[0]["y"] == 2.0);
        CHECK(j[1].is_object());
        //CHECK(j[1].isa<Point>());
        CHECK(j[1].size() == 2);
        CHECK(j[1].has_member("x"));
        CHECK(j[1]["x"] == 3.0);
        CHECK(j[1].has_member("y"));
        CHECK(j[1]["y"] == 4.0);
        auto p0 = j[0].as<Point>();
        CHECK(p0.x == 1.0);
        CHECK(p0.y == 2.0);
        auto p1 = j[1].as<Point>();
        CHECK(p1.x == 3.0);
        CHECK(p1.y == 4.0);
        auto pts = j.as<std::vector<Point>>();
        CHECK(pts.size() == 2);
        CHECK(pts[0].x == 1.0);
        CHECK(pts[0].y == 2.0);
        CHECK(pts[1].x == 3.0);
        CHECK(pts[1].y == 4.0);
    }

#if 0
    SECTION("object")
    {
        std::map<int, Point> points{
            {1, {1.0, 2.0}},
            {2, {3.0, 4.0}},
        };
        json::Value j = points;
        CHECK(j.is_object());
        CHECK(j.size() == 2);
        CHECK(j.has_member("1"));
        CHECK(j["1"].is_object());
        CHECK(j["1"].has_member("x"));
        CHECK(j["1"]["x"] == 1.0);
        CHECK(j["1"]["y"] == 2.0);
        CHECK(j.has_member("2"));
        CHECK(j["2"].is_object());
        CHECK(j["2"].has_member("x"));
        CHECK(j["2"]["x"] == 3.0);
        CHECK(j["2"]["y"] == 4.0);
        auto pts = j.as<std::map<json::Value, json::Value>>();
        CHECK(pts.size() == 2);
        CHECK(pts["1"].is_object());
        CHECK(pts["1"].has_member("x"));
        CHECK(pts["1"]["x"] == 1.0);
        CHECK(pts["1"]["y"] == 2.0);
        CHECK(pts["2"].is_object());
        CHECK(pts["2"].has_member("x"));
        CHECK(pts["2"]["x"] == 3.0);
        CHECK(pts["2"]["y"] == 4.0);
        auto pt2 = j.as<std::map<int, Point>>();
        CHECK(pt2.size() == 2);
        CHECK(pt2[1].x == 1.0);
        CHECK(pt2[1].y == 2.0);
        CHECK(pt2[2].x == 3.0);
        CHECK(pt2[2].y == 4.0);
    }
#endif
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
        using tag = TagFor<T>;

        template <typename V>
        static Value to_json(V&& in) // V = std::optional<T> [const][&]
        {
            if (!in.has_value())
                return json::undefined_tag;

            return TraitsFor<T>::to_json(std::forward<V>(in).value());
        }

        template <typename V>
        static std::optional<T> from_json(V&& in) // V = Value [const][&]
        {
            if (/*in.is_undefined() ||*/ in.type() != tag::value)
                return std::nullopt;
#if 1
            return TraitsFor<T>::from_json(std::forward<V>(in));
#else
            return static_cast<T>( TraitsFor<TargetTypeFor<T>>::from_json(std::forward<V>(in)) );
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
        CHECK(j0.is_undefined());

        std::optional<int> oi1 = 123;
        CHECK(oi1.has_value());
        CHECK(oi1.value() == 123);

        json::Value j1 = oi1;
        CHECK(j1.is_number());
        CHECK(j1.get_number() == 123);

        json::Value j2;
        j2 = oi0;
        CHECK(j2.is_undefined());

        json::Value j3;
        j3 = oi1;
        CHECK(j3.is_number());
        CHECK(j3.get_number() == 123);
    }

    SECTION("Value -> optional")
    {
        json::Value j0 = 123;
        CHECK(j0.is_number());
        CHECK(j0.get_number() == 123);

        auto oi0 = j0.as<std::optional<int>>(); // warning expected -- XXX: need a way to silence this warning...
        CHECK(oi0.has_value());
        CHECK(oi0.value() == 123);

        auto oi1 = j0.as<std::optional<std::string>>();
        CHECK(!oi1.has_value());

#if JSON_VALUE_HAS_EXPLICIT_OPERATOR_T
        // This is the tricky part...
        // optional<T>'s _implicit_ constructor is preferred over Value's _explicit_ conversion operator.
        // The implicit constructor then calls 'explicit operator T' (not 'explicit operator std::optional<T>').

        // This works,
        // since Value's 'explicit operator int' is called, which calls get_number() and j0 actually contains a number.
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
        CAPTURE(test.inp);

        json::Value val1;
        auto const res1 = json::parse(val1, test.inp.data(), test.inp.data() + test.inp.size());
        CHECK(res1.ec == json::ParseStatus::success);
        CHECK(val1.is_array());
        CHECK(val1.get_array().size() == 1);
        CHECK(val1[0].is_string());
        CHECK(val1[0].get_string() == test.expected);

        std::string s;
        auto const str_ok = json::stringify(s, val1);
        CHECK(str_ok);

        json::Value val2;
        auto const res2 = json::parse(val2, s.data(), s.data() + s.size());
        CHECK(res2.ec == json::ParseStatus::success);
        CHECK(val2.is_array());
        CHECK(val2.get_array().size() == 1);
        CHECK(val2[0].is_string());
        CHECK(val2[0].get_string() == test.expected);
    }
}

TEST_CASE("Invalid/incomplete strings")
{
    static const std::string kStrings[] = {
        "[\"0123456789ABCDEF",
    };

    for (auto const& s : kStrings) {
        json::Value j;
        auto const ec = json::parse(j, s);
        CHECK(ec != json::ParseStatus::success);
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

TEST_CASE("Conversion")
{
    SECTION("ToBoolean")
    {
        json::Value copy;

        json::Value j1(json::Type::null);
        CHECK(false == j1.to_boolean());
        j1 = j1.to_boolean();
        copy = j1;
        CHECK(copy == j1);
        CHECK(!(copy != j1));
        CHECK(!(copy < j1));
        CHECK(copy <= j1);
        CHECK(!(copy > j1));
        CHECK(copy >= j1);

        json::Value j2(json::Type::boolean);
        CHECK(false == j2.to_boolean());
        j2 = true;
        CHECK(true == j2.to_boolean());
        copy = j2;
        CHECK(copy == j2);
        CHECK(!(copy != j2));
        CHECK(!(copy < j2));
        CHECK(copy <= j2);
        CHECK(!(copy > j2));
        CHECK(copy >= j2);

        json::Value j3(json::Type::number); // !nan(x) && x != 0
        CHECK(false == j3.to_boolean());
        j3 = 1.0;
        CHECK(true == j3.to_boolean());
        j3 = -std::numeric_limits<double>::quiet_NaN();
        CHECK(false == j3.to_boolean());
        j3 = std::numeric_limits<double>::infinity();
        CHECK(true == j3.to_boolean());
        j3 = j3.to_boolean();
        copy = j3;
        CHECK(copy == j3);
        CHECK(!(copy != j3));
        CHECK(!(copy < j3));
        CHECK(copy <= j3);
        CHECK(!(copy > j3));
        CHECK(copy >= j3);

        json::Value j4(json::Type::string); // !empty(x)
        CHECK(false == j4.to_boolean());
        j4 = "hello";
        CHECK(true == j4.to_boolean());
        j4 = j4.to_boolean();
        copy = j4;
        CHECK(copy == j4);
        CHECK(!(copy != j4));
        CHECK(!(copy < j4));
        CHECK(copy <= j4);
        CHECK(!(copy > j4));
        CHECK(copy >= j4);
    }

    SECTION("ToNumber")
    {
        json::Value copy;

        json::Value j1(json::Type::null);
        CHECK(0.0 == j1.to_number());
        j1 = j1.to_number();
        copy = j1;
        CHECK(copy == j1);
        CHECK(!(copy != j1));
        CHECK(!(copy < j1));
        CHECK(copy <= j1);
        CHECK(!(copy > j1));
        CHECK(copy >= j1);

        json::Value j2(json::Type::boolean);
        CHECK(0.0 == j2.to_number());
        j2 = true;
        CHECK(1.0 == j2.to_number());
        j2 = j2.to_number();
        copy = j2;
        CHECK(copy == j2);
        CHECK(!(copy != j2));
        CHECK(!(copy < j2));
        CHECK(copy <= j2);
        CHECK(!(copy > j2));
        CHECK(copy >= j2);

        json::Value j3(json::Type::number);
        CHECK(0.0 == j3.to_number());
        j3 = 1.0;
        CHECK(1.0 == j3.to_number());
        j3 = -std::numeric_limits<double>::quiet_NaN();
        CHECK(std::isnan(j3.to_number()));
        j3 = +std::numeric_limits<double>::quiet_NaN();
        CHECK(std::isnan(j3.to_number()));
        j3 = std::numeric_limits<double>::infinity();
        CHECK(std::numeric_limits<double>::infinity() == j3.to_number());
        j3 = j3.to_number();
        copy = j3;
        CHECK(copy == j3);
        CHECK(!(copy != j3));
        CHECK(!(copy < j3));
        CHECK(copy <= j3);
        CHECK(!(copy > j3));
        CHECK(copy >= j3);

        json::Value j4(json::Type::string); // !empty(x)
        CHECK(0.0 == j4.to_number());
        j4 = "1.0";
        CHECK(1.0 == j4.to_number());
        j4 = j4.to_number();
        CHECK(1.0 == j4.get_number());
        copy = j4;
        CHECK(copy == j4);
        CHECK(!(copy != j4));
        CHECK(!(copy < j4));
        CHECK(copy <= j4);
        CHECK(!(copy > j4));
        CHECK(copy >= j4);
    }

    SECTION("ToString")
    {
        json::Value copy;

        json::Value j1(json::Type::null);
        CHECK("null" == j1.to_string());
        j1 = j1.to_string();
        CHECK("null" == j1.get_string());
        copy = j1;
        CHECK(copy == j1);
        CHECK(!(copy != j1));
        CHECK(!(copy < j1));
        CHECK(copy <= j1);
        CHECK(!(copy > j1));
        CHECK(copy >= j1);

        json::Value j2(json::Type::boolean);
        CHECK("false" == j2.to_string());
        j2 = true;
        CHECK("true" == j2.to_string());
        j2 = j2.to_string();
        CHECK("true" == j2.get_string());
        copy = j2;
        CHECK(copy == j2);
        CHECK(!(copy != j2));
        CHECK(!(copy < j2));
        CHECK(copy <= j2);
        CHECK(!(copy > j2));
        CHECK(copy >= j2);

        json::Value j3(json::Type::number);
        CHECK("0" == j3.to_string());
        j3 = 1.0;
        CHECK("1" == j3.to_string());
        j3 = j3.to_string();
        CHECK("1" == j3.get_string());
        copy = j3;
        CHECK(copy == j3);
        CHECK(!(copy != j3));
        CHECK(!(copy < j3));
        CHECK(copy <= j3);
        CHECK(!(copy > j3));
        CHECK(copy >= j3);

        json::Value j4(json::Type::string);
        CHECK("" == j4.to_string());
        j4 = "hello";
        CHECK("hello" == j4.to_string());
        j4 = j4.to_string();
        CHECK("hello" == j4.get_string());
        copy = j4;
        CHECK(copy == j4);
        CHECK(!(copy != j4));
        CHECK(!(copy < j4));
        CHECK(copy <= j4);
        CHECK(!(copy > j4));
        CHECK(copy >= j4);
    }

    SECTION("ToUint32")
    {
        auto CheckIt = [](double x, uint32_t expected) {
            CAPTURE(x);
            CAPTURE(expected);
            auto const u1 = json::Value(x).to_uint32();
            CHECK(u1 == expected);
            // The ToUint32 abstract operation is idempotent:
            // if applied to a result that it produced, the second application leaves that value unchanged.
            auto const u2 = json::Value(u1).to_uint32();
            CHECK(u1 == u2);
            // ToUint32(ToInt32(x)) is equal to ToUint32(x) for all values of x.
            // (It is to preserve this latter property that +inf and -inf are mapped to +0.)
            auto const u3 = json::Value(json::Value(x).to_int32()).to_uint32();
            CHECK(u1 == u3);
        };

        CheckIt(         -0.0,          0u);
        CheckIt(          0.0,          0u);
        CheckIt(-2147483649.0, 2147483647u);
        CheckIt(-2147483648.0, 2147483648u);
        CheckIt(-2147483647.0, 2147483649u);
        CheckIt( 2147483647.0, 2147483647u);
        CheckIt( 2147483648.0, 2147483648u);
        CheckIt( 4294967295.0, 4294967295u);
        CheckIt( 4294967296.0,          0u);
        CheckIt( 4294967297.0,          1u);
        CheckIt( std::numeric_limits<double>::quiet_NaN(), 0u);
        CheckIt( std::numeric_limits<double>::infinity(), 0u);
        CheckIt(-std::numeric_limits<double>::infinity(), 0u);
    }

    SECTION("ToInt32")
    {
        auto CheckIt = [](double x, int32_t expected) {
            CAPTURE(x);
            CAPTURE(expected);
            auto const i1 = json::Value(x).to_int32();
            CHECK(i1 == expected);
            // The ToInt32 abstract operation is idempotent:
            // if applied to a result that it produced, the second application leaves that value unchanged.
            auto const i2 = json::Value(i1).to_int32();
            CHECK(i1 == i2);
            // ToInt32(ToUint32(x)) is equal to ToInt32(x) for all values of x.
            // (It is to preserve this latter property that +inf and -inf are mapped to +0.)
            auto const i3 = json::Value(json::Value(x).to_uint32()).to_int32();
            CHECK(i1 == i3);
        };

        CheckIt(         -0.0,           0);
        CheckIt(          0.0,           0);
        CheckIt(-2147483649.0,  2147483647);
        CheckIt(-2147483648.0, -2147483647 - 1);
        CheckIt(-2147483647.0, -2147483647);
        CheckIt( 2147483647.0,  2147483647);
        CheckIt( 2147483648.0, -2147483647 - 1);
        CheckIt( 4294967295.0,          -1);
        CheckIt( 4294967296.0,           0);
        CheckIt( 4294967297.0,           1);
        CheckIt( std::numeric_limits<double>::quiet_NaN(), 0);
        CheckIt( std::numeric_limits<double>::infinity(), 0);
        CheckIt(-std::numeric_limits<double>::infinity(), 0);
    }
}

TEST_CASE("Value - array op")
{
    json::Value j;
    CHECK(j.is_undefined());

    j.push_back(1);
    CHECK(j.is_array());
    CHECK(j.get_array().size() == 1);
    CHECK(j[0].is_number());
    CHECK(j[0] == 1);

    j.push_back(2);
    j.push_back(3);
    CHECK(j.is_array());
    CHECK(j.get_array().size() == 3);
    CHECK(j[0].is_number());
    CHECK(j[0] == 1);
    CHECK(j[1].is_number());
    CHECK(j[1] == 2);
    CHECK(j[2].is_number());
    CHECK(j[2] == 3);

    j.pop_back();
    CHECK(j.is_array());
    CHECK(j.get_array().size() == 2);
    CHECK(j[0].is_number());
    CHECK(j[0] == 1);
    CHECK(j[1].is_number());
    CHECK(j[1] == 2);
    CHECK(j.get_ptr(0) != nullptr);
    CHECK(j.get_ptr(1) != nullptr);
    CHECK(j.get_ptr(2) == nullptr);
    CHECK(j.get_ptr(0)->is_number());
    CHECK(j.get_ptr(1)->is_number());
    CHECK(*j.get_ptr(0) == 1);
    CHECK(*j.get_ptr(1) == 2);

    j.emplace_back("Hello");
    CHECK(j.is_array());
    CHECK(j.size() == 3);
    CHECK(j[2].is_string());
    CHECK(j[2] == "Hello");

    const json::Value j2 = j;

    j.erase(0);
    CHECK(j.is_array());
    CHECK(j.size() == 2);
    CHECK(j[0].is_number());
    CHECK(j[0].get_number() == 2);
    CHECK(j[0] == 2);
    CHECK(j[1].is_string());
    CHECK(j[1].size() == 5);
    CHECK(j[1].get_string() == "Hello");
    CHECK(j[1] == "Hello");

    CHECK(j2.is_array());
    CHECK(j2.get_array().size() == 3);
    CHECK(j2.size() == 3);
    CHECK(j2[0].is_number());
    CHECK(j2[0] == 1);
    CHECK(j2[1].is_number());
    CHECK(j2[1] == 2);
    CHECK(j2[2].is_string());
    CHECK(j2[2] == "Hello");
    CHECK(j2.get_ptr(0) != nullptr);
    CHECK(j2.get_ptr(1) != nullptr);
    CHECK(j2.get_ptr(2) != nullptr);
    CHECK(j2.get_ptr(3) == nullptr);
    CHECK(j2.get_ptr(0)->is_number());
    CHECK(j2.get_ptr(1)->is_number());
    CHECK(j2.get_ptr(2)->is_string());
    CHECK(*j2.get_ptr(0) == 1);
    CHECK(*j2.get_ptr(1) == 2);
    CHECK((*j2.get_ptr(2)).get_string() == "Hello");
}

TEST_CASE("Value - object op")
{
    json::Value j;
    CHECK(j.is_undefined());

    j["eins"] = 1;
    CHECK(j.is_object());
    CHECK(j.size() == 1);
    CHECK(j.has_member("eins"));
    CHECK(j["eins"].is_number());
    CHECK(j["eins"].get_number() == 1);
    CHECK(j["eins"] == 1);

    j["zwei"] = "zwei";
    CHECK(j.is_object());
    CHECK(j.size() == 2);
    CHECK(j.has_member("zwei"));
    CHECK(j.get_ptr("zwei") != nullptr);
    CHECK(j.get_ptr("zwei")->is_string());
    CHECK(j.get_ptr("zwei")->get_string() == "zwei");
    CHECK(*j.get_ptr("zwei") == "zwei");

    j.emplace("drei", 333);
    CHECK(j.is_object());
    CHECK(j.size() == 3);
    CHECK(j.has_member("drei"));
    CHECK(j["drei"].is_number());
    CHECK(j.get_ptr("drei") != nullptr);
    CHECK(j.get_ptr("drei")->get_number() == 333);
    CHECK(*j.get_ptr("drei") == 333);

    const auto j3 = j;

    const auto n1 = j.erase("zwei");
    CHECK(j.is_object());
    CHECK(j.size() == 2);
    CHECK(n1 == 1);
    CHECK(j.has_member("eins"));
    CHECK(!j.has_member("zwei"));
    CHECK(j.has_member("drei"));

    auto j2 = j;

    const auto n2 = j.erase("zwei");
    CHECK(n2 == 0);
    CHECK(j == j2);

    j2.erase(std::next(j2.items_begin()));
    CHECK(j2.is_object());
    CHECK(j2.size() == 1);
    CHECK(j2.has_member("drei"));
    CHECK(!j2.has_member("eins"));
    CHECK(j2["drei"] == 333);

    j.erase(j.items_begin(), j.items_end());
    CHECK(j.is_object());
    CHECK(j.empty());
    CHECK(j.size() == 0);

    CHECK(j3.is_object());
    CHECK(j3.size() == 3);
    CHECK(j3.has_member("eins"));
    CHECK(j3.has_member("zwei"));
    CHECK(j3.has_member("drei"));
    CHECK(j3["eins"].is_number());
    CHECK(j3["eins"].get_number() == 1);
    CHECK(j3["eins"] == 1);
    CHECK(j3.get_ptr("zwei") != nullptr);
    CHECK(j3.get_ptr("zwei")->is_string());
    CHECK(j3.get_ptr("zwei")->get_string() == "zwei");
    CHECK(*j3.get_ptr("zwei") == "zwei");
    CHECK(j3["drei"].is_number());
    CHECK(j3.get_ptr("drei") != nullptr);
    CHECK(j3.get_ptr("drei")->get_number() == 333);
    CHECK(*j3.get_ptr("drei") == 333);
    CHECK(j3.get_ptr("Zwei") == nullptr);
}

TEST_CASE("Iterators")
{
    SECTION("array")
    {
    }

    SECTION("object items")
    {
        json::Value j = { json::object_tag, {{"a", 1}, {"b", 2}, {"c", 3}} };
        CHECK(j.is_object());
        CHECK(j.size() == 3);

        auto I = j.items_begin();
        auto E = j.items_end();
        CHECK(I != E);
        CHECK(I->first == "a");
        CHECK(I->second == 1);
        ++I;
        CHECK(I != E);
        CHECK(I->first == "b");
        CHECK(I->second == 2);
        ++I;
        CHECK(I != E);
        CHECK(I->first == "c");
        CHECK(I->second == 3);
        ++I;
        CHECK(I == E);
    }

    SECTION("object keys")
    {
        json::Value j = json::Object{{"a", 1}, {"b", 2}, {"c", 3}};
        CHECK(j.is_object());
        CHECK(j.size() == 3);

        auto I = j.keys_begin();
        auto E = j.keys_end();
        CHECK(I != E);
        CHECK(*I == "a");
        ++I;
        CHECK(I != E);
        CHECK(*I == "b");
        ++I;
        CHECK(I != E);
        CHECK(*I == "c");
        ++I;
        CHECK(I == E);

        json::Value j2 = json::Array(j.keys_begin(), j.keys_end());
        CHECK(j2.is_array());
        CHECK(j2.size() == 3);
        CHECK(j2[0] == "a");
        CHECK(j2[1] == "b");
        CHECK(j2[2] == "c");
    }

    SECTION("object values")
    {
        json::Value j = json::Object{{"a", 1}, {"b", 2}, {"c", 3}};
        CHECK(j.is_object());
        CHECK(j.size() == 3);

        auto I = j.values_begin();
        auto E = j.values_end();
        CHECK(I != E);
        CHECK(*I == 1);
        ++I;
        CHECK(I != E);
        CHECK(*I == 2);
        ++I;
        CHECK(I != E);
        CHECK(*I == 3);
        ++I;
        CHECK(I == E);

        json::Value j2 = json::Array(j.values_begin(), j.values_end());
        CHECK(j2.is_array());
        CHECK(j2.size() == 3);
        CHECK(j2[0] == 1);
        CHECK(j2[1] == 2);
        CHECK(j2[2] == 3);

        json::Value const j3 = j;
        auto I3 = j3.values_begin();
        auto E3 = j3.values_end();
        I3 = I;
        E3 = E;
        //I = I3;
        //E = E3;
        CHECK(I3 == E);
        CHECK(I3 == E3);
        CHECK(I == E);
        CHECK(I == E3);
        CHECK(E == I3);
        CHECK(E3 == I3);
        CHECK(E == I);
        CHECK(E3 == I);
    }
}

TEST_CASE("as")
{
    json::Value j;

    j = 1.23;
    CHECK(j.as<double>() == 1.23);
    j.as<double&>() = 2.34;
    CHECK(j.as<double const&>() == 2.34);
    //j.as<json::String&>() = "hello";
    //j.as<int&>() = 3.45;
    //j.as<int>() = 3;
    auto x = j.as<int>();
    Unused(x);
}

#if JSON_VALUE_ALLOW_UNDEFINED_ACCESS
TEST_CASE("undefined")
{
    SECTION("array 1")
    {
        const json::Value j{};
        const auto& x1 = j[100];
        CHECK(x1.is_undefined());
        const auto& x2 = j[100][200];
        CHECK(x2.is_undefined());
    }

    SECTION("array 2")
    {
        const json::Value j = json::Value(json::array_tag);
        CHECK(j.is_array());
        CHECK(j.empty());
        const auto& x1 = j[100];
        CHECK(x1.is_undefined());
        const auto& x2 = j[100][200];
        CHECK(x2.is_undefined());
    }

    SECTION("array 3")
    {
        const json::Value j = json::Value(json::array_tag, size_t(101), json::Value(999));
        CHECK(j.is_array());
        CHECK(j.size() == 101);
        const auto& x1 = j[100];
        CHECK(x1.is_number());
        CHECK(x1 == 999);
        //const auto& x2 = j[100][200];
        //CHECK(x2.is_undefined());
    }

    SECTION("object 1")
    {
        const json::Value j{};
        CHECK(j.is_undefined());
        const auto& x1 = j["eins"];
        CHECK(j.is_undefined());
        CHECK(x1.is_undefined());
        const auto& x2 = j["eins"]["zwei"];
        CHECK(j.is_undefined());
        CHECK(x2.is_undefined());
    }

    SECTION("object 2")
    {
        const json::Value j = json::Object{{"eins", 1}};
        CHECK(j.is_object());
        const auto& x1 = j["eins"];
        CHECK(x1.is_number());
        CHECK(x1 == 1);
        //const auto& x2 = j["eins"]["zwei"];
        //CHECK(x2.is_undefined());
    }
}
#endif

#if JSON_VALUE_UNDEFINED_IS_UNORDERED
#define UNDEF_COMP(X) (false)
#else
#define UNDEF_COMP(X) (X)
#endif

TEST_CASE("undefined relational")
{
    const double Infinity = std::numeric_limits<double>::infinity();
    const double NaN = std::numeric_limits<double>::quiet_NaN();

    json::Value j_undefined;
    json::Value j_null = nullptr;
    json::Value j_boolean = false;
    json::Value j_number = 0.0;
    json::Value j_string = "";

    CHECK(j_undefined.is_undefined());
    CHECK(j_null.is_null());
    CHECK(j_boolean.is_boolean());
    CHECK(j_number.is_number());
    CHECK(j_string.is_string());

    // XXX:
    //
    // In JavaScript all these comparisons with 'undefined' are false,
    // i.e. 'undefined' is unordered (like NaN).
    //
    // Here 'undefined' is ordered, to preserve  x <= y <==> !(y < x)

    CHECK((j_undefined <  j_undefined) == UNDEF_COMP(false));
    CHECK((j_undefined <= j_undefined) == UNDEF_COMP(true));
    CHECK((j_undefined >  j_undefined) == UNDEF_COMP(false));
    CHECK((j_undefined >= j_undefined) == UNDEF_COMP(true));
    CHECK((j_undefined <  j_null     ) == UNDEF_COMP(true));
    CHECK((j_undefined <= j_null     ) == UNDEF_COMP(true));
    CHECK((j_undefined >  j_null     ) == UNDEF_COMP(false));
    CHECK((j_undefined >= j_null     ) == UNDEF_COMP(false));
    CHECK((j_undefined <  j_boolean  ) == UNDEF_COMP(true));
    CHECK((j_undefined <= j_boolean  ) == UNDEF_COMP(true));
    CHECK((j_undefined >  j_boolean  ) == UNDEF_COMP(false));
    CHECK((j_undefined >= j_boolean  ) == UNDEF_COMP(false));
    CHECK((j_undefined <  j_number   ) == UNDEF_COMP(true));
    CHECK((j_undefined <= j_number   ) == UNDEF_COMP(true));
    CHECK((j_undefined >  j_number   ) == UNDEF_COMP(false));
    CHECK((j_undefined >= j_number   ) == UNDEF_COMP(false));
    CHECK((j_undefined <  j_string   ) == UNDEF_COMP(true));
    CHECK((j_undefined <= j_string   ) == UNDEF_COMP(true));
    CHECK((j_undefined >  j_string   ) == UNDEF_COMP(false));
    CHECK((j_undefined >= j_string   ) == UNDEF_COMP(false));

    CHECK((j_undefined <  j_undefined) == UNDEF_COMP(false));
    CHECK((j_undefined <= j_undefined) == UNDEF_COMP(true));
    CHECK((j_undefined >  j_undefined) == UNDEF_COMP(false));
    CHECK((j_undefined >= j_undefined) == UNDEF_COMP(true));
    CHECK((j_null      <  j_undefined) == UNDEF_COMP(false));
    CHECK((j_null      <= j_undefined) == UNDEF_COMP(false));
    CHECK((j_null      >  j_undefined) == UNDEF_COMP(true));
    CHECK((j_null      >= j_undefined) == UNDEF_COMP(true));
    CHECK((j_boolean   <  j_undefined) == UNDEF_COMP(false));
    CHECK((j_boolean   <= j_undefined) == UNDEF_COMP(false));
    CHECK((j_boolean   >  j_undefined) == UNDEF_COMP(true));
    CHECK((j_boolean   >= j_undefined) == UNDEF_COMP(true));
    CHECK((j_number    <  j_undefined) == UNDEF_COMP(false));
    CHECK((j_number    <= j_undefined) == UNDEF_COMP(false));
    CHECK((j_number    >  j_undefined) == UNDEF_COMP(true));
    CHECK((j_number    >= j_undefined) == UNDEF_COMP(true));
    CHECK((j_string    <  j_undefined) == UNDEF_COMP(false));
    CHECK((j_string    <= j_undefined) == UNDEF_COMP(false));
    CHECK((j_string    >  j_undefined) == UNDEF_COMP(true));
    CHECK((j_string    >= j_undefined) == UNDEF_COMP(true));

    CHECK((j_undefined <  nullptr    ) == UNDEF_COMP(true));
    CHECK((j_undefined <= nullptr    ) == UNDEF_COMP(true));
    CHECK((j_undefined >  nullptr    ) == UNDEF_COMP(false));
    CHECK((j_undefined >= nullptr    ) == UNDEF_COMP(false));
    CHECK((j_undefined <  false      ) == UNDEF_COMP(true));
    CHECK((j_undefined <= false      ) == UNDEF_COMP(true));
    CHECK((j_undefined >  false      ) == UNDEF_COMP(false));
    CHECK((j_undefined >= false      ) == UNDEF_COMP(false));
    CHECK((j_undefined <  0.0        ) == UNDEF_COMP(true));
    CHECK((j_undefined <= 0.0        ) == UNDEF_COMP(true));
    CHECK((j_undefined >  0.0        ) == UNDEF_COMP(false));
    CHECK((j_undefined >= 0.0        ) == UNDEF_COMP(false));
    CHECK((j_undefined <  ""         ) == UNDEF_COMP(true));
    CHECK((j_undefined <= ""         ) == UNDEF_COMP(true));
    CHECK((j_undefined >  ""         ) == UNDEF_COMP(false));
    CHECK((j_undefined >= ""         ) == UNDEF_COMP(false));

    CHECK((nullptr     <  j_undefined) == UNDEF_COMP(false));
    CHECK((nullptr     <= j_undefined) == UNDEF_COMP(false));
    CHECK((nullptr     >  j_undefined) == UNDEF_COMP(true));
    CHECK((nullptr     >= j_undefined) == UNDEF_COMP(true));
    CHECK((false       <  j_undefined) == UNDEF_COMP(false));
    CHECK((false       <= j_undefined) == UNDEF_COMP(false));
    CHECK((false       >  j_undefined) == UNDEF_COMP(true));
    CHECK((false       >= j_undefined) == UNDEF_COMP(true));
    CHECK((0.0         <  j_undefined) == UNDEF_COMP(false));
    CHECK((0.0         <= j_undefined) == UNDEF_COMP(false));
    CHECK((0.0         >  j_undefined) == UNDEF_COMP(true));
    CHECK((0.0         >= j_undefined) == UNDEF_COMP(true));
    CHECK((""          <  j_undefined) == UNDEF_COMP(false));
    CHECK((""          <= j_undefined) == UNDEF_COMP(false));
    CHECK((""          >  j_undefined) == UNDEF_COMP(true));
    CHECK((""          >= j_undefined) == UNDEF_COMP(true));

    //bool const b1 = j_undefined < json::undefined_t;
}

TEST_CASE("Escape special")
{
    json::Value j = "hello </world>";
    std::string str;
    json::stringify(str, j);
    CHECK(str == "\"hello <\\/world>\"");
}

TEST_CASE("NaN/Inf")
{
    json::Value j;

    CHECK(json::ParseStatus::invalid_number == json::parse(j, "NaN"));
    CHECK(json::ParseStatus::invalid_number == json::parse(j, "Infinity"));
    CHECK(json::ParseStatus::invalid_number == json::parse(j, "-NaN"));
    CHECK(json::ParseStatus::invalid_number == json::parse(j, "-Infinity"));

    CHECK(json::ParseStatus::invalid_number == json::parse(j, "+NaN"));
    CHECK(json::ParseStatus::invalid_number == json::parse(j, "+Infinity"));

    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "Na"));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "nan"));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "NaNNaNNaN"));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "Inf"));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "InfInf"));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "Infinit"));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "infinity"));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "InfinityInfinityInfinity"));
}
