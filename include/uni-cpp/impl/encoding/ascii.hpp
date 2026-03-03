#ifndef UNI_CPP_IMPL_ASCII_HPP
#define UNI_CPP_IMPL_ASCII_HPP

/// @file
///
/// @brief ASCII validating and decoding helper functions.
///

#include <cstddef>
#include <cstdint>

namespace upp
{
    struct ascii_error
    {
        std::size_t valid_up_to;

        [[nodiscard]] constexpr bool operator==(const ascii_error&) const noexcept = default;
    };

    namespace impl::ascii
    {
        [[nodiscard]] consteval bool ordinary_string_literal_is_ascii() noexcept
        {
            return '0' == 0x30 && 'A' == 0x41 && 'Z' == 0x5A && 'a' == 0x61 && 'z' == 0x7A;
        }
    } // namespace impl::ascii
} // namespace upp

#endif // UNI_CPP_IMPL_ASCII_HPP