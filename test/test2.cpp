
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <streamjson.hpp>

const std::string json = " \
{ \"owners\": [ \
        {\"name\": \"John\", \
        \"age\": 30, \
        \"cars\":  [ \
            \"name\": \"Ford\", \
            \"name\": \"BMW\", \
        ] }, \
        {\"name\": \"Jane\", \
        \"age\": 25, \
        \"cars\":  [ \
            \"name\": \"Audi\", \
            \"name\": \"Mercedes\" \
            \"name\": \"Fiat\" \
        ] } \
    ] \
}";

int main(int argc, char* argv[] )
{
    std::string name;

    streamjson::FilterListener<"owners\\[[0-9]+\\]\\.name"> name_filter([&](const std::string & key, const streamjson::JSONValue & value, const std::vector<size_t> & indexes)
    {
        name = value.to_string();
    });


    streamjson::FilterListener<"owners\\[[0-9]+\\]\\.cars\\[[0-9]+\\]\\.name"> car_filter([&](const std::string & key, const streamjson::JSONValue & value, const std::vector<size_t> & indexes)
    {
        std::cout << name << " has a " << value.to_string() << std::endl;
    });

    streamjson::MultiListener multi_filter({&name_filter, &car_filter});

    // Feed the file by chunks
    constexpr size_t BUFFER_SIZE = 30;
    streamjson::AutofeedStreamJson<BUFFER_SIZE> chunk_parser(multi_filter);

    constexpr size_t CHUNK_SIZE = 10;
    const char * ptr = json.data();
    while (ptr < (json.data() + json.size()))
    {
        size_t actual_size = std::min(CHUNK_SIZE, json.size() - (ptr - json.data()));
        chunk_parser.feed(ptr, actual_size);
        ptr += actual_size;
    }

    return 0;
}
