#include "../catch2.hpp"

#include <uni-cpp/uchar.hpp>
#include <uni-cpp/ranges.hpp>

#include "base.hpp"

#include <span>
#include <string_view>

TEST_CASE("valid_code_unit_range concept", "[ranges]")
{
    STATIC_CHECK(upp::ranges::valid_code_unit_range<decltype(upp::uchar{}.encode_utf8()), upp::encoding::utf8>);
    STATIC_CHECK(upp::ranges::valid_code_unit_range<decltype(upp::uchar{}.encode_utf16()), upp::encoding::utf16>);

    STATIC_CHECK(!upp::ranges::valid_code_unit_range<decltype(upp::uchar{}.encode_utf8()), upp::encoding::ascii>);

    STATIC_CHECK(upp::ranges::valid_code_unit_range<decltype(std::string_view{} | upp::views::mark_as_valid_ascii), upp::encoding::ascii>);
    STATIC_CHECK(upp::ranges::valid_code_unit_range<decltype(std::string_view{} | upp::views::mark_as_valid_ascii), upp::encoding::utf8>);

    STATIC_CHECK(upp::ranges::valid_code_unit_range<decltype(std::string_view{} | upp::views::mark_as_valid_utf8), upp::encoding::utf8>);
    STATIC_CHECK(!upp::ranges::valid_code_unit_range<decltype(std::string_view{} | upp::views::mark_as_valid_utf8), upp::encoding::ascii>);

    STATIC_CHECK(upp::ranges::valid_code_unit_range<std::ranges::empty_view<char>, upp::encoding::utf8>);
    STATIC_CHECK(!upp::ranges::valid_code_unit_range<std::ranges::empty_view<char16_t>, upp::encoding::utf8>);

    STATIC_CHECK(
        upp::ranges::valid_code_unit_range<upp::ranges::cast_code_units_to_view<std::ranges::empty_view<char>, char8_t>, upp::encoding::utf8>);
    STATIC_CHECK(!upp::ranges::valid_code_unit_range<upp::ranges::cast_code_units_to_view<std::string_view, char8_t>, upp::encoding::utf8>);

    STATIC_CHECK(upp::ranges::valid_code_unit_range<decltype(std::string_view{} | upp::views::transcode_lossy_utf8_to_utf16), upp::encoding::utf16>);

    STATIC_CHECK(!upp::ranges::valid_code_unit_range<decltype(std::string_view{} | upp::views::transcode_lossy_ascii_to_utf8), upp::encoding::ascii>);
    STATIC_CHECK(
        upp::ranges::valid_code_unit_range<decltype(std::string_view{} | upp::views::mark_as_valid_ascii | upp::views::transcode_valid_ascii_to_utf8),
                                           upp::encoding::ascii>);

    STATIC_CHECK(
        upp::ranges::valid_code_unit_range<upp::ranges::encode_view<std::span<upp::uchar>, upp::encoding::utf8, std::uint8_t>, upp::encoding::utf8>);
    STATIC_CHECK(!upp::ranges::valid_code_unit_range<upp::ranges::encode_view<std::span<upp::uchar>, upp::encoding::utf8, std::uint8_t>,
                                                     upp::encoding::ascii>);
}

TEST_CASE("views::mark_as_valid_encoding", "[ranges]")
{
    STATIC_CHECK(IS_EXPR_OF_TYPE(std::string_view{} | upp::views::mark_as_valid_ascii,
                                 upp::ranges::valid_code_unit_view<std::string_view, upp::encoding::ascii>));
    STATIC_CHECK(IS_EXPR_OF_TYPE(std::u16string_view{} | upp::views::mark_as_valid_utf16,
                                 upp::ranges::valid_code_unit_view<std::u16string_view, upp::encoding::utf16>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::u8string_view{} | upp::views::mark_as_valid_utf8 | upp::views::mark_as_valid_utf8,
                                 upp::ranges::valid_code_unit_view<std::u8string_view, upp::encoding::utf8>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(upp::uchar{}.encode_utf8() | upp::views::mark_as_valid_utf8, std::ranges::owning_view<upp::uchar::encode_utf8_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(upp::uchar{}.encode_utf16() | upp::views::mark_as_valid_utf16 | upp::views::mark_as_valid_utf16,
                                 std::ranges::owning_view<upp::uchar::encode_utf16_t>));
}