#ifndef UNI_CPP_IMPL_STRING_STRING_HPP
#define UNI_CPP_IMPL_STRING_STRING_HPP

#include "../../uchar.hpp"
#include <string>

#include "fwd.hpp"

#include <memory>

// DOXYGEN-PREPROCESSOR: PREPROCESSOR_START

#define UNI_CPP_IMPL_CURRENT_CLASS_NAME // here will be the class name
#define UNI_CPP_IMPL_CONSTRUCTOR        UNI_CPP_IMPL_CURRENT_CLASS_NAME
#define UNI_CPP_IMPL_DESTRUCTOR         UNI_CPP_IMPL_CURRENT_CLASS_NAME

namespace upp
{
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
        struct encoding_traits
        {
        };

        template<>
        struct encoding_traits<string_encoding::ascii>
        {
            using char_type      = ascii_char;
            using code_unit_type = char;

            static constexpr bool is_variable_width = false;
        };

        template<>
        struct encoding_traits<string_encoding::utf8>
        {
            using char_type      = uchar;
            using code_unit_type = char8_t;

            static constexpr bool is_variable_width = true;
        };

        template<>
        struct encoding_traits<string_encoding::utf16>
        {
            using char_type      = uchar;
            using code_unit_type = char16_t;

            static constexpr bool is_variable_width = true;
        };

        template<>
        struct encoding_traits<string_encoding::utf32>
        {
            using char_type      = uchar;
            using code_unit_type = char32_t;

            static constexpr bool is_variable_width = false;
        };

#undef UNI_CPP_IMPL_CURRENT_CLASS_NAME
#define UNI_CPP_IMPL_CURRENT_CLASS_NAME basic_string_base

        template<string_encoding Encoding, typename Allocator>
        class basic_string_base
        {
        private:
            static constexpr auto s_encoding = Encoding;
            using traits                     = encoding_traits<s_encoding>;

        public:
            // DOXYGEN-PREPROCESSOR: START basic_string_base

            using code_unit_type  = traits::code_unit_type;
            using allocator_type  = Allocator;
            using size_type       = std::allocator_traits<Allocator>::size_type;
            using difference_type = std::allocator_traits<Allocator>::difference_type;
            using char_type       = traits::char_type;

            // DOXYGEN-PREPROCESSOR: END basic_string_base
        protected:
            std::basic_string<code_unit_type, std::char_traits<code_unit_type>, Allocator> m_string;
        };

#undef UNI_CPP_IMPL_CURRENT_CLASS_NAME
#define UNI_CPP_IMPL_CURRENT_CLASS_NAME basic_utf_string_base

        template<string_encoding Encoding, typename Allocator>
        class basic_utf_string_base : public basic_string_base<Encoding, Allocator>
        {
        private:
            using base = basic_string_base<Encoding, Allocator>;

            static constexpr auto s_encoding = Encoding;
            using traits                     = encoding_traits<s_encoding>;

        public:
            using base::base;

            // DOXYGEN-PREPROCESSOR: START basic_utf_string_base
            // DOXYGEN-PREPROCESSOR: END basic_utf_string_base
        };
    } // namespace impl

#undef UNI_CPP_IMPL_CURRENT_CLASS_NAME
#define UNI_CPP_IMPL_CURRENT_CLASS_NAME basic_ascii_string

    template<allocator_for<char> Allocator>
    class basic_ascii_string : public impl::basic_string_base<impl::string_encoding::ascii, Allocator> // DOXYGEN-PREPROCESSOR: HIDE_INHERITANCE
    {
    private:
        static constexpr auto s_encoding = impl::string_encoding::ascii;

        using traits = impl::encoding_traits<s_encoding>;
        using base   = impl::basic_string_base<s_encoding, Allocator>;

#ifndef UNI_CPP_DOXYGEN
    public:
        using base::base;

    private:
        using base::m_string;
#else
    public:
        // DOXYGEN-PREPROCESSOR: PASTE basic_string_base
#endif
    };

#undef UNI_CPP_IMPL_CURRENT_CLASS_NAME
#define UNI_CPP_IMPL_CURRENT_CLASS_NAME basic_utf8_string

    template<allocator_for<char8_t> Allocator>
    class basic_utf8_string : public impl::basic_utf_string_base<impl::string_encoding::utf8, Allocator> // DOXYGEN-PREPROCESSOR: HIDE_INHERITANCE
    {
    private:
        static constexpr auto s_encoding = impl::string_encoding::utf8;

        using traits = impl::encoding_traits<s_encoding>;
        using base   = impl::basic_utf_string_base<s_encoding, Allocator>;

#ifndef UNI_CPP_DOXYGEN
    public:
        using base::base;

    private:
        using base::m_string;
#else
    public:
        // DOXYGEN-PREPROCESSOR: PASTE basic_string_base
        // DOXYGEN-PREPROCESSOR: PASTE basic_utf_string_base
#endif
    };

#undef UNI_CPP_IMPL_CURRENT_CLASS_NAME
#define UNI_CPP_IMPL_CURRENT_CLASS_NAME basic_utf16_string

    template<allocator_for<char16_t> Allocator>
    class basic_utf16_string : public impl::basic_utf_string_base<impl::string_encoding::utf16, Allocator> // DOXYGEN-PREPROCESSOR: HIDE_INHERITANCE
    {
    private:
        static constexpr auto s_encoding = impl::string_encoding::utf16;

        using traits = impl::encoding_traits<s_encoding>;
        using base   = impl::basic_utf_string_base<s_encoding, Allocator>;

#ifndef UNI_CPP_DOXYGEN
    public:
        using base::base;

    private:
        using base::m_string;
#else
    public:
        // DOXYGEN-PREPROCESSOR: PASTE basic_string_base
        // DOXYGEN-PREPROCESSOR: PASTE basic_utf_string_base
#endif
    };

#undef UNI_CPP_IMPL_CURRENT_CLASS_NAME
#define UNI_CPP_IMPL_CURRENT_CLASS_NAME basic_utf32_string

    template<allocator_for<char32_t> Allocator>
    class basic_utf32_string : public impl::basic_utf_string_base<impl::string_encoding::utf32, Allocator> // DOXYGEN-PREPROCESSOR: HIDE_INHERITANCE
    {
    private:
        static constexpr auto s_encoding = impl::string_encoding::utf32;

        using traits = impl::encoding_traits<s_encoding>;
        using base   = impl::basic_utf_string_base<s_encoding, Allocator>;

#ifndef UNI_CPP_DOXYGEN
    public:
        using base::base;

    private:
        using base::m_string;
#else
    public:
        // DOXYGEN-PREPROCESSOR: PASTE basic_string_base
        // DOXYGEN-PREPROCESSOR: PASTE basic_utf_string_base
#endif
    };
} // namespace upp

#undef UNI_CPP_IMPL_CURRENT_CLASS_NAME
#undef UNI_CPP_IMPL_CONSTRUCTOR
#undef UNI_CPP_IMPL_DESTRUCTOR

#endif // UNI_CPP_IMPL_STRING_STRING_HPP