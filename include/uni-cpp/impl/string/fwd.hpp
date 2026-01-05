#ifndef UNI_CPP_IMPL_STRING_FWD_HPP
#define UNI_CPP_IMPL_STRING_FWD_HPP

#include "../utility.hpp"

#include <type_traits>
#include <memory>
#include <memory_resource>

namespace upp
{
    /// @brief Concept that checks whether `Allocator` is an allocator type for `ValueType` type.
    ///
    /// @tparam ValueType The value type the allocator must allocate.
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<typename Allocator, typename ValueType>
    concept allocator_for = requires(Allocator& alloc) {
        typename Allocator::value_type;
        alloc.deallocate(alloc.allocate(1uz), 1uz);
    } && std::is_same_v<typename Allocator::value_type, ValueType>;

    /// @brief ASCII string type.
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<allocator_for<char> Allocator = std::allocator<char>>
    class basic_ascii_string;

    /// @brief UTF-8 string type.
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<allocator_for<char8_t> Allocator = std::allocator<char8_t>>
    class basic_utf8_string;

    /// @brief UTF-16 string type.
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<allocator_for<char16_t> Allocator = std::allocator<char16_t>>
    class basic_utf16_string;

    /// @brief UTF-32 string type.
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<allocator_for<char32_t> Allocator = std::allocator<char32_t>>
    class basic_utf32_string;

    namespace impl
    {
        template<typename>
        struct is_unicode_string : std::false_type
        {
        };

        template<allocator_for<char8_t> Allocator>
        struct is_unicode_string<basic_utf8_string<Allocator>> : std::true_type
        {
        };

        template<allocator_for<char16_t> Allocator>
        struct is_unicode_string<basic_utf16_string<Allocator>> : std::true_type
        {
        };

        template<allocator_for<char32_t> Allocator>
        struct is_unicode_string<basic_utf32_string<Allocator>> : std::true_type
        {
        };

        template<typename>
        struct is_ascii_string : std::false_type
        {
        };

        template<allocator_for<char> Allocator>
        struct is_ascii_string<basic_ascii_string<Allocator>> : std::true_type
        {
        };
    } // namespace impl

    /// @brief Concept for identifying UTF-encoded string types defined by the uni-cpp library.
    ///
    /// This concept checks whether the type `T` is a template instantiation of
    /// `basic_utf8_string`, `basic_utf16_string`, or `basic_utf32_string`.
    ///
    /// @see basic_utf8_string, basic_utf16_string, basic_utf32_string
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<typename T>
    concept unicode_string_type = impl::is_unicode_string<T>::value;

    /// @brief Concept for identifying string types defined by the uni-cpp library.
    ///
    /// This concept checks whether the type `T` is a template instantiation of
    /// `basic_ascii_string`, `basic_utf8_string`, `basic_utf16_string`, or `basic_utf32_string`.
    ///
    /// @see basic_ascii_string, basic_utf8_string, basic_utf16_string, basic_utf32_string
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<typename T>
    concept string_type = unicode_string_type<T> || impl::is_ascii_string<T>::value;

    template<allocator_for<char8_t> Allocator = std::allocator<char8_t>>
    using basic_ustring = basic_utf8_string<Allocator>;

    using ascii_string = basic_ascii_string<>;
    using utf8_string  = basic_utf8_string<>;
    using utf16_string = basic_utf16_string<>;
    using utf32_string = basic_utf32_string<>;
    using ustring      = basic_ustring<>;

    namespace pmr
    {
        using ascii_string = basic_ascii_string<std::pmr::polymorphic_allocator<char>>;
        using utf8_string  = basic_utf8_string<std::pmr::polymorphic_allocator<char8_t>>;
        using utf16_string = basic_utf16_string<std::pmr::polymorphic_allocator<char16_t>>;
        using utf32_string = basic_utf32_string<std::pmr::polymorphic_allocator<char32_t>>;
        using ustring      = basic_ustring<std::pmr::polymorphic_allocator<char8_t>>;
    } // namespace pmr
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_FWD_HPP