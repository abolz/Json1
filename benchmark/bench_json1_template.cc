#include "json1.h"
#include "../src/json_numbers.h"

#include "traverse.h"

using namespace json1;

namespace {

struct SaxHandler
{
    jsonstats& stats;

    SaxHandler(jsonstats& s) : stats(s) {}

    ParseStatus HandleNull(Options const& /*options*/)
    {
        ++stats.null_count;
        return {};
    }

    ParseStatus HandleBoolean(bool value, Options const& /*options*/)
    {
        if (value)
            ++stats.true_count;
        else
            ++stats.false_count;
        return {};
    }

    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc, Options const& /*options*/)
    {
        ++stats.number_count;
        stats.total_number_value += json::numbers::StringToNumber(first, last, static_cast<json::NumberClass>(nc));
        return {};
    }

    ParseStatus HandleString(char const* first, char const* last, bool needs_cleaning, Options const& /*options*/)
    {
        ++stats.string_count;
        if (needs_cleaning)
        {
            size_t len = 0;
            auto const res = json1::strings::UnescapeString(first, last, [&](char) { ++len; });

            if (res.status != json1::strings::Status::success)
                return ParseStatus::invalid_string;

            stats.total_string_length += len;
        }
        else
        {
            stats.total_string_length += static_cast<size_t>(last - first);
        }
        return {};
    }

    ParseStatus HandleBeginArray(Options const& /*options*/)
    {
        ++stats.array_count;
        return {};
    }

    ParseStatus HandleEndArray(size_t /*count*/, Options const& /*options*/)
    {
        //stats.total_array_length += count;
        return {};
    }

    ParseStatus HandleEndElement(size_t& /*count*/, Options const& /*options*/)
    {
        return {};
    }

    ParseStatus HandleBeginObject(Options const& /*options*/)
    {
        ++stats.object_count;
        return {};
    }

    ParseStatus HandleEndObject(size_t /*count*/, Options const& /*options*/)
    {
        //stats.total_object_length += count;
        return {};
    }

    ParseStatus HandleEndMember(size_t& /*count*/, Options const& /*options*/)
    {
        return {};
    }

    ParseStatus HandleKey(char const* first, char const* last, bool needs_cleaning, Options const& /*options*/)
    {
        ++stats.key_count;
        if (needs_cleaning)
        {
            size_t len = 0;
            auto const res = json1::strings::UnescapeString(first, last, [&](char) { ++len; });

            if (res.status != json1::strings::Status::success)
                return ParseStatus::invalid_string;
     
            stats.total_key_length += len;
        }
        else
        {
            stats.total_key_length += static_cast<size_t>(last - first);
        }
        return {};
    }
};

struct DomHandler
{
    static constexpr int kMaxElements = 120;
    static constexpr int kMaxMembers = 120;

    std::vector<json::Value> stack;
    std::vector<json::String> keys;

    ParseStatus HandleNull(Options const& /*options*/)
    {
        stack.emplace_back(nullptr);
        return {};
    }

    ParseStatus HandleBoolean(bool value, Options const& /*options*/)
    {
        stack.emplace_back(value);
        return {};
    }

    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc, Options const& options)
    {
        if (options.parse_numbers_as_strings)
            stack.emplace_back(json::string_tag, first, last);
        else
            stack.emplace_back(json::numbers::StringToNumber(first, last, static_cast<json::NumberClass>(nc)));

        return {};
    }

    ParseStatus HandleString(char const* first, char const* last, bool needs_cleaning, Options const& /*options*/)
    {
        if (needs_cleaning)
        {
            json::String str;
            str.reserve(static_cast<size_t>(last - first));

            auto const res = json1::strings::UnescapeString(first, last, [&](char ch) {
                str.push_back(ch);
            });

            if (res.status != json1::strings::Status::success)
                return ParseStatus::invalid_string;

            stack.emplace_back(std::move(str));
        }
        else
        {
            stack.emplace_back(json::string_tag, first, last);
        }

        return {};
    }

    ParseStatus HandleBeginArray(Options const& /*options*/)
    {
        stack.emplace_back(json::array_tag);
        return {};
    }

    ParseStatus HandleEndArray(size_t count, Options const& /*options*/)
    {
        PopElements(count);
        return {};
    }

    ParseStatus HandleEndElement(size_t& count, Options const& /*options*/)
    {
        JSON1_ASSERT(!stack.empty());
        JSON1_ASSERT(count != 0);

        if (count >= kMaxElements)
        {
            PopElements(count);
            count = 0;
        }

        return {};
    }

    ParseStatus HandleBeginObject(Options const& /*options*/)
    {
        stack.emplace_back(json::object_tag);
        return {};
    }

    ParseStatus HandleEndObject(size_t count, Options const& /*options*/)
    {
        PopMembers(count);
        return {};
    }

    ParseStatus HandleKey(char const* first, char const* last, bool needs_cleaning, Options const& /*options*/)
    {
        if (needs_cleaning)
        {
            keys.emplace_back();
            keys.back().reserve(static_cast<size_t>(last - first));

            auto const res = json1::strings::UnescapeString(first, last, [&](char ch) {
                keys.back().push_back(ch);
            });

            if (res.status != json1::strings::Status::success)
                return ParseStatus::invalid_string; // return ParseStatus::invalid_key;
        }
        else
        {
            keys.emplace_back(first, last);
        }

        return {};
    }

    ParseStatus HandleEndMember(size_t& count, Options const& /*options*/)
    {
        JSON1_ASSERT(!keys.empty());
        JSON1_ASSERT(!stack.empty());
        JSON1_ASSERT(count != 0);

        if (count >= kMaxMembers)
        {
            PopMembers(count);
            count = 0;
        }

        return {};
    }

private:
    ParseStatus PopElements(size_t num_elements)
    {
        if (num_elements == 0)
            return {};

        JSON1_ASSERT(num_elements <= SIZE_MAX - 1);
        JSON1_ASSERT(num_elements <= PTRDIFF_MAX);
        JSON1_ASSERT(stack.size() >= 1 + num_elements);

        auto const count = static_cast<std::ptrdiff_t>(num_elements);

        auto const I = stack.end() - count;
        auto const E = stack.end();

        auto& arr = I[-1].get_array();
        arr.insert(arr.end(), std::make_move_iterator(I), std::make_move_iterator(E));

        stack.erase(I, E);

        return {};
    }

    ParseStatus PopMembers(size_t num_members)
    {
        if (num_members == 0)
            return {};

        JSON1_ASSERT(num_members <= keys.size());
        JSON1_ASSERT(num_members <= SIZE_MAX - 1);
        JSON1_ASSERT(num_members <= PTRDIFF_MAX);
        JSON1_ASSERT(stack.size() >= 1 + num_members);
        JSON1_ASSERT(keys.size() >= num_members);

        auto const count = static_cast<std::ptrdiff_t>(num_members);

        auto const Iv = stack.end() - count;
        auto const Ik = keys.end() - count;

        auto& obj = Iv[-1].get_object();

        for (std::ptrdiff_t i = 0; i != count; ++i)
        {
            auto& K = Ik[i];
            auto& V = Iv[i];
            obj[std::move(K)] = std::move(V);
        }

        stack.erase(Iv, stack.end());
        keys.erase(Ik, keys.end());

        return {};
    }
};

} // namespace

bool json1_template_sax_stats(jsonstats& stats, char const* first, char const* last)
{
    SaxHandler handler(stats);
    auto const res = json1::Parse(handler, first, last);

    return res.ec == json1::ParseStatus::success;
}

bool json1_template_dom_stats(jsonstats& stats, char const* first, char const* last)
{
    DomHandler handler;
    auto const res = json1::Parse(handler, first, last);

    if (res.ec != json1::ParseStatus::success)
        return false;

    auto value = std::move(handler.stack.back());
    traverse(stats, value);
    return true;
}
