#ifndef UNI_CPP_IMPL_ENCODING_UTF32_HPP
#define UNI_CPP_IMPL_ENCODING_UTF32_HPP

/// @file
///
/// @brief UTF-32 validating and decoding helper functions.
///

#include <cstddef>
#include <cstdint>

namespace upp
{
    enum class utf32_error_code : std::uint8_t
    {
        encoded_surrogate,
        out_of_range,
    };

    struct utf32_error
    {
        utf32_error_code code;

        [[nodiscard]] constexpr bool operator==(const utf32_error&) const noexcept = default;
    };

    struct from_utf32_error
    {
        std::size_t valid_up_to;
        utf32_error error;

        [[nodiscard]] constexpr bool operator==(const from_utf32_error&) const noexcept = default;
    };
} // namespace upp

#endif // UNI_CPP_IMPL_ENCODING_UTF32_HPP