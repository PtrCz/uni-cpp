#ifndef UNI_CPP_IMPL_STRING_STRING_LITERAL_HPP
#define UNI_CPP_IMPL_STRING_STRING_LITERAL_HPP

/// @file
///
/// @brief Helper types for defining user-defined string literals.
///

#include <cstddef>
#include <bit>

#include "../ordinary_literal_encoding.hpp"

namespace upp::impl
{
    template<typename OriginalCharT, typename ConvertedCharT, std::size_t N>
        requires(sizeof(OriginalCharT) == sizeof(ConvertedCharT))
    struct string_literal
    {
        static constexpr std::size_t length = N - 1;

        consteval string_literal(const OriginalCharT (&str)[N]) noexcept
        {
            for (std::size_t i = 0; i < length; ++i)
                value[i] = std::bit_cast<ConvertedCharT>(str[i]);
        }

        ConvertedCharT value[length];
    };

    template<typename OriginalCharT, typename ConvertedCharT>
    struct string_literal<OriginalCharT, ConvertedCharT, 1uz>
    {
        consteval string_literal(const OriginalCharT (&)[1]) noexcept {}

        static constexpr std::size_t length = 0;
    };

    template<typename OriginalCharT, typename ConvertedCharT>
    struct string_literal<OriginalCharT, ConvertedCharT, 0uz>
    {
    };

    template<typename ConvertedCharT, std::size_t N>
    struct string_literal_ascii
    {
        static constexpr std::size_t length = N - 1;

#ifdef UNI_CPP_ASCII_IS_THE_ORDINARY_LITERAL_ENCODING

        consteval string_literal_ascii(const char (&str)[N]) noexcept
        {
            for (std::size_t i = 0; i < length; ++i)
                value[i] = std::bit_cast<ConvertedCharT>(str[i]);
        }

#endif

        consteval string_literal_ascii(const char8_t (&str)[N]) noexcept
        {
            for (std::size_t i = 0; i < length; ++i)
                value[i] = std::bit_cast<ConvertedCharT>(str[i]);
        }

        ConvertedCharT value[length];
    };

    template<typename ConvertedCharT>
    struct string_literal_ascii<ConvertedCharT, 1uz>
    {
#ifdef UNI_CPP_ASCII_IS_THE_ORDINARY_LITERAL_ENCODING

        consteval string_literal_ascii(const char (&)[1]) noexcept {}

#endif

        consteval string_literal_ascii(const char8_t (&)[1]) noexcept {}

        static constexpr std::size_t length = 0;
    };

    template<typename ConvertedCharT>
    struct string_literal_ascii<ConvertedCharT, 0uz>
    {
    };

    template<std::size_t N>
    using string_literal_utf8_as_char = string_literal<char8_t, char, N>;

    template<std::size_t N>
    using string_literal_char8_t = string_literal<char8_t, char8_t, N>;

    template<std::size_t N>
    using string_literal_char16_t = string_literal<char16_t, char16_t, N>;

    template<std::size_t N>
    using string_literal_char32_t = string_literal<char32_t, char32_t, N>;

    template<std::size_t N>
    using string_literal_ascii_as_char = string_literal_ascii<char, N>;

    template<std::size_t N>
    using string_literal_ascii_as_char8_t = string_literal_ascii<char8_t, N>;
} // namespace upp::impl

#endif // UNI_CPP_IMPL_STRING_STRING_LITERAL_HPP