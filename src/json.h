// Copyright (c) 2017 Alexander Bolz
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

#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

//==================================================================================================
// Value
//==================================================================================================

#define JSON_VALUE_HAS_EXPLICIT_OPERATOR_T                      0
#define JSON_VALUE_HAS_IMPLICIT_INITIALIZER_LIST_CONSTRUCTOR    0
#define JSON_VALUE_UNDEFINED_IS_UNORDERED                       0 // undefined OP x ==> false, undefined != x ==> true
#define JSON_VALUE_ALLOW_UNDEFINED_ACCESS                       0

#if __cplusplus >= 201703 || __cpp_inline_variables >= 201606
#define JSON_INLINE_VARIABLE inline
#else
#define JSON_INLINE_VARIABLE
#endif

namespace json {

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

class Value;

using Null    = std::nullptr_t;
using String  = std::string;
using Array   = std::vector<Value>;
using Object  = std::map<String, Value, std::less</*transparent*/>>;

enum class Type : int {
    undefined,
    null,
    boolean,
    number,
    string,
    array,
    object,
};

inline constexpr bool operator<(Type lhs, Type rhs)
{
    return static_cast<std::underlying_type_t<Type>>(lhs) < static_cast<std::underlying_type_t<Type>>(rhs);
}

inline constexpr bool operator>(Type lhs, Type rhs)
{
    return rhs < lhs;
}

inline constexpr bool operator<=(Type lhs, Type rhs)
{
    return !(rhs < lhs);
}

inline constexpr bool operator>=(Type lhs, Type rhs)
{
    return !(lhs < rhs);
}

#if 1
template <Type K>
struct Type_const
{
    static constexpr Type value = K;
    // NB:
    // No _implicit_ "operator Type() const" here!
};
#else
template <Type K>
using Type_const = std::integral_constant<Type, K>;
#endif

using Tag_undefined = Type_const<Type::undefined>;
using Tag_null      = Type_const<Type::null   >;
using Tag_boolean   = Type_const<Type::boolean>;
using Tag_number    = Type_const<Type::number >;
using Tag_string    = Type_const<Type::string >;
using Tag_array     = Type_const<Type::array  >;
using Tag_object    = Type_const<Type::object >;

JSON_INLINE_VARIABLE constexpr Tag_undefined const undefined_tag{};
JSON_INLINE_VARIABLE constexpr Tag_null      const null_tag{};
JSON_INLINE_VARIABLE constexpr Tag_boolean   const boolean_tag{};
JSON_INLINE_VARIABLE constexpr Tag_number    const number_tag{};
JSON_INLINE_VARIABLE constexpr Tag_string    const string_tag{};
JSON_INLINE_VARIABLE constexpr Tag_array     const array_tag{};
JSON_INLINE_VARIABLE constexpr Tag_object    const object_tag{};

namespace impl {

template <Type> struct TargetType {};
template <>     struct TargetType<Type::undefined> { using type = void;    };
template <>     struct TargetType<Type::null     > { using type = Null;    };
template <>     struct TargetType<Type::boolean  > { using type = bool;    };
template <>     struct TargetType<Type::number   > { using type = double;  };
template <>     struct TargetType<Type::string   > { using type = String;  };
template <>     struct TargetType<Type::array    > { using type = Array;   };
template <>     struct TargetType<Type::object   > { using type = Object;  };

template <typename T>
struct AlwaysFalse { static constexpr bool value = false; };

struct DefaultTraits_null {
    using tag = Tag_null;
    template <typename V> static decltype(auto) to_json(V&&) { return nullptr; }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in); }
};

struct DefaultTraits_boolean {
    using tag = Tag_boolean;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).get_boolean(); }
};

struct DefaultTraits_number {
    using tag = Tag_number;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).get_number(); }
};

struct DefaultTraits_string {
    using tag = Tag_string;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).get_string(); }
};

struct DefaultTraits_array {
    using tag = Tag_array;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).get_array(); }
};

struct DefaultTraits_object {
    using tag = Tag_object;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).get_object(); }
};

template <typename T, typename /*Enable*/ = void>
struct DefaultTraits
{
};

template <> struct DefaultTraits<std::nullptr_t    > : DefaultTraits_null    {};
template <> struct DefaultTraits<bool              > : DefaultTraits_boolean {};
template <> struct DefaultTraits<double            > : DefaultTraits_number  {};
template <> struct DefaultTraits<float             > : DefaultTraits_number  {};
template <> struct DefaultTraits<signed char       > : DefaultTraits_number  {};
template <> struct DefaultTraits<signed short      > : DefaultTraits_number  {};
template <> struct DefaultTraits<signed int        > : DefaultTraits_number  {};
template <> struct DefaultTraits<signed long       > : DefaultTraits_number  {};
template <> struct DefaultTraits<signed long long  > : DefaultTraits_number  {};
template <> struct DefaultTraits<unsigned char     > : DefaultTraits_number  {};
template <> struct DefaultTraits<unsigned short    > : DefaultTraits_number  {};
template <> struct DefaultTraits<unsigned int      > : DefaultTraits_number  {};
template <> struct DefaultTraits<unsigned long     > : DefaultTraits_number  {};
template <> struct DefaultTraits<unsigned long long> : DefaultTraits_number  {};
template <> struct DefaultTraits<String            > : DefaultTraits_string  {};
template <> struct DefaultTraits<Array             > : DefaultTraits_array   {};
template <> struct DefaultTraits<Object            > : DefaultTraits_object  {};

template <>
struct DefaultTraits<char const*>
{
    using tag = Tag_string;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in)
    {
        static_assert(std::is_lvalue_reference<V>::value, "Dangling pointer");
        return in.get_string().c_str();
    }
};

template <>
struct DefaultTraits<char*>
{
    using tag = Tag_string;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in)
    {
        static_assert(std::is_lvalue_reference<V>::value, "Dangling pointer");
        return in.get_string().empty() ? static_cast<char*>(nullptr) : &in.get_string()[0];
    }
};

template <typename It>
inline decltype(auto) safe_make_move_iterator(It it, std::true_type /*dont_move*/)
{
    return it;
}

template <typename It>
inline decltype(auto) safe_make_move_iterator(It it, std::false_type /*dont_move*/)
{
    return std::make_move_iterator(it);
}

// Don't accidentally create move_iterator's from a mutable 'Container&'
template <typename Container, typename It>
inline decltype(auto) safe_make_move_iterator(It it)
{
#if 0
    using dont_move = std::integral_constant<bool,
        !std::is_rvalue_reference<Container>::value
        >;
#else
    using dont_move = std::integral_constant<bool,
        std::is_lvalue_reference<Container>::value
        || std::is_const<Container /*std::remove_reference_t<Container>*/>::value
        >;
#endif

    return json::impl::safe_make_move_iterator(it, dont_move{});
}

template <typename T>
struct DefaultTraits_any_array
{
    using tag = Tag_array;

    template <typename V>
    static decltype(auto) to_json(V&& in)
    {
        auto I = json::impl::safe_make_move_iterator<V>(in.begin());
        auto E = json::impl::safe_make_move_iterator<V>(in.end());

        return Array(I, E);
    }

    template <typename V>
    static decltype(auto) from_json(V&& in)
    {
        auto&& arr = in.get_array();
        auto I = json::impl::safe_make_move_iterator<V>(arr.begin());
        auto E = json::impl::safe_make_move_iterator<V>(arr.end());

        T out;
        out.reserve(arr.size());
        for ( ; I != E; ++I)
        {
            out.emplace_back(I->template as<typename T::value_type>());
        }

        return out;
    }
};

template <typename T, typename Alloc>
struct DefaultTraits<std::vector<T, Alloc>> : DefaultTraits_any_array<std::vector<T, Alloc>>
{
};

} // namespace impl

template <typename T, typename /*Enable*/ = void>
struct Traits
    : impl::DefaultTraits<T>
{
    //
    // Specializations must have the following typedefs:
    //
    // using tag = Tag_X;
    //      where X is any of {null, boolean, number, string, array, object}.
    //
    // Specializations must implement the following two functions.
    // (They may be delete'd, though, to disallow conversion in either direction.)
    //
    // template <typename V> static R to_json(V&&);
    //      where R must be convertible to any of {Null, Boolean, Number, String, Array, Object}.
    //
    //      This function is used to (implicitly) construct a json::Value from objects of type T.
    //      There is no need to write this as a template: You may also just implement some overloads,
    //      but the return type must be the same.
    //
    // template <typename V> static R from_json(V&&);
    //      where R must be convertible to T.
    //
    //      This function is used to (explicitly) convert a json::Value to an object of type T.
    //      There is no need to write this as a template: V will always be a (rvalue) reference
    //      to a (possibly const qualified) object of type Value.
    //
};

template <typename T>
using TraitsFor = Traits<std::decay_t<T>>;

template <typename T>
using TagFor = typename TraitsFor<T>::tag;

template <typename T>
using TargetTypeFor = typename impl::TargetType<TagFor<T>::value>::type;

template <typename T>
using ToJsonResultTypeFor = decltype(( TraitsFor<T>::to_json(std::declval<T>()) ));

namespace impl {

template <typename T, typename /*Enable*/ = void>
struct CheckHasTag : std::false_type
{
    static_assert(AlwaysFalse<T>::value, R"(

Invalid (or missing) json::Traits<> specialization:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A specialization of json::Traits<T> must exist and must have a member type named 'tag',

    which must be one of {Tag_null, Tag_boolean, Tag_number, Tag_string, Tag_array, Tag_object}.

)");
};

template <typename T>
struct CheckHasTag<T,
    std::enable_if_t< std::is_same<TagFor<T>, Tag_null>::value
                      || std::is_same<TagFor<T>, Tag_boolean>::value
                      || std::is_same<TagFor<T>, Tag_number>::value
                      || std::is_same<TagFor<T>, Tag_string>::value
                      || std::is_same<TagFor<T>, Tag_array>::value
                      || std::is_same<TagFor<T>, Tag_object>::value
                      >
    >
    : std::true_type
{
};

template <typename T, typename /*Enable*/ = void>
struct CheckHasToJson : std::false_type
{
    static_assert(AlwaysFalse<T>::value, R"(

Invalid (or missing) json::Traits<> specialization:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

json::Traits<T> must have a static member function 'to_json',

    which must be callable with an argument of type 'T',

    and must return a json::Value
             or the result must be convertible to TargetType<T>
             or TargetType<T> must be explicitly constructible from the result (in this case one needs to call 'json::Value's explicit constructor).

)");
};

template <typename T>
struct CheckHasToJson<T,
    std::enable_if_t< std::is_same<Value, std::decay_t<ToJsonResultTypeFor<T>>>::value
                      || std::is_constructible<TargetTypeFor<T>, ToJsonResultTypeFor<T>>::value
                      >
    >
    : std::true_type
{
};

} // namespace impl

// In case you get an error like
//      error: conversion from 'T' to non-scalar type 'json::Value' requested
// or
//      error C2440: 'initializing': cannot convert from 'T' to 'json::Value'
// you can try to instantiate CheckToJsonTraits<T> to get a more detailed error message.
template <typename T>
struct TestConversionToJson
    : std::integral_constant<bool, impl::CheckHasTag<T>::value
                                   && impl::CheckHasToJson<T>::value
                                   >
{
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

class Value final
{
    union Data {
        bool    boolean;
        double  number;
        String* string;
        Array*  array;
        Object* object;
    };

    Data data_;
    Type type_ = Type::undefined;

    static Value const kUndefined;

    template <typename T>
    using IsValue = std::is_same<Value, std::decay_t<T>>;

public:
    Value() noexcept = default;
   ~Value() noexcept
    {
       assign(undefined_tag);
    }

    Value(Value const& rhs);
    Value(Value&& rhs) noexcept
        : data_(rhs.data_)
        , type_(std::exchange(rhs.type_, Type::undefined))
    {
    }

    Value& operator=(Value const& rhs);
    Value& operator=(Value&& rhs) noexcept
    {
        data_ = rhs.data_;
        type_ = std::exchange(rhs.type_, Type::undefined);
        return *this;
    }

    Value(Type t);

    // undefined

    Value(Tag_undefined) noexcept
        : type_(Type::undefined)
    {
    }

    // null

    Value(Tag_null, Null /*arg*/ = {}) noexcept
        : type_(Type::null)
    {
    }

    // boolean

    Value(Tag_boolean, bool arg = {}) noexcept
        : type_(Type::boolean)
    {
        data_.boolean = arg;
    }

    // number

    Value(Tag_number, double arg = {}) noexcept
        : type_(Type::number)
    {
        data_.number = arg;
    }

    // string

    template <typename ...Args>
    Value(Tag_string, Args&&... args)
    {
        data_.string = new String(std::forward<Args>(args)...);
        type_ = Type::string;
    }

    // array

    template <typename ...Args>
    Value(Tag_array, Args&&... args)
    {
        data_.array = new Array(std::forward<Args>(args)...);
        type_ = Type::array;
    }

    Value(Tag_array, std::initializer_list<Array::value_type> ilist)
    {
        data_.array = new Array(ilist);
        type_ = Type::array;
    }

    // object

    template <typename ...Args>
    Value(Tag_object, Args&&... args)
    {
        data_.object = new Object(std::forward<Args>(args)...);
        type_ = Type::object;
    }

    Value(Tag_object, std::initializer_list<Object::value_type> ilist)
    {
        data_.object = new Object(ilist);
        type_ = Type::object;
    }

    // generic constructors

    template <typename T,
        std::enable_if_t< !IsValue<T>::value
                          && !IsValue<ToJsonResultTypeFor<T>>::value
                          && std::is_convertible<ToJsonResultTypeFor<T>, TargetTypeFor<T>>::value,
        int> = 0>
    Value(T&& v)
        : Value(TagFor<T>{}, TraitsFor<T>::to_json(std::forward<T>(v)))
    {
    }

    template <typename T,
        std::enable_if_t< !IsValue<T>::value
                          && !IsValue<ToJsonResultTypeFor<T>>::value
                          && !std::is_convertible<ToJsonResultTypeFor<T>, TargetTypeFor<T>>::value
                          && std::is_constructible<TargetTypeFor<T>, ToJsonResultTypeFor<T>>::value,
        int> = 1>
    explicit Value(T&& v)
        : Value(TagFor<T>{}, static_cast<TargetTypeFor<T>>( TraitsFor<T>::to_json(std::forward<T>(v)) ))
    {
    }

    template <typename T,
        std::enable_if_t< !IsValue<T>::value
                          && IsValue<ToJsonResultTypeFor<T>>::value,
        int> = 2>
    Value(T&& v)
        : Value(TraitsFor<T>::to_json(std::forward<T>(v)))
    {
    }

#if JSON_VALUE_HAS_IMPLICIT_INITIALIZER_LIST_CONSTRUCTOR
    Value(std::initializer_list<std::pair<String, Value>> il)
        : Value(Tag_object{}, il)
    {
    }
#endif

public:
    // undefined

    void assign(Tag_undefined) noexcept;

    // null

    void assign(Tag_null, Null /*value*/ = {}) noexcept;

    // boolean

    bool& assign(Tag_boolean, bool value = {}) noexcept;

    // number

    double& assign(Tag_number, double value = {}) noexcept;

    // string

    String& assign(Tag_string);
    String& assign(Tag_string, String const& value);
    String& assign(Tag_string, String&& value);

    // array

    Array& assign(Tag_array);
    Array& assign(Tag_array, Array const& value);
    Array& assign(Tag_array, Array&& value);

    // object

    Object& assign(Tag_object);
    Object& assign(Tag_object, Object const& value);
    Object& assign(Tag_object, Object&& value);

    // assignment operators

    template <typename T,
        std::enable_if_t< !IsValue<T>::value
                          && !IsValue<ToJsonResultTypeFor<T>>::value
                          && std::is_convertible<ToJsonResultTypeFor<T>, TargetTypeFor<T>>::value,
        int> = 0>
    Value& operator=(T&& v)
    {
        assign(TagFor<T>{}, TraitsFor<T>::to_json(std::forward<T>(v)));
        return *this;
    }

    template <typename T,
        std::enable_if_t< !IsValue<T>::value
                          && IsValue<ToJsonResultTypeFor<T>>::value,
        int> = 1>
    Value& operator=(T&& v)
    {
        return *this = TraitsFor<T>::to_json(std::forward<T>(v));
    }

    template <Type Ty>
    Value& operator=(Type_const<Ty> tag)
    {
        assign(tag);
        return *this;
    }

private:
    void _clear();
    template <typename T> String& _assign_string(T&& value);
    template <typename T> Array&  _assign_array (T&& value);
    template <typename T> Object& _assign_object(T&& value);

public:
    // Returns the type of the actual value stored in this JSON object.
    Type type() const noexcept
    {
        return type_;
    }

    // is_X returns whether the actual value stored in this JSON object is of type X.
    bool is_undefined() const noexcept { return type() == Type::undefined; }
    bool is_null()      const noexcept { return type() == Type::null;      }
    bool is_boolean()   const noexcept { return type() == Type::boolean;   }
    bool is_number()    const noexcept { return type() == Type::number;    }
    bool is_string()    const noexcept { return type() == Type::string;    }
    bool is_array()     const noexcept { return type() == Type::array;     }
    bool is_object()    const noexcept { return type() == Type::object;    }
    bool is_primitive() const noexcept { return Type::null <= type() && type() <= Type::string; }

    bool is(Type t) const noexcept { return type() == t; }

    // get_X returns a reference to the value of type X stored in this JSON object.
    // PRE: is_X() == true

    bool& get_boolean() & noexcept
    {
        assert(is_boolean());
        return data_.boolean;
    }

    bool const& get_boolean() const& noexcept
    {
        assert(is_boolean());
        return data_.boolean;
    }

    bool get_boolean() && noexcept
    {
        assert(is_boolean());
        return data_.boolean;
    }

    double& get_number() & noexcept
    {
        assert(is_number());
        return data_.number;
    }

    double const& get_number() const& noexcept
    {
        assert(is_number());
        return data_.number;
    }

    double get_number() && noexcept
    {
        assert(is_number());
        return data_.number;
    }

    String& get_string() & noexcept
    {
        assert(is_string());
        return *data_.string;
    }

    String const& get_string() const& noexcept
    {
        assert(is_string());
        return *data_.string;
    }

    String get_string() && noexcept
    {
        assert(is_string());
        return std::move(*data_.string);
    }

    Array& get_array() & noexcept
    {
        assert(is_array());
        return *data_.array;
    }

    Array const& get_array() const& noexcept
    {
        assert(is_array());
        return *data_.array;
    }

    Array get_array() && noexcept
    {
        assert(is_array());
        return std::move(*data_.array);
    }

    Object& get_object() & noexcept
    {
        assert(is_object());
        return *data_.object;
    }

    Object const& get_object() const& noexcept
    {
        assert(is_object());
        return *data_.object;
    }

    Object get_object() && noexcept
    {
        assert(is_object());
        return std::move(*data_.object);
    }

    // as<T> uses Traits::from_json to convert this JSON value into an object
    // of type T.

    template <typename T> T as() const&  { return TraitsFor<T>::from_json(*this); }
    template <typename T> T as() &       { return TraitsFor<T>::from_json(*this); }
    template <typename T> T as() const&& { return TraitsFor<T>::from_json(static_cast<Value const&&>(*this)); }
    template <typename T> T as() &&      { return TraitsFor<T>::from_json(static_cast<Value&&      >(*this)); }

#if JSON_VALUE_HAS_EXPLICIT_OPERATOR_T
    template <typename T> explicit operator T() const&  { return this->as<T>(); }
#if 0
    // These are disabled to let g++ generate "conversion sequence is better" warnings...
    template <typename T> explicit operator T() &       { return this->as<T>(); }
    template <typename T> explicit operator T() const&& { return static_cast<Value const&&>(*this).as<T>(); }
#endif
    template <typename T> explicit operator T() &&      { return static_cast<Value&&      >(*this).as<T>(); }
#endif

    // Compare this value to another. Strict equality (i.e. types must match).
    bool equal_to(Value const& rhs) const noexcept;

    // Lexicographically compare this value to another.
    bool less_than(Value const& rhs) const noexcept;

    // Compute a hash value for this JSON value.
    size_t hash() const noexcept;

    // Swap this value with another
    void swap(Value& other) noexcept;

    // Returns the size of this string or array or object.
    // PRE: is_string() or is_array() or is_object()
    size_t size() const noexcept;

    // Returns whether this string or array or object is empty.
    // PRE: is_string() or is_array() or is_object()
    bool empty() const noexcept;

    //--------------------------------------------------------------------------
    // Array helper:
    //

private:
    Array& _get_or_assign_array();

public:
    using element_iterator       = Array::iterator;
    using const_element_iterator = Array::const_iterator;

    element_iterator       elements_begin()      & { return get_array().begin(); }
    element_iterator       elements_end()        & { return get_array().end();   }
    const_element_iterator elements_begin() const& { return get_array().begin(); }
    const_element_iterator elements_end()   const& { return get_array().end();   }

    template <typename It>
    struct ItRange {
        It begin_;
        It end_;
        It begin() const { return begin_; }
        It end() const { return end_; }
    };

    ItRange<element_iterator>       elements()      &  { return {elements_begin(), elements_end()}; }
    ItRange<const_element_iterator> elements() const&  { return {elements_begin(), elements_end()}; }

    // Convert this value into an array an return a reference to the index-th element.
    // PRE: is_undefined() or is_array()
    Value& operator[](size_t index);

    // Returns a reference to the index-th element.
    // Or a reference to an 'undefined' value if the index is out of range.
    // PRE: is_array()
    Value const& operator[](size_t index) const noexcept;

    // Returns a pointer the the value at the given index.
    // Or nullptr if this value is not an array of if the index is out bounds.
    Value*       get_ptr(size_t index);
    Value const* get_ptr(size_t index) const;

    // Convert this value into an array and append a new element constructed from the given arguments.
    // PRE: is_undefined() or is_array()
    template <typename ...Args>
    Value& emplace_back(Args&&... args)
    {
        auto& arr = _get_or_assign_array();
        arr.emplace_back(std::forward<Args>(args)...);
        return arr.back();
    }

    // Convert this value into an array and append a new element.
    // PRE: is_undefined() or is_array()
    Value& push_back(Value const& value) { return emplace_back(value); }
    Value& push_back(Value&&      value) { return emplace_back(std::move(value)); }

    // Remove the last element of the array.
    // PRE: is_array()
    // PRE: size() > 0
    void pop_back();

    // Erase the element at the given index.
    // PRE: is_array()
    // PRE: index < size()
    element_iterator erase(size_t index);

    //--------------------------------------------------------------------------
    // Object helper:
    //

private:
    template <typename T>
    using IsObjectKeyType = std::is_same<Object::key_type, std::decay_t<T>>;

    template <typename T>
    using IsTransparentKey = std::integral_constant<bool,
        // Integral "keys" are used to index arrays.
        // Disallow literal '0': String might be convertible from nullptr...
        !std::is_integral<std::decay_t<T>>::value
        // Check if the values of type 'T' can be looked up.
        && std::is_convertible< decltype(( std::declval<Object const&>().find(std::declval<T>()) )),
                                typename Object::const_iterator
                                >::value
        >;

    Object& _get_or_assign_object();

public:
    // items:

    using item_iterator       = Object::iterator;
    using const_item_iterator = Object::const_iterator;

    item_iterator       items_begin()      & { return get_object().begin(); }
    item_iterator       items_end()        & { return get_object().end();   }
    const_item_iterator items_begin() const& { return get_object().begin(); }
    const_item_iterator items_end()   const& { return get_object().end();   }

    ItRange<item_iterator>       items()      &  { return {items_begin(), items_end()}; }
    ItRange<const_item_iterator> items() const&  { return {items_begin(), items_end()}; }

    // keys:

    class const_key_iterator
    {
        Object::const_iterator it;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = Object::key_type;
        using reference         = const value_type&;
        using pointer           = const value_type*;
        using difference_type   = Object::const_iterator::difference_type;

        const_key_iterator() = default;
        explicit const_key_iterator(Object::const_iterator it_) : it(it_) {}

        reference operator*() const { return it->first; }
        pointer operator->() const { return &it->first; }

        const_key_iterator& operator++() { ++it; return *this; }
        const_key_iterator& operator--() { --it; return *this; }
        const_key_iterator operator++(int) { auto I = *this; ++it; return I; }
        const_key_iterator operator--(int) { auto I = *this; --it; return I; }

        friend bool operator==(const_key_iterator lhs, const_key_iterator rhs) { return lhs.it == rhs.it; }
        friend bool operator!=(const_key_iterator lhs, const_key_iterator rhs) { return lhs.it != rhs.it; }
    };

    const_key_iterator keys_begin() const& { return const_key_iterator(items_begin()); }
    const_key_iterator keys_end()   const& { return const_key_iterator(items_end());   }

    ItRange<const_key_iterator> keys() const& { return {keys_begin(), keys_end()}; }

    // values:

    class const_value_iterator;

    class value_iterator
    {
        friend class const_value_iterator;

        Object::iterator it;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = Object::mapped_type;
        using reference         = value_type&;
        using pointer           = value_type*;
        using difference_type   = Object::iterator::difference_type;

        value_iterator() = default;
        explicit value_iterator(Object::iterator it_) : it(it_) {}

        reference operator*() const { return it->second; }
        pointer operator->() const { return &it->second; }

        value_iterator& operator++() { ++it; return *this; }
        value_iterator& operator--() { --it; return *this; }
        value_iterator operator++(int) { auto I = *this; ++it; return I; }
        value_iterator operator--(int) { auto I = *this; --it; return I; }

        friend bool operator==(value_iterator lhs, value_iterator rhs) { return lhs.it == rhs.it; }
        friend bool operator!=(value_iterator lhs, value_iterator rhs) { return lhs.it != rhs.it; }
    };

    class const_value_iterator
    {
        Object::const_iterator it;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = Object::mapped_type;
        using reference         = const value_type&;
        using pointer           = const value_type*;
        using difference_type   = Object::const_iterator::difference_type;

        const_value_iterator() = default;
        explicit const_value_iterator(Object::const_iterator it_) : it(it_) {}

        // mutable -> const
        // implicit.
        const_value_iterator(value_iterator rhs) : it(rhs.it) {}

        reference operator*() const { return it->second; }
        pointer operator->() const { return &it->second; }

        const_value_iterator& operator++() { ++it; return *this; }
        const_value_iterator& operator--() { --it; return *this; }
        const_value_iterator operator++(int) { auto I = *this; ++it; return I; }
        const_value_iterator operator--(int) { auto I = *this; --it; return I; }

        friend bool operator==(const_value_iterator lhs, const_value_iterator rhs) { return lhs.it == rhs.it; }
        friend bool operator!=(const_value_iterator lhs, const_value_iterator rhs) { return lhs.it != rhs.it; }
    };

    value_iterator       values_begin()      & { return value_iterator(items_begin());       }
    value_iterator       values_end()        & { return value_iterator(items_end());         }
    const_value_iterator values_begin() const& { return const_value_iterator(items_begin()); }
    const_value_iterator values_end()   const& { return const_value_iterator(items_end());   }

    ItRange<value_iterator>       values()      & { return {values_begin(), values_end()}; }
    ItRange<const_value_iterator> values() const& { return {values_begin(), values_end()}; }

    // Convert this value into an object and return a reference to the value with the given key.
    // PRE: is_undefined() or is_object()
    Value& operator[](Object::key_type const& key);
    Value& operator[](Object::key_type&& key);

    // Convert this value into an object and return a reference to the value with the given key.
    // PRE: is_undefined() or is_object()
    template <typename T, std::enable_if_t< !IsObjectKeyType<T>::value && IsTransparentKey<T>::value, int > = 0>
    Value& operator[](T&& key)
    {
        auto& obj = _get_or_assign_object();
#if 1
        // map::operator[] does not work for transparent keys.
        // map::find() does.
        // Optimize slightly for the case that an element with the given key already exists.

        auto const it = obj.find(key);
        if (it != obj.end()) {
            return it->second;
        }
#endif
        return obj[std::forward<T>(key)];
    }

    // Returns a reference to the value with the given key.
    // Or a reference to an 'undefined' value if an element for 'key' does not exist.
    // PRE: is_object()
    template <typename T, std::enable_if_t< IsTransparentKey<T>::value, int > = 0>
    Value const& operator[](T&& key) const noexcept
    {
#if JSON_VALUE_ALLOW_UNDEFINED_ACCESS
        assert(is_undefined() || is_object());
        if (is_object())
#endif
        {
            auto& obj = get_object();
            auto const it = obj.find(std::forward<T>(key));
            if (it != obj.end()) {
                return it->second;
            }
        }
        return kUndefined;
    }

    // Returns a pointer to the value with the given key.
    // Or nullptr if no such key exists, or this value is not an object.
    template <typename T, std::enable_if_t< IsTransparentKey<T>::value, int > = 0>
    Value* get_ptr(T&& key)
    {
        if (is_object())
        {
            auto& obj = get_object();
            auto const it = obj.find(std::forward<T>(key));
            if (it != obj.end()) {
                return &it->second;
            }
        }
        return nullptr;
    }

    // Returns a pointer to the value with the given key.
    // Or nullptr if no such key exists, or this value is not an object.
    template <typename T, std::enable_if_t< IsTransparentKey<T>::value, int > = 0>
    Value const* get_ptr(T&& key) const
    {
        if (is_object())
        {
            auto& obj = get_object();
            auto const it = obj.find(std::forward<T>(key));
            if (it != obj.end()) {
                return &it->second;
            }
        }
        return nullptr;
    }

    // Returns whether a value with the given key exists.
    template <typename T, std::enable_if_t< IsTransparentKey<T>::value, int > = 0>
    bool has_member(T&& key) const
    {
        return get_ptr(std::forward<T>(key)) != nullptr;
    }

    // Convert this value into an object and insert a new element constructed from the given arguments.
    // PRE: is_undefined() or is_object()
    template <typename ...Args>
    std::pair<item_iterator, bool> emplace(Args&&... args)
    {
        return _get_or_assign_object().emplace(std::forward<Args>(args)...);
    }

    // Convert this value to an object and insert the given {key, value} pair.
    // PRE: is_undefined() or is_object()
    std::pair<item_iterator, bool> insert(Object::value_type const& pair) { return emplace(pair); }
    std::pair<item_iterator, bool> insert(Object::value_type&&      pair) { return emplace(std::move(pair)); }

    // Erase the the given key.
    // PRE: is_object()
    template <typename T, std::enable_if_t< IsTransparentKey<T>::value && !std::is_convertible<T, const_item_iterator>::value, int > = 0>
    size_t erase(T&& key)
    {
        return get_object().erase(std::forward<T>(key));
    }

    // Erase the the given key.
    // PRE: is_object()
    item_iterator erase(const_item_iterator pos);

    // Erase all items in [first, last).
    // PRE: is_object()
    item_iterator erase(const_item_iterator first, const_item_iterator last);

public:
    // Type conversions
    //
    // See: https://tc39.github.io/ecma262/#sec-type-conversion
    //
    // PRE: value.is_primitive()
    // For arrays and objects: use stringify()

    bool     to_boolean() const noexcept;
    double   to_number() const noexcept;
    double   to_integer() const noexcept;
    int32_t  to_int32() const noexcept;
    uint32_t to_uint32() const noexcept;
    int16_t  to_int16() const noexcept;
    uint16_t to_uint16() const noexcept;
    int8_t   to_int8() const noexcept;
    uint8_t  to_uint8() const noexcept;
    uint8_t  to_uint8_clamped() const noexcept;
    String   to_string() const;
};

template <typename T> inline T cast(Value const& val) { return val.template as<T>(); }
template <typename T> inline T cast(Value&&      val) { return std::move(val).template as<T>(); }

inline void swap(Value& lhs, Value& rhs) noexcept
{
    lhs.swap(rhs);
}

inline bool operator==(Value const& lhs, Value const& rhs) noexcept
{
    return lhs.equal_to(rhs);
}

inline bool operator!=(Value const& lhs, Value const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (lhs.is_undefined() || rhs.is_undefined())
        return true;
#endif

    return !(lhs == rhs);
}

inline bool operator<(Value const& lhs, Value const& rhs) noexcept
{
    return lhs.less_than(rhs);
}

inline bool operator>=(Value const& lhs, Value const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (lhs.is_undefined() || rhs.is_undefined())
        return false;
#endif

    return !(lhs < rhs);
}

inline bool operator>(Value const& lhs, Value const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (lhs.is_undefined() || rhs.is_undefined())
        return false;
#endif

    return rhs < lhs;
}

inline bool operator<=(Value const& lhs, Value const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (lhs.is_undefined() || rhs.is_undefined())
        return false;
#endif

    return !(rhs < lhs);
}

namespace impl
{
    template <typename T> bool cmp_eq(Value const& lhs, T const&,     Tag_null   ) noexcept { return lhs.is_null(); }
    template <typename T> bool cmp_eq(Value const& lhs, T const& rhs, Tag_boolean) noexcept { return lhs.type() == Type::boolean && lhs.get_boolean() == rhs; }
    template <typename T> bool cmp_eq(Value const& lhs, T const& rhs, Tag_number ) noexcept { return lhs.type() == Type::number  && lhs.get_number () == rhs; }
    template <typename T> bool cmp_eq(Value const& lhs, T const& rhs, Tag_string ) noexcept { return lhs.type() == Type::string  && lhs.get_string () == rhs; }
    template <typename T> bool cmp_eq(Value const& lhs, T const& rhs, Tag_array  ) noexcept { return lhs.type() == Type::array   && lhs.get_array  () == rhs; }
    template <typename T> bool cmp_eq(Value const& lhs, T const& rhs, Tag_object ) noexcept { return lhs.type() == Type::object  && lhs.get_object () == rhs; }

    template <typename T> bool cmp_lt(Value const& lhs, T const&,     Tag_null   ) noexcept { return lhs.type() < Type::null; } // type < null || (type == null && nullptr < nullptr)
    template <typename T> bool cmp_lt(Value const& lhs, T const& rhs, Tag_boolean) noexcept { return lhs.type() < Type::boolean || (lhs.type() == Type::boolean && lhs.get_boolean() < rhs); }
    template <typename T> bool cmp_lt(Value const& lhs, T const& rhs, Tag_number ) noexcept { return lhs.type() < Type::number  || (lhs.type() == Type::number  && lhs.get_number () < rhs); }
    template <typename T> bool cmp_lt(Value const& lhs, T const& rhs, Tag_string ) noexcept { return lhs.type() < Type::string  || (lhs.type() == Type::string  && lhs.get_string () < rhs); }
    template <typename T> bool cmp_lt(Value const& lhs, T const& rhs, Tag_array  ) noexcept { return lhs.type() < Type::array   || (lhs.type() == Type::array   && lhs.get_array  () < rhs); }
    template <typename T> bool cmp_lt(Value const& lhs, T const& rhs, Tag_object ) noexcept { return lhs.type() < Type::object  || (lhs.type() == Type::object  && lhs.get_object () < rhs); }

    template <typename T> bool cmp_gt(Value const& lhs, T const&,     Tag_null   ) noexcept { return Type::null    < lhs.type(); } // null < type || (null == type && nullptr < nullptr)
    template <typename T> bool cmp_gt(Value const& lhs, T const& rhs, Tag_boolean) noexcept { return Type::boolean < lhs.type() || (Type::boolean == lhs.type() && rhs < lhs.get_boolean()); }
    template <typename T> bool cmp_gt(Value const& lhs, T const& rhs, Tag_number ) noexcept { return Type::number  < lhs.type() || (Type::number  == lhs.type() && rhs < lhs.get_number ()); }
    template <typename T> bool cmp_gt(Value const& lhs, T const& rhs, Tag_string ) noexcept { return Type::string  < lhs.type() || (Type::string  == lhs.type() && rhs < lhs.get_string ()); }
    template <typename T> bool cmp_gt(Value const& lhs, T const& rhs, Tag_array  ) noexcept { return Type::array   < lhs.type() || (Type::array   == lhs.type() && rhs < lhs.get_array  ()); }
    template <typename T> bool cmp_gt(Value const& lhs, T const& rhs, Tag_object ) noexcept { return Type::object  < lhs.type() || (Type::object  == lhs.type() && rhs < lhs.get_object ()); }
}

// Value == T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator==(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (lhs.is_undefined())
        return false;
#endif

    return impl::cmp_eq(lhs, rhs, TagFor<R>{});
}

// T == Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator==(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (rhs.is_undefined())
        return false;
#endif

    return impl::cmp_eq(rhs, lhs, TagFor<L>{});
}

// Value != T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator!=(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (lhs.is_undefined())
        return true;
#endif

    return !(lhs == rhs);
}

// T != Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator!=(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (rhs.is_undefined())
        return true;
#endif

    return !(lhs == rhs);
}

// Value < T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator<(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (lhs.is_undefined())
        return false;
#endif

    return impl::cmp_lt(lhs, rhs, TagFor<R>{});
}

// T < Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator<(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (rhs.is_undefined())
        return false;
#endif

    return impl::cmp_gt(rhs, lhs, TagFor<L>{});
}

// Value >= T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator>=(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (lhs.is_undefined())
        return false;
#endif

    return !(lhs < rhs);
}

// T >= Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator>=(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (rhs.is_undefined())
        return false;
#endif

    return !(lhs < rhs);
}

// Value > T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator>(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (lhs.is_undefined())
        return false;
#endif

    return rhs < lhs;
}

// T > Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator>(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (rhs.is_undefined())
        return false;
#endif

    return rhs < lhs;
}

// Value <= T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator<=(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (lhs.is_undefined())
        return false;
#endif

    return !(rhs < lhs);
}

// T <= Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator<=(L const& lhs, R const& rhs) noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (rhs.is_undefined())
        return false;
#endif

    return !(rhs < lhs);
}

} // namespace json

namespace std
{
    template <>
    struct hash< ::json::Value >
    {
        size_t operator()( ::json::Value const& j ) const noexcept {
            return j.hash();
        }
    };
}

//==================================================================================================
// parse
//==================================================================================================

namespace json {

enum class ErrorCode {
    success = 0,
    duplicate_key_in_object,
    expected_colon_after_key,
    expected_comma_or_closing_brace,
    expected_comma_or_closing_bracket,
    expected_eof,
    expected_key,
    invalid_escaped_character_in_string,
    invalid_numeric_literal,
    invalid_unicode_sequence_in_string,
    max_depth_reached,
    number_out_of_range,
    unescaped_control_character_in_string,
    unexpected_end_of_string,
    unexpected_token,
    unrecognized_identifier,
};

struct ParseResult
{
    ErrorCode ec;
    // On return, PTR denotes the position after the parsed value, or if an
    // error occurred, denotes the position of the offending token.
    char const* ptr;
    // If an error occurred, END denotes the position after the offending
    // token. This field is unused on success.
    char const* end;
};

struct ParseOptions
{
    // If true, skip UTF-8 byte order mark - if any.
    // Default is true.
    bool skip_bom = true;

    // If true, replace invalid unicode sequences with a
    // replacement char (U+FFFD).
    // Default is false.
    bool allow_invalid_unicode = false;

    // If true, allows trailing commas in arrays or objects.
    // Default is false.
    bool allow_trailing_comma = false;

    // If true, allows strings be quoted with a single quote, like 'hello'.
    // Default is false.
    bool allow_single_quoted_strings = false;

    // If true, allows unquoted strings as object keys: "{hello: 123}".
    // These unquoted strings must be valid JavaScript identifiers.
    // Default is false.
    bool allow_unquoted_keys = false;

    // If true, parse numbers as raw strings.
    // Default is false.
    bool parse_numbers_as_strings = false;

    // If true, parses "NaN" and "Infinity" (without the quotes) as numbers.
    // Default is true.
    bool allow_nan_inf = true;

    // If true, allows a leading '+' in numbers.
    // Default is false.
    bool allow_leading_plus = false;

    // If true, allow leading '.' in numbers (no leading 0 required).
    // Default is false.
    bool allow_leading_dot = false;

    // If true, skip line comments (introduced with "//") and block
    // comments like "/* hello */".
    // Default is false.
    bool allow_comments = false;

    // If true, allow characters after value.
    // Might be used to parse strings like "[1,2,3]{"hello":"world"}" into
    // different values by repeatedly calling parse.
    // Default is false.
    bool allow_trailing_characters = false;

    // If true, issue an error if an objects contains a duplicate key.
    // Otherwise, older keys will be overwritten by the following key.
    // Default is false.
    bool reject_duplicate_keys = false;
};

// Parse the JSON value stored in [NEXT, LAST).
ParseResult parse(Value& value, char const* next, char const* last, ParseOptions const& options = {});

// Parse the JSON value stored in STR.
ErrorCode parse(Value& value, std::string const& str, ParseOptions const& options = {});

} // namespace json

//==================================================================================================
// stringify
//==================================================================================================

namespace json {

struct StringifyOptions
{
    // If >= 0, pretty-print the JSON.
    // Default is < 0, that is the JSON is rendered as the shortest string possible.
    int8_t indent_width = -1;

    // If true, replaces each invalid UTF-8 sequence in strings with a single
    // replacement character (U+FFFD). Otherwise rendering fails for invalid
    // UTF-8 strings.
    // Default is false.
    bool allow_invalid_unicode = false;

    // If true, converts the special numbers NaN and infinity to nan_string and
    // inf_string, resp. Otherwise they are converted to "null".
    // Default is false.
    bool allow_nan_inf = false;

    // If true, escapes '/' in strings. This allows the JSON string to be
    // embedded in HTML.
    // Default is true.
    bool escape_slash = true;
};

// Write a stringified version of the given value to str.
// Returns true if successful.
// Returns false only if the JSON value contains invalid UTF-8 strings and
// options.allow_invalid_unicode is false.
bool stringify(std::string& str, Value const& value, StringifyOptions const& options = {});

} // namespace json
