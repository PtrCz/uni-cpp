#ifndef UNI_CPP_IMPL_STRING_STRING_LITERAL_HPP
#define UNI_CPP_IMPL_STRING_STRING_LITERAL_HPP

/// @file
///
/// @brief Helper types for defining user-defined string literals.
///

#include <cstddef>
#include <bit>

namespace upp::impl
{
    template<typename OriginalCharT, typename ConvertedCharT, std::size_t N>
        requires(sizeof(OriginalCharT) == sizeof(ConvertedCharT))
    struct string_literal
    {
        static constexpr std::size_t length = N - 1;

        constexpr string_literal(const OriginalCharT (&str)[N]) noexcept
        {
            for (std::size_t i = 0; i < length; ++i)
                value[i] = std::bit_cast<ConvertedCharT>(str[i]);
        }

        ConvertedCharT value[length];
    };

    template<typename OriginalCharT, typename ConvertedCharT>
    struct string_literal<OriginalCharT, ConvertedCharT, 1uz>
    {
        constexpr string_literal(const OriginalCharT (&)[1]) noexcept {}

        static constexpr std::size_t length = 0;
    };

    template<typename OriginalCharT, typename ConvertedCharT>
    struct string_literal<OriginalCharT, ConvertedCharT, 0uz>
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
} // namespace upp::impl

#endif // UNI_CPP_IMPL_STRING_STRING_LITERAL_HPP