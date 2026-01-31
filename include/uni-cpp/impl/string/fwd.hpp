#ifndef UNI_CPP_IMPL_STRING_FWD_HPP
#define UNI_CPP_IMPL_STRING_FWD_HPP

/// @file
///
/// @brief Forward declarations of string types
///

#include "../../encoding.hpp"

#include <string>
#include <concepts>
#include <ranges>

namespace upp
{
    /// @brief Concept for identifying container types.
    ///
    /// The requirements of this concept are similar to those of [C++'s named requirements](https://en.cppreference.com/w/cpp/named_req/Container).
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<typename C>
    concept container =
        requires(C c, const C cc) {
            typename C::value_type;
            typename C::iterator;
            typename C::const_iterator;
            typename C::size_type;

            { c.begin() } -> std::same_as<typename C::iterator>;
            { c.end() } -> std::same_as<typename C::iterator>;
            { cc.begin() } -> std::same_as<typename C::const_iterator>;
            { cc.end() } -> std::same_as<typename C::const_iterator>;

            { c.size() } -> std::same_as<typename C::size_type>;
            { c.empty() } -> std::same_as<bool>;
        } && std::unsigned_integral<typename C::size_type> && std::default_initializable<C> && std::copy_constructible<C> &&
        std::assignable_from<C&, const C> && std::equality_comparable<C>;

    /// @brief Concept for identifying contiguous sequence container types.
    ///
    /// The requirements of this concept are similar to those of C++'s named requirements for
    /// [SequenceContainer](https://en.cppreference.com/w/cpp/named_req/SequenceContainer.html)s and
    /// [ContiguousContainer](https://en.cppreference.com/w/cpp/named_req/ContiguousContainer.html)s.
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<typename C>
    concept contiguous_sequence_container = container<C> && std::ranges::contiguous_range<C> &&

                                            std::constructible_from<C, typename C::iterator, typename C::iterator> &&
                                            std::constructible_from<C, std::initializer_list<typename C::value_type>> &&

                                            requires(C c, const C cc, typename C::size_type n, typename C::value_type v, typename C::iterator it,
                                                     typename C::const_iterator cit, std::initializer_list<typename C::value_type> il) {
                                                { c.assign(n, v) };
                                                { c.assign(it, it) };
                                                { c.assign(il) };

                                                { c.insert(cit, v) } -> std::same_as<typename C::iterator>;
                                                { c.insert(cit, n, v) } -> std::same_as<typename C::iterator>;
                                                { c.insert(cit, it, it) } -> std::same_as<typename C::iterator>;

                                                { c.erase(cit) } -> std::same_as<typename C::iterator>;
                                                { c.erase(cit, cit) } -> std::same_as<typename C::iterator>;

                                                { c.clear() };

                                                { c[n] } -> std::same_as<typename C::value_type&>;
                                                { cc[n] } -> std::same_as<const typename C::value_type&>;

                                                { c.data() } -> std::same_as<typename C::value_type*>;
                                                { cc.data() } -> std::same_as<const typename C::value_type*>;
                                            };

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
    template<unicode_encoding Encoding, string_compatible_container<static_cast<encoding>(Encoding)> Container =
                                            std::basic_string<typename encoding_traits<static_cast<encoding>(Encoding)>::default_code_unit_type>>
    class basic_ustring;

    /// @brief UTF-8 string type.
    ///
    /// @tparam Container Underlying container type used for storing code units.
    ///         The `Container` type must satisfy the `upp::string_compatible_container<encoding::utf8>` concept.
    ///         Default value is `std::u8string`.
    ///
    template<string_compatible_container<encoding::utf8> Container = std::u8string>
    using basic_utf8_string = basic_ustring<unicode_encoding::utf8, Container>;

    /// @brief UTF-16 string type.
    ///
    /// @tparam Container Underlying container type used for storing code units.
    ///         The `Container` type must satisfy the `upp::string_compatible_container<encoding::utf16>` concept.
    ///         Default value is `std::u16string`.
    ///
    template<string_compatible_container<encoding::utf16> Container = std::u16string>
    using basic_utf16_string = basic_ustring<unicode_encoding::utf16, Container>;

    /// @brief UTF-32 string type.
    ///
    /// @tparam Container Underlying container type used for storing code units.
    ///         The `Container` type must satisfy the `upp::string_compatible_container<encoding::utf32>` concept.
    ///         Default value is `std::u32string`.
    ///
    template<string_compatible_container<encoding::utf32> Container = std::u32string>
    using basic_utf32_string = basic_ustring<unicode_encoding::utf32, Container>;

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
    using ustring = basic_ustring<unicode_encoding::utf8, std::u8string>;
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_FWD_HPP