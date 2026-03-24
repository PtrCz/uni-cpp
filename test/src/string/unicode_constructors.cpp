#include "../bugspray.hpp"

#include <uni-cpp/string.hpp>

#include <string>
#include <string_view>

#include "utility.hpp"
#include "ranges.hpp"
#include "../encoding/utf.hpp"

TEST_CASE("upp::basic_ustring from_utf()", "[UTF encoding][string types][Unicode string types]")
{
    auto from_utf = []<upp::encoding Encoding, typename StringType>
        requires upp::unicode_encoding<Encoding>
    (auto&& range) {
        if constexpr (Encoding == upp::encoding::utf8)
        {
            return StringType::from_utf8(std::forward<decltype(range)>(range));
        }
        else if constexpr (Encoding == upp::encoding::utf16)
        {
            return StringType::from_utf16(std::forward<decltype(range)>(range));
        }
        else if constexpr (Encoding == upp::encoding::utf32)
        {
            return StringType::from_utf32(std::forward<decltype(range)>(range));
        }
    };

    SECTION("Correct transcoding")
    {
        upp_test::run_for_each_unicode_encoding(
            [&]<upp::encoding Encoding>
                requires upp::unicode_encoding<Encoding>
            () {
                upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
                    using code_unit_type = upp::encoding_traits<Encoding>::default_code_unit_type;

                    for (const auto& sequences : upp_test::utf::valid_sequences())
                    {
                        const auto& input_sequence = sequences.template encoded_as<Encoding>();

                        upp_test::string_input_range<code_unit_type> input_range{std::basic_string_view<code_unit_type>{input_sequence}};

                        const auto result             = from_utf.template operator()<Encoding, StringType>(input_sequence);
                        const auto input_range_result = from_utf.template operator()<Encoding, StringType>(input_range);

                        REQUIRE(result.has_value());
                        REQUIRE(input_range_result.has_value());

                        auto&& expected = sequences.template encoded_as<StringType::encoding_value>();

                        CHECK(result->underlying() == expected);
                        CHECK(input_range_result->underlying() == expected);
                    }
                });
            });
    }

    SECTION("Detecting errors")
    {
        upp_test::run_for_each_unicode_encoding(
            [&]<upp::encoding Encoding>
                requires upp::unicode_encoding<Encoding>
            () {
                upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
                    using code_unit_type = upp::encoding_traits<Encoding>::default_code_unit_type;

                    for (const auto& test_case : upp_test::utf::invalid_sequences_for_encoding<Encoding>())
                    {
                        upp_test::string_input_range<code_unit_type> input_range{std::basic_string_view<code_unit_type>{test_case.sequence}};

                        const auto result             = from_utf.template operator()<Encoding, StringType>(test_case.sequence);
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
    auto from_utf_lossy = []<upp::encoding SourceEncoding, typename StringType>
        requires upp::unicode_encoding<SourceEncoding>
    (auto&& range) {
        if constexpr (SourceEncoding == upp::encoding::utf8)
        {
            return StringType::from_utf8_lossy(std::forward<decltype(range)>(range));
        }
        else if constexpr (SourceEncoding == upp::encoding::utf16)
        {
            return StringType::from_utf16_lossy(std::forward<decltype(range)>(range));
        }
        else if constexpr (SourceEncoding == upp::encoding::utf32)
        {
            return StringType::from_utf32_lossy(std::forward<decltype(range)>(range));
        }
    };

    upp_test::run_for_each_unicode_encoding(
        [&]<upp::encoding SourceEncoding>
            requires upp::unicode_encoding<SourceEncoding>
        () {
            upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
                using code_unit_type = upp::encoding_traits<SourceEncoding>::default_code_unit_type;

                for (const auto& test_case : upp_test::utf::invalid_sequences_for_encoding<SourceEncoding>())
                {
                    upp_test::string_input_range<code_unit_type> input_range{std::basic_string_view<code_unit_type>{test_case.sequence}};

                    const auto result             = from_utf_lossy.template operator()<SourceEncoding, StringType>(test_case.sequence);
                    const auto input_range_result = from_utf_lossy.template operator()<SourceEncoding, StringType>(input_range);

                    auto&& expected = test_case.template lossily_encoded_as<StringType::encoding_value>();

                    CHECK(result.underlying() == expected);
                    CHECK(input_range_result.underlying() == expected);
                }
            });
        });
}
EVAL_TEST_CASE("upp::basic_ustring from_utf_lossy()");

TEST_CASE("upp::basic_ustring from_utf_unchecked()", "[UTF encoding][string types][Unicode string types]")
{
    upp_test::run_for_each_unicode_encoding( //
        [&]<upp::encoding Encoding>
            requires upp::unicode_encoding<Encoding>
        () {
            upp_test::run_for_each_unicode_string_type([&]<typename StringType>() {
                auto from_unchecked = [](const auto& sequences) {
                    if constexpr (Encoding == upp::encoding::utf8)
                    {
                        return StringType::from_utf8_unchecked(sequences.utf8_seq);
                    }
                    else if constexpr (Encoding == upp::encoding::utf16)
                    {
                        return StringType::from_utf16_unchecked(sequences.utf16_seq);
                    }
                    else if constexpr (Encoding == upp::encoding::utf32)
                    {
                        return StringType::from_utf32_unchecked(sequences.utf32_seq);
                    }
                };

                for (const auto& sequences : upp_test::utf::valid_sequences())
                {
                    const StringType result = from_unchecked(sequences);

                    const auto& expected = sequences.template encoded_as<StringType::encoding_value>();

                    CHECK(result.underlying() == expected);
                }
            });
        });
}
EVAL_TEST_CASE("upp::basic_ustring from_utf_unchecked()");