﻿// Copyright 2013-2024 Daniel Parker
// Distributed under Boost license

#include <jsoncons/json.hpp>
#include <jsoncons/json_encoder.hpp>
#include <jsoncons/json_parser.hpp>
#include <jsoncons/json_decoder.hpp>
#include <catch/catch.hpp>
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <fstream>

using namespace jsoncons;

TEST_CASE("Test cyrillic.json")
{
    std::string path = "./corelib/input/cyrillic.json";
    std::fstream is(path);
    if (!is)
    {
        std::cout << "Cannot open " << path << std::endl;
    }
    REQUIRE(is);
    json j = json::parse(is);
}

TEST_CASE("test_object2")
{
json source = json::parse(R"(
{
    "a" : "2",
    "c" : [4,5,6]
}
)");

    std::cout << source << std::endl;
}

TEST_CASE("test_object_with_three_members")
{
    std::string input = "{\"A\":\"Jane\", \"B\":\"Roe\",\"C\":10}";
    json val = json::parse(input);

    CHECK(true == val.is_object());
    CHECK(3 == val.size());
}

TEST_CASE("test_double")
{
    json val = json::parse("42.229999999999997");
}

TEST_CASE("test_array_of_integer")
{
    std::string s = "[1,2,3]";
    json j1 = json::parse(s);
    CHECK(true == j1.is_array());
    CHECK(3 == j1.size());

    std::istringstream is(s);
    json j2 = json::parse(is);
    CHECK(true == j2.is_array());
    CHECK(3 == j2.size());
}

TEST_CASE("test_skip_bom")
{
    std::string s = "\xEF\xBB\xBF[1,2,3]";
    json j1 = json::parse(s);
    CHECK(true == j1.is_array());
    CHECK(3 == j1.size());

    std::istringstream is(s);
    json j2 = json::parse(is);
    CHECK(true == j2.is_array());
    CHECK(3 == j2.size());
}

TEST_CASE("test_parse_empty_object")
{
    jsoncons::json_decoder<json> decoder;
    json_parser parser;

    parser.reset();

    static std::string s("{}");

    parser.set_buffer(s.data(),s.length());
    parser.parse_some(decoder);
    parser.finish_parse(decoder);
    CHECK(parser.done());

    json j = decoder.get_result();
}

TEST_CASE("test_parse_array")
{
    jsoncons::json_decoder<json> decoder;
    json_parser parser;

    parser.reset();

    static std::string s("[]");

    parser.set_buffer(s.data(),s.length());
    parser.parse_some(decoder);
    parser.finish_parse(decoder);
    CHECK(parser.done());

    json j = decoder.get_result();
}

TEST_CASE("test_parse_string")
{
    jsoncons::json_decoder<json> decoder;
    json_parser parser;

    parser.reset();

    static std::string s("\"\"");

    parser.set_buffer(s.data(),s.length());
    parser.parse_some(decoder);
    parser.finish_parse(decoder);
    CHECK(parser.done());

    json j = decoder.get_result();
}

TEST_CASE("test_parse_integer")
{
    jsoncons::json_decoder<json> decoder;
    json_parser parser;

    parser.reset();

    static std::string s("10");

    parser.set_buffer(s.data(),s.length());
    parser.parse_some(decoder);
    parser.finish_parse(decoder);
    CHECK(parser.done());

    json j = decoder.get_result();
}

TEST_CASE("test_parse_integer_space")
{
    jsoncons::json_decoder<json> decoder;
    json_parser parser;

    parser.reset();

    static std::string s("10 ");

    parser.set_buffer(s.data(),s.length());
    parser.parse_some(decoder);
    parser.finish_parse(decoder);
    CHECK(parser.done());

    json j = decoder.get_result();
}

TEST_CASE("test_parse_double_space")
{
    jsoncons::json_decoder<json> decoder;
    json_parser parser;

    parser.reset();

    static std::string s("10.0 ");

    parser.set_buffer(s.data(),s.length());
    parser.parse_some(decoder);
    parser.finish_parse(decoder);
    CHECK(parser.done());

    json j = decoder.get_result();
}

TEST_CASE("test_parse_false")
{
    jsoncons::json_decoder<json> decoder;
    json_parser parser;

    parser.reset();

    static std::string s("false");

    parser.set_buffer(s.data(),s.length());
    parser.parse_some(decoder);
    parser.finish_parse(decoder);
    CHECK(parser.done());

    json j = decoder.get_result();
}

TEST_CASE("test_parse_true")
{
    jsoncons::json_decoder<json> decoder;
    json_parser parser;

    parser.reset();

    static std::string s("true");

    parser.set_buffer(s.data(),s.length());
    parser.parse_some(decoder);
    parser.finish_parse(decoder);
    CHECK(parser.done());

    json j = decoder.get_result();
}

TEST_CASE("test_parse_null")
{
    jsoncons::json_decoder<json> decoder;
    json_parser parser;

    parser.reset();

    static std::string s("null");

    parser.set_buffer(s.data(),s.length());
    parser.parse_some(decoder);
    parser.finish_parse(decoder);
    CHECK(parser.done());

    json j = decoder.get_result();
}

TEST_CASE("test_incremental_parsing")
{
    SECTION("Array of strings")
    {
        std::vector<std::string> chunks = {"[fal", "se, ", "t", "rue", ", null", ",", "1", "234", ",\"Hello ", "World\",", 
            "0.", "1234", "]"};
        std::size_t index = 0;

        auto read_chunk = [&](jsoncons::parser_input& input, std::error_code& /*ec*/) -> bool
        {
            if (index < chunks.size())
            {
                input.set_buffer(chunks[index].data(), chunks[index].size());
                ++index;
                return true;
            }
            else
            {
                return false;
            }
        };

        jsoncons::json_decoder<json> decoder;
        json_parser parser{read_chunk};

        parser.reset();

        parser.parse_some(decoder);
        CHECK_FALSE(parser.done());
        CHECK(parser.source_exhausted());
        parser.finish_parse(decoder);
        CHECK(parser.done());

        json j = decoder.get_result();
        REQUIRE(j.is_array());
        REQUIRE(j.size() == 6);
        CHECK_FALSE(j[0].as<bool>());
        CHECK(j[1].as<bool>());
        CHECK(j[2].is_null());
        CHECK(j[3].as<int>() == 1234);
        CHECK(j[4].as<std::string>() == "Hello World");
        CHECK(j[5].as<double>() == 0.1234);
    }
}

TEST_CASE("test_parser_reinitialization")
{
    jsoncons::json_decoder<json> decoder;
    json_parser parser;

    parser.reset();
    parser.set_buffer("false true", 10);
    parser.finish_parse(decoder);
    CHECK(parser.done());
    CHECK_FALSE(parser.source_exhausted());
    json j1 = decoder.get_result();
    REQUIRE(j1.is_bool());
    CHECK_FALSE(j1.as<bool>());

    parser.reinitialize();
    parser.set_buffer("-42", 3);
    parser.finish_parse(decoder);
    CHECK(parser.done());
    CHECK(parser.source_exhausted());
    json j2 = decoder.get_result();
    REQUIRE(j2.is_int64());
    CHECK(j2.as<int64_t>() == -42);
}

TEST_CASE("test_diagnostics_visitor", "")
{
    SECTION("narrow char")
    {
        std::ostringstream os;
        json_diagnostics_visitor visitor(os, "  ");
        json_parser parser;
        std::string input(R"({"foo":[42,null]})");
        parser.set_buffer(input.data(), input.size());
        parser.finish_parse(visitor);
        std::ostringstream expected;
        expected << "visit_begin_object"  << std::endl
                 << "  visit_key:foo"     << std::endl
                 << "  visit_begin_array" << std::endl
                 << "    visit_uint64:42" << std::endl
                 << "    visit_null"      << std::endl
                 << "  visit_end_array"   << std::endl
                 << "visit_end_object"    << std::endl;
        CHECK(os.str() == expected.str());
    }

    SECTION("wide char")
    {
        std::wostringstream os;
        wjson_diagnostics_visitor visitor(os, L"  ");
        wjson_parser parser;
        std::wstring input(LR"({"foo":[42,null]})");
        parser.set_buffer(input.data(), input.size());
        parser.finish_parse(visitor);
        std::wostringstream expected;
        expected << L"visit_begin_object"  << std::endl
                 << L"  visit_key:foo"     << std::endl
                 << L"  visit_begin_array" << std::endl
                 << L"    visit_uint64:42" << std::endl
                 << L"    visit_null"      << std::endl
                 << L"  visit_end_array"   << std::endl
                 << L"visit_end_object"    << std::endl;
        CHECK(os.str() == expected.str());
    }
}

