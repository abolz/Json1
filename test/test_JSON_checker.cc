// From:
// http://www.json.org/JSON_checker/test.zip

#if 1

#include "catch.hpp"
#include "../src/json.h"

struct JSON_checker_test {
    std::string input;
    std::string name;
};

static const JSON_checker_test kJSON_checker_pass[] = {
    {std::string("[\n    \"JSON Test Pattern pass1\",\n    {\"object with 1 member\":[\"array with 1 element\"]},\n    {},\n    [],\n    -42,\n    true,\n    false,\n    null,\n    {\n        \"integer\": 1234567890,\n        \"real\": -9876.543210,\n        \"e\": 0.123456789e-12,\n        \"E\": 1.234567890E+34,\n        \"\":  23456789012E66,\n        \"zero\": 0,\n        \"one\": 1,\n        \"space\": \" \",\n        \"quote\": \"\\\"\",\n        \"backslash\": \"\\\\\",\n        \"controls\": \"\\b\\f\\n\\r\\t\",\n        \"slash\": \"/ & \\/\",\n        \"alpha\": \"abcdefghijklmnopqrstuvwyz\",\n        \"ALPHA\": \"ABCDEFGHIJKLMNOPQRSTUVWYZ\",\n        \"digit\": \"0123456789\",\n        \"0123456789\": \"digit\",\n        \"special\": \"`1~!@#$%^&*()_+-={\':[,]}|;.</>?\",\n        \"hex\": \"\\u0123\\u4567\\u89AB\\uCDEF\\uabcd\\uef4A\",\n        \"true\": true,\n        \"false\": false,\n        \"null\": null,\n        \"array\":[  ],\n        \"object\":{  },\n        \"address\": \"50 St. James Street\",\n        \"url\": \"http://www.JSON.org/\",\n        \"comment\": \"// /* <!-- --\",\n        \"# -- --> */\": \" \",\n        \" s p a c e d \" :[1,2 , 3\n\n,\n\n4 , 5        ,          6           ,7        ],\"compact\":[1,2,3,4,5,6,7],\n        \"jsontext\": \"{\\\"object with 1 member\\\":[\\\"array with 1 element\\\"]}\",\n        \"quotes\": \"&#34; \\u0022 %22 0x22 034 &#x22;\",\n        \"\\/\\\\\\\"\\uCAFE\\uBABE\\uAB98\\uFCDE\\ubcda\\uef4A\\b\\f\\n\\r\\t`1~!@#$%^&*()_+-=[]{}|;:\',./<>?\"\n: \"A key can be any string\"\n    },\n    0.5 ,98.6\n,\n99.44\n,\n\n1066,\n1e1,\n0.1e1,\n1e-1,\n1e00,2e+00,2e-00\n,\"rosebud\"]", 1441), "pass1"},
    {std::string("[[[[[[[[[[[[[[[[[[[\"Not too deep\"]]]]]]]]]]]]]]]]]]]", 52), "pass2"},
    {std::string("{\n    \"JSON Test Pattern pass3\": {\n        \"The outermost value\": \"must be an object or array.\",\n        \"In this test\": \"It is an object.\"\n    }\n}\n", 148), "pass3"},
};

static const JSON_checker_test kJSON_checker_fail[] = {
    //{std::string("\"A JSON payload should be an object or array, not a string.\"", 60), "fail1"},
    {std::string("[\"Unclosed array\"", 17), "fail2"},
    {std::string("{unquoted_key: \"keys must be quoted\"}", 37), "fail3"},
    {std::string("[\"extra comma\",]", 16), "fail4"},
    {std::string("[\"double extra comma\",,]", 24), "fail5"},
    {std::string("[   , \"<-- missing value\"]", 26), "fail6"},
    {std::string("[\"Comma after the close\"],", 26), "fail7"},
    {std::string("[\"Extra close\"]]", 16), "fail8"},
    {std::string("{\"Extra comma\": true,}", 22), "fail9"},
    {std::string("{\"Extra value after close\": true} \"misplaced quoted value\"", 58), "fail10"},
    {std::string("{\"Illegal expression\": 1 + 2}", 29), "fail11"},
    {std::string("{\"Illegal invocation\": alert()}", 31), "fail12"},
    {std::string("{\"Numbers cannot have leading zeroes\": 013}", 43), "fail13"},
    {std::string("{\"Numbers cannot be hex\": 0x14}", 31), "fail14"},
    {std::string("[\"Illegal backslash escape: \\x15\"]", 34), "fail15"},
    {std::string("[\\naked]", 8), "fail16"},
    {std::string("[\"Illegal backslash escape: \\017\"]", 34), "fail17"},
    //{std::string("[[[[[[[[[[[[[[[[[[[[\"Too deep\"]]]]]]]]]]]]]]]]]]]]", 50), "fail18"},
    {std::string("{\"Missing colon\" null}", 22), "fail19"},
    {std::string("{\"Double colon\":: null}", 23), "fail20"},
    {std::string("{\"Comma instead of colon\", null}", 32), "fail21"},
    {std::string("[\"Colon instead of comma\": false]", 33), "fail22"},
    {std::string("[\"Bad value\", truth]", 20), "fail23"},
    {std::string("[\'single quote\']", 16), "fail24"},
    {std::string("[\"\ttab\tcharacter\tin\tstring\t\"]", 29), "fail25"},
    {std::string("[\"tab\\   character\\   in\\  string\\  \"]", 38), "fail26"},
    {std::string("[\"line\nbreak\"]", 14), "fail27"},
    {std::string("[\"line\\\nbreak\"]", 15), "fail28"},
    {std::string("[0e]", 4), "fail29"},
    {std::string("[0e+]", 5), "fail30"},
    {std::string("[0e+-1]", 7), "fail31"},
    {std::string("{\"Comma instead if closing brace\": true,", 40), "fail32"},
    {std::string("[\"mismatch\"}", 12), "fail33"},
};

TEST_CASE("JSON_checker")
{
    SECTION("pass")
    {
        for (auto const& test : kJSON_checker_pass)
        {
            CAPTURE(test.name);

            json::Value val;
            auto const res = json::parse(val, test.input.data(), test.input.data() + test.input.size());
            CHECK(res.ec == json::ParseStatus::success);

            std::string str;
            auto const ok = json::stringify(str, val);
            CHECK(ok == true);

            json::Value val2;
            auto const res2 = json::parse(val2, str.data(), str.data() + str.size());
            CHECK(res2.ec == json::ParseStatus::success);

            CHECK(val == val2);
            CHECK(!(val != val2));
            CHECK(!(val < val2));
            CHECK(val <= val2);
            CHECK(!(val > val2));
            CHECK(val >= val2);
        }
    }

    SECTION("fail")
    {
        for (auto const& test : kJSON_checker_fail)
        {
            CAPTURE(test.name);

            json::Value val;
            auto const res = json::parse(val, test.input.data(), test.input.data() + test.input.size());
            CHECK(res.ec != json::ParseStatus::success);
        }
    }
}

#endif
