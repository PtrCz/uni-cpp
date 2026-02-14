#ifndef UNI_CPP_ENCODING_HPP
#define UNI_CPP_ENCODING_HPP

/// @file
///
/// @brief Provides enumerations of text encodings and traits for those encodings.
///

#include "uchar.hpp"

#include <cstdint>
#include <utility>
#include <concepts>

namespace upp
{
    /// @brief Enumeration of text encodings.
    ///
    /// @see unicode_encoding
    ///
    enum class encoding : std::uint8_t
    {
        ascii, ///< ASCII encoding
        utf8,  ///< UTF-8 encoding
        utf16, ///< UTF-16 encoding
        utf32  ///< UTF-32 encoding
    };

    /// @brief Enumeration of Unicode text encodings.
    ///
    /// @see encoding
    ///
    enum class unicode_encoding : std::uint8_t
    {
        utf8  = std::to_underlying(encoding::utf8),  ///< UTF-8 encoding, equal to `encoding::utf8`
        utf16 = std::to_underlying(encoding::utf16), ///< UTF-16 encoding, equal to `encoding::utf16`
        utf32 = std::to_underlying(encoding::utf32)  ///< UTF-32 encoding, equal to `encoding::utf32`
    };

    /// @brief Provides traits for a given text encoding.
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<encoding Encoding>
    struct encoding_traits;

    /// @copydoc encoding_traits
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<>
    struct encoding_traits<encoding::ascii>
    {
        /// The uni-cpp type for storing code points of a given encoding.
        using char_type = ascii_char;

        /// Built-in integral type for storing code units of a given encoding.
        using default_code_unit_type = char;

        /// `true` for [variable-width encodings](https://en.wikipedia.org/wiki/Variable-length_encoding), otherwise `false`.
        static constexpr bool is_variable_width = false;
    };

    /// @copydoc encoding_traits
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<>
    struct encoding_traits<encoding::utf8>
    {
        /// The uni-cpp type for storing code points of a given encoding.
        using char_type = uchar;

        /// Built-in integral type for storing code units of a given encoding.
        using default_code_unit_type = char8_t;

        /// `true` for [variable-width encodings](https://en.wikipedia.org/wiki/Variable-length_encoding), otherwise `false`.
        static constexpr bool is_variable_width = true;
    };

    /// @copydoc encoding_traits
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<>
    struct encoding_traits<encoding::utf16>
    {
        /// The uni-cpp type for storing code points of a given encoding.
        using char_type = uchar;

        /// Built-in integral type for storing code units of a given encoding.
        using default_code_unit_type = char16_t;

        /// `true` for [variable-width encodings](https://en.wikipedia.org/wiki/Variable-length_encoding), otherwise `false`.
        static constexpr bool is_variable_width = true;
    };

    /// @copydoc encoding_traits
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<>
    struct encoding_traits<encoding::utf32>
    {
        /// The uni-cpp type for storing code points of a given encoding.
        using char_type = uchar;

        /// Built-in integral type for storing code units of a given encoding.
        using default_code_unit_type = char32_t;

        /// `true` for [variable-width encodings](https://en.wikipedia.org/wiki/Variable-length_encoding), otherwise `false`.
        static constexpr bool is_variable_width = false;
    };

    /// @brief Identifies types that could be used as a code unit type for a given encoding.
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<typename T, encoding E>
    concept code_unit_type_for = std::integral<T> && sizeof(T) == sizeof(typename encoding_traits<E>::default_code_unit_type);

    namespace impl
    {
        template<std::integral T, unicode_encoding SourceEncoding, unicode_encoding TargetEncoding>
        inline constexpr T utf_transcoding_upper_bound_size_hint_factor = []() {
            // Each line after the "|||" has every case written out. The number in the parentheses is the calculated transcoding factor.
            // For a given source encoding and target encoding pair, the greatest transcoding factor is always chosen.
            //
            // From UTF-8:
            //     1. to UTF-8:  1    |||    1 -> 1 (1)    or    2 -> 2 ( 1 )    or    3 -> 3 ( 1 )    or    4 -> 4 (1)
            //     2. to UTF-16: 1    |||    1 -> 1 (1)    or    2 -> 1 (1/2)    or    3 -> 1 (1/3)    or    4 -> 2 (1/2)
            //     3. to UTF-32: 1    |||    1 -> 1 (1)    or    2 -> 1 (1/2)    or    3 -> 1 (1/3)    or    4 -> 1 (1/4)
            //
            // From UTF-16:
            //     4. to UTF-8:  3    |||    1 -> 1 (1)    or    1 -> 2 ( 2 )    or    1 -> 3 (3)    or    2 -> 4 (2)
            //     5. to UTF-16: 1    |||    1 -> 1 (1)    or    2 -> 2 ( 1 )
            //     6. to UTF-32: 1    |||    1 -> 1 (1)    or    2 -> 1 (1/2)
            //
            // From UTF-32:
            //     7. to UTF-8:  4    |||    1 -> 1 (1)    or    1 -> 2 (2)    or    1 -> 3 (3)    or    1 -> 4 (4)
            //     8. to UTF-16: 2    |||    1 -> 1 (1)    or    1 -> 2 (2)
            //     9. to UTF-32: 1    |||    1 -> 1 (1)

            if constexpr (SourceEncoding == TargetEncoding || TargetEncoding == unicode_encoding::utf32 ||
                          SourceEncoding == unicode_encoding::utf8) // 1, 2, 3, 5, 6, 9
                return 1;
            else if constexpr (SourceEncoding == unicode_encoding::utf16) // 4
                return 3;
            else if constexpr (TargetEncoding == unicode_encoding::utf8) // 7
                return 4;
            else if constexpr (TargetEncoding == unicode_encoding::utf16) // 8
                return 2;
        }();

        template<std::integral T, unicode_encoding SourceEncoding, unicode_encoding TargetEncoding>
        inline constexpr T utf_transcoding_lower_bound_size_hint_divisor = []() {
            // Look at all the cases written above. This time we choose the smallest factor.
            //
            // We write the factor as a fraction and we use the divisor as the result.
            // The dividend is 1 in all cases so there is no need to have a query for it.
            //
            // From UTF-8:
            //     1. to UTF-8:  1 (1/1)
            //     2. to UTF-16: 3 (1/3)
            //     3. to UTF-32: 4 (1/4)
            //
            // From UTF-16:
            //     4. to UTF-8:  1 (1/1)
            //     5. to UTF-16: 1 (1/1)
            //     6. to UTF-32: 2 (1/2)
            //
            // From UTF-32:
            //     7. to UTF-8:  1 (1/1)
            //     8. to UTF-16: 1 (1/1)
            //     9. to UTF-32: 1 (1/1)

            return utf_transcoding_upper_bound_size_hint_factor<T, TargetEncoding, SourceEncoding>;
        }();
    } // namespace impl
} // namespace upp

#endif // UNI_CPP_ENCODING_HPP