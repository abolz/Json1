#include "../src/json_strings.h"
#include "../src/json.h"
#include "catch.hpp"

struct UTF8Test {
    std::string input;
    int num_replacements; // 0: ignore
};

// Test cases from:
// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt

static const UTF8Test kInvalidUTF8[] = {
    // 3.1  Unexpected continuation bytes
    // Each unexpected continuation byte should be separately signalled as a
    // malformed sequence of its own.
    {"\x80", 1},                          // 3.1.1  First continuation byte 0x80
    {"\xBF", 1},                          // 3.1.2  Last continuation byte 0xbf
    {"\x80\xBF", 2},                      // 3.1.3  2 continuation bytes
    {"\x80\xBF\x80", 3},                  // 3.1.4  3 continuation bytes
    {"\x80\xBF\x80\xBF", 4},              // 3.1.5  4 continuation bytes
    {"\x80\xBF\x80\xBF\x80", 5},          // 3.1.6  5 continuation bytes
    {"\x80\xBF\x80\xBF\x80\xBF", 6},      // 3.1.7  6 continuation bytes
    {"\x80\xBF\x80\xBF\x80\xBF\x80", 7},  // 3.1.8  7 continuation bytes
    // 3.1.9  Sequence of all 64 possible continuation bytes (0x80-0xbf)
    {"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F"
     "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F"
     "\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF"
     "\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF", 64},

    // 3.2  Lonely start characters
    // 3.2.1  All 32 first bytes of 2-byte sequences (0xc0-0xdf)
    {"\xC0", 1},
    {"\xC1", 1},
    {"\xC2", 1},
    {"\xC3", 1},
    {"\xC4", 1},
    {"\xC5", 1},
    {"\xC6", 1},
    {"\xC7", 1},
    {"\xC8", 1},
    {"\xC9", 1},
    {"\xCA", 1},
    {"\xCB", 1},
    {"\xCC", 1},
    {"\xCD", 1},
    {"\xCE", 1},
    {"\xCF", 1},
    {"\xD0", 1},
    {"\xD1", 1},
    {"\xD2", 1},
    {"\xD3", 1},
    {"\xD4", 1},
    {"\xD5", 1},
    {"\xD6", 1},
    {"\xD7", 1},
    {"\xD8", 1},
    {"\xD9", 1},
    {"\xDA", 1},
    {"\xDB", 1},
    {"\xDC", 1},
    {"\xDD", 1},
    {"\xDE", 1},
    {"\xDF", 1},
    // 3.2.2  All 16 first bytes of 3-byte sequences (0xe0-0xef)
    {"\xE0", 1},
    {"\xE1", 1},
    {"\xE2", 1},
    {"\xE3", 1},
    {"\xE4", 1},
    {"\xE5", 1},
    {"\xE6", 1},
    {"\xE7", 1},
    {"\xE8", 1},
    {"\xE9", 1},
    {"\xEA", 1},
    {"\xEB", 1},
    {"\xEC", 1},
    {"\xED", 1},
    {"\xEE", 1},
    {"\xEF", 1},
    // 3.2.3  All 8 first bytes of 4-byte sequences (0xf0-0xf7)
    {"\xF0", 1},
    {"\xF1", 1},
    {"\xF2", 1},
    {"\xF3", 1},
    {"\xF4", 1},
    {"\xF5", 1},
    {"\xF6", 1},
    {"\xF7", 1},
    // 3.2.4  All 4 first bytes of 5-byte sequences (0xf8-0xfb)
    {"\xF8", 1},
    {"\xF9", 1},
    {"\xFA", 1},
    {"\xFB", 1},
    // 3.2.5  All 2 first bytes of 6-byte sequences (0xfc-0xfd)
    {"\xFC", 1},
    {"\xFD", 1},

    // 3.3  Sequences with last continuation byte missing
    // All bytes of an incomplete sequence should be signalled as a single
    // malformed sequence, i.e., you should see only a single replacement
    // character in each of the next tests.
    {"\xC0", 1},                 // 2-byte sequence with last byte missing (U+0000)
    {"\xE0\x80", 2},             // 3-byte sequence with last byte missing (U+0000)
    {"\xF0\x80\x80", 3},         // 4-byte sequence with last byte missing (U+0000)

    {"\xE0\xA0", 1},             // 3-byte sequence with last byte missing (U+0000)
    {"\xF0\x90\x80", 1},         // 4-byte sequence with last byte missing (U+0000)

    {"\xDF", 1},                 // 2-byte sequence with last byte missing (U-000007FF)
    {"\xEF\xBF", 1},             // 3-byte sequence with last byte missing (U-0000FFFF)
    {"\xF7\xBF\xBF", 3},         // 4-byte sequence with last byte missing (U-001FFFFF)

    {"\xF4\x8F\xBF", 1},         // 4-byte sequence with last byte missing (U-001FFFFF)

    // 3.4  Concatenation of incomplete sequences
    // All the 9 sequences of 3.3 concatenated, you should see 14 malformed
    // sequences being signalled
    {"\xC0"
     "\xE0\x80"
     "\xF0\x80\x80"
     "\xDF"
     "\xEF\xBF"
     "\xF7\xBF\xBF"
     "\xE0\xA0"
     "\xF0\x90\x80"
     "\xF4\x8F\xBF", 14},

    // 3.5  Impossible bytes
    {"\xFE", 1},
    {"\xFF", 1},
    {"\xFE\xFE\xFF\xFF", 4},

    // 4.1  Examples of an overlong ASCII character
    {"\xC0\xAF", 2},                 // U+002F
    {"\xE0\x80\xAF", 3},             // U+002F
    {"\xF0\x80\x80\xAF", 4},         // U+002F
#if 1
    {"\xF8\x80\x80\x80\xAF", 5},     // U+002F
    {"\xFC\x80\x80\x80\x80\xAF", 6}, // U+002F
#endif

    // 4.2  Maximum overlong sequences
    {"\xC1\xBF", 2},                 // U-0000007F
    {"\xE0\x9F\xBF", 3},             // U-000007FF
    {"\xF0\x8F\xBF\xBF", 4},         // U-0000FFFF
#if 1
    {"\xF8\x87\xBF\xBF\xBF", 5},     // U-001FFFFF
    {"\xFC\x83\xBF\xBF\xBF\xBF", 6}, // U-03FFFFFF
#endif

    // 4.3  Overlong representation of the NUL character
    {"\xC0\x80", 2},                 // U+0000
    {"\xE0\x80\x80", 3},             // U+0000
    {"\xF0\x80\x80\x80", 4},         // U+0000
#if 1
    {"\xF8\x80\x80\x80\x80", 5},     // U+0000
    {"\xFC\x80\x80\x80\x80\x80", 6}, // U+0000
#endif

    // 5.1 Single UTF-16 surrogates
    {"\xED\xA0\x80", 3}, // U+D800
    {"\xED\xAD\xBF", 3}, // U+DB7F
    {"\xED\xAE\x80", 3}, // U+DB80
    {"\xED\xAF\xBF", 3}, // U+DBFF
    {"\xED\xB0\x80", 3}, // U+DC00
    {"\xED\xBE\x80", 3}, // U+DF80
    {"\xED\xBF\xBF", 3}, // U+DFFF

    // 5.2 Paired UTF-16 surrogates
    {"\xED\xA0\x80\xED\xB0\x80", 6}, // U+D800 U+DC00
    {"\xED\xA0\x80\xED\xBF\xBF", 6}, // U+D800 U+DFFF
    {"\xED\xAD\xBF\xED\xB0\x80", 6}, // U+DB7F U+DC00
    {"\xED\xAD\xBF\xED\xBF\xBF", 6}, // U+DB7F U+DFFF
    {"\xED\xAE\x80\xED\xB0\x80", 6}, // U+DB80 U+DC00
    {"\xED\xAE\x80\xED\xBF\xBF", 6}, // U+DB80 U+DFFF
    {"\xED\xAF\xBF\xED\xB0\x80", 6}, // U+DBFF U+DC00
    {"\xED\xAF\xBF\xED\xBF\xBF", 6}, // U+DBFF U+DFFF
};

static inline std::string TimesString(std::string const& s, int count)
{
    std::string out;
    for (int i = 0; i < count; ++i) {
        out += s;
    }
    return out;
}

static inline std::string ToPrintableString(std::string const& s)
{
    std::string out;
    for (char ch : s)
    {
        uint8_t const byte = static_cast<uint8_t>(ch);
        if (0x20 <= byte && byte <= 0x7E)
        {
            out += ch;
        }
        else
        {
#if 0
            out += '\\';
            out += "01234567"[(byte >> 6) & 0x7];
            out += "01234567"[(byte >> 3) & 0x7];
            out += "01234567"[(byte >> 0) & 0x7];
#else
            out += "\\x";
            out += "0123456789ABCDEF"[(byte >> 4) & 0xF];
            out += "0123456789ABCDEF"[(byte >> 0) & 0xF];
#endif
        }
    }
    return out;
}

TEST_CASE("Invalid UTF-8")
{
    for (auto const& test : kInvalidUTF8)
    {
        CAPTURE(ToPrintableString(test.input));

        json::Value j;
        auto const ec = json::parse(j, "\"" + test.input + "\"");
        CHECK(ec != json::ParseStatus::success);

        j = test.input;
        CHECK(j.is_string());

        std::string out;
        auto const ok = json::stringify(out, j);
        CHECK(ok == false);
    }

#if 1
    // When using 'allow_invalid_unicode' all the strings should be decoded into a
    // sequence of U+FFFD replacement characters.
    for (auto const& test : kInvalidUTF8)
    {
        CAPTURE(ToPrintableString(test.input));

        char const* next = test.input.data();
        char const* last = test.input.data() + test.input.size();

        std::string str;
        auto const res = json::strings::UnescapeString(next, last, [&](char ch) { str += ch; }, /*allow_invalid_unicode*/ true);
        CHECK(res.ec == json::strings::Status::success);
        CHECK(res.ptr == last);

        CHECK(str == TimesString("\xEF\xBF\xBD", test.num_replacements));
    }
#endif
}

TEST_CASE("Invalid UTF-8 (Unicode 11.0)")
{
    auto test = [](std::string const& input, std::string const& expected) {
        CAPTURE(ToPrintableString(input));
        CAPTURE(ToPrintableString(expected));

        std::string output;
        json::strings::EscapeString(input.data(), input.data() + input.size(), [&](char ch) { output.push_back(ch); }, /*allow_invalid_unicode*/ true);
        CAPTURE(ToPrintableString(output));

        CHECK(output == expected);
    };

    test("\xC2\x41\x42",                            "\xEF\xBF\xBD" "\x41" "\x42");
    test("\xF0\x80\x80\x41",                        "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\x41");
    test("\xC0\xAF\xE0\x80\xBF\xF0\x81\x82\x41",    "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\x41");
    test("\xED\xA0\x80\xED\xBF\xBF\xED\xAF\x41",    "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\x41");
    test("\xF4\x91\x92\x93\xFF\x41\x80\xBF\x42",    "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\x41" "\xEF\xBF\xBD""\xEF\xBF\xBD" "\x42");
    test("\xE1\x80\xE2\xF0\x91\x92\xF1\xBF\x41",    "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\xEF\xBF\xBD" "\x41");
}
