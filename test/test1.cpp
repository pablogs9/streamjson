
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <streamjson.hpp>

int main(int argc, char* argv[] )
{
    std::ifstream file("/home/pgarrido/dev/pgarrido/streamjson/out");
    std::string str;
    std::string file_contents;
    while (std::getline(file, str))
    {
        file_contents += str;
    }

    streamjson::FilterListener<"jobs\\[[0-9]+\\]\\.conclusion"> conclusion_filter([](const std::string_view & key, const streamjson::JSONValue & value, const std::vector<size_t> & indexes)
    {
        std::cout << key << " : " << value.to_string() << std::endl;
    });

    streamjson::FilterListener<"jobs\\[[0-9]+\\]\\.status"> status_filter([](const std::string_view & key, const streamjson::JSONValue & value, const std::vector<size_t> & indexes)
    {
        std::cout << key << " : " << value.to_string() << std::endl;
    });

    streamjson::MultiListener multi_filter({&conclusion_filter, &status_filter});

    // Feed the file by chunks
    streamjson::AutofeedStreamJson<1024> chunk_parser(multi_filter);

    const char * ptr = file_contents.data();
    while (ptr < (file_contents.data() + file_contents.size()))
    {
        size_t actual_size = std::min(100UL, file_contents.size() - (ptr - file_contents.data()));
        chunk_parser.feed(ptr, actual_size);
        ptr += actual_size;
    }

    // Feed the file at once
    streamjson::StreamJson complete_parser(multi_filter);
    complete_parser.feed(file_contents.data(), file_contents.size());

    return 0;
}
