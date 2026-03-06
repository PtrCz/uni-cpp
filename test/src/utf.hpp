#ifndef TEST_UTF_HPP
#define TEST_UTF_HPP

#include <uni-cpp/encoding.hpp>
#include <uni-cpp/string.hpp>

#include <array>
#include <string>

namespace upp_test::utf
{
    struct utf_sequences
    {
        std::u8string  utf8_seq;
        std::u16string utf16_seq;
        std::u32string utf32_seq;

        template<upp::encoding Encoding, typename Self>
            requires upp::unicode_encoding<Encoding>
        [[nodiscard]] constexpr auto&& encoded_with(this Self&& self) noexcept
        {
            if constexpr (Encoding == upp::encoding::utf8)
                return std::forward<Self>(self).utf8_seq;
            else if constexpr (Encoding == upp::encoding::utf16)
                return std::forward<Self>(self).utf16_seq;
            else if constexpr (Encoding == upp::encoding::utf32)
                return std::forward<Self>(self).utf32_seq;
        }
    };

    [[nodiscard]] constexpr auto valid_sequences()
    {
        using namespace std::string_literals;

// Use the languages ability to transcode the string literal to different encodings for us.
#define TEST_UTF_SEQ(literal)                                                              \
    utf_sequences                                                                          \
    {                                                                                      \
        .utf8_seq = u8##literal##s, .utf16_seq = u##literal##s, .utf32_seq = U##literal##s \
    }

        return std::to_array<utf_sequences>(
            {TEST_UTF_SEQ(""), TEST_UTF_SEQ("a"), TEST_UTF_SEQ("\U0001FCCC"), TEST_UTF_SEQ("\u0041\u0062\u007A\u00A9\u00F1"),
             TEST_UTF_SEQ("\u03A9\u20AC\u221E"), TEST_UTF_SEQ("\u0065\u0301\u006E\u0303\u0061\u0308"), TEST_UTF_SEQ("\u00E9\u0065\u0301"),
             TEST_UTF_SEQ("\u200B\u200C\u200D\u2060\uFEFF"), TEST_UTF_SEQ("\U0001F600\U0001F642"), TEST_UTF_SEQ("\uFDD0\uFDEF\U0001FFFE"),
             TEST_UTF_SEQ("\uFFF0\uFFFD\uFFE8")});

#undef TEST_UTF_SEQ
    }

    template<typename InputType, typename ErrorType>
    class invalid_test_case_type
    {
    public:
        InputType      input;
        ErrorType      expected_error;
        std::u8string  utf8_lossy;
        std::u16string utf16_lossy;
        std::u32string utf32_lossy;

        template<upp::encoding Encoding, typename Self>
            requires upp::unicode_encoding<Encoding>
        [[nodiscard]] constexpr auto&& encoded_lossily_with(this Self&& self) noexcept
        {
            if constexpr (Encoding == upp::encoding::utf8)
                return std::forward<Self>(self).utf8_lossy;
            else if constexpr (Encoding == upp::encoding::utf16)
                return std::forward<Self>(self).utf16_lossy;
            else if constexpr (Encoding == upp::encoding::utf32)
                return std::forward<Self>(self).utf32_lossy;
        }
    };

// Use the languages ability to transcode the string literal to different encodings for us.
#define TEST_INVALID_UTF_LOSSY(literal) .utf8_lossy = u8##literal##s, .utf16_lossy = u##literal##s, .utf32_lossy = U##literal##s

    [[nodiscard]] constexpr auto invalid_utf8_test_cases()
    {
        using test_case = invalid_test_case_type<std::u8string, upp::utf8_error>;
        using namespace std::string_literals;

        return std::to_array<test_case>({
            {
                .input          = {0x8F}, // Too Long (0 bytes)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0x39, 0x8C}, // Too Long (1 byte)
                .expected_error = {.valid_up_to = 1, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\u0039\uFFFD"),
            },
            {
                .input          = {0xC6, 0x84, 0x98}, // Too Long (2 bytes)
                .expected_error = {.valid_up_to = 2, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\u0184\uFFFD"),
            },
            {
                .input          = {0xE0, 0xA0, 0x8C, 0x9E}, // Too Long (3 bytes)
                .expected_error = {.valid_up_to = 3, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\u080C\uFFFD"),
            },
            {
                .input          = {0xF3, 0xA0, 0x81, 0x88, 0xBC}, // Too Long (4 bytes)
                .expected_error = {.valid_up_to = 4, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\u{E0048}\uFFFD"),
            },
            {
                .input          = {0xF8, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (5 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD"),
            },
            {
                .input          = {0xFC, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (6 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD"),
            },
            {
                .input          = {0xFE, 0x80, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (7 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD"),
            },
            {
                .input          = {0xFF, 0x80, 0x80, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (8 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD"),
            },
            {
                .input          = {0xC4}, // Too Short (at the end, should be 2-byte long, 1 byte missing, U+0104)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0xC4, u8'A'}, // Too Short (in the middle, should be 2-byte long, 1 byte missing, U+0104)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\u0041"),
            },
            {
                .input          = {0xE1}, // Too Short (at the end, should be 3-byte long, 2 bytes missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0xE1, u8'0'}, // Too Short (in the middle, should be 3-byte long, 2 bytes missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\u0030"),
            },
            {
                .input          = {0xE1, 0x83}, // Too Short (at the end, should be 3-byte long, 1 byte missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0xE1, 0x83, u8' '}, // Too Short (in the middle, should be 3-byte long, 1 byte missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error_length = 2},
                TEST_INVALID_UTF_LOSSY("\uFFFD\u0020"),
            },
            {
                .input          = {0xF3}, // Too Short (at the end, should be 4-byte long, 3 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0xF3, u8' '}, // Too Short (in the middle, should be 4-byte long, 3 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\u0020"),
            },
            {
                .input          = {0xF3, 0xA0}, // Too Short (at the end, should be 4-byte long, 2 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0xF3, 0xA0, u8' '}, // Too Short (in the middle, should be 4-byte long, 2 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = 2},
                TEST_INVALID_UTF_LOSSY("\uFFFD\u0020"),
            },
            {
                .input          = {0xF3, 0xA0, 0x86}, // Too Short (at the end, should be 4-byte long, 1 byte missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0xF3, 0xA0, 0x86, u8' '}, // Too Short (in the middle, should be 4-byte long, 1 byte missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error_length = 3},
                TEST_INVALID_UTF_LOSSY("\uFFFD\u0020"),
            },
            {
                .input          = {0xC0, 0xB6}, // Overlong (2-byte long, U+0036)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD"),
            },
            {
                .input          = {0xE0, 0x97, 0x9F}, // Overlong (3-byte long, U+05DF)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD\uFFFD"),
            },
            {
                .input          = {0xF0, 0x8F, 0xAC, 0xB4}, // Overlong (4-byte long, U+FB34)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD"),
            },
            {
                .input          = {0xF7, 0x8C, 0x9A, 0x8D}, // Too Large (detectable at 1st byte)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD"),
            },
            {
                .input          = {0xF4, 0x90, 0x91, 0xB1}, // Too Large (detectable at 2nd byte)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD"),
            },
            {
                .input          = {0xED, 0xA1, 0xB4}, // Surrogate (0xD874)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD\uFFFD"),
            },
        });
    }

    [[nodiscard]] constexpr auto invalid_utf16_test_cases()
    {
        using test_case = invalid_test_case_type<std::u16string, upp::utf16_error>;
        using namespace std::string_literals;

        return std::to_array<test_case>({
            {
                .input          = {0xD800}, // Isolated high surrogate (at the end)
                .expected_error = {.valid_up_to = 0, .error_length = std::nullopt},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0xD800, u'A'}, // Isolated high surrogate (in the middle)
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\u0041"),
            },
            {
                .input          = {0xDC00}, // Isolated low surrogate
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {u'A', 0xDC00}, // Isolated low surrogate
                .expected_error = {.valid_up_to = 1, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("A\uFFFD"),
            },
            {
                .input          = {0xDC00, 0xD800}, // Reversed surrogate pair
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD"),
            },
            {
                .input          = {0xD800, 0xD801}, // High surrogate followed by high surrogate
                .expected_error = {.valid_up_to = 0, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\uFFFD\uFFFD"),
            },
            {
                .input          = {0xD83D, 0xDE00, 0xDC00}, // Valid surrogate pair followed by an isolated low surrogate
                .expected_error = {.valid_up_to = 2, .error_length = 1},
                TEST_INVALID_UTF_LOSSY("\u{1F600}\uFFFD"),
            },
        });
    }

    [[nodiscard]] constexpr auto invalid_utf32_test_cases()
    {
        using test_case = invalid_test_case_type<std::u32string, upp::utf32_error>;
        using namespace std::string_literals;

        return std::to_array<test_case>({
            {
                .input          = {0xD800}, // Surrogate (high surrogate range)
                .expected_error = {.valid_up_to = 0},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0xDFFF}, // Surrogate (low surrogate range)
                .expected_error = {.valid_up_to = 0},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0x110000}, // Too Large
                .expected_error = {.valid_up_to = 0},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0xFFFFFFFF}, // Too Large
                .expected_error = {.valid_up_to = 0},
                TEST_INVALID_UTF_LOSSY("\uFFFD"),
            },
            {
                .input          = {0x0041, 0xD834}, // Valid code point followed by a surrogate
                .expected_error = {.valid_up_to = 1},
                TEST_INVALID_UTF_LOSSY("\u0041\uFFFD"),
            },
            {
                .input          = {0x1F600, 0x110000}, // Valid code point followed by an out-of-range value
                .expected_error = {.valid_up_to = 1},
                TEST_INVALID_UTF_LOSSY("\u{1F600}\uFFFD"),
            },
        });
    }

#undef TEST_INVALID_UTF_LOSSY

    template<upp::encoding Encoding>
        requires upp::unicode_encoding<Encoding>
    [[nodiscard]] constexpr auto invalid_test_cases_for_encoding()
    {
        if constexpr (Encoding == upp::encoding::utf8)
            return invalid_utf8_test_cases();
        else if constexpr (Encoding == upp::encoding::utf16)
            return invalid_utf16_test_cases();
        else if constexpr (Encoding == upp::encoding::utf32)
            return invalid_utf32_test_cases();
    }
} // namespace upp_test::utf

#endif // TEST_UTF_HPP