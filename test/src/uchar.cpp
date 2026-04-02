#include "bugspray.hpp"

#include <uni-cpp/uchar.hpp>

#include <type_traits>
#include <array>
#include <utility>

TEST_CASE("upp::ascii_char type traits", "[upp::ascii_char]")
{
    CHECK_FALSE(std::is_trivially_default_constructible_v<upp::ascii_char>);
    CHECK(std::is_trivially_copy_constructible_v<upp::ascii_char>);
    CHECK(std::is_trivially_copy_assignable_v<upp::ascii_char>);
    CHECK(std::is_trivially_destructible_v<upp::ascii_char>);
}
EVAL_TEST_CASE("upp::ascii_char type traits");

TEST_CASE("upp::ascii_char user-defined literals", "[upp::ascii_char]")
{
    using namespace upp::char_literals;

    CHECK(u8'A'_ac.value() == 0x41);
    CHECK((0x1A_ac).value() == 0x1A);

    CHECK(u8'\u0000'_ac.value() == 0x00);
    CHECK(u8'\u007F'_ac.value() == 0x7F);

    CHECK((0x00_ac).value() == 0x00);
    CHECK((0x7F_ac).value() == 0x7F);
}
EVAL_TEST_CASE("upp::ascii_char user-defined literals");

TEST_CASE("upp::ascii_char default constructor", "[upp::ascii_char]")
{
    upp::ascii_char ch;
    CHECK(ch.value() == 0);
}
EVAL_TEST_CASE("upp::ascii_char default constructor");

TEST_CASE("upp::ascii_char from(), from_lossy() & from_unchecked()", "[upp::ascii_char]")
{
    struct test_case
    {
        std::uint8_t value;
        bool         is_valid;
    };

    const auto test_cases = std::to_array<test_case>({
        {.value = 0x00, .is_valid = true},
        {.value = 0x41, .is_valid = true},
        {.value = 0x7F, .is_valid = true},
        {.value = 0x80, .is_valid = false},
        {.value = 0xA2, .is_valid = false},
        {.value = 0xFF, .is_valid = false},
    });

    for (const auto& [value, is_valid] : test_cases)
    {
        std::expected<upp::ascii_char, upp::ascii_error> from_result = upp::ascii_char::from(value);

        CHECK(from_result.has_value() == is_valid);

        if (is_valid)
        {
            CHECK(upp::ascii_char::from_unchecked(value).value() == value);
            CHECK(upp::ascii_char::from_lossy(value).value() == value);
        }
        else
            CHECK(upp::ascii_char::from_lossy(value) == upp::ascii_char::substitute_character());

        if (from_result.has_value())
            CHECK(from_result->value() == value);
    }
}
EVAL_TEST_CASE("upp::ascii_char from(), from_lossy() & from_unchecked()");

TEST_CASE("upp::ascii_char comparison operators", "[upp::ascii_char]")
{
    using namespace upp::char_literals;

    CHECK(u8'\0'_ac == 0_ac);
    CHECK(u8'\n'_ac == 0xA_ac);
    CHECK(u8'A'_ac != u8'B'_ac);
    CHECK(0x32_ac != u8'\t'_ac);

    CHECK_FALSE(u8'A'_ac == u8'B'_ac);
    CHECK_FALSE(u8'\n'_ac != 0xA_ac);

    CHECK(0x00_ac <= 0x50_ac);
    CHECK(0x00_ac >= 0x00_ac);
    CHECK(u8'Z'_ac >= u8'A'_ac);
    CHECK(u8'Z'_ac > u8'A'_ac);

    CHECK_FALSE(0x00_ac > 0x50_ac);
    CHECK_FALSE(0x00_ac < 0x00_ac);
    CHECK_FALSE(u8'Z'_ac < u8'A'_ac);
    CHECK_FALSE(u8'a'_ac <= u8'A'_ac);
}
EVAL_TEST_CASE("upp::ascii_char comparison operators");

TEST_CASE("upp::uchar type traits", "[upp::uchar]")
{
    CHECK_FALSE(std::is_trivially_default_constructible_v<upp::uchar>);
    CHECK(std::is_trivially_copy_constructible_v<upp::uchar>);
    CHECK(std::is_trivially_copy_assignable_v<upp::uchar>);
    CHECK(std::is_trivially_destructible_v<upp::uchar>);
}
EVAL_TEST_CASE("upp::uchar type traits");

TEST_CASE("upp::uchar user-defined literals", "[upp::uchar]")
{
    using namespace upp::char_literals;

    CHECK(U'A'_uc.value() == 0x41U);
    CHECK((0xFFFD_uc).value() == 0xFFFDU);

    CHECK(U'\u0000'_uc.value() == 0x0000U);
    CHECK(U'\uD7FF'_uc.value() == 0xD7FFU);
    CHECK(U'\uE000'_uc.value() == 0xE000U);
    CHECK(U'\U0010FFFF'_uc.value() == 0x0010FFFFU);

    CHECK((0x0000_uc).value() == 0x0000U);
    CHECK((0xD7FF_uc).value() == 0xD7FFU);
    CHECK((0xE000_uc).value() == 0xE000U);
    CHECK((0x10FFFF_uc).value() == 0x0010FFFFU);
}
EVAL_TEST_CASE("upp::uchar user-defined literals");

TEST_CASE("upp::uchar default constructor", "[upp::uchar]")
{
    upp::uchar ch;
    CHECK(ch.value() == 0U);
}
EVAL_TEST_CASE("upp::uchar default constructor");

TEST_CASE("upp::uchar from(), from_lossy() & from_unchecked()", "[upp::uchar]")
{
    struct test_case
    {
        std::uint32_t                   value;
        bool                            is_valid;
        std::optional<upp::utf32_error> expected_error;
    };

    const auto test_cases = std::to_array<test_case>({
        // clang-format off
        {.value = 0x00000000U, .is_valid = true , .expected_error = std::nullopt},
        {.value = 0x00007022U, .is_valid = true , .expected_error = std::nullopt},
        {.value = 0x0000D7FFU, .is_valid = true , .expected_error = std::nullopt},
        {.value = 0x0000D800U, .is_valid = false, .expected_error = upp::utf32_error{.code = upp::utf32_error_code::encoded_surrogate}},
        {.value = 0x0000DEBAU, .is_valid = false, .expected_error = upp::utf32_error{.code = upp::utf32_error_code::encoded_surrogate}},
        {.value = 0x0000DFFFU, .is_valid = false, .expected_error = upp::utf32_error{.code = upp::utf32_error_code::encoded_surrogate}},
        {.value = 0x0000E000U, .is_valid = true , .expected_error = std::nullopt},
        {.value = 0x0005AEFDU, .is_valid = true , .expected_error = std::nullopt},
        {.value = 0x0010FFFFU, .is_valid = true , .expected_error = std::nullopt},
        {.value = 0x00110000U, .is_valid = false, .expected_error = upp::utf32_error{.code = upp::utf32_error_code::out_of_range}},
        {.value = 0x00201330U, .is_valid = false, .expected_error = upp::utf32_error{.code = upp::utf32_error_code::out_of_range}},
        {.value = 0xFFFFFFFFU, .is_valid = false, .expected_error = upp::utf32_error{.code = upp::utf32_error_code::out_of_range}},
        // clang-format on
    });

    for (const auto& [value, is_valid, expected_error] : test_cases)
    {
        std::expected<upp::uchar, upp::utf32_error> from_result = upp::uchar::from(value);

        CHECK(from_result.has_value() == is_valid);

        if (is_valid)
        {
            CHECK(upp::uchar::from_unchecked(value).value() == value);
            CHECK(upp::uchar::from_lossy(value).value() == value);
        }
        else
            CHECK(upp::uchar::from_lossy(value) == upp::uchar::replacement_character());

        if (from_result.has_value())
        {
            CHECK(from_result->value() == value);
        }
        else if (!is_valid)
        {
            CHECK(from_result.error() == *expected_error); // NOLINT(bugprone-unchecked-optional-access)
        }
    }
}
EVAL_TEST_CASE("upp::uchar from(), from_lossy() & from_unchecked()");

TEST_CASE("upp::uchar comparison operators", "[upp::uchar]")
{
    using namespace upp::char_literals;

    CHECK(U'\0'_uc == 0_uc);
    CHECK(U'\n'_uc == 0xA_uc);
    CHECK(U'A'_uc != U'B'_uc);
    CHECK(0x32_uc != U'\t'_uc);

    CHECK_FALSE(U'A'_uc == U'B'_uc);
    CHECK_FALSE(U'\n'_uc != 0xA_uc);

    CHECK(0x00_uc <= 0x50_uc);
    CHECK(0x00_uc >= 0x00_uc);
    CHECK(U'Z'_uc >= U'A'_uc);
    CHECK(U'Z'_uc > U'A'_uc);

    CHECK_FALSE(0x00_uc > 0x50_uc);
    CHECK_FALSE(0x00_uc < 0x00_uc);
    CHECK_FALSE(U'Z'_uc < U'A'_uc);
    CHECK_FALSE(U'a'_uc <= U'A'_uc);
}
EVAL_TEST_CASE("upp::uchar comparison operators");

TEST_CASE("upp::uchar is_ascii() & as_ascii()", "[upp::uchar]")
{
    using namespace upp::char_literals;

    CHECK(U'a'_uc.is_ascii());
    CHECK((0x7F_uc).is_ascii());
    CHECK_FALSE((0x80_uc).is_ascii());
    CHECK_FALSE((0xFFFD_uc).is_ascii());

    CHECK_FALSE((0x80_uc).as_ascii().has_value());
    CHECK_FALSE((0xFFFD_uc).as_ascii().has_value());

    const auto ascii_a = U'a'_uc.as_ascii();
    REQUIRE(ascii_a.has_value());
    CHECK(U'a'_uc.value() == static_cast<std::uint32_t>(ascii_a->value())); // NOLINT(bugprone-unchecked-optional-access)

    const auto ascii_7f = (0x7F_uc).as_ascii();
    REQUIRE(ascii_7f.has_value());
    CHECK((0x7F_uc).value() == static_cast<std::uint32_t>(ascii_7f->value())); // NOLINT(bugprone-unchecked-optional-access)
}
EVAL_TEST_CASE("upp::uchar is_ascii() & as_ascii()");

TEST_CASE("upp::uchar length_utf8() & length_utf16()", "[upp::uchar]")
{
    using namespace upp::char_literals;

    CHECK(U'\U00000000'_uc.length_utf8() == 1U);
    CHECK(U'\U0000007F'_uc.length_utf8() == 1U);
    CHECK(U'\U00000080'_uc.length_utf8() == 2U);
    CHECK(U'\U000007FF'_uc.length_utf8() == 2U);
    CHECK(U'\U00000800'_uc.length_utf8() == 3U);
    CHECK(U'\U0000FFFF'_uc.length_utf8() == 3U);
    CHECK(U'\U00010000'_uc.length_utf8() == 4U);
    CHECK(U'\U0010FFFF'_uc.length_utf8() == 4U);

    CHECK(U'\U00000000'_uc.length_utf16() == 1U);
    CHECK(U'\U0000FFFF'_uc.length_utf16() == 1U);
    CHECK(U'\U00010000'_uc.length_utf16() == 2U);
    CHECK(U'\U0010FFFF'_uc.length_utf16() == 2U);
}
EVAL_TEST_CASE("upp::uchar length_utf8() & length_utf16()");
