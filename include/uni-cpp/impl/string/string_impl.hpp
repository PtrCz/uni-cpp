#ifndef UNI_CPP_IMPL_STRING_STRING_IMPL_HPP
#define UNI_CPP_IMPL_STRING_STRING_IMPL_HPP

/// @file
///
/// @brief Implementations of string types
///

#include "fwd.hpp"
#include "string.hpp"

namespace upp
{
    namespace impl
    {
        template<string_encoding Encoding, typename Allocator>
        constexpr basic_string_base<Encoding, Allocator>::basic_string_base() noexcept(noexcept(allocator_type()))
            : basic_string_base(allocator_type())
        {
        }

        template<string_encoding Encoding, typename Allocator>
        constexpr basic_string_base<Encoding, Allocator>::basic_string_base(const allocator_type& alloc) noexcept
            : m_string{alloc}
        {
        }

        template<string_encoding Encoding, typename Allocator>
        constexpr basic_string_base<Encoding, Allocator>::basic_string_base(const basic_string_base& other)
            : m_string{other.m_string}
        {
        }

        template<string_encoding Encoding, typename Allocator>
        constexpr basic_string_base<Encoding, Allocator>::basic_string_base(const basic_string_base& other, const allocator_type& alloc)
            : m_string{other.m_string, alloc}
        {
        }

        template<string_encoding Encoding, typename Allocator>
        constexpr basic_string_base<Encoding, Allocator>::basic_string_base(basic_string_base&& other) noexcept
            : m_string{std::move(other.m_string)}
        {
            other.clear();
        }

        template<string_encoding Encoding, typename Allocator>
        constexpr basic_string_base<Encoding, Allocator>::basic_string_base(basic_string_base&& other, const allocator_type& alloc)
            : m_string{std::move(other.m_string), alloc}
        {
            other.clear();
        }

        template<string_encoding Encoding, typename Allocator>
        [[nodiscard]] constexpr Allocator basic_string_base<Encoding, Allocator>::get_allocator() const noexcept
        {
            return m_string.get_allocator();
        }

        template<string_encoding Encoding, typename Allocator>
        constexpr void basic_string_base<Encoding, Allocator>::clear() noexcept
        {
            m_string.clear();
        }
    } // namespace impl
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_STRING_IMPL_HPP