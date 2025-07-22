export module uni_cpp:uchar;

import uni_cpp.impl.unicode_data;

export import std;

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
/// - Checking character properties
///

namespace upp
{
    export {
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
        template<typename T>
        concept char_type = std::same_as<T, ascii_char> || std::same_as<T, uchar>;
    }

    namespace impl
    {
        /// @brief Immutable, in-place buffer with fixed capacity and dynamic size.
        ///
        /// Small, contiguous iterable buffer used by functions that return a short range of elements with a dynamic size.
        ///
        /// This serves as the base class for:
        /// - `encode_utf8_t`, `encode_utf16_t`,
        /// - `to_lowercase_t`, `to_uppercase_t` and `to_titlecase_t`.
        ///
        /// @tparam T Type of the elements stored in the buffer.
        /// @tparam MaxSize The capacity of the buffer.
        ///
        template<typename T, std::size_t MaxSize>
            requires(MaxSize > 0 && MaxSize <= 255)
        class immutable_inplace_buffer
        {
        public:
            using const_iterator         = const T*;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        public:
            constexpr immutable_inplace_buffer(const immutable_inplace_buffer&) noexcept = default;
            constexpr immutable_inplace_buffer(immutable_inplace_buffer&&) noexcept      = default;

            constexpr ~immutable_inplace_buffer() noexcept = default;

            constexpr immutable_inplace_buffer& operator=(const immutable_inplace_buffer&) noexcept = default;
            constexpr immutable_inplace_buffer& operator=(immutable_inplace_buffer&&) noexcept      = default;

            [[nodiscard]] constexpr const_iterator         begin() const noexcept { return m_data.data(); }
            [[nodiscard]] constexpr const_iterator         cbegin() const noexcept { return begin(); }
            [[nodiscard]] constexpr const_iterator         end() const noexcept { return m_data.data() + m_size; }
            [[nodiscard]] constexpr const_iterator         cend() const noexcept { return end(); }
            [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
            [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
            [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
            [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

            [[nodiscard]] constexpr std::size_t size() const noexcept { return static_cast<std::size_t>(m_size); }

            [[nodiscard]] constexpr const T* data() const noexcept { return m_data.data(); }

            [[nodiscard]] constexpr bool operator==(const immutable_inplace_buffer& other) const noexcept
            {
                if (m_size != other.m_size)
                    return false;

                for (auto it1 = cbegin(), it2 = other.cbegin(); it1 != cend(); ++it1, ++it2)
                {
                    if (*it1 != *it2)
                        return false;
                }
                return true;
            }

        protected:
            /// @brief Constructs the buffer with given data and size.
            ///
            /// @param p_data The array of elements to initialize with.
            /// @param p_size The number of valid elements in the array.
            ///
            constexpr immutable_inplace_buffer(std::array<T, MaxSize> p_data, std::uint8_t p_size) noexcept
                : m_data(p_data)
                , m_size(p_size)
            {
            }

        private:
            std::array<T, MaxSize> m_data;
            std::uint8_t           m_size;
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

        enum class to_case_enum
        {
            lower,
            upper,
            title
        };

        /// @tparam Case Used to make `to_lowercase_t`, `to_uppercase_t` and `to_titlecase_t` distinct types.
        /// @tparam T Always `uchar`; only a template parameter due to forward declaration constraints.
        ///
        template<to_case_enum Case, typename T = uchar>
        class to_case : public immutable_inplace_buffer<T, 3>
        {
        private:
            using base                   = immutable_inplace_buffer<T, 3>;
            static constexpr auto m_case = Case;
            friend uchar;

        public:
            using base::base;
        };
    } // namespace impl

    export {
        /// @brief An ASCII character type representing a single ASCII character code.
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

            /// @brief Constructs `ascii_char` from the given `value` if it's a valid ASCII character, otherwise returns `std::nullopt`.
            ///
            /// Attempts to convert `value` to an `ascii_char`. This conversion succeeds only if
            /// `value` is within the ASCII range `[0, 127]`. If `value` is outside this range, `std::nullopt` is returned.
            ///
            /// Use `from_lossy` to substitute invalid values with a fallback character, or `from_unchecked`
            /// if you are certain `value` is valid and want to avoid a check.
            ///
            /// @see from_lossy, from_unchecked
            ///
            [[nodiscard]] static constexpr std::optional<ascii_char> from(std::uint8_t value) noexcept
            {
                if (is_valid_ascii(value))
                    return std::optional(ascii_char{value});

                return std::optional<ascii_char>();
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
            /// @pre `value` MUST BE in the ASCII range - `[0, 127]`.
            ///
            /// @warning If the precondition of this function isn't met, the behavior is undefined.
            /// Use `from` or `from_lossy` for safe alternatives that perform validation.
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

        /// @brief A Unicode character type representing a single [Unicode scalar value](https://www.unicode.org/glossary/#unicode_scalar_value).
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

            /// @brief Constructs `uchar` from the given `value` if it's a valid Unicode scalar value, otherwise returns `std::nullopt`.
            ///
            /// Attempts to convert `value` to a `uchar`. This conversion succeeds only if
            /// `value` is a valid Unicode scalar value. If it's not, `std::nullopt` is returned.
            ///
            /// Use `from_lossy` to substitute invalid values with a replacement character, or `from_unchecked`
            /// if you are certain `value` is valid and want to avoid a check.
            ///
            /// @see from_lossy, from_unchecked
            ///
            [[nodiscard]] static constexpr std::optional<uchar> from(std::uint32_t value) noexcept
            {
                if (is_valid_usv(value))
                    return std::optional(uchar{value});

                return std::optional<uchar>();
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
            /// @pre `value` MUST BE a valid [Unicode scalar value](https://www.unicode.org/glossary/#unicode_scalar_value).
            ///
            /// @warning If the precondition of this function isn't met, the behavior is undefined.
            /// Use `from` or `from_lossy` for safe alternatives that perform validation.
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
                // license: https://github.com/cceckman/unicode-branchless/blob/main/LICENSE

                static constexpr std::array<std::uint8_t, 33> length_lookup_table{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 3,
                                                                                  3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1};

                return static_cast<std::size_t>(length_lookup_table[std::countl_zero(m_value | 1)]);
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
                std::array<char8_t, 4> arr;
                const std::size_t      size_utf8 = length_utf8();

                switch (size_utf8)
                {
                case 1uz: {
                    arr[0] = static_cast<char8_t>(m_value);
                    break;
                }
                case 2uz: {
                    arr[0] = static_cast<char8_t>((m_value >> 6) | 0xC0U);
                    arr[1] = static_cast<char8_t>((m_value & 0x3FU) | 0x80U);
                    break;
                }
                case 3uz: {
                    arr[0] = static_cast<char8_t>((m_value >> 12) | 0xE0U);
                    arr[1] = static_cast<char8_t>(((m_value >> 6) & 0x3FU) | 0x80U);
                    arr[2] = static_cast<char8_t>((m_value & 0x3FU) | 0x80U);
                    break;
                }
                case 4uz: {
                    arr[0] = static_cast<char8_t>((m_value >> 18) | 0xF0U);
                    arr[1] = static_cast<char8_t>(((m_value >> 12) & 0x3FU) | 0x80U);
                    arr[2] = static_cast<char8_t>(((m_value >> 6) & 0x3FU) | 0x80U);
                    arr[3] = static_cast<char8_t>((m_value & 0x3FU) | 0x80U);
                    break;
                }
                default: std::unreachable();
                }

                return encode_utf8_t(arr, static_cast<std::uint8_t>(size_utf8));
            }

            /// @brief Returns a sequence of UTF-16 code units representing this character encoded in UTF-16.
            ///
            /// @return A sized range of `char16_t`s that are UTF-16 code units.
            ///
            /// @see encode_utf8, length_utf16
            /// 
            [[nodiscard]] constexpr encode_utf16_t encode_utf16() const noexcept
            {
                std::array<char16_t, 2> arr;
                const std::size_t       size_utf16 = length_utf16();

                switch (size_utf16)
                {
                case 1uz: {
                    arr[0] = static_cast<char16_t>(m_value);
                    break;
                }
                case 2uz: {
                    const std::uint32_t code = m_value - 0x10'000;

                    arr[0] = static_cast<char16_t>(0xD800 | (code >> 10));
                    arr[1] = static_cast<char16_t>(0xDC00 | (code & 0x3FF));
                    break;
                }
                default: std::unreachable();
                }

                return encode_utf16_t(arr, static_cast<std::uint8_t>(size_utf16));
            }

            /// @brief Returns a sequence of `uchar`s that are the lowercase mapping of this `uchar`.
            ///
            /// If this `uchar` does not have a lowercase mapping, it maps to itself.
            ///
            /// This conversion is performed without tailoring; it is independent of context and language.
            ///
            /// See [Unicode Standard Chapter 4.2 (Case)](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124722)
            /// and [Unicode Standard Chapter 3.13 (Default Case Algorithms)](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G33992).
            ///
            /// @return A sized range of `uchar`s that are the lowercase mapping.
            /// @see to_uppercase, to_titlecase
            ///
            [[nodiscard]] constexpr to_lowercase_t to_lowercase() const noexcept
            {
                namespace upp_data = impl::unicode_data::case_conversion;

                const auto mapping = upp_data::lookup_case_mapping<upp_data::case_mapping_type::lowercase>(m_value);

                const std::array<uchar, 3> data = {uchar{mapping.code_points[0]}, uchar{mapping.code_points[1]}, uchar{mapping.code_points[2]}};

                return to_lowercase_t{data, mapping.length};
            }

            /// @brief Returns a sequence of `uchar`s that are the uppercase mapping of this `uchar`.
            ///
            /// If this `uchar` does not have an uppercase mapping, it maps to itself.
            ///
            /// This conversion is performed without tailoring; it is independent of context and language.
            ///
            /// See [Unicode Standard Chapter 4.2 (Case)](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124722)
            /// and [Unicode Standard Chapter 3.13 (Default Case Algorithms)](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G33992).
            ///
            /// @return A sized range of `uchar`s that are the uppercase mapping.
            /// @see to_lowercase, to_titlecase
            ///
            [[nodiscard]] constexpr to_uppercase_t to_uppercase() const noexcept
            {
                namespace upp_data = impl::unicode_data::case_conversion;

                const auto mapping = upp_data::lookup_case_mapping<upp_data::case_mapping_type::uppercase>(m_value);

                const std::array<uchar, 3> data = {uchar{mapping.code_points[0]}, uchar{mapping.code_points[1]}, uchar{mapping.code_points[2]}};

                return to_uppercase_t{data, mapping.length};
            }

            /// @brief Returns a sequence of `uchar`s that are the titlecase mapping of this `uchar`.
            ///
            /// If this `uchar` does not have a titlecase mapping, it maps to itself.
            ///
            /// This conversion is performed without tailoring; it is independent of context and language.
            ///
            /// See [Unicode Standard Chapter 4.2 (Case)](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124722)
            /// and [Unicode Standard Chapter 3.13 (Default Case Algorithms)](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G33992).
            ///
            /// @note Titlecase in Unicode is **NOT** the same as uppercase.
            ///       See [Unicode Standard Chapter 4.2 (Case)](https://www.unicode.org/versions/latest/core-spec/chapter-4/#G124722).
            ///
            /// @return A sized range of `uchar`s that are the titlecase mapping.
            /// @see to_lowercase, to_uppercase
            ///
            [[nodiscard]] constexpr to_titlecase_t to_titlecase() const noexcept
            {
                namespace upp_data = impl::unicode_data::case_conversion;

                const auto mapping = upp_data::lookup_case_mapping<upp_data::case_mapping_type::titlecase>(m_value);

                const std::array<uchar, 3> data = {uchar{mapping.code_points[0]}, uchar{mapping.code_points[1]}, uchar{mapping.code_points[2]}};

                return to_titlecase_t{data, mapping.length};
            }

        private:
            explicit constexpr uchar(std::uint32_t value) noexcept
                : m_value(value)
            {
            }

        private:
            std::uint32_t m_value;
        };

        inline namespace literals
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
        } // namespace literals
    }
} // namespace upp