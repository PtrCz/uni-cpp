#ifndef UNI_CPP_IMPL_UTF16_HPP
#define UNI_CPP_IMPL_UTF16_HPP

/// @file
///
/// @brief UTF-16 validating and decoding helper functions.
///

#include <cstddef>
#include <cstdint>

namespace upp
{
    struct utf16_error
    {
        std::size_t valid_up_to;

        [[nodiscard]] constexpr bool operator==(const utf16_error&) const noexcept = default;
    };

    namespace impl::utf16
    {
        [[nodiscard]] constexpr bool is_surrogate(char16_t code_unit) noexcept
        {
            return code_unit >= 0xD800U && code_unit <= 0xDFFFU;
        }
    } // namespace impl::utf16
} // namespace upp

#endif // UNI_CPP_IMPL_UTF16_HPP