#include "../catch2.hpp"

#include <uni-cpp/ranges.hpp>

#include "base.hpp"

#include <array>
#include <string_view>
#include <bit>

TEST_CASE("cast_code_units_to_view", "[ranges]")
{
    CONSTEXPR_AND_RUNTIME_TEST()
    {
        using namespace std::string_view_literals;

        CRTT_CHECK(upp_test::ranges::equal("Hello"sv | upp::views::cast_code_units_to<char8_t>, u8"Hello"sv));

        CRTT_CHECK(upp_test::ranges::equal(std::array<std::uint8_t, 2>{0xC3, 0xA9} | upp::views::cast_code_units_to<char8_t>, u8"\u00E9"sv));

        CRTT_CHECK(upp_test::ranges::equal(std::array<char, 2>{std::bit_cast<char>(std::uint8_t{0xC3}), std::bit_cast<char>(std::uint8_t{0xA9})} |
                                               upp::views::cast_code_units_to<char8_t>,
                                           u8"\u00E9"sv));

        CRTT_CHECK(upp_test::ranges::equal(u8"\u00E9"sv | upp::views::cast_code_units_to<char>,
                                           {std::bit_cast<char>(std::uint8_t{0xC3}), std::bit_cast<char>(std::uint8_t{0xA9})}));
    };
}

TEST_CASE("views::cast_code_units_to", "[ranges]")
{
    STATIC_CHECK(IS_EXPR_OF_TYPE(std::string_view{} | upp::views::cast_code_units_to<char8_t>,
                                 upp::ranges::cast_code_units_to_view<std::string_view, char8_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::u8string_view{} | upp::views::cast_code_units_to<char8_t>, std::u8string_view));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::u16string_view{} | upp::views::cast_code_units_to<std::uint16_t>,
                                 upp::ranges::cast_code_units_to_view<std::u16string_view, std::uint16_t>));
}