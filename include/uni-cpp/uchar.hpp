#ifndef UNI_CPP_UCHAR_HPP
#define UNI_CPP_UCHAR_HPP

/// @file
///
/// @brief Character type definitions and utilities for the uni-cpp library.
///
/// This file defines core types for representing and manipulating Unicode and ASCII characters,
/// including `ascii_char` for 7-bit ASCII values and `uchar` for Unicode scalar values.
///
/// It provides:
/// - Validation of ASCII and Unicode characters
/// - Safe conversion between ASCII and Unicode characters
/// - UTF-8 and UTF-16 encoding of characters
/// - Case conversion for characters
/// - Full canonical and compatibility decompositions of code points
/// - Checking character properties
///

#include <cstddef>
#include <cstdint>
#include <optional>
#include <expected>

#include "impl/unicode_data/case_mapping.hpp"
#include "impl/unicode_data/decomposition.hpp"
#include "impl/unicode_data/data/canonical_combining_class.hpp"
#include "impl/encoding/ascii.hpp"
#include "impl/encoding/utf32.hpp"
#include "impl/inplace_vector.hpp"

#include <concepts>
#include <iterator>
#include <array>
#include <compare>
#include <bit>
#include <utility>
#include <limits>
#include <stdexcept>

namespace upp
{
    /// @brief Check whether `value` is within the ASCII range (`0` to `0x7F`, inclusive).
    ///
    [[nodiscard]] constexpr bool is_valid_ascii(std::uint8_t value) noexcept
    {
        return value < 0x80;
    }

    /// @brief Check whether `value` is a valid [Unicode scalar value](https://www.unicode.org/glossary/#unicode_scalar_value).
    ///
    /// The set of valid Unicode scalar values consists of the
    /// ranges `0` to `0xD7FF` and `0xE000` to `0x10FFFF`, inclusive.
    ///
    [[nodiscard]] constexpr bool is_valid_usv(std::uint32_t value) noexcept
    {
        // read: https://github.com/rust-lang/rust/blob/1.87.0/library/core/src/char/convert.rs#L225
        return (value ^ 0xD800U) - 0x800U < 0x10F800U;
    }

    class ascii_char;
    class uchar;

    /// @brief Concept for identifying character types defined by the uni-cpp library (`uchar` and `ascii_char`).
    ///
    /// @see uchar, ascii_char
    ///
    /// @headerfile "" <uni-cpp/uchar.hpp>
    ///
    template<typename T>
    concept char_type = std::same_as<T, ascii_char> || std::same_as<T, uchar>;

    namespace impl
    {
        /// @brief Immutable, in-place buffer with fixed capacity and dynamic size.
        ///
        /// Small, contiguous iterable buffer used by functions that return a short range of elements with a dynamic size.
        ///
        /// This serves as the base class for:
        /// - `encode_utf8_t`, `encode_utf16_t`,
        /// - `to_lowercase_t`, `to_uppercase_t`, `to_titlecase_t`, `to_casefold_t`,
        /// - `full_decomposition_t` and `full_compatibility_decomposition_t`.
        ///
        /// @tparam T Type of the elements stored in the buffer.
        /// @tparam MaxSize The capacity of the buffer.
        ///
        template<typename T, std::size_t MaxSize>
        class immutable_inplace_buffer
        {
        public:
            using const_iterator         = inplace_vector<T, MaxSize>::const_iterator;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        public:
            constexpr immutable_inplace_buffer(const immutable_inplace_buffer&) noexcept = default;
            constexpr immutable_inplace_buffer(immutable_inplace_buffer&&) noexcept      = default;

            constexpr ~immutable_inplace_buffer() noexcept = default;

            constexpr immutable_inplace_buffer& operator=(const immutable_inplace_buffer&) noexcept = default;
            constexpr immutable_inplace_buffer& operator=(immutable_inplace_buffer&&) noexcept      = default;

            [[nodiscard]] constexpr const_iterator         begin() const noexcept { return m_data.begin(); }
            [[nodiscard]] constexpr const_iterator         cbegin() const noexcept { return m_data.cbegin(); }
            [[nodiscard]] constexpr const_iterator         end() const noexcept { return m_data.end(); }
            [[nodiscard]] constexpr const_iterator         cend() const noexcept { return m_data.cend(); }
            [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return m_data.rbegin(); }
            [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return m_data.crbegin(); }
            [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return m_data.rend(); }
            [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return m_data.crend(); }

            [[nodiscard]] constexpr std::size_t size() const noexcept { return m_data.size(); }

            [[nodiscard]] constexpr const T* data() const noexcept { return m_data.data(); }

            [[nodiscard]] constexpr bool operator==(const immutable_inplace_buffer& other) const noexcept = default;

        protected:
            /// @brief Constructs the buffer with the given data.
            ///
            constexpr immutable_inplace_buffer(inplace_vector<T, MaxSize>&& p_data) noexcept
                : m_data{std::move(p_data)}
            {
            }

        private:
            inplace_vector<T, MaxSize> m_data;
        };

        template<typename T, std::size_t MaxSize>
        class encode_utf : public immutable_inplace_buffer<T, MaxSize>
        {
        private:
            using base = immutable_inplace_buffer<T, MaxSize>;
            friend uchar;

        public:
            using base::base;
        };

        enum class to_case_enum : std::uint8_t
        {
            lower,
            upper,
            title,
            fold
        };

        /// @tparam Case Used to make `to_lowercase_t`, `to_uppercase_t`, `to_titlecase_t` and `to_casefold_t` distinct types.
        /// @tparam T Always `uchar`; only a template parameter due to forward declaration constraints.
        ///
        template<to_case_enum Case, typename T = uchar>
        class to_case : public immutable_inplace_buffer<T, 3>
        {
        private:
            using base = immutable_inplace_buffer<T, 3>;
            friend uchar;

        public:
            using base::base;
        };

        /// @tparam Kind Used to make `full_decomposition_t` and `full_compatibility_decomposition_t` distinct types.
        /// @tparam T Always `uchar`; only a template parameter due to forward declaration constraints.
        ///
        template<unicode_data::decomposition::decomposition_kind Kind, typename T = uchar>
        class decomposition_t : public immutable_inplace_buffer<T, 18>
        {
        private:
            using base = immutable_inplace_buffer<T, 18>;
            friend uchar;

        public:
            using base::base;
        };
    } // namespace impl

    /// @brief An ASCII character type representing a single ASCII character code.
    ///
    /// @headerfile "" <uni-cpp/uchar.hpp>
    ///
    class ascii_char
    {
    public:
        /// @brief Default constructor. Initializes the value to the Null character (`0x00`).
        ///
        /// @see from, from_lossy, from_unchecked
        ///
        constexpr ascii_char() noexcept
            : m_value(0)
        {
        }

        /// @brief Copy constructor.
        ///
        constexpr ascii_char(const ascii_char&) noexcept = default;

        /// @brief Copy assignment operator.
        ///
        constexpr ascii_char& operator=(const ascii_char&) noexcept = default;

        /// @brief Constructs `ascii_char` from the given `value` if it's a valid ASCII character, otherwise returns `ascii_error`.
        ///
        /// Attempts to convert `value` to an `ascii_char`. This conversion succeeds only if
        /// `value` is within the ASCII range `[0, 127]`. If `value` is outside this range, `ascii_error` is returned.
        ///
        /// Use `from_lossy` to substitute invalid values with a fallback character, or `from_unchecked`
        /// if you are certain `value` is valid and want to avoid a check.
        ///
        /// @see from_lossy, from_unchecked
        ///
        [[nodiscard]] static constexpr std::expected<ascii_char, ascii_error> from(std::uint8_t value) noexcept
        {
            if (!is_valid_ascii(value))
                return std::expected<ascii_char, ascii_error>{std::unexpect, ascii_error{}};

            return std::expected<ascii_char, ascii_error>{std::in_place, ascii_char{value}};
        }

        /// @brief Constructs `ascii_char` from the given `value` if it's a valid ASCII character, otherwise returns the ASCII substitute character.
        ///
        /// Converts `value` to an `ascii_char`, returning the original character if it's within
        /// the ASCII range. If `value` is invalid, the ASCII substitute character (`ascii_char::substitute_character()`) is returned instead.
        ///
        /// This is a safe conversion that ensures a valid `ascii_char` is always returned.
        ///
        /// @see from, from_unchecked
        ///
        [[nodiscard]] static constexpr ascii_char from_lossy(std::uint8_t value) noexcept
        {
            if (is_valid_ascii(value))
                return ascii_char{value};

            return substitute_character();
        }

        /// @brief Constructs `ascii_char` from the given `value` without validation.
        ///
        /// This function constructs an `ascii_char` assuming the provided `value`
        /// is within the valid ASCII range.
        ///
        /// @pre `value` MUST be in the ASCII range - `[0, 127]`.
        ///
        /// @warning If the precondition of this function isn't met, the behavior is undefined.
        /// Use `from` or `from_lossy` as a safe alternative that performs validation.
        ///
        /// @see from, from_lossy
        ///
        [[nodiscard]] static constexpr ascii_char from_unchecked(std::uint8_t value) noexcept
        {
            // ASSERT(is_valid_ascii(value));
            return ascii_char{value};
        }

        /// @brief Returns the ASCII [substitute character](https://en.wikipedia.org/wiki/Substitute_character) (`0x1A`).
        ///
        /// Commonly used to represent invalid or unrecognized characters, such as those resulting from decoding errors.
        ///
        [[nodiscard]] static constexpr ascii_char substitute_character() noexcept { return ascii_char{std::uint8_t{0x1A}}; }

        /// @brief Compares two `ascii_char` values for equality.
        ///
        /// Equivalent to `lhs.value() == rhs.value()`.
        ///
        [[nodiscard]] constexpr bool operator==(ascii_char other) const noexcept { return m_value == other.m_value; }

        /// @brief Performs a three-way comparison between two `ascii_char` values.
        ///
        /// Equivalent to `lhs.value() <=> rhs.value()`.
        ///
        [[nodiscard]] constexpr std::strong_ordering operator<=>(ascii_char other) const noexcept { return m_value <=> other.m_value; }

        /// @brief Retrieves the underlying ASCII character code.
        ///
        [[nodiscard]] constexpr std::uint8_t value() const noexcept { return m_value; }

    private:
        explicit constexpr ascii_char(std::uint8_t value) noexcept
            : m_value(value)
        {
        }

    private:
        std::uint8_t m_value;
    };

    namespace impl
    {
        inline constexpr std::uint32_t max_usv = 0x10FFFFU;
    }

    /// @brief The decomposition type of a code point.
    ///
    /// Code points with a compatibility decomposition mapping have an associated decomposition type.
    /// The decomposition type generally indicates the formatting information removed by the compatibility decomposition.
    /// Code points with a canonical decomposition mapping have no decomposition type.
    ///
    enum class decomposition_type : std::uint8_t
    {
        // Note: zero is used in the data tables to indicate a `None` value
        font = 1, ///< Font variant (for example, a blackletter form)
        no_break, ///< No-break version of a space or hyphen
        initial,  ///< Initial presentation form (Arabic)
        medial,   ///< Medial presentation form (Arabic)
        final,    ///< Final presentation form (Arabic)
        isolated, ///< Isolated presentation form (Arabic)
        circle,   ///< Encircled form
        super,    ///< Superscript form
        sub,      ///< Subscript form
        vertical, ///< Vertical layout presentation form
        wide,     ///< Wide (or zenkaku) compatibility character
        narrow,   ///< Narrow (or hankaku) compatibility character
        small,    ///< Small variant form (CNS compatibility)
        square,   ///< CJK squared font variant
        fraction, ///< Vulgar fraction form
        compat    ///< Otherwise unspecified compatibility character
    };

    /// @brief A Unicode character type representing a single [Unicode scalar value](https://www.unicode.org/glossary/#unicode_scalar_value).
    ///
    /// @headerfile "" <uni-cpp/uchar.hpp>
    ///
    class uchar
    {
    public:
        /// A sized range of UTF-8 code units returned by the `encode_utf8` method. See its documentation for more.
        using encode_utf8_t = impl::encode_utf<char8_t, 4>;
        /// A sized range of UTF-16 code units returned by the `encode_utf16` method. See its documentation for more.
        using encode_utf16_t = impl::encode_utf<char16_t, 2>;

        /// A sized range of `uchar`s returned by the `to_lowercase` method. See its documentation for more.
        using to_lowercase_t = impl::to_case<impl::to_case_enum::lower>;
        /// A sized range of `uchar`s returned by the `to_uppercase` method. See its documentation for more.
        using to_uppercase_t = impl::to_case<impl::to_case_enum::upper>;
        /// A sized range of `uchar`s returned by the `to_titlecase` method. See its documentation for more.
        using to_titlecase_t = impl::to_case<impl::to_case_enum::title>;
        /// A sized range of `uchar`s returned by the `to_casefold` method. See its documentation for more.
        using to_casefold_t = impl::to_case<impl::to_case_enum::fold>;

        /// A sized range of `uchar`s returned by the `full_decomposition` method. See its documentation for more.
        using full_decomposition_t = impl::decomposition_t<impl::unicode_data::decomposition::decomposition_kind::canonical>;
        /// A sized range of `uchar`s returned by the `full_compatibility_decomposition` method. See its documentation for more.
        using full_compatibility_decomposition_t = impl::decomposition_t<impl::unicode_data::decomposition::decomposition_kind::compatibility>;

    public:
        /// @brief Default constructor. Initializes the value to the Null character (`U+0000`).
        ///
        /// @see from, from_lossy, from_unchecked
        ///
        constexpr uchar() noexcept
            : m_value(0)
        {
        }
        /// @brief Converts `ascii_char` to `uchar`, preserving its value.
        ///
        /// Converts `ascii_char` to `uchar` as if by `uchar::from_unchecked(static_cast<std::uint32_t>(ch.value()))`.
        /// This conversion never fails, because all valid ASCII codes are valid Unicode scalar values.
        ///
        /// @see from, from_lossy, from_unchecked
        ///
        explicit constexpr uchar(ascii_char ch) noexcept
            : m_value(static_cast<std::uint32_t>(ch.value()))
        {
        }

        /// @brief Copy constructor.
        ///
        constexpr uchar(const uchar&) noexcept = default;

        /// @brief Copy assignment operator.
        ///
        constexpr uchar& operator=(const uchar&) noexcept = default;

        /// @brief Constructs `uchar` from the given `value` if it's a valid Unicode scalar value, otherwise returns `utf32_error`.
        ///
        /// Attempts to convert `value` to a `uchar`. This conversion succeeds only if
        /// `value` is a valid Unicode scalar value. If it's not, `utf32_error` is returned.
        ///
        /// Use `from_lossy` to substitute invalid values with a replacement character, or `from_unchecked`
        /// if you are certain `value` is valid and want to avoid a check.
        ///
        /// @see from_lossy, from_unchecked
        ///
        [[nodiscard]] static constexpr std::expected<uchar, utf32_error> from(std::uint32_t value) noexcept
        {
            if (!is_valid_usv(value))
            {
                if (value > impl::max_usv)
                {
                    return std::expected<uchar, utf32_error>{std::unexpect, utf32_error{.code = utf32_error_code::out_of_range}};
                }
                else
                {
                    return std::expected<uchar, utf32_error>{std::unexpect, utf32_error{.code = utf32_error_code::encoded_surrogate}};
                }
            }

            return std::expected<uchar, utf32_error>{std::in_place, uchar{value}};
        }

        /// @brief Constructs `uchar` from the given `value` if it's a valid Unicode scalar value, otherwise returns the Unicode replacement character.
        ///
        /// Converts `value` to a `uchar`, returning the original character if it's a valid Unicode scalar value.
        /// If `value` is invalid, the Unicode replacement character (`uchar::replacement_character()`) is returned instead.
        ///
        /// This is a safe conversion that ensures a valid `uchar` is always returned.
        ///
        /// @see from, from_unchecked
        ///
        [[nodiscard]] static constexpr uchar from_lossy(std::uint32_t value) noexcept
        {
            if (is_valid_usv(value))
                return uchar{value};

            return replacement_character();
        }

        /// @brief Constructs `uchar` from the given `value` without validation.
        ///
        /// This function constructs a `uchar` assuming the provided `value`
        /// is a valid Unicode scalar value.
        ///
        /// @pre `value` MUST be a valid [Unicode scalar value](https://www.unicode.org/glossary/#unicode_scalar_value).
        ///
        /// @warning If the precondition of this function isn't met, the behavior is undefined.
        /// Use `from` or `from_lossy` as a safe alternative that performs validation.
        ///
        /// @see from, from_lossy
        ///
        [[nodiscard]] static constexpr uchar from_unchecked(std::uint32_t value) noexcept
        {
            // ASSERT(is_valid_usv(value));
            return uchar{value};
        }

        /// @brief Returns the [Unicode replacement character](https://www.unicode.org/glossary/#replacement_character) (`U+FFFD`).
        ///
        /// Commonly used to represent invalid or unrecognized characters, such as those resulting from decoding errors.
        ///
        [[nodiscard]] static constexpr uchar replacement_character() noexcept { return uchar{std::uint32_t{0xFFFD}}; }

        /// @brief Compares two `uchar` values for equality.
        ///
        /// Equivalent to `lhs.value() == rhs.value()`.
        ///
        [[nodiscard]] constexpr bool operator==(uchar other) const noexcept { return m_value == other.m_value; }

        /// @brief Performs a three-way comparison between two `uchar` values.
        ///
        /// Equivalent to `lhs.value() <=> rhs.value()`.
        ///
        [[nodiscard]] constexpr std::strong_ordering operator<=>(uchar other) const noexcept { return m_value <=> other.m_value; }

        /// @brief Retrieves the underlying Unicode scalar value.
        ///
        [[nodiscard]] constexpr std::uint32_t value() const noexcept { return m_value; }

        /// @brief Checks whether the character is within the ASCII range (`U+0000` to `U+007F`, inclusive).
        ///
        /// @see as_ascii, as_ascii_lossy
        ///
        [[nodiscard]] constexpr bool is_ascii() const noexcept { return m_value < 0x80; }

        /// @brief Attempts to convert the character to an `ascii_char`, if possible (`is_ascii() == true`).
        ///
        /// @see is_ascii, as_ascii_lossy
        ///
        [[nodiscard]] constexpr std::optional<ascii_char> as_ascii() const noexcept
        {
            if (is_ascii())
                return std::optional(ascii_char::from_unchecked(static_cast<std::uint8_t>(m_value)));

            return std::optional<ascii_char>();
        }

        /// @brief Constructs `ascii_char` from the character if it's a valid ASCII character (`is_ascii() == true`),
        ///        otherwise returns the ASCII substitute character (`ascii_char::substitute_character()`).
        ///
        /// This is a safe conversion that ensures a valid `ascii_char` is always returned.
        ///
        /// @see is_ascii, as_ascii
        ///
        [[nodiscard]] constexpr ascii_char as_ascii_lossy() const noexcept
        {
            if (is_ascii())
                return ascii_char::from_unchecked(static_cast<std::uint8_t>(m_value));

            return ascii_char::substitute_character();
        }

        /// @brief Returns the number of UTF-8 code units (bytes) required to encode this `uchar` in UTF-8.
        ///
        /// @return Number between 1 and 4, inclusive.
        ///
        /// @see length_utf16, encode_utf8
        ///
        [[nodiscard]] constexpr std::size_t length_utf8() const noexcept
        {
            // read: https://cceckman.com/writing/branchless-utf8-encoding/
            // license: https://codeberg.org/cceckman/unicode-branchless/src/branch/main/LICENSE

            static constexpr std::array<std::uint8_t, 33> length_lookup_table{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 3,
                                                                              3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1};

            return static_cast<std::size_t>(length_lookup_table[std::countl_zero(m_value | 1U)]);
        }

        /// @brief Returns the number of UTF-16 code units required to encode this `uchar` in UTF-16.
        ///
        /// @return Number that is always either 1 or 2.
        ///
        /// @see length_utf8, encode_utf16
        ///
        [[nodiscard]] constexpr std::size_t length_utf16() const noexcept { return (m_value < 0x10000) ? 1uz : 2uz; }

        /// @brief Returns a sequence of UTF-8 code units (bytes) representing this character encoded in UTF-8.
        ///
        /// @return A sized range of `char8_t`s that are UTF-8 code units.
        ///
        /// @see encode_utf16, length_utf8
        ///
        [[nodiscard]] constexpr encode_utf8_t encode_utf8() const noexcept
        {
            using buffer_t = impl::inplace_vector<char8_t, 4>;

            // clang-format off

            switch (length_utf8())
            {
            case 1uz: {
                return encode_utf8_t{buffer_t{static_cast<char8_t>(m_value)}};
            }
            case 2uz: {
                return encode_utf8_t{buffer_t{
                    static_cast<char8_t>((m_value >> 6U) | 0xC0U),
                    static_cast<char8_t>((m_value & 0x3FU) | 0x80U)
                }};
            }
            case 3uz: {
                return encode_utf8_t{buffer_t{
                    static_cast<char8_t>((m_value >> 12U) | 0xE0U),
                    static_cast<char8_t>(((m_value >> 6U) & 0x3FU) | 0x80U),
                    static_cast<char8_t>((m_value & 0x3FU) | 0x80U)
                }};
            }
            case 4uz: {
                return encode_utf8_t{buffer_t{
                    static_cast<char8_t>((m_value >> 18U) | 0xF0U),
                    static_cast<char8_t>(((m_value >> 12U) & 0x3FU) | 0x80U),
                    static_cast<char8_t>(((m_value >> 6U) & 0x3FU) | 0x80U),
                    static_cast<char8_t>((m_value & 0x3FU) | 0x80U)
                }};
            }
            default: std::unreachable();
            }

            // clang-format on
        }

        /// @brief Returns a sequence of UTF-16 code units representing this character encoded in UTF-16.
        ///
        /// @return A sized range of `char16_t`s that are UTF-16 code units.
        ///
        /// @see encode_utf8, length_utf16
        ///
        [[nodiscard]] constexpr encode_utf16_t encode_utf16() const noexcept
        {
            using buffer_t = impl::inplace_vector<char16_t, 2>;

            switch (length_utf16())
            {
            case 1uz: {
                return encode_utf16_t{buffer_t{static_cast<char16_t>(m_value)}};
            }
            case 2uz: {
                const std::uint32_t code = m_value - 0x10'000;

                return encode_utf16_t{buffer_t{
                    // clang-format off
                    static_cast<char16_t>(0xD800U | (code >> 10U)),
                    static_cast<char16_t>(0xDC00U | (code & 0x3FFU))
                    // clang-format on
                }};
            }
            default: std::unreachable();
            }
        }

        /// @brief Returns the lowercase mapping of this `uchar`.
        ///
        /// Most lowercase mappings consist of a single `uchar`, but some consist of multiple.
        /// For example, U+0130 LATIN CAPITAL LETTER I WITH DOT ABOVE has a lowercase
        /// mapping to the sequence <U+0069 LATIN SMALL LETTER I, U+0307 COMBINING DOT ABOVE>.
        /// If this `uchar` does not have a lowercase mapping, the result is the `uchar` itself.
        ///
        /// This conversion is performed without tailoring; it is independent of context and language.
        ///
        /// See [Unicode Standard Chapter 4.2 (Case)](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124722)
        /// and [Unicode Standard Chapter 3.13 (Default Case Algorithms)](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G33992).
        ///
        /// @note If you want to perform case-insensitive matching of characters, use `to_casefold` instead. It is specifically designed
        ///       for that purpose and it slightly differs in its mappings. See `to_casefold` documentation.
        ///
        /// @return A range of `uchar`s.
        /// @see to_uppercase, to_casefold, to_titlecase
        ///
        [[nodiscard]] constexpr to_lowercase_t to_lowercase() const noexcept
        {
            return to_case_impl<to_lowercase_t, impl::unicode_data::case_mapping::case_mapping_type::lowercase>();
        }

        /// @brief Returns the uppercase mapping of this `uchar`.
        ///
        /// Most uppercase mappings consist of a single `uchar`, but some consist of multiple.
        /// For example, U+00DF LATIN SMALL LETTER SHARP S has an uppercase
        /// mapping to the sequence <U+0053 LATIN CAPITAL LETTER S, U+0053 LATIN CAPITAL LETTER S>.
        /// If this `uchar` does not have an uppercase mapping, the result is the `uchar` itself.
        ///
        /// This conversion is performed without tailoring; it is independent of context and language.
        ///
        /// See [Unicode Standard Chapter 4.2 (Case)](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124722)
        /// and [Unicode Standard Chapter 3.13 (Default Case Algorithms)](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G33992).
        ///
        /// @note If you want to perform case-insensitive matching of characters, use `to_casefold` instead. It is specifically designed
        ///       for that purpose. See `to_casefold` documentation.
        ///
        /// @return A range of `uchar`s.
        /// @see to_lowercase, to_titlecase, to_casefold
        ///
        [[nodiscard]] constexpr to_uppercase_t to_uppercase() const noexcept
        {
            return to_case_impl<to_uppercase_t, impl::unicode_data::case_mapping::case_mapping_type::uppercase>();
        }

        /// @brief Returns the titlecase mapping of this `uchar`.
        ///
        /// Most titlecase mappings consist of a single `uchar`, but some consist of multiple.
        /// For example, U+00DF LATIN SMALL LETTER SHARP S has a titlecase
        /// mapping to the sequence <U+0053 LATIN CAPITAL LETTER S, U+0073 LATIN SMALL LETTER S>.
        /// If this `uchar` does not have a titlecase mapping, the result is the `uchar` itself.
        ///
        /// This conversion is performed without tailoring; it is independent of context and language.
        ///
        /// See [Unicode Standard Chapter 4.2 (Case)](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124722)
        /// and [Unicode Standard Chapter 3.13 (Default Case Algorithms)](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G33992).
        ///
        /// @note Titlecase in Unicode is **not** the same as uppercase.
        ///       For example, `ß` has an uppercase mapping to `SS`, but a titlecase mapping to `Ss`.
        ///       See [Unicode Standard Chapter 4.2 (Case)](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124722).
        ///
        /// @return A range of `uchar`s.
        /// @see to_uppercase, to_lowercase, to_casefold
        ///
        [[nodiscard]] constexpr to_titlecase_t to_titlecase() const noexcept
        {
            return to_case_impl<to_titlecase_t, impl::unicode_data::case_mapping::case_mapping_type::titlecase>();
        }

        /// @brief Returns the casefold mapping of this `uchar`.
        ///
        /// Most casefold mappings consist of a single `uchar`, but some consist of multiple.
        /// For example, U+00DF LATIN SMALL LETTER SHARP S has a casefold
        /// mapping to the sequence <U+0073 LATIN SMALL LETTER S, U+0073 LATIN SMALL LETTER S>.
        /// If this `uchar` does not have a casefold mapping, the result is the `uchar` itself.
        ///
        /// This conversion is performed without tailoring; it is independent of context and language.
        /// See [Unicode 3.13.3 Default Case Folding](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G53253).
        ///
        /// @note Case folding in Unicode is **not** the same as applying lowercase. Case folding is a mapping
        ///       intended for case-insensitive matching of characters and sequences. For example, the `ß` character
        ///       has a lowercase mapping to `ß` (itself), but its case folding is `ss`. That's because `ß` has an uppercase
        ///       mapping to `SS`, which when case folded becomes `ss`, matching the case folding of `ß`.
        ///       See [Unicode 3.13.3 Default Case Folding](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G53253)
        ///       and [Unicode 5.18.4 Caseless Matching](https://www.unicode.org/versions/latest/core-spec/chapter-5/#G21790).
        ///
        /// @return A range of `uchar`s.
        /// @see to_lowercase, to_uppercase, to_titlecase
        ///
        [[nodiscard]] constexpr to_casefold_t to_casefold() const noexcept
        {
            return to_case_impl<to_casefold_t, impl::unicode_data::case_mapping::case_mapping_type::casefold>();
        }

        /// @brief The [full canonical decomposition](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G7425) of this `uchar`.
        ///
        /// @return A `std::ranges::contiguous_range` of `uchar`s representing the full canonical decomposition of this `uchar`.
        ///
        /// If this `uchar` does not have a defined canonical decomposition, it maps to itself.
        ///
        /// @par Example
        ///
        /// U+00E0 LATIN SMALL LETTER A WITH GRAVE has a canonical decomposition to the sequence
        /// <U+0061 LATIN SMALL LETTER A, U+0300 COMBINING GRAVE ACCENT>.
        ///
        /// @see full_compatibility_decomposition, decomposition_type
        ///
        [[nodiscard]] constexpr full_decomposition_t full_decomposition() const noexcept
        {
            static constexpr auto kind = impl::unicode_data::decomposition::decomposition_kind::canonical;

            return full_decomposition_t{impl::unicode_data::decomposition::lookup_decomposition<kind>(m_value)};
        }

        /// @brief The [full compatibility decomposition](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G749) of this `uchar`.
        ///
        /// @return A `std::ranges::contiguous_range` of `uchar`s representing the full compatibility decomposition of this `uchar`.
        ///
        /// If this `uchar` does not have a defined compatibility decomposition, it maps to itself.
        ///
        /// @par Example
        ///
        /// U+00B5 MICRO SIGN has a compatibility decomposition to U+03BC GREEK SMALL LETTER MU.
        ///
        /// U+03D3 GREEK UPSILON WITH ACUTE AND HOOK SYMBOL canonically decomposes to the sequence
        /// <U+03D2 GREEK UPSILON WITH HOOK SYMBOL, U+0301 COMBINING ACUTE ACCENT>. That sequence has a compatibility decomposition of
        /// <U+03A5 GREEK CAPITAL LETTER UPSILON, U+0301 COMBINING ACUTE ACCENT>. Thus, the full compatibility decomposition of
        /// U+03D3 GREEK UPSILON WITH ACUTE AND HOOK SYMBOL is the sequence <U+03A5 GREEK CAPITAL LETTER UPSILON, U+0301 COMBINING ACUTE ACCENT>.
        ///
        /// @see full_decomposition, decomposition_type
        ///
        [[nodiscard]] constexpr full_compatibility_decomposition_t full_compatibility_decomposition() const noexcept
        {
            static constexpr auto kind = impl::unicode_data::decomposition::decomposition_kind::compatibility;

            return full_compatibility_decomposition_t{impl::unicode_data::decomposition::lookup_decomposition<kind>(m_value)};
        }

        /// @brief Returns the decomposition type of this code point, if one exists.
        ///
        /// Code points with a compatibility decomposition mapping have an associated decomposition type.
        /// The decomposition type generally indicates the formatting information removed by the compatibility decomposition.
        /// Code points with a canonical decomposition mapping have no decomposition type.
        ///
        /// @return For code points with a compatibility decomposition mapping, the associated decomposition type;
        ///         for code points with a canonical decomposition mapping, or no defined decomposition mapping, `std::nullopt`.
        ///
        /// @see upp::decomposition_type, full_decomposition, full_compatibility_decomposition
        ///
        [[nodiscard]] constexpr std::optional<upp::decomposition_type> decomposition_type() const noexcept
        {
            const std::uint8_t type = impl::unicode_data::decomposition::lookup_decomposition_type(m_value);

            if (type == 0)
                return {};

            return {static_cast<upp::decomposition_type>(type)};
        }

        /// @brief Returns the [canonical combining class](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G32493) of this code point.
        ///
        [[nodiscard]] constexpr std::uint8_t canonical_combining_class() const noexcept
        {
            return impl::unicode_data::canonical_combining_class::impl::lookup(m_value);
        }

    private:
        explicit constexpr uchar(std::uint32_t value) noexcept
            : m_value(value)
        {
        }

        template<typename ResultType, impl::unicode_data::case_mapping::case_mapping_type MappingType>
        [[nodiscard]] constexpr ResultType to_case_impl() const noexcept
        {
            return ResultType{impl::unicode_data::case_mapping::lookup_case_mapping<MappingType>(m_value)};
        }

    private:
        std::uint32_t m_value;
    };

    inline namespace literals
    {
        /// @brief Inline namespace containing user-defined literals for uni-cpp character types.
        ///
        /// Contains the following user-defined literals:
        ///
        /// - `_ac` for creating `upp::ascii_char` from:
        ///     - an integer literal (example: `0x41_ac`),
        ///     - a UTF-8 character literal (example: ``u8'A'_ac``),
        ///
        /// - `_uc` for creating `upp::uchar` from:
        ///     - an integer literal (example: `0xFFFD_uc`),
        ///     - a UTF-32 character literal (example: ``U'a'_uc``).
        ///
        inline namespace char_literals
        {
            /// @brief User-defined literal for creating an `ascii_char` from an integer literal.
            /// @param value The ASCII character code.
            ///
            /// @throws std::invalid_argument If the `value` is **not** a valid ASCII character code.
            ///
            /// @note This function is evaluated at compile time.
            ///
            [[nodiscard]] consteval ascii_char operator""_ac(const unsigned long long int value)
            {
                if (value > static_cast<unsigned long long int>(std::numeric_limits<std::uint8_t>::max()) ||
                    !is_valid_ascii(static_cast<std::uint8_t>(value)))
                {
                    throw std::invalid_argument("Invalid ASCII value");
                }

                return ascii_char::from_unchecked(static_cast<std::uint8_t>(value));
            }

            /// @brief User-defined literal for creating an `ascii_char` from a UTF-8 character literal.
            /// @param value The ASCII character.
            ///
            /// @throws std::invalid_argument If the `value` is **not** a valid ASCII character.
            ///
            /// @note This function is evaluated at compile time.
            ///
            [[nodiscard]] consteval ascii_char operator""_ac(const char8_t value)
            {
                if (!is_valid_ascii(static_cast<std::uint8_t>(value)))
                {
                    throw std::invalid_argument("Invalid ASCII value");
                }

                return ascii_char::from_unchecked(static_cast<std::uint8_t>(value));
            }

            /// @brief User-defined literal for creating a `uchar` from an integer literal.
            /// @param value The Unicode scalar value.
            ///
            /// @throws std::invalid_argument If the `value` is **not** a valid Unicode scalar value.
            ///
            /// @note This function is evaluated at compile time.
            ///
            [[nodiscard]] consteval uchar operator""_uc(const unsigned long long int value)
            {
                if (value > static_cast<unsigned long long int>(std::numeric_limits<std::uint32_t>::max()) ||
                    !is_valid_usv(static_cast<std::uint32_t>(value)))
                {
                    throw std::invalid_argument("Invalid Unicode scalar value");
                }

                return uchar::from_unchecked(static_cast<std::uint32_t>(value));
            }

            /// @brief User-defined literal for creating a `uchar` from a UTF-32 character literal.
            /// @param value The Unicode scalar value.
            ///
            /// @throws std::invalid_argument If the `value` is **not** a valid Unicode scalar value.
            ///
            /// @note This function is evaluated at compile time.
            ///
            [[nodiscard]] consteval uchar operator""_uc(const char32_t value)
            {
                if (!is_valid_usv(static_cast<std::uint32_t>(value)))
                {
                    throw std::invalid_argument("Invalid Unicode scalar value");
                }

                return uchar::from_unchecked(static_cast<std::uint32_t>(value));
            }
        } // namespace char_literals
    } // namespace literals
} // namespace upp

#endif // UNI_CPP_UCHAR_HPP