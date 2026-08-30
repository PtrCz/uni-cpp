#include "../catch2.hpp"

#include <uni-cpp/uchar.hpp>
#include <uni-cpp/ranges.hpp>

#include "base.hpp"
#include "to_input.hpp"

#include <string_view>
#include <vector>

TEST_CASE("decode_view", "[ranges][UTF encoding]")
{
    CONSTEXPR_AND_RUNTIME_TEST()
    {
        using namespace std::string_view_literals;
        using namespace upp::char_literals;

        CRTT_CHECK(upp_test::ranges::equal(u8"abc"sv | upp::views::decode_lossy_ascii, {u8'a'_ac, u8'b'_ac, u8'c'_ac}));
        CRTT_CHECK(upp_test::ranges::equal(u8"abc"sv | upp::views::decode_lossy_ascii_to_uchars, {U'a'_uc, U'b'_uc, U'c'_uc}));

        CRTT_CHECK(upp_test::ranges::equal(std::ranges::single_view{std::uint8_t{0x80}} | upp::views::decode_lossy_ascii,
                                           {upp::ascii_char::substitute_character()}));

        CRTT_CHECK(upp_test::ranges::equal(std::ranges::single_view{std::uint8_t{0x80}} | upp::views::decode_lossy_ascii_to_uchars,
                                           {upp::uchar::replacement_character()}));

        CRTT_CHECK(upp_test::ranges::equal(u8"abc"sv | upp::views::mark_as_valid_utf8 | upp::views::decode_valid_utf8, {U'a'_uc, U'b'_uc, U'c'_uc}));

        CRTT_CHECK(upp_test::ranges::equal(std::ranges::single_view{std::uint8_t{0x80}} | upp::views::decode_lossy_utf8,
                                           {upp::uchar::replacement_character()}));

        CRTT_CHECK(upp_test::ranges::equal(u8"\u00E9\U0001ED05\u20AC"sv | upp::views::mark_as_valid_utf8 | upp::views::decode_valid_utf8,
                                           {0x00E9_uc, 0x01ED05_uc, 0x20AC_uc}));

        CRTT_CHECK(upp_test::ranges::equal(u"\u00E9\U0001ED05\u20AC"sv | upp::views::decode_lossy_utf16, {0x00E9_uc, 0x01ED05_uc, 0x20AC_uc}));
        CRTT_CHECK(upp_test::ranges::equal(U"\u00E9\U0001ED05\u20AC"sv | upp::views::decode_lossy_utf32, {0x00E9_uc, 0x01ED05_uc, 0x20AC_uc}));
    };
}

TEST_CASE("views::decode", "[ranges]")
{
    STATIC_CHECK(IS_EXPR_OF_TYPE(std::ranges::empty_view<char>{} | upp::views::decode_expected_ascii,
                                 std::ranges::empty_view<std::expected<upp::ascii_char, upp::ascii_error>>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::ranges::empty_view<char>{} | upp::views::decode_lossy_ascii, std::ranges::empty_view<upp::ascii_char>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::ranges::empty_view<char>{} | upp::views::decode_expected_ascii_to_uchars,
                                 std::ranges::empty_view<std::expected<upp::uchar, upp::ascii_error>>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::ranges::empty_view<char>{} | upp::views::decode_lossy_ascii_to_uchars, std::ranges::empty_view<upp::uchar>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::vector<upp::uchar>{} | upp::views::encode_as_utf16 | upp::views::decode_valid_utf16,
                                 std::ranges::owning_view<std::vector<upp::uchar>>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(
        std::vector<upp::uchar>{} | upp::views::encode_as_utf8 | upp::views::decode_lossy_ascii,
        upp::ranges::decode_view<upp::ranges::encode_view<std::ranges::owning_view<std::vector<upp::uchar>>, upp::encoding::utf8, char8_t>,
                                 upp::encoding::ascii, upp::ranges::decode_view_kind::lossy, upp::ascii_char>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::string_view{} | upp::views::transcode_lossy_ascii_to_utf8 | upp::views::decode_lossy_ascii,
                                 upp::ranges::decode_view<upp::ranges::transcode_view<std::string_view, upp::encoding::ascii, upp::encoding::utf8,
                                                                                      upp::ranges::transcode_view_kind::lossy, char8_t>,
                                                          upp::encoding::ascii, upp::ranges::decode_view_kind::lossy, upp::ascii_char>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::string_view{} | upp::views::transcode_lossy_utf8_to_utf16 | upp::views::decode_valid_utf16,
                                 upp::ranges::decode_view<std::string_view, upp::encoding::utf8, upp::ranges::decode_view_kind::lossy, upp::uchar>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::string_view{} | upp::views::decode_lossy_utf8,
                                 upp::ranges::decode_view<std::string_view, upp::encoding::utf8, upp::ranges::decode_view_kind::lossy, upp::uchar>));

    STATIC_CHECK(
        IS_EXPR_OF_TYPE(std::string_view{} | upp::views::decode_lossy_ascii,
                        upp::ranges::decode_view<std::string_view, upp::encoding::ascii, upp::ranges::decode_view_kind::lossy, upp::ascii_char>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::string_view{} | upp::views::decode_lossy_ascii_to_uchars,
                                 upp::ranges::decode_view<std::string_view, upp::encoding::ascii, upp::ranges::decode_view_kind::lossy, upp::uchar>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::vector<upp::uchar>{} | upp_test::views::to_input | upp::views::encode_as_utf16 | upp::views::decode_valid_utf16,
                                 upp_test::ranges::to_input_view<std::ranges::owning_view<std::vector<upp::uchar>>>));

    STATIC_CHECK(
        IS_EXPR_OF_TYPE(std::string_view{} | upp_test::views::to_input | upp::views::transcode_lossy_utf8_to_utf16 | upp::views::decode_valid_utf16,
                        upp::ranges::decode_view<upp_test::ranges::to_input_view<std::string_view>, upp::encoding::utf8,
                                                 upp::ranges::decode_view_kind::lossy, upp::uchar>));
}