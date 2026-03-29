#include "../bugspray.hpp"

#include <uni-cpp/ranges.hpp>

#include "../utility.hpp"
#include "../encoding/encoding.hpp"
#include "to_input.hpp"

// Note: Clang has some difficulties to run the following test at compile-time.
//       Regardless, it would be very slow to compile (it already is).
//       The implementation of `upp::views::transcode` does not differ for constant evaluation.

TEST_CASE("transcode_view", "[ranges][UTF encoding]", runtime)
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
                            CHECK(std::ranges::equal(rg | transcode_valid, expected));

                        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
                        CHECK(std::ranges::equal(rg | transcode_expected, expected_expected));
                        CHECK(std::ranges::equal(rg | transcode_lossy, expected_lossy));

                        if constexpr (std::ranges::bidirectional_range<decltype(rg)>)
                        {
                            // Test reading backwards (operator--)

                            if constexpr (upp::ranges::valid_code_unit_range<decltype(rg), SourceEncoding>)
                                CHECK(std::ranges::equal(rg | transcode_valid | std::views::reverse, expected | std::views::reverse));

                            CHECK(std::ranges::equal(rg | transcode_expected | std::views::reverse, expected_expected | std::views::reverse));
                            CHECK(std::ranges::equal(rg | transcode_lossy | std::views::reverse, expected_lossy | std::views::reverse));
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

                    CHECK(std::ranges::equal(seq.sequence | transcode_expected, expected_expected));
                    CHECK(std::ranges::equal(seq.sequence | upp_test::views::to_input | transcode_expected, expected_expected));
                    CHECK(std::ranges::equal(seq.sequence | transcode_expected | std::views::reverse, expected_expected | std::views::reverse));

                    CHECK(std::ranges::equal(seq.sequence | transcode_lossy, expected_lossy));
                    CHECK(std::ranges::equal(seq.sequence | upp_test::views::to_input | transcode_lossy, expected_lossy));
                    CHECK(std::ranges::equal(seq.sequence | transcode_lossy | std::views::reverse, expected_lossy | std::views::reverse));
                }
            });
        });
    }
}