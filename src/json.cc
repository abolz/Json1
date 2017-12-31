#include "json.h"
#include "__conv.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>

#ifndef JSON_HAS_DOUBLE_CONVERSION
#define JSON_HAS_DOUBLE_CONVERSION 0
#endif
#ifndef JSON_HAS_STRTOD_L
#define JSON_HAS_STRTOD_L 0
#endif

#ifndef JSON_UNREACHABLE
#if defined(_MSC_VER)
#define JSON_UNREACHABLE() __assume(false)
#elif defined(__GNUC__)
#define JSON_UNREACHABLE() __builtin_unreachable()
#else
#define JSON_UNREACHABLE() abort()
#endif
#endif

using namespace json;

//==================================================================================================
// Number conversions
//==================================================================================================

static constexpr char const* const kNaNString = "NaN";
static constexpr char const* const kInfString = "Infinity";

#if JSON_HAS_DOUBLE_CONVERSION
#include <double-conversion/double-conversion.h>

static double Strtod(char const* str, int len)
{
    using double_conversion::StringToDoubleConverter;

    StringToDoubleConverter conv(
        StringToDoubleConverter::ALLOW_LEADING_SPACES |
        StringToDoubleConverter::ALLOW_TRAILING_SPACES,
        0.0,                                        // empty_string_value
        std::numeric_limits<double>::quiet_NaN(),   // junk_string_value,
        kInfString,                                 // infinity_symbol,
        kNaNString);                                // nan_symbol

    int processed = 0;
    return conv.StringToDouble(str, len, &processed);
}

#elif defined(_MSC_VER)
#include <clocale>
#include <cstdio>

namespace
{
    struct ClassicLocale {
        const ::_locale_t loc;
        ClassicLocale() noexcept : loc(::_create_locale(LC_ALL, "C")) {}
       ~ClassicLocale() noexcept { ::_free_locale(loc); }
    };
}
static const ClassicLocale s_clocale;

static double Strtod(char const* c_str, char** end)
{
    return ::_strtod_l(c_str, end, s_clocale.loc);
}

#elif JSON_HAS_STRTOD_L // not tested...
#include <clocale>
#include <cstdio>

namespace
{
    struct ClassicLocale {
        const ::locale_t loc;
        ClassicLocale() noexcept : loc(::newlocale(LC_ALL, "C", 0)) {}
       ~ClassicLocale() noexcept { ::freelocale(loc); }
    };
}
static const ClassicLocale s_clocale;

static double Strtod(char const* c_str, char** end)
{
    return ::strtod_l(c_str, end, s_clocale.loc);
}

#else
#include <cstdio>

// FIXME!
static double Strtod(char const* c_str, char** end)
{
    return ::strtod(c_str, end);
}

#endif

#if !JSON_HAS_DOUBLE_CONVERSION

static double Strtod(char const* str, int len)
{
    static constexpr int const kBufSize = 200;

    assert(str != nullptr);
    assert(len >= 0);

    if (len < kBufSize)
    {
        char buf[kBufSize];
        std::memcpy(buf, str, static_cast<size_t>(len));
        buf[len] = '\0';

        return Strtod(buf, nullptr);
    }

    std::vector<char> buf(str, str + len);
    buf.push_back('\0');

    return Strtod(buf.data(), nullptr);
}

#endif

//==================================================================================================
// Unicode support
//==================================================================================================

namespace {
namespace unicode {

using Codepoint = uint32_t;

static constexpr Codepoint const kInvalidCodepoint = 0xFFFFFFFF;
//static constexpr Codepoint const kReplacementCharacter = 0xFFFD;

enum class ErrorCode {
    success,
    insufficient_data,
    invalid_codepoint,
    invalid_lead_byte,
    invalid_continuation_byte,
    overlong_sequence,
    invalid_high_surrogate,
    invalid_low_surrogate,
};

struct DecodedCodepoint
{
    Codepoint U;
    ErrorCode ec;
};

static bool IsValidCodePoint(Codepoint U)
{
    // 1. Characters with values greater than 0x10FFFF cannot be encoded in
    //    UTF-16.
    // 2. Values between 0xD800 and 0xDFFF are specifically reserved for use
    //    with UTF-16, and don't have any characters assigned to them.
    return U <= 0x10FFFF && (U < 0xD800 || U > 0xDFFF);
}

static int GetUTF8SequenceLengthFromCodepoint(Codepoint U)
{
    assert(IsValidCodePoint(U));

    if (U <= 0x7F) return 1;
    if (U <= 0x7FF) return 2;
    if (U <= 0xFFFF) return 3;
    return 4;
}

static bool IsUTF8OverlongSequence(Codepoint U, int slen)
{
    return slen != GetUTF8SequenceLengthFromCodepoint(U);
}

static int GetUTF8SequenceLengthFromLeadByte(char ch)
{
    uint8_t const b = static_cast<uint8_t>(ch);

    if (b < 0x80) return 1;
    if (b < 0xC0) return 0; // 10xxxxxx, i.e. continuation byte
    if (b < 0xE0) return 2;
    if (b < 0xF0) return 3;
    if (b < 0xF8) return 4;
    return 0; // invalid
}

static int GetUTF8SequenceLengthFromLeadByte(char ch, Codepoint& U)
{
    uint8_t const b = static_cast<uint8_t>(ch);

    if (b < 0x80) { U = b;        return 1; }
    if (b < 0xC0) {               return 0; }
    if (b < 0xE0) { U = b & 0x1F; return 2; }
    if (b < 0xF0) { U = b & 0x0F; return 3; }
    if (b < 0xF8) { U = b & 0x07; return 4; }
    return 0;
}

static bool IsUTF8LeadByte(char ch)
{
    return 0 != GetUTF8SequenceLengthFromLeadByte(ch);
}

static char const* FindNextUTF8LeadByte(char const* first, char const* last)
{
    for ( ; first != last && !IsUTF8LeadByte(*first); ++first)
    {
    }

    return first;
}

static bool IsUTF8ContinuationByte(char ch)
{
    uint8_t const b = static_cast<uint8_t>(ch);

    return 0x80 == (b & 0xC0); // b == 10xxxxxx ???
}

static DecodedCodepoint DecodeUTF8Sequence(char const*& next, char const* last)
{
    //
    // Char. number range  |        UTF-8 octet sequence
    //    (hexadecimal)    |              (binary)
    // --------------------+---------------------------------------------
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    //
    // Decoding a UTF-8 character proceeds as follows:
    //
    // 1.  Initialize a binary number with all bits set to 0.  Up to 21 bits
    // may be needed.
    //
    // 2.  Determine which bits encode the character number from the number
    // of octets in the sequence and the second column of the table
    // above (the bits marked x).
    //
    // 3.  Distribute the bits from the sequence to the binary number, first
    // the lower-order bits from the last octet of the sequence and
    // proceeding to the left until no x bits are left.  The binary
    // number is now equal to the character number.
    //

    if (next == last)
        return { kInvalidCodepoint, ErrorCode::insufficient_data };

    Codepoint U = 0;
    const int slen = GetUTF8SequenceLengthFromLeadByte(*next, U);

    if (slen == 0)
        return { kInvalidCodepoint, ErrorCode::invalid_lead_byte };
    if (last - next < slen)
        return { kInvalidCodepoint, ErrorCode::insufficient_data };

    const auto end = next + slen;
    for (++next; next != end; ++next)
    {
        if (!IsUTF8ContinuationByte(*next))
            return { U, ErrorCode::invalid_continuation_byte };

        U = (U << 6) | (static_cast<uint8_t>(*next) & 0x3F);
    }

    //
    // Implementations of the decoding algorithm above MUST protect against
    // decoding invalid sequences.  For instance, a naive implementation may
    // decode the overlong UTF-8 sequence C0 80 into the character U+0000,
    // or the surrogate pair ED A1 8C ED BE B4 into U+233B4.  Decoding
    // invalid sequences may have security consequences or cause other
    // problems.
    //

    if (!IsValidCodePoint(U))
        return { U, ErrorCode::invalid_codepoint };
    if (IsUTF8OverlongSequence(U, slen))
        return { U, ErrorCode::overlong_sequence };

    return { U, ErrorCode::success };
}

template <typename Put8>
static bool EncodeUTF8(Codepoint U, Put8 put)
{
    assert(IsValidCodePoint(U));

    //
    // Char. number range  |        UTF-8 octet sequence
    //    (hexadecimal)    |              (binary)
    // --------------------+---------------------------------------------
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    //
    // Encoding a character to UTF-8 proceeds as follows:
    //
    // 1.  Determine the number of octets required from the character number
    // and the first column of the table above.  It is important to note
    // that the rows of the table are mutually exclusive, i.e., there is
    // only one valid way to encode a given character.
    //
    // 2.  Prepare the high-order bits of the octets as per the second
    // column of the table.
    //
    // 3.  Fill in the bits marked x from the bits of the character number,
    // expressed in binary.  Start by putting the lowest-order bit of
    // the character number in the lowest-order position of the last
    // octet of the sequence, then put the next higher-order bit of the
    // character number in the next higher-order position of that octet,
    // etc.  When the x bits of the last octet are filled in, move on to
    // the next to last octet, then to the preceding one, etc. until all
    // x bits are filled in.
    //

    if (U <= 0x7F)
    {
        return put(static_cast<char>(U));
    }
    else if (U <= 0x7FF)
    {
        return put(static_cast<char>(0xC0 | ((U >> 6)       )))
            && put(static_cast<char>(0x80 | ((U     ) & 0x3F)));
    }
    else if (U <= 0xFFFF)
    {
        return put(static_cast<char>(0xE0 | ((U >> 12)       )))
            && put(static_cast<char>(0x80 | ((U >>  6) & 0x3F)))
            && put(static_cast<char>(0x80 | ((U      ) & 0x3F)));
    }
    else /*if (U <= 0x10FFFF)*/
    {
        return put(static_cast<char>(0xF0 | ((U >> 18)       )))
            && put(static_cast<char>(0x80 | ((U >> 12) & 0x3F)))
            && put(static_cast<char>(0x80 | ((U >>  6) & 0x3F)))
            && put(static_cast<char>(0x80 | ((U      ) & 0x3F)));
    }
}

template <typename Get16>
static DecodedCodepoint DecodeUTF16Sequence(Get16 get)
{
    //
    // Decoding of a single character from UTF-16 to an ISO 10646 character
    // value proceeds as follows. Let W1 be the next 16-bit integer in the
    // sequence of integers representing the text. Let W2 be the (eventual)
    // next integer following W1.
    //

    uint16_t W1 = 0;

    if (!get(W1))
        return { 0, ErrorCode::insufficient_data };

    //
    // 1) If W1 < 0xD800 or W1 > 0xDFFF, the character value U is the value
    // of W1. Terminate.
    //

    if (W1 < 0xD800 || W1 > 0xDFFF)
        return { uint32_t{W1}, ErrorCode::success };

    //
    // 2) Determine if W1 is between 0xD800 and 0xDBFF. If not, the sequence
    // is in error and no valid character can be obtained using W1.
    // Terminate.
    //

    if (W1 > 0xDBFF)
        return { uint32_t{W1}, ErrorCode::invalid_high_surrogate };

    //
    // 3) If there is no W2 (that is, the sequence ends with W1), or if W2
    // is not between 0xDC00 and 0xDFFF, the sequence is in error.
    // Terminate.
    //

    uint16_t W2 = 0;

    if (!get(W2))
        return { uint32_t{W1}, ErrorCode::insufficient_data };

    if (W2 < 0xDC00 || W2 > 0xDFFF)
        return { uint32_t{W2}, ErrorCode::invalid_low_surrogate }; // Note: returns W2

    //
    // 4) Construct a 20-bit unsigned integer U', taking the 10 low-order
    // bits of W1 as its 10 high-order bits and the 10 low-order bits of
    // W2 as its 10 low-order bits.
    //
    //
    // 5) Add 0x10000 to U' to obtain the character value U. Terminate.
    //

    Codepoint const Up = ((Codepoint{W1} & 0x3FF) << 10) | (Codepoint{W2} & 0x3FF);
    Codepoint const U = Up + 0x10000;

    return { U, ErrorCode::success };
}

} // namespace unicode
} // namespace

//==================================================================================================
// Value
//==================================================================================================

Value::Value(Value const& rhs)
{
    switch (rhs.type_)
    {
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
    default:
        JSON_UNREACHABLE();
        break;
    }
}

Value& Value::operator=(Value const& rhs)
{
    if (this != &rhs)
    {
        switch (rhs.type())
        {
        case Type::null:
            assign_null();
            break;
        case Type::boolean:
            assign_boolean(rhs.as_boolean());
            break;
        case Type::number:
            assign_number(rhs.as_number());
            break;
        case Type::string:
            _assign_string(rhs.as_string());
            break;
        case Type::array:
            _assign_array(rhs.as_array());
            break;
        case Type::object:
            _assign_object(rhs.as_object());
            break;
        default:
            JSON_UNREACHABLE();
            break;
        }
    }

    return *this;
}

Value::Value(Tag_string, String const& v)
{
    data_.string = new String(v);
    type_ = Type::string;
}

Value::Value(Tag_string, String&& v)
{
    data_.string = new String(std::move(v));
    type_ = Type::string;
}

Value::Value(Tag_array, Array const& v)
{
    data_.array = new Array(v);
    type_ = Type::array;
}

Value::Value(Tag_array, Array&& v)
{
    data_.array = new Array(std::move(v));
    type_ = Type::array;
}

Value::Value(Tag_object, Object const& v)
{
    data_.object = new Object(v);
    type_ = Type::object;
}

Value::Value(Tag_object, Object&& v)
{
    data_.object = new Object(std::move(v));
    type_ = Type::object;
}

Value::Value(Type t)
{
    switch (t)
    {
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
        JSON_UNREACHABLE();
        break;
    }
    type_ = t; // Don't move to constructor initializer list!
}

void Value::assign_null() noexcept
{
    switch (type_)
    {
    case Type::null:
    case Type::boolean:
    case Type::number:
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
    default:
        JSON_UNREACHABLE();
        break;
    }

    type_ = Type::null;
}

bool& Value::assign_boolean(bool v) noexcept
{
    assign_null(); // release resources

    data_.boolean = v;
    type_ = Type::boolean;

    return as_boolean();
}

double& Value::assign_number(double v) noexcept
{
    assign_null(); // release resources

    data_.number = v;
    type_ = Type::number;

    return as_number();
}

String& Value::assign_string(String const& v)
{
    return _assign_string(v);
}

String& Value::assign_string(String&& v)
{
    return _assign_string(std::move(v));
}

Array& Value::assign_array(Array const& v)
{
    return _assign_array(v);
}

Array& Value::assign_array(Array&& v)
{
    return _assign_array(std::move(v));
}

Object& Value::assign_object(Object const& v)
{
    return _assign_object(v);
}

Object& Value::assign_object(Object&& v)
{
    return _assign_object(std::move(v));
}

bool& Value::create_boolean() noexcept
{
    assign_null(); // release resources

    data_.boolean = bool{};
    type_ = Type::boolean;

    return as_boolean();
}

double& Value::create_number() noexcept
{
    assign_null(); // release resources

    data_.number = double{};
    type_ = Type::number;

    return as_number();
}

String& Value::create_string()
{
    return _assign_string();
}

Array& Value::create_array()
{
    return _assign_array();
}

Object& Value::create_object()
{
    return _assign_object();
}

//
// https://www.ecma-international.org/publications/files/ECMA-ST/Ecma-262.pdf
// Section 7.1.2
//
//  The abstract operation ToBoolean converts argument to a value of type
//  Boolean according to Table 9:
//
//  Table 9: ToBoolean Conversions
//
//  Argument Type   Result
//--------------------------------------------------------------------------
//  Undefined       Return false.
//  Null            Return false.
//  Boolean         Return argument.
//  Number          If argument is +0, -0, or NaN, return false; otherwise
//                  return true.
//  String          If argument is the empty String (its length is zero), return
//                  false; otherwise return true.
//  Symbol          Return true.
//  Object          Return true.
//

static bool BooleanFromNumber(double value) noexcept
{
    return !std::isnan(value) && value != 0.0;
}

static bool BooleanFromString(String const& value) noexcept
{
    return !value.empty();
}

static bool ToBoolean(Value const& value) noexcept
{
    switch (value.type())
    {
    case Type::null:
        return false;
    case Type::boolean:
        return value.as_boolean();
    case Type::number:
        return BooleanFromNumber(value.as_number());
    case Type::string:
        return BooleanFromString(value.as_string());
    case Type::array:
        return true;
    case Type::object:
        return true;
    default:
        JSON_UNREACHABLE();
        break;
    }
}

bool& Value::inplace_convert_to_boolean() noexcept
{
    if (!is_boolean())
    {
        auto new_val = ToBoolean(*this);
        assign_boolean(new_val);
    }

    return as_boolean();
}

//
// https://www.ecma-international.org/publications/files/ECMA-ST/Ecma-262.pdf
// Section 7.1.3
//
//  The abstract operation ToNumber converts argument to a value of type Number
//  according to Table 10:
//
//  Table 10: ToNumber Conversions
//
//  Argument Type   Result
//--------------------------------------------------------------------------
//  Undefined       Return NaN.
//  Null            Return +0.
//  Boolean         If argument is true, return 1.
//                  If argument is false, return +0.
//  Number          Return argument (no conversion).
//  String          See grammar and conversion algorithm below.
//  Symbol          Throw a TypeError exception.
//  Object          Apply the following steps:
//                      1. Let primValue be ToPrimitive(argument, hint Number).
//                      2. Return ? ToNumber(primValue).
//

static double NumberFromBoolean(bool value) noexcept
{
    return value ? 1.0 : +0.0;
}

static double NumberFromString(String const& value) noexcept
{
    // TODO:
    // To be compatible with parse() this should handle all different NaN and Infinity forms...

#if JSON_HAS_DOUBLE_CONVERSION
    assert(value.size() <= INT_MAX);
    int const len = static_cast<int>(value.size());

    return Strtod(value.c_str(), len);
#else
    return Strtod(value.c_str(), nullptr);
#endif
}

static double NumberFromArray(Array const& value) noexcept
{
    if (value.empty())
        return 0;

    if (value.size() == 1)
    {
        // Note:
        // NOT recursive!

        auto const& v = value[0];
        switch (v.type())
        {
        case Type::number:
            return v.as_number();
        case Type::string:
            return NumberFromString(v.as_string());
        default:
            break;
        }
    }

    return std::numeric_limits<double>::quiet_NaN();
}

static double ToNumber(Value const& value) noexcept
{
    switch (value.type())
    {
    case Type::null:
        return +0.0;
    case Type::boolean:
        return NumberFromBoolean(value.as_boolean());
    case Type::number:
        return value.as_number();
    case Type::string:
        return NumberFromString(value.as_string());
    case Type::array:
        return NumberFromArray(value.as_array());
    case Type::object:
        return std::numeric_limits<double>::quiet_NaN();
    default:
        JSON_UNREACHABLE();
        break;
    }
}

double& Value::inplace_convert_to_number() noexcept
{
    if (!is_number())
    {
        auto new_val = ToNumber(*this);
        assign_number(new_val);
    }

    return as_number();
}

//
// https://www.ecma-international.org/publications/files/ECMA-ST/Ecma-262.pdf
// Section 7.1.12
//
//  The abstract operation ToString converts argument to a value of type String
//  according to Table 11:
//
//  Table 11: ToString Conversions
//
//  Argument Type   Result
//--------------------------------------------------------------------------
//  Undefined       Return "undefined".
//  Null            Return "null".
//  Boolean         If argument is true, return "true".
//                  If argument is false, return "false".
//  Number          See 7.1.12.1
//  String          Return argument.
//  Symbol          Throw a TypeError exception.
//  Object          Apply the following steps:
//                      1. Let primValue be ToPrimitive(argument, hint String).
//                      2. Return ? ToString(primValue).
//

static String ToString(Value const& value);

static String StringFromBoolean(bool value)
{
    return value ? "true" : "false";
}

static String StringFromNumber(double value)
{
    char buf[32];
    char* end = dconv::ToShort(buf, buf + 32, value);

    return String(buf, end);
}

static String StringFromArray(Array const& value)
{
    String str;

    auto       I = value.begin();
    auto const E = value.end();
    if (I != E)
    {
        for (;;)
        {
            str += ToString(*I);
            if (++I == E)
                break;
            str += ',';
        }
    }

    return str;
}

static String ToString(Value const& value)
{
    switch (value.type())
    {
    case Type::null:
        return "null";
    case Type::boolean:
        return StringFromBoolean(value.as_boolean());
    case Type::number:
        return StringFromNumber(value.as_number());
    case Type::string:
        return value.as_string();
    case Type::array:
        return StringFromArray(value.as_array());
    case Type::object:
        // XXX:
        // Yes, doesn't make much sense...
        return "[object Object]";
    default:
        JSON_UNREACHABLE();
        break;
    }
}

String& Value::inplace_convert_to_string()
{
    if (!is_string())
    {
        auto new_val = ToString(*this);
        assign_string(std::move(new_val));
    }

    return as_string();
}

Array& Value::inplace_convert_to_array()
{
    switch (type())
    {
    case Type::null:
        data_.array = new Array();
        type_ = Type::array;
        break;
    case Type::boolean:
        {
            auto p = std::make_unique<Array>();
            p->emplace_back(data_.boolean);
            // nothrow ->
            data_.array = p.release();
            type_ = Type::array;
        }
        break;
    case Type::number:
        {
            auto p = std::make_unique<Array>();
            p->emplace_back(data_.number);
            // nothrow ->
            data_.array = p.release();
            type_ = Type::array;
        }
        break;
    case Type::string:
        {
            auto p = std::make_unique<Array>();
            p->emplace_back();
            // nothrow ->
            p->back().data_ = data_;
            p->back().type_ = type_;
            data_.array = p.release();
            type_ = Type::array;
        }
        break;
    case Type::array:
        break;
    case Type::object:
        {
            auto p = std::make_unique<Array>();
            p->emplace_back();
            // nothrow ->
            p->back().data_ = data_;
            p->back().type_ = type_;
            data_.array = p.release();
            type_ = Type::array;
        }
        break;
    default:
        JSON_UNREACHABLE();
        break;
    }

    return as_array();
}

Object& Value::inplace_convert_to_object()
{
    switch (type())
    {
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_.object = new Object();
        type_ = Type::object;
        break;
    case Type::string:
        {
            auto p = std::make_unique<Object>();
            // nothrow ->
            delete data_.string;
            data_.object = p.release();
            type_ = Type::object;
        }
        break;
    case Type::array:
        {
            auto p = std::make_unique<Object>();
            // nothrow ->
            delete data_.array;
            data_.object = p.release();
            type_ = Type::object;
        }
        break;
    case Type::object:
        break;
    default:
        JSON_UNREACHABLE();
        break;
    }

    return as_object();
}

Array& Value::embed_in_array()
{
    auto p = std::make_unique<Array>();
    p->emplace_back();

    // nothrow ->
    p->back().data_ = data_;
    p->back().type_ = type_;
    data_.array = p.release();
    type_ = Type::array;

    return as_array();
}

Object& Value::embed_in_object(String key)
{
    auto p = std::make_unique<Object>();
    (*p)[std::move(key)] = {};

    // nothrow ->
    p->begin()->second.data_ = data_;
    p->begin()->second.type_ = type_;
    data_.object = p.release();
    type_ = Type::object;

    return as_object();
}

bool Value::convert_to_boolean() const noexcept
{
    return ToBoolean(*this);
}

double Value::convert_to_number() const noexcept
{
    return ToNumber(*this);
}

String Value::convert_to_string() const
{
    return ToString(*this);
}

Array Value::convert_to_array() const
{
    switch (type())
    {
    case Type::null:
        return Array();
    case Type::boolean:
    case Type::number:
    case Type::string:
        return Array(1, *this); // ... an array filled with the current value
    case Type::array:
        return as_array();
    case Type::object:
        return Array(1, *this); // ... an array filled with the current value
    default:
        JSON_UNREACHABLE();
        break;
    }
}

Object Value::convert_to_object() const
{
    switch (type())
    {
    case Type::null:
    case Type::boolean:
    case Type::number:
    case Type::string:
    case Type::array:
        return Object();
    case Type::object:
        return as_object();
    default:
        JSON_UNREACHABLE();
        break;
    }
}

bool Value::equal_to(Value const& rhs) const noexcept
{
    if (this == &rhs)
        return true;

    if (type() != rhs.type())
        return false;

    switch (type())
    {
    case Type::null:
        return true;
    case Type::boolean:
        return as_boolean() == rhs.as_boolean();
    case Type::number:
        return as_number() == rhs.as_number();
    case Type::string:
        return as_string() == rhs.as_string();
    case Type::array:
        return as_array() == rhs.as_array();
    case Type::object:
        return as_object() == rhs.as_object();
    default:
        JSON_UNREACHABLE();
        break;
    }
}

bool Value::less_than(Value const& rhs) const noexcept
{
    if (this == &rhs)
        return false;

    if (type() < rhs.type())
        return true;
    if (type() > rhs.type())
        return false;

    switch (type())
    {
    case Type::null:
        return false;
    case Type::boolean:
        return as_boolean() < rhs.as_boolean();
    case Type::number:
        return as_number() < rhs.as_number();
    case Type::string:
        return as_string() < rhs.as_string();
    case Type::array:
        return as_array() < rhs.as_array();
    case Type::object:
        return as_object() < rhs.as_object();
    default:
        JSON_UNREACHABLE();
        break;
    }
}

static size_t HashCombine(size_t h1, size_t h2) noexcept
{
    h1 ^= h2 + 0x9E3779B9 + (h1 << 6) + (h1 >> 2);
//  h1 ^= h2 + 0x9E3779B97F4A7C15 + (h1 << 6) + (h1 >> 2);
    return h1;
}

size_t Value::hash() const noexcept
{
    switch (type())
    {
    case Type::null:
        return 0;
    case Type::boolean:
        return std::hash<bool>()(as_boolean());
    case Type::number:
        return std::hash<double>()(as_number());
    case Type::string:
        return std::hash<String>()(as_string());
    case Type::array:
        {
            size_t h = std::hash<char>()('['); // initial value for empty arrays
            for (auto const& v : as_array())
            {
                h = HashCombine(h, v.hash());
            }
            return h;
        }
    case Type::object:
        {
            size_t h = std::hash<char>()('{'); // initial value for empty objects
            for (auto const& v : as_object())
            {
                auto const h1 = std::hash<String>()(v.first);
                auto const h2 = v.second.hash();
                h ^= HashCombine(h1, h2); // Permutation resistant to support unordered maps.
            }
            return h;
        }
    default:
        JSON_UNREACHABLE();
        break;
    }
}

void Value::swap(Value& other) noexcept
{
    std::swap(data_, other.data_);
    std::swap(type_, other.type_);
}

size_t Value::size() const noexcept
{
    switch (type())
    {
    case Type::null:
    case Type::boolean:
    case Type::number:
        assert(false && "size() must not be called for null, boolean or number");
        return 0;
    case Type::string:
        return as_string().size();
    case Type::array:
        return as_array().size();
    case Type::object:
        return as_object().size();
    default:
        JSON_UNREACHABLE();
        break;
    }
}

bool Value::empty() const noexcept
{
    switch (type())
    {
    case Type::null:
    case Type::boolean:
    case Type::number:
        assert(false && "empty() must not be called for null, boolean or number");
        return true; // i.e. size() == 0
    case Type::string:
        return as_string().empty();
    case Type::array:
        return as_array().empty();
    case Type::object:
        return as_object().empty();
    default:
        JSON_UNREACHABLE();
        break;
    }
}

Value& Value::operator[](size_t index)
{
    assert(is_null() || is_array());
    auto& arr = inplace_convert_to_array();
    if (arr.size() <= index)
    {
        assert(index < SIZE_MAX);
        arr.resize(index + 1);
    }
    return arr[index];
}

Value const& Value::operator[](size_t index) const noexcept
{
    auto& arr = as_array();
    assert(index < arr.size());
    return arr[index];
}

Value* Value::get_ptr(size_t index)
{
    if (is_array())
    {
        auto& arr = as_array();
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
        auto& arr = as_array();
        if (index < arr.size()) {
            return &arr[index];
        }
    }
    return nullptr;
}

void Value::pop_back()
{
    as_array().pop_back();
}

void Value::erase(size_t index)
{
    auto& arr = as_array();
    if (index >= arr.size())
        return;
    arr.erase(arr.begin() + static_cast<intptr_t>(index));
}

Value& Value::operator[](Object::key_type const& key)
{
    assert(is_null() || is_object());
    return inplace_convert_to_object()[key];
}

Value& Value::operator[](Object::key_type&& key)
{
    assert(is_null() || is_object());
    return inplace_convert_to_object()[std::move(key)];
}

Value::item_iterator Value::erase(const_item_iterator pos)
{
    auto& obj = as_object();
    return obj.erase(pos);
}

template <typename ...Args>
String& Value::_assign_string(Args&&... args)
{
    switch (type_)
    {
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_.string = new String(std::forward<Args>(args)...);
        type_ = Type::string;
        break;
    case Type::string:
        *data_.string = String(std::forward<Args>(args)...);
        break;
    case Type::array:
        {
            auto p = std::make_unique<String>(std::forward<Args>(args)...);
            delete data_.array;
            data_.string = p.release();
            type_ = Type::string;
        }
        break;
    case Type::object:
        {
            auto p = std::make_unique<String>(std::forward<Args>(args)...);
            delete data_.object;
            data_.string = p.release();
            type_ = Type::string;
        }
        break;
    default:
        JSON_UNREACHABLE();
        break;
    }

    return as_string();
}

template <typename ...Args>
Array& Value::_assign_array(Args&&... args)
{
    switch (type_)
    {
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_.array = new Array(std::forward<Args>(args)...);
        type_ = Type::array;
        break;
    case Type::string:
        {
            auto p = std::make_unique<Array>(std::forward<Args>(args)...);
            delete data_.string;
            data_.array = p.release();
            type_ = Type::array;
        }
        break;
    case Type::array:
        *data_.array = Array(std::forward<Args>(args)...);
        break;
    case Type::object:
        {
            auto p = std::make_unique<Array>(std::forward<Args>(args)...);
            delete data_.object;
            data_.array = p.release();
            type_ = Type::array;
        }
        break;
    default:
        JSON_UNREACHABLE();
        break;
    }

    return as_array();
}

template <typename ...Args>
Object& Value::_assign_object(Args&&... args)
{
    switch (type_)
    {
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_.object = new Object(std::forward<Args>(args)...);
        type_ = Type::object;
        break;
    case Type::string:
        {
            auto p = std::make_unique<Object>(std::forward<Args>(args)...);
            delete data_.string;
            data_.object = p.release();
            type_ = Type::object;
        }
        break;
    case Type::array:
        {
            auto p = std::make_unique<Object>(std::forward<Args>(args)...);
            delete data_.array;
            data_.object = p.release();
            type_ = Type::object;
        }
        break;
    case Type::object:
        *data_.object = Object(std::forward<Args>(args)...);
        break;
    default:
        JSON_UNREACHABLE();
        break;
    }

    return as_object();
}

//==================================================================================================
// parse
//==================================================================================================

namespace
{
    struct Token
    {
        enum Kind : unsigned char {
            unknown,
            incomplete,
            eof,
            l_brace,
            r_brace,
            l_square,
            r_square,
            comma,
            colon,
            string,
            number,
            identifier,
            line_comment,
            block_comment,
            binary_data,
        };

        char const* ptr = nullptr;
        char const* end = nullptr;
        Kind        kind = Kind::unknown;
        bool        needs_cleaning = false;
    };
}

// 0x01: ' ', '\n', '\r', '\t'
// 0x02: '0'...'9'
// 0x04: 'a'...'z', 'A'...'Z'
// 0x08: IsDigit, IsLetter, '.', '+', '-', '#'
// 0x10: IsDigit, IsLetter, '_', '$'
// 0x20: IsDigit, 'a'...'f', 'A'...'F'
// 0x40: '"', '\\', '/', 'b', 'f', 'n', 'r', 't', 'u'

static constexpr uint8_t const kCharClass[256] = {
//      0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F
//      NUL     SOH     STX     ETX     EOT     ENQ     ACK     BEL     BS      HT      LF      VT      FF      CR      SO      SI
/*0*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0x01,   0x01,   0,      0,      0x01,   0,      0,
//      DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB     CAN     EM      SUB     ESC     FS      GS      RS      US
/*1*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
//      space   !       "       #       $       %       &       '       (       )       *       +       ,       -       .       /
/*2*/   0x01,   0,      0x40,   0x08,   0x10,   0,      0,      0,      0,      0,      0,      0x08,   0,      0x08,   0x08,   0x40,
//      0       1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
/*3*/   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0,      0,      0,      0,      0,      0,
//      @       A       B       C       D       E       F       G       H       I       J       K       L       M       N       O
/*4*/   0,      0x3C,   0x3C,   0x3C,   0x3C,   0x3C,   0x3C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,
//      P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
/*5*/   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0,      0x40,   0,      0,      0x10,
//      `       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
/*6*/   0,      0x3C,   0x7C,   0x3C,   0x3C,   0x3C,   0x7C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x5C,   0x1C,
//      p       q       r       s       t       u       v       w       x       y       z       {       |       }       ~       DEL
/*7*/   0x1C,   0x1C,   0x5C,   0x1C,   0x5C,   0x5C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0,      0,      0,      0,      0,
/*8*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*9*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*A*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*B*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*C*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*D*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*E*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*F*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
};

static constexpr bool IsWhitespace        (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & 0x01) != 0; } // ' ', '\n', '\r', '\t'
static constexpr bool IsDigit             (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & 0x02) != 0; } // '0'...'9'
static constexpr bool IsNumberBody        (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & 0x08) != 0; } // IsDigit, IsLetter, '.', '+', '-', '#'
static constexpr bool IsIdentifierBody    (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & 0x10) != 0; } // IsDigit, IsLetter, '_', '$'
// static constexpr bool IsValidEscapedChar  (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & 0x40) != 0; } // '"', '\\', '/', 'b', 'f', 'n', 'r', 't', 'u'

static char const* SkipWhitespace(char const* f, char const* l)
{
    for ( ; f != l && IsWhitespace(*f); ++f)
    {
    }

    return f;
}

namespace
{
    struct Lexer
    {
        char const* src = nullptr;
        char const* end = nullptr;
        char const* ptr = nullptr; // position in [src, end)

        Lexer();
        explicit Lexer(char const* first, char const* last);

        Token Lex(ParseOptions const& options);

        Token MakeToken(char const* p, Token::Kind kind, bool needs_cleaning = false);

        Token LexString(char const* p, char quote_char);
        Token LexNumber(char const* p);
        Token LexIdentifier(char const* p);
        Token LexComment(char const* p);
    };
}

Lexer::Lexer()
{
}

Lexer::Lexer(char const* first, char const* last)
    : src(first)
    , end(last)
    , ptr(first)
{
}

Token Lexer::Lex(ParseOptions const& options)
{
L_again:
    ptr = SkipWhitespace(ptr, end);

    char const* p = ptr;

    if (p == end)
        return MakeToken(p, Token::eof);

    Token::Kind kind = Token::unknown;

    switch (*p++)
    {
    case '{':
        kind = Token::l_brace;
        break;
    case '}':
        kind = Token::r_brace;
        break;
    case '[':
        kind = Token::l_square;
        break;
    case ']':
        kind = Token::r_square;
        break;
    case ',':
        kind = Token::comma;
        break;
    case ':':
        kind = Token::colon;
        break;
    case '\'':
        if (options.allow_single_quoted_strings)
            return LexString(p, '\'');
        break;
    case '"':
        return LexString(p, '"');
    case '+':
        if (options.allow_leading_plus)
            return LexNumber(p);
        break;
    case '.':
        if (options.allow_leading_dot)
            return LexNumber(p);
        break;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return LexNumber(p);
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '_':
        return LexIdentifier(p);
    case '/':
        if (options.allow_comments)
        {
            Token tok = LexComment(p);
            if (tok.kind == Token::unknown)
                return tok;
            goto L_again;
        }
        break;
    case '$':
    default:
        break;
    }

    return MakeToken(p, kind);
}

Token Lexer::MakeToken(char const* p, Token::Kind kind, bool needs_cleaning)
{
    Token tok;

    tok.ptr            = ptr;
    tok.end            = p;
    tok.kind           = kind;
    tok.needs_cleaning = needs_cleaning;

    ptr = p;

    return tok;
}

Token Lexer::LexString(char const* p, char quote_char)
{
    bool needs_cleaning = false;

    ptr = p; // skip " or '

    if (p == end)
        return MakeToken(p, Token::incomplete, needs_cleaning);

    unsigned char uch = static_cast<unsigned char>(*p++);
    for (;;)
    {
        if (uch == quote_char)
            break;

        if (uch < 0x20) // Unescaped control character
        {
            needs_cleaning = true;
        }
        else if (uch >= 0x80) // Possibly a UTF-8 lead byte (sequence length >= 2)
        {
            needs_cleaning = true;
        }
        else if (uch == '\\')
        {
            if (p != end) // Skip the escaped character.
                ++p;
            needs_cleaning = true;
        }

        if (p == end)
            return MakeToken(p, Token::incomplete, needs_cleaning);

        uch = static_cast<unsigned char>(*p++);
    }

    Token tok = MakeToken(p - 1, Token::string, needs_cleaning);

    ptr = p; // skip " or '

    return tok;
}

Token Lexer::LexNumber(char const* p)
{
    // Lex everything which might possibly occur in numbers.
    //
    // No need to lex "1.2e+3+4" as ["1.2e+3", "+", "4"] as the exact format
    // of the number will be checked later.

    for ( ; p != end && IsNumberBody(*p); ++p)
    {
    }

    return MakeToken(p, Token::number);
}

Token Lexer::LexIdentifier(char const* p)
{
    for ( ; p != end && IsIdentifierBody(*p); ++p)
    {
    }

    return MakeToken(p, Token::identifier);
}

Token Lexer::LexComment(char const* p)
{
    assert(p[-1] == '/');

    if (p == end)
        return MakeToken(p, Token::unknown);

    if (*p == '/')
    {
        ptr = ++p; // skip leading "//"
        for (;;)
        {
            if (p == end)
                break;
            if (*p == '\n' || *p == '\r')
                break;
            ++p;
        }

        return MakeToken(p, Token::line_comment);
    }

    if (*p == '*')
    {
        ptr = ++p; // skip leading "/*"
        for (;;)
        {
            if (p == end)
                return MakeToken(p, Token::unknown);
            if (*p == '*' && ++p != end && *p == '/')
                break;
            ++p;
        }

        Token tok = MakeToken(p - 1, Token::block_comment);
        ptr = ++p; // skip trailing "*/"
        return tok;
    }

    return MakeToken(p, Token::unknown);
}

//------------------------------------------------------------------------------
// Parser
//------------------------------------------------------------------------------

namespace
{
    struct Parser
    {
        static const int kMaxDepth = 500;

        Lexer lexer;
        Token token;

        ErrorCode ParseObject    (Value& value, int depth, ParseOptions const& options);
        ErrorCode ParsePair      (String& key, Value& value, int depth, ParseOptions const& options);
        ErrorCode ParseArray     (Value& value, int depth, ParseOptions const& options);
        ErrorCode ParseString    (Value& value, int depth, ParseOptions const& options);
        ErrorCode ParseNumber    (Value& value, int depth, ParseOptions const& options);
        ErrorCode ParseIdentifier(Value& value, int depth, ParseOptions const& options);
        ErrorCode ParseValue     (Value& value, int depth, ParseOptions const& options);

        static ErrorCode GetCleanString(String& out, char const*& first, char const* last, ParseOptions const& options);

        enum class NumberClass {
            invalid,
            neg_integer,
            pos_integer,
            floating_point,
            pos_nan,
            neg_nan,
            pos_inf,
            neg_inf,
        };

        NumberClass ClassifyNumber(char const* first, char const* last, ParseOptions const& options);
    };
}

//
//  object
//      {}
//      { members }
//  members
//      pair
//      pair , members
//
ErrorCode Parser::ParseObject(Value& value, int depth, ParseOptions const& options)
{
    assert(token.kind == Token::l_brace);

    if (depth >= kMaxDepth)
        return ErrorCode::max_depth_reached;

    token = lexer.Lex(options); // skip '{'

    auto& obj = value.inplace_convert_to_object();

    if (token.kind != Token::r_brace)
    {
        for (;;)
        {
            String K;
            Value V;

            ErrorCode ec = ParsePair(K, V, depth + 1, options);
            if (ec != ErrorCode::success)
                return ec;

            if (options.reject_duplicate_keys)
            {
#if __cplusplus >= 201703 || (_MSC_VER >= 1911 && _HAS_CXX17)
                auto const p = obj.try_emplace(std::move(K), std::move(V));
                if (!p.second)
                    return ErrorCode::duplicate_key_in_object;
#else
                auto const it = obj.find(K);
                if (it != obj.end())
                    return ErrorCode::duplicate_key_in_object;

                obj.emplace_hint(it, std::move(K), std::move(V));
#endif
            }
            else
            {
                obj[std::move(K)] = std::move(V);
            }

            if (token.kind != Token::comma)
                break;

            token = lexer.Lex(options); // skip ','

            if (options.allow_trailing_comma && token.kind == Token::r_brace)
                break;
        }

        if (token.kind != Token::r_brace)
            return ErrorCode::expected_comma_or_closing_brace;
    }

    token = lexer.Lex(options); // skip '}'

    return ErrorCode::success;
}

//
//  pair
//      string : value
//
ErrorCode Parser::ParsePair(String& key, Value& value, int depth, ParseOptions const& options)
{
    if (depth >= kMaxDepth)
        return ErrorCode::max_depth_reached;

    if (token.kind != Token::string && (!options.allow_unquoted_keys || token.kind != Token::identifier))
        return ErrorCode::expected_key;

    if (token.needs_cleaning)
    {
        ErrorCode ec = GetCleanString(key, token.ptr, token.end, options);
        if (ec != ErrorCode::success)
            return ec;
    }
    else
    {
        key.assign(token.ptr, token.end);
    }

    // skip 'key'
    token = lexer.Lex(options);

    if (token.kind != Token::colon)
        return ErrorCode::expected_colon_after_key;

    // skip ':'
    token = lexer.Lex(options);

    // Convert string to VALUE
    return ParseValue(value, depth, options);
}

//
//  array
//      []
//      [ elements ]
//  elements
//      value
//      value , elements
//
ErrorCode Parser::ParseArray(Value& value, int depth, ParseOptions const& options)
{
    assert(token.kind == Token::l_square);

    if (depth >= kMaxDepth)
        return ErrorCode::max_depth_reached;

    token = lexer.Lex(options); // skip '['

    auto& arr = value.inplace_convert_to_array();

    if (token.kind != Token::r_square)
    {
        for (;;)
        {
            Value V;

            ErrorCode ec = ParseValue(V, depth + 1, options);
            if (ec != ErrorCode::success)
                return ec;

            arr.push_back(std::move(V));

            if (token.kind != Token::comma)
                break;

            // skip ','
            token = lexer.Lex(options);

            if (options.allow_trailing_comma && token.kind == Token::r_square)
                break;
        }

        if (token.kind != Token::r_square)
            return ErrorCode::expected_comma_or_closing_bracket;
    }

    token = lexer.Lex(options); // skip ']'

    return ErrorCode::success;
}

ErrorCode Parser::ParseString(Value& value, int /*depth*/, ParseOptions const& options)
{
    if (token.needs_cleaning)
    {
        String out;

        ErrorCode ec = GetCleanString(out, token.ptr, token.end, options);
        if (ec != ErrorCode::success)
            return ec;

        value.assign_string(std::move(out));
    }
    else
    {
        value.assign_string(String(token.ptr, token.end));
    }

    token = lexer.Lex(options); // skip string

    return ErrorCode::success;
}

static bool IsNaNString(char const* f, char const* l)
{
    auto const len = l - f;
    switch (len)
    {
    case 3:
        return memcmp(f, "NaN", 3) == 0
            || memcmp(f, "nan", 3) == 0
            || memcmp(f, "NAN", 3) == 0;
    case 6:
        return memcmp(f, "1.#IND", 6) == 0;
    case 7:
        return memcmp(f, "1.#QNAN", 7) == 0
            || memcmp(f, "1.#SNAN", 7) == 0;
    default:
        return false;
    }
}

static bool IsInfinityString(char const* f, char const* l)
{
    auto const len = l - f;
    switch (len)
    {
    case 3:
        return memcmp(f, "Inf", 3) == 0
            || memcmp(f, "inf", 3) == 0
            || memcmp(f, "INF", 3) == 0;
    case 6:
        return memcmp(f, "1.#INF", 6) == 0;
    case 8:
        return memcmp(f, "Infinity", 8) == 0
            || memcmp(f, "infinity", 8) == 0
            || memcmp(f, "INFINITY", 8) == 0;
    default:
        return false;
    }
}

//
//  number
//     int
//     int frac
//     int exp
//     int frac exp
//  int
//     digit
//     digit1-9 digits
//     - digit
//     - digit1-9 digits
//  frac
//     . digits
//  exp
//     e digits
//  digits
//     digit
//     digit digits
//  e
//     e
//     e+
//     e-
//     E
//     E+
//     E-
//
Parser::NumberClass Parser::ClassifyNumber(char const* f, char const* l, ParseOptions const& options)
{
    if (f == l)
        return NumberClass::invalid;

// [-]

    bool is_neg = false;

    if (*f == '-')
    {
        is_neg = true;
        ++f;
        if (f == l)
            return NumberClass::invalid;
    }
    else if (options.allow_leading_plus && *f == '+')
    {
        ++f;
        if (f == l)
            return NumberClass::invalid;
    }

// NaN/Infinity

    if (options.allow_nan_inf)
    {
        if (IsNaNString(f, l))
            return is_neg ? NumberClass::neg_nan : NumberClass::pos_nan;

        if (IsInfinityString(f, l))
            return is_neg ? NumberClass::neg_inf : NumberClass::pos_inf;
    }

// int

    if (*f == '0')
    {
        ++f;
        if (f == l)
            return is_neg ? NumberClass::neg_integer : NumberClass::pos_integer;
        if (IsDigit(*f))
            return NumberClass::invalid;
    }
    else if (IsDigit(*f)) // non '0'
    {
        for (;;)
        {
            ++f;
            if (f == l)
                return is_neg ? NumberClass::neg_integer : NumberClass::pos_integer;
            if (!IsDigit(*f))
                break;
        }
    }
    else if (options.allow_leading_dot && *f == '.')
    {
        // Parsed again below.
    }
    else
    {
        return NumberClass::invalid;
    }

// frac

    bool is_float = false;

    if (*f == '.')
    {
        is_float = true;
        ++f;
        if (f == l)
            return NumberClass::invalid;

        if (!IsDigit(*f))
            return NumberClass::invalid;

        for (;;)
        {
            ++f;
            if (f == l)
                return NumberClass::floating_point;
            if (!IsDigit(*f))
                break;
        }
    }

// exp

    if (*f == 'e' || *f == 'E')
    {
        is_float = true;
        ++f;
        if (f == l)
            return NumberClass::invalid;

        if (*f == '+' || *f == '-')
        {
            ++f;
            if (f == l)
                return NumberClass::invalid;
        }

        if (!IsDigit(*f))
            return NumberClass::invalid;

        for (;;)
        {
            ++f;
            if (f == l)
                return NumberClass::floating_point;
            if (!IsDigit(*f))
                break;
        }
    }

    if (f != l)
        return NumberClass::invalid;

    if (is_float)
        return NumberClass::floating_point;

    return is_neg ? NumberClass::neg_integer : NumberClass::pos_integer;
}

static int HexDigitValue(char ch)
{
    if ('0' <= ch && ch <= '9') return ch - '0';
    if ('A' <= ch && ch <= 'F') return ch - 'A' + 10;
    if ('a' <= ch && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

// 2^53 = 9007199254740992 is the largest integer which can be represented
// without loss of precision in an IEEE double. That's 16 digits, so an integer
// with at most 15 digits always can be converted to double without loss of
// precision.
static constexpr int const kIntDigits10 = 15;

static int64_t ParseInteger(char const* f, char const* l)
{
    assert(l - f > 0); // internal error
    assert(l - f <= kIntDigits10); // internal error

    int64_t val = 0;
    for ( ; f != l; ++f)
    {
        assert(*f >= '0'); // internal error
        assert(*f <= '9'); // internal error
        val = val * 10 + (*f - '0');
    }

    return val;
}

ErrorCode Parser::ParseNumber(Value& value, int /*depth*/, ParseOptions const& options)
{
    // Validate number even if parsing numbers as strings!
    NumberClass const nc = ClassifyNumber(token.ptr, token.end, options);

    if (nc == NumberClass::invalid)
        return ErrorCode::invalid_numeric_literal;

    if (options.parse_numbers_as_strings)
    {
        value.assign_string(String(token.ptr, token.end));

        token = lexer.Lex(options); // skip number

        return ErrorCode::success;
    }

    double num = 0.0;

    // Use a _slightly_ faster method for parsing integers which will fit into a
    // double-precision number without loss of precision. Larger numbers will be
    // handled by strtod.
    bool const is_pos_int = (nc == NumberClass::pos_integer) && (token.end - token.ptr) <= kIntDigits10;
    bool const is_neg_int = (nc == NumberClass::neg_integer) && (token.end - token.ptr) <= kIntDigits10 + 1/*minus sign*/;

    if (is_pos_int)
    {
        num = static_cast<double>(ParseInteger(token.ptr, token.end));
    }
    else if (is_neg_int)
    {
        assert(token.end - token.ptr >= 1); // internal error
        assert(token.ptr[0] == '-'); // internal error

        // NB:
        // Works for signed zeros.
        num = -static_cast<double>(ParseInteger(token.ptr + 1, token.end));
    }
    else if (nc == NumberClass::neg_nan)
    {
        num = -std::numeric_limits<double>::quiet_NaN();
    }
    else if (nc == NumberClass::pos_nan)
    {
        num = +std::numeric_limits<double>::quiet_NaN();
    }
    else if (nc == NumberClass::neg_inf)
    {
        num = -std::numeric_limits<double>::infinity();
    }
    else if (nc == NumberClass::pos_inf)
    {
        num = +std::numeric_limits<double>::infinity();
    }
    else
    {
        num = Strtod(token.ptr, static_cast<int>(token.end - token.ptr));
    }

    value.assign_number(num);

    token = lexer.Lex(options); // skip number

    return ErrorCode::success;
}

ErrorCode Parser::ParseIdentifier(Value& value, int /*depth*/, ParseOptions const& options)
{
    assert(token.end - token.ptr > 0 && "internal error");

    auto const f = token.ptr;
    auto const l = token.end;
    auto const len = l - f;

    if (len == 4 && memcmp(f, "null", 4) == 0)
    {
        value.assign_null();
    }
    else if (len == 4 && memcmp(f, "true", 4) == 0)
    {
        value.assign_boolean(true);
    }
    else if (len == 5 && memcmp(f, "false", 5) == 0)
    {
        value.assign_boolean(false);
    }
    else if (options.allow_nan_inf && IsNaNString(f, l))
    {
        value.assign_number(std::numeric_limits<double>::quiet_NaN());
    }
    else if (options.allow_nan_inf && IsInfinityString(f, l))
    {
        value.assign_number(std::numeric_limits<double>::infinity());
    }
    else
    {
        return ErrorCode::unrecognized_identifier;
    }

    token = lexer.Lex(options); // skip 'identifier'

    return ErrorCode::success;
}

//
//  value
//      string
//      number
//      object
//      array
//      true
//      false
//      null
//
ErrorCode Parser::ParseValue(Value& value, int depth, ParseOptions const& options)
{
    if (depth >= kMaxDepth)
        return ErrorCode::max_depth_reached;

    switch (token.kind)
    {
    case Token::l_brace:
        return ParseObject(value, depth, options);
    case Token::l_square:
        return ParseArray(value, depth, options);
    case Token::string:
        return ParseString(value, depth, options);
    case Token::number:
        return ParseNumber(value, depth, options);
    case Token::identifier:
        return ParseIdentifier(value, depth, options);
    default:
        return ErrorCode::unexpected_token;
    }
}

//
//  string
//      ""
//      " chars "
//  chars
//      char
//      char chars
//  char
//      <any Unicode character except " or \ or control-character>
//      '\"'
//      '\\'
//      '\/'
//      '\b'
//      '\f'
//      '\n'
//      '\r'
//      '\t'
//      '\u' four-hex-digits
//
ErrorCode Parser::GetCleanString(String& out, char const*& first, char const* last, ParseOptions const& options)
{
    out.clear();
    out.reserve(static_cast<size_t>(last - first));

    char const* f = first;
    while (f != last)
    {
        auto const uch = static_cast<unsigned char>(*f);

        if (uch < 0x20) // unescaped control character
        {
            first = f;
            return ErrorCode::unescaped_control_character_in_string;
        }
        else if (uch < 0x80) // ASCII printable or DEL
        {
            if (*f != '\\')
            {
                out += *f;
                ++f;
                continue;
            }

            ++f; // skip '\'
            if (f == last)
            {
                first = f;
                return ErrorCode::unexpected_end_of_string;
            }

            switch (*f)
            {
            case '"':
                out += '"';
                ++f;
                break;
            case '\\':
                out += '\\';
                ++f;
                break;
            case '/':
                out += '/';
                ++f;
                break;
            case 'b':
                out += '\b';
                ++f;
                break;
            case 'f':
                out += '\f';
                ++f;
                break;
            case 'n':
                out += '\n';
                ++f;
                break;
            case 'r':
                out += '\r';
                ++f;
                break;
            case 't':
                out += '\t';
                ++f;
                break;
            case 'u':
                {
                    --f; // Put back '\'

                    auto const res = unicode::DecodeUTF16Sequence([&](uint16_t& W)
                    {
                        if (last - f < 6)
                            return false;

                        if (*f++ != '\\' || *f++ != 'u')
                            return false;

                        int const h0 = HexDigitValue(*f++);
                        int const h1 = HexDigitValue(*f++);
                        int const h2 = HexDigitValue(*f++);
                        int const h3 = HexDigitValue(*f++);

                        int const value = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
                        if (value >= 0)
                        {
                            assert(value <= 0xFFFF);
                            W = static_cast<uint16_t>(value);
                            return true;
                        }

                        return false;
                    });

                    switch (res.ec)
                    {
                    case unicode::ErrorCode::success:
                        unicode::EncodeUTF8(res.U, [&](char ch) { out += ch; return true; });
                        break;

                    case unicode::ErrorCode::insufficient_data:
                        // This is either an "end of file" or an "invalid UCN".
                        // In both cases this is not strictly an "invalid unicode" error.
                        first = f;
                        return ErrorCode::invalid_unicode_sequence_in_string;

                    default:
                        if (!options.allow_invalid_unicode)
                        {
                            first = f;
                            return ErrorCode::invalid_unicode_sequence_in_string;
                        }

//                      unicode::EncodeUTF8(unicode::kReplacementCharacter, [&](char ch) { out += ch; return true; });
                        out += "\xEF\xBF\xBD";
                        break;
                    }
                }
                break;
            default:
                first = f;
                return ErrorCode::invalid_escaped_character_in_string;
            }
        }
        else // (possibly) the start of a UTF-8 sequence.
        {
            auto f0 = f;
            auto const res = unicode::DecodeUTF8Sequence(f, last);

            switch (res.ec)
            {
            case unicode::ErrorCode::success:
                // The range [f0, f) already contains a valid UTF-8 encoding of U.
                switch (f - f0) {
                case 4: out += *f0++; // fall through
                case 3: out += *f0++; // fall through
                case 2: out += *f0++; // fall through
                        out += *f0++;
                    break;
                default:
                    assert(false && "internal error");
                    break;
                }
                break;

            case unicode::ErrorCode::insufficient_data:
                first = f;
                return ErrorCode::invalid_unicode_sequence_in_string;

            default:
                if (!options.allow_invalid_unicode)
                {
                    first = f;
                    return ErrorCode::invalid_unicode_sequence_in_string;
                }

//              unicode::EncodeUTF8(unicode::kReplacementCharacter, [&](char ch) { out += ch; return true; });
                out += "\xEF\xBF\xBD";

                // Skip to the start of the next sequence (if not already done yet)
                f = unicode::FindNextUTF8LeadByte(f, last);
                break;
            }
        }
    }

    first = f;
    return ErrorCode::success;
}

ParseResult json::parse(Value& value, char const* next, char const* last, ParseOptions const& options)
{
    value.assign_null(); // clear!

    if (options.skip_bom && last - next >= 3)
    {
        if (static_cast<unsigned char>(next[0]) == 0xEF &&
            static_cast<unsigned char>(next[1]) == 0xBB &&
            static_cast<unsigned char>(next[2]) == 0xBF)
        {
            next += 3;
        }
    }

    Parser parser;

    parser.lexer = Lexer(next, last);
    parser.token = parser.lexer.Lex(options);

    ErrorCode ec = parser.ParseValue(value, 0, options);

    if (ec == ErrorCode::success)
    {
        if (!options.allow_trailing_characters && parser.token.kind != Token::eof)
        {
            ec = ErrorCode::expected_eof;
        }
    }

    return {ec, parser.token.ptr, parser.token.end};
}

ErrorCode json::parse(Value& value, std::string const& str, ParseOptions const& options)
{
    char const* next = str.data();
    char const* last = str.data() + str.size();

    return ::json::parse(value, next, last, options).ec;
}

//==================================================================================================
// stringify
//==================================================================================================

static bool StringifyValue(std::string& str, Value const& value, StringifyOptions const& options, int curr_indent);

static bool StringifyNull(std::string& str)
{
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
    if (!std::isfinite(value))
    {
        if (!options.allow_nan_inf)
        {
            str += "null";
            return true;
        }

        if (std::isnan(value))
        {
            str += kNaNString;
            return true;
        }

        if (value < 0)
            str += '-';
        str += kInfString;
        return true;
    }

    char buf[32];

    // Handle +-0.
    // Interpret -0 as a floating-point number and +0 as an integer.
    if (value == 0.0)
    {
        if (std::signbit(value))
            str += "-0.0";
        else
            str += '0';
        return true;
    }

    // The test for |value| <= 2^53 is not strictly required, but ensures that a number
    // is only printed as an integer if it is exactly representable as double.
    if (value >= -9007199254740992.0 && value <= 9007199254740992.0)
    {
        const int64_t i = static_cast<int64_t>(value);
        if (static_cast<double>(i) == value)
        {
            char* end = iconv::s64toa(buf, i);
            str.append(buf, end);
            return true;
        }
    }

    char* end = dconv::ToShort(buf, buf + 32, value);
    str.append(buf, end);
    return true;
}

static bool StringifyString(std::string& str, String const& value, StringifyOptions const& options)
{
    static constexpr char const kHexDigits[] = "0123456789ABCDEF";

    char const*       first = value.data();
    char const* const last  = value.data() + value.size();

    str += '"';

    char ch_prev = '\0';
    char ch = '\0';

    while (first != last)
    {
        ch_prev = ch;
        ch = *first;

        auto const uch = static_cast<unsigned char>(ch);

        if (uch < 0x20) // (ASCII) control character
        {
            switch (ch)
            {
            case '\b':
                str += '\\';
                str += 'b';
                break;
            case '\f':
                str += '\\';
                str += 'f';
                break;
            case '\n':
                str += '\\';
                str += 'n';
                break;
            case '\r':
                str += '\\';
                str += 'r';
                break;
            case '\t':
                str += '\\';
                str += 't';
                break;
            default:
                str += "\\u00";
                str += kHexDigits[uch >> 4];
                str += kHexDigits[uch & 0xF];
                break;
            }
            ++first;
        }
        else if (uch < 0x80) // ASCII printable or DEL
        {
            switch (ch)
            {
            case '"':   // U+0022
                str += '\\';
                break;
            case '/':   // U+002F
                if (options.escape_slash && ch_prev == '<') {
                    str += '\\';
                }
                break;
            case '\\':  // U+005C
                str += '\\';
                break;
            }
            str += ch;
            ++first;
        }
        else // (possibly) the start of a UTF-8 sequence.
        {
            auto f0 = first;
            auto const res = unicode::DecodeUTF8Sequence(first, last);

            switch (res.ec)
            {
            case unicode::ErrorCode::success:
                //
                // Always escape U+2028 (LINE SEPARATOR) and U+2029 (PARAGRAPH
                // SEPARATOR). No string in JavaScript can contain a literal
                // U+2028 or a U+2029.
                //
                // (See http://timelessrepo.com/json-isnt-a-javascript-subset)
                //
                if (res.U == 0x2028)
                {
                    str += "\\u2028";
                }
                else if (res.U == 0x2029)
                {
                    str += "\\u2029";
                }
                else
                {
                    // The UTF-8 sequence is valid. No need to re-encode.
                    switch (first - f0) {
                    case 4: str += *f0++; // fall through
                    case 3: str += *f0++; // fall through
                    case 2: str += *f0++; // fall through
                            str += *f0++;
                        break;
                    default:
                        assert(false && "internal error");
                        break;
                    }
                }
                break;

            default:
                if (!options.allow_invalid_unicode)
                {
                    // TODO:
                    // Caller should be notified of the position of the invalid sequence
                    return false;
                }

                str += "\\uFFFD";

                // Scan to the start of the next UTF-8 sequence.
                // This includes ASCII characters.
                first = unicode::FindNextUTF8LeadByte(first, last);
                break;
            }
        }
    }

    str += '"';

    return true;
}

static bool StringifyArray(std::string& str, Array const& value, StringifyOptions const& options, int curr_indent)
{
    str += '[';

    auto       I = value.begin();
    auto const E = value.end();
    if (I != E)
    {
        if (options.indent_width >= 0)
        {
            assert(curr_indent <= INT_MAX - options.indent_width);
            curr_indent += options.indent_width;

            for (;;)
            {
                str += '\n';
                str.append(static_cast<size_t>(curr_indent), ' ');

                if (!StringifyValue(str, *I, options, curr_indent))
                    return false;

                if (++I == E)
                    break;

                str += ',';
            }

            curr_indent -= options.indent_width;

            str += '\n';
            str.append(static_cast<size_t>(curr_indent), ' ');
        }
        else
        {
            for (;;)
            {
                if (!StringifyValue(str, *I, options, curr_indent))
                    return false;

                if (++I == E)
                    break;

                str += ',';
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
        if (options.indent_width >= 0)
        {
            assert(curr_indent <= INT_MAX - options.indent_width);
            curr_indent += options.indent_width;

            for (;;)
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

            curr_indent -= options.indent_width;

            str += '\n';
            str.append(static_cast<size_t>(curr_indent), ' ');
        }
        else
        {
            for (;;)
            {
                if (!StringifyString(str, I->first, options))
                    return false;
                str += ':';
                if (!StringifyValue(str, I->second, options, curr_indent))
                    return false;

                if (++I == E)
                    break;

                str += ',';
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
    case Type::null:
        return StringifyNull(str);
    case Type::boolean:
        return StringifyBoolean(str, value.as_boolean());
    case Type::number:
        return StringifyNumber(str, value.as_number(), options);
    case Type::string:
        return StringifyString(str, value.as_string(), options);
    case Type::array:
        return StringifyArray(str, value.as_array(), options, curr_indent);
    case Type::object:
        return StringifyObject(str, value.as_object(), options, curr_indent);
    default:
        JSON_UNREACHABLE();
        break;
    }
}

bool json::stringify(std::string& str, Value const& value, StringifyOptions const& options)
{
    return StringifyValue(str, value, options, 0);
}

/*
Copyright 2017 Alexander Bolz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
