#ifndef UNI_CPP_IMPL_STRING_STRING_LITERAL_HPP
#define UNI_CPP_IMPL_STRING_STRING_LITERAL_HPP

/// @file
///
/// @brief Helper types for defining user-defined string literals.
///

#include <cstddef>

namespace upp::impl
{
    template<typename CharT, std::size_t N>
    struct string_literal
    {
        using char_type = CharT;

        static constexpr std::size_t length = N - 1;

        constexpr string_literal(const CharT (&str)[N]) noexcept
        {
            for (std::size_t i = 0; i < length; ++i)
                value[i] = str[i];
        }

        CharT value[length];
    };

    template<typename CharT>
    struct string_literal<CharT, 1>
    {
        using char_type = CharT;

        constexpr string_literal(const CharT (&)[1]) noexcept {}

        static constexpr std::size_t length = 0;
    };

    template<typename CharT>
    struct string_literal<CharT, 0>
    {
    };

    template<std::size_t N>
    using string_literal_char = string_literal<char, N>;

    template<std::size_t N>
    using string_literal_char8_t = string_literal<char8_t, N>;

    template<std::size_t N>
    using string_literal_char16_t = string_literal<char16_t, N>;

    template<std::size_t N>
    using string_literal_char32_t = string_literal<char32_t, N>;
} // namespace upp::impl

#endif // UNI_CPP_IMPL_STRING_STRING_LITERAL_HPP