#ifndef UNI_CPP_ENCODING_HPP
#define UNI_CPP_ENCODING_HPP

/// @file
///
/// @brief Provides enumerations of text encodings and traits for those encodings.
///

#include "uchar.hpp"

#include <cstdint>
#include <utility>

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
} // namespace upp

#endif // UNI_CPP_ENCODING_HPP