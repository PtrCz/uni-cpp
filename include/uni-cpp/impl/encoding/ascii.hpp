#ifndef UNI_CPP_IMPL_ENCODING_ASCII_HPP
#define UNI_CPP_IMPL_ENCODING_ASCII_HPP

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
        [[nodiscard]] constexpr bool operator==(const ascii_error&) const noexcept = default;
    };

    struct from_ascii_error
    {
        std::size_t valid_up_to;
        ascii_error error;

        [[nodiscard]] constexpr bool operator==(const from_ascii_error&) const noexcept = default;
    };
} // namespace upp

#endif // UNI_CPP_IMPL_ENCODING_ASCII_HPP