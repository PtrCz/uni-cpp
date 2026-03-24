#ifndef TEST_ASCII_HPP
#define TEST_ASCII_HPP

#include <uni-cpp/encoding.hpp>
#include <uni-cpp/string.hpp>

#include <array>
#include <string>
#include <string_view>
#include <stdexcept>
#include <expected>
#include <vector>
#include <bit>

#include "macros.hpp"
#include "../utility.hpp"

namespace upp_test::ascii
{
    [[nodiscard]] consteval char operator""_char(const unsigned long long int value)
    {
        if (value > 0xFFULL)
        {
            throw std::invalid_argument("Invalid 8-bit value");
        }

        return std::bit_cast<char>(static_cast<std::uint8_t>(value));
    }

    struct valid_sequence
    {
        std::string    sequence;
        std::u8string  as_utf8;
        std::u16string as_utf16;
        std::u32string as_utf32;

        template<upp::encoding Encoding, typename Self>
            requires upp::unicode_encoding<Encoding>
        [[nodiscard]] constexpr auto&& encoded_as(this Self&& self) noexcept
        {
            if constexpr (Encoding == upp::encoding::utf8)
                return std::forward<Self>(self).as_utf8;
            else if constexpr (Encoding == upp::encoding::utf16)
                return std::forward<Self>(self).as_utf16;
            else if constexpr (Encoding == upp::encoding::utf32)
                return std::forward<Self>(self).as_utf32;
        }
    };

    [[nodiscard]] constexpr auto valid_sequences()
    {
        using namespace upp_test::string_literals;

#define VALID_ASCII_SEQ(literal)                                                                                    \
    valid_sequence                                                                                                  \
    {                                                                                                               \
        .sequence = literal##_s, .as_utf8 = u8##literal##_s, .as_utf16 = u##literal##_s, .as_utf32 = U##literal##_s \
    }

        return std::to_array<valid_sequence>(
            {VALID_ASCII_SEQ(""), VALID_ASCII_SEQ("\x20"), VALID_ASCII_SEQ("\x00"), VALID_ASCII_SEQ("\x0A"), VALID_ASCII_SEQ("\x61"),
             VALID_ASCII_SEQ("\x41\x31"), VALID_ASCII_SEQ("\x21\x7E"), VALID_ASCII_SEQ("\x22"), VALID_ASCII_SEQ("\x61\x09\x62"),
             VALID_ASCII_SEQ("\x61\x00\x62")});

#undef VALID_ASCII_SEQ
    }

    struct invalid_sequence
    {
        std::string           sequence;
        upp::from_ascii_error expected_error;

        std::string    as_ascii_lossy;
        std::u8string  as_utf8_lossy;
        std::u16string as_utf16_lossy;
        std::u32string as_utf32_lossy;

        std::vector<std::expected<char8_t, upp::ascii_error>>  expected_utf8_transcoding_with_errors;
        std::vector<std::expected<char16_t, upp::ascii_error>> expected_utf16_transcoding_with_errors;
        std::vector<std::expected<char32_t, upp::ascii_error>> expected_utf32_transcoding_with_errors;

        template<upp::encoding Encoding, typename Self>
            requires upp::unicode_encoding<Encoding>
        [[nodiscard]] constexpr auto&& lossily_encoded_as(this Self&& self) noexcept
        {
            if constexpr (Encoding == upp::encoding::utf8)
                return std::forward<Self>(self).as_utf8_lossy;
            else if constexpr (Encoding == upp::encoding::utf16)
                return std::forward<Self>(self).as_utf16_lossy;
            else if constexpr (Encoding == upp::encoding::utf32)
                return std::forward<Self>(self).as_utf32_lossy;
        }

        template<upp::encoding Encoding, typename Self>
            requires upp::unicode_encoding<Encoding>
        [[nodiscard]] constexpr auto&& transcoded_with_errors_to(this Self&& self) noexcept
        {
            if constexpr (Encoding == upp::encoding::utf8)
                return std::forward<Self>(self).expected_utf8_transcoding_with_errors;
            else if constexpr (Encoding == upp::encoding::utf16)
                return std::forward<Self>(self).expected_utf16_transcoding_with_errors;
            else if constexpr (Encoding == upp::encoding::utf32)
                return std::forward<Self>(self).expected_utf32_transcoding_with_errors;
        }
    };

    [[nodiscard]] constexpr auto invalid_sequences()
    {
        using namespace upp_test::string_literals;

#define AS_UTF_LOSSY(literal) .as_utf8_lossy = u8##literal##_s, .as_utf16_lossy = u##literal##_s, .as_utf32_lossy = U##literal##_s

#define EXPECTED_UTF_TRANSCODING_WITH_ERRORS(...) TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS(upp::encoding::ascii, __VA_ARGS__)

#define APPEND_STRING_LITERAL(literal) TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_APPEND_STRING_LITERAL(upp::ascii_error, literal)
#define APPEND_ERROR(...)              TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_APPEND_ERROR(upp::ascii_error, __VA_ARGS__)

        return std::to_array<invalid_sequence>({
            {
                .sequence       = {0x93_char},
                .expected_error = {.valid_up_to = 0, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x1A"_s,
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({ APPEND_ERROR(upp::ascii_error{}); }),
            },
            {
                .sequence       = {0x71_char, 0x7A_char, 0x39_char, 0x5F_char, 0x80_char, 0x58_char},
                .expected_error = {.valid_up_to = 4, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x71\x7A\x39\x5F\x1A\x58"_s,
                AS_UTF_LOSSY("\u0071\u007A\u0039\u005F\uFFFD\u0058"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0071\u007A\u0039\u005F"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u0058"sv);
                }),
            },
            {
                .sequence       = {0x41_char, 0x37_char, 0x23_char, 0xFF_char, 0x40_char},
                .expected_error = {.valid_up_to = 3, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x41\x37\x23\x1A\x40"_s,
                AS_UTF_LOSSY("\u0041\u0037\u0023\uFFFD\u0040"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0041\u0037\u0023"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u0040"sv);
                }),
            },
            {
                .sequence       = {0x80_char, 0x4B_char, 0x33_char, 0x24_char, 0x5E_char},
                .expected_error = {.valid_up_to = 0, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x1A\x4B\x33\x24\x5E"_s,
                AS_UTF_LOSSY("\uFFFD\u004B\u0033\u0024\u005E"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u004B\u0033\u0024\u005E"sv);
                }),
            },
            {
                .sequence       = {0x5A_char, 0x39_char, 0x7F_char, 0x80_char, 0x21_char},
                .expected_error = {.valid_up_to = 3, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x5A\x39\x7F\x1A\x21"_s,
                AS_UTF_LOSSY("\u005A\u0039\u007F\uFFFD\u0021"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u005A\u0039\u007F"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u0021"sv);
                }),
            },
            {
                .sequence       = {0x31_char, 0x5F_char, 0x32_char, 0x2D_char, 0xC0_char},
                .expected_error = {.valid_up_to = 4, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x31\x5F\x32\x2D\x1A"_s,
                AS_UTF_LOSSY("\u0031\u005F\u0032\u002D\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0031\u005F\u0032\u002D"sv);
                    APPEND_ERROR(upp::ascii_error{});
                }),
            },
            {
                .sequence       = {0x40_char, 0x25_char, 0xA0_char, 0x5E_char, 0x7E_char},
                .expected_error = {.valid_up_to = 2, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x40\x25\x1A\x5E\x7E"_s,
                AS_UTF_LOSSY("\u0040\u0025\uFFFD\u005E\u007E"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0040\u0025"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u005E\u007E"sv);
                }),
            },
            {
                .sequence       = {0x51_char, 0xE2_char, 0x52_char, 0x34_char},
                .expected_error = {.valid_up_to = 1, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x51\x1A\x52\x34"_s,
                AS_UTF_LOSSY("\u0051\uFFFD\u0052\u0034"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0051"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u0052\u0034"sv);
                }),
            },
            {
                .sequence       = {0x38_char, 0x2A_char, 0xFF_char, 0xFF_char, 0x28_char},
                .expected_error = {.valid_up_to = 2, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x38\x2A\x1A\x1A\x28"_s,
                AS_UTF_LOSSY("\u0038\u002A\uFFFD\uFFFD\u0028"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0038\u002A"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u0028"sv);
                }),
            },
            {
                .sequence       = {0x4D_char, 0x2D_char, 0x9B_char, 0x5F_char},
                .expected_error = {.valid_up_to = 2, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x4D\x2D\x1A\x5F"_s,
                AS_UTF_LOSSY("\u004D\u002D\uFFFD\u005F"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u004D\u002D"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u005F"sv);
                }),
            },
            {
                .sequence       = {0x58_char, 0x80_char, 0x59_char, 0x80_char, 0x5A_char},
                .expected_error = {.valid_up_to = 1, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x58\x1A\x59\x1A\x5A"_s,
                AS_UTF_LOSSY("\u0058\uFFFD\u0059\uFFFD\u005A"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0058"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u0059"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u005A"sv);
                }),
            },
            {
                .sequence       = {0x1A_char, 0x83_char, 0x25_char, 0x61_char, 0xE1_char, 0x1A_char, 0x1A_char},
                .expected_error = {.valid_up_to = 1, .error = upp::ascii_error{}},
                .as_ascii_lossy = "\x1A\x1A\x25\x61\x1A\x1A\x1A"_s,
                AS_UTF_LOSSY("\u001A\uFFFD\u0025\u0061\uFFFD\u001A\u001A"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u001A"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u0025\u0061"sv);
                    APPEND_ERROR(upp::ascii_error{});
                    APPEND_STRING_LITERAL("\u001A\u001A"sv);
                }),
            },
        });

#undef AS_UTF_LOSSY
#undef EXPECTED_UTF_TRANSCODING_WITH_ERRORS
#undef APPEND_STRING_LITERAL
#undef APPEND_ERROR
    }
} // namespace upp_test::ascii

#endif // TEST_ASCII_HPP