#include "../bugspray.hpp"

#include <uni-cpp/string.hpp>

#include <string>
#include <string_view>

#include "utility.hpp"
#include "ranges.hpp"
#include "../utf.hpp"

TEST_CASE("upp::basic_ustring from_utf()", "[UTF encoding][string types][Unicode string types]")
{
    auto from_utf = []<upp::unicode_encoding Encoding, typename StringType>(auto&& range) {
        if constexpr (Encoding == upp::unicode_encoding::utf8)
        {
            return StringType::from_utf8(std::forward<decltype(range)>(range));
        }
        else if constexpr (Encoding == upp::unicode_encoding::utf16)
        {
            return StringType::from_utf16(std::forward<decltype(range)>(range));
        }
        else if constexpr (Encoding == upp::unicode_encoding::utf32)
        {
            return StringType::from_utf32(std::forward<decltype(range)>(range));
        }
    };

    SECTION("Correct transcoding")
    {
        upp_test::run_for_each_unicode_encoding([&]<upp::unicode_encoding Encoding>() {
            upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
                using code_unit_type = upp::encoding_traits<static_cast<upp::encoding>(Encoding)>::default_code_unit_type;

                for (const auto& sequences : upp_test::utf::valid_sequences())
                {
                    const auto& input_sequence = sequences.template encoded_with<Encoding>();

                    upp_test::string_input_range<code_unit_type> input_range{std::basic_string_view<code_unit_type>{input_sequence}};

                    const auto result             = from_utf.template operator()<Encoding, StringType>(input_sequence);
                    const auto input_range_result = from_utf.template operator()<Encoding, StringType>(input_range);

                    REQUIRE(result.has_value());
                    REQUIRE(input_range_result.has_value());

                    auto&& expected = sequences.template encoded_with<StringType::unicode_encoding_value>();

                    CHECK(result->underlying() == expected);
                    CHECK(input_range_result->underlying() == expected);
                }
            });
        });
    }

    SECTION("Detecting errors")
    {
        upp_test::run_for_each_unicode_encoding([&]<upp::unicode_encoding Encoding>() {
            upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
                using code_unit_type = upp::encoding_traits<static_cast<upp::encoding>(Encoding)>::default_code_unit_type;

                for (const auto& test_case : upp_test::utf::invalid_test_cases_for_encoding<Encoding>())
                {
                    upp_test::string_input_range<code_unit_type> input_range{std::basic_string_view<code_unit_type>{test_case.input}};

                    const auto result             = from_utf.template operator()<Encoding, StringType>(test_case.input);
                    const auto input_range_result = from_utf.template operator()<Encoding, StringType>(input_range);

                    REQUIRE(!result.has_value());
                    REQUIRE(!input_range_result.has_value());

                    CHECK(result.error() == test_case.expected_error);
                    CHECK(input_range_result.error() == test_case.expected_error);
                }
            });
        });
    }
}
EVAL_TEST_CASE("upp::basic_ustring from_utf()");

TEST_CASE("upp::basic_ustring from_utf_lossy()", "[UTF encoding][string types][Unicode string types]")
{
    auto from_utf_lossy = []<upp::unicode_encoding SourceEncoding, typename StringType>(auto&& range) {
        if constexpr (SourceEncoding == upp::unicode_encoding::utf8)
        {
            return StringType::from_utf8_lossy(std::forward<decltype(range)>(range));
        }
        else if constexpr (SourceEncoding == upp::unicode_encoding::utf16)
        {
            return StringType::from_utf16_lossy(std::forward<decltype(range)>(range));
        }
        else if constexpr (SourceEncoding == upp::unicode_encoding::utf32)
        {
            return StringType::from_utf32_lossy(std::forward<decltype(range)>(range));
        }
    };

    upp_test::run_for_each_unicode_encoding([&]<upp::unicode_encoding SourceEncoding>() {
        upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
            using code_unit_type = upp::encoding_traits<static_cast<upp::encoding>(SourceEncoding)>::default_code_unit_type;

            for (const auto& test_case : upp_test::utf::invalid_test_cases_for_encoding<SourceEncoding>())
            {
                upp_test::string_input_range<code_unit_type> input_range{std::basic_string_view<code_unit_type>{test_case.input}};

                const auto result             = from_utf_lossy.template operator()<SourceEncoding, StringType>(test_case.input);
                const auto input_range_result = from_utf_lossy.template operator()<SourceEncoding, StringType>(input_range);

                auto&& expected = test_case.template encoded_lossily_with<StringType::unicode_encoding_value>();

                CHECK(result.underlying() == expected);
                CHECK(input_range_result.underlying() == expected);
            }
        });
    });
}
EVAL_TEST_CASE("upp::basic_ustring from_utf_lossy()");

TEST_CASE("upp::basic_ustring from_utf_unchecked()", "[UTF encoding][string types][Unicode string types]")
{
    upp_test::run_for_each_unicode_encoding([&]<upp::unicode_encoding Encoding>() {
        upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
            auto from_unchecked = [](const auto& sequences) {
                if constexpr (Encoding == upp::unicode_encoding::utf8)
                {
                    return StringType::from_utf8_unchecked(sequences.utf8_seq);
                }
                else if constexpr (Encoding == upp::unicode_encoding::utf16)
                {
                    return StringType::from_utf16_unchecked(sequences.utf16_seq);
                }
                else if constexpr (Encoding == upp::unicode_encoding::utf32)
                {
                    return StringType::from_utf32_unchecked(sequences.utf32_seq);
                }
            };

            for (const auto& sequences : upp_test::utf::valid_sequences())
            {
                const StringType result = from_unchecked(sequences);

                const auto& expected = sequences.template encoded_with<StringType::unicode_encoding_value>();

                CHECK(result.underlying() == expected);
            }
        });
    });
}
EVAL_TEST_CASE("upp::basic_ustring from_utf_unchecked()");