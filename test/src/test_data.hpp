#ifndef TEST_TEST_DATA_HPP
#define TEST_TEST_DATA_HPP

#include <uni-cpp/uchar.hpp>
#include <uni-cpp/encoding.hpp>

#include "catch2.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <bit>

#include <concepts>
#include <charconv>
#include <system_error>
#include <print>
#include <fstream>
#include <ranges>
#include <utility>
#include <type_traits>
#include <functional>

namespace upp_test
{
    namespace impl
    {
        template<std::unsigned_integral T>
        T parse_hex(std::string_view str)
        {
            T num{};

            auto result = std::from_chars(str.data(), str.data() + str.size(), num, 16);

            REQUIRE(result.ec == std::errc{});

            return num;
        }

        constexpr void remove_comment(std::string& line)
        {
            if (const std::size_t hash_pos = line.find('#'); hash_pos != std::string::npos)
            {
                line = line.substr(0, hash_pos);
            }
        }
    } // namespace impl

    template<std::unsigned_integral ValueType>
    [[nodiscard]] std::unordered_map<std::uint32_t, std::vector<ValueType>> load_test_data(const std::filesystem::path& filepath)
    {
        // Note: test data files have the following format:
        //
        // <code point>:<value>;[<value>;...]
        //
        // or in proper regex:
        //
        // ^[0-9A-Fa-f]+:([0-9A-Fa-f]+;)+$

        std::println("Loading test data from file: {}", filepath.generic_string());

        std::ifstream file{filepath};
        REQUIRE(file.is_open());

        std::unordered_map<std::uint32_t, std::vector<ValueType>> result;

        std::string line;
        while (std::getline(file, line))
        {
            impl::remove_comment(line);

            if (line.empty())
                continue;

            std::size_t colon_pos = line.find(':');
            REQUIRE(colon_pos != std::string::npos);

            using diff_t = std::string::difference_type;

            std::string_view code_point_str{line.begin(), line.begin() + static_cast<diff_t>(colon_pos)};
            std::string_view values_str{line.begin() + static_cast<diff_t>(colon_pos + 1), line.end()};

            std::uint32_t code_point = impl::parse_hex<std::uint32_t>(code_point_str);

            std::vector<ValueType> values;
            values.reserve(4uz);

            for (auto value_chars : std::views::split(values_str, ';'))
            {
                std::string_view value_sv{value_chars};

                if (!value_sv.empty())
                    values.push_back(impl::parse_hex<ValueType>(value_sv));
            }

            result[code_point] = std::move(values);
        }

        return result;
    }

    struct NormalizationTestCase
    {
        std::u32string source;
        std::u32string nfc;
        std::u32string nfd;
        std::u32string nfkc;
        std::u32string nfkd;
    };

    [[nodiscard]] inline const std::vector<NormalizationTestCase>& load_normalization_test_data()
    {
        static const auto result = [] {
            const std::filesystem::path filepath{"test_data/ucd/NormalizationTest.txt"};

            std::println("Loading normalization test data from file: {}", filepath.generic_string());

            std::ifstream file{filepath};
            REQUIRE(file.is_open());

            std::vector<NormalizationTestCase> result;

            std::string line;
            while (std::getline(file, line))
            {
                impl::remove_comment(line);

                if (line.empty() || line.starts_with('@'))
                    continue;

                NormalizationTestCase test_case{};

                auto fields = line | std::views::split(';');

                auto output_strings = std::array{&test_case.source, &test_case.nfc, &test_case.nfd, &test_case.nfkc, &test_case.nfkd};

                auto count = 0uz;

                auto parse_code_point = [](auto&& code_point) {
                    return std::bit_cast<char32_t>(impl::parse_hex<std::uint32_t>(std::string{std::from_range, code_point}));
                };

                for (auto&& [field, output_str] : std::views::zip(fields, output_strings))
                {
                    output_str->assign_range(field | std::views::split(' ') | std::views::transform(parse_code_point));

                    REQUIRE(upp::encoding_traits<upp::encoding::utf32>::validate_range(*output_str).has_value());

                    ++count;
                }

                REQUIRE(count == 5uz);

                result.push_back(std::move(test_case));
            }

            return result;
        }();

        return result;
    }
} // namespace upp_test

#endif // TEST_TEST_DATA_HPP