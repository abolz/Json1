// Copyright 2018 Alexander Bolz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "json.h"
#include "json_numbers.h"
#include "json_strings.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>

using namespace json;

//==================================================================================================
// Value
//==================================================================================================

Value const Value::kUndefined = {};

Value::Value(Value const& rhs)
{
    switch (rhs.type_)
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_ = rhs.data_;
        type_ = rhs.type_;
        break;
    case Type::string:
        data_.string = new String(*rhs.data_.string);
        type_ = Type::string;
        break;
    case Type::array:
        data_.array = new Array(*rhs.data_.array);
        type_ = Type::array;
        break;
    case Type::object:
        data_.object = new Object(*rhs.data_.object);
        type_ = Type::object;
        break;
    }
}

Value::Value(Type t)
{
    switch (t)
    {
    case Type::undefined:
    case Type::null:
        break;
    case Type::boolean:
        data_.boolean = {};
        break;
    case Type::number:
        data_.number = {};
        break;
    case Type::string:
        data_.string = new String{};
        break;
    case Type::array:
        data_.array = new Array{};
        break;
    case Type::object:
        data_.object = new Object{};
        break;
    default:
        JSON_ASSERT(false && "invalid type"); // LCOV_EXCL_LINE
        t = Type::undefined;
        break;
    }

    type_ = t; // Don't move to constructor initializer list!
}

void Value::assign(Tag_undefined) noexcept
{
    _clear();

    type_ = Type::undefined;
}

void Value::assign(Tag_null, Null) noexcept
{
    _clear();

    type_ = Type::null;
}

bool& Value::assign(Tag_boolean, bool v) noexcept
{
    _clear();

    data_.boolean = v;
    type_ = Type::boolean;

    return get_boolean();
}

double& Value::assign(Tag_number, double v) noexcept
{
    _clear();

    data_.number = v;
    type_ = Type::number;

    return get_number();
}

String& Value::assign(Tag_string)
{
    return _assign_string(String{});
}

String& Value::assign(Tag_string, String const& v)
{
    return _assign_string(v);
}

String& Value::assign(Tag_string, String&& v)
{
    return _assign_string(std::move(v));
}

Array& Value::assign(Tag_array)
{
    return _assign_array(Array{});
}

Array& Value::assign(Tag_array, Array const& v)
{
    return _assign_array(v);
}

Array& Value::assign(Tag_array, Array&& v)
{
    return _assign_array(std::move(v));
}

Object& Value::assign(Tag_object)
{
    return _assign_object(Object{});
}

Object& Value::assign(Tag_object, Object const& v)
{
    return _assign_object(v);
}

Object& Value::assign(Tag_object, Object&& v)
{
    return _assign_object(std::move(v));
}

Value& Value::operator=(Value const& rhs)
{
    if (this != &rhs)
    {
        switch (rhs.type())
        {
        case Type::undefined:
            assign(undefined_tag);
            break;
        case Type::null:
            assign(null_tag);
            break;
        case Type::boolean:
            assign(boolean_tag, rhs.get_boolean());
            break;
        case Type::number:
            assign(number_tag, rhs.get_number());
            break;
        case Type::string:
            assign(string_tag, rhs.get_string());
            break;
        case Type::array:
            assign(array_tag, rhs.get_array());
            break;
        case Type::object:
            assign(object_tag, rhs.get_object());
            break;
        }
    }

    return *this;
}

void Value::_clear_allocated()
{
    switch (type_)
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        JSON_ASSERT(false && "invalid function call");
        break;
    case Type::string:
        delete data_.string;
        break;
    case Type::array:
        delete data_.array;
        break;
    case Type::object:
        delete data_.object;
        break;
    }

    type_ = Type::undefined;
}

template <typename T>
String& Value::_assign_string(T&& value)
{
    switch (type_)
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_.string = new String(std::forward<T>(value));
        type_ = Type::string;
        break;
    case Type::string:
        *data_.string = std::forward<T>(value);
        break;
    case Type::array:
        {
            auto p = new String(std::forward<T>(value));
            // noexcept ->
            delete data_.array;
            data_.string = p;
            type_ = Type::string;
        }
        break;
    case Type::object:
        {
            auto p = new String(std::forward<T>(value));
            // noexcept ->
            delete data_.object;
            data_.string = p;
            type_ = Type::string;
        }
        break;
    }

    return get_string();
}

template <typename T>
Array& Value::_assign_array(T&& value)
{
    switch (type_)
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_.array = new Array(std::forward<T>(value));
        type_ = Type::array;
        break;
    case Type::string:
        {
            auto p = new Array(std::forward<T>(value));
            // noexcept ->
            delete data_.string;
            data_.array = p;
            type_ = Type::array;
        }
        break;
    case Type::array:
        *data_.array = std::forward<T>(value);
        break;
    case Type::object:
        {
            auto p = new Array(std::forward<T>(value));
            // noexcept ->
            delete data_.object;
            data_.array = p;
            type_ = Type::array;
        }
        break;
    }

    return get_array();
}

template <typename T>
Object& Value::_assign_object(T&& value)
{
    switch (type_)
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_.object = new Object(std::forward<T>(value));
        type_ = Type::object;
        break;
    case Type::string:
        {
            auto p = new Object(std::forward<T>(value));
            // noexcept ->
            delete data_.string;
            data_.object = p;
            type_ = Type::object;
        }
        break;
    case Type::array:
        {
            auto p = new Object(std::forward<T>(value));
            // noexcept ->
            delete data_.array;
            data_.object = p;
            type_ = Type::object;
        }
        break;
    case Type::object:
        *data_.object = std::forward<T>(value);
        break;
    }

    return get_object();
}

bool Value::equal_to(Value const& rhs) const noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (is_undefined() || rhs.is_undefined())
        return false;
#endif

    if (this == &rhs)
        return true;

    if (type() != rhs.type())
        return false;

    switch (type())
    {
    case Type::undefined:
        return true;
    case Type::null:
        return true;
    case Type::boolean:
        return get_boolean() == rhs.get_boolean();
    case Type::number:
        return get_number() == rhs.get_number();
    case Type::string:
        return get_string() == rhs.get_string();
    case Type::array:
        return get_array() == rhs.get_array();
    case Type::object:
        return get_object() == rhs.get_object();
    default:
        JSON_ASSERT(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

bool Value::less_than(Value const& rhs) const noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (is_undefined() || rhs.is_undefined())
        return false;
#endif

    if (this == &rhs)
        return false;

    if (type() < rhs.type())
        return true;
    if (type() > rhs.type())
        return false;

    switch (type())
    {
    case Type::undefined:
        return false;
    case Type::null:
        return false;
    case Type::boolean:
        return get_boolean() < rhs.get_boolean();
    case Type::number:
        return get_number() < rhs.get_number();
    case Type::string:
        return get_string() < rhs.get_string();
    case Type::array:
        return get_array() < rhs.get_array();
    case Type::object:
        return get_object() < rhs.get_object();
    default:
        JSON_ASSERT(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

static size_t HashCombine(size_t h1, size_t h2) noexcept
{
#if SIZE_MAX == UINT32_MAX
    h1 ^= h2 + 0x9E3779B9 + (h1 << 6) + (h1 >> 2);
#else
    h1 ^= h2 + 0x9E3779B97F4A7C15 + (h1 << 6) + (h1 >> 2);
#endif
    return h1;
}

size_t Value::hash() const noexcept
{
    switch (type())
    {
    case Type::undefined:
        JSON_ASSERT(false && "cannot compute hash value for 'undefined'");
        return std::numeric_limits<size_t>::max(); // -1
    case Type::null:
        return 0;
    case Type::boolean:
        return std::hash<bool>()(get_boolean());
    case Type::number:
        return std::hash<double>()(get_number());
    case Type::string:
        return std::hash<String>()(get_string());
    case Type::array:
        {
            size_t h = std::hash<char>()('['); // initial value for empty arrays
            for (auto const& v : get_array())
            {
                h = HashCombine(h, v.hash());
            }
            return h;
        }
    case Type::object:
        {
            size_t h = std::hash<char>()('{'); // initial value for empty objects
            for (auto const& v : get_object())
            {
                auto const h1 = std::hash<String>()(v.first);
                auto const h2 = v.second.hash();
                h ^= HashCombine(h1, h2); // Permutation resistant to support unordered maps.
            }
            return h;
        }
    default:
        JSON_ASSERT(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

void Value::swap(Value& rhs) noexcept
{
    std::swap(data_, rhs.data_);
    std::swap(type_, rhs.type_);
}

size_t Value::size() const noexcept
{
    switch (type())
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        JSON_ASSERT(false && "cannot read property 'size' of undefined, null, boolean or number"); // LCOV_EXCL_LINE
        return 0;
    case Type::string:
        return get_string().size();
    case Type::array:
        return get_array().size();
    case Type::object:
        return get_object().size();
    default:
        JSON_ASSERT(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

bool Value::empty() const noexcept
{
    switch (type())
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        JSON_ASSERT(false && "cannot read property 'empty' of undefined, null, boolean or number"); // LCOV_EXCL_LINE
        return true; // i.e. size() == 0
    case Type::string:
        return get_string().empty();
    case Type::array:
        return get_array().empty();
    case Type::object:
        return get_object().empty();
    default:
        JSON_ASSERT(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

Array& Value::_get_or_assign_array()
{
    JSON_ASSERT(is_undefined() || is_array());
    if (!is_array()) {
        assign(array_tag);
    }
    return get_array();
}

Value& Value::operator[](size_t index)
{
    auto& arr = _get_or_assign_array();
    if (arr.size() <= index)
    {
        JSON_ASSERT(index < SIZE_MAX);
        arr.resize(index + 1);
    }
    return arr[index];
}

Value const& Value::operator[](size_t index) const noexcept
{
#if JSON_VALUE_ALLOW_UNDEFINED_ACCESS
    JSON_ASSERT(is_undefined() || is_array());
    if (is_array())
#endif
    {
        auto& arr = get_array();
        if (index < arr.size()) {
            return arr[index];
        }
    }
    return kUndefined;
}

Value* Value::get_ptr(size_t index)
{
    if (is_array())
    {
        auto& arr = get_array();
        if (index < arr.size()) {
            return &arr[index];
        }
    }
    return nullptr;
}

Value const* Value::get_ptr(size_t index) const
{
    if (is_array())
    {
        auto& arr = get_array();
        if (index < arr.size()) {
            return &arr[index];
        }
    }
    return nullptr;
}

void Value::pop_back()
{
    get_array().pop_back();
}

Value::element_iterator Value::erase(size_t index)
{
    auto& arr = get_array();
    JSON_ASSERT(index < arr.size());
    return arr.erase(arr.begin() + static_cast<intptr_t>(index));
}

Value::element_iterator Value::erase(const_element_iterator pos)
{
    auto& arr = get_array();
    return arr.erase(pos);
}

Value::element_iterator Value::erase(const_element_iterator first, const_element_iterator last)
{
    auto& arr = get_array();
    return arr.erase(first, last);
}

Object& Value::_get_or_assign_object()
{
    JSON_ASSERT(is_undefined() || is_object());
    if (!is_object()) {
        assign(object_tag);
    }
    return get_object();
}

Value& Value::operator[](Object::key_type const& key)
{
    return _get_or_assign_object()[key];
}

Value& Value::operator[](Object::key_type&& key)
{
    return _get_or_assign_object()[std::move(key)];
}

Value::item_iterator Value::erase(const_item_iterator pos)
{
    auto& obj = get_object();
    return obj.erase(pos);
}

Value::item_iterator Value::erase(const_item_iterator first, const_item_iterator last)
{
    auto& obj = get_object();
    return obj.erase(first, last);
}

bool Value::to_boolean() const noexcept
{
    switch (type())
    {
    case Type::undefined:
        return false;
    case Type::null:
        return false;
    case Type::boolean:
        return get_boolean();
    case Type::number:
        {
            auto v = get_number();
            return !std::isnan(v) && v != 0.0;
        }
    case Type::string:
        return !get_string().empty();
    case Type::array:
    case Type::object:
        JSON_ASSERT(false && "to_boolean must not be called for arrays or objects"); // LCOV_EXCL_LINE
        return {};
    default:
        JSON_ASSERT(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

double Value::to_number() const noexcept
{
    switch (type())
    {
    case Type::undefined:
        return std::numeric_limits<double>::quiet_NaN();
    case Type::null:
        return 0.0;
    case Type::boolean:
        return get_boolean() ? 1.0 : 0.0;
    case Type::number:
        return get_number();
    case Type::string:
        {
            auto const& str = get_string();
            double result;
            json::numbers::StringToNumber(result, str.c_str(), str.c_str() + str.size());
            return result;
        }
    case Type::array:
    case Type::object:
        JSON_ASSERT(false && "to_number must not be called for arrays or objects"); // LCOV_EXCL_LINE
        return {};
    default:
        JSON_ASSERT(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

double Value::to_integer() const noexcept
{
    auto const v = to_number();
    if (std::isnan(v)) {
        return 0.0;
    }
    if (std::isinf(v) || v == 0.0) {
        return v;
    }

    return std::trunc(v);
}

// The notation "x modulo y" (y must be finite and nonzero) computes
// a value k of the same sign as y (or zero)
// such that abs(k) < abs(y) and x-k = q * y for some integer q.
static double Modulo(double x, double y)
{
    JSON_ASSERT(std::isfinite(x));
    JSON_ASSERT(std::isfinite(y) && y > 0.0);

    double m = std::fmod(x, y);
    if (m < 0.0) {
        m += y;
    }

    return m; // m in [-0.0, y)
}

int32_t Value::to_int32() const noexcept
{
    constexpr double kTwo32 = 4294967296.0;
    constexpr double kTwo31 = 2147483648.0;

    auto const v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto k = Modulo(std::trunc(v), kTwo32);
    if (k >= kTwo31) {
        k -= kTwo32;
    }

    return static_cast<int32_t>(k);
}

uint32_t Value::to_uint32() const noexcept
{
    constexpr double kTwo32 = 4294967296.0;

    auto const v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto k = Modulo(std::trunc(v), kTwo32);

    return static_cast<uint32_t>(k);
}

int16_t Value::to_int16() const noexcept
{
    constexpr double kTwo16 = 65536.0;
    constexpr double kTwo15 = 32768.0;

    auto const v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto k = Modulo(std::trunc(v), kTwo16);
    if (k >= kTwo15) {
        k -= kTwo16;
    }

    return static_cast<int16_t>(k);
}

uint16_t Value::to_uint16() const noexcept
{
    constexpr double kTwo16 = 65536.0;

    auto const v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto k = Modulo(std::trunc(v), kTwo16);

    return static_cast<uint16_t>(k);
}

int8_t Value::to_int8() const noexcept
{
    constexpr double kTwo8 = 256.0;
    constexpr double kTwo7 = 128.0;

    auto const v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto k = Modulo(std::trunc(v), kTwo8);
    if (k >= kTwo7) {
        k -= kTwo8;
    }

    return static_cast<int8_t>(k);
}

uint8_t Value::to_uint8() const noexcept
{
    constexpr double kTwo8 = 256.0;

    auto const v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto k = Modulo(std::trunc(v), kTwo8);

    return static_cast<uint8_t>(k);
}

uint8_t Value::to_uint8_clamped() const noexcept
{
    auto v = to_number();
    if (std::isnan(v)) {
        return 0;
    }

    if (v <= 0.0) { // NB: -0 => +0
        v = 0.0;
    }
    if (v >= 255.0) {
        v = 255.0;
    }

    auto f = std::floor(v);
    if (f + 0.5 < v) {
        // round up
        f += 1.0;
    }
    else if (v < f + 0.5) {
        // round down
    }
    else if ((static_cast<int>(f) & 1) != 0) {
        // round to even
        f += 1.0;
    }

    return static_cast<uint8_t>(f);
}

String Value::to_string() const
{
    switch (type())
    {
    case Type::undefined:
        return "undefined";
    case Type::null:
        return "null";
    case Type::boolean:
        return get_boolean() ? "true" : "false";
    case Type::number:
        {
            char buf[32];
            char* end = numbers::NumberToString(buf, 32, get_number(), /*force_trailing_dot_zero*/ false);
            return String(buf, end);
        }
    case Type::string:
        return get_string();
    case Type::array:
    case Type::object:
        JSON_ASSERT(false && "to_string must not be called for arrays or objects"); // LCOV_EXCL_LINE
        return {};
    default:
        JSON_ASSERT(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

//==================================================================================================
// parse
//==================================================================================================

struct ParseValueCallbacks
{
    static constexpr int kMaxElements = 120;
    static constexpr int kMaxMembers = 120;

    std::vector<Value> stack;
    std::vector<String> keys;

    ParseStatus HandleNull(char const* /*first*/, char const* /*last*/, Options const& /*options*/)
    {
        stack.emplace_back(nullptr);
        return {};
    }

    ParseStatus HandleTrue(char const* /*first*/, char const* /*last*/, Options const& /*options*/)
    {
        stack.emplace_back(true);
        return {};
    }

    ParseStatus HandleFalse(char const* /*first*/, char const* /*last*/, Options const& /*options*/)
    {
        stack.emplace_back(false);
        return {};
    }

    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc, Options const& /*options*/)
    {
        //if (options.parse_numbers_as_strings)
        //    stack.emplace_back(json::string_tag, first, last);
        //else
            stack.emplace_back(numbers::StringToNumber(first, last, nc));

        return {};
    }

    ParseStatus HandleString(char const* first, char const* last, StringClass string_class, Options const& /*options*/)
    {
        if (string_class == StringClass::needs_cleaning)
        {
            String str;

            str.reserve(static_cast<size_t>(last - first));

            auto const res = strings::UnescapeString(first, last, [&](char ch) { str.push_back(ch); });
            if (res.status != strings::Status::success)
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
        JSON_ASSERT(!stack.empty());
        JSON_ASSERT(count != 0);

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

    ParseStatus HandleKey(char const* first, char const* last, StringClass string_class, Options const& /*options*/)
    {
        if (string_class == StringClass::needs_cleaning)
        {
            keys.emplace_back();
            keys.back().reserve(static_cast<size_t>(last - first));

            auto const res = strings::UnescapeString(first, last, [&](char ch) { keys.back().push_back(ch); });
            if (res.status != strings::Status::success)
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
        JSON_ASSERT(!keys.empty());
        JSON_ASSERT(!stack.empty());
        JSON_ASSERT(count != 0);

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

        JSON_ASSERT(num_elements <= SIZE_MAX - 1);
        JSON_ASSERT(num_elements <= PTRDIFF_MAX);
        JSON_ASSERT(stack.size() >= 1 + num_elements);

        auto const count = static_cast<std::ptrdiff_t>(num_elements);

        auto const I = stack.end() - count;
        auto const E = stack.end();

        auto& arr = I[-1].get_array();
        //if (options.ignore_null_values)
        //{
        //    for (std::ptrdiff_t i = 0; i != count; ++i)
        //    {
        //        if (I[i].is_null())
        //            continue;
        //        arr.push_back(std::move(*I));
        //    }
        //}
        //else
        {
            arr.insert(arr.end(), std::make_move_iterator(I), std::make_move_iterator(E));
        }

        stack.erase(I, E);

        return {};
    }

    ParseStatus PopMembers(size_t num_members)
    {
        if (num_members == 0)
            return {};

        JSON_ASSERT(num_members <= keys.size());
        JSON_ASSERT(num_members <= SIZE_MAX - 1);
        JSON_ASSERT(num_members <= PTRDIFF_MAX);
        JSON_ASSERT(stack.size() >= 1 + num_members);
        JSON_ASSERT(keys.size() >= num_members);

        auto const count = static_cast<std::ptrdiff_t>(num_members);

        auto const Iv = stack.end() - count;
        auto const Ik = keys.end() - count;

        auto& obj = Iv[-1].get_object();

        for (std::ptrdiff_t i = 0; i != count; ++i)
        {
            auto& K = Ik[i];
            auto& V = Iv[i];

            //if (options.ignore_null_values && V.is_null())
            //    continue;

            obj[std::move(K)] = std::move(V);
        }

        stack.erase(Iv, stack.end());
        keys.erase(Ik, keys.end());

        return {};
    }
};

ParseResult json::parse(Value& value, char const* next, char const* last, Options const& options)
{
    ParseValueCallbacks cb;

    auto const res = json::ParseSAX(cb, next, last, options);
    if (res.ec == ParseStatus::success)
    {
        JSON_ASSERT(cb.stack.size() == 1);

//      value.swap(cb.stack.back());
        value = std::move(cb.stack.back());
    }

    return res;
}

ParseStatus json::parse(Value& value, std::string const& str, Options const& options)
{
    char const* next = str.data();
    char const* last = str.data() + str.size();

    return json::parse(value, next, last, options).ec;
}

//==================================================================================================
// stringify
//==================================================================================================

static bool StringifyValue(std::string& str, Value const& value, StringifyOptions const& options, int curr_indent);

static bool StringifyNull(std::string& str, StringifyOptions const& /*options*/)
{
    //if (!options.ignore_null_values)
        str += "null";
    return true;
}

static bool StringifyBoolean(std::string& str, bool value)
{
    str += value ? "true" : "false";
    return true;
}

static bool StringifyNumber(std::string& str, double value, StringifyOptions const& options)
{
    if (!options.allow_nan_inf && !std::isfinite(value))
    {
        // XXX:
        // Should ignore_null_values apply here?!?!
        //if (!options.ignore_null_values)
            str += "null";
        return true;
    }

    char buf[32];
    char* end = numbers::NumberToString(buf, 32, value, /*force_trailing_dot_zero*/ true);
    str.append(buf, end);

    return true;
}

static bool StringifyString(std::string& str, String const& value, StringifyOptions const& /*options*/)
{
    bool success = true;

    str += '"';

    char const* const next = value.data();
    char const* const last = value.data() + value.size();
    if (next != last)
    {
        auto const res = strings::EscapeString(next, last, [&](char ch) { str += ch; });
        success = res.status == strings::Status::success;
    }

    str += '"';

    return success;
}

static bool StringifyArray(std::string& str, Array const& value, StringifyOptions const& options, int curr_indent)
{
    str += '[';

    auto       I = value.begin();
    auto const E = value.end();
    if (I != E)
    {
        if (options.indent_width > 0)
        {
            bool skipped_all = true;

            // Prevent overflow in curr_indent + options.indent_width
            int const indent_width = (curr_indent <= INT_MAX - options.indent_width) ? options.indent_width : 0;
            curr_indent += indent_width;

            for (;;)
            {
                //bool const skip = options.ignore_null_values && I->is_null();
                //if (skip)
                //{
                //    if (++I == E)
                //        break;
                //}
                //else
                {
                    skipped_all = false;

                    str += '\n';
                    str.append(static_cast<size_t>(curr_indent), ' ');

                    if (!StringifyValue(str, *I, options, curr_indent))
                        return false;

                    if (++I == E)
                        break;

                    str += ',';
                }
            }

            curr_indent -= indent_width;

            if (!skipped_all)
            {
                str += '\n';
                str.append(static_cast<size_t>(curr_indent), ' ');
            }
        }
        else
        {
            for (;;)
            {
                //bool const skip = options.ignore_null_values && I->is_null();
                //if (skip)
                //{
                //    if (++I == E)
                //        break;
                //}
                //else
                {
                    if (!StringifyValue(str, *I, options, curr_indent))
                        return false;

                    if (++I == E)
                        break;

                    str += ',';
                    if (options.indent_width == 0)
                        str += ' ';
                }
            }
        }
    }

    str += ']';

    return true;
}

static bool StringifyObject(std::string& str, Object const& value, StringifyOptions const& options, int curr_indent)
{
    str += '{';

    auto       I = value.begin();
    auto const E = value.end();
    if (I != E)
    {
        if (options.indent_width > 0)
        {
            bool skipped_all = true;

            // Prevent overflow in curr_indent + options.indent_width
            int const indent_width = (curr_indent <= INT_MAX - options.indent_width) ? options.indent_width : 0;
            curr_indent += indent_width;

            for (;;)
            {
                //bool const skip = options.ignore_null_values && I->second.is_null();
                //if (skip)
                //{
                //    if (++I == E)
                //        break;
                //}
                //else
                {
                    str += '\n';
                    str.append(static_cast<size_t>(curr_indent), ' ');

                    if (!StringifyString(str, I->first, options))
                        return false;
                    str += ':';
                    str += ' ';
                    if (!StringifyValue(str, I->second, options, curr_indent))
                        return false;

                    if (++I == E)
                        break;

                    str += ',';
                }
            }

            curr_indent -= indent_width;

            if (!skipped_all)
            {
                str += '\n';
                str.append(static_cast<size_t>(curr_indent), ' ');
            }
        }
        else
        {
            for (;;)
            {
                //bool const skip = options.ignore_null_values && I->second.is_null();
                //if (skip)
                //{
                //    if (++I == E)
                //        break;
                //}
                //else
                {
                    if (!StringifyString(str, I->first, options))
                        return false;
                    str += ':';
                    if (options.indent_width == 0)
                        str += ' ';
                    if (!StringifyValue(str, I->second, options, curr_indent))
                        return false;

                    if (++I == E)
                        break;

                    str += ',';
                    if (options.indent_width == 0)
                        str += ' ';
                }
            }
        }
    }

    str += '}';

    return true;
}

static bool StringifyValue(std::string& str, Value const& value, StringifyOptions const& options, int curr_indent)
{
    switch (value.type())
    {
    case Type::undefined:
        JSON_ASSERT(false && "cannot stringify 'undefined'"); // LCOV_EXCL_LINE
        return StringifyNull(str, options);
    case Type::null:
        return StringifyNull(str, options);
    case Type::boolean:
        return StringifyBoolean(str, value.get_boolean());
    case Type::number:
        return StringifyNumber(str, value.get_number(), options);
    case Type::string:
        return StringifyString(str, value.get_string(), options);
    case Type::array:
        return StringifyArray(str, value.get_array(), options, curr_indent);
    case Type::object:
        return StringifyObject(str, value.get_object(), options, curr_indent);
    default:
        JSON_ASSERT(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

bool json::stringify(std::string& str, Value const& value, StringifyOptions const& options)
{
    return StringifyValue(str, value, options, 0);
}
