#include "bugspray.hpp"

#include "test_data.hpp"

#include <uni-cpp/uchar.hpp>
#include <ranges>

TEST_CASE("Lowercase conversion & lowercase mappings", "[case conversion][upp::uchar]", runtime)
{
    const auto test_data = upp_test::load_test_data<std::uint32_t>("lowercase_mappings.txt");

    for (const auto& [code_point, data] : test_data)
    {
        const auto ch = upp::uchar::from(code_point);
        REQUIRE(ch.has_value());

        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        const auto expected = ch->to_lowercase() | std::views::transform([](upp::uchar c) { return c.value(); });

        CHECK(std::ranges::equal(expected, data));
    }
}

TEST_CASE("Uppercase conversion & uppercase mappings", "[case conversion][upp::uchar]", runtime)
{
    const auto test_data = upp_test::load_test_data<std::uint32_t>("uppercase_mappings.txt");

    for (const auto& [code_point, data] : test_data)
    {
        const auto ch = upp::uchar::from(code_point);
        REQUIRE(ch.has_value());

        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        const auto expected = ch->to_uppercase() | std::views::transform([](upp::uchar c) { return c.value(); });

        CHECK(std::ranges::equal(expected, data));
    }
}

TEST_CASE("Titlecase conversion & titlecase mappings", "[case conversion][upp::uchar]", runtime)
{
    const auto test_data = upp_test::load_test_data<std::uint32_t>("titlecase_mappings.txt");

    for (const auto& [code_point, data] : test_data)
    {
        const auto ch = upp::uchar::from(code_point);
        REQUIRE(ch.has_value());

        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        const auto expected = ch->to_titlecase() | std::views::transform([](upp::uchar c) { return c.value(); });

        CHECK(std::ranges::equal(expected, data));
    }
}