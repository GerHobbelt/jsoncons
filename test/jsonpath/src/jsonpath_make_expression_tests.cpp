// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#if defined(_MSC_VER)
#include "windows.h" // test no inadvertant macro expansions
#endif
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>
#include <catch/catch.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <utility>
#include <ctime>
#include <new>
#include <unordered_set> // std::unordered_set
#include <fstream>

using namespace jsoncons;

TEST_CASE("jsonpath make_expression test")
{
    std::string input = R"(
    {
        "books":
        [
            {
                "category": "fiction",
                "title" : "A Wild Sheep Chase",
                "author" : "Haruki Murakami",
                "price" : 22.72
            },
            {
                "category": "fiction",
                "title" : "The Night Watch",
                "author" : "Sergei Lukyanenko",
                "price" : 23.58
            },
            {
                "category": "fiction",
                "title" : "The Comedians",
                "author" : "Graham Greene",
                "price" : 21.99
            },
            {
                "category": "memoir",
                "title" : "The Night Watch",
                "author" : "Phillips, David Atlee"
            }
        ]
    }
    )";
#if 0
    SECTION("test 1")
    {
        int count = 0;

        const json doc = json::parse(input);

        auto expr = jsoncons::jsonpath::make_expression_for_update<json>("$.books[*]");

        auto callback = [&](const std::string& /*path*/, const json& book)
        {
            if (book.at("category") == "memoir" && !book.contains("price"))
            {
                ++count;
            }
        };

        expr.evaluate(doc, callback);

        CHECK(count == 1);
        CHECK_FALSE(doc["books"][3].contains("price"));
    }
#endif
    SECTION("test 2")
    {
        int count = 0;

        json doc = json::parse(input);

        auto expr = jsoncons::jsonpath::make_expression_for_update<json>("$.books[*]");

        auto callback1 = [&](const std::string& /*path*/, const json& book)
        {
            if (book.at("category") == "memoir" && !book.contains("price"))
            {
                ++count;
            }
        };

        auto callback2 = [](const std::string& /*path*/, json& book)
        {
            if (book.at("category") == "memoir" && !book.contains("price"))
            {
                book.try_emplace("price", 140.0);
            }
        };

        expr.evaluate(doc, callback1);

        CHECK(count == 1);

        CHECK_FALSE(doc["books"][3].contains("price"));
        expr.evaluate_and_update(doc, callback2);
        CHECK(doc["books"][3].contains("price"));
        CHECK(doc["books"][3].at("price") == 140);
    }
}

