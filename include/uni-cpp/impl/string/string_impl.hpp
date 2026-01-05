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
    } // namespace impl
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_STRING_IMPL_HPP