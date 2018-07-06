#include "bench_nlohmann.h"

#include "../ext/nlohmann/json.hpp"

#include "traverse.h"

namespace {

class SaxHandler : public nlohmann::json_sax<nlohmann::json>
{
    jsonstats& stats;
    bool errored;

public:
    using BasicJsonType = nlohmann::json;
    using number_integer_t = BasicJsonType::number_integer_t;
    using number_unsigned_t = BasicJsonType::number_unsigned_t;
    using number_float_t = BasicJsonType::number_float_t;
    using string_t = BasicJsonType::string_t;

    SaxHandler(jsonstats& s)
        : stats(s)
        , errored(false)
    {
    }

    bool null() override
    {
        ++stats.null_count;
        return true;
    }

    bool boolean(bool val) override
    {
        if (val)
            ++stats.true_count;
        else
            ++stats.false_count;
        return true;
    }

    bool number_integer(number_integer_t val) override
    {
        ++stats.number_count;
        stats.total_number_value += static_cast<double>(val);
        return true;
    }

    bool number_unsigned(number_unsigned_t val) override
    {
        ++stats.number_count;
        stats.total_number_value += static_cast<double>(val);
        return true;
    }

    bool number_float(number_float_t val, const string_t&) override
    {
        ++stats.number_count;
        stats.total_number_value += val;
        return true;
    }

    bool string(string_t& val) override
    {
        ++stats.string_count;
        stats.total_string_length += val.size();
        return true;
    }

    bool start_object(std::size_t /*len*/) override
    {
        ++stats.object_count;
        return true;
    }

    bool key(string_t& val) override
    {
        ++stats.key_count;
        stats.total_key_length += val.size();
        return true;
    }

    bool end_object() override
    {
        // XXX:
        return true;
    }

    bool start_array(std::size_t /*len*/) override
    {
        ++stats.array_count;
        return true;
    }

    bool end_array() override
    {
        // XXX:
        return true;
    }

    bool parse_error(std::size_t, const std::string&, const nlohmann::detail::exception& /*ex*/) override
    {
        errored = true;
        return false;
    }

    constexpr bool is_errored() const
    {
        return errored;
    }
};

class DomHandler : public nlohmann::json_sax<nlohmann::json>
{
public:
    using BasicJsonType = nlohmann::json;
    using number_integer_t = BasicJsonType::number_integer_t;
    using number_unsigned_t = BasicJsonType::number_unsigned_t;
    using number_float_t = BasicJsonType::number_float_t;
    using string_t = BasicJsonType::string_t;

    DomHandler(json::Value& r)
        : root(r)
    {}

    bool null() override
    {
        handle_value(nullptr);
        return true;
    }

    bool boolean(bool val) override
    {
        handle_value(val);
        return true;
    }

    bool number_integer(number_integer_t val) override
    {
        handle_value(static_cast<double>(val));
        return true;
    }

    bool number_unsigned(number_unsigned_t val) override
    {
        handle_value(static_cast<double>(val));
        return true;
    }

    bool number_float(number_float_t val, const string_t&) override
    {
        handle_value(static_cast<double>(val));
        return true;
    }

    bool string(string_t& val) override
    {
        handle_value(val);
        return true;
    }

    bool start_object(std::size_t /*len*/) override
    {
        ref_stack.push_back(handle_value(json::object_tag));
        return true;
    }

    bool key(string_t& val) override
    {
        object_element = &(ref_stack.back()->get_object().operator[](val));
        return true;
    }

    bool end_object() override
    {
        ref_stack.pop_back();
        return true;
    }

    bool start_array(std::size_t /*len*/) override
    {
        ref_stack.push_back(handle_value(json::array_tag));
        return true;
    }

    bool end_array() override
    {
        ref_stack.pop_back();
        return true;
    }

    bool parse_error(std::size_t, const std::string&, const nlohmann::detail::exception&) override
    {
        errored = true;
        return false;
    }

    constexpr bool is_errored() const
    {
        return errored;
    }

  private:
    template<typename Value>
    json::Value* handle_value(Value&& v)
    {
        if (ref_stack.empty())
        {
            root = json::Value(std::forward<Value>(v));
            return &root;
        }
        else
        {
            assert(ref_stack.back()->is_array() or ref_stack.back()->is_object());
            if (ref_stack.back()->is_array())
            {
                ref_stack.back()->get_array().emplace_back(std::forward<Value>(v));
                return &(ref_stack.back()->get_array().back());
            }
            else
            {
                assert(object_element);
                *object_element = json::Value(std::forward<Value>(v));
                return object_element;
            }
        }
    }

    json::Value& root;
    std::vector<json::Value*> ref_stack;
    json::Value* object_element = nullptr;
    bool errored = false;
};

} // namespace

bool nlohmann_sax_stats(jsonstats& stats, char const* next, char const* last)
{
    SaxHandler handler(stats);

    nlohmann::json::sax_parse(next, last, &handler);

    return !handler.is_errored();
}

bool nlohmann_dom_stats(jsonstats& stats, char const* next, char const* last)
{
    json::Value value;
    DomHandler handler(value);

    nlohmann::json::sax_parse(next, last, &handler);

    if (handler.is_errored())
        return false;

    traverse(stats, value);
    return true;
}
