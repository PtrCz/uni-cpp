#ifndef UNI_CPP_IMPL_UTF32_HPP
#define UNI_CPP_IMPL_UTF32_HPP

/// @file
///
/// @brief UTF-32 validating and decoding helper functions.
///

#include <cstddef>
#include <cstdint>

namespace upp
{
    struct utf32_error
    {
        std::size_t valid_up_to;

        [[nodiscard]] constexpr bool operator==(const utf32_error&) const noexcept = default;
    };
} // namespace upp

#endif // UNI_CPP_IMPL_UTF32_HPP