// From:
// https://github.com/nst/JSONTestSuite
//
// MIT License
//
// Copyright (c) 2016 Nicolas Seriot
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "catch.hpp"
#include "../src/json.h"

inline char const* TokenKindDescr(json::TokenKind kind)
{
    switch (kind) {
    case json::TokenKind::unknown:
        assert(false && "unreachable");
        return "unknown";
    case json::TokenKind::invalid_character:
        return "invalid_character";
    case json::TokenKind::eof:
        return "eof";
    case json::TokenKind::l_brace:
        return "l_brace";
    case json::TokenKind::r_brace:
        return "r_brace";
    case json::TokenKind::l_square:
        return "l_square";
    case json::TokenKind::r_square:
        return "r_square";
    case json::TokenKind::comma:
        return "comma";
    case json::TokenKind::colon:
        return "colon";
    case json::TokenKind::string:
        return "string";
    case json::TokenKind::number:
        return "number";
    case json::TokenKind::identifier:
        return "identifier";
    case json::TokenKind::comment:
        return "comment";
    case json::TokenKind::incomplete_string:
        return "incomplete_string";
    case json::TokenKind::incomplete_comment:
        return "incomplete_comment";
    }

    assert(false && "unreachable");
    return "<unknown>";
}

inline char const* ParseStatusDescr(json::ParseStatus ec)
{
    switch (ec) {
    case json::ParseStatus::success:
        return "success";
    case json::ParseStatus::duplicate_key:
        return "duplicate_key";
    case json::ParseStatus::expected_colon_after_key:
        return "expected ':' after key";
    case json::ParseStatus::expected_comma_or_closing_brace:
        return "expected ',' or '}'";
    case json::ParseStatus::expected_comma_or_closing_bracket:
        return "expected ',' or ']'";
    case json::ParseStatus::expected_eof:
        return "expected EOF";
    case json::ParseStatus::expected_key:
        return "expected key";
    case json::ParseStatus::expected_value:
        return "expected value";
    case json::ParseStatus::invalid_key:
        return "invalid key";
    case json::ParseStatus::invalid_number:
        return "invalid number";
    case json::ParseStatus::invalid_string:
        return "invalid string";
    case json::ParseStatus::invalid_value:
        return "invalid value";
    case json::ParseStatus::max_depth_reached:
        return "max. depth reached";
    case json::ParseStatus::unexpected_eof:
        return "unexpected EOF";
    case json::ParseStatus::unknown:
        assert(false && "unreachable");
        return "unknown";
    case json::ParseStatus::unrecognized_identifier:
        return "unrecognized identifier";
    }

    assert(false && "unreachable");
    return "<unknown>";
}

inline std::string ToPrintableString(char const* next, char const* last)
{
    std::string str;

    for ( ; next != last; ++next)
    {
        uint8_t const b = static_cast<uint8_t>(*next);
        if (b >= 0x20 && b <= 0x7E)
        {
            str.push_back(*next);
        }
        else
        {
            str.push_back('<');
            str.push_back('x');
            str.push_back("0123456789ABCDEF"[(b >> 4) & 0xF]);
            str.push_back("0123456789ABCDEF"[(b >> 0) & 0xF]);
            str.push_back('>');
        }
    }

    return str;
}

struct JSONTestSuiteTest {
    std::string input;
    std::string name;
};

static const JSONTestSuiteTest kJSONTestSuite_y[] = {
    {std::string("[[]   ]", 7), "y_array_arraysWithSpaces"},
    {std::string("[\"\"]", 4), "y_array_empty-string"},
    {std::string("[]", 2), "y_array_empty"},
    {std::string("[\"a\"]", 5), "y_array_ending_with_newline"},
    {std::string("[false]", 7), "y_array_false"},
    {std::string("[null, 1, \"1\", {}]", 18), "y_array_heterogeneous"},
    {std::string("[null]", 6), "y_array_null"},
    {std::string("[1\n]", 4), "y_array_with_1_and_newline"},
    {std::string(" [1]", 4), "y_array_with_leading_space"},
    {std::string("[1,null,null,null,2]", 20), "y_array_with_several_null"},
    {std::string("[2] ", 4), "y_array_with_trailing_space"},
    {std::string("[123e65]", 8), "y_number"},
    {std::string("[0e+1]", 6), "y_number_0e+1"},
    {std::string("[0e1]", 5), "y_number_0e1"},
    {std::string("[ 4]", 4), "y_number_after_space"},
    {std::string("[-0.000000000000000000000000000000000000000000000000000000000000000000000000000001]\n", 84), "y_number_double_close_to_zero"},
    {std::string("[20e1]", 6), "y_number_int_with_exp"},
    {std::string("[-0]", 4), "y_number_minus_zero"},
    {std::string("[-123]", 6), "y_number_negative_int"},
    {std::string("[-1]", 4), "y_number_negative_one"},
    {std::string("[-0]", 4), "y_number_negative_zero"},
    {std::string("[1E22]", 6), "y_number_real_capital_e"},
    {std::string("[1E-2]", 6), "y_number_real_capital_e_neg_exp"},
    {std::string("[1E+2]", 6), "y_number_real_capital_e_pos_exp"},
    {std::string("[123e45]", 8), "y_number_real_exponent"},
    {std::string("[123.456e78]", 12), "y_number_real_fraction_exponent"},
    {std::string("[1e-2]", 6), "y_number_real_neg_exp"},
    {std::string("[1e+2]", 6), "y_number_real_pos_exponent"},
    {std::string("[123]", 5), "y_number_simple_int"},
    {std::string("[123.456789]", 12), "y_number_simple_real"},
    {std::string("{\"asd\":\"sdf\", \"dfg\":\"fgh\"}", 26), "y_object"},
    {std::string("{\"asd\":\"sdf\"}", 13), "y_object_basic"},
    {std::string("{\"a\":\"b\",\"a\":\"c\"}", 17), "y_object_duplicated_key"},
    {std::string("{\"a\":\"b\",\"a\":\"b\"}", 17), "y_object_duplicated_key_and_value"},
    {std::string("{}", 2), "y_object_empty"},
    {std::string("{\"\":0}", 6), "y_object_empty_key"},
    {std::string("{\"foo\\u0000bar\": 42}", 20), "y_object_escaped_null_in_key"},
    {std::string("{ \"min\": -1.0e+28, \"max\": 1.0e+28 }", 35), "y_object_extreme_numbers"},
    {std::string("{\"x\":[{\"id\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}], \"id\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}", 108), "y_object_long_strings"},
    {std::string("{\"a\":[]}", 8), "y_object_simple"},
    {std::string("{\"title\":\"\\u041f\\u043e\\u043b\\u0442\\u043e\\u0440\\u0430 \\u0417\\u0435\\u043c\\u043b\\u0435\\u043a\\u043e\\u043f\\u0430\" }", 110), "y_object_string_unicode"},
    {std::string("{\n\"a\": \"b\"\n}", 12), "y_object_with_newlines"},
    {std::string("[\"\\u0060\\u012a\\u12AB\"]", 22), "y_string_1_2_3_bytes_UTF-8_sequences"},
    {std::string("[\"\\uD801\\udc37\"]", 16), "y_string_accepted_surrogate_pair"},
    {std::string("[\"\\ud83d\\ude39\\ud83d\\udc8d\"]", 28), "y_string_accepted_surrogate_pairs"},
    {std::string("[\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"]", 20), "y_string_allowed_escapes"},
    {std::string("[\"\\\\u0000\"]", 11), "y_string_backslash_and_u_escaped_zero"},
    {std::string("[\"\\\"\"]", 6), "y_string_backslash_doublequotes"},
    {std::string("[\"a/*b*/c/*d//e\"]", 17), "y_string_comments"},
    {std::string("[\"\\\\a\"]", 7), "y_string_double_escape_a"},
    {std::string("[\"\\\\n\"]", 7), "y_string_double_escape_n"},
    {std::string("[\"\\u0012\"]", 10), "y_string_escaped_control_character"},
    {std::string("[\"\\uFFFF\"]", 10), "y_string_escaped_noncharacter"},
    {std::string("[\"asd\"]", 7), "y_string_in_array"},
    {std::string("[ \"asd\"]", 8), "y_string_in_array_with_leading_space"},
    {std::string("[\"\\uDBFF\\uDFFF\"]", 16), "y_string_last_surrogates_1_and_2"},
    {std::string("[\"new\\u00A0line\"]", 17), "y_string_nbsp_uescaped"},
    {std::string("[\"\364\217\277\277\"]", 8), "y_string_nonCharacterInUTF-8_U+10FFFF"},
    {std::string("[\"\360\233\277\277\"]", 8), "y_string_nonCharacterInUTF-8_U+1FFFF"},
    {std::string("[\"\357\277\277\"]", 7), "y_string_nonCharacterInUTF-8_U+FFFF"},
    {std::string("[\"\\u0000\"]", 10), "y_string_null_escape"},
    {std::string("[\"\\u002c\"]", 10), "y_string_one-byte-utf-8"},
    {std::string("[\"\317\200\"]", 6), "y_string_pi"},
    {std::string("[\"\360\233\277\277\"]", 8), "y_string_reservedCharacterInUTF-8_U+1BFFF"},
    {std::string("[\"asd \"]", 8), "y_string_simple_ascii"},
    {std::string("\" \"", 3), "y_string_space"},
    {std::string("[\"\\uD834\\uDd1e\"]", 16), "y_string_surrogates_U+1D11E_MUSICAL_SYMBOL_G_CLEF"},
    {std::string("[\"\\u0821\"]", 10), "y_string_three-byte-utf-8"},
    {std::string("[\"\\u0123\"]", 10), "y_string_two-byte-utf-8"},
    {std::string("[\"\342\200\250\"]", 7), "y_string_u+2028_line_sep"},
    {std::string("[\"\342\200\251\"]", 7), "y_string_u+2029_par_sep"},
    {std::string("[\"\\u0061\\u30af\\u30EA\\u30b9\"]", 28), "y_string_uEscape"},
    {std::string("[\"new\\u000Aline\"]", 17), "y_string_uescaped_newline"},
    {std::string("[\"\177\"]", 5), "y_string_unescaped_char_delete"},
    {std::string("[\"\\uA66D\"]", 10), "y_string_unicode"},
    {std::string("[\"\\u005C\"]", 10), "y_string_unicodeEscapedBackslash"},
    {std::string("[\"\342\215\202\343\210\264\342\215\202\"]", 13), "y_string_unicode_2"},
    {std::string("[\"\\u0022\"]", 10), "y_string_unicode_escaped_double_quote"},
    {std::string("[\"\\uDBFF\\uDFFE\"]", 16), "y_string_unicode_U+10FFFE_nonchar"},
    {std::string("[\"\\uD83F\\uDFFE\"]", 16), "y_string_unicode_U+1FFFE_nonchar"},
    {std::string("[\"\\u200B\"]", 10), "y_string_unicode_U+200B_ZERO_WIDTH_SPACE"},
    {std::string("[\"\\u2064\"]", 10), "y_string_unicode_U+2064_invisible_plus"},
    {std::string("[\"\\uFDD0\"]", 10), "y_string_unicode_U+FDD0_nonchar"},
    {std::string("[\"\\uFFFE\"]", 10), "y_string_unicode_U+FFFE_nonchar"},
    {std::string("[\"\342\202\254\360\235\204\236\"]", 11), "y_string_utf8"},
    {std::string("[\"a\177a\"]", 7), "y_string_with_del_character"},
    {std::string("false", 5), "y_structure_lonely_false"},
    {std::string("42", 2), "y_structure_lonely_int"},
    {std::string("-0.1", 4), "y_structure_lonely_negative_real"},
    {std::string("null", 4), "y_structure_lonely_null"},
    {std::string("\"asd\"", 5), "y_structure_lonely_string"},
    {std::string("true", 4), "y_structure_lonely_true"},
    {std::string("\"\"", 2), "y_structure_string_empty"},
    {std::string("[\"a\"]\n", 6), "y_structure_trailing_newline"},
    {std::string("[true]", 6), "y_structure_true_in_array"},
    {std::string(" [] ", 4), "y_structure_whitespace_array"},

    // From the implementation-defined tests:
    {std::string("[123.456e-789]", 14), "i_number_double_huge_neg_exp"},
    {std::string("[0.4e00669999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999969999999006]", 137), "i_number_huge_exp"},
    {std::string("[-1e+9999]", 10), "i_number_neg_int_huge_exp"},
    {std::string("[1.5e+9999]", 11), "i_number_pos_double_huge_exp"},
    {std::string("[-123123e100000]", 16), "i_number_real_neg_overflow"},
    {std::string("[123123e100000]", 15), "i_number_real_pos_overflow"},
    {std::string("[123e-10000000]", 15), "i_number_real_underflow"},
    {std::string("[-123123123123123123123123123123]", 33), "i_number_too_big_neg_int"},
    {std::string("[100000000000000000000]", 23), "i_number_too_big_pos_int"},
    {std::string("[-237462374673276894279832749832423479823246327846]", 51), "i_number_very_big_negative_int"},

    // From the implementation-defined tests:
#if 1
    {std::string("[-NaN]", 6), "i_n_number_-NaN"},
    //{std::string("[Inf]", 5), "i_n_number_Inf"},
    {std::string("[Infinity]", 10), "i_n_number_infinity"},
    {std::string("[-Infinity]", 11), "i_n_number_minus_infinity"},
    {std::string("[NaN]", 5), "i_n_number_NaN"},
#endif

    // From the implementation-defined tests:
    {std::string("\357\273\277{}", 5), "i_structure_UTF-8_BOM_empty_object"},
};

static const JSONTestSuiteTest kJSONTestSuite_n[] = {
    {std::string("[1 true]", 8), "n_array_1_true_without_comma"},
    {std::string("[a\345]", 4), "n_array_a_invalid_utf8"},
    {std::string("[\"\": 1]", 7), "n_array_colon_instead_of_comma"},
    {std::string("[\"\"],", 5), "n_array_comma_after_close"},
    {std::string("[,1]", 4), "n_array_comma_and_number"},
    {std::string("[1,,2]", 6), "n_array_double_comma"},
    {std::string("[\"x\",,]", 7), "n_array_double_extra_comma"},
    {std::string("[\"x\"]]", 6), "n_array_extra_close"},
    {std::string("[\"\",]", 5), "n_array_extra_comma"},
    {std::string("[\"x\"", 4), "n_array_incomplete"},
    {std::string("[x", 2), "n_array_incomplete_invalid_value"},
    {std::string("[3[4]]", 6), "n_array_inner_array_no_comma"},
    {std::string("[\377]", 3), "n_array_invalid_utf8"},
    {std::string("[1:2]", 5), "n_array_items_separated_by_semicolon"},
    {std::string("[,]", 3), "n_array_just_comma"},
    {std::string("[-]", 3), "n_array_just_minus"},
    {std::string("[   , \"\"]", 9), "n_array_missing_value"},
    {std::string("[\"a\",\n4\n,1,", 11), "n_array_newlines_unclosed"},
    {std::string("[1,]", 4), "n_array_number_and_comma"},
    {std::string("[1,,]", 5), "n_array_number_and_several_commas"},
    {std::string("[\"\va\"\\f]", 8), "n_array_spaces_vertical_tab_formfeed"},
    {std::string("[*]", 3), "n_array_star_inside"},
    {std::string("[\"\"", 3), "n_array_unclosed"},
    {std::string("[1,", 3), "n_array_unclosed_trailing_comma"},
    {std::string("[1,\n1\n,1", 8), "n_array_unclosed_with_new_lines"},
    {std::string("[{}", 3), "n_array_unclosed_with_object_inside"},
    {std::string("[fals]", 6), "n_incomplete_false"},
    {std::string("[nul]", 5), "n_incomplete_null"},
    {std::string("[tru]", 5), "n_incomplete_true"},
    {std::string("123\000", 4), "n_multidigit_number_then_00"},
    {std::string("[++1234]", 8), "n_number_++"},
    {std::string("[+1]", 4), "n_number_+1"},
    {std::string("[+Inf]", 6), "n_number_+Inf"},
    {std::string("[-01]", 5), "n_number_-01"},
    {std::string("[-1.0.]", 7), "n_number_-1.0."},
    {std::string("[-2.]", 5), "n_number_-2."},
    {std::string("[-NaN]", 6), "n_number_-NaN"},
    {std::string("[.-1]", 5), "n_number_.-1"},
    {std::string("[.2e-3]", 7), "n_number_.2e-3"},
    {std::string("[0.1.2]", 7), "n_number_0.1.2"},
    {std::string("[0.3e+]", 7), "n_number_0.3e+"},
    {std::string("[0.3e]", 6), "n_number_0.3e"},
    {std::string("[0.e1]", 6), "n_number_0.e1"},
    {std::string("[0e+]", 5), "n_number_0e+"},
    {std::string("[0e]", 4), "n_number_0e"},
    {std::string("[0E+]", 5), "n_number_0_capital_E+"},
    {std::string("[0E]", 4), "n_number_0_capital_E"},
    {std::string("[1.0e+]", 7), "n_number_1.0e+"},
    {std::string("[1.0e-]", 7), "n_number_1.0e-"},
    {std::string("[1.0e]", 6), "n_number_1.0e"},
    {std::string("[1eE2]", 6), "n_number_1eE2"},
    {std::string("[1 000.0]", 9), "n_number_1_000"},
    {std::string("[2.e+3]", 7), "n_number_2.e+3"},
    {std::string("[2.e-3]", 7), "n_number_2.e-3"},
    {std::string("[2.e3]", 6), "n_number_2.e3"},
    {std::string("[9.e+]", 6), "n_number_9.e+"},
    {std::string("[1+2]", 5), "n_number_expression"},
    {std::string("[0x1]", 5), "n_number_hex_1_digit"},
    {std::string("[0x42]", 6), "n_number_hex_2_digits"},
    {std::string("[Inf]", 5), "n_number_Inf"},
    {std::string("[Infinity]", 10), "n_number_infinity"},
    {std::string("[0e+-1]", 7), "n_number_invalid+-"},
    {std::string("[-123.123foo]", 13), "n_number_invalid-negative-real"},
    {std::string("[123\345]", 6), "n_number_invalid-utf-8-in-bigger-int"},
    {std::string("[1e1\345]", 6), "n_number_invalid-utf-8-in-exponent"},
    {std::string("[0\345]\n", 5), "n_number_invalid-utf-8-in-int"},
    {std::string("[-Infinity]", 11), "n_number_minus_infinity"},
    {std::string("[-foo]", 6), "n_number_minus_sign_with_trailing_garbage"},
    {std::string("[- 1]", 5), "n_number_minus_space_1"},
    {std::string("[NaN]", 5), "n_number_NaN"},
    {std::string("[-012]", 6), "n_number_neg_int_starting_with_zero"},
    {std::string("[-.123]", 7), "n_number_neg_real_without_int_part"},
    {std::string("[-1x]", 5), "n_number_neg_with_garbage_at_end"},
    {std::string("[1ea]", 5), "n_number_real_garbage_after_e"},
    {std::string("[1.]", 4), "n_number_real_without_fractional_part"},
    {std::string("[1e\345]", 5), "n_number_real_with_invalid_utf8_after_e"},
    {std::string("[.123]", 6), "n_number_starting_with_dot"},
    {std::string("[\357\274\221]", 5), "n_number_U+FF11_fullwidth_digit_one"},
    {std::string("[1.2a-3]", 8), "n_number_with_alpha"},
    {std::string("[1.8011670033376514H-308]", 25), "n_number_with_alpha_char"},
    {std::string("[012]", 5), "n_number_with_leading_zero"},
    {std::string("[\"x\", truth]", 12), "n_object_bad_value"},
    {std::string("{[: \"x\"}\n", 9), "n_object_bracket_key"},
    {std::string("{\"x\", null}", 11), "n_object_comma_instead_of_colon"},
    {std::string("{\"x\"::\"b\"}", 10), "n_object_double_colon"},
    {std::string("{\360\237\207\250\360\237\207\255}", 10), "n_object_emoji"},
    {std::string("{\"a\":\"a\" 123}", 13), "n_object_garbage_at_end"},
    {std::string("{key: \'value\'}", 14), "n_object_key_with_single_quotes"},
    {std::string("{\"\271\":\"0\",}", 10), "n_object_lone_continuation_byte_in_key_and_trailing_comma"},
    {std::string("{\"a\" b}", 7), "n_object_missing_colon"},
    {std::string("{:\"b\"}", 6), "n_object_missing_key"},
    {std::string("{\"a\" \"b\"}", 9), "n_object_missing_semicolon"},
    {std::string("{\"a\":", 5), "n_object_missing_value"},
    {std::string("{\"a\"", 4), "n_object_no-colon"},
    {std::string("{1:1}", 5), "n_object_non_string_key"},
    {std::string("{9999E9999:1}", 13), "n_object_non_string_key_but_huge_number_instead"},
    {std::string("{\"\271\":\"0\",}", 10), "n_object_pi_in_key_and_trailing_comma"},
    {std::string("{null:null,null:null}", 21), "n_object_repeated_null_null"},
    {std::string("{\"id\":0,,,,,}", 13), "n_object_several_trailing_commas"},
    {std::string("{\'a\':0}", 7), "n_object_single_quote"},
    {std::string("{\"id\":0,}", 9), "n_object_trailing_comma"},
    {std::string("{\"a\":\"b\"}/**/", 13), "n_object_trailing_comment"},
    {std::string("{\"a\":\"b\"}/**//", 14), "n_object_trailing_comment_open"},
    {std::string("{\"a\":\"b\"}//", 11), "n_object_trailing_comment_slash_open"},
    {std::string("{\"a\":\"b\"}/", 10), "n_object_trailing_comment_slash_open_incomplete"},
    {std::string("{\"a\":\"b\",,\"c\":\"d\"}", 18), "n_object_two_commas_in_a_row"},
    {std::string("{a: \"b\"}", 8), "n_object_unquoted_key"},
    {std::string("{\"a\":\"a", 7), "n_object_unterminated-value"},
    {std::string("{ \"foo\" : \"bar\", \"a\" }", 22), "n_object_with_single_string"},
    {std::string("{\"a\":\"b\"}#", 10), "n_object_with_trailing_garbage"},
    {std::string(" ", 1), "n_single_space"},
    {std::string("[\"\\uD800\\\"]", 11), "n_string_1_surrogate_then_escape"},
    {std::string("[\"\\uD800\\u\"]", 12), "n_string_1_surrogate_then_escape_u"},
    {std::string("[\"\\uD800\\u1\"]", 13), "n_string_1_surrogate_then_escape_u1"},
    {std::string("[\"\\uD800\\u1x\"]", 14), "n_string_1_surrogate_then_escape_u1x"},
    {std::string("[\303\251]", 4), "n_string_accentuated_char_no_quotes"},
    {std::string("[\"\\\000\"]", 6), "n_string_backslash_00"},
    {std::string("[\"\\\\\\\"]", 7), "n_string_escaped_backslash_bad"},
    {std::string("[\"\\\t\"]", 6), "n_string_escaped_ctrl_char_tab"},
    {std::string("[\"\\\360\237\214\200\"]", 9), "n_string_escaped_emoji"},
    {std::string("[\"\\x00\"]", 8), "n_string_escape_x"},
    {std::string("[\"\\\"]", 5), "n_string_incomplete_escape"},
    {std::string("[\"\\u00A\"]", 9), "n_string_incomplete_escaped_character"},
    {std::string("[\"\\uD834\\uDd\"]", 14), "n_string_incomplete_surrogate"},
    {std::string("[\"\\uD800\\uD800\\x\"]", 18), "n_string_incomplete_surrogate_escape_invalid"},
    {std::string("[\"\\u\345\"]", 7), "n_string_invalid-utf-8-in-escape"},
    {std::string("[\"\\a\"]", 6), "n_string_invalid_backslash_esc"},
    {std::string("[\"\\uqqqq\"]", 10), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\\345\"]", 6), "n_string_invalid_utf8_after_escape"},
    {std::string("[\\u0020\"asd\"]", 13), "n_string_leading_uescaped_thinspace"},
    {std::string("[\\n]", 4), "n_string_no_quotes_with_bad_escape"},
    {std::string("\"", 1), "n_string_single_doublequote"},
    {std::string("[\'single quote\']", 16), "n_string_single_quote"},
    {std::string("abc", 3), "n_string_single_string_no_double_quotes"},
    {std::string("[\"\\", 3), "n_string_start_escape_unclosed"},
    {std::string("[\"a\000a\"]", 7), "n_string_unescaped_crtl_char"},
    {std::string("[\"new\nline\"]", 12), "n_string_unescaped_newline"},
    {std::string("[\"\t\"]", 5), "n_string_unescaped_tab"},
    {std::string("\"\\UA66D\"", 8), "n_string_unicode_CapitalU"},
    {std::string("\"\"x", 3), "n_string_with_trailing_garbage"},
    {std::string("<.>", 3), "n_structure_angle_bracket_."},
    {std::string("[<null>]", 8), "n_structure_angle_bracket_null"},
    {std::string("[1]x", 4), "n_structure_array_trailing_garbage"},
    {std::string("[1]]", 4), "n_structure_array_with_extra_array_close"},
    {std::string("[\"asd]", 6), "n_structure_array_with_unclosed_string"},
    {std::string("a\303\245", 3), "n_structure_ascii-unicode-identifier"},
    {std::string("[True]", 6), "n_structure_capitalized_True"},
    {std::string("1]", 2), "n_structure_close_unopened_array"},
    {std::string("{\"x\": true,", 11), "n_structure_comma_instead_of_closing_brace"},
    {std::string("[][]", 4), "n_structure_double_array"},
    {std::string("]", 1), "n_structure_end_array"},
    {std::string("\357\273{}", 4), "n_structure_incomplete_UTF8_BOM"},
    {std::string("\345", 1), "n_structure_lone-invalid-utf-8"},
    {std::string("[", 1), "n_structure_lone-open-bracket"},
    {std::string("", 0), "n_structure_no_data"},
    {std::string("[\000]", 3), "n_structure_null-byte-outside-string"},
    {std::string("2@", 2), "n_structure_number_with_trailing_garbage"},
    {std::string("{}}", 3), "n_structure_object_followed_by_closing_object"},
    {std::string("{\"\":", 4), "n_structure_object_unclosed_no_value"},
    {std::string("{\"a\":/*comment*/\"b\"}", 20), "n_structure_object_with_comment"},
    {std::string("{\"a\": true} \"x\"", 15), "n_structure_object_with_trailing_garbage"},
    {std::string("[\'", 2), "n_structure_open_array_apostrophe"},
    {std::string("[,", 2), "n_structure_open_array_comma"},
    {std::string("[{", 2), "n_structure_open_array_open_object"},
    {std::string("[\"a", 3), "n_structure_open_array_open_string"},
    {std::string("[\"a\"", 4), "n_structure_open_array_string"},
    {std::string("{", 1), "n_structure_open_object"},
    {std::string("{]", 2), "n_structure_open_object_close_array"},
    {std::string("{,", 2), "n_structure_open_object_comma"},
    {std::string("{[", 2), "n_structure_open_object_open_array"},
    {std::string("{\"a", 3), "n_structure_open_object_open_string"},
    {std::string("{\'a\'", 4), "n_structure_open_object_string_with_apostrophes"},
    {std::string("[\"\\{[\"\\{[\"\\{[\"\\{", 16), "n_structure_open_open"},
    {std::string("\351", 1), "n_structure_single_eacute"},
    {std::string("*", 1), "n_structure_single_star"},
    {std::string("{\"a\":\"b\"}#{}", 12), "n_structure_trailing_#"},
    {std::string("[\342\201\240]", 5), "n_structure_U+2060_word_joined"},
    {std::string("[\\u000A\"\"]", 10), "n_structure_uescaped_LF_before_string"},
    {std::string("[1", 2), "n_structure_unclosed_array"},
    {std::string("[ false, nul", 12), "n_structure_unclosed_array_partial_null"},
    {std::string("[ true, fals", 12), "n_structure_unclosed_array_unfinished_false"},
    {std::string("[ false, tru", 12), "n_structure_unclosed_array_unfinished_true"},
    {std::string("{\"asd\":\"asd\"", 12), "n_structure_unclosed_object"},
    {std::string("\303\245", 2), "n_structure_unicode-identifier"},
    {std::string("\357\273\277", 3), "n_structure_UTF8_BOM_no_data"},
    {std::string("[\f]", 3), "n_structure_whitespace_formfeed"},
    {std::string("[\342\201\240]", 5), "n_structure_whitespace_U+2060_word_joiner"},

    // From the implementation-defined tests:
    {std::string("{\"\\uDFAA\":0}", 12), "i_object_key_lone_2nd_surrogate"},
    {std::string("[\"\\uDADA\"]", 10), "i_string_1st_surrogate_but_2nd_missing"},
    {std::string("[\"\\uD888\\u1234\"]", 16), "i_string_1st_valid_surrogate_2nd_invalid"},
    {std::string("[\"\\uD800\\uD800\\n\"]", 18), "i_string_incomplete_surrogates_escape_valid"},
    {std::string("[\"\\uD800\\n\"]", 12), "i_string_incomplete_surrogate_and_escape_valid"},
    {std::string("[\"\\uDd1ea\"]", 11), "i_string_incomplete_surrogate_pair"},
    {std::string("[\"\\ud800\"]", 10), "i_string_invalid_lonely_surrogate"},
    {std::string("[\"\\ud800abc\"]", 13), "i_string_invalid_surrogate"},
    {std::string("[\"\377\"]", 5), "i_string_invalid_utf-8"},
    {std::string("[\"\\uDd1e\\uD834\"]", 16), "i_string_inverted_surrogates_U+1D11E"},
    {std::string("[\"\351\"]", 5), "i_string_iso_latin_1"},
    {std::string("[\"\\uDFAA\"]", 10), "i_string_lone_second_surrogate"},
    {std::string("[\"\201\"]", 5), "i_string_lone_utf8_continuation_byte"},
    {std::string("[\"\364\277\277\277\"]", 8), "i_string_not_in_unicode_range"},
    {std::string("[\"\300\257\"]", 6), "i_string_overlong_sequence_2_bytes"},
    {std::string("[\"\374\203\277\277\277\277\"]", 10), "i_string_overlong_sequence_6_bytes"},
    {std::string("[\"\374\200\200\200\200\200\"]", 10), "i_string_overlong_sequence_6_bytes_null"},
    {std::string("[\"\340\377\"]", 6), "i_string_truncated-utf-8"},
    {std::string("\377\376[\000\"\000\351\000\"\000]\000", 12), "i_string_UTF-16LE_with_BOM"},
    {std::string("[\"\346\227\245\321\210\372\"]", 10), "i_string_UTF-8_invalid_sequence"},
    {std::string("\000[\000\"\000\351\000\"\000]", 10), "i_string_utf16BE_no_BOM"},
    {std::string("[\000\"\000\351\000\"\000]\000", 10), "i_string_utf16LE_no_BOM"},
    {std::string("[\"\355\240\200\"]", 7), "i_string_UTF8_surrogate_U+D800"},

    {std::string("[\"\\u\"]", 6), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\u0\"]", 7), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\u0A\"]", 8), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\u0A1\"]", 9), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\u0A1x\"]", 10), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\u0Ax1\"]", 10), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\u0xA1\"]", 10), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\ux0A1\"]", 10), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\uDBFF\\\"]", 11), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\uDBFF\\u\"]", 12), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\uDBFF\\uD\"]", 13), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\uDBFF\\uDF\"]", 14), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\uDBFF\\uDFF\"]", 15), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\uDBFF\\uDFFx\"]", 16), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\uDBFF\\uDFxF\"]", 16), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\uDBFF\\uDxFF\"]", 16), "n_string_invalid_unicode_escape"},
    {std::string("[\"\\uDBFF\\uxDFF\"]", 16), "n_string_invalid_unicode_escape"},
};

TEST_CASE("JSONTestSuite")
{
    SECTION("y")
    {
        for (auto const& test : kJSONTestSuite_y)
        {
            CAPTURE(test.name);

            json::Options options;
#if 1
            options.allow_nan_inf = true;
#endif

            json::Value val;
            auto const res = json::parse(val, test.input.data(), test.input.data() + test.input.size(), options);
            CHECK(res.ec == json::ParseStatus::success);

            // Only the "y" tests must round-trip
            if (test.name[0] == 'y')
            {
                std::string str;
                auto const ok = json::stringify(str, val);
                CHECK(ok == true);

                json::Value val2;
                auto const res2 = json::parse(val2, str.data(), str.data() + str.size());
                CHECK(res2.ec == json::ParseStatus::success);

                CHECK(val == val2);
                CHECK(!(val != val2));
                CHECK(!(val < val2));
                CHECK(val <= val2);
                CHECK(!(val > val2));
                CHECK(val >= val2);
            }
        }
    }

    SECTION("n")
    {
        for (auto const& test : kJSONTestSuite_n)
        {
            CAPTURE(test.name);

            json::Value val;
            auto const res = json::parse(val, test.input.data(), test.input.data() + test.input.size());
            CHECK(res.ec != json::ParseStatus::success);
#if 0
            printf("Test '%s'\n", test.name.c_str());
            printf("   Error: %s\n", ParseStatusDescr(res.ec));
            printf("   Found: %s |%s|\n", TokenKindDescr(res.token.kind), ToPrintableString(res.token.ptr, res.token.end).c_str());
#endif
        }
    }
}

#if 0
static const JSONTestSuiteTest kJSONTestSuite_i[] = {
    {std::string("[123.456e-789]", 14), "i_number_double_huge_neg_exp"},
    {std::string("[0.4e00669999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999969999999006]", 137), "i_number_huge_exp"},
    {std::string("[-1e+9999]", 10), "i_number_neg_int_huge_exp"},
    {std::string("[1.5e+9999]", 11), "i_number_pos_double_huge_exp"},
    {std::string("[-123123e100000]", 16), "i_number_real_neg_overflow"},
    {std::string("[123123e100000]", 15), "i_number_real_pos_overflow"},
    {std::string("[123e-10000000]", 15), "i_number_real_underflow"},
    {std::string("[-123123123123123123123123123123]", 33), "i_number_too_big_neg_int"},
    {std::string("[100000000000000000000]", 23), "i_number_too_big_pos_int"},
    {std::string("[-237462374673276894279832749832423479823246327846]", 51), "i_number_very_big_negative_int"},

    //{std::string("[+Inf]", 6), "i_n_number_+Inf"},
    {std::string("[-NaN]", 6), "i_n_number_-NaN"},
    {std::string("[Inf]", 5), "i_n_number_Inf"},
    {std::string("[Infinity]", 10), "i_n_number_infinity"},
    {std::string("[-Infinity]", 11), "i_n_number_minus_infinity"},
    {std::string("[NaN]", 5), "i_n_number_NaN"},

    {std::string("{\"\\uDFAA\":0}", 12), "i_object_key_lone_2nd_surrogate"},
    {std::string("[\"\\uDADA\"]", 10), "i_string_1st_surrogate_but_2nd_missing"},
    {std::string("[\"\\uD888\\u1234\"]", 16), "i_string_1st_valid_surrogate_2nd_invalid"},
    {std::string("[\"\\uD800\\uD800\\n\"]", 18), "i_string_incomplete_surrogates_escape_valid"},
    {std::string("[\"\\uD800\\n\"]", 12), "i_string_incomplete_surrogate_and_escape_valid"},
    {std::string("[\"\\uDd1ea\"]", 11), "i_string_incomplete_surrogate_pair"},
    {std::string("[\"\\ud800\"]", 10), "i_string_invalid_lonely_surrogate"},
    {std::string("[\"\\ud800abc\"]", 13), "i_string_invalid_surrogate"},
    {std::string("[\"\377\"]", 5), "i_string_invalid_utf-8"},
    {std::string("[\"\\uDd1e\\uD834\"]", 16), "i_string_inverted_surrogates_U+1D11E"},
    {std::string("[\"\351\"]", 5), "i_string_iso_latin_1"},
    {std::string("[\"\\uDFAA\"]", 10), "i_string_lone_second_surrogate"},
    {std::string("[\"\201\"]", 5), "i_string_lone_utf8_continuation_byte"},
    {std::string("[\"\364\277\277\277\"]", 8), "i_string_not_in_unicode_range"},
    {std::string("[\"\300\257\"]", 6), "i_string_overlong_sequence_2_bytes"},
    {std::string("[\"\374\203\277\277\277\277\"]", 10), "i_string_overlong_sequence_6_bytes"},
    {std::string("[\"\374\200\200\200\200\200\"]", 10), "i_string_overlong_sequence_6_bytes_null"},
    {std::string("[\"\340\377\"]", 6), "i_string_truncated-utf-8"},
    {std::string("\377\376[\000\"\000\351\000\"\000]\000", 12), "i_string_UTF-16LE_with_BOM"},
    {std::string("[\"\346\227\245\321\210\372\"]", 10), "i_string_UTF-8_invalid_sequence"},
    {std::string("\000[\000\"\000\351\000\"\000]", 10), "i_string_utf16BE_no_BOM"},
    {std::string("[\000\"\000\351\000\"\000]\000", 10), "i_string_utf16LE_no_BOM"},
    {std::string("[\"\355\240\200\"]", 7), "i_string_UTF8_surrogate_U+D800"},

    {std::string("\357\273\277{}", 5), "i_structure_UTF-8_BOM_empty_object"},
};

TEST_CASE("JSONTestSuite - implementation defined")
{
    SECTION("i")
    {
        for (auto const& test : kJSONTestSuite_i)
        {
            CAPTURE(test.name);

            json::Options options;
            options.allow_nan_inf = true;
            //options.allow_leading_plus = true;

            json::Value val;
            auto const res = json::parse(val, test.input.data(), test.input.data() + test.input.size(), options);
            if (res.ec == json::ParseStatus::success) {
                printf("JSONTestSuite '%s':\n    passed\n", test.name.c_str());
            } else {
                printf("JSONTestSuite '%s':\n    FAILED with error code %d\n", test.name.c_str(), (int)res.ec);
            }
        }
    }
}
#endif
