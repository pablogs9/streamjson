# streamjson

`streamjson` is a C++20 library for reading JSON streams by chunks.

Its primary objective is to be used in embedded scenarios where the whole JSON document is not available at once (e.g. network streams, file streams, etc.).
It is designed to be memory efficient, fast and simple.

The library is header-only and has a single dependency on [hanickadot/compile-time-regular-expressions](https://github.com/hanickadot/compile-time-regular-expressions/tree) for compile-time regular expressions, this dependency is also header-only and introduces the C++20 requirement.

## Example

```cpp
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

    streamjson::FilterListener<"owners\\[[0-9]+\\]\\.name"> name_filter([&](const std::string_view & key, const streamjson::JSONValue & value, const std::vector<size_t> & indexes)
    {
        name = value.to_string();
    });


    streamjson::FilterListener<"owners\\[[0-9]+\\]\\.cars\\[[0-9]+\\]\\.name"> car_filter([&](const std::string_view & key, const streamjson::JSONValue & value, const std::vector<size_t> & indexes)
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
```