#include "bugspray.hpp"

#include "test_data.hpp"

#include <uni-cpp/uchar.hpp>
#include <cstdint>
#include <ranges>

TEST_CASE("UTF-8 encoding", "[UTF encoding][upp::uchar]")
{
    SECTION("compiletime-friendly encoding test")
    {
        using namespace upp::char_literals;

        const std::initializer_list<std::pair<upp::uchar, std::u8string>> test_cases = {
            {U'a'_uc, {0x61}},
            {U'\u00D1'_uc, {0xC3, 0x91}},
            {U'\u08A8'_uc, {0xE0, 0xA2, 0xA8}},
            {U'\U000E0186'_uc, {0xF3, 0xA0, 0x86, 0x86}},
        };

        for (const auto& [ch, encoded] : test_cases)
        {
            CHECK(std::ranges::equal(ch.encode_utf8(), encoded));
        }
    }

    SECTION("large dataset test", runtime)
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
}
EVAL_TEST_CASE("UTF-8 encoding");

TEST_CASE("UTF-16 encoding", "[UTF encoding][upp::uchar]")
{
    SECTION("compiletime-friendly encoding test")
    {
        using namespace upp::char_literals;

        const std::initializer_list<std::pair<upp::uchar, std::u16string>> test_cases = {
            {U'a'_uc, {0x61}},
            {U'\u00D1'_uc, {0xD1}},
            {U'\u08A8'_uc, {0x8A8}},
            {U'\U000E0186'_uc, {0xDB40, 0xDD86}},
        };

        for (const auto& [ch, encoded] : test_cases)
        {
            CHECK(std::ranges::equal(ch.encode_utf16(), encoded));
        }
    }

    SECTION("large dataset test", runtime)
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
}
EVAL_TEST_CASE("UTF-16 encoding");