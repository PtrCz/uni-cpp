#include "../catch2.hpp"

#include <uni-cpp/ranges.hpp>

#include "base.hpp"
#include "to_input.hpp"
#include "../test_data.hpp"

TEST_CASE("normalize_view", "[ranges][normalization]")
{
    const auto& test_data = upp_test::load_normalization_test_data();

    const auto decode = upp::views::mark_as_valid_utf32 | upp::views::decode_valid_utf32;

    for (const auto& test_case : test_data)
    {
        const auto source   = test_case.source | decode;
        const auto test_nfc = test_case.nfc | decode, test_nfd = test_case.nfd | decode;
        const auto test_nfkc = test_case.nfkc | decode, test_nfkd = test_case.nfkd | decode;

        auto test_equality = [](auto&& actual_range, auto&& expected_range) {
            CHECK(upp_test::ranges::equal(actual_range, expected_range));
            CHECK(upp_test::ranges::equal(actual_range | upp::views::reverse, expected_range | std::views::reverse));
        };

        test_equality(source | upp::views::normalize_to_nfc, test_nfc);
        test_equality(test_nfc | upp::views::normalize_to_nfc, test_nfc), test_equality(test_nfd | upp::views::normalize_to_nfc, test_nfc);

        test_equality(test_nfkc | upp::views::normalize_to_nfc, test_nfkc), test_equality(test_nfkd | upp::views::normalize_to_nfc, test_nfkc);

        test_equality(source | upp::views::normalize_to_nfd, test_nfd);
        test_equality(test_nfd | upp::views::normalize_to_nfd, test_nfd), test_equality(test_nfc | upp::views::normalize_to_nfd, test_nfd);

        test_equality(test_nfkd | upp::views::normalize_to_nfd, test_nfkd), test_equality(test_nfkc | upp::views::normalize_to_nfd, test_nfkd);

        test_equality(source | upp::views::normalize_to_nfkc, test_nfkc);
        test_equality(test_nfc | upp::views::normalize_to_nfkc, test_nfkc), test_equality(test_nfd | upp::views::normalize_to_nfkc, test_nfkc);
        test_equality(test_nfkc | upp::views::normalize_to_nfkc, test_nfkc), test_equality(test_nfkd | upp::views::normalize_to_nfkc, test_nfkc);

        test_equality(source | upp::views::normalize_to_nfkd, test_nfkd);
        test_equality(test_nfc | upp::views::normalize_to_nfkd, test_nfkd), test_equality(test_nfd | upp::views::normalize_to_nfkd, test_nfkd);
        test_equality(test_nfkc | upp::views::normalize_to_nfkd, test_nfkd), test_equality(test_nfkd | upp::views::normalize_to_nfkd, test_nfkd);
    }
}