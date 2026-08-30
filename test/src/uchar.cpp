#include "catch2.hpp"

#include <uni-cpp/uchar.hpp>

#include "test_data.hpp"
#include "ranges/base.hpp"

#include <type_traits>
#include <array>
#include <utility>

TEST_CASE("upp::ascii_char type traits", "[upp::ascii_char]")
{
    STATIC_CHECK(!std::is_trivially_default_constructible_v<upp::ascii_char>);
    STATIC_CHECK(std::is_trivially_copy_constructible_v<upp::ascii_char>);
    STATIC_CHECK(std::is_trivially_copy_assignable_v<upp::ascii_char>);
    STATIC_CHECK(std::is_trivially_destructible_v<upp::ascii_char>);
}

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

TEST_CASE("upp::ascii_char default constructor", "[upp::ascii_char]")
{
    upp::ascii_char ch;
    CHECK(ch.value() == 0);
}

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

TEST_CASE("upp::uchar type traits", "[upp::uchar]")
{
    STATIC_CHECK(!std::is_trivially_default_constructible_v<upp::uchar>);
    STATIC_CHECK(std::is_trivially_copy_constructible_v<upp::uchar>);
    STATIC_CHECK(std::is_trivially_copy_assignable_v<upp::uchar>);
    STATIC_CHECK(std::is_trivially_destructible_v<upp::uchar>);
}

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

TEST_CASE("upp::uchar default constructor", "[upp::uchar]")
{
    upp::uchar ch;
    CHECK(ch.value() == 0U);
}

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

TEST_CASE("upp::uchar composition()", "[upp::uchar]")
{
    const auto test_data = upp_test::load_test_data<std::uint32_t, std::uint64_t>("test_data/composition_mappings.txt");

    for (const auto& [key, composition_vec] : test_data)
    {
        const std::uint32_t code_point1_int = static_cast<std::uint32_t>(key & ((1ull << 21u) - 1u));
        const std::uint32_t code_point2_int = static_cast<std::uint32_t>(key >> 21u);

        REQUIRE(upp::is_valid_usv(code_point1_int));
        REQUIRE(upp::is_valid_usv(code_point2_int));

        const auto code_point1 = upp::uchar::from_unchecked(code_point1_int);
        const auto code_point2 = upp::uchar::from_unchecked(code_point2_int);

        REQUIRE((composition_vec.size() == 1uz && upp::is_valid_usv(composition_vec.front())));

        const auto expected_composition = upp::uchar::from_unchecked(composition_vec.front());

        CHECK(upp::uchar::composition(code_point1, code_point2) == expected_composition);
    }
}

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

TEST_CASE("upp::uchar full_decomposition() & full_compatibility_decomposition()", "[upp::uchar]")
{
    const auto canonical_data = upp_test::load_test_data<std::uint32_t>("test_data/full_canonical_decomposition.txt");

    for (const auto& [code_point, expected] : canonical_data)
    {
        REQUIRE(upp::is_valid_usv(code_point));

        const upp::uchar ch = upp::uchar::from_unchecked(code_point);

        CHECK(upp_test::ranges::equal(ch.full_decomposition() | std::views::transform([](upp::uchar c) { return c.value(); }), expected));
    }

    const auto compatibility_data = upp_test::load_test_data<std::uint32_t>("test_data/full_compatibility_decomposition.txt");

    for (const auto& [code_point, expected] : compatibility_data)
    {
        REQUIRE(upp::is_valid_usv(code_point));

        const upp::uchar ch = upp::uchar::from_unchecked(code_point);

        CHECK(
            upp_test::ranges::equal(ch.full_compatibility_decomposition() | std::views::transform([](upp::uchar c) { return c.value(); }), expected));
    }
}

TEST_CASE("upp::uchar decomposition_type()", "[upp::uchar]")
{
    const auto data = upp_test::load_test_data<std::uint8_t>("test_data/decomposition_type.txt");

    for (const auto& [code_point, expected] : data)
    {
        REQUIRE(upp::is_valid_usv(code_point));

        const upp::uchar ch = upp::uchar::from_unchecked(code_point);

        REQUIRE(expected.size() == 1uz);

        const auto result = ch.decomposition_type();

        if (expected.front() == 0)
        {
            CHECK(!result.has_value());
        }
        else
        {
            REQUIRE(result.has_value());
            CHECK(std::to_underlying(*result) == expected.front()); // NOLINT(bugprone-unchecked-optional-access)
        }
    }
}

TEST_CASE("upp::uchar canonical_combining_class()", "[upp::uchar]")
{
    CONSTEXPR_AND_RUNTIME_TEST()
    {
        using namespace upp::char_literals;

        struct test_case
        {
            upp::uchar   code_point;
            std::uint8_t ccc;
        };

        const auto test_cases = std::to_array<test_case>(
            {{.code_point = 0x0061_uc, .ccc = 0},
             {.code_point = 0x0300_uc, .ccc = 230},
             {.code_point = 0x0301_uc, .ccc = 230},
             {.code_point = 0x0302_uc, .ccc = 230},
             {.code_point = 0x0323_uc, .ccc = 220},
             {.code_point = 0x0315_uc, .ccc = 232},
             {.code_point = 0x0345_uc, .ccc = 240},
             {.code_point = 0x05B0_uc, .ccc = 10},
             {.code_point = 0x05B1_uc, .ccc = 11},
             {.code_point = 0x05B2_uc, .ccc = 12},
             {.code_point = 0x093C_uc, .ccc = 7}});

        for (auto [code_point, ccc] : test_cases)
        {
            CRTT_CHECK(code_point.canonical_combining_class() == ccc);
        }
    };
}

TEST_CASE("upp::uchar general_category()", "[upp::uchar]")
{
    const auto data = upp_test::load_test_data<std::uint8_t>("test_data/general_category.txt");

    for (const auto& [code_point, expected] : data)
    {
        REQUIRE(upp::is_valid_usv(code_point));

        const upp::uchar ch = upp::uchar::from_unchecked(code_point);

        REQUIRE(expected.size() == 1uz);

        CHECK(std::to_underlying(ch.general_category()) == expected.front());
    }
}