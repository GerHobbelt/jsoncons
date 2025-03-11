// Copyright 2013-2025 Daniel Parker
// Distributed under Boost license

#include <jsoncons/json.hpp>
#include <jsoncons_ext/csv/csv.hpp>
#include <catch/catch.hpp>

namespace csv = jsoncons::csv; 

TEST_CASE("test json to flat csv")
{
    SECTION("array of objects to csv")
    {
        std::string expected = R"(boolean,datetime,float,text
true,1971-01-01T04:14:00,1.0,Chicago Reader
true,1948-01-01T14:57:13,1.27,Chicago Sun-Times
)";
        
        std::string jtext = R"(
[
    {
        "text": "Chicago Reader", 
        "float": 1.0, 
        "datetime": "1971-01-01T04:14:00", 
        "boolean": true,
        "nested": {
          "time": "04:14:00",
          "nested": {
            "date": "1971-01-01",
            "integer": 40
          }
        }
    }, 
    {
        "text": "Chicago Sun-Times", 
        "float": 1.27, 
        "datetime": "1948-01-01T14:57:13", 
        "boolean": true,
        "nested": {
          "time": "14:57:13",
          "nested": {
            "date": "1948-01-01",
            "integer": 63
          }
        }
    }
]
        )";

        
        auto j = jsoncons::json::parse(jtext);
        //std::cout << pretty_print(j) << "\n";

        std::string buf;
        csv::csv_string_encoder encoder(buf);
        j.dump(encoder);
        
        //std::cout << buf << "\n";
        
        CHECK(expected == buf);
    }

#if 0

    SECTION("array of objects with some missing members to csv")
    {
        std::string expected = R"(boolean,datetime,float,text
true,1971-01-01T04:14:00,1.0,Chicago Reader
true,1948-01-01T14:57:13,,Chicago Sun-Times
)";

        std::string jtext = R"(
[
    {
        "text": "Chicago Reader", 
        "float": 1.0, 
        "datetime": "1971-01-01T04:14:00", 
        "boolean": true,
        "nested": {
          "time": "04:14:00",
          "nested": {
            "date": "1971-01-01",
            "integer": 40
          }
        }
    }, 
    {
        "text": "Chicago Sun-Times", 
        "datetime": "1948-01-01T14:57:13", 
        "boolean": true,
        "nested": {
          "time": "14:57:13",
          "nested": {
            "date": "1948-01-01",
            "integer": 63
          }
        }
    }
]
        )";


        auto j = jsoncons::json::parse(jtext);
        //std::cout << pretty_print(j) << "\n";

        std::string buf;
        csv::csv_string_encoder encoder(buf);
        j.dump(encoder);

        //std::cout << buf << "\n";

        CHECK(expected == buf);
    }

    SECTION("array of arrays to csv")
    {
        std::string expected = R"(Chicago Reader,1.0,1971-01-01T04:14:00,true
Chicago Sun-Times,1.27,1948-01-01T14:57:13,true
)";

        std::string jtext = R"(
[
    [
        "Chicago Reader", 
        1.0, 
        "1971-01-01T04:14:00", 
        true,
        [ 
          "04:14:00",
          [
            "1971-01-01",
            40
          ]
        ]
    ], 
    [
        "Chicago Sun-Times", 
        1.27, 
        "1948-01-01T14:57:13", 
        true,
        [
          "14:57:13",
          [
            "1948-01-01",
            63
          ]
        ]
    ]
]
        )";

        auto j = jsoncons::json::parse(jtext);
        //std::cout << pretty_print(j) << "\n";

        std::string buf;
        csv::csv_string_encoder encoder(buf);
        j.dump(encoder);
        
        CHECK(expected == buf);
    }    

    SECTION("array of arrays and subarrays to csv")
    {
        std::string expected = R"(calculationPeriodCenters,paymentCenters,resetCenters
NY;LON,TOR,LON
NY,LON,TOR;LON
NY;LON,TOR,LON
NY,LON,TOR;LON
)";

    std::string jtext = R"(
[
    ["calculationPeriodCenters","paymentCenters","resetCenters"],
    [
        ["NY","LON"],"TOR","LON"
    ],
    ["NY","LON",
        ["TOR","LON"]
    ],
    [
        ["NY","LON"],"TOR","LON"
    ],
    ["NY","LON",
        ["TOR","LON"]
    ]
]
    )";

        auto j = jsoncons::json::parse(jtext);
        //std::cout << pretty_print(j) << "\n";

        auto options = csv::csv_options{}
            .subfield_delimiter(';');
            
        std::string buf;
        csv::csv_string_encoder encoder(buf, options);
        j.dump(encoder);

        CHECK(expected == buf);
    }    
    SECTION("object of arrays and subarrays to csv")
    {
        std::string expected = R"(a,b,c
1;true;null,7;8;9,15
-4;5.5;6,10;11;12,16
,,17
)";

        const std::string jtext = R"(
{
   "a" : [[1,true,null],[-4,5.5,"6"]],
   "b" : [[7,8,9],[10,11,12]],
   "c" : [15,16,17]       
}
        )";

        auto j = jsoncons::json::parse(jtext);
        //std::cout << pretty_print(j) << "\n";

        auto options = csv::csv_options{}
            .subfield_delimiter(';');

        std::string buf;
        csv::csv_string_encoder encoder(buf, options);
        j.dump(encoder);

        //std::cout << buf << "\n"; 
        CHECK(expected == buf);
    }    
#endif   
    /*SECTION("array of subarrays to csv")
    {
        const std::string jtext = R"(
[
   [[1,2,3],[4,5,6]],
   [[7,8,9],[10,11,12]]   
]
        )";

        auto j = jsoncons::json::parse(jtext);
        //std::cout << pretty_print(j) << "\n";

        auto options = csv::csv_options{}
            .subfield_delimiter(';');

        std::string buf;
        csv::csv_string_encoder encoder(buf, options);
        j.dump(encoder);

        std::cout << buf << "\n"; 
        //CHECK(expected == buf);
    }*/    
}

TEST_CASE("test json to non-flat csv")
{
#if 0
    SECTION("array of objects to csv")
    {
        std::string expected = R"(/boolean,/datetime,/float,/nested/nested/date,/nested/nested/integer,/nested/time,/text
true,1971-01-01T04:14:00,1.0,1971-01-01,40,04:14:00,Chicago Reader
true,1948-01-01T14:57:13,1.27,1948-01-01,63,14:57:13,Chicago Sun-Times
)";
        
        std::string jtext = R"(
[
    {
        "text": "Chicago Reader", 
        "float": 1.0, 
        "datetime": "1971-01-01T04:14:00", 
        "boolean": true,
        "nested": {
          "time": "04:14:00",
          "nested": {
            "date": "1971-01-01",
            "integer": 40
          }
        }
    }, 
    {
        "text": "Chicago Sun-Times", 
        "float": 1.27, 
        "datetime": "1948-01-01T14:57:13", 
        "boolean": true,
        "nested": {
          "time": "14:57:13",
          "nested": {
            "date": "1948-01-01",
            "integer": 63
          }
        }
    }
]
        )";

        auto j = jsoncons::json::parse(jtext);
        //std::cout << pretty_print(j) << "\n";

        auto options = csv::csv_options{}
            .flat(false);

        std::string buf;
        csv::csv_string_encoder encoder(buf, options);
        j.dump(encoder);
        
        CHECK(expected == buf);
    }

    SECTION("array of objects with some missing members to csv")
    {
        std::string expected = R"(/boolean,/datetime,/float,/nested/nested/date,/nested/nested/integer,/nested/time,/text
true,1971-01-01T04:14:00,1.0,1971-01-01,40,04:14:00,Chicago Reader
true,,1.27,,63,14:57:13,Chicago Sun-Times
)";

        std::string jtext = R"(
[
    {
        "text": "Chicago Reader", 
        "float": 1.0, 
        "datetime": "1971-01-01T04:14:00", 
        "boolean": true,
        "nested": {
          "time": "04:14:00",
          "nested": {
            "date": "1971-01-01",
            "integer": 40
          }
        }
    }, 
    {
        "text": "Chicago Sun-Times", 
        "float": 1.27, 
        "boolean": true,
        "nested": {
          "time": "14:57:13",
          "nested": {
            "integer": 63
          }
        }
    }
]
        )";

        auto j = jsoncons::json::parse(jtext);
        //std::cout << pretty_print(j) << "\n";

        auto options = csv::csv_options{}
            .flat(false);

        std::string buf;
        csv::csv_string_encoder encoder(buf, options);
        j.dump(encoder);

        CHECK(expected == buf);
    }
    
    SECTION("array of arrays to csv")
    {
        std::string expected = R"(/0,/1,/2,/3,/3/0,/3/0/0,/3/0/1
Chicago Reader,1.0,1971-01-01T04:14:00,true,04:14:00,1971-01-01,40
Chicago Sun-Times,1.27,1948-01-01T14:57:13,true,14:57:13,1948-01-01,63
)";

        std::string jtext = R"(
[
    [
        "Chicago Reader", 
        1.0, 
        "1971-01-01T04:14:00", 
        true,
        [ 
          "04:14:00",
          [
            "1971-01-01",
            40
          ]
        ]
    ], 
    [
        "Chicago Sun-Times", 
        1.27, 
        "1948-01-01T14:57:13", 
        true,
        [
          "14:57:13",
          [
            "1948-01-01",
            63
          ]
        ]
    ]
]
        )";

        auto j = jsoncons::json::parse(jtext);
        //std::cout << pretty_print(j) << "\n";

        auto options = csv::csv_options{}
            .flat(false);

        std::string buf;
        csv::csv_string_encoder encoder(buf, options);
        j.dump(encoder);
        
        CHECK(expected == buf);
    }

    SECTION("array of object and subarrays to csv")
    {
        std::string expected = R"(calculationPeriodCenters,paymentCenters,resetCenters
NY;LON,TOR,LON
NY,LON,TOR;LON
NY;LON,TOR,LON
NY,LON,TOR;LON
)";

    std::string jtext = R"(
[
    {"calculationPeriodCenters" : ["NY","LON"],
     "paymentCenters" : "TOR",
     "resetCenters" : "LON"},
    {"calculationPeriodCenters" : "NY",
     "paymentCenters" : "LON",
     "resetCenters" : ["TOR","LON"]},
    {"calculationPeriodCenters" : ["NY","LON"],
     "paymentCenters" : "TOR",
     "resetCenters" : "LON"},
    {"calculationPeriodCenters" : "NY",
     "paymentCenters" : "LON",
     "resetCenters" : ["TOR","LON"]}
]
    )";

        auto j = jsoncons::json::parse(jtext);
        //std::cout << pretty_print(j) << "\n";

        auto options = csv::csv_options{}
            .flat(true)
            .subfield_delimiter(';');

        std::string buf;
        csv::csv_string_encoder encoder(buf, options);
        j.dump(encoder);

        CHECK(expected == buf);
    }    
#endif
}

