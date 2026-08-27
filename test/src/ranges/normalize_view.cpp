#include "../catch2.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>

#include <uni-cpp/ranges.hpp>

#include "base.hpp"
#include "to_input.hpp"
#include "../test_data.hpp"

#include <filesystem>
#include <fstream>

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

#define BENCHMARK_NORMALIZATION(lipsum, bytes, code_points)                                                                                          \
    BENCHMARK_ADVANCED("Lazy UTF-8-to-UTF-8 NFC normalization: " lipsum " (" bytes "; " code_points ")")(Catch::Benchmark::Chronometer meter)        \
    {                                                                                                                                                \
        const std::filesystem::path path{"benchmark_data/lipsum/data/" lipsum "-Lipsum.utf8.txt"};                                                   \
                                                                                                                                                     \
        std::ifstream file(path, std::ios::binary);                                                                                                  \
                                                                                                                                                     \
        REQUIRE(file);                                                                                                                               \
                                                                                                                                                     \
        std::string str(std::filesystem::file_size(path), '\0');                                                                                     \
        file.read(str.data(), str.size());                                                                                                           \
                                                                                                                                                     \
        std::string_view input{str};                                                                                                                 \
                                                                                                                                                     \
        meter.measure([&] {                                                                                                                          \
            std::uintmax_t sum{}; /* sum all elements to prevent optimization */                                                                     \
                                                                                                                                                     \
            for (char8_t code_unit :                                                                                                                 \
                 input | upp::views::mark_as_valid_utf8 | upp::views::decode_valid_utf8 | upp::views::normalize_to_nfc | upp::views::encode_as_utf8) \
            {                                                                                                                                        \
                sum += static_cast<std::uintmax_t>(code_unit);                                                                                       \
            }                                                                                                                                        \
                                                                                                                                                     \
            return sum;                                                                                                                              \
        });                                                                                                                                          \
    }

TEST_CASE("normalize_view benchmark", "[ranges][normalization][!benchmark]")
{
    BENCHMARK_NORMALIZATION("Arabic", "81'685 bytes", "45'764 code points");
    BENCHMARK_NORMALIZATION("Chinese", "69'840 bytes", "23'460 code points");
    BENCHMARK_NORMALIZATION("Emoji", "65'542 bytes", "16'386 code points");
    BENCHMARK_NORMALIZATION("Hebrew", "66'495 bytes", "37'305 code points");
    BENCHMARK_NORMALIZATION("Hindi", "87'997 bytes", "32'765 code points");
    BENCHMARK_NORMALIZATION("Japanese", "67'808 bytes", "23'374 code points");
    BENCHMARK_NORMALIZATION("Korean", "66'600 bytes", "27'144 code points");
    BENCHMARK_NORMALIZATION("Latin", "86'940 bytes", "86'940 code points");
    BENCHMARK_NORMALIZATION("Russian", "104'770 bytes", "57'980 code points");
}