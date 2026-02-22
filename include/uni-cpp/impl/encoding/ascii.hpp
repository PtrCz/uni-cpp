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
} // namespace upp

#endif // UNI_CPP_IMPL_ASCII_HPP