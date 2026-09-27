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
#include "impl/unicode_data/composition.hpp"
#include "impl/unicode_data/data/canonical_combining_class.hpp"
#include "impl/unicode_data/data/general_category.hpp"
#include "impl/unicode_data/data/core_properties.hpp"
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
        /// - `encode_as_utf8_t`, `encode_as_utf16_t`,
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
        class encode_as_utf : public immutable_inplace_buffer<T, MaxSize>
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

        /// @brief Converts an ASCII base-36 digit to an integer value.
        ///
        /// If @p ascii_ch is a decimal ASCII digit, returns `ascii_ch - '0'`.
        /// Otherwise, if @p ascii_ch is a lowercase ASCII letter, returns `ascii_ch - 'a' + 10`.
        /// Otherwise, if @p ascii_ch is an uppercase ASCII letter, returns `ascii_ch - 'A' + 10`.
        /// Otherwise, returns `0xFF`.
        ///
        /// @pre `is_valid_ascii(ascii_ch)`
        ///
        [[nodiscard]] constexpr std::uint8_t ascii_base36_to_value(std::uint8_t ascii_ch) noexcept
        {
            static constexpr std::array<std::uint8_t, 128> values{
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // [0x00, 0x0F]
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // [0x10, 0x1F]
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // [0x20, 0x2F]
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // [0x30, 0x3F]
                0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, // [0x40, 0x4F]
                0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // [0x50, 0x5F]
                0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, // [0x60, 0x6F]
                0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // [0x70, 0x7F]
            };

            return values[ascii_ch];
        }
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

    /// @brief The general category of a code point.
    ///
    /// Each Unicode code point is assigned a normative `General_Category` value.
    /// The `General_Category` value for a character serves as a basic classification of that character, based on its primary usage.
    /// Many characters have multiple uses, and not all such uses can be captured by a single, simple partition property such as `General_Category`.
    /// Thus, many letters often serve dual functions as numerals in traditional numeral systems. Examples can be found in the Roman numeral system,
    /// in Greek usage of letters as numbers, in Hebrew, and similarly for many scripts. In such cases the `General_Category` is assigned based on
    /// the primary letter usage of the character, even though it may also have numeric values, occur in numeric expressions,
    /// or be used symbolically in mathematical expressions, and so on.
    ///
    /// See [The Unicode Standard, Chapter 4.5 (General Category)](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124142).
    ///
    enum class general_category : std::uint8_t
    {
        uppercase_letter,      ///< An uppercase letter
        lowercase_letter,      ///< A lowercase letter
        titlecase_letter,      ///< A digraph encoded as a single character, with first part uppercase
        modifier_letter,       ///< A modifier letter
        other_letter,          ///< Other letters, including syllables and ideographs
        nonspacing_mark,       ///< A nonspacing combining mark (zero advance width)
        spacing_mark,          ///< A spacing combining mark (positive advance width)
        enclosing_mark,        ///< An enclosing combining mark
        decimal_number,        ///< A decimal digit
        letter_number,         ///< A letter-like numeric character
        other_number,          ///< A numeric character of other type
        connector_punctuation, ///< A connecting punctuation mark, like a tie
        dash_punctuation,      ///< A dash or hyphen punctuation mark
        open_punctuation,      ///< An opening punctuation mark (of a pair)
        close_punctuation,     ///< A closing punctuation mark (of a pair)
        initial_punctuation,   ///< An initial quotation mark
        final_punctuation,     ///< A final quotation mark
        other_punctuation,     ///< A punctuation mark of other type
        math_symbol,           ///< A symbol of mathematical use
        currency_symbol,       ///< A currency sign
        modifier_symbol,       ///< A non-letter-like modifier symbol
        other_symbol,          ///< A symbol of other type
        space_separator,       ///< A space character (of various non-zero widths)
        line_separator,        ///< U+2028 LINE SEPARATOR only
        paragraph_separator,   ///< U+2029 PARAGRAPH SEPARATOR only
        control,               ///< A C0 or C1 control code
        format,                ///< A format control character
        surrogate,             ///< A surrogate code point
        private_use,           ///< A private-use character
        unassigned,            ///< A reserved unassigned code point or a noncharacter

        lu = uppercase_letter,      ///< Abbreviated alias for `uppercase_letter`
        ll = lowercase_letter,      ///< Abbreviated alias for `lowercase_letter`
        lt = titlecase_letter,      ///< Abbreviated alias for `titlecase_letter`
        lm = modifier_letter,       ///< Abbreviated alias for `modifier_letter`
        lo = other_letter,          ///< Abbreviated alias for `other_letter`
        mn = nonspacing_mark,       ///< Abbreviated alias for `nonspacing_mark`
        mc = spacing_mark,          ///< Abbreviated alias for `spacing_mark`
        me = enclosing_mark,        ///< Abbreviated alias for `enclosing_mark`
        nd = decimal_number,        ///< Abbreviated alias for `decimal_number`
        nl = letter_number,         ///< Abbreviated alias for `letter_number`
        no = other_number,          ///< Abbreviated alias for `other_number`
        pc = connector_punctuation, ///< Abbreviated alias for `connector_punctuation`
        pd = dash_punctuation,      ///< Abbreviated alias for `dash_punctuation`
        ps = open_punctuation,      ///< Abbreviated alias for `open_punctuation`
        pe = close_punctuation,     ///< Abbreviated alias for `close_punctuation`
        pi = initial_punctuation,   ///< Abbreviated alias for `initial_punctuation`
        pf = final_punctuation,     ///< Abbreviated alias for `final_punctuation`
        po = other_punctuation,     ///< Abbreviated alias for `other_punctuation`
        sm = math_symbol,           ///< Abbreviated alias for `math_symbol`
        sc = currency_symbol,       ///< Abbreviated alias for `currency_symbol`
        sk = modifier_symbol,       ///< Abbreviated alias for `modifier_symbol`
        so = other_symbol,          ///< Abbreviated alias for `other_symbol`
        zs = space_separator,       ///< Abbreviated alias for `space_separator`
        zl = line_separator,        ///< Abbreviated alias for `line_separator`
        zp = paragraph_separator,   ///< Abbreviated alias for `paragraph_separator`
        cc = control,               ///< Abbreviated alias for `control`
        cf = format,                ///< Abbreviated alias for `format`
        cs = surrogate,             ///< Abbreviated alias for `surrogate`
        co = private_use,           ///< Abbreviated alias for `private_use`
        cn = unassigned,            ///< Abbreviated alias for `unassigned`
    };

    /// @brief The normalization `Quick_Check` property value of a code point.
    ///
    /// See [Unicode Standard Annex #15, Detecting Normalization Forms](https://www.unicode.org/reports/tr15/#Detecting_Normalization_Forms).
    ///
    enum class quick_check : std::uint8_t
    {
        /// Characters that cannot ever occur in the respective normalization form
        no = impl::unicode_data::core_properties::impl::quick_check_no,

        /// Characters that may occur in the respective normalization form, depending on the context
        maybe = impl::unicode_data::core_properties::impl::quick_check_maybe,

        /// All other characters. This is the default value for the `Quick_Check` properties
        yes = impl::unicode_data::core_properties::impl::quick_check_yes,
    };

    /// @brief A Unicode character type representing a single [Unicode scalar value](https://www.unicode.org/glossary/#unicode_scalar_value).
    ///
    /// @headerfile "" <uni-cpp/uchar.hpp>
    ///
    class uchar
    {
    public:
        /// A sized range of UTF-8 code units returned by the `encode_as_utf8` method. See its documentation for more.
        using encode_as_utf8_t = impl::encode_as_utf<char8_t, 4>;
        /// A sized range of UTF-16 code units returned by the `encode_as_utf16` method. See its documentation for more.
        using encode_as_utf16_t = impl::encode_as_utf<char16_t, 2>;

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

        /// @brief An enum used in the `from_digit` function.
        ///
        enum class from_digit_case
        {
            lowercase, ///< Uses letters `a-z` for representing digits with values `10-35`.
            uppercase, ///< Uses letters `A-Z` for representing digits with values `10-35`.
        };

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

        /// @brief Returns the composition of two code points, if one exists.
        ///
        /// @param code_point1 The starter code point.
        /// @param code_point2 The code point to compose with the starter.
        ///
        [[nodiscard]] static constexpr std::optional<uchar> composition(uchar code_point1, uchar code_point2) noexcept
        {
            return impl::unicode_data::composition_mapping::composition<upp::uchar>(code_point1.value(), code_point2.value());
        }

        /// @brief Converts a digit in the given radix to a `uchar`.
        ///
        /// @param num   The digit to convert.
        /// @param radix The radix (base) of the digit. `2` is binary, `10` is decimal, `16` is hex, etc.
        ///
        /// @return A character representing the digit `num`: `'0'-'9'` for values `0-9` and `'A'-'Z'` for values `10-35`.
        ///         Returns `std::nullopt` if the input is not a digit in the given radix, i.e., @p num ≥ @p radix.
        ///
        /// @tparam Case By default, the values `10-35` are represented using the uppercase letters `A-Z`.
        ///              This template parameter can be overriden to use the lowercase letters `a-z` for these values instead.
        ///
        /// @pre @p radix ≤ `36` <br><small><i><b>Note:</b> If this precondition isn't met, the behavior is undefined.</i></small>
        ///
        /// @par Examples
        ///
        /// @code{.cpp}
        ///
        /// assert(uchar::from_digit( 4, 10) == U'4'_uc);
        /// assert(uchar::from_digit(13, 16) == U'D'_uc);
        ///
        /// // Use lowercase:
        /// assert(uchar::from_digit<uchar::from_digit_case::lowercase>(22, 32) == U'm'_uc);
        ///
        /// // `7` is not a binary digit:
        /// assert(uchar::from_digit(7, 2) == std::nullopt);
        ///
        /// // Undefined behaviour, `radix` > 36:
        /// // assert(uchar::from_digit(50, 100) == ???);
        ///
        /// @endcode
        ///
        /// @see to_digit, is_digit
        ///
        template<from_digit_case Case = from_digit_case::uppercase>
        [[nodiscard]] static constexpr std::optional<uchar> from_digit(std::uint8_t num, std::uint8_t radix) noexcept
        {
            if (num >= radix)
                return {};

            const std::uint32_t num32 = num;

            constexpr std::uint32_t digit_0 = 0x30u;

            if (num32 < 10u)
                return std::optional<uchar>{std::in_place, uchar{digit_0 + num32}};

            if constexpr (Case == from_digit_case::lowercase)
            {
                constexpr std::uint32_t lowercase_letter_a = 0x61u;

                return std::optional<uchar>{std::in_place, uchar{lowercase_letter_a + num32 - 10u}};
            }
            else if constexpr (Case == from_digit_case::uppercase)
            {
                constexpr std::uint32_t uppercase_letter_a = 0x41u;

                return std::optional<uchar>{std::in_place, uchar{uppercase_letter_a + num32 - 10u}};
            }
            else
                static_assert(false);
        }

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
        /// @see as_ascii, as_ascii_lossy, as_ascii_unchecked
        ///
        [[nodiscard]] constexpr bool is_ascii() const noexcept { return m_value < 0x80; }

        /// @brief Attempts to convert the character to an `ascii_char`, if possible (`is_ascii() == true`).
        ///
        /// Use `as_ascii_lossy` to substitute non-ASCII characters with the ASCII substitute character, or `as_ascii_unchecked`
        /// if you are certain this character is within the ASCII range and want to avoid a check.
        ///
        /// @see is_ascii, as_ascii_lossy, as_ascii_unchecked
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
        /// @see is_ascii, as_ascii, as_ascii_unchecked
        ///
        [[nodiscard]] constexpr ascii_char as_ascii_lossy() const noexcept
        {
            if (is_ascii())
                return ascii_char::from_unchecked(static_cast<std::uint8_t>(m_value));

            return ascii_char::substitute_character();
        }

        /// @brief Constructs an `ascii_char` from this `uchar` without any checks.
        ///
        /// This function constructs an `ascii_char` assuming this `uchar` is within the ASCII range.
        ///
        /// @pre `is_ascii()` is `true`
        ///
        /// @warning If the precondition of this function isn't met, the behavior is undefined.
        /// Use `as_ascii` or `as_ascii_lossy` as safe alternatives that perform validation.
        ///
        /// @see is_ascii, as_ascii, as_ascii_lossy
        ///
        [[nodiscard]] constexpr ascii_char as_ascii_unchecked() const noexcept
        {
            return ascii_char::from_unchecked(static_cast<std::uint8_t>(m_value));
        }

        /// @brief Returns the number of UTF-8 code units (bytes) required to encode this `uchar` in UTF-8.
        ///
        /// @return Number between 1 and 4, inclusive.
        ///
        /// @see length_utf16, encode_as_utf8
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
        /// @see length_utf8, encode_as_utf16
        ///
        [[nodiscard]] constexpr std::size_t length_utf16() const noexcept { return (m_value < 0x10000) ? 1uz : 2uz; }

        /// @brief Returns a sequence of UTF-8 code units (bytes) representing this character encoded in UTF-8.
        ///
        /// @return A sized range of `char8_t`s that are UTF-8 code units.
        ///
        /// @see encode_as_utf16, length_utf8
        ///
        [[nodiscard]] constexpr encode_as_utf8_t encode_as_utf8() const noexcept
        {
            using buffer_t = impl::inplace_vector<char8_t, 4>;

            // clang-format off

            switch (length_utf8())
            {
            case 1uz: {
                return encode_as_utf8_t{buffer_t{static_cast<char8_t>(m_value)}};
            }
            case 2uz: {
                return encode_as_utf8_t{buffer_t{
                    static_cast<char8_t>((m_value >> 6U) | 0xC0U),
                    static_cast<char8_t>((m_value & 0x3FU) | 0x80U)
                }};
            }
            case 3uz: {
                return encode_as_utf8_t{buffer_t{
                    static_cast<char8_t>((m_value >> 12U) | 0xE0U),
                    static_cast<char8_t>(((m_value >> 6U) & 0x3FU) | 0x80U),
                    static_cast<char8_t>((m_value & 0x3FU) | 0x80U)
                }};
            }
            case 4uz: {
                return encode_as_utf8_t{buffer_t{
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
        /// @see encode_as_utf8, length_utf16
        ///
        [[nodiscard]] constexpr encode_as_utf16_t encode_as_utf16() const noexcept
        {
            using buffer_t = impl::inplace_vector<char16_t, 2>;

            switch (length_utf16())
            {
            case 1uz: {
                return encode_as_utf16_t{buffer_t{static_cast<char16_t>(m_value)}};
            }
            case 2uz: {
                const std::uint32_t code = m_value - 0x10'000;

                return encode_as_utf16_t{buffer_t{
                    // clang-format off
                    static_cast<char16_t>(0xD800U | (code >> 10U)),
                    static_cast<char16_t>(0xDC00U | (code & 0x3FFU))
                    // clang-format on
                }};
            }
            default: std::unreachable();
            }
        }

        /// @brief Checks whether this code point has been assigned a meaning by Unicode, as of `upp::unicode_version`.
        ///
        /// @return `false` for characters which have the `Unassigned` [general category][GeneralCategory],
        ///         including the [noncharacters][Noncharacters]. `true` for all other characters.
        ///
        /// [GeneralCategory]: https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124142 "The Unicode Standard, Chapter 4.5 (General Category)"
        /// [Noncharacters]: https://www.unicode.org/faq/private_use.html#noncharacters "Unicode FAQ, Noncharacters"
        ///
        [[nodiscard]] constexpr bool is_assigned() const noexcept { return general_category() != upp::general_category::unassigned; }

        /// @brief Returns `true` if this `uchar` is a [noncharacter][Noncharacters].
        ///
        /// [Noncharacters]: https://www.unicode.org/faq/private_use.html#noncharacters "Unicode FAQ, Noncharacters"
        ///
        [[nodiscard]] constexpr bool is_noncharacter() const noexcept
        {
            const std::uint32_t low16 = m_value & 0xFFFFu;

            return (m_value >= 0xFDD0u && m_value <= 0xFDEFu) || (low16 == 0xFFFEu || low16 == 0xFFFFu);
        }

        /// @brief Returns `true` if this `uchar` is a [private-use character][PrivateUseChar].
        ///
        /// [PrivateUseChar]: https://www.unicode.org/faq/private_use.html "Unicode FAQ, Private-Use Characters"
        ///
        [[nodiscard]] constexpr bool is_private_use() const noexcept { return general_category() == upp::general_category::private_use; }

        /// @brief Returns `true` if this character has the [Lowercase][CaseDefinitions] property.
        ///
        /// @see is_uppercase, is_titlecase
        /// @see to_lowercase
        ///
        /// [CaseDefinitions]: https://www.unicode.org/versions/latest/core-spec/chapter-4/#G136255 "Unicode 4.2.1 Definitions of Case and Casing"
        ///
        [[nodiscard]] constexpr bool is_lowercase() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::lowercase_bit>();
        }

        /// @brief Returns `true` if this character has the [Uppercase][CaseDefinitions] property.
        ///
        /// @see is_lowercase, is_titlecase
        /// @see to_uppercase
        ///
        /// [CaseDefinitions]: https://www.unicode.org/versions/latest/core-spec/chapter-4/#G136255 "Unicode 4.2.1 Definitions of Case and Casing"
        ///
        [[nodiscard]] constexpr bool is_uppercase() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::uppercase_bit>();
        }

        /// @brief Returns `true` if this character has the `Titlecase_Letter` [general category][GeneralCategory].
        ///
        /// Titlecase in Unicode is **not** the same as uppercase. Uppercase letters are **not** considered titlecase by this function.
        /// This function only considers "digraphs encoded as single code points, with their first part uppercase" to be titlecase characters.
        /// For example, the code point 'A' _is **not**_ considered titlecase. The digraph code point 'ǲ' *is* considered titlecase.
        ///
        /// @note Try to select the 'D' part of 'ǲ'. You can't, because it's a single code point, a *[digraph](https://www.unicode.org/faq/ligature_digraph.html)*.
        /// The digraph 'Ǳ' is considered uppercase, not titlecase. The digraph 'ǲ' is considered titlecase.
        /// That's also the reason why other regular uppercase letters like 'A' aren't considered titlecase. They are considered uppercase.
        ///
        /// @see is_uppercase, is_lowercase
        /// @see to_titlecase
        ///
        /// [GeneralCategory]: https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124142 "The Unicode Standard, Chapter 4.5 (General Category)"
        ///
        [[nodiscard]] constexpr bool is_titlecase() const noexcept { return general_category() == upp::general_category::titlecase_letter; }

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
        ///
        /// @see to_uppercase, to_casefold, to_titlecase
        /// @see is_lowercase
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
        ///
        /// @see to_lowercase, to_titlecase, to_casefold
        /// @see is_uppercase
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
        ///
        /// @see to_uppercase, to_lowercase, to_casefold
        /// @see is_titlecase
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

        /// @brief Returns `true` if this code point has the [Cased][Cased] property.
        ///
        /// [Cased]: https://www.unicode.org/versions/latest/core-spec/chapter-3/#G44595 "Unicode, Chapter 3, Cased"
        ///
        [[nodiscard]] constexpr bool is_cased() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::cased_bit>();
        }

        /// @brief Returns `true` if this code point is [case-ignorable][CaseIgnorable].
        ///
        /// [CaseIgnorable]: https://www.unicode.org/versions/latest/core-spec/chapter-3/#G63116 "Unicode, Chapter 3, Case-Ignorable"
        ///
        [[nodiscard]] constexpr bool is_case_ignorable() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::case_ignorable_bit>();
        }

        /// @brief Returns `true` if this code point has the [Alphabetic][Alphabetic] property.
        ///
        /// [Alphabetic]: https://www.unicode.org/versions/latest/core-spec/chapter-4/#G32524 "Unicode, Chapter 4, Alphabetic"
        ///
        [[nodiscard]] constexpr bool is_alphabetic() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::alphabetic_bit>();
        }

        /// @brief Returns `true` if this code point has one of the [general categories][GeneralCategory] for numbers.
        ///
        /// Returns `true` if this code point's [general category][GeneralCategory] is one of:
        /// - `Decimal_Number`,
        /// - `Letter_Number`,
        /// - `Other_Number`.
        ///
        /// @note The set of code points this function considers numeric doesn't include everything that could be considered a number.
        ///       For example, ideographic numbers like '三' are **not** considered numeric by this function. See the examples below.
        ///
        /// @par Examples
        ///
        /// @code{.cpp}
        ///
        /// assert(U'2'_uc.is_numeric());
        /// assert(U'¾'_uc.is_numeric());
        /// assert(U'①'_uc.is_numeric());
        ///
        /// assert(not U'و'_uc.is_numeric());
        /// assert(not U'藏'_uc.is_numeric());
        /// assert(not U'三'_uc.is_numeric());
        ///
        /// @endcode
        ///
        /// @see is_digit
        ///
        /// [GeneralCategory]: https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124142 "The Unicode Standard, Chapter 4.5 (General Category)"
        ///
        [[nodiscard]] constexpr bool is_numeric() const noexcept
        {
            switch (general_category())
            {
            case upp::general_category::decimal_number: [[fallthrough]];
            case upp::general_category::letter_number: [[fallthrough]];
            case upp::general_category::other_number: return true;
            default: return false;
            }
        }

        /// @brief Returns `true` if this code point `is_alphabetic()` or `is_numeric()`.
        ///
        /// @see is_alphabetic
        /// @see is_numeric
        ///
        [[nodiscard]] constexpr bool is_alphanumeric() const noexcept { return is_alphabetic() || is_numeric(); }

        /// @brief Returns `true` if this code point has the [White_Space][White_Space] property.
        ///
        /// @par Examples
        ///
        /// @code{.cpp}
        ///
        /// assert(U' '_uc.is_whitespace());
        /// assert(U'\n'_uc.is_whitespace());
        ///
        /// assert(U'\N{NO-BREAK SPACE}'_uc.is_whitespace());
        /// assert(U'\N{THIN SPACE}'_uc.is_whitespace());
        ///
        /// assert(not U'$'_uc.is_whitespace());
        ///
        /// @endcode
        ///
        /// @note It's worth knowing that `is_pattern_whitespace()` can be a better fit in certain contexts than `is_whitespace()`.
        ///
        /// @see is_pattern_whitespace
        ///
        /// [White_Space]: https://www.unicode.org/reports/tr44/#White_Space "UAX #44, Unicode Character Database, White_Space"
        ///
        [[nodiscard]] constexpr bool is_whitespace() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::white_space_bit>();
        }

        /// @brief Returns `true` if this code point has the `Control` [general category][GeneralCategory].
        ///
        /// [GeneralCategory]: https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124142 "The Unicode Standard, Chapter 4.5 (General Category)"
        ///
        [[nodiscard]] constexpr bool is_control() const noexcept { return general_category() == upp::general_category::control; }

        /// @brief Checks if this `uchar` is a digit in the given radix.
        ///
        /// @param radix The radix (base) of the digit. `2` is binary, `10` is decimal, `16` is hex, etc.
        ///
        /// Unlike `is_numeric()`, this function only recognizes the ASCII characters `0-9`, `a-z` and `A-Z` as digits.
        ///
        /// @pre `2` ≤ @p radix ≤ `36` <br><small><i><b>Note:</b> If this precondition isn't met, the behavior is undefined.</i></small>
        ///
        /// @par Examples
        ///
        /// @code{.cpp}
        ///
        /// assert(U'4'_uc.is_digit(10));
        ///
        /// // 'C' is not a decimal digit, but it is a hex digit:
        /// assert(U'C'_uc.is_digit(10) == false);
        /// assert(U'C'_uc.is_digit(16) == true );
        ///
        /// assert(U'1'_uc.is_digit(2));
        ///
        /// // Undefined behaviour, invalid radix:
        /// // assert(U'0'_uc.is_digit(1) == ???);
        /// // assert(U'+'_uc.is_digit(64) == ???);
        ///
        /// @endcode
        ///
        /// @see to_digit, from_digit
        ///
        [[nodiscard]] constexpr bool is_digit(std::uint8_t radix) const noexcept
        {
            return is_ascii() ? (impl::ascii_base36_to_value(static_cast<std::uint8_t>(m_value)) < radix) : false;
        }

        /// @brief Converts this `uchar` to a digit in the given radix.
        ///
        /// @param radix The radix (base) of the digit. `2` is binary, `10` is decimal, `16` is hex, etc.
        ///
        /// 'Digit' is defined to be only the following ASCII characters:
        /// - `0-9` represent the values `0-9`,
        /// - `a-z` represent the values `10-35`,
        /// - `A-Z` represent the values `10-35`.
        ///
        /// @return A value this character represents: values `0-9` for characters `'0'-'9'` and values `10-35` for characters `'a'-'z'` and `'A'-'Z'`.
        ///         Returns `std::nullopt` if this character does not represent a digit in the given radix.
        ///
        /// @pre `2` ≤ @p radix ≤ `36` <br><small><i><b>Note:</b> If this precondition isn't met, the behavior is undefined.</i></small>
        ///
        /// @par Examples
        ///
        /// @code{.cpp}
        ///
        /// assert(U'4'_uc.to_digit(10) == 4);
        ///
        /// // 'C' is not a decimal digit, but it is a hex digit:
        /// assert(U'C'_uc.to_digit(10) == std::nullopt);
        /// assert(U'C'_uc.to_digit(16) == 0x0C);
        ///
        /// assert(U'1'_uc.to_digit(2) == 1);
        ///
        /// // Undefined behaviour, invalid radix:
        /// // assert(U'0'_uc.to_digit(1) == ???);
        /// // assert(U'+'_uc.to_digit(64) == ???);
        ///
        /// @endcode
        ///
        /// @see from_digit, is_digit
        ///
        [[nodiscard]] constexpr std::optional<std::uint8_t> to_digit(std::uint8_t radix) const noexcept
        {
            if (!is_ascii())
                return {};

            const std::uint8_t value = impl::ascii_base36_to_value(static_cast<std::uint8_t>(m_value));

            return value < radix ? std::optional<std::uint8_t>{std::in_place, value} : std::optional<std::uint8_t>{};
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

        /// @brief Returns the [general category](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124142) of this code point.
        ///
        /// Each Unicode code point is assigned a normative `General_Category` value.
        /// The `General_Category` value for a character serves as a basic classification of that character, based on its primary usage.
        /// Many characters have multiple uses, and not all such uses can be captured by a single, simple partition property such as `General_Category`.
        /// Thus, many letters often serve dual functions as numerals in traditional numeral systems. Examples can be found in the Roman numeral system,
        /// in Greek usage of letters as numbers, in Hebrew, and similarly for many scripts. In such cases the `General_Category` is assigned based on
        /// the primary letter usage of the character, even though it may also have numeric values, occur in numeric expressions,
        /// or be used symbolically in mathematical expressions, and so on.
        ///
        /// See [The Unicode Standard, Chapter 4.5 (General Category)](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124142).
        ///
        /// @see upp::general_category
        ///
        [[nodiscard]] constexpr upp::general_category general_category() const noexcept
        {
            return static_cast<upp::general_category>(impl::unicode_data::general_category::impl::lookup(m_value));
        }

        /// @brief Returns the `NFD_Quick_Check` property value of this code point.
        ///
        /// @return Either `upp::quick_check::no` or `upp::quick_check::yes`. `upp::quick_check::maybe` is never returned for this normalization form.
        ///
        /// See [Unicode Standard Annex #15, Detecting Normalization Forms](https://www.unicode.org/reports/tr15/#Detecting_Normalization_Forms).
        ///
        /// @see upp::quick_check
        ///
        [[nodiscard]] constexpr quick_check nfd_quick_check() const noexcept
        {
            return static_cast<quick_check>(get_property_value<impl::unicode_data::core_properties::impl::nfd_quick_check_bit, 1uz>());
        }

        /// @brief Returns the `NFKD_Quick_Check` property value of this code point.
        ///
        /// @return Either `upp::quick_check::no` or `upp::quick_check::yes`. `upp::quick_check::maybe` is never returned for this normalization form.
        ///
        /// See [Unicode Standard Annex #15, Detecting Normalization Forms](https://www.unicode.org/reports/tr15/#Detecting_Normalization_Forms).
        ///
        /// @see upp::quick_check
        ///
        [[nodiscard]] constexpr quick_check nfkd_quick_check() const noexcept
        {
            return static_cast<quick_check>(get_property_value<impl::unicode_data::core_properties::impl::nfkd_quick_check_bit, 1uz>());
        }

        /// @brief Returns the `NFC_Quick_Check` property value of this code point.
        ///
        /// See [Unicode Standard Annex #15, Detecting Normalization Forms](https://www.unicode.org/reports/tr15/#Detecting_Normalization_Forms).
        ///
        /// @see upp::quick_check
        ///
        [[nodiscard]] constexpr quick_check nfc_quick_check() const noexcept
        {
            return static_cast<quick_check>(get_property_value<impl::unicode_data::core_properties::impl::nfc_quick_check_bit, 2uz>());
        }

        /// @brief Returns the `NFKC_Quick_Check` property value of this code point.
        ///
        /// See [Unicode Standard Annex #15, Detecting Normalization Forms](https://www.unicode.org/reports/tr15/#Detecting_Normalization_Forms).
        ///
        /// @see upp::quick_check
        ///
        [[nodiscard]] constexpr quick_check nfkc_quick_check() const noexcept
        {
            return static_cast<quick_check>(get_property_value<impl::unicode_data::core_properties::impl::nfkc_quick_check_bit, 2uz>());
        }

        /// @brief Returns `true` if this code point has the [Pattern_Syntax][Pattern_Syntax] property.
        ///
        /// @see is_pattern_whitespace
        ///
        /// [Pattern_Syntax]: https://www.unicode.org/reports/tr44/#Pattern_Syntax "UAX #44, Unicode Character Database, Pattern_Syntax"
        ///
        [[nodiscard]] constexpr bool is_pattern_syntax() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::pattern_syntax_bit>();
        }

        /// @brief Returns `true` if this code point has the [Pattern_White_Space][Pattern_White_Space] property.
        ///
        /// @par Examples
        ///
        /// @code{.cpp}
        ///
        /// assert(U' '_uc.is_pattern_whitespace());
        /// assert(U'\n'_uc.is_pattern_whitespace());
        ///
        /// assert(not U'$'_uc.is_pattern_whitespace());
        ///
        /// // Unlike `is_whitespace()`, these are not considered Pattern_White_Space:
        ///
        /// assert(not U'\N{NO-BREAK SPACE}'_uc.is_pattern_whitespace());
        /// assert(not U'\N{THIN SPACE}'_uc.is_pattern_whitespace());
        ///
        /// @endcode
        ///
        /// @see is_whitespace
        /// @see is_pattern_syntax
        ///
        /// [Pattern_White_Space]: https://www.unicode.org/reports/tr44/#Pattern_White_Space "UAX #44, Unicode Character Database, Pattern_White_Space"
        ///
        [[nodiscard]] constexpr bool is_pattern_whitespace() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::pattern_white_space_bit>();
        }

        /// @brief Returns `true` if this code point has the [ID_Start][ID_Start] property.
        ///
        /// @see has_id_continue_property
        /// @see has_xid_start_property, has_id_compat_math_start_property
        ///
        /// [ID_Start]: https://www.unicode.org/reports/tr44/#ID_Start "UAX #44, Unicode Character Database, ID_Start"
        ///
        [[nodiscard]] constexpr bool has_id_start_property() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::id_start_bit>();
        }

        /// @brief Returns `true` if this code point has the [ID_Continue][ID_Continue] property.
        ///
        /// @see has_id_start_property
        /// @see has_xid_continue_property, has_id_compat_math_continue_property
        ///
        /// [ID_Continue]: https://www.unicode.org/reports/tr44/#ID_Continue "UAX #44, Unicode Character Database, ID_Continue"
        ///
        [[nodiscard]] constexpr bool has_id_continue_property() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::id_continue_bit>();
        }

        /// @brief Returns `true` if this code point has the [XID_Start][XID_Start] property.
        ///
        /// @see has_xid_continue_property
        /// @see has_id_start_property, has_id_compat_math_start_property
        ///
        /// [XID_Start]: https://www.unicode.org/reports/tr44/#XID_Start "UAX #44, Unicode Character Database, XID_Start"
        ///
        [[nodiscard]] constexpr bool has_xid_start_property() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::xid_start_bit>();
        }

        /// @brief Returns `true` if this code point has the [XID_Continue][XID_Continue] property.
        ///
        /// @see has_xid_start_property
        /// @see has_id_continue_property, has_id_compat_math_continue_property
        ///
        /// [XID_Continue]: https://www.unicode.org/reports/tr44/#XID_Continue "UAX #44, Unicode Character Database, XID_Continue"
        ///
        [[nodiscard]] constexpr bool has_xid_continue_property() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::xid_continue_bit>();
        }

        /// @brief Returns `true` if this code point has the [ID_Compat_Math_Start][ID_Compat_Math_Start] property.
        ///
        /// @see has_id_compat_math_continue_property
        /// @see has_id_start_property, has_xid_start_property
        ///
        /// [ID_Compat_Math_Start]: https://www.unicode.org/reports/tr44/#ID_Compat_Math_Start "UAX #44, Unicode Character Database, ID_Compat_Math_Start"
        ///
        [[nodiscard]] constexpr bool has_id_compat_math_start_property() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::id_compat_math_start_bit>();
        }

        /// @brief Returns `true` if this code point has the [ID_Compat_Math_Continue][ID_Compat_Math_Continue] property.
        ///
        /// @see has_id_compat_math_start_property
        /// @see has_id_continue_property, has_xid_continue_property
        ///
        /// [ID_Compat_Math_Continue]: https://www.unicode.org/reports/tr44/#ID_Compat_Math_Continue "UAX #44, Unicode Character Database, ID_Compat_Math_Continue"
        ///
        [[nodiscard]] constexpr bool has_id_compat_math_continue_property() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::id_compat_math_continue_bit>();
        }

        /// @brief Returns `true` if this code point has the [Math][Math] property.
        ///
        /// [Math]: https://www.unicode.org/versions/latest/core-spec/chapter-22/#G26707 "Unicode, Chapter 22, The Math Property"
        ///
        [[nodiscard]] constexpr bool has_math_property() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::math_bit>();
        }

        /// @brief Returns `true` if this code point has the [Dash][Dash] property.
        ///
        /// [Dash]: https://www.unicode.org/reports/tr44/#Dash "UAX #44, Unicode Character Database, Dash"
        ///
        [[nodiscard]] constexpr bool has_dash_property() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::dash_bit>();
        }

        /// @brief Returns `true` if this code point has the [Quotation_Mark][Quotation_Mark] property.
        ///
        /// [Quotation_Mark]: https://www.unicode.org/reports/tr44/#Quotation_Mark "UAX #44, Unicode Character Database, Quotation_Mark"
        ///
        [[nodiscard]] constexpr bool has_quotation_mark_property() const noexcept
        {
            return get_boolean_property<impl::unicode_data::core_properties::impl::quotation_mark_bit>();
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

        template<std::size_t BitOffset, std::size_t BitLength>
        [[nodiscard]] constexpr std::uint8_t get_property_value() const noexcept
        {
            const std::uint32_t encoded = impl::unicode_data::core_properties::impl::lookup(m_value);

            static constexpr std::uint32_t mask = (1U << BitLength) - 1;

            return static_cast<std::uint8_t>((encoded >> BitOffset) & mask);
        }

        template<std::size_t BitOffset>
        [[nodiscard]] constexpr bool get_boolean_property() const noexcept
        {
            return static_cast<bool>(get_property_value<BitOffset, 1uz>());
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