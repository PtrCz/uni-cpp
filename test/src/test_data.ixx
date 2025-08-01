module;

#include <catch2/catch_test_macros.hpp>

export module test_data;

export import std;
export import uni_cpp;

namespace upp_test
{
    template<std::unsigned_integral T>
    T parse_hex(std::string_view str)
    {
        T num;

        auto result = std::from_chars(str.data(), str.data() + str.size(), num, 16);

        REQUIRE(result.ec == std::errc{});

        return num;
    }

    constexpr auto unicode_scalar_values = std::views::join(std::array{std::views::iota(std::uint32_t{0x0000}, std::uint32_t{0xD800}),
                                                                       std::views::iota(std::uint32_t{0xE000}, std::uint32_t{0x110000})}) |
                                           std::views::transform([](std::uint32_t code_point) { return upp::uchar::from_unchecked(code_point); });

    export {
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
                if (line.empty())
                    continue;

                std::size_t colon_pos = line.find(':');
                REQUIRE(colon_pos != std::string::npos);

                std::string_view code_point_str{line.begin(), line.begin() + colon_pos};
                std::string_view values_str{line.begin() + colon_pos + 1, line.end()};

                std::uint32_t code_point = parse_hex<std::uint32_t>(code_point_str);

                std::vector<ValueType> values;
                values.reserve(4uz);

                for (auto value_chars : std::views::split(values_str, ';'))
                {
                    std::string_view value_sv{value_chars};

                    if (!value_sv.empty())
                        values.push_back(parse_hex<ValueType>(value_sv));
                }

                result[code_point] = std::move(values);
            }

            return result;
        }

        template<typename F>
            requires std::invocable<F, upp::uchar> && std::ranges::range<std::invoke_result_t<F, upp::uchar>> &&
                     std::unsigned_integral<std::ranges::range_value_t<std::invoke_result_t<F, upp::uchar>>>
        void test_using_data_from_file(const std::filesystem::path& filepath, F func)
        {
            using integer_t = std::ranges::range_value_t<std::invoke_result_t<F, upp::uchar>>;

            const auto test_data = load_test_data<integer_t>(filepath);

            for (upp::uchar ch : upp_test::unicode_scalar_values)
            {
                CHECK(std::ranges::equal(std::invoke(func, ch), test_data.at(ch.value())));
            }
        }
    }
} // namespace upp_test