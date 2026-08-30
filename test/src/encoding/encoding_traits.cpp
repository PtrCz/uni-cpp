#include "../catch2.hpp"
#include "encoding.hpp"

#include "../utility.hpp"

#include <bit>

TEST_CASE("upp::encoding_traits", "[UTF encoding]")
{
    SECTION("Decoding and validating well-formed sequences")
    {
        upp_test::run_for_each_encoding([&]<upp::encoding SourceEncoding>() {
            using traits = upp::encoding_traits<SourceEncoding>;

            auto expected_decoded_value = [](const auto& seq, std::size_t index) {
                if constexpr (SourceEncoding == upp::encoding::ascii)
                    return upp::ascii_char::from_unchecked(std::bit_cast<std::uint8_t>(seq.sequence[index]));
                else
                    return upp::uchar::from_unchecked(std::bit_cast<std::uint32_t>(seq.as_utf32[index]));
            };

            for (const auto& seq : upp_test::valid_sequences<SourceEncoding>())
            {
                {
                    std::size_t count  = 0uz;
                    auto        result = traits::validate_range(
                        seq.sequence, [&](traits::default_code_unit_type code_unit) { CHECK(code_unit == seq.sequence[count++]); });

                    REQUIRE(result.has_value());
                }

                std::size_t count = 0uz;

                auto check_decoded_value = [&](traits::char_type code_point) { CHECK(code_point == expected_decoded_value(seq, count++)); };

                auto result = traits::decode_range(seq.sequence, check_decoded_value);

                REQUIRE(result.has_value());

                count = 0uz;
                traits::decode_range_lossy(seq.sequence, check_decoded_value);

                count = 0uz;
                traits::decode_range_unchecked(seq.sequence, check_decoded_value);
            }
        });
    }

    SECTION("Decoding and validating ill-formed sequences")
    {
        upp_test::run_for_each_encoding([&]<upp::encoding SourceEncoding>() {
            using traits = upp::encoding_traits<SourceEncoding>;

            auto expected_lossily_decoded_value = [](const auto& seq, std::size_t index) {
                if constexpr (SourceEncoding == upp::encoding::ascii)
                    return upp::ascii_char::from_unchecked(std::bit_cast<std::uint8_t>(seq.as_ascii_lossy[index]));
                else
                    return upp::uchar::from_unchecked(std::bit_cast<std::uint32_t>(seq.as_utf32_lossy[index]));
            };

            for (const auto& seq : upp_test::invalid_sequences<SourceEncoding>())
            {
                {
                    std::size_t count  = 0uz;
                    auto        result = traits::validate_range(
                        seq.sequence, [&](traits::default_code_unit_type code_unit) { CHECK(code_unit == seq.sequence[count++]); });

                    REQUIRE(!result.has_value());
                    CHECK(result.error() == seq.expected_error);
                }

                std::size_t count = 0uz;

                auto check_lossily_decoded_value = [&](traits::char_type code_point) {
                    CHECK(code_point == expected_lossily_decoded_value(seq, count++));
                };

                auto result = traits::decode_range(seq.sequence, check_lossily_decoded_value);

                REQUIRE(!result.has_value());
                CHECK(result.error() == seq.expected_error);

                count = 0uz;
                traits::decode_range_lossy(seq.sequence, check_lossily_decoded_value);
            }
        });
    }
}