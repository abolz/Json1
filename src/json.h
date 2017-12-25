#pragma once

#include "string_view.h"
#include "__flat_hash_map.h"

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

#define JSON_VALUE_HAS_EXPLICIT_OPERATOR_T                      1
#define JSON_VALUE_HAS_IMPLICIT_INITIALIZER_LIST_CONSTRUCTOR    0

namespace json {

class Value;

using Null    = std::nullptr_t;
using String  = std::string;
using Array   = std::vector<Value>;
using Object  = std::map<String, Value, std::less</*transparent*/>>;

enum class Type : int {
    null,
    boolean,
    number,
    string,
    array,
    object,
};

inline constexpr bool operator<(Type lhs, Type rhs) {
    return static_cast<std::underlying_type_t<Type>>(lhs) < static_cast<std::underlying_type_t<Type>>(rhs);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

template <Type K>
using Type_const = std::integral_constant<Type, K>;

using Tag_null    = Type_const<Type::null   >;
using Tag_boolean = Type_const<Type::boolean>;
using Tag_number  = Type_const<Type::number >;
using Tag_string  = Type_const<Type::string >;
using Tag_array   = Type_const<Type::array  >;
using Tag_object  = Type_const<Type::object >;

template <Type> struct TargetType;
template <>     struct TargetType<Type::null   > { using type = Null;    };
template <>     struct TargetType<Type::boolean> { using type = bool;    };
template <>     struct TargetType<Type::number > { using type = double;  };
template <>     struct TargetType<Type::string > { using type = String;  };
template <>     struct TargetType<Type::array  > { using type = Array;   };
template <>     struct TargetType<Type::object > { using type = Object;  };

namespace impl {

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
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).as_boolean(); }
};

struct DefaultTraits_float {
    using tag = Tag_number;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).as_number(); }
};

struct DefaultTraits_int {
    using tag = Tag_number;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).as_number(); }
};

struct DefaultTraits_string {
    using tag = Tag_string;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).as_string(); }
};

struct DefaultTraits_array {
    using tag = Tag_array;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).as_array(); }
};

struct DefaultTraits_object {
    using tag = Tag_object;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
    template <typename V> static decltype(auto) from_json(V&& in) { return std::forward<V>(in).as_object(); }
};

template <typename T>
struct DefaultTraits
{
    struct NotConvertible {};

    using tag = Tag_null;
    template <typename V> static NotConvertible to_json(V&&) {
        static_assert(impl::AlwaysFalse<T>::value, "Converting objects of type T to JSON is not supported. Implement Traits<T>::to_json.");
        return {};
    }
    template <typename V> static NotConvertible from_json(V&&) {
        static_assert(impl::AlwaysFalse<T>::value, "Converting JSON to objects of type T is not supported. Implement Traits<T>::from_json.");
        return {};
    }
};

template <> struct DefaultTraits<std::nullptr_t    > : DefaultTraits_null    {};
template <> struct DefaultTraits<bool              > : DefaultTraits_boolean {};
template <> struct DefaultTraits<double            > : DefaultTraits_float   {};
template <> struct DefaultTraits<float             > : DefaultTraits_float   {};
template <> struct DefaultTraits<signed char       > : DefaultTraits_int     {};
template <> struct DefaultTraits<signed short      > : DefaultTraits_int     {};
template <> struct DefaultTraits<signed int        > : DefaultTraits_int     {};
template <> struct DefaultTraits<signed long       > : DefaultTraits_int     {};
template <> struct DefaultTraits<signed long long  > : DefaultTraits_int     {};
template <> struct DefaultTraits<unsigned char     > : DefaultTraits_int     {};
template <> struct DefaultTraits<unsigned short    > : DefaultTraits_int     {};
template <> struct DefaultTraits<unsigned int      > : DefaultTraits_int     {};
template <> struct DefaultTraits<unsigned long     > : DefaultTraits_int     {};
template <> struct DefaultTraits<unsigned long long> : DefaultTraits_int     {};
template <> struct DefaultTraits<String            > : DefaultTraits_string  {};
template <> struct DefaultTraits<Array             > : DefaultTraits_array   {};
template <> struct DefaultTraits<Object            > : DefaultTraits_object  {};

template <>
struct DefaultTraits<char const*>
{
    using tag = Tag_string;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
#if 1
    template <typename V> static decltype(auto) from_json(V&&) = delete;
#else
    template <typename V> static decltype(auto) from_json(V&& in)
    {
        static_assert(std::is_lvalue_reference<V>::value, "Dangling pointer");
        return in.as_string().c_str();
    }
#endif
};

template <>
struct DefaultTraits<char*>
{
    using tag = Tag_string;
    template <typename V> static decltype(auto) to_json(V&& in) { return std::forward<V>(in); }
#if 1
    template <typename V> static decltype(auto) from_json(V&&) = delete;
#else
    template <typename V> static decltype(auto) from_json(V&& in)
    {
#if __cplusplus >= 201703 || (_MSC_VER >= 1910 && _HAS_CXX17)
        static_assert(std::is_lvalue_reference<V>::value, "Dangling pointer");
        return in.as_string().data(); // works since C++17
#else
#if 0
        static_assert(std::is_lvalue_reference<V>::value, "Dangling pointer");
        return in.as_string().empty() ? static_cast<char*>(nullptr) : &in.as_string()[0];
#else
        static_assert(AlwaysFalse<V>::value, "Unsupported conversion");
        return static_cast<char*>(nullptr);
#endif
#endif
    }
#endif
};

template <>
struct DefaultTraits<cxx::string_view>
{
    using tag = Tag_string;
    template <typename V> static decltype(auto) to_json(V&& in) { return String(in); }
    template <typename V> static decltype(auto) from_json(V&& in)
    {
        static_assert(std::is_lvalue_reference<V>::value, "Dangling pointer");
        return cxx::string_view(in.as_string());
    }
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
using Traits_t = Traits<std::decay_t<T>>;

template <typename T>
using Tag_t = typename Traits_t<T>::tag;

template <typename T>
using TargetType_t = typename TargetType<Tag_t<T>::value>::type;

template <typename T>
using SourceType_t = decltype(( Traits_t<T>::to_json(std::declval<T>()) ));

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
    Type type_ = Type::null;

public:
    Value() noexcept = default;
   ~Value() noexcept
    {
        assign_null();
    }

    Value(Value const& rhs);
    Value(Value&& rhs) noexcept
        : data_(rhs.data_)
        , type_(std::exchange(rhs.type_, Type::null))
    {
    }

    Value& operator=(Value const& rhs);
    Value& operator=(Value&& rhs) noexcept
    {
        data_ = rhs.data_;
        type_ = std::exchange(rhs.type_, Type::null);
        return *this;
    }

private:
    template <typename T>
    using AllowConversion
        = std::integral_constant<bool,
            !std::is_same< Value, std::decay_t<T> >::value>;

    template <typename T>
    using IsConvertible
        = std::integral_constant<bool,
            std::is_same< std::decay_t<SourceType_t<T>>, Value >::value // to_json might return a 'Value'
            || std::is_convertible< SourceType_t<T>, TargetType_t<T> >::value>;

    template <typename T>
    using IsConstructible
        = std::integral_constant<bool,
            std::is_same< std::decay_t<SourceType_t<T>>, Value >::value // to_json might return a 'Value'
            || std::is_constructible< TargetType_t<T>, SourceType_t<T> >::value>;

    Value(Tag_null,    Null           ) noexcept : type_(Type::null) {}
    Value(Tag_boolean, bool          v) noexcept : type_(Type::boolean) { data_.boolean = v; }
    Value(Tag_number,  double        v) noexcept : type_(Type::number) { data_.number = v; }
    Value(Tag_string,  String const& v);
    Value(Tag_string,  String&&      v);
    Value(Tag_array,   Array const&  v);
    Value(Tag_array,   Array&&       v);
    Value(Tag_object,  Object const& v);
    Value(Tag_object,  Object&&      v);

    // to_json might return a 'Value'
    template <Type Ty> Value(Type_const<Ty>, Value const& v) : Value(v) {}
    template <Type Ty> Value(Type_const<Ty>, Value&&      v) : Value(std::move(v)) {}

public:
    Value(Type t);

    template <typename T, std::enable_if_t< AllowConversion<T>::value && IsConvertible<T>::value, int > = 0>
    Value(T&& v)
        : Value(Tag_t<T>{}, Traits_t<T>::to_json(std::forward<T>(v)))
    {
    }

    template <typename T, std::enable_if_t< AllowConversion<T>::value && !IsConvertible<T>::value && IsConstructible<T>::value, int > = 1>
    explicit Value(T&& v)
        : Value(Tag_t<T>{}, static_cast<TargetType_t<T>>( Traits_t<T>::to_json(std::forward<T>(v)) ))
    {
    }

#if JSON_VALUE_HAS_IMPLICIT_INITIALIZER_LIST_CONSTRUCTOR
    Value(std::initializer_list<std::pair<String, Value>> il)
        : Value(Tag_object{}, il)
    {
    }
#endif

private:
    void _assign_from(Tag_null,    Null           ) noexcept { assign_null(); }
    void _assign_from(Tag_boolean, bool          v) noexcept { assign_boolean(v); }
    void _assign_from(Tag_number,  double        v) noexcept { assign_number(v); }
    void _assign_from(Tag_string,  String const& v)          { assign_string(v); }
    void _assign_from(Tag_string,  String&&      v)          { assign_string(std::move(v)); }
    void _assign_from(Tag_array,   Array const&  v)          { assign_array(v); }
    void _assign_from(Tag_array,   Array&&       v)          { assign_array(std::move(v)); }
    void _assign_from(Tag_object,  Object const& v)          { assign_object(v); }
    void _assign_from(Tag_object,  Object&&      v)          { assign_object(std::move(v)); }

    // to_json might return a 'Value'
    template <Type Ty> void _assign_from(Type_const<Ty>, Value const& v) { *this = v; }
    template <Type Ty> void _assign_from(Type_const<Ty>, Value&&      v) { *this = std::move(v); }

public:
    template <typename T, std::enable_if_t< AllowConversion<T>::value && IsConvertible<T>::value, int > = 0>
    Value& operator=(T&& v)
    {
        _assign_from(Tag_t<T>{}, Traits_t<T>::to_json(std::forward<T>(v)));
        return *this;
    }

    // Construct from any array
    template <typename T>
    static Value from_array(T const& v)
    {
        return Array(v.begin(), v.end());
    }

    // Construct from any object
    template <typename T>
    static Value from_object(T const& v)
    {
        return Object(v.begin(), v.end());
    }

    // Returns the type of the actual value stored in this JSON object.
    Type type() const noexcept
    {
        return type_;
    }

    // is_X returns whether the actual value stored in this JSON object is of type X.
    bool is_null()    const noexcept { return type() == Type::null;    }
    bool is_boolean() const noexcept { return type() == Type::boolean; }
    bool is_number()  const noexcept { return type() == Type::number;  }
    bool is_string()  const noexcept { return type() == Type::string;  }
    bool is_array()   const noexcept { return type() == Type::array;   }
    bool is_object()  const noexcept { return type() == Type::object;  }

    bool is(Type t) const noexcept { return type() == t; }

    // get_X returns a reference to the value of type X stored in this JSON object.
    // PRE: is_X() == true

    bool& as_boolean() & noexcept
    {
        assert(is_boolean());
        return data_.boolean;
    }

    bool const& as_boolean() const& noexcept
    {
        assert(is_boolean());
        return data_.boolean;
    }

    bool as_boolean() && noexcept
    {
        assert(is_boolean());
        return data_.boolean;
    }

    double& as_number() & noexcept
    {
        assert(is_number());
        return data_.number;
    }

    double const& as_number() const& noexcept
    {
        assert(is_number());
        return data_.number;
    }

    double as_number() && noexcept
    {
        assert(is_number());
        return data_.number;
    }

    String& as_string() & noexcept
    {
        assert(is_string());
        return *data_.string;
    }

    String const& as_string() const& noexcept
    {
        assert(is_string());
        return *data_.string;
    }

    String as_string() && noexcept
    {
        assert(is_string());
        return std::move(*data_.string);
    }

    Array& as_array() & noexcept
    {
        assert(is_array());
        return *data_.array;
    }

    Array const& as_array() const& noexcept
    {
        assert(is_array());
        return *data_.array;
    }

    Array as_array() && noexcept
    {
        assert(is_array());
        return std::move(*data_.array);
    }

    Object& as_object() & noexcept
    {
        assert(is_object());
        return *data_.object;
    }

    Object const& as_object() const& noexcept
    {
        assert(is_object());
        return *data_.object;
    }

    Object as_object() && noexcept
    {
        assert(is_object());
        return std::move(*data_.object);
    }

    // assign_X assigns a value of type X to this JSON object.
    // POST: is_X() == true
    //
    // If this JSON object already contains a value of type X, the copy/move
    // assignment operator is used. Otherwise the copy/move constructor will be
    // used to construct a new X.

    void    assign_null   () noexcept;
    bool&   assign_boolean(bool          v) noexcept;
    double& assign_number (double        v) noexcept;
    String& assign_string (String const& v);
    String& assign_string (String&&      v);
    Array&  assign_array  (Array const&  v);
    Array&  assign_array  (Array&&       v);
    Object& assign_object (Object const& v);
    Object& assign_object (Object&&      v);

    // isa<T>

    template <typename T>
    bool isa() const noexcept { return type() == Tag_t<T>::value; }

    // to<T>

    template <typename T> decltype(auto) cast() const&  noexcept { return Traits_t<T>::from_json(static_cast<Value const& >(*this)); }
    template <typename T> decltype(auto) cast() &&      noexcept { return Traits_t<T>::from_json(static_cast<Value &&     >(*this)); }

#if JSON_VALUE_HAS_EXPLICIT_OPERATOR_T
    template <typename T> explicit operator T() const&  noexcept { return this->cast<T>(); }
    template <typename T> explicit operator T() &&      noexcept { return this->cast<T>(); }
#endif

    bool&    create_boolean() noexcept;
    double&  create_number() noexcept;
    String&  create_string();
    Array&   create_array();
    Object&  create_object();

    // inplace_convert_to_X converts the value stored in this JSON object into a value of type X.
    //
    // If the value stored in this JSON object is already of type X, calling
    // this function has no effect.
    // The conversion is similiar to JavaScript's ToBoolean, ToNumber, etc.

    bool&    inplace_convert_to_boolean() noexcept;
    double&  inplace_convert_to_number() noexcept;
    String&  inplace_convert_to_string();
    Array&   inplace_convert_to_array();
    Object&  inplace_convert_to_object();

    // Embed the current JSON value into an array.
    Array& embed_in_array();

    // Embed the current JSON value into an object.
    Object& embed_in_object(String key);

    // convert_to_X returns a copy of the value stored in this JSON object converted to X.
    //
    // The conversion is similiar to JavaScript's ToBoolean, ToNumber, etc.

    bool   convert_to_boolean() const noexcept;
    double convert_to_number() const noexcept;
    String convert_to_string() const;
    Array  convert_to_array() const;
    Object convert_to_object() const;

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

// Arrays:

    using element_iterator       = Array::iterator;
    using const_element_iterator = Array::const_iterator;

    element_iterator       elements_begin()      & { return as_array().begin(); }
    element_iterator       elements_end()        & { return as_array().end();   }
    const_element_iterator elements_begin() const& { return as_array().begin(); }
    const_element_iterator elements_end()   const& { return as_array().end();   }

    template <typename It>
    struct Elements {
        It begin_;
        It end_;
        It begin() const { return begin_; }
        It end() const { return end_; }
    };

    Elements<element_iterator>       elements()      &  { return {elements_begin(), elements_end()}; }
    Elements<const_element_iterator> elements() const&  { return {elements_begin(), elements_end()}; }

    // Convert this value into an array an return a reference to the index-th element.
    // PRE: is_array() or is_null()
    Value& operator[](size_t index);

    // Returns a reference to the index-th element.
    // PRE: is_array()
    // PRE: size() > index
    Value const& operator[](size_t index) const noexcept;

    // Returns a pointer the the value at the given index.
    // Or nullptr if this value is not an array of if the index is out bounds.
    Value*       get_ptr(size_t index);
    Value const* get_ptr(size_t index) const;

    // Convert this value into an array and append a new element constructed from the given arguments.
    // PRE: is_array() or is_null()
    template <typename ...Args>
    Value& emplace_back(Args&&... args)
    {
        assert(is_null() || is_array());
        auto& arr = inplace_convert_to_array();
        arr.emplace_back(std::forward<Args>(args)...);
        return arr.back();
    }

    // Convert this value into an array and append a new element.
    // PRE: is_array() or is_null()
    Value& push_back(Value const& value) { return emplace_back(value); }
    Value& push_back(Value&&      value) { return emplace_back(std::move(value)); }

    // Remove the last element of the array.
    // PRE: is_array()
    // PRE: size() > 0
    void pop_back();

    // Erase the element at the given index.
    // PRE: is_array()
    // PRE: index < size()
    void erase(size_t index);

// Objects:

private:
    template <typename T>
    using EnableIfIsKey = std::enable_if_t<
        !std::is_integral<T>::value // disallow literal '0': String might be convertible from nullptr...
#if 1
        && std::is_convertible<
            decltype(std::declval<Object const&>().find(std::declval<T>())), typename Object::const_iterator >::value
#endif
    >;

public:
    using item_iterator       = Object::iterator;
    using const_item_iterator = Object::const_iterator;

    item_iterator       items_begin()      & { return as_object().begin(); }
    item_iterator       items_end()        & { return as_object().end();   }
    const_item_iterator items_begin() const& { return as_object().begin(); }
    const_item_iterator items_end()   const& { return as_object().end();   }

    template <typename It>
    struct Items {
        It begin_;
        It end_;
        It begin() const { return begin_; }
        It end() const { return end_; }
    };

    Items<item_iterator>       items()      &  { return {items_begin(), items_end()}; }
    Items<const_item_iterator> items() const&  { return {items_begin(), items_end()}; }

    // Convert this value into an object and return a reference to the value with the given key.
    // PRE: is_object() or is_null()
    Value& operator[](Object::key_type const& key);
    Value& operator[](Object::key_type&& key);

    // Returns a reference to the value with the given key.
    // PRE: is_object()
    // PRE: as_object().find(key) != as_object().end()
    template <typename T, typename = EnableIfIsKey<T>>
    Value const& operator[](T&& key) const noexcept
    {
        auto&& obj = as_object();
        auto it = obj.find(std::forward<T>(key));
        assert(it != obj.end());
        return it->second;
    }

    // Returns a pointer to the value with the given key.
    // Or nullptr if no such key exists, or this value is not an object.
    template <typename T, typename = EnableIfIsKey<T>>
    Value* get_ptr(T&& key)
    {
        if (is_object()) {
            auto&& obj = as_object();
            auto it = obj.find(std::forward<T>(key));
            if (it != obj.end()) {
                return &it->second;
            }
        }
        return nullptr;
    }
    template <typename T, typename = EnableIfIsKey<T>>
    Value const* get_ptr(T&& key) const
    {
        if (is_object()) {
            auto&& obj = as_object();
            auto it = obj.find(std::forward<T>(key));
            if (it != obj.end()) {
                return &it->second;
            }
        }
        return nullptr;
    }

    // Returns whether a value with the given key exists.
    template <typename T, typename = EnableIfIsKey<T>>
    bool has_member(T&& key) const
    {
        return get_ptr(std::forward<T>(key)) != nullptr;
    }

    // Convert this value into an object and insert a new element constructed from the given arguments.
    // PRE: is_object() or is_null()
    template <typename ...Args>
    std::pair<item_iterator, bool> emplace(Args&&... args)
    {
        assert(is_null() || is_object());
        return inplace_convert_to_object().emplace(std::forward<Args>(args)...);
    }

    // Convert this value to an object and insert the given {key, value} pair.
    // PRE: is_object() or is_null()
    std::pair<item_iterator, bool> insert(Object::value_type const& pair) { return emplace(pair); }
    std::pair<item_iterator, bool> insert(Object::value_type&&      pair) { return emplace(std::move(pair)); }

    // Erase the the given key.
    // PRE: is_object()
    template <typename T, typename = EnableIfIsKey<T>>
    size_t erase(T&& key)
    {
        return as_object().erase(std::forward<T>(key));
    }

    // Erase the the given key.
    // PRE: is_object()
    item_iterator erase(const_item_iterator pos);

private:
    template <typename ...Args> String& _assign_string(Args&&... args);
    template <typename ...Args> Array&  _assign_array (Args&&... args);
    template <typename ...Args> Object& _assign_object(Args&&... args);
};

template <typename T> inline decltype(auto) cast(Value const&  val) { return val.template cast<T>(); }
template <typename T> inline decltype(auto) cast(Value&&       val) { return std::move(val).template cast<T>(); }

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
    return !(lhs == rhs);
}

inline bool operator<(Value const& lhs, Value const& rhs) noexcept
{
    return lhs.less_than(rhs);
}

inline bool operator>=(Value const& lhs, Value const& rhs) noexcept
{
    return !(lhs < rhs);
}

inline bool operator>(Value const& lhs, Value const& rhs) noexcept
{
    return rhs < lhs;
}

inline bool operator<=(Value const& lhs, Value const& rhs) noexcept
{
    return !(rhs < lhs);
}

namespace impl
{
    template <typename T> bool CmpEQ(Value const& lhs, T const&,     Tag_null   ) noexcept { return lhs.is_null(); }
    template <typename T> bool CmpEQ(Value const& lhs, T const& rhs, Tag_boolean) noexcept { return lhs.type() == Type::boolean && lhs.as_boolean() == rhs; }
    template <typename T> bool CmpEQ(Value const& lhs, T const& rhs, Tag_number ) noexcept { return lhs.type() == Type::number  && lhs.as_number () == rhs; }
    template <typename T> bool CmpEQ(Value const& lhs, T const& rhs, Tag_string ) noexcept { return lhs.type() == Type::string  && lhs.as_string () == rhs; }
    template <typename T> bool CmpEQ(Value const& lhs, T const& rhs, Tag_array  ) noexcept { return lhs.type() == Type::array   && lhs.as_array  () == rhs; }
    template <typename T> bool CmpEQ(Value const& lhs, T const& rhs, Tag_object ) noexcept { return lhs.type() == Type::object  && lhs.as_object () == rhs; }

    template <typename T> bool CmpLT(Value const&,     T const&,     Tag_null   ) noexcept { return false; } // type < null || (type == null && nullptr < nullptr)
    template <typename T> bool CmpLT(Value const& lhs, T const& rhs, Tag_boolean) noexcept { return lhs.type() < Type::boolean || (lhs.type() == Type::boolean && lhs.as_boolean() < rhs); }
    template <typename T> bool CmpLT(Value const& lhs, T const& rhs, Tag_number ) noexcept { return lhs.type() < Type::number  || (lhs.type() == Type::number  && lhs.as_number () < rhs); }
    template <typename T> bool CmpLT(Value const& lhs, T const& rhs, Tag_string ) noexcept { return lhs.type() < Type::string  || (lhs.type() == Type::string  && lhs.as_string () < rhs); }
    template <typename T> bool CmpLT(Value const& lhs, T const& rhs, Tag_array  ) noexcept { return lhs.type() < Type::array   || (lhs.type() == Type::array   && lhs.as_array  () < rhs); }
    template <typename T> bool CmpLT(Value const& lhs, T const& rhs, Tag_object ) noexcept { return lhs.type() < Type::object  || (lhs.type() == Type::object  && lhs.as_object () < rhs); }

    template <typename T> bool CmpGT(Value const& lhs, T const&,     Tag_null   ) noexcept { return !lhs.is_null(); } // null < type || (null == type && nullptr < nullptr)
    template <typename T> bool CmpGT(Value const& lhs, T const& rhs, Tag_boolean) noexcept { return Type::boolean < lhs.type() || (Type::boolean == lhs.type() && rhs < lhs.as_boolean()); }
    template <typename T> bool CmpGT(Value const& lhs, T const& rhs, Tag_number ) noexcept { return Type::number  < lhs.type() || (Type::number  == lhs.type() && rhs < lhs.as_number ()); }
    template <typename T> bool CmpGT(Value const& lhs, T const& rhs, Tag_string ) noexcept { return Type::string  < lhs.type() || (Type::string  == lhs.type() && rhs < lhs.as_string ()); }
    template <typename T> bool CmpGT(Value const& lhs, T const& rhs, Tag_array  ) noexcept { return Type::array   < lhs.type() || (Type::array   == lhs.type() && rhs < lhs.as_array  ()); }
    template <typename T> bool CmpGT(Value const& lhs, T const& rhs, Tag_object ) noexcept { return Type::object  < lhs.type() || (Type::object  == lhs.type() && rhs < lhs.as_object ()); }
}

// Value == T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator==(L const& lhs, R const& rhs) noexcept
{
    return impl::CmpEQ(lhs, rhs, Tag_t<R>{});
}

// T == Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator==(L const& lhs, R const& rhs) noexcept
{
    return impl::CmpEQ(rhs, lhs, Tag_t<L>{});
}

// Value != T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator!=(L const& lhs, R const& rhs) noexcept
{
    return !(lhs == rhs);
}

// T != Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator!=(L const& lhs, R const& rhs) noexcept
{
    return !(lhs == rhs);
}

// Value < T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator<(L const& lhs, R const& rhs) noexcept
{
    return impl::CmpLT(lhs, rhs, Tag_t<R>{});
}

// T < Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator<(L const& lhs, R const& rhs) noexcept
{
    return impl::CmpGT(rhs, lhs, Tag_t<L>{});
}

// Value >= T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator>=(L const& lhs, R const& rhs) noexcept
{
    return !(lhs < rhs);
}

// T >= Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator>=(L const& lhs, R const& rhs) noexcept
{
    return !(lhs < rhs);
}

// Value > T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator>(L const& lhs, R const& rhs) noexcept
{
    return rhs < lhs;
}

// T > Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator>(L const& lhs, R const& rhs) noexcept
{
    return rhs < lhs;
}

// Value <= T
template < typename L, typename R, std::enable_if_t< std::is_same<Value, L>::value && !std::is_same<Value, R>::value, int > = 0 >
bool operator<=(L const& lhs, R const& rhs) noexcept
{
    return !(rhs < lhs);
}

// T <= Value
template < typename L, typename R, std::enable_if_t< !std::is_same<Value, L>::value && std::is_same<Value, R>::value, int > = 1 >
bool operator<=(L const& lhs, R const& rhs) noexcept
{
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
ErrorCode parse(Value& value, cxx::string_view str, ParseOptions const& options = {});

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
    // Default is true.
    bool allow_invalid_unicode = true;

    // If true, converts the special numbers NaN and infinity to nan_string and
    // inf_string, resp. Otherwise they are converted to "null".
    // Default is true.
    bool allow_nan_inf = true;

    // If true, escapes '/' in strings. This allows the JSON string to be
    // embedded in HTML.
    // Default is false.
    bool escape_slash = false;
};

// Write a stringified version of the given value to str.
// Returns true if successful.
// Returns false only if the JSON value contains invalid UTF-8 strings and
// options.allow_invalid_unicode is false.
bool stringify(std::string& str, Value const& value, StringifyOptions const& options = {});

} // namespace json
