// Copyright 2013-2025 Daniel Parker
// Distributed under Boost license

#if defined(_MSC_VER)
#include "windows.h" // test no inadvertant macro expansions
#endif

#include <jsoncons_ext/jmespath/jmespath.hpp>
#include <jsoncons/json.hpp>

#include <iostream>
#include <catch/catch.hpp>

using jsoncons::json;
using jsoncons::ojson;
namespace jmespath = jsoncons::jmespath;

TEST_CASE("jmespath_expression tests")
{
    SECTION("Test 1")
    {
        std::string jtext = R"(
            {
              "people": [
                {
                  "age": 20,
                  "other": "foo",
                  "name": "Bob"
                },
                {
                  "age": 25,
                  "other": "bar",
                  "name": "Fred"
                },
                {
                  "age": 30,
                  "other": "baz",
                  "name": "George"
                }
              ]
            }
        )";

        auto expr = jmespath::make_expression<json>("sum(people[].age)");

        json doc = json::parse(jtext);

        json result = expr.evaluate(doc);
        CHECK(result == json(75.0));
    }    
    SECTION("Test 2")
    {
        std::string jtext = R"(
{
    "group": {
      "value": 1
    },
    "array": [
      {"value": 2}
    ]
}
        )";

        json doc = json::parse(jtext);

        auto expr1 = jmespath::make_expression<json>("group.value");
        json result1 = expr1.evaluate(doc);
        CHECK(json(1) == result1);

        auto expr2 = jmespath::make_expression<json>("array[0].value");
        json result2 = expr2.evaluate(doc);
        CHECK(json(2) == result2);

        auto expr3 = jmespath::make_expression<json>("nullable.value");
        json result3 = expr3.evaluate(doc);
        CHECK(result3 == json::null());
    }
}

TEST_CASE("jmespath issue") 
{
    SECTION("issue 1")
    {
        std::string jtext = R"(
        {
          "locations": [
            {"name": "Seattle", "state": "WA"},
            {"name": "New York", "state": "NY"},
            {"name": "Bellevue", "state": "WA"},
            {"name": "Olympia", "state": "WA"}
          ]
        }        
        )";

        std::string expr = R"(
        {
            name: locations[].name,
            state: locations[].state
        }
        )";

        auto doc = ojson::parse(jtext);

        auto result = jmespath::search(doc, expr);

        //std::cout << pretty_print(result) << "\n\n";
    }
    SECTION("parentheses issue")
    {
        auto doc = jsoncons::json::parse(R"(
    {"foo" : [[0, 1], [2, 3], [4, 5]]}
        )");

        auto expected = jsoncons::json::parse(R"([0, 1])");

        std::string query = R"((foo[*])[0])";
        auto expr = jmespath::make_expression<jsoncons::json>(query);

        jsoncons::json result = expr.evaluate(doc);
        //std::cout << pretty_print(result) << "\n";
        CHECK(expected == result);
    }
}

TEST_CASE("jmespath issue 605")
{
    SECTION("function with 1 arg")
    {
        std::string query = R"(
to_array("gw:GWallInfo"."gw:DocumentStatistics"."gw:ContentGroups"."gw:ContentGroup" || 
    "gw:DocumentStatistics"."gw:ContentGroups"."gw:ContentGroup")
)";

        auto expr = jsoncons::jmespath::make_expression<json>(query);
        json j;
        j["gw:DocumentStatistics"]["gw:ContentGroups"]["gw:ContentGroup"] = 9;
        auto result = expr.evaluate(j);
        REQUIRE(result.is_array());
        REQUIRE_FALSE(result.empty());
        CHECK(9 == result[0]);

        //std::cout << pretty_print(result) << "\n\n";
    }
    SECTION("function with 2 args")
    {
        std::string query = R"(starts_with(B || A,null || 'a'))";

        auto expr = jsoncons::jmespath::make_expression<json>(query);
        json j;
        j["A"] = "ab";
        //auto result = jsoncons::jmespath::search(j, expr);
        auto result = expr.evaluate(j);
        //std::cout << result << "\n";

        CHECK(result.as<bool>());
    }
    SECTION("function with 2 args (2)")
    {
        std::string query = R"(starts_with(A || B,null || 'a'))";

        auto expr = jsoncons::jmespath::make_expression<json>(query);
        json j;
        j["A"] = "ab";
        //auto result = jsoncons::jmespath::search(j, expr);
        auto result = expr.evaluate(j);
        //std::cout << result << "\n";

        CHECK(result.as<bool>());
    }
}

