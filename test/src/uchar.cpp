#include <catch2/catch_test_macros.hpp>

#include <uni-cpp/all.hpp>

#include <type_traits>
#include <array>
#include <utility>

TEST_CASE("upp::ascii_char type traits", "[upp::ascii_char]")
{
    STATIC_CHECK_FALSE(std::is_trivially_default_constructible_v<upp::ascii_char>);
    STATIC_CHECK(std::is_trivially_copy_constructible_v<upp::ascii_char>);
    STATIC_CHECK(std::is_trivially_copy_assignable_v<upp::ascii_char>);
    STATIC_CHECK(std::is_trivially_destructible_v<upp::ascii_char>);
}

TEST_CASE("upp::ascii_char user-defined literals", "[upp::ascii_char]")
{
    using namespace upp::literals;

    CHECK(u8'A'_ac.value() == 0x41);
    CHECK((0x1A_ac).value() == 0x1A);

    CHECK(u8'\u0000'_ac.value() == 0x00);
    CHECK(u8'\u007F'_ac.value() == 0x7F);

    CHECK((0x00_ac).value() == 0x00);
    CHECK((0x7F_ac).value() == 0x7F);
}

TEST_CASE("upp::ascii_char default constructor", "[upp::ascii_char]")
{
    upp::ascii_char ch;
    CHECK(ch.value() == 0);
}

TEST_CASE("upp::ascii_char from(), from_lossy() & from_unchecked()", "[upp::ascii_char]")
{
    std::array<std::pair<std::uint8_t, bool>, 6> tests = {
        // {value, is_valid} pairs
        // clang-format off
        std::make_pair(std::uint8_t{0x00}, true ),
        std::make_pair(std::uint8_t{0x41}, true ),
        std::make_pair(std::uint8_t{0x7F}, true ),
        std::make_pair(std::uint8_t{0x80}, false),
        std::make_pair(std::uint8_t{0xA2}, false),
        std::make_pair(std::uint8_t{0xFF}, false),
        // clang-format on
    };

    for (const auto& test : tests)
    {
        const auto& value    = test.first;
        const auto& is_valid = test.second;

        std::optional<upp::ascii_char> from_result = upp::ascii_char::from(value);

        CHECK(from_result.has_value() == is_valid);

        if (is_valid)
        {
            CHECK(upp::ascii_char::from_unchecked(value).value() == value);
            CHECK(upp::ascii_char::from_lossy(value).value() == value);
        }
        else
            CHECK(upp::ascii_char::from_lossy(value) == upp::ascii_char::substitute_character());

        if (from_result.has_value())
            CHECK(from_result.value().value() == value);
    }
}

TEST_CASE("upp::ascii_char comparison operators", "[upp::ascii_char]")
{
    using namespace upp::literals;

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

TEST_CASE("upp::uchar type traits", "[upp::uchar]")
{
    STATIC_CHECK_FALSE(std::is_trivially_default_constructible_v<upp::uchar>);
    STATIC_CHECK(std::is_trivially_copy_constructible_v<upp::uchar>);
    STATIC_CHECK(std::is_trivially_copy_assignable_v<upp::uchar>);
    STATIC_CHECK(std::is_trivially_destructible_v<upp::uchar>);
}

TEST_CASE("upp::uchar user-defined literals", "[upp::uchar]")
{
    using namespace upp::literals;

    CHECK(U'A'_uc.value() == 0x41);
    CHECK((0xFFFD_uc).value() == 0xFFFD);

    CHECK(U'\u0000'_uc.value() == 0x0000);
    CHECK(U'\uD7FF'_uc.value() == 0xD7FF);
    CHECK(U'\uE000'_uc.value() == 0xE000);
    CHECK(U'\U0010FFFF'_uc.value() == 0x0010FFFF);

    CHECK((0x0000_uc).value() == 0x0000);
    CHECK((0xD7FF_uc).value() == 0xD7FF);
    CHECK((0xE000_uc).value() == 0xE000);
    CHECK((0x10FFFF_uc).value() == 0x0010FFFF);
}

TEST_CASE("upp::uchar default constructor", "[upp::uchar]")
{
    upp::uchar ch;
    CHECK(ch.value() == 0);
}

TEST_CASE("upp::uchar from(), from_lossy() & from_unchecked()", "[upp::uchar]")
{
    std::array<std::pair<std::uint32_t, bool>, 12> tests = {
        // {value, is_valid} pairs
        // clang-format off
        std::make_pair(0x00000000U, true ),
        std::make_pair(0x00007022U, true ),
        std::make_pair(0x0000D7FFU, true ),
        std::make_pair(0x0000D800U, false),
        std::make_pair(0x0000DEBAU, false),
        std::make_pair(0x0000DFFFU, false),
        std::make_pair(0x0000E000U, true ),
        std::make_pair(0x0005AEFDU, true ),
        std::make_pair(0x0010FFFFU, true ),
        std::make_pair(0x00110000U, false),
        std::make_pair(0x00201330U, false),
        std::make_pair(0xFFFFFFFFU, false),
        // clang-format on
    };

    for (const auto& test : tests)
    {
        const auto& value    = test.first;
        const auto& is_valid = test.second;

        std::optional<upp::uchar> from_result = upp::uchar::from(value);

        CHECK(from_result.has_value() == is_valid);

        if (is_valid)
        {
            CHECK(upp::uchar::from_unchecked(value).value() == value);
            CHECK(upp::uchar::from_lossy(value).value() == value);
        }
        else
            CHECK(upp::uchar::from_lossy(value) == upp::uchar::replacement_character());

        if (from_result.has_value())
            CHECK(from_result.value().value() == value);
    }
}

TEST_CASE("upp::uchar comparison operators", "[upp::uchar]")
{
    using namespace upp::literals;

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

TEST_CASE("upp::uchar is_ascii() & as_ascii()", "[upp::uchar]")
{
    using namespace upp::literals;

    CHECK(U'a'_uc.is_ascii());
    CHECK((0x7F_uc).is_ascii());
    CHECK_FALSE((0x80_uc).is_ascii());
    CHECK_FALSE((0xFFFD_uc).is_ascii());

    CHECK_FALSE((0x80_uc).as_ascii().has_value());
    CHECK_FALSE((0xFFFD_uc).as_ascii().has_value());

    REQUIRE(U'a'_uc.as_ascii().has_value());
    CHECK(U'a'_uc.value() == static_cast<std::uint32_t>(U'a'_uc.as_ascii().value().value()));

    REQUIRE((0x7F_uc).as_ascii().has_value());
    CHECK((0x7F_uc).value() == static_cast<std::uint32_t>((0x7F_uc).as_ascii().value().value()));
}

TEST_CASE("upp::uchar length_utf8() & length_utf16()", "[upp::uchar]")
{
    using namespace upp::literals;

    CHECK(U'\U00000000'_uc.length_utf8() == 1);
    CHECK(U'\U0000007F'_uc.length_utf8() == 1);
    CHECK(U'\U00000080'_uc.length_utf8() == 2);
    CHECK(U'\U000007FF'_uc.length_utf8() == 2);
    CHECK(U'\U00000800'_uc.length_utf8() == 3);
    CHECK(U'\U0000FFFF'_uc.length_utf8() == 3);
    CHECK(U'\U00010000'_uc.length_utf8() == 4);
    CHECK(U'\U0010FFFF'_uc.length_utf8() == 4);

    CHECK(U'\U00000000'_uc.length_utf16() == 1);
    CHECK(U'\U0000FFFF'_uc.length_utf16() == 1);
    CHECK(U'\U00010000'_uc.length_utf16() == 2);
    CHECK(U'\U0010FFFF'_uc.length_utf16() == 2);
}
