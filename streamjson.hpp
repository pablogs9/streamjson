// Copyright 2024 Pablo Garrido

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef STREAMJSON_HPP
#define STREAMJSON_HPP

#include <string>
#include <string_view>
#include <cstring>
#include <array>
#include <vector>
#include <functional>

namespace streamjson
{

/**
 * @class JsonValue
 *
 * @brief A simple class to hold a JSON value
*/
struct JsonValue
    {
        enum class Type
        {
            STRING,
            FLOATING,
            INTEGER,
            BOOLEAN,
            INVALID
        };

        Type type;

        std::string string;
        union
        {
            double floating;
            int64_t integer;
            bool boolean;
        };

        JsonValue(const char * string, size_t size)
        {
            bool success = parse_string(string, size);
            success = success || parse_number(string, size);
            success = success || parse_boolean(string, size);

            if (!success)
            {
                type = Type::INVALID;
            }
        }

        std::string to_string() const
        {
            switch (type)
            {
                case Type::STRING:
                    return string;
                case Type::FLOATING:
                    return std::to_string(floating);
                case Type::INTEGER:
                    return std::to_string(integer);
                case Type::BOOLEAN:
                    return boolean ? "true" : "false";
                default:
                    return "INVALID";
            }
        }

    protected:
        bool parse_string(const char * data, size_t size)
        {
            bool ret = false;

            // An string has to start and end with a quote
            int initial_quote_pos = -1;
            int end_qoute_pos = -1;

            for (size_t i = 0; i <= size; i++)
            {
                if (data[i] == '"')
                {
                    if (initial_quote_pos == -1)
                    {
                        initial_quote_pos = i;
                    }
                    else
                    {
                        end_qoute_pos = i;
                        break;
                    }
                }
            }

            if(initial_quote_pos != -1 && end_qoute_pos != -1)
            {
                data += initial_quote_pos + 1;
                size = end_qoute_pos - initial_quote_pos - 1;

                string = std::string(data, size);
                type = Type::STRING;
                ret = true;
            }

            return ret;
        }

        bool parse_boolean(const char * data, size_t size)
        {
            bool ret = false;

            // Try to find true or false along the string

            while (*data == ' ' && size > 0)
            {
                data++;
                size--;
            }

            const std::array<const char*, 6> values = {"true", "false", "True", "False", "TRUE", "FALSE"};
            const std::array<bool, 6> boolean_values = {true, false, true, false, true, false};

            for (size_t i = 0; i < values.size(); i++)
            {
                if (size >= strlen(values[i]) && strncmp(data, values[i], strlen(values[i])) == 0)
                {
                    boolean = boolean_values[i];
                    type = Type::BOOLEAN;
                    ret = true;
                    break;
                }
            }

            return ret;
        }

        bool parse_number(const char * data, size_t size)
        {
            bool ret = false;

            // Try to find a number
            double number = 0;
            bool negative = false;
            bool decimal = false;
            double decimal_pos = 0.1;
            bool error = false;

            // Skip spaces
            while (*data == ' ' && size > 0)
            {
                data++;
                size--;
            }

            for (size_t i = 0; i < size; i++)
            {
                if (data[i] == '-')
                {
                    negative = true;
                }
                else if (data[i] == '.')
                {
                    decimal = true;
                }
                else if (data[i] >= '0' && data[i] <= '9')
                {
                    if (decimal)
                    {
                        number += (data[i] - '0') * decimal_pos;
                        decimal_pos *= 0.1;
                    }
                    else
                    {
                        number = number * 10 + (data[i] - '0');
                    }
                }
                else if (data[i] == ' ')
                {
                    // End on space
                }
                else
                {
                    error = true;
                    break;
                }
            }

            if (!error)
            {
                if (decimal)
                {
                    type = Type::FLOATING;
                    this->floating = negative ? -number : number;
                }
                else
                {
                    type = Type::INTEGER;
                    this->integer = int64_t(negative ? -number : number);
                }
                ret = true;
            }

            return ret;
        }
};

/**
 * @class IJSONListener
 *
 * @brief Low level interface for JSON listeners
*/
struct IJSONListener
{
    virtual void on_object_start() {};
    virtual void on_object_end() {};
    virtual void on_array_start() {};
    virtual void on_array_end() {};
    virtual void on_array_next_element() {};
    virtual void on_key(const std::string_view & key) {};
    virtual void on_value(const JsonValue & value) {};
};

/**
 * @class JSONListener
 *
 * @brief A simple JSON listener that builds a aggregated key string
*/
struct JSONListener : public IJSONListener
{
    void on_object_start() override {

        aggregate_key_ += (aggregate_key_.size() ? "." : "");
        if (key_.empty())
        {
            aggregate_key_ += "_";
        }
        else
        {
            aggregate_key_ += std::string(key_.data(), key_.size());
        }

        key_.clear();
    };

    void on_object_end() override {
        size_t pos = aggregate_key_.rfind(".");

        if (pos != std::string::npos)
        {
            aggregate_key_ = aggregate_key_.substr(0, pos);
        }
        else
        {
            aggregate_key_.clear();
        }
    };

    void on_array_start() override {

        array_depth_.push_back(0);

        aggregate_key_ += (aggregate_key_.size() ? "." : "");

        if (key_.empty())
        {
            aggregate_key_ += "_";
        }
        else
        {
            aggregate_key_ += std::string(key_.data(), key_.size());
        }

        aggregate_key_ += "[" + std::to_string(array_depth_.back()) + "]";

        key_.clear();
    };

    void on_array_end() override {
        array_depth_.pop_back();

        size_t pos = aggregate_key_.rfind(".");

        if (pos != std::string::npos)
        {
            aggregate_key_ = aggregate_key_.substr(0, pos);
        }
        else
        {
            aggregate_key_.clear();
        }
    };

    void on_array_next_element() override {
        array_depth_.back()++;

        aggregate_key_ = aggregate_key_.substr(0, aggregate_key_.rfind("[")) + "[" + std::to_string(array_depth_.back()) + "]";
    };

    void on_key(const std::string_view& string) override {
        key_ = std::string(string.data(), string.size());
    };

    void on_value(const JsonValue & /* value */) override {
        if(!key_.empty())
        {
            // Got a key and a value
            key_.clear();
        }
    };

protected:
    std::string key_;
    std::string aggregate_key_;
    std::vector<size_t> array_depth_;
};

/**
 * @class FilterListener
 *
 * @brief A JSON listener that filters keys based on a string and calls a callback on match
*/
struct FilterListener : public JSONListener
{
    using CallBackType = std::function<void(const std::string & key, const JsonValue & value, const std::vector<size_t>& depths)>;

    FilterListener(const std::string & filter, CallBackType callback)
    : filter_(filter)
    , callback_(callback)
    {
    }

    void on_value(const JsonValue & value) override {

        // Find and replace "_."
        std::string query = aggregate_key_ + "." + std::string(key_.data(), key_.size());
        size_t pos = query.find("_.");
        while (pos != std::string::npos)
        {
            query.replace(pos, 2, "");
            pos = query.find("_.");
        }

        bool matched = true;

        const char * filter_ptr = filter_.c_str();
        const char * query_ptr = query.c_str();

        while(matched && filter_ptr < filter_.c_str() + filter_.size() && query_ptr < query.c_str() + query.size())
        {
            // If inside an array index, handle it
            if (*filter_ptr == '[' && *query_ptr == '[')
            {
                // Check that filter has a wildcard
                if (*(filter_ptr + 1) == '*')
                {
                    while(*filter_ptr != ']')
                    {
                        filter_ptr++;
                    }

                    while(*query_ptr != ']')
                    {
                        query_ptr++;
                    }

                    matched = matched && true;
                }
            }else if (*filter_ptr == *query_ptr || *filter_ptr == '*')
            {
                matched = matched && true;
            }
            else
            {
                matched = false;
                break;
            }

            filter_ptr++;
            query_ptr++;
        }

        if (matched)
        {
            callback_(query, value, array_depth_);
        }

        JSONListener::on_value(value);
    }

protected:

    const std::string filter_;
    CallBackType callback_;
};

/**
 * @class MultiListener
 *
 * @brief A JSON listener that acts as a multiplexer
*/
class MultiListener : public IJSONListener
{
public:
    MultiListener() = default;

    MultiListener(std::initializer_list<IJSONListener *> listeners)
    : listeners_(listeners)
    {
    }

    void on_object_start() override
    {
        for (auto listener : listeners_)
        {
            listener->on_object_start();
        }
    };
    void on_object_end() override
    {
        for (auto listener : listeners_)
        {
            listener->on_object_end();
        }
    };
    void on_array_start() override
    {
        for (auto listener : listeners_)
        {
            listener->on_array_start();
        }
    };
    void on_array_end() override
    {
        for (auto listener : listeners_)
        {
            listener->on_array_end();
        }
    };
    void on_array_next_element() override
    {
        for (auto listener : listeners_)
        {
            listener->on_array_next_element();
        }
    };
    void on_key(const std::string_view& key) override
    {
        for (auto listener : listeners_)
        {
            listener->on_key(key);
        }
    };
    void on_value(const JsonValue& value) override
    {
        for (auto listener : listeners_)
        {
            listener->on_value(value);
        }
    };

    void add_listener(IJSONListener & listener)
    {
        listeners_.push_back(&listener);
    }

private:
    std::vector<IJSONListener *> listeners_;
};

/**
 * @class StreamJson
 *
 * @brief A JSON parser that can be fed with chunks of data
*/
class StreamJson {
public:
    StreamJson()
    : listener_(&dummy_listener_)
    {
    }

    StreamJson(IJSONListener & listener)
    : listener_(&listener)
    {
    }

    size_t feed(const char * chunk, size_t size, size_t offset = 0)
    {
        if (offset != 0)
        {
            value_start_ = chunk;
            value_size_ = offset - 1;
        }

        for (size_t i = offset; i < size; i++)
        {
            const char & c = *(chunk + i);

            Token token = get_token(c);

            value_size_++;

            // If we are processing a string, the only valid token is the quote
            if (!state_stack_.empty() && state_stack_.back() == State::IN_STRING && token != Token::QUOTE)
            {
                continue;
            }

            switch (token)
            {
                case Token::QUOTE:
                    // If we are in a string, we are going to end it
                    if (state_stack_.back() == State::IN_STRING)
                    {
                        state_stack_.pop_back();

                        // If this string is after a colon, it is a value
                        if (after_colon_)
                        {
                            after_colon_ = false;
                            listener_->on_value(JsonValue(value_start_, value_size_));
                        }
                        else
                        {
                            listener_->on_key(std::string_view(value_start_ + 1, value_size_ - 1));
                        }

                        value_start_ = nullptr;
                        value_size_ = 0;
                    }
                    else
                    {
                        value_start_ = &c;
                        value_size_ = 0;
                        state_stack_.push_back(State::IN_STRING);
                    }
                    break;
                case Token::OBJECT_START:
                    after_colon_ = false;
                    listener_->on_object_start();
                    state_stack_.push_back(State::IN_OBJECT);
                    break;
                case Token::OBJECT_END:
                    if( after_colon_)
                    {
                        listener_->on_value(JsonValue(value_start_, value_size_ - 1));
                    }
                    after_colon_ = false;
                    if(state_stack_.back() == State::IN_OBJECT)
                    {
                        listener_->on_object_end();
                        state_stack_.pop_back();
                    }
                    value_start_ = nullptr;
                    value_size_ = 0;
                    break;
                case Token::ARRAY_START:
                    after_colon_ = false;
                    value_start_ = &c + 1;
                    value_size_ = 0;
                    listener_->on_array_start();
                    state_stack_.push_back(State::IN_ARRAY);
                    break;
                case Token::ARRAY_END:
                    after_colon_ = false;

                    if(value_start_ != nullptr)
                    {
                        listener_->on_value(JsonValue(value_start_, value_size_ - 1));
                        value_start_ = nullptr;
                        value_size_ = 0;
                    }

                    if(state_stack_.back() == State::IN_ARRAY)
                    {
                        listener_->on_array_end();
                        state_stack_.pop_back();
                    }
                    break;
                case Token::COLON:
                    after_colon_ = true;
                    value_start_ = &c + 1;
                    value_size_ = 0;
                    break;
                case Token::COMMA:
                    // Maybe we found a value
                    if( after_colon_)
                    {
                        listener_->on_value(JsonValue(value_start_, value_size_ - 1));
                        value_start_ = nullptr;
                        value_size_ = 0;
                    }
                    else if (state_stack_.back() == State::IN_ARRAY && value_start_ != nullptr)
                    {
                        listener_->on_value(JsonValue(value_start_, value_size_ - 1));
                        value_start_ = &c + 1;
                        value_size_ = 0;
                    }

                    if (state_stack_.back() == State::IN_ARRAY)
                    {
                        listener_->on_array_next_element();
                    }

                    after_colon_ = false;
                    break;
                case Token::NONE:
                    break;
                default:
                    break;
            }
        }

        // Return the required point to keep
        size_t allow_to_remove = value_start_ ? value_start_ - chunk : size;

        return allow_to_remove;
    }

    virtual void reset(IJSONListener & listener)
    {
        listener_ = &listener;
        state_stack_.clear();
        after_colon_ = false;
        value_start_ = nullptr;
        value_size_ = 0;

    }

protected:

    enum class State : uint8_t
    {
        IN_OBJECT,
        IN_ARRAY,
        IN_STRING,
    };

    enum class Token
    {
        OBJECT_START,
        OBJECT_END,
        ARRAY_START,
        ARRAY_END,
        QUOTE,
        COLON,
        COMMA,
        NONE
    };

    using TokenMapType = std::array<std::pair<Token, char>, static_cast<size_t>(Token::NONE)>;
    const TokenMapType token_map_ = {
        std::make_pair(Token::OBJECT_START, '{'),
        std::make_pair(Token::OBJECT_END, '}'),
        std::make_pair(Token::ARRAY_START, '['),
        std::make_pair(Token::ARRAY_END, ']'),
        std::make_pair(Token::QUOTE, '"'),
        std::make_pair(Token::COLON, ':'),
        std::make_pair(Token::COMMA, ',')
    };

    Token get_token(const char c)
    {
        for (const auto & pair : token_map_)
        {
            if (pair.second == c)
            {
                return pair.first;
            }
        }

        return Token::NONE;
    }

    IJSONListener * listener_;
    IJSONListener dummy_listener_;

    // State variables
    std::vector<State> state_stack_;
    bool after_colon_ = false;
    char const * value_start_ = nullptr;
    size_t value_size_ = 0;
};

/**
 * @class AutofeedStreamJson
 *
 * @brief A JSON parser that can be fed with chunks of data and automatically calls the feed method
*/
template<size_t CHUNK_SIZE>
class AutofeedStreamJson : public StreamJson
{
public:
    AutofeedStreamJson()
    : StreamJson()
    {
    }

    AutofeedStreamJson(IJSONListener & listener)
    : StreamJson(listener)
    {
    }

    void feed(const char * chunk, size_t size)
    {
        if(!failed_)
        {
            // Copy new data to the buffer
            memcpy(buffer_.data() + next_offset_, chunk, size);

            // Actual size of the buffer
            size += next_offset_;

            // Feed the buffer
            size_t to_remove = StreamJson::feed(buffer_.data(), size, next_offset_);

            // Remove the processed data
            memmove(buffer_.data(), buffer_.data() + to_remove, size - to_remove + 1);
            next_offset_ = size - to_remove;

            if (next_offset_ >= CHUNK_SIZE)
            {
                failed_ = true;
            }
        }
    };

    void reset(IJSONListener & listener)
    {
        StreamJson::reset(listener);
        next_offset_ = 0;
        failed_ = false;
    }

protected:

    std::array<char, CHUNK_SIZE> buffer_;
    size_t next_offset_ = 0;
    bool failed_ = false;
};

} // namespace streamjson

#endif // STREAMJSON_HPP