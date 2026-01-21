#ifndef UNI_CPP_IMPL_STRING_STRING_HPP
#define UNI_CPP_IMPL_STRING_STRING_HPP

/// @file
///
/// @brief Class definitions of string types
///

#include "../../uchar.hpp"
#include "../../encoding.hpp"

#include "fwd.hpp"

#include <type_traits>
#include <memory>
#include <span>

namespace upp
{
    template<string_compatible_container<encoding::ascii> Container>
    class basic_ascii_string
    {
    public:
        using traits_type    = encoding_traits<encoding::ascii>;
        using container_type = Container;
        using code_unit_type = Container::value_type;
        using char_type      = ascii_char;

    public:
        /// @brief Default constructor. Constructs an empty string.
        ///
        constexpr basic_ascii_string() noexcept(std::is_nothrow_default_constructible_v<container_type>)
            : m_container()
        {
        }

        /// @brief Copy constructor. Constructs the string with a copy of the contents of `other`.
        ///
        constexpr basic_ascii_string(const basic_ascii_string& other) noexcept(std::is_nothrow_copy_constructible_v<container_type>)
            : m_container{other.m_container}
        {
        }

        /// @brief Move constructor. Constructs the string with the contents of `other` using move semantics.
        ///
        /// After the move, `other` is **guaranteed** to be empty.
        ///
        /// @internal
        ///
        /// > **Internal note:** The move constructor **must** clear `other` after the move construction, otherwise
        /// > (by the standard) `other` could have garbage data in it, including invalid ASCII (which violates the invariants).
        /// > It doesn't cost us anything to state this guarantee in the public API.
        ///
        /// @endinternal
        ///
        constexpr basic_ascii_string(basic_ascii_string&& other) noexcept(std::is_nothrow_move_constructible_v<container_type>)
            : m_container{std::move(other.m_container)}
        {
            other.clear();
        }

        /// @brief Constructs an empty string with the given allocator.
        ///
        /// @param alloc is used as the allocator.
        ///
        /// @note This constructor participates in overload resolution only if the underlying container type
        /// is allocator-aware and `std::uses_allocator_v<container_type, Allocator>` is `true`.
        ///
        template<typename Allocator>
            requires std::uses_allocator_v<container_type, Allocator>
        explicit constexpr basic_ascii_string(const Allocator& alloc) noexcept(std::is_nothrow_constructible_v<container_type, const Allocator&>)
            : m_container{alloc}
        {
        }

        /// @brief Copy constructor. Constructs the string with a copy of the contents of `other`.
        ///
        /// @param alloc is used as the allocator.
        ///
        /// @note This constructor participates in overload resolution only if the underlying container type
        /// is allocator-aware and `std::uses_allocator_v<container_type, Allocator>` is `true`.
        ///
        template<typename Allocator>
            requires std::uses_allocator_v<container_type, Allocator>
        constexpr basic_ascii_string(const basic_ascii_string& other, const Allocator& alloc) noexcept(
            std::is_nothrow_constructible_v<container_type, const container_type&, const Allocator&>)
            : m_container{other.m_container, alloc}
        {
        }

        /// @brief Move constructor. Constructs the string with the contents of `other` using move semantics.
        ///
        /// After the move, `other` is **guaranteed** to be empty.
        ///
        /// @internal
        ///
        /// > **Internal note:** The move constructor **must** clear `other` after the move construction, otherwise
        /// > (by the standard) `other` could have garbage data in it, including invalid ASCII (which violates the invariants).
        /// > It doesn't cost us anything to state this guarantee in the public API.
        ///
        /// @endinternal
        ///
        /// @param alloc is used as the allocator.
        ///
        /// @note This constructor participates in overload resolution only if the underlying container type
        /// is allocator-aware and `std::uses_allocator_v<container_type, Allocator>` is `true`.
        ///
        template<typename Allocator>
            requires std::uses_allocator_v<container_type, Allocator>
        constexpr basic_ascii_string(basic_ascii_string&& other, const Allocator& alloc) noexcept(
            std::is_nothrow_constructible_v<container_type, container_type&&, const Allocator&>)
            : m_container{std::move(other.m_container), alloc}
        {
            other.clear();
        }

        /// @brief Returns a `const` reference to the underlying container.
        ///
        /// It is intended for interoperability with APIs that expect the underlying container as an input.
        ///
        /// @note The underlying container is not encoding-aware, so it is generally better
        /// to access string contents through this class’s API rather than directly through
        /// the underlying container.
        ///
        [[nodiscard]] constexpr const container_type& underlying() const noexcept { return m_container; }

        /// @brief Returns a view of the underlying code units.
        ///
        [[nodiscard]] constexpr std::span<const code_unit_type> code_units() const noexcept { return std::span<const code_unit_type>{m_container}; }

        /// @brief Removes all characters from the string.
        ///
        /// All pointers, references, and iterators are invalidated.
        ///
        constexpr void clear() noexcept { m_container.clear(); }

    private:
        Container m_container;
    };

    template<unicode_encoding Encoding, string_compatible_container<static_cast<encoding>(Encoding)> Container>
    class basic_ustring
    {
    public:
        using traits_type    = encoding_traits<static_cast<encoding>(Encoding)>;
        using container_type = Container;
        using code_unit_type = Container::value_type;
        using char_type      = uchar;

    public:
        /// @brief Default constructor. Constructs an empty string.
        ///
        constexpr basic_ustring() noexcept(std::is_nothrow_default_constructible_v<container_type>)
            : m_container()
        {
        }

        /// @brief Copy constructor. Constructs the string with a copy of the contents of `other`.
        ///
        constexpr basic_ustring(const basic_ustring& other) noexcept(std::is_nothrow_copy_constructible_v<container_type>)
            : m_container{other.m_container}
        {
        }

        /// @brief Move constructor. Constructs the string with the contents of `other` using move semantics.
        ///
        /// After the move, `other` is **guaranteed** to be empty.
        ///
        /// @internal
        ///
        /// > **Internal note:** The move constructor **must** clear `other` after the move construction, otherwise
        /// > (by the standard) `other` could have garbage data in it, including invalid UTF (which violates the invariants).
        /// > It doesn't cost us anything to state this guarantee in the public API.
        ///
        /// @endinternal
        ///
        constexpr basic_ustring(basic_ustring&& other) noexcept(std::is_nothrow_move_constructible_v<container_type>)
            : m_container{std::move(other.m_container)}
        {
            other.clear();
        }

        /// @brief Constructs an empty string with the given allocator.
        ///
        /// @param alloc is used as the allocator.
        ///
        /// @note This constructor participates in overload resolution only if the underlying container type
        /// is allocator-aware and `std::uses_allocator_v<container_type, Allocator>` is `true`.
        ///
        template<typename Allocator>
            requires std::uses_allocator_v<container_type, Allocator>
        explicit constexpr basic_ustring(const Allocator& alloc) noexcept(std::is_nothrow_constructible_v<container_type, const Allocator&>)
            : m_container{alloc}
        {
        }

        /// @brief Copy constructor. Constructs the string with a copy of the contents of `other`.
        ///
        /// @param alloc is used as the allocator.
        ///
        /// @note This constructor participates in overload resolution only if the underlying container type
        /// is allocator-aware and `std::uses_allocator_v<container_type, Allocator>` is `true`.
        ///
        template<typename Allocator>
            requires std::uses_allocator_v<container_type, Allocator>
        constexpr basic_ustring(const basic_ustring& other, const Allocator& alloc) noexcept(
            std::is_nothrow_constructible_v<container_type, const container_type&, const Allocator&>)
            : m_container{other.m_container, alloc}
        {
        }

        /// @brief Move constructor. Constructs the string with the contents of `other` using move semantics.
        ///
        /// After the move, `other` is **guaranteed** to be empty.
        ///
        /// @internal
        ///
        /// > **Internal note:** The move constructor **must** clear `other` after the move construction, otherwise
        /// > (by the standard) `other` could have garbage data in it, including invalid UTF (which violates the invariants).
        /// > It doesn't cost us anything to state this guarantee in the public API.
        ///
        /// @endinternal
        ///
        /// @param alloc is used as the allocator.
        ///
        /// @note This constructor participates in overload resolution only if the underlying container type
        /// is allocator-aware and `std::uses_allocator_v<container_type, Allocator>` is `true`.
        ///
        template<typename Allocator>
            requires std::uses_allocator_v<container_type, Allocator>
        constexpr basic_ustring(basic_ustring&&  other,
                                const Allocator& alloc) noexcept(std::is_nothrow_constructible_v<container_type, container_type&&, const Allocator&>)
            : m_container{std::move(other.m_container), alloc}
        {
            other.clear();
        }

        /// @brief Returns a `const` reference to the underlying container.
        ///
        /// It is intended for interoperability with APIs that expect the underlying container as an input.
        ///
        /// @note The underlying container is not encoding-aware, so it is generally better
        /// to access string contents through this class’s API rather than directly through
        /// the underlying container.
        ///
        [[nodiscard]] constexpr const container_type& underlying() const noexcept { return m_container; }

        /// @brief Returns a view of the underlying code units.
        ///
        [[nodiscard]] constexpr std::span<const code_unit_type> code_units() const noexcept { return std::span<const code_unit_type>{m_container}; }

        /// @brief Removes all characters from the string.
        ///
        /// All pointers, references, and iterators are invalidated.
        ///
        constexpr void clear() noexcept { m_container.clear(); }

    private:
        Container m_container;
    };
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_STRING_HPP