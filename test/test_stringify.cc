#include "catch.hpp"
#include "../src/json.h"

// sorted objects
static constexpr char const* svg_menu = R"({"menu":{"array":[1],"empty_array":[],"header":"SVG Viewer","items":[{"id":"Open"},{"id":"OpenNew","label":"Open New"},null]}})";

TEST_CASE("Stringify - compact")
{
    json::Value j;
    auto const ec = json::parse(j, svg_menu);
    REQUIRE(ec == json::ParseStatus::success);

    std::string str;
    auto const ecs = json::stringify(str, j);
    REQUIRE(ecs == true);
    CHECK(svg_menu == str);
}

TEST_CASE("Stringify - pretty")
{
    static constexpr char const* expected = R"({
    "menu": {
        "array": [
            1
        ],
        "empty_array": [],
        "header": "SVG Viewer",
        "items": [
            {
                "id": "Open"
            },
            {
                "id": "OpenNew",
                "label": "Open New"
            },
            null
        ]
    }
})";

    json::Value j;
    auto const ec = json::parse(j, svg_menu);
    REQUIRE(ec == json::ParseStatus::success);

    json::StringifyOptions options;

    options.indent_width = 4;

    std::string str;
    auto const ecs = json::stringify(str, j, options);
    REQUIRE(ecs == true);
    CHECK(expected == str);
}
