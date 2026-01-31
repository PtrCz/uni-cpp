#include "bugspray.hpp"

#include "test_data.hpp"

#include <uni-cpp/uchar.hpp>
#include <cstdint>
#include <ranges>

TEST_CASE("UTF-8 encoding", "[UTF encoding][upp::uchar]", runtime)
{
    const auto test_data = upp_test::load_test_data<std::uint8_t>("utf_8_encoding.txt");

    for (const auto& [code_point, data] : test_data)
    {
        const auto ch = upp::uchar::from(code_point);
        REQUIRE(ch.has_value());

        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        CHECK(std::ranges::equal(ch->encode_utf8(), data));
    }
}

TEST_CASE("UTF-16 encoding", "[UTF encoding][upp::uchar]", runtime)
{
    const auto test_data = upp_test::load_test_data<std::uint16_t>("utf_16_encoding.txt");

    for (const auto& [code_point, data] : test_data)
    {
        const auto ch = upp::uchar::from(code_point);
        REQUIRE(ch.has_value());

        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        CHECK(std::ranges::equal(ch->encode_utf16(), data));
    }
}