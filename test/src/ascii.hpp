#ifndef TEST_ASCII_HPP
#define TEST_ASCII_HPP

#include <uni-cpp/encoding.hpp>
#include <uni-cpp/string.hpp>

#include <array>
#include <string>
#include <stdexcept>
#include <bit>

namespace upp_test::ascii
{
    [[nodiscard]] consteval char operator""_char(const unsigned long long int value)
    {
        if (value > 0xFFULL)
        {
            throw std::invalid_argument("Invalid 8-bit value");
        }

        return std::bit_cast<char>(static_cast<std::uint8_t>(value));
    }

    [[nodiscard]] constexpr auto valid_sequences()
    {
        using namespace std::string_literals;

        return std::to_array<std::string>(
            {""s, "\x20"s, "\x00"s, "\x0A"s, "\x61"s, "\x41\x31"s, "\x21\x7E"s, "\x22"s, "\x61\x09\x62"s, "\x61\x00\x62"s});
    }

    [[nodiscard]] constexpr auto invalid_test_cases()
    {
        struct test_case
        {
            std::string      input;
            upp::ascii_error expected_error;
            std::string      ascii_lossy;
        };

        using namespace std::string_literals;

        return std::to_array<test_case>({
            {
                .input          = {0x71_char, 0x7A_char, 0x39_char, 0x5F_char, 0x80_char, 0x58_char},
                .expected_error = {.valid_up_to = 4},
                .ascii_lossy    = {"\x71\x7A\x39\x5F\x1A\x58"s},
            },
            {
                .input          = {0x41_char, 0x37_char, 0x23_char, 0xFF_char, 0x40_char},
                .expected_error = {.valid_up_to = 3},
                .ascii_lossy    = {"\x41\x37\x23\x1A\x40"s},
            },
            {
                .input          = {0x80_char, 0x4B_char, 0x33_char, 0x24_char, 0x5E_char},
                .expected_error = {.valid_up_to = 0},
                .ascii_lossy    = {"\x1A\x4B\x33\x24\x5E"s},
            },
            {
                .input          = {0x5A_char, 0x39_char, 0x7F_char, 0x80_char, 0x21_char},
                .expected_error = {.valid_up_to = 3},
                .ascii_lossy    = {"\x5A\x39\x7F\x1A\x21"s},
            },
            {
                .input          = {0x31_char, 0x5F_char, 0x32_char, 0x2D_char, 0xC0_char},
                .expected_error = {.valid_up_to = 4},
                .ascii_lossy    = {"\x31\x5F\x32\x2D\x1A"s},
            },
            {
                .input          = {0x40_char, 0x25_char, 0xA0_char, 0x5E_char, 0x7E_char},
                .expected_error = {.valid_up_to = 2},
                .ascii_lossy    = {"\x40\x25\x1A\x5E\x7E"s},
            },
            {
                .input          = {0x51_char, 0xE2_char, 0x52_char, 0x34_char},
                .expected_error = {.valid_up_to = 1},
                .ascii_lossy    = {"\x51\x1A\x52\x34"s},
            },
            {
                .input          = {0x38_char, 0x2A_char, 0xFF_char, 0xFF_char, 0x28_char},
                .expected_error = {.valid_up_to = 2},
                .ascii_lossy    = {"\x38\x2A\x1A\x1A\x28"s},
            },
            {
                .input          = {0x4D_char, 0x2D_char, 0x9B_char, 0x5F_char},
                .expected_error = {.valid_up_to = 2},
                .ascii_lossy    = {"\x4D\x2D\x1A\x5F"s},
            },
            {
                .input          = {0x58_char, 0x80_char, 0x59_char, 0x80_char, 0x5A_char},
                .expected_error = {.valid_up_to = 1},
                .ascii_lossy    = {"\x58\x1A\x59\x1A\x5A"s},
            },
        });
    }
} // namespace upp_test::ascii

#endif // TEST_ASCII_HPP