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
#include <expected>
#include <ranges>
#include <utility>
#include <concepts>
#include <bit>

namespace upp
{
    namespace impl
    {
        struct from_container_t
        {
            explicit constexpr from_container_t() noexcept = default;
        };

        inline constexpr from_container_t from_container{};
    } // namespace impl

    template<string_compatible_container<encoding::ascii> Container>
    class basic_ascii_string
    {
    public:
        using traits_type    = encoding_traits<encoding::ascii>;
        using container_type = Container;
        using size_type      = Container::size_type;
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

    namespace impl
    {
        /// @brief Serves as a namespace for the implementation of `basic_ustring`.
        ///
        /// The reason for this not being a `namespace` is that then we would need to `friend` all functions
        /// in it one by one. And the reason for this not being private functions in `basic_ustring` is
        /// because then all those functions would be `template`s depending on the template parameters of
        /// the `basic_ustring`, which could cause unnecessarily many template instantiations of those functions.
        ///
        class basic_ustring_impl;
    } // namespace impl

    struct utf8_error
    {
        std::size_t                 valid_up_to;
        std::optional<std::uint8_t> error_length;

        [[nodiscard]] constexpr bool operator==(const utf8_error&) const noexcept = default;
    };

    template<unicode_encoding Encoding, string_compatible_container<static_cast<encoding>(Encoding)> Container>
    class basic_ustring
    {
    public:
        using traits_type    = encoding_traits<static_cast<encoding>(Encoding)>;
        using container_type = Container;
        using size_type      = Container::size_type;
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

        /// @brief Constructs a `basic_ustring` from UTF-8 encoded data with error checking.
        ///
        /// @return `std::expected` containing the string on success, or a `utf8_error` on failure.
        ///
        /// If you are absolutely certain that `range` is valid UTF-8, you can use `from_utf8_unchecked` instead.
        ///
        /// @see from_utf8_unchecked
        ///
        /// @tparam Range Input range of UTF-8 code units. Needs to satisfy `std::ranges::input_range` and
        /// `upp::code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, upp::encoding::utf8>`.
        ///
        template<std::ranges::input_range Range>
            requires code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, encoding::utf8>
        [[nodiscard]] static constexpr std::expected<basic_ustring, utf8_error> from_utf8(Range&& range);

        /// @brief Constructs a `basic_ustring` from UTF-8 encoded data without error checking.
        ///
        /// @pre `range` MUST be valid UTF-8.
        ///
        /// @warning If the precondition of this function isn't met, the behavior is undefined.
        /// Use `from_utf8` as a safe alternative that performs validation.
        ///
        /// @see from_utf8
        ///
        /// @tparam Range Input range of UTF-8 code units. Needs to satisfy `std::ranges::input_range` and
        /// `upp::code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, upp::encoding::utf8>`.
        ///
        template<std::ranges::input_range Range>
            requires code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, encoding::utf8>
        [[nodiscard]] static constexpr basic_ustring from_utf8_unchecked(Range&& range);

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
        /// @brief Constructs the string directly from the underlying container type.
        ///
        constexpr basic_ustring(impl::from_container_t, const Container& other) noexcept(std::is_nothrow_copy_constructible_v<Container>)
            : m_container{other}
        {
        }

        /// @brief Constructs the string directly from the underlying container type.
        ///
        constexpr basic_ustring(impl::from_container_t, Container&& other) noexcept(std::is_nothrow_move_constructible_v<Container>)
            : m_container{std::move(other)}
        {
        }

        /// @brief Appends a single code unit to the end of the string.
        ///
        template<typename T>
            requires code_unit_type_for<T, static_cast<encoding>(Encoding)>
        constexpr void push_back_code_unit(T code_unit)
        {
            const auto value = std::bit_cast<code_unit_type>(code_unit);

            if constexpr (requires(container_type& c, code_unit_type v) { c.push_back(v); })
            {
                m_container.push_back(value);
            }
            else
            {
                m_container.insert(std::as_const(m_container).end(), value);
            }
        }

        /// @brief Appends a range of code units to the end of the string.
        ///
        /// @pre The `range` must not depend on the state of this string. For example, it cannot be a view into this string's underlying container.
        ///
        template<std::ranges::input_range Range>
            requires code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, static_cast<encoding>(Encoding)>
        constexpr void append_code_units_range(Range&& range)
        {
            using range_code_unit_t = std::remove_cvref_t<std::ranges::range_reference_t<Range>>;

            auto code_units = std::views::transform(
                std::forward<Range>(range), [](const range_code_unit_t code_unit) static { return std::bit_cast<code_unit_type>(code_unit); });

            using code_units_range_t = decltype(code_units);

            using const_iter = container_type::const_iterator;

            if constexpr (requires(container_type& c, code_units_range_t& r) { c.append_range(r); })
            {
                m_container.append_range(code_units);
            }
            else if constexpr (requires(container_type& c, const_iter cit, code_units_range_t& r) { c.insert_range(cit, r); })
            {
                m_container.insert_range(std::as_const(m_container).end(), code_units);
            }
            else
            {
                auto common_range = std::views::common(std::move(code_units));

                m_container.insert(std::as_const(m_container).end(), std::ranges::begin(common_range), std::ranges::end(common_range));
            }
        }

        /// @brief Encodes the `code_point` and appends it to the end of the string.
        ///
        constexpr void push_back(const uchar code_point)
        {
            if constexpr (Encoding == unicode_encoding::utf8)
            {
                append_code_units_range(code_point.encode_utf8());
            }
            else if constexpr (Encoding == unicode_encoding::utf16)
            {
                append_code_units_range(code_point.encode_utf16());
            }
            else if constexpr (Encoding == unicode_encoding::utf32)
            {
                push_back_code_unit(code_point.value());
            }
        }

    private:
        Container m_container;

        friend impl::basic_ustring_impl;
    };
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_STRING_HPP