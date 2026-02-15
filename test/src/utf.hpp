#ifndef TEST_UTF8_HPP
#define TEST_UTF8_HPP

#include <uni-cpp/encoding.hpp>
#include <uni-cpp/string.hpp>

#include <array>
#include <tuple>
#include <string>

namespace upp_test::utf
{
    struct utf_sequences
    {
        std::u8string  utf8_seq;
        std::u16string utf16_seq;
        std::u32string utf32_seq;

        template<upp::unicode_encoding Encoding, typename Self>
        [[nodiscard]] constexpr auto&& encoded_with(this Self&& self) noexcept
        {
            if constexpr (Encoding == upp::unicode_encoding::utf8)
                return std::forward<Self>(self).utf8_seq;
            else if constexpr (Encoding == upp::unicode_encoding::utf16)
                return std::forward<Self>(self).utf16_seq;
            else if constexpr (Encoding == upp::unicode_encoding::utf32)
                return std::forward<Self>(self).utf32_seq;
        }
    };

    [[nodiscard]] constexpr auto valid_sequences()
    {
#define TEST_UTF_SEQ(literal)                                                     \
    utf_sequences                                                                 \
    {                                                                             \
        .utf8_seq = u8##literal, .utf16_seq = u##literal, .utf32_seq = U##literal \
    }

        return std::to_array<utf_sequences>(
            {TEST_UTF_SEQ(""), TEST_UTF_SEQ("a"), TEST_UTF_SEQ("\U0001FCCC"), TEST_UTF_SEQ("\u0041\u0062\u007A\u00A9\u00F1"),
             TEST_UTF_SEQ("\u03A9\u20AC\u221E"), TEST_UTF_SEQ("\u0065\u0301\u006E\u0303\u0061\u0308"), TEST_UTF_SEQ("\u00E9\u0065\u0301"),
             TEST_UTF_SEQ("\u200B\u200C\u200D\u2060\uFEFF"), TEST_UTF_SEQ("\U0001F600\U0001F642"), TEST_UTF_SEQ("\uFDD0\uFDEF\U0001FFFE"),
             TEST_UTF_SEQ("\uFFF0\uFFFD\uFFE8")});

#undef TEST_UTF_SEQ
    }

    [[nodiscard]] constexpr auto invalid_utf8_test_cases()
    {
        struct test_case
        {
            std::u8string   input;
            upp::utf8_error expected_error;
        };

        return std::to_array<test_case>({
               {.input          = {0x8F}, // Too Long (0 bytes)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0x39, 0x8C}, // Too Long (1 byte)
                .expected_error = {.valid_up_to = 1, .error_length = 1}

            }, {.input          = {0xC6, 0x84, 0x98}, // Too Long (2 bytes)
                .expected_error = {.valid_up_to = 2, .error_length = 1}

            }, {.input          = {0xE0, 0xA0, 0x8C, 0x9E}, // Too Long (3 bytes)
                .expected_error = {.valid_up_to = 3, .error_length = 1}

            }, {.input          = {0xF3, 0xA0, 0x81, 0x88, 0xBC}, // Too Long (4 bytes)
                .expected_error = {.valid_up_to = 4, .error_length = 1}
            
            }, {.input          = {0xF8, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (5 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0xFC, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (6 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0xFE, 0x80, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (7 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0xFF, 0x80, 0x80, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (8 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error_length = 1}
            
            }, {.input          = {0xC4}, // Too Short (at the end, should be 2-byte long, 1 byte missing, U+0104)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt}

            }, {.input          = {0xC4, u8'A'}, // Too Short (in the middle, should be 2-byte long, 1 byte missing, U+0104)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0xE1}, // Too Short (at the end, should be 3-byte long, 2 bytes missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt}

            }, {.input          = {0xE1, u8'0'}, // Too Short (in the middle, should be 3-byte long, 2 bytes missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error_length = 1}
            
            }, {.input          = {0xE1, 0x83}, // Too Short (at the end, should be 3-byte long, 1 byte missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt}

            }, {.input          = {0xE1, 0x83, u8' '}, // Too Short (in the middle, should be 3-byte long, 1 byte missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error_length = 2}

            }, {.input          = {0xF3}, // Too Short (at the end, should be 4-byte long, 3 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt}

            }, {.input          = {0xF3, u8' '}, // Too Short (in the middle, should be 4-byte long, 3 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0xF3, 0xA0}, // Too Short (at the end, should be 4-byte long, 2 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt}

            }, {.input          = {0xF3, 0xA0, u8' '}, // Too Short (in the middle, should be 4-byte long, 2 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = 2}

            }, {.input          = {0xF3, 0xA0, 0x86}, // Too Short (at the end, should be 4-byte long, 1 byte missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt}

            }, {.input          = {0xF3, 0xA0, 0x86, u8' '}, // Too Short (in the middle, should be 4-byte long, 1 byte missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = 3}

            }, {.input          = {0xC0, 0xB6}, // Overlong (2-byte long, U+0036)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0xE0, 0x97, 0x9F}, // Overlong (3-byte long, U+05DF)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0xF0, 0x8F, 0xAC, 0xB4}, // Overlong (4-byte long, U+FB34)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0xF7, 0x8C, 0x9A, 0x8D}, // Too Large (detectable at 1st byte)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0xF4, 0x90, 0x91, 0xB1}, // Too Large (detectable at 2nd byte)
                .expected_error = {.valid_up_to = 0, .error_length = 1}

            }, {.input          = {0xED, 0xA1, 0xB4}, // Surrogate (0xD874)
                .expected_error = {.valid_up_to = 0, .error_length = 1}
            },
        });
    }
} // namespace upp_test::utf

#endif // TEST_UTF8_HPP