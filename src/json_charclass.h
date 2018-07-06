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

#pragma once

#include <cassert>
#include <cstdint>

#ifndef JSON_ASSERT
#define JSON_ASSERT(X) assert(X)
#endif

namespace json {
namespace charclass {

enum ECharClass : uint8_t {
    CC_None             = 0,    // nothing special
    CC_Whitespace       = 0x01, // whitespace  : '\t', '\n', '\r', ' '
    CC_Digit            = 0x02, // digit       : '0'...'9'
    CC_AsciiControl     = 0x04,
    CC_IdentifierBody   = 0x10, // ident-body  : IsDigit, IsLetter, '_', '$'
    CC_ValidEscapedChar = 0x20, // valid escaped chars: '"', '\\', '/', 'b', 'f', 'n', 'r', 't', 'u'
    CC_StringSpecial    = 0x40, // quote or bs : '"', '\'', '`', '\\'
    CC_NeedsCleaning    = 0x80, // needs cleaning (strings)
};

inline unsigned CharClass(char ch)
{
#define W CC_Whitespace
#define D CC_Digit
#define A CC_AsciiControl
#define I CC_IdentifierBody
#define E CC_ValidEscapedChar
#define S CC_StringSpecial
#define C CC_NeedsCleaning

    static constexpr uint8_t const kMap[] = {
    //  NUL     SOH     STX     ETX     EOT     ENQ     ACK     BEL     BS      HT      LF      VT      FF      CR      SO      SI
        A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|W|C,  A|W|C,  A|C,    A|C,    A|W|C,  A|C,    A|C,
    //  DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB     CAN     EM      SUB     ESC     FS      GS      RS      US
        A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,
    //  space   !       "       #       $       %       &       '       (       )       *       +       ,       -       .       /
        W,      0,      S|E,    0,      I,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      E,
    //  0       1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
        D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    0,      0,      0,      0,      0,      0,
    //  @       A       B       C       D       E       F       G       H       I       J       K       L       M       N       O
        0,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,
    //  P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
        I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      0,      E|S|C,  0,      0,      I,
    //  `       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
        0,      I,      E|I,    I,      I,      I  ,    E|I,    I,      I,      I,      I,      I,      I,      I,      E|I,    I,
    //  p       q       r       s       t       u       v       w       x       y       z       {       |       }       ~       DEL
        I,      I,      E|I,    I,      E|I,    E|I,    I,      I,      I,      I,      I,      0,      0,      0,      0,      0,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
    };

#undef C
#undef S
#undef E
#undef I
#undef A
#undef D
#undef W

    return kMap[static_cast<unsigned char>(ch)];
}

inline bool IsWhitespace      (char ch) { return (CharClass(ch) & CC_Whitespace      ) != 0; }
inline bool IsDigit           (char ch) { return (CharClass(ch) & CC_Digit           ) != 0; }
inline bool IsIdentifierBody  (char ch) { return (CharClass(ch) & CC_IdentifierBody  ) != 0; }
inline bool IsValidEscapedChar(char ch) { return (CharClass(ch) & CC_ValidEscapedChar) != 0; }
inline bool IsStringSpecial   (char ch) { return (CharClass(ch) & CC_StringSpecial   ) != 0; }
inline bool NeedsCleaning     (char ch) { return (CharClass(ch) & CC_NeedsCleaning   ) != 0; }

#if 1
inline int HexDigitValue(char ch)
{
#define N -1
    static constexpr int8_t const kMap[256] = {
    //  NUL     SOH     STX     ETX     EOT     ENQ     ACK     BEL     BS      HT      LF      VT      FF      CR      SO      SI
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB     CAN     EM      SUB     ESC     FS      GS      RS      US
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  space   !       "       #       $       %       &       '       (       )       *       +       ,       -       .       /
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  0       1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
        0,      1,      2,      3,      4,      5,      6,      7,      8,      9,      N,      N,      N,      N,      N,      N,
    //  @       A       B       C       D       E       F       G       H       I       J       K       L       M       N       O
        N,      10,     11,     12,     13,     14,     15,     N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  `       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
        N,      10,     11,     12,     13,     14,     15,     N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  p       q       r       s       t       u       v       w       x       y       z       {       |       }       ~       DEL
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    };
#undef N

    return kMap[static_cast<unsigned char>(ch)];
}
#else
inline int HexDigitValue(char ch)
{
    if ('0' <= ch && ch <= '9') { return ch - '0'; }
    if ('A' <= ch && ch <= 'F') { return ch - 'A' + 10; }
    if ('a' <= ch && ch <= 'f') { return ch - 'a' + 10; }
    return -1;
}
#endif

} // namespace charclass
} // namespace json
