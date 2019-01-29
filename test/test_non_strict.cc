#include "catch.hpp"
#include "../src/json.h"

#include <cmath>
#include <limits>

TEST_CASE("Comments")
{
    std::string const inp = R"(// comment
    /* nested /* multi line */
    { /* object
// starts
here */
        "empty_array" /* comment */: [ // comment
        ],
        "/*empty object*/" : { /**/ }
    })";

    CAPTURE(inp);

    json::Value val;
    auto const res = json::parse(val, inp.data(), inp.data() + inp.size(), json::Mode::lenient);
    //printf("|%.*s|\n", static_cast<int>(res.end - res.ptr), res.ptr);
    REQUIRE(res.ec == json::ParseStatus::success);

    CHECK(val.is_object());
    CHECK(val.size() == 2);
    CHECK(val.has_member("empty_array"));
    CHECK(val["empty_array"].is_array());
    CHECK(val["empty_array"].size() == 0);
    CHECK(val.has_member("/*empty object*/"));
    CHECK(val["/*empty object*/"].is_object());
    CHECK(val["/*empty object*/"].empty());
}

TEST_CASE("Invalid block comments")
{
    std::vector<std::string> inputs = {
        "/*",
        "/* ",
        "/*/",
        "/**",
        "/*\n/",
        "/*\n*",
        "/*/\n",
        "/**\n",
        "/*\r\n/",
        "/*\r\n*",
        "/*\r/\n",
        "/*\r*\n",
        "/**\n/",
        "/**\r\n/",
        "/** /",
        "/*/**/",
    };

    for (auto const& inp : inputs)
    {
        CAPTURE(inp);
        json::Value val;
        auto const res = json::parse(val, inp.data(), inp.data() + inp.size(), json::Mode::lenient);
        CHECK(res.ec != json::ParseStatus::success);
    }
}

TEST_CASE("Lenient")
{
    std::string const input = u8R"({
    "😎": [],
    😍 : {},
    FirstName: "John",
    Last\u004Eame: "Doe",
    Age: 43,
    \u0041ddress: {
        Street: "Downing \"Street\" 10",
        City: "London",
        Country: "Great Britain",
    },
    "Phone numbers": [
        "+44\u20281234567",
        "+44\u20292345678",
    ],
    Infinity: Infinity,
    NaN: NaN,
})";

    json::Value val;
    auto const res = json::parse(val, input.data(), input.data() + input.size(), json::Mode::lenient);
    REQUIRE(res.ec == json::ParseStatus::success);

    REQUIRE(val.is_object());
    REQUIRE(val.size() == 9);
    REQUIRE(val.has_member(u8"😎"));
    REQUIRE(val[u8"😎"].is_array());
    REQUIRE(val[u8"😎"].size() == 0);
    REQUIRE(val.has_member(u8"😍"));
    REQUIRE(val[u8"😍"].is_object());
    REQUIRE(val[u8"😍"].size() == 0);
    REQUIRE(val.has_member("FirstName"));
    REQUIRE(val["FirstName"].is_string());
    REQUIRE(val["FirstName"] == "John");
    REQUIRE(val.has_member("LastName"));
    REQUIRE(val["LastName"].is_string());
    REQUIRE(val["LastName"] == "Doe");
    REQUIRE(val.has_member("Age"));
    REQUIRE(val["Age"].is_number());
    REQUIRE(val["Age"] == 43);
    REQUIRE(val.has_member("Address"));
    REQUIRE(val["Address"].is_object());
    REQUIRE(val["Address"].has_member("Street"));
    REQUIRE(val["Address"]["Street"].is_string());
    REQUIRE(val["Address"]["Street"] == "Downing \"Street\" 10");
    REQUIRE(val["Address"].has_member("City"));
    REQUIRE(val["Address"]["City"].is_string());
    REQUIRE(val["Address"]["City"] == "London");
    REQUIRE(val["Address"].has_member("Country"));
    REQUIRE(val["Address"]["Country"].is_string());
    REQUIRE(val["Address"]["Country"] == "Great Britain");
    REQUIRE(val.has_member("Phone numbers"));
    REQUIRE(val["Phone numbers"].is_array());
    REQUIRE(val["Phone numbers"].size() == 2);
    REQUIRE(val["Phone numbers"][0].is_string());
    REQUIRE(val["Phone numbers"][0] == u8"+44\u20281234567");
    REQUIRE(val["Phone numbers"][1].is_string());
    REQUIRE(val["Phone numbers"][1] == u8"+44\u20292345678");
    REQUIRE(val["Infinity"].get_number() == std::numeric_limits<double>::infinity());
    REQUIRE(std::isnan(val["NaN"].get_number()));

    std::string const str_expected = u8R"({
    "Address": {
        "City": "London",
        "Country": "Great Britain",
        "Street": "Downing \"Street\" 10"
    },
    "Age": 43,
    "FirstName": "John",
    "Infinity": Infinity,
    "LastName": "Doe",
    "NaN": NaN,
    "Phone numbers": [
        "+44\u20281234567",
        "+44\u20292345678"
    ],
    "😍": {},
    "😎": []
})";

    json::StringifyOptions opts;

    opts.mode = json::Mode::lenient;
    opts.indent_width = 4;

    std::string str;
    auto const ok = json::stringify(str, val, opts);
    REQUIRE(ok);
    REQUIRE(str_expected == str);
}

TEST_CASE("NaN/Inf")
{
    json::Mode mode = json::Mode::lenient;
    json::Value j;

    CHECK(json::ParseStatus::success == json::parse(j, "NaN", mode));
    CHECK(j.is_number());
    CHECK(std::isnan(j.get_number()));
    CHECK(json::ParseStatus::success == json::parse(j, "Infinity", mode));
    CHECK(j.is_number());
    CHECK(j.get_number() == std::numeric_limits<double>::infinity());
    CHECK(json::ParseStatus::success == json::parse(j, "-NaN", mode));
    CHECK(j.is_number());
    CHECK(std::isnan(j.get_number()));
    CHECK(json::ParseStatus::success == json::parse(j, "-Infinity", mode));
    CHECK(j.is_number());
    CHECK(j.get_number() == -std::numeric_limits<double>::infinity());

    CHECK(json::ParseStatus::invalid_number == json::parse(j, "+NaN", mode));
    CHECK(json::ParseStatus::invalid_number == json::parse(j, "+Infinity", mode));

    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "Na", mode));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "nan", mode));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "NaNNaNNaN", mode));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "Inf", mode));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "InfInf", mode));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "Infinit", mode));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "infinity", mode));
    CHECK(json::ParseStatus::unrecognized_identifier == json::parse(j, "InfinityInfinityInfinity", mode));
}
