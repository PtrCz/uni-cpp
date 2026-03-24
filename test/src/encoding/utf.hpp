#ifndef TEST_UTF_HPP
#define TEST_UTF_HPP

#include <uni-cpp/encoding.hpp>
#include <uni-cpp/string.hpp>

#include <array>
#include <string>
#include <string_view>

#include "macros.hpp"
#include "../utility.hpp"

namespace upp_test::utf
{
    struct utf_sequences
    {
        std::u8string  utf8_seq;
        std::u16string utf16_seq;
        std::u32string utf32_seq;

        template<upp::encoding Encoding, typename Self>
            requires upp::unicode_encoding<Encoding>
        [[nodiscard]] constexpr auto&& encoded_as(this Self&& self) noexcept
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
        using namespace upp_test::string_literals;

// Use the languages ability to transcode the string literal to different encodings for us.
#define VALID_UTF_SEQ(literal)                                                                \
    utf_sequences                                                                             \
    {                                                                                         \
        .utf8_seq = u8##literal##_s, .utf16_seq = u##literal##_s, .utf32_seq = U##literal##_s \
    }

        return std::to_array<utf_sequences>(
            {VALID_UTF_SEQ(""), VALID_UTF_SEQ("a"), VALID_UTF_SEQ("\U0001FCCC"), VALID_UTF_SEQ("\u0041\u0062\u007A\u00A9\u00F1"),
             VALID_UTF_SEQ("\u03A9\u20AC\u221E"), VALID_UTF_SEQ("\u0065\u0301\u006E\u0303\u0061\u0308"), VALID_UTF_SEQ("\u00E9\u0065\u0301"),
             VALID_UTF_SEQ("\u200B\u200C\u200D\u2060\uFEFF"), VALID_UTF_SEQ("\U0001F600\U0001F642"), VALID_UTF_SEQ("\uFDD0\uFDEF\U0001FFFE"),
             VALID_UTF_SEQ("\uFFF0\uFFFD\uFFE8")});

#undef VALID_UTF_SEQ
    }

    template<upp::encoding Encoding>
    class invalid_sequence
    {
    private:
        using string_type     = std::basic_string<typename upp::encoding_traits<Encoding>::default_code_unit_type>;
        using error_type      = upp::encoding_traits<Encoding>::error_type;
        using from_error_type = upp::encoding_traits<Encoding>::from_error_type;

    public:
        string_type     sequence;
        from_error_type expected_error;

        std::u8string  as_utf8_lossy;
        std::u16string as_utf16_lossy;
        std::u32string as_utf32_lossy;

        std::vector<std::expected<char8_t, error_type>>  expected_utf8_transcoding_with_errors;
        std::vector<std::expected<char16_t, error_type>> expected_utf16_transcoding_with_errors;
        std::vector<std::expected<char32_t, error_type>> expected_utf32_transcoding_with_errors;

        template<upp::encoding TargetEncoding, typename Self>
            requires upp::unicode_encoding<TargetEncoding>
        [[nodiscard]] constexpr auto&& lossily_encoded_as(this Self&& self) noexcept
        {
            if constexpr (TargetEncoding == upp::encoding::utf8)
                return std::forward<Self>(self).as_utf8_lossy;
            else if constexpr (TargetEncoding == upp::encoding::utf16)
                return std::forward<Self>(self).as_utf16_lossy;
            else if constexpr (TargetEncoding == upp::encoding::utf32)
                return std::forward<Self>(self).as_utf32_lossy;
        }

        template<upp::encoding TargetEncoding, typename Self>
            requires upp::unicode_encoding<TargetEncoding>
        [[nodiscard]] constexpr auto&& transcoded_with_errors_to(this Self&& self) noexcept
        {
            if constexpr (TargetEncoding == upp::encoding::utf8)
                return std::forward<Self>(self).expected_utf8_transcoding_with_errors;
            else if constexpr (TargetEncoding == upp::encoding::utf16)
                return std::forward<Self>(self).expected_utf16_transcoding_with_errors;
            else if constexpr (TargetEncoding == upp::encoding::utf32)
                return std::forward<Self>(self).expected_utf32_transcoding_with_errors;
        }
    };

// Use the languages ability to transcode the string literal to different encodings for us.
#define AS_UTF_LOSSY(literal) .as_utf8_lossy = u8##literal##_s, .as_utf16_lossy = u##literal##_s, .as_utf32_lossy = U##literal##_s

#define EXPECTED_UTF_TRANSCODING_WITH_ERRORS(...) TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS(upp::encoding::utf8, __VA_ARGS__)

#define APPEND_STRING_LITERAL(literal) TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_APPEND_STRING_LITERAL(upp::utf8_error, literal)
#define APPEND_ERROR(...)              TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_APPEND_ERROR(upp::utf8_error, __VA_ARGS__)

    [[nodiscard]] constexpr auto invalid_utf8_sequences()
    {
        using namespace upp_test::string_literals;

        using invalid_seq = invalid_sequence<upp::encoding::utf8>;

        return std::to_array<invalid_seq>({
            {
                .sequence       = {0x8F}, // Too Long (0 bytes)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS(
                    { APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte}); }),
            },
            {
                .sequence       = {0x39, 0x8C}, // Too Long (1 byte)
                .expected_error = {.valid_up_to = 1, .error = {.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte}},
                AS_UTF_LOSSY("\u0039\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0039"sv);
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xC6, 0x84, 0x98}, // Too Long (2 bytes)
                .expected_error = {.valid_up_to = 2, .error = {.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte}},
                AS_UTF_LOSSY("\u0184\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0184"sv);
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xE0, 0xA0, 0x8C, 0x9E}, // Too Long (3 bytes)
                .expected_error = {.valid_up_to = 3, .error = {.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte}},
                AS_UTF_LOSSY("\u080C\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u080C"sv);
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xF3, 0xA0, 0x81, 0x88, 0xBC}, // Too Long (4 bytes)
                .expected_error = {.valid_up_to = 4, .error = {.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte}},
                AS_UTF_LOSSY("\u{E0048}\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u{E0048}"sv);
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xF8, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (5 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::invalid_leading_byte}},
                AS_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::invalid_leading_byte});

                    for (auto i = 0uz; i < 4; ++i)
                        APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xFC, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (6 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::invalid_leading_byte}},
                AS_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::invalid_leading_byte});

                    for (auto i = 0uz; i < 5; ++i)
                        APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xFE, 0x80, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (7 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::invalid_leading_byte}},
                AS_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::invalid_leading_byte});

                    for (auto i = 0uz; i < 6; ++i)
                        APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xFF, 0x80, 0x80, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (8 bytes, U+E0061)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::invalid_leading_byte}},
                AS_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::invalid_leading_byte});

                    for (auto i = 0uz; i < 7; ++i)
                        APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xC4}, // Too Short (at the end, should be 2-byte long, 1 byte missing, U+0104)
                .expected_error = {.valid_up_to = 0, .error = {.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS(
                    { APPEND_ERROR(upp::utf8_error{.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}); }),
            },
            {
                .sequence       = {0xC4, u8'A'}, // Too Short (in the middle, should be 2-byte long, 1 byte missing, U+0104)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD\u0041"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::truncated_sequence});
                    APPEND_STRING_LITERAL("\u0041"sv);
                }),
            },
            {
                .sequence       = {0xE1}, // Too Short (at the end, should be 3-byte long, 2 bytes missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error = {.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS(
                    { APPEND_ERROR(upp::utf8_error{.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}); }),
            },
            {
                .sequence       = {0xE1, u8'0'}, // Too Short (in the middle, should be 3-byte long, 2 bytes missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD\u0030"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::truncated_sequence});
                    APPEND_STRING_LITERAL("\u0030"sv);
                }),
            },
            {
                .sequence       = {0xE1, 0x83}, // Too Short (at the end, should be 3-byte long, 1 byte missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error = {.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS(
                    { APPEND_ERROR(upp::utf8_error{.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}); }),
            },
            {
                .sequence       = {0xE1, 0x83, u8' '}, // Too Short (in the middle, should be 3-byte long, 1 byte missing, U+10C4)
                .expected_error = {.valid_up_to = 0, .error = {.length = 2, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD\u0020"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 2, .code = upp::utf8_error_code::truncated_sequence});
                    APPEND_STRING_LITERAL("\u0020"sv);
                }),
            },
            {
                .sequence       = {0xF3}, // Too Short (at the end, should be 4-byte long, 3 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error = {.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS(
                    { APPEND_ERROR(upp::utf8_error{.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}); }),
            },
            {
                .sequence       = {0xF3, u8' '}, // Too Short (in the middle, should be 4-byte long, 3 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD\u0020"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::truncated_sequence});
                    APPEND_STRING_LITERAL("\u0020"sv);
                }),
            },
            {
                .sequence       = {0xF3, 0xA0}, // Too Short (at the end, should be 4-byte long, 2 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error = {.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS(
                    { APPEND_ERROR(upp::utf8_error{.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}); }),
            },
            {
                .sequence       = {0xF3, 0xA0, u8' '}, // Too Short (in the middle, should be 4-byte long, 2 bytes missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error = {.length = 2, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD\u0020"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 2, .code = upp::utf8_error_code::truncated_sequence});
                    APPEND_STRING_LITERAL("\u0020"sv);
                }),
            },
            {
                .sequence       = {0xF3, 0xA0, 0x86}, // Too Short (at the end, should be 4-byte long, 1 byte missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error = {.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS(
                    { APPEND_ERROR(upp::utf8_error{.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence}); }),
            },
            {
                .sequence       = {0xF3, 0xA0, 0x86, u8' '}, // Too Short (in the middle, should be 4-byte long, 1 byte missing, U+E0198)
                .expected_error = {.valid_up_to = 0, .error = {.length = 3, .code = upp::utf8_error_code::truncated_sequence}},
                AS_UTF_LOSSY("\uFFFD\u0020"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 3, .code = upp::utf8_error_code::truncated_sequence});
                    APPEND_STRING_LITERAL("\u0020"sv);
                }),
            },
            {
                .sequence       = {0xC0, 0xB6}, // Overlong (2-byte long, U+0036)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::invalid_leading_byte}},
                AS_UTF_LOSSY("\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::invalid_leading_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xE0, 0x97, 0x9F}, // Overlong (3-byte long, U+05DF)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::overlong}},
                AS_UTF_LOSSY("\uFFFD\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::overlong});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xF0, 0x8F, 0xAC, 0xB4}, // Overlong (4-byte long, U+FB34)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::overlong}},
                AS_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::overlong});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xF7, 0x8C, 0x9A, 0x8D}, // Too Large (detectable at 1st byte)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::invalid_leading_byte}},
                AS_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::invalid_leading_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xF4, 0x90, 0x91, 0xB1}, // Too Large (detectable at 2nd byte)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::out_of_range}},
                AS_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::out_of_range});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0xED, 0xA1, 0xB4}, // Surrogate (0xD874)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::encoded_surrogate}},
                AS_UTF_LOSSY("\uFFFD\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::encoded_surrogate});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                }),
            },
            {
                .sequence       = {0x20, 0xFF, 0xD0, 0x9F, 0xD1, 0x80, 0x4E, 0xD0, 0xB8, 0xE0, 0x97, 0x9F, 0xD0},
                .expected_error = {.valid_up_to = 1, .error = {.length = 1, .code = upp::utf8_error_code::invalid_leading_byte}},
                AS_UTF_LOSSY("\u0020\uFFFD\u041F\u0440\u004E\u0438\uFFFD\uFFFD\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0020"sv);
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::invalid_leading_byte});
                    APPEND_STRING_LITERAL("\u041F\u0440\u004E\u0438"sv);
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::overlong});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = std::nullopt, .code = upp::utf8_error_code::truncated_sequence});
                }),
            },
            {
                .sequence       = {0xF4, 0xB0, 0x94, 0x82, 0xE6, 0xBC, 0xA2, 0xED, 0xA1, 0xB4, 0x30, 0xE2, 0x97, 0x00},
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf8_error_code::out_of_range}},
                AS_UTF_LOSSY("\uFFFD\uFFFD\uFFFD\uFFFD\u6F22\uFFFD\uFFFD\uFFFD\u0030\uFFFD\u0000"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::out_of_range});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_STRING_LITERAL("\u6F22"sv);
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::encoded_surrogate});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_ERROR(upp::utf8_error{.length = 1, .code = upp::utf8_error_code::unexpected_continuation_byte});
                    APPEND_STRING_LITERAL("\u0030"sv);
                    APPEND_ERROR(upp::utf8_error{.length = 2, .code = upp::utf8_error_code::truncated_sequence});
                    APPEND_STRING_LITERAL("\u0000"sv);
                }),
            },
        });
    }

#undef EXPECTED_UTF_TRANSCODING_WITH_ERRORS
#undef APPEND_STRING_LITERAL
#undef APPEND_ERROR

#define EXPECTED_UTF_TRANSCODING_WITH_ERRORS(...) TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS(upp::encoding::utf16, __VA_ARGS__)

#define APPEND_STRING_LITERAL(literal) TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_APPEND_STRING_LITERAL(upp::utf16_error, literal)
#define APPEND_ERROR(...)              TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_APPEND_ERROR(upp::utf16_error, __VA_ARGS__)

    [[nodiscard]] constexpr auto invalid_utf16_sequences()
    {
        using namespace upp_test::string_literals;

        using invalid_seq = invalid_sequence<upp::encoding::utf16>;

        return std::to_array<invalid_seq>({
            {
                .sequence       = {0xD800}, // Isolated high surrogate (at the end)
                .expected_error = {.valid_up_to = 0, .error = {.length = std::nullopt, .code = upp::utf16_error_code::unpaired_high_surrogate}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS(
                    { APPEND_ERROR(upp::utf16_error{.length = std::nullopt, .code = upp::utf16_error_code::unpaired_high_surrogate}); }),
            },
            {
                .sequence       = {0xD800, u'A'}, // Isolated high surrogate (in the middle)
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf16_error_code::unpaired_high_surrogate}},
                AS_UTF_LOSSY("\uFFFD\u0041"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf16_error{.length = 1, .code = upp::utf16_error_code::unpaired_high_surrogate});
                    APPEND_STRING_LITERAL("\u0041"sv);
                }),
            },
            {
                .sequence       = {0xDC00}, // Isolated low surrogate
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf16_error_code::unpaired_low_surrogate}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS(
                    { APPEND_ERROR(upp::utf16_error{.length = 1, .code = upp::utf16_error_code::unpaired_low_surrogate}); }),
            },
            {
                .sequence       = {u'A', 0xDC00}, // Isolated low surrogate
                .expected_error = {.valid_up_to = 1, .error = {.length = 1, .code = upp::utf16_error_code::unpaired_low_surrogate}},
                AS_UTF_LOSSY("\u0041\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0041"sv);
                    APPEND_ERROR(upp::utf16_error{.length = 1, .code = upp::utf16_error_code::unpaired_low_surrogate});
                }),
            },
            {
                .sequence       = {0xDC00, 0xD800}, // Reversed surrogate pair
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf16_error_code::unpaired_low_surrogate}},
                AS_UTF_LOSSY("\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf16_error{.length = 1, .code = upp::utf16_error_code::unpaired_low_surrogate});
                    APPEND_ERROR(upp::utf16_error{.length = std::nullopt, .code = upp::utf16_error_code::unpaired_high_surrogate});
                }),
            },
            {
                .sequence       = {0xD800, 0xD801}, // High surrogate followed by high surrogate
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf16_error_code::unpaired_high_surrogate}},
                AS_UTF_LOSSY("\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf16_error{.length = 1, .code = upp::utf16_error_code::unpaired_high_surrogate});
                    APPEND_ERROR(upp::utf16_error{.length = std::nullopt, .code = upp::utf16_error_code::unpaired_high_surrogate});
                }),
            },
            {
                .sequence       = {0xD83D, 0xDE00, 0xDC00}, // Valid surrogate pair followed by an isolated low surrogate
                .expected_error = {.valid_up_to = 2, .error = {.length = 1, .code = upp::utf16_error_code::unpaired_low_surrogate}},
                AS_UTF_LOSSY("\u{1F600}\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u{1F600}"sv);
                    APPEND_ERROR(upp::utf16_error{.length = 1, .code = upp::utf16_error_code::unpaired_low_surrogate});
                }),
            },
            {
                .sequence       = {0xD850, 0xEF00, 0xD8FF, 0xDEAD, 0x0350, 0xDF00, 0xD9FD},
                .expected_error = {.valid_up_to = 0, .error = {.length = 1, .code = upp::utf16_error_code::unpaired_high_surrogate}},
                AS_UTF_LOSSY("\uFFFD\uEF00\U0004FEAD\u0350\uFFFD\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf16_error{.length = 1, .code = upp::utf16_error_code::unpaired_high_surrogate});
                    APPEND_STRING_LITERAL("\uEF00\U0004FEAD\u0350"sv);
                    APPEND_ERROR(upp::utf16_error{.length = 1, .code = upp::utf16_error_code::unpaired_low_surrogate});
                    APPEND_ERROR(upp::utf16_error{.length = std::nullopt, .code = upp::utf16_error_code::unpaired_high_surrogate});
                }),
            },
        });
    }

#undef EXPECTED_UTF_TRANSCODING_WITH_ERRORS
#undef APPEND_STRING_LITERAL
#undef APPEND_ERROR

#define EXPECTED_UTF_TRANSCODING_WITH_ERRORS(...) TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS(upp::encoding::utf32, __VA_ARGS__)

#define APPEND_STRING_LITERAL(literal) TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_APPEND_STRING_LITERAL(upp::utf32_error, literal)
#define APPEND_ERROR(...)              TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_APPEND_ERROR(upp::utf32_error, __VA_ARGS__)

    [[nodiscard]] constexpr auto invalid_utf32_sequences()
    {
        using namespace upp_test::string_literals;

        using invalid_seq = invalid_sequence<upp::encoding::utf32>;

        return std::to_array<invalid_seq>({
            {
                .sequence       = {0xD800}, // Surrogate (high surrogate range)
                .expected_error = {.valid_up_to = 0, .error = {.code = upp::utf32_error_code::encoded_surrogate}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({ APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::encoded_surrogate}); }),
            },
            {
                .sequence       = {0xDFFF}, // Surrogate (low surrogate range)
                .expected_error = {.valid_up_to = 0, .error = {.code = upp::utf32_error_code::encoded_surrogate}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({ APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::encoded_surrogate}); }),
            },
            {
                .sequence       = {0x110000}, // Too Large
                .expected_error = {.valid_up_to = 0, .error = {.code = upp::utf32_error_code::out_of_range}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({ APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::out_of_range}); }),
            },
            {
                .sequence       = {0xFFFFFFFF}, // Too Large
                .expected_error = {.valid_up_to = 0, .error = {.code = upp::utf32_error_code::out_of_range}},
                AS_UTF_LOSSY("\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({ APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::out_of_range}); }),
            },
            {
                .sequence       = {0x0041, 0xD834}, // Valid code point followed by a surrogate
                .expected_error = {.valid_up_to = 1, .error = {.code = upp::utf32_error_code::encoded_surrogate}},
                AS_UTF_LOSSY("\u0041\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u0041"sv);
                    APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::encoded_surrogate});
                }),
            },
            {
                .sequence       = {0x1F600, 0x110000}, // Valid code point followed by an out-of-range value
                .expected_error = {.valid_up_to = 1, .error = {.code = upp::utf32_error_code::out_of_range}},
                AS_UTF_LOSSY("\u{1F600}\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u{1F600}"sv);
                    APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::out_of_range});
                }),
            },
            {
                .sequence       = {0x053F, 0x03AB, 0xD87F, 0xDF4F, 0x10FAC6, 0x3DCA, 0x1BD000},
                .expected_error = {.valid_up_to = 2, .error = {.code = upp::utf32_error_code::encoded_surrogate}},
                AS_UTF_LOSSY("\u053F\u03AB\uFFFD\uFFFD\U0010FAC6\u3DCA\uFFFD"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_STRING_LITERAL("\u053F\u03AB"sv);
                    APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::encoded_surrogate});
                    APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::encoded_surrogate});
                    APPEND_STRING_LITERAL("\U0010FAC6\u3DCA"sv);
                    APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::out_of_range});
                }),
            },
            {
                .sequence       = {0xD85F, 0x00040000, 0x20FA, 0x12FABC, 0x5F00},
                .expected_error = {.valid_up_to = 0, .error = {.code = upp::utf32_error_code::encoded_surrogate}},
                AS_UTF_LOSSY("\uFFFD\U00040000\u20FA\uFFFD\u5F00"),
                EXPECTED_UTF_TRANSCODING_WITH_ERRORS({
                    APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::encoded_surrogate});
                    APPEND_STRING_LITERAL("\U00040000\u20FA"sv);
                    APPEND_ERROR(upp::utf32_error{.code = upp::utf32_error_code::out_of_range});
                    APPEND_STRING_LITERAL("\u5F00"sv);
                }),
            },
        });
    }

#undef EXPECTED_UTF_TRANSCODING_WITH_ERRORS
#undef APPEND_STRING_LITERAL
#undef APPEND_ERROR

#undef AS_UTF_LOSSY

    template<upp::encoding Encoding>
        requires upp::unicode_encoding<Encoding>
    [[nodiscard]] constexpr auto invalid_sequences_for_encoding()
    {
        if constexpr (Encoding == upp::encoding::utf8)
            return invalid_utf8_sequences();
        else if constexpr (Encoding == upp::encoding::utf16)
            return invalid_utf16_sequences();
        else if constexpr (Encoding == upp::encoding::utf32)
            return invalid_utf32_sequences();
    }
} // namespace upp_test::utf

#endif // TEST_UTF_HPP