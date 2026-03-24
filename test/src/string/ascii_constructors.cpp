#include "../bugspray.hpp"

#include <uni-cpp/string.hpp>

#include <string>
#include <string_view>

#include "ranges.hpp"
#include "../encoding/ascii.hpp"

TEST_CASE("upp::basic_ascii_string from_ascii()", "[string types]")
{
    SECTION("Valid sequences")
    {
        for (const auto& seq : upp_test::ascii::valid_sequences())
        {
            const auto& valid_sequence = seq.sequence;

            upp_test::string_input_range<char> input_range{std::string_view{valid_sequence}};

            const auto result             = upp::ascii_string::from_ascii(valid_sequence);
            const auto input_range_result = upp::ascii_string::from_ascii(input_range);

            REQUIRE(result.has_value());
            REQUIRE(input_range_result.has_value());

            CHECK(result->underlying() == valid_sequence);
            CHECK(input_range_result->underlying() == valid_sequence);
        }
    }

    SECTION("Detecting errors")
    {
        for (const auto& test_case : upp_test::ascii::invalid_sequences())
        {
            upp_test::string_input_range<char> input_range{std::string_view{test_case.sequence}};

            const auto result             = upp::ascii_string::from_ascii(test_case.sequence);
            const auto input_range_result = upp::ascii_string::from_ascii(input_range);

            REQUIRE(!result.has_value());
            REQUIRE(!input_range_result.has_value());

            CHECK(result.error() == test_case.expected_error);
            CHECK(input_range_result.error() == test_case.expected_error);
        }
    }
}
EVAL_TEST_CASE("upp::basic_ascii_string from_ascii()");

TEST_CASE("upp::basic_ascii_string from_ascii_lossy()", "[string types]")
{
    for (const auto& test_case : upp_test::ascii::invalid_sequences())
    {
        upp_test::string_input_range<char> input_range{std::string_view{test_case.sequence}};

        const auto result             = upp::ascii_string::from_ascii_lossy(test_case.sequence);
        const auto input_range_result = upp::ascii_string::from_ascii_lossy(input_range);

        CHECK(result.underlying() == test_case.as_ascii_lossy);
        CHECK(input_range_result.underlying() == test_case.as_ascii_lossy);
    }
}
EVAL_TEST_CASE("upp::basic_ascii_string from_ascii_lossy()");

TEST_CASE("upp::basic_ascii_string from_ascii_unchecked()", "[string types]")
{
    for (const auto& valid_sequence : upp_test::ascii::valid_sequences())
    {
        const auto result = upp::ascii_string::from_ascii_unchecked(valid_sequence.sequence);

        CHECK(result.underlying() == valid_sequence.sequence);
    }
}
EVAL_TEST_CASE("upp::basic_ascii_string from_ascii_unchecked()");