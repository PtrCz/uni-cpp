#ifndef UNI_CPP_STRING_HPP
#define UNI_CPP_STRING_HPP

#include "uchar.hpp"
#include <string>

#include "impl/unicode_data/all.hpp"
#include "impl/utility.hpp"

#include <type_traits>
#include <memory>
#include <memory_resource>

namespace upp
{
    template<typename Allocator, typename ValueType>
    concept allocator_for = requires(Allocator& alloc) {
        typename Allocator::value_type;
        alloc.deallocate(alloc.allocate(1uz), 1uz);
    } && std::is_same_v<typename Allocator::value_type, ValueType>;

    template<allocator_for<char> Allocator = std::allocator<char>>
    class basic_ascii_string;

    template<allocator_for<char8_t> Allocator = std::allocator<char8_t>>
    class basic_utf8_string;

    template<allocator_for<char16_t> Allocator = std::allocator<char16_t>>
    class basic_utf16_string;

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

    template<typename T>
    concept unicode_string_type = impl::is_unicode_string<T>::value;

    template<typename T>
    concept string_type = unicode_string_type<T> || impl::is_ascii_string<T>::value;

    namespace impl
    {
        enum class string_encoding
        {
            ascii,
            utf8,
            utf16,
            utf32
        };

        template<string_encoding>
        struct encoding_properties
        {
        };

        template<>
        struct encoding_properties<string_encoding::ascii>
        {
            using char_type      = ascii_char;
            using code_unit_type = char;

            static constexpr bool is_variable_width = false;
        };

        template<>
        struct encoding_properties<string_encoding::utf8>
        {
            using char_type      = uchar;
            using code_unit_type = char8_t;

            static constexpr bool is_variable_width = true;
        };

        template<>
        struct encoding_properties<string_encoding::utf16>
        {
            using char_type      = uchar;
            using code_unit_type = char16_t;

            static constexpr bool is_variable_width = true;
        };

        template<>
        struct encoding_properties<string_encoding::utf32>
        {
            using char_type      = uchar;
            using code_unit_type = char32_t;

            static constexpr bool is_variable_width = false;
        };

        template<string_encoding Encoding, typename Allocator>
        class basic_string_base
        {
        public:
            using code_unit_type  = encoding_properties<Encoding>::code_unit_type;
            using allocator_type  = Allocator;
            using size_type       = std::allocator_traits<Allocator>::size_type;
            using difference_type = std::allocator_traits<Allocator>::difference_type;
            using char_type       = encoding_properties<Encoding>::char_type;

        public:
        protected:
            std::basic_string<code_unit_type, std::char_traits<code_unit_type>, Allocator> m_string;
        };

        template<string_encoding Encoding, typename Allocator>
        class basic_utf_string_base : public basic_string_base<Encoding, Allocator>
        {
        private:
            using base = basic_string_base<Encoding, Allocator>;

        protected:
            using base::m_string;

        public:
            using base::base;
        };
    } // namespace impl
} // namespace upp

#endif // UNI_CPP_STRING_HPP