
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <streamjson.hpp>

const char sample[] = " \
{ \
    \"name\": \"John\", \
    \"age\": 30, \
    \"cars\": { \
        \"car1\": \"Ford\", \
        \"car2\": \"BMW\", \
        \"car3\": \"Fiat\" \
    } \
}";

int main(int argc, char* argv[] )
{
    std::ifstream file("/home/pgarrido/dev/pgarrido/streamjson/out");
    std::string str;
    std::string file_contents;
    while (std::getline(file, str))
    {
        file_contents += str;
    }

    streamjson::FilterListener filter("jobs[*].conclusion", [](const std::string & key, const streamjson::JsonValue & value, const std::vector<size_t> & indexes)
    {
        std::cout << key << " : " << value.to_string() << std::endl;
    });


    streamjson::FilterListener filter2("jobs[*].status", [](const std::string & key, const streamjson::JsonValue & value, const std::vector<size_t> & indexes)
    {
        std::cout << key << " : " << value.to_string() << std::endl;
    });

    std::cout << "AutofeedStreamJson" << std::endl;
    // streamjson::AutofeedStreamJson<1024> parser(filter);
    streamjson::MultiListener multi_listener({&filter, &filter2});
    streamjson::StreamJson parser(multi_listener);

    parser.feed(file_contents.data(), file_contents.size());

    // const char * ptr = file_contents.data();
    // while (ptr < (file_contents.data() + file_contents.size()))
    // {
    //     size_t actual_size = std::min(1024UL, file_contents.size() - (ptr - file_contents.data()));
    //     parser.feed(ptr, actual_size);
    //     ptr += actual_size;
    // }

    return 0;
}
