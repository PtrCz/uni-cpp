#ifndef UNI_CPP_IMPL_ENCODING_UTF16_HPP
#define UNI_CPP_IMPL_ENCODING_UTF16_HPP

/// @file
///
/// @brief UTF-16 validating and decoding helper functions.
///

#include <cstddef>
#include <cstdint>
#include <optional>

namespace upp
{
    enum class utf16_error_code : std::uint8_t
    {
        unpaired_high_surrogate,
        unpaired_low_surrogate,
    };

    struct utf16_error
    {
        std::optional<std::uint8_t> length;
        utf16_error_code            code;

        [[nodiscard]] constexpr bool operator==(const utf16_error&) const noexcept = default;
    };

    struct from_utf16_error
    {
        std::size_t valid_up_to;
        utf16_error error;

        [[nodiscard]] constexpr bool operator==(const from_utf16_error&) const noexcept = default;
    };

    namespace impl::utf16
    {
        [[nodiscard]] constexpr bool is_surrogate(char16_t code_unit) noexcept
        {
            return code_unit >= 0xD800U && code_unit <= 0xDFFFU;
        }

        [[nodiscard]] constexpr bool is_high_surrogate(char16_t code_unit) noexcept
        {
            return code_unit >= 0xD800U && code_unit <= 0xDBFFU;
        }

        [[nodiscard]] constexpr bool is_low_surrogate(char16_t code_unit) noexcept
        {
            return code_unit >= 0xDC00U && code_unit <= 0xDFFFU;
        }

        [[nodiscard]] constexpr std::uint32_t decode_valid_surrogate_pair(char16_t high_surrogate, char16_t low_surrogate) noexcept
        {
            return ((static_cast<std::uint32_t>(high_surrogate & 0x3FFU) << 10) | static_cast<std::uint32_t>(low_surrogate & 0x3FFU)) + 0x10'000U;
        }
    } // namespace impl::utf16
} // namespace upp

#endif // UNI_CPP_IMPL_ENCODING_UTF16_HPP