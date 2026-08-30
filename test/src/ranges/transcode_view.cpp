#include "../catch2.hpp"

#include <uni-cpp/ranges.hpp>

#include "../utility.hpp"
#include "../encoding/encoding.hpp"
#include "base.hpp"
#include "to_input.hpp"

#include <string_view>
#include <vector>

TEST_CASE("transcode_view", "[ranges][UTF encoding]")
{
    SECTION("Transcoding well-formed sequences")
    {
        upp_test::run_for_each_encoding([&]<upp::encoding SourceEncoding>() {
            upp_test::run_for_each_unicode_encoding([&]<upp::encoding TargetEncoding>() {
                using code_unit_type = typename upp::encoding_traits<TargetEncoding>::default_code_unit_type;
                using error_type     = typename upp::encoding_traits<SourceEncoding>::error_type;

                const auto transcode_fn = []<upp::ranges::transcode_view_kind Kind>() -> const auto& {
                    return upp::views::transcode<SourceEncoding, TargetEncoding, Kind>;
                };

                const auto& transcode_valid    = transcode_fn.template operator()<upp::ranges::transcode_view_kind::valid>();
                const auto& transcode_expected = transcode_fn.template operator()<upp::ranges::transcode_view_kind::expected>();
                const auto& transcode_lossy    = transcode_fn.template operator()<upp::ranges::transcode_view_kind::lossy>();

                for (const auto& seq : upp_test::valid_sequences<SourceEncoding>())
                {
                    const auto& expected       = seq.template encoded_as<TargetEncoding>();
                    const auto& expected_lossy = expected;

                    const auto expected_expected = seq.template encoded_as<TargetEncoding>() | std::views::transform([](code_unit_type code_unit) {
                                                       return std::expected<code_unit_type, error_type>{std::in_place, code_unit};
                                                   });

                    const auto test_for_range = [&](auto&& rg) {
                        if constexpr (upp::ranges::valid_code_unit_range<decltype(rg), SourceEncoding>)
                            CHECK(upp_test::ranges::equal(rg | transcode_valid, expected));

                        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
                        CHECK(upp_test::ranges::equal(rg | transcode_expected, expected_expected));
                        CHECK(upp_test::ranges::equal(rg | transcode_lossy, expected_lossy));

                        if constexpr (std::ranges::bidirectional_range<decltype(rg)>)
                        {
                            // Test reading backwards (operator--)

                            if constexpr (upp::ranges::valid_code_unit_range<decltype(rg), SourceEncoding>)
                                CHECK(upp_test::ranges::equal(rg | transcode_valid | std::views::reverse, expected | std::views::reverse));

                            CHECK(upp_test::ranges::equal(rg | transcode_expected | std::views::reverse, expected_expected | std::views::reverse));
                            CHECK(upp_test::ranges::equal(rg | transcode_lossy | std::views::reverse, expected_lossy | std::views::reverse));
                        }
                    };

                    // Note: the implementation of transcode_view differs for all of the following
                    test_for_range(seq.sequence);
                    test_for_range(seq.sequence | upp::views::mark_as_valid_encoding<SourceEncoding>);
                    test_for_range(seq.sequence | upp_test::views::to_input);
                    test_for_range(seq.sequence | upp_test::views::to_input | upp::views::mark_as_valid_encoding<SourceEncoding>);
                }
            });
        });
    }

    SECTION("Transcoding ill-formed sequences")
    {
        upp_test::run_for_each_encoding([&]<upp::encoding SourceEncoding>() {
            upp_test::run_for_each_unicode_encoding([&]<upp::encoding TargetEncoding>() {
                const auto transcode_fn = []<upp::ranges::transcode_view_kind Kind>() -> const auto& {
                    return upp::views::transcode<SourceEncoding, TargetEncoding, Kind>;
                };

                const auto& transcode_expected = transcode_fn.template operator()<upp::ranges::transcode_view_kind::expected>();
                const auto& transcode_lossy    = transcode_fn.template operator()<upp::ranges::transcode_view_kind::lossy>();

                for (const auto& seq : upp_test::invalid_sequences<SourceEncoding>())
                {
                    const auto& expected_expected = seq.template transcoded_with_errors_to<TargetEncoding>();
                    const auto& expected_lossy    = seq.template lossily_encoded_as<TargetEncoding>();

                    CHECK(upp_test::ranges::equal(seq.sequence | transcode_expected, expected_expected));
                    CHECK(upp_test::ranges::equal(seq.sequence | upp_test::views::to_input | transcode_expected, expected_expected));
                    CHECK(upp_test::ranges::equal(seq.sequence | transcode_expected | std::views::reverse, expected_expected | std::views::reverse));

                    CHECK(upp_test::ranges::equal(seq.sequence | transcode_lossy, expected_lossy));
                    CHECK(upp_test::ranges::equal(seq.sequence | upp_test::views::to_input | transcode_lossy, expected_lossy));
                    CHECK(upp_test::ranges::equal(seq.sequence | transcode_lossy | std::views::reverse, expected_lossy | std::views::reverse));
                }
            });
        });
    }
}

TEST_CASE("views::transcode", "[ranges]")
{
    STATIC_CHECK(
        IS_EXPR_OF_TYPE(std::string_view{} | upp::views::mark_as_valid_ascii | upp::views::transcode_lossy_ascii_to_utf8,
                        upp::ranges::cast_code_units_to_view<upp::ranges::valid_code_unit_view<std::string_view, upp::encoding::ascii>, char8_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::ranges::empty_view<char>{} | upp::views::transcode_expected_ascii_to_utf16,
                                 std::ranges::empty_view<std::expected<char16_t, upp::ascii_error>>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::ranges::empty_view<char>{} | upp::views::transcode_lossy_ascii_to_utf16, std::ranges::empty_view<char16_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::vector<upp::uchar>{} | upp::views::encode_as_utf8 | upp::views::transcode_lossy_utf8_to_utf16,
                                 upp::ranges::encode_view<std::ranges::owning_view<std::vector<upp::uchar>>, upp::encoding::utf16, char16_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(
        std::vector<upp::uchar>{} | upp::views::encode_as_utf8 | upp::views::transcode_lossy_ascii_to_utf16,
        upp::ranges::transcode_view<upp::ranges::encode_view<std::ranges::owning_view<std::vector<upp::uchar>>, upp::encoding::utf8, char8_t>,
                                    upp::encoding::ascii, upp::encoding::utf16, upp::ranges::transcode_view_kind::lossy, char16_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(
        std::string_view{} | upp::views::transcode_lossy_utf8_to_utf16 | upp::views::transcode_valid_utf16_to_utf32,
        upp::ranges::transcode_view<std::string_view, upp::encoding::utf8, upp::encoding::utf32, upp::ranges::transcode_view_kind::lossy, char32_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(
        std::string_view{} | upp::views::transcode_lossy_utf8_to_utf16 | upp::views::reverse | upp::views::transcode_lossy_utf16_to_utf32,
        upp::ranges::transcode_view<upp::ranges::reverse_view<upp::ranges::transcode_view<std::string_view, upp::encoding::utf8, upp::encoding::utf16,
                                                                                          upp::ranges::transcode_view_kind::lossy, char16_t>>,
                                    upp::encoding::utf16, upp::encoding::utf32, upp::ranges::transcode_view_kind::lossy, char32_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::vector<upp::uchar>{} | upp_test::views::to_input | upp::views::encode_as_utf8 |
                                     upp::views::transcode_lossy_utf8_to_utf16,
                                 upp::ranges::encode_view<upp_test::ranges::to_input_view<std::ranges::owning_view<std::vector<upp::uchar>>>,
                                                          upp::encoding::utf16, char16_t>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::string_view{} | upp_test::views::to_input | upp::views::transcode_lossy_utf8_to_utf16 |
                                     upp::views::transcode_valid_utf16_to_utf32,
                                 upp::ranges::transcode_view<upp_test::ranges::to_input_view<std::string_view>, upp::encoding::utf8,
                                                             upp::encoding::utf32, upp::ranges::transcode_view_kind::lossy, char32_t>));
}