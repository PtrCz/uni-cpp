#include "../catch2.hpp"

#include <uni-cpp/uchar.hpp>
#include <uni-cpp/ranges.hpp>

#include "base.hpp"

#include <string_view>
#include <array>
#include <vector>

TEST_CASE("encode_view", "[ranges][UTF encoding]")
{
    CONSTEXPR_AND_RUNTIME_TEST()
    {
        using namespace std::string_view_literals;
        using namespace upp::char_literals;

        CRTT_CHECK(
            upp_test::ranges::equal(std::array{u8'a'_ac, u8'b'_ac, u8'c'_ac} | upp::views::encode_as<upp::encoding::ascii, char8_t>, u8"abc"sv));

        CRTT_CHECK(upp_test::ranges::equal(std::array{u8'a'_ac, u8'b'_ac, u8'c'_ac} | upp::views::encode_as_utf16, u"abc"sv));

        CRTT_CHECK(upp_test::ranges::equal(std::array{U'a'_uc, U'b'_uc, U'c'_uc} | upp::views::encode_as_utf8, u8"abc"sv));
        CRTT_CHECK(upp_test::ranges::equal(std::array{U'a'_uc, U'b'_uc, U'c'_uc} | upp::views::encode_as_utf16, u"abc"sv));
        CRTT_CHECK(upp_test::ranges::equal(std::array{U'a'_uc, U'b'_uc, U'c'_uc} | upp::views::encode_as_utf32, U"abc"sv));

        CRTT_CHECK(upp_test::ranges::equal(std::array{0x00E9_uc, 0x01ED05_uc, 0x20AC_uc} | upp::views::encode_as_utf8, u8"\u00E9\U0001ED05\u20AC"sv));
        CRTT_CHECK(upp_test::ranges::equal(std::array{0x00E9_uc, 0x01ED05_uc, 0x20AC_uc} | upp::views::encode_as_utf16, u"\u00E9\U0001ED05\u20AC"sv));
        CRTT_CHECK(upp_test::ranges::equal(std::array{0x00E9_uc, 0x01ED05_uc, 0x20AC_uc} | upp::views::encode_as_utf32, U"\u00E9\U0001ED05\u20AC"sv));
    };
}

TEST_CASE("views::encode_as", "[ranges]")
{
    STATIC_CHECK(IS_EXPR_OF_TYPE(std::ranges::empty_view<upp::uchar>{} | upp::views::encode_as_utf16, std::ranges::empty_view<char16_t>));

    STATIC_CHECK(
        IS_EXPR_OF_TYPE(std::string_view{} | upp::views::decode_lossy_ascii | upp::views::encode_as_utf16,
                        upp::ranges::encode_view<
                            upp::ranges::decode_view<std::string_view, upp::encoding::ascii, upp::ranges::decode_view_kind::lossy, upp::ascii_char>,
                            upp::encoding::utf16, char16_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(
        std::string_view{} | upp::views::decode_lossy_utf8 | upp::views::encode_as_utf16,
        upp::ranges::transcode_view<std::string_view, upp::encoding::utf8, upp::encoding::utf16, upp::ranges::transcode_view_kind::lossy, char16_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::u8string_view{} | upp::views::mark_as_valid_utf8 | upp::views::decode_valid_utf8 | upp::views::encode_as_utf8,
                                 upp::ranges::valid_code_unit_view<std::u8string_view, upp::encoding::utf8>));

    STATIC_CHECK(
        IS_EXPR_OF_TYPE(std::string_view{} | upp::views::mark_as_valid_utf8 | upp::views::decode_valid_utf8 | upp::views::encode_as_utf8,
                        upp::ranges::cast_code_units_to_view<upp::ranges::valid_code_unit_view<std::string_view, upp::encoding::utf8>, char8_t>));

    STATIC_CHECK(
        IS_EXPR_OF_TYPE(std::string_view{} | upp::views::mark_as_valid_utf8 | upp::views::decode_valid_utf8 | upp::views::encode_as_utf16,
                        upp::ranges::transcode_view<upp::ranges::valid_code_unit_view<std::string_view, upp::encoding::utf8>, upp::encoding::utf8,
                                                    upp::encoding::utf16, upp::ranges::transcode_view_kind::valid, char16_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::vector<upp::uchar>{} | upp::views::encode_as_utf8,
                                 upp::ranges::encode_view<std::ranges::owning_view<std::vector<upp::uchar>>, upp::encoding::utf8, char8_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::vector<upp::ascii_char>{} | upp::views::encode_as_ascii,
                                 upp::ranges::encode_view<std::ranges::owning_view<std::vector<upp::ascii_char>>, upp::encoding::ascii, char>));
}