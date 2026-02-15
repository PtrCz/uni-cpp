#ifndef TEST_TEST_DATA_HPP
#define TEST_TEST_DATA_HPP

#include <uni-cpp/uchar.hpp>

#include <string_view>
#include <unordered_map>
#include <vector>
#include <filesystem>

#include <concepts>
#include <charconv>
#include <system_error>
#include <print>
#include <fstream>
#include <ranges>
#include <utility>
#include <type_traits>
#include <functional>
#include <cassert>

namespace upp_test
{
    namespace impl
    {
        template<std::unsigned_integral T>
        T parse_hex(std::string_view str)
        {
            T num;

            auto result = std::from_chars(str.data(), str.data() + str.size(), num, 16);

            assert(result.ec == std::errc{});

            return num;
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
        assert(file.is_open());

        std::unordered_map<std::uint32_t, std::vector<ValueType>> result;

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty())
                continue;

            std::size_t colon_pos = line.find(':');
            assert(colon_pos != std::string::npos);

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
} // namespace upp_test

#endif // TEST_TEST_DATA_HPP