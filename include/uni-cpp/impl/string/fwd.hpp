#ifndef UNI_CPP_IMPL_STRING_FWD_HPP
#define UNI_CPP_IMPL_STRING_FWD_HPP

/// @file
///
/// @brief Forward declarations of string types
///

#include "../../encoding.hpp"
#include "container.hpp"

#include <string>
#include <concepts>
#include <ranges>

namespace upp
{
    /// @brief Concept for identifying container types that work with uni-cpp's string types.
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<typename C, encoding E>
    concept string_compatible_container = contiguous_sequence_container<C> && code_unit_type_for<typename C::value_type, E>;

    /// @brief ASCII string type.
    ///
    /// @tparam Container Underlying container type used for storing character codes.
    ///         The `Container` type must satisfy the `upp::string_compatible_container<encoding::ascii>` concept.
    ///         Default value is `std::string`.
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<string_compatible_container<encoding::ascii> Container = std::string>
    class basic_ascii_string;

    /// @brief Unicode string type.
    ///
    /// @tparam Encoding Used for encoding the Unicode code points.
    /// @tparam Container Underlying container type used for storing code units.
    ///         The `Container` type must satisfy the `upp::string_compatible_container<Encoding>` concept.
    ///         Default value is `std::basic_string<typename encoding_traits<Encoding>::default_code_unit_type>`.
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<encoding                              Encoding,
             string_compatible_container<Encoding> Container = std::basic_string<typename encoding_traits<Encoding>::default_code_unit_type>>
        requires unicode_encoding<Encoding>
    class basic_ustring;

    /// @brief UTF-8 string type.
    ///
    /// @tparam Container Underlying container type used for storing code units.
    ///         The `Container` type must satisfy the `upp::string_compatible_container<encoding::utf8>` concept.
    ///         Default value is `std::u8string`.
    ///
    template<string_compatible_container<encoding::utf8> Container = std::u8string>
    using basic_utf8_string = basic_ustring<encoding::utf8, Container>;

    /// @brief UTF-16 string type.
    ///
    /// @tparam Container Underlying container type used for storing code units.
    ///         The `Container` type must satisfy the `upp::string_compatible_container<encoding::utf16>` concept.
    ///         Default value is `std::u16string`.
    ///
    template<string_compatible_container<encoding::utf16> Container = std::u16string>
    using basic_utf16_string = basic_ustring<encoding::utf16, Container>;

    /// @brief UTF-32 string type.
    ///
    /// @tparam Container Underlying container type used for storing code units.
    ///         The `Container` type must satisfy the `upp::string_compatible_container<encoding::utf32>` concept.
    ///         Default value is `std::u32string`.
    ///
    template<string_compatible_container<encoding::utf32> Container = std::u32string>
    using basic_utf32_string = basic_ustring<encoding::utf32, Container>;

    /// @brief Default ASCII string type.
    ///
    using ascii_string = basic_ascii_string<>;

    /// @brief Default UTF-8 string type.
    ///
    using utf8_string = basic_utf8_string<>;

    /// @brief Default UTF-16 string type.
    ///
    using utf16_string = basic_utf16_string<>;

    /// @brief Default UTF-32 string type.
    ///
    using utf32_string = basic_utf32_string<>;

    /// @brief Default Unicode string type. Uses the UTF-8 encoding.
    ///
    using ustring = basic_ustring<encoding::utf8, std::u8string>;
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_FWD_HPP