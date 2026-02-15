#include "../bugspray.hpp"

#include <uni-cpp/string.hpp>

#include <string>
#include <string_view>

#include "utility.hpp"
#include "ranges.hpp"
#include "../utf.hpp"

TEST_CASE("upp::basic_ustring from_utf8()", "[UTF encoding][string types][Unicode string types]")
{
    // `from_utf8` differs slightly in implementation depending on whether the argument is a bidirectional_range or not.
    // Here we're testing it on both kinds of ranges.

    SECTION("Correct transcoding")
    {
        upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
            for (const auto& sequences : upp_test::utf::valid_sequences())
            {
                upp_test::string_input_range<char8_t> input_range{std::u8string_view{sequences.utf8_seq}};

                const auto result             = StringType::from_utf8(sequences.utf8_seq);
                const auto input_range_result = StringType::from_utf8(input_range);

                REQUIRE(result.has_value());
                REQUIRE(input_range_result.has_value());

                auto&& expected = sequences.template encoded_with<StringType::unicode_encoding_value>();

                CHECK(result->underlying() == expected);
                CHECK(input_range_result->underlying() == expected);
            }
        });
    }

    SECTION("Detecting errors")
    {
        upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
            for (const auto& test_case : upp_test::utf::invalid_utf8_test_cases())
            {
                upp_test::string_input_range<char8_t> input_range{std::u8string_view{test_case.input}};

                const auto result             = StringType::from_utf8(test_case.input);
                const auto input_range_result = StringType::from_utf8(input_range);

                REQUIRE_FALSE(result.has_value());
                REQUIRE_FALSE(input_range_result.has_value());

                CHECK(result.error() == test_case.expected_error);
                CHECK(input_range_result.error() == test_case.expected_error);
            }
        });
    }
}
EVAL_TEST_CASE("upp::basic_ustring from_utf8()");

TEST_CASE("upp::basic_ustring from_utf8_unchecked()", "[UTF encoding][string types][Unicode string types]")
{
    upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
        for (const auto& sequences : upp_test::utf::valid_sequences())
        {
            const StringType result = StringType::from_utf8_unchecked(sequences.utf8_seq);

            const auto& expected = sequences.template encoded_with<StringType::unicode_encoding_value>();

            CHECK(result.underlying() == expected);
        }
    });
}
EVAL_TEST_CASE("upp::basic_ustring from_utf8_unchecked()");