#ifndef UNI_CPP_ENCODING_HPP
#define UNI_CPP_ENCODING_HPP

/// @file
///
/// @brief Provides enumerations of text encodings and traits for those encodings.
///

#include "uchar.hpp"

#include "impl/encoding/ascii.hpp"
#include "impl/encoding/utf8.hpp"
#include "impl/encoding/utf16.hpp"
#include "impl/encoding/utf32.hpp"

#include <cstdint>
#include <utility>
#include <concepts>
#include <optional>
#include <bit>
#include <ranges>
#include <expected>
#include <functional>

namespace upp
{
    /// @brief Enumeration of text encodings.
    ///
    /// @see unicode_encoding
    ///
    enum class encoding : std::uint8_t
    {
        ascii, ///< ASCII encoding
        utf8,  ///< UTF-8 encoding
        utf16, ///< UTF-16 encoding
        utf32  ///< UTF-32 encoding
    };

    /// @brief Enumeration of Unicode text encodings.
    ///
    /// @see encoding
    ///
    enum class unicode_encoding : std::uint8_t
    {
        utf8  = std::to_underlying(encoding::utf8),  ///< UTF-8 encoding, equal to `encoding::utf8`
        utf16 = std::to_underlying(encoding::utf16), ///< UTF-16 encoding, equal to `encoding::utf16`
        utf32 = std::to_underlying(encoding::utf32)  ///< UTF-32 encoding, equal to `encoding::utf32`
    };

    /// @brief Provides traits for a given text encoding.
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<encoding Encoding>
    struct encoding_traits;

    /// @copydoc encoding_traits
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<>
    struct encoding_traits<encoding::ascii>
    {
        /// The uni-cpp type for storing code points of a given encoding.
        using char_type = ascii_char;

        /// Built-in integral type for storing code units of a given encoding.
        using default_code_unit_type = char;

        /// Error type returned by ASCII validating, decoding and transcoding functions.
        using error_type = ascii_error;

        /// `true` for [variable-width encodings](https://en.wikipedia.org/wiki/Variable-length_encoding), otherwise `false`.
        static constexpr bool is_variable_width = false;

        /// `true` iff type `T` could be used as a code unit type for the ASCII encoding.
        template<typename T>
        static constexpr bool is_code_unit_type = std::integral<T> && sizeof(T) == sizeof(default_code_unit_type);

        /// `true` iff type `R` is a range of ASCII code units.
        template<typename R>
        static constexpr bool is_code_unit_range = std::ranges::range<R> && is_code_unit_type<std::remove_cvref_t<std::ranges::range_reference_t<R>>>;

        /// @brief Validates a range of ASCII.
        ///
        /// @param range Range of ASCII code units (ASCII character codes) to verify.
        ///
        /// @param code_unit_callback Callback function called with every code unit (character code) of the range (unless the range is invalid ASCII).
        /// It should have exactly one parameter of type `char`.
        /// This makes it possible to use this function with single-pass ranges while also getting the code units out of that range.
        ///
        /// @return `std::expected<void, ascii_error>` containing: `void` if the range is valid ASCII, otherwise `ascii_error`.
        ///
        /// @see decode_range, decode_range_lossy, decode_range_unchecked
        ///
        template<std::ranges::input_range Range, std::invocable<default_code_unit_type> CodeUnitCallback>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> validate_range(Range&& range, const CodeUnitCallback& code_unit_callback)
        {
            using expected_type = std::expected<void, ascii_error>;

            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            std::size_t index = 0;

            for (; it != sentinel; ++index, ++it)
            {
                const std::uint8_t code_unit = std::bit_cast<std::uint8_t>(*it);

                if (!is_valid_ascii(code_unit))
                {
                    return expected_type{std::unexpect, ascii_error{.valid_up_to = index}};
                }

                std::invoke(code_unit_callback, std::bit_cast<char>(code_unit));
            }

            return expected_type{};
        }

        /// @brief Validates a range of ASCII.
        ///
        /// @param range Range of ASCII code units (ASCII character codes) to verify.
        ///
        /// @return `std::expected<void, ascii_error>` containing: `void` if the range is valid ASCII, otherwise `ascii_error`.
        ///
        /// @see decode_range, decode_range_lossy, decode_range_unchecked
        ///
        template<std::ranges::input_range Range>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> validate_range(Range&& range)
        {
            return validate_range(std::forward<Range>(range), [](char) static {});
        }

        /// @brief Decodes a range of ASCII with error checking.
        ///
        /// @param range Range of ASCII code units (ASCII character codes) to decode.
        ///
        /// @param code_point_callback Callback function called with every decoded ASCII character.
        /// It should have exactly one parameter of type `upp::ascii_char`.
        ///
        /// @return `std::expected<void, ascii_error>` containing: `void` if no error occurs, otherwise `ascii_error`.
        ///
        /// @see decode_range_lossy, decode_range_unchecked, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> decode_range(Range&& range, const CodePointCallback& code_point_callback)
        {
            using expected_type = std::expected<void, ascii_error>;

            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            std::size_t index = 0;

            for (; it != sentinel; ++index, ++it)
            {
                const std::uint8_t code_unit = std::bit_cast<std::uint8_t>(*it);

                const std::optional<ascii_char> code_point = ascii_char::from(code_unit);

                if (!code_point.has_value())
                {
                    return expected_type{std::unexpect, ascii_error{.valid_up_to = index}};
                }

                std::invoke(code_point_callback, *code_point);
            }

            return expected_type{};
        }

        /// @brief Lossily decodes a range of ASCII.
        ///
        /// Decodes a range of ASCII while replacing invalid character codes with `ascii_char::substitute_character()`.
        ///
        /// @param range Range of ASCII code units (ASCII character codes) to lossily decode.
        ///
        /// @param code_point_callback Callback function called with every decoded ASCII character, including the substitute characters.
        /// It should have exactly one parameter of type `upp::ascii_char`.
        ///
        /// @see decode_range, decode_range_unchecked, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        static constexpr void decode_range_lossy(Range&& range, const CodePointCallback& code_point_callback)
        {
            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            for (; it != sentinel; ++it)
            {
                const ascii_char code_point = ascii_char::from_lossy(std::bit_cast<std::uint8_t>(*it));

                std::invoke(code_point_callback, code_point);
            }
        }

        /// @brief Decodes a range of ASCII without error checking.
        ///
        /// @param range Range of ASCII code units (ASCII character codes) to decode.
        ///
        /// @param code_point_callback Callback function called with every decoded ASCII character.
        /// It should have exactly one parameter of type `upp::ascii_char`.
        ///
        /// @see decode_range, decode_range_lossy, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        static constexpr void decode_range_unchecked(Range&& range, const CodePointCallback& code_point_callback)
        {
            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            for (; it != sentinel; ++it)
            {
                const auto code_point = ascii_char::from_unchecked(std::bit_cast<std::uint8_t>(*it));

                std::invoke(code_point_callback, code_point);
            }
        }
    };

    /// @copydoc encoding_traits
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<>
    struct encoding_traits<encoding::utf8>
    {
        /// The uni-cpp type for storing code points of a given encoding.
        using char_type = uchar;

        /// Built-in integral type for storing code units of a given encoding.
        using default_code_unit_type = char8_t;

        /// Error type returned by UTF-8 validating, decoding and transcoding functions.
        using error_type = utf8_error;

        /// `true` for [variable-width encodings](https://en.wikipedia.org/wiki/Variable-length_encoding), otherwise `false`.
        static constexpr bool is_variable_width = true;

        /// `true` iff type `T` could be used as a code unit type for the UTF-8 encoding.
        template<typename T>
        static constexpr bool is_code_unit_type = std::integral<T> && sizeof(T) == sizeof(default_code_unit_type);

        /// `true` iff type `R` is a range of UTF-8 code units.
        template<typename R>
        static constexpr bool is_code_unit_range = std::ranges::range<R> && is_code_unit_type<std::remove_cvref_t<std::ranges::range_reference_t<R>>>;

        /// @brief Validates a range of UTF-8.
        ///
        /// @param range Range of UTF-8 code units to verify.
        ///
        /// @param code_unit_callback Callback function called with every code unit of the range (unless the range is invalid UTF-8).
        /// It should have exactly one parameter of type `char8_t`.
        /// This makes it possible to use this function with single-pass ranges while also getting the code units out of that range.
        ///
        /// @return `std::expected<void, utf8_error>` containing: `void` if the range is valid UTF-8, otherwise `utf8_error`.
        ///
        /// @see decode_range, decode_range_lossy, decode_range_unchecked
        ///
        template<std::ranges::input_range Range, std::invocable<default_code_unit_type> CodeUnitCallback>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> validate_range(Range&& range, const CodeUnitCallback& code_unit_callback)
        {
            using expected_type = std::expected<void, utf8_error>;

            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            std::uint32_t state       = impl::utf8::dfa::state::accept;
            std::size_t   valid_up_to = 0;

            for (std::size_t index = 0; it != sentinel; ++index, ++it)
            {
                const char8_t current_code_unit = std::bit_cast<char8_t>(*it);

                const std::uint32_t type = impl::utf8::dfa::character_class_from_byte[current_code_unit];

                state = impl::utf8::dfa::state_transition_table[state + type];

                if (state == impl::utf8::dfa::state::reject)
                {
                    const std::size_t invalid_code_units_length = index - valid_up_to + 1uz;

                    const std::uint8_t error_length = impl::utf8::get_error_length_from_invalid_code_units_length(invalid_code_units_length);

                    return expected_type{std::unexpect, utf8_error{.valid_up_to = valid_up_to, .error_length = error_length}};
                }

                if (state == impl::utf8::dfa::state::accept)
                    valid_up_to = index + 1;

                std::invoke(code_unit_callback, current_code_unit);
            }

            // Check if the range ended in the middle of a code point.

            if (state != impl::utf8::dfa::state::accept)
            {
                return expected_type{std::unexpect, utf8_error{.valid_up_to = valid_up_to, .error_length = {std::nullopt}}};
            }

            return expected_type{};
        }

        /// @brief Validates a range of UTF-8.
        ///
        /// @param range Range of UTF-8 code units to verify.
        ///
        /// @return `std::expected<void, utf8_error>` containing: `void` if the range is valid UTF-8, otherwise `utf8_error`.
        ///
        /// @see decode_range, decode_range_lossy, decode_range_unchecked
        ///
        template<std::ranges::input_range Range>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> validate_range(Range&& range)
        {
            return validate_range(std::forward<Range>(range), [](char8_t) static {});
        }

        /// @brief Decodes a range of UTF-8 with error checking.
        ///
        /// @param range Range of UTF-8 code units to decode.
        ///
        /// @param code_point_callback Callback function called with every decoded code point.
        /// It should have exactly one parameter of type `upp::uchar`.
        ///
        /// @return `std::expected<void, utf8_error>` containing: `void` if no error occurs, otherwise `utf8_error`.
        ///
        /// @see decode_range_lossy, decode_range_unchecked, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> decode_range(Range&& range, const CodePointCallback& code_point_callback)
        {
            using expected_type = std::expected<void, utf8_error>;

            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            std::uint32_t state       = impl::utf8::dfa::state::accept;
            std::size_t   valid_up_to = 0;

            std::uint32_t current_code_point;

            for (std::size_t index = 0; it != sentinel; ++index, ++it)
            {
                const char8_t current_code_unit = std::bit_cast<char8_t>(*it);

                const std::uint32_t type = impl::utf8::dfa::character_class_from_byte[current_code_unit];

                current_code_point = (state != impl::utf8::dfa::state::accept) ? (current_code_unit & 0x3FU) | (current_code_point << 6)
                                                                               : (0xFF >> type) & (current_code_unit);

                state = impl::utf8::dfa::state_transition_table[state + type];

                if (state == impl::utf8::dfa::state::reject)
                {
                    const std::size_t invalid_code_units_length = index - valid_up_to + 1uz;

                    const std::uint8_t error_length = impl::utf8::get_error_length_from_invalid_code_units_length(invalid_code_units_length);

                    return expected_type{std::unexpect, utf8_error{.valid_up_to = valid_up_to, .error_length = error_length}};
                }

                if (state == impl::utf8::dfa::state::accept)
                {
                    valid_up_to = index + 1;

                    std::invoke(code_point_callback, uchar::from_unchecked(current_code_point));
                }
            }

            // Check if the range ended in the middle of a code point.

            if (state != impl::utf8::dfa::state::accept)
            {
                return expected_type{std::unexpect, utf8_error{.valid_up_to = valid_up_to, .error_length = {std::nullopt}}};
            }

            return expected_type{};
        }

        /// @brief Lossily decodes a range of UTF-8.
        ///
        /// Decodes a range of UTF-8 while replacing invalid subsequences with `uchar::replacement_character()`.
        ///
        /// @param range Range of UTF-8 code units to lossily decode.
        ///
        /// @param code_point_callback Callback function called with every decoded code point, including the replacement characters.
        /// It should have exactly one parameter of type `upp::uchar`.
        ///
        /// @see decode_range, decode_range_unchecked, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        static constexpr void decode_range_lossy(Range&& range, const CodePointCallback& code_point_callback)
        {
            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            std::uint32_t state = impl::utf8::dfa::state::accept;

            std::uint32_t current_code_point;

            std::uint8_t code_units_since_last_state_accept = 1;

            // Reuse the previous code unit due to an error that happened.
            // Note: there is no error under which multiple code units would need to be reused.
            bool reuse_previous_code_unit = false;

            char8_t current_code_unit;

            for (; reuse_previous_code_unit || it != sentinel; ++code_units_since_last_state_accept)
            {
                if (!reuse_previous_code_unit)
                {
                    current_code_unit = std::bit_cast<char8_t>(*it);
                    ++it;
                }
                else
                    reuse_previous_code_unit = false;

                const std::uint32_t type = impl::utf8::dfa::character_class_from_byte[current_code_unit];

                current_code_point = (state != impl::utf8::dfa::state::accept) ? (current_code_unit & 0x3FU) | (current_code_point << 6)
                                                                               : (0xFF >> type) & (current_code_unit);

                state = impl::utf8::dfa::state_transition_table[state + type];

                if (state == impl::utf8::dfa::state::reject)
                {
                    const std::uint8_t error_length = impl::utf8::get_error_length_from_invalid_code_units_length(code_units_since_last_state_accept);

                    std::invoke(code_point_callback, uchar::replacement_character());

                    if (error_length < code_units_since_last_state_accept)
                        reuse_previous_code_unit = true;

                    state                              = impl::utf8::dfa::state::accept;
                    code_units_since_last_state_accept = 0;
                }
                else if (state == impl::utf8::dfa::state::accept)
                {
                    code_units_since_last_state_accept = 0;

                    std::invoke(code_point_callback, uchar::from_unchecked(current_code_point));
                }
            }

            // Check if the range ended in the middle of a code point.

            if (state != impl::utf8::dfa::state::accept)
            {
                std::invoke(code_point_callback, uchar::replacement_character());
            }
        }

        /// @brief Decodes a range of UTF-8 without error checking.
        ///
        /// @param range Range of UTF-8 code units to decode.
        ///
        /// @param code_point_callback Callback function called with every decoded code point.
        /// It should have exactly one parameter of type `upp::uchar`.
        ///
        /// @see decode_range, decode_range_lossy, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        static constexpr void decode_range_unchecked(Range&& range, const CodePointCallback& code_point_callback)
        {
            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            std::uint32_t state = impl::utf8::dfa::state::accept;
            std::uint32_t current_code_point;

            for (; it != sentinel; ++it)
            {
                const char8_t current_code_unit = std::bit_cast<char8_t>(*it);

                std::uint32_t type = impl::utf8::dfa::character_class_from_byte[current_code_unit];

                current_code_point = (state != impl::utf8::dfa::state::accept) ? (current_code_unit & 0x3FU) | (current_code_point << 6)
                                                                               : (0xFF >> type) & (current_code_unit);

                state = impl::utf8::dfa::state_transition_table[state + type];

                if (state == impl::utf8::dfa::state::accept)
                {
                    std::invoke(code_point_callback, uchar::from_unchecked(current_code_point));
                }
            }
        }
    };

    /// @copydoc encoding_traits
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<>
    struct encoding_traits<encoding::utf16>
    {
        /// The uni-cpp type for storing code points of a given encoding.
        using char_type = uchar;

        /// Built-in integral type for storing code units of a given encoding.
        using default_code_unit_type = char16_t;

        /// Error type returned by UTF-16 validating, decoding and transcoding functions.
        using error_type = utf16_error;

        /// `true` for [variable-width encodings](https://en.wikipedia.org/wiki/Variable-length_encoding), otherwise `false`.
        static constexpr bool is_variable_width = true;

        /// `true` iff type `T` could be used as a code unit type for the UTF-16 encoding.
        template<typename T>
        static constexpr bool is_code_unit_type = std::integral<T> && sizeof(T) == sizeof(default_code_unit_type);

        /// `true` iff type `R` is a range of UTF-16 code units.
        template<typename R>
        static constexpr bool is_code_unit_range = std::ranges::range<R> && is_code_unit_type<std::remove_cvref_t<std::ranges::range_reference_t<R>>>;

        /// @brief Validates a range of UTF-16.
        ///
        /// @param range Range of UTF-16 code units to verify.
        ///
        /// @param code_unit_callback Callback function called with every code unit of the range (unless the range is invalid UTF-16).
        /// It should have exactly one parameter of type `char16_t`.
        /// This makes it possible to use this function with single-pass ranges while also getting the code units out of that range.
        ///
        /// @return `std::expected<void, utf16_error>` containing: `void` if the range is valid UTF-16, otherwise `utf16_error`.
        ///
        /// @see decode_range, decode_range_lossy, decode_range_unchecked
        ///
        template<std::ranges::input_range Range, std::invocable<default_code_unit_type> CodeUnitCallback>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> validate_range(Range&& range, const CodeUnitCallback& code_unit_callback)
        {
            using expected_type = std::expected<void, utf16_error>;

            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            std::size_t index = 0;

            while (it != sentinel)
            {
                const std::size_t valid_up_to = index;

                const char16_t first_code_unit = std::bit_cast<char16_t>(*it);
                ++index, ++it;

                std::invoke(code_unit_callback, first_code_unit);

                if (impl::utf16::is_surrogate(first_code_unit))
                {
                    if (first_code_unit >= 0xDC00U)
                        return expected_type{std::unexpect, utf16_error{.valid_up_to = valid_up_to, .error_length = 1}};

                    if (it == sentinel)
                        return expected_type{std::unexpect, utf16_error{.valid_up_to = valid_up_to, .error_length = std::nullopt}};

                    const char16_t second_code_unit = std::bit_cast<char16_t>(*it);
                    ++index, ++it;

                    if (second_code_unit < 0xDC00U || second_code_unit > 0xDFFFU)
                        return expected_type{std::unexpect, utf16_error{.valid_up_to = valid_up_to, .error_length = 1}};

                    std::invoke(code_unit_callback, second_code_unit);
                }
            }

            return expected_type{};
        }

        /// @brief Validates a range of UTF-16.
        ///
        /// @param range Range of UTF-16 code units to verify.
        ///
        /// @return `std::expected<void, utf16_error>` containing: `void` if the range is valid UTF-16, otherwise `utf16_error`.
        ///
        /// @see decode_range, decode_range_lossy, decode_range_unchecked
        ///
        template<std::ranges::input_range Range>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> validate_range(Range&& range)
        {
            return validate_range(std::forward<Range>(range), [](char16_t) static {});
        }

        /// @brief Decodes a range of UTF-16 with error checking.
        ///
        /// @param range Range of UTF-16 code units to decode.
        ///
        /// @param code_point_callback Callback function called with every decoded code point.
        /// It should have exactly one parameter of type `upp::uchar`.
        ///
        /// @return `std::expected<void, utf16_error>` containing: `void` if no error occurs, otherwise `utf16_error`.
        ///
        /// @see decode_range_lossy, decode_range_unchecked, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> decode_range(Range&& range, const CodePointCallback& code_point_callback)
        {
            using expected_type = std::expected<void, utf16_error>;

            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            std::size_t index = 0;

            while (it != sentinel)
            {
                const std::size_t valid_up_to = index;

                const char16_t first_code_unit = std::bit_cast<char16_t>(*it);
                ++index, ++it;

                if (impl::utf16::is_surrogate(first_code_unit))
                {
                    if (first_code_unit >= 0xDC00U)
                        return expected_type{std::unexpect, utf16_error{.valid_up_to = valid_up_to, .error_length = 1}};

                    if (it == sentinel)
                        return expected_type{std::unexpect, utf16_error{.valid_up_to = valid_up_to, .error_length = std::nullopt}};

                    const char16_t second_code_unit = std::bit_cast<char16_t>(*it);
                    ++index, ++it;

                    if (second_code_unit < 0xDC00U || second_code_unit > 0xDFFFU)
                        return expected_type{std::unexpect, utf16_error{.valid_up_to = valid_up_to, .error_length = 1}};

                    std::uint32_t code_point =
                        ((static_cast<std::uint32_t>(first_code_unit & 0x3FFU) << 10) | static_cast<std::uint32_t>(second_code_unit & 0x3FFU)) +
                        0x10'000U;

                    std::invoke(code_point_callback, uchar::from_unchecked(code_point));
                }
                else
                {
                    std::invoke(code_point_callback, uchar::from_unchecked(static_cast<std::uint32_t>(first_code_unit)));
                }
            }

            return expected_type{};
        }

        /// @brief Lossily decodes a range of UTF-16.
        ///
        /// Decodes a range of UTF-16 while replacing invalid subsequences with `uchar::replacement_character()`.
        ///
        /// @param range Range of UTF-16 code units to lossily decode.
        ///
        /// @param code_point_callback Callback function called with every decoded code point, including the replacement characters.
        /// It should have exactly one parameter of type `upp::uchar`.
        ///
        /// @see decode_range, decode_range_unchecked, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        static constexpr void decode_range_lossy(Range&& range, const CodePointCallback& code_point_callback)
        {
            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            // Reuse the previous code unit due to an error that happened.
            bool reuse_previous_code_unit = false;

            char16_t previous_code_unit;

            while (reuse_previous_code_unit || it != sentinel)
            {
                char16_t first_code_unit;

                if (!reuse_previous_code_unit)
                {
                    first_code_unit = std::bit_cast<char16_t>(*it);
                    ++it;
                }
                else
                {
                    first_code_unit          = previous_code_unit;
                    reuse_previous_code_unit = false;
                }

                if (impl::utf16::is_surrogate(first_code_unit))
                {
                    if (first_code_unit >= 0xDC00U)
                    {
                        std::invoke(code_point_callback, uchar::replacement_character());
                        continue;
                    }

                    if (it == sentinel)
                    {
                        std::invoke(code_point_callback, uchar::replacement_character());
                        break;
                    }

                    const char16_t second_code_unit = std::bit_cast<char16_t>(*it);
                    ++it;

                    if (second_code_unit < 0xDC00U || second_code_unit > 0xDFFFU)
                    {
                        std::invoke(code_point_callback, uchar::replacement_character());
                        reuse_previous_code_unit = true;
                        previous_code_unit       = second_code_unit;
                        continue;
                    }

                    std::uint32_t code_point =
                        ((static_cast<std::uint32_t>(first_code_unit & 0x3FFU) << 10) | static_cast<std::uint32_t>(second_code_unit & 0x3FFU)) +
                        0x10'000U;

                    std::invoke(code_point_callback, uchar::from_unchecked(code_point));
                }
                else
                {
                    std::invoke(code_point_callback, uchar::from_unchecked(static_cast<std::uint32_t>(first_code_unit)));
                }
            }
        }

        /// @brief Decodes a range of UTF-16 without error checking.
        ///
        /// @param range Range of UTF-16 code units to decode.
        ///
        /// @param code_point_callback Callback function called with every decoded code point.
        /// It should have exactly one parameter of type `upp::uchar`.
        ///
        /// @see decode_range, decode_range_lossy, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        static constexpr void decode_range_unchecked(Range&& range, const CodePointCallback& code_point_callback)
        {
            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            while (it != sentinel)
            {
                const char16_t first_code_unit = std::bit_cast<char16_t>(*it);
                ++it;

                if (impl::utf16::is_surrogate(first_code_unit))
                {
                    const char16_t second_code_unit = std::bit_cast<char16_t>(*it);
                    ++it;

                    std::uint32_t code_point =
                        ((static_cast<std::uint32_t>(first_code_unit & 0x3FFU) << 10) | static_cast<std::uint32_t>(second_code_unit & 0x3FFU)) +
                        0x10'000;

                    std::invoke(code_point_callback, uchar::from_unchecked(code_point));
                }
                else
                    std::invoke(code_point_callback, uchar::from_unchecked(static_cast<std::uint32_t>(first_code_unit)));
            }
        }
    };

    /// @copydoc encoding_traits
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<>
    struct encoding_traits<encoding::utf32>
    {
        /// The uni-cpp type for storing code points of a given encoding.
        using char_type = uchar;

        /// Built-in integral type for storing code units of a given encoding.
        using default_code_unit_type = char32_t;

        /// Error type returned by UTF-32 validating, decoding and transcoding functions.
        using error_type = utf32_error;

        /// `true` for [variable-width encodings](https://en.wikipedia.org/wiki/Variable-length_encoding), otherwise `false`.
        static constexpr bool is_variable_width = false;

        /// `true` iff type `T` could be used as a code unit type for the UTF-32 encoding.
        template<typename T>
        static constexpr bool is_code_unit_type = std::integral<T> && sizeof(T) == sizeof(default_code_unit_type);

        /// `true` iff type `R` is a range of UTF-32 code units.
        template<typename R>
        static constexpr bool is_code_unit_range = std::ranges::range<R> && is_code_unit_type<std::remove_cvref_t<std::ranges::range_reference_t<R>>>;

        /// @brief Validates a range of UTF-32.
        ///
        /// @param range Range of UTF-32 code units to verify.
        ///
        /// @param code_unit_callback Callback function called with every code unit of the range (unless the range is invalid UTF-32).
        /// It should have exactly one parameter of type `char32_t`.
        /// This makes it possible to use this function with single-pass ranges while also getting the code units out of that range.
        ///
        /// @return `std::expected<void, utf32_error>` containing: `void` if the range is valid UTF-32, otherwise `utf32_error`.
        ///
        /// @see decode_range, decode_range_lossy, decode_range_unchecked
        ///
        template<std::ranges::input_range Range, std::invocable<default_code_unit_type> CodeUnitCallback>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> validate_range(Range&& range, const CodeUnitCallback& code_unit_callback)
        {
            using expected_type = std::expected<void, utf32_error>;

            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            std::size_t index = 0;

            for (; it != sentinel; ++index, ++it)
            {
                const std::uint32_t code_unit = std::bit_cast<std::uint32_t>(*it);

                if (!is_valid_usv(code_unit))
                {
                    return expected_type{std::unexpect, utf32_error{.valid_up_to = index}};
                }

                std::invoke(code_unit_callback, std::bit_cast<char32_t>(code_unit));
            }

            return expected_type{};
        }

        /// @brief Validates a range of UTF-32.
        ///
        /// @param range Range of UTF-32 code units to verify.
        ///
        /// @return `std::expected<void, utf32_error>` containing: `void` if the range is valid UTF-32, otherwise `utf32_error`.
        ///
        /// @see decode_range, decode_range_lossy, decode_range_unchecked
        ///
        template<std::ranges::input_range Range>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> validate_range(Range&& range)
        {
            return validate_range(std::forward<Range>(range), [](char32_t) static {});
        }

        /// @brief Decodes a range of UTF-32 with error checking.
        ///
        /// @param range Range of UTF-32 code units to decode.
        ///
        /// @param code_point_callback Callback function called with every decoded code point.
        /// It should have exactly one parameter of type `upp::uchar`.
        ///
        /// @return `std::expected<void, utf32_error>` containing: `void` if no error occurs, otherwise `utf32_error`.
        ///
        /// @see decode_range_lossy, decode_range_unchecked, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        [[nodiscard]] static constexpr std::expected<void, error_type> decode_range(Range&& range, const CodePointCallback& code_point_callback)
        {
            using expected_type = std::expected<void, utf32_error>;

            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            std::size_t index = 0;

            for (; it != sentinel; ++index, ++it)
            {
                const std::uint32_t code_unit = std::bit_cast<std::uint32_t>(*it);

                const std::optional<uchar> code_point = uchar::from(code_unit);

                if (!code_point.has_value())
                {
                    return expected_type{std::unexpect, utf32_error{.valid_up_to = index}};
                }

                std::invoke(code_point_callback, *code_point);
            }

            return expected_type{};
        }

        /// @brief Lossily decodes a range of UTF-32.
        ///
        /// Decodes a range of UTF-32 while replacing invalid code units with `uchar::replacement_character()`.
        ///
        /// @param range Range of UTF-32 code units to lossily decode.
        ///
        /// @param code_point_callback Callback function called with every decoded code point, including the replacement characters.
        /// It should have exactly one parameter of type `upp::uchar`.
        ///
        /// @see decode_range, decode_range_unchecked, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        static constexpr void decode_range_lossy(Range&& range, const CodePointCallback& code_point_callback)
        {
            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            for (; it != sentinel; ++it)
            {
                const uchar code_point = uchar::from_lossy(std::bit_cast<std::uint32_t>(*it));

                std::invoke(code_point_callback, code_point);
            }
        }

        /// @brief Decodes a range of UTF-32 without error checking.
        ///
        /// @param range Range of UTF-32 code units to decode.
        ///
        /// @param code_point_callback Callback function called with every decoded code point.
        /// It should have exactly one parameter of type `upp::uchar`.
        ///
        /// @see decode_range, decode_range_lossy, validate_range
        ///
        template<std::ranges::input_range Range, std::invocable<char_type> CodePointCallback>
            requires is_code_unit_range<Range>
        static constexpr void decode_range_unchecked(Range&& range, const CodePointCallback& code_point_callback)
        {
            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            for (; it != sentinel; ++it)
            {
                const auto code_point = uchar::from_unchecked(std::bit_cast<std::uint32_t>(*it));

                std::invoke(code_point_callback, code_point);
            }
        }
    };

    /// @brief Identifies types that could be used as a code unit type for a given encoding.
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<typename T, encoding Encoding>
    concept code_unit_type_for = encoding_traits<Encoding>::template is_code_unit_type<T>;

    /// @brief Identifies types that are ranges of code units of a given encoding.
    ///
    /// @headerfile "" <uni-cpp/encoding.hpp>
    ///
    template<typename R, encoding Encoding>
    concept code_unit_range = encoding_traits<Encoding>::template is_code_unit_range<R>;

    namespace impl
    {
        template<std::integral T, unicode_encoding SourceEncoding, unicode_encoding TargetEncoding>
        inline constexpr T utf_transcoding_upper_bound_size_hint_factor = []() {
            // Each line after the "|||" has every case written out. The number in the parentheses is the calculated transcoding factor.
            // For a given source encoding and target encoding pair, the greatest transcoding factor is always chosen.
            //
            // From UTF-8:
            //     1. to UTF-8:  1    |||    1 -> 1 (1)    or    2 -> 2 ( 1 )    or    3 -> 3 ( 1 )    or    4 -> 4 (1)
            //     2. to UTF-16: 1    |||    1 -> 1 (1)    or    2 -> 1 (1/2)    or    3 -> 1 (1/3)    or    4 -> 2 (1/2)
            //     3. to UTF-32: 1    |||    1 -> 1 (1)    or    2 -> 1 (1/2)    or    3 -> 1 (1/3)    or    4 -> 1 (1/4)
            //
            // From UTF-16:
            //     4. to UTF-8:  3    |||    1 -> 1 (1)    or    1 -> 2 ( 2 )    or    1 -> 3 (3)    or    2 -> 4 (2)
            //     5. to UTF-16: 1    |||    1 -> 1 (1)    or    2 -> 2 ( 1 )
            //     6. to UTF-32: 1    |||    1 -> 1 (1)    or    2 -> 1 (1/2)
            //
            // From UTF-32:
            //     7. to UTF-8:  4    |||    1 -> 1 (1)    or    1 -> 2 (2)    or    1 -> 3 (3)    or    1 -> 4 (4)
            //     8. to UTF-16: 2    |||    1 -> 1 (1)    or    1 -> 2 (2)
            //     9. to UTF-32: 1    |||    1 -> 1 (1)

            if constexpr (SourceEncoding == TargetEncoding || TargetEncoding == unicode_encoding::utf32 ||
                          SourceEncoding == unicode_encoding::utf8) // 1, 2, 3, 5, 6, 9
                return 1;
            else if constexpr (SourceEncoding == unicode_encoding::utf16) // 4
                return 3;
            else if constexpr (TargetEncoding == unicode_encoding::utf8) // 7
                return 4;
            else if constexpr (TargetEncoding == unicode_encoding::utf16) // 8
                return 2;
        }();

        template<std::integral T, unicode_encoding SourceEncoding, unicode_encoding TargetEncoding>
        inline constexpr T utf_transcoding_lower_bound_size_hint_divisor = []() {
            // Look at all the cases written above. This time we choose the smallest factor.
            //
            // We write the factor as a fraction and we use the divisor as the result.
            // The dividend is 1 in all cases so there is no need to have a query for it.
            //
            // From UTF-8:
            //     1. to UTF-8:  1 (1/1)
            //     2. to UTF-16: 3 (1/3)
            //     3. to UTF-32: 4 (1/4)
            //
            // From UTF-16:
            //     4. to UTF-8:  1 (1/1)
            //     5. to UTF-16: 1 (1/1)
            //     6. to UTF-32: 2 (1/2)
            //
            // From UTF-32:
            //     7. to UTF-8:  1 (1/1)
            //     8. to UTF-16: 1 (1/1)
            //     9. to UTF-32: 1 (1/1)

            return utf_transcoding_upper_bound_size_hint_factor<T, TargetEncoding, SourceEncoding>;
        }();
    } // namespace impl
} // namespace upp

#endif // UNI_CPP_ENCODING_HPP