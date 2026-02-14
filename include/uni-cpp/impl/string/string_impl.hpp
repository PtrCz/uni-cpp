#ifndef UNI_CPP_IMPL_STRING_STRING_IMPL_HPP
#define UNI_CPP_IMPL_STRING_STRING_IMPL_HPP

/// @file
///
/// @brief Implementations of string types
///

#include "fwd.hpp"
#include "string.hpp"

#include "../ranges.hpp"

#include <cstddef>
#include <cstdint>
#include <bit>
#include <array>
#include <type_traits>

namespace upp
{
    namespace impl
    {
        namespace utf8
        {
            /// @brief Björn Höhrmann`s Deterministic Finite Automaton (DFA) for decoding and validating UTF-8.
            ///
            /// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
            /// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
            ///
            /// ## License
            ///
            /// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
            ///
            /// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
            /// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
            /// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
            ///
            /// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
            ///
            /// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
            /// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
            /// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
            ///
            namespace dfa
            {
                /// @brief Common state values of the UTF-8 decoder.
                ///
                namespace state
                {
                    /// @brief Signals that the decoder just fully decoded a valid code point. The next byte should be starting a new code point.
                    ///
                    inline constexpr std::uint32_t accept = 0;

                    /// @brief Signals that an error occurred while decoding the UTF-8 sequence, i.e., the UTF-8 sequence being decoded is invalid.
                    ///
                    inline constexpr std::uint32_t reject = 12;
                } // namespace state

                /// @brief Maps a given byte of a UTF-8 sequence to it's corresponding character class.
                ///
                inline constexpr std::array<std::uint8_t, 256> character_class_from_byte{
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,  9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                    7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                    2, 2, 2, 2, 2, 2, 2, 2, 2, 10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                };

                /// @brief Transition table that maps a sum of a state of the automaton and a character class to a new state.
                ///
                inline constexpr std::array<std::uint8_t, 108> state_transition_table{
                    0,  12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 0,  12,
                    12, 12, 12, 12, 0,  12, 0,  12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12,
                    12, 24, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 36, 12,
                    36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12
                };
            } // namespace dfa

            inline constexpr std::array<std::uint8_t, 256> char_width_from_leading_byte_table{
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            };

            /// @return `0` on an invalid `leading_byte`, otherwise the byte length of a UTF-8-encoded code point starting with `leading_byte`.
            ///
            [[nodiscard]] constexpr std::uint8_t char_width_from_leading_byte(std::uint8_t leading_byte) noexcept
            {
                return char_width_from_leading_byte_table[static_cast<std::size_t>(leading_byte)];
            }

            /// @brief Checks whether `byte` is a UTF-8 leading byte (a code unit starting a new code point).
            ///
            [[nodiscard]] constexpr bool is_leading_byte(std::uint8_t byte) noexcept
            {
                return std::bit_cast<std::int8_t>(byte) >= -64;
            }

            /// @brief Get the number of bytes to skip in a lossy decoding of UTF-8 when decoding `code_units`.
            ///
            /// The `code_units` are supposed to be the code units of a single invalidly-encoded code point.
            /// If the first byte isn't a valid leading byte then it should be the only byte.
            /// The last byte in `code_units` should be the first byte that triggered a decoding error.
            ///
            /// In general this function would be used like this:
            /// A decoder decodes a UTF-8 sequence, for example using the DFA.
            /// On the first byte to cause the state to be `reject` the decoder would note the index of this byte
            /// and go back to the first byte of the current code point, i.e., the first byte that occurred after the last time
            /// the state was `accept` (or in the case of the current byte being such byte, the decoder would not move).
            /// Then, it would use this function with `code_units` being the code units in between and including the first byte
            /// of the current code point and the first byte to cause the error.
            ///
            /// @pre `code_units.front()` has to be a valid expression. It must be a code unit starting an invalid sequence of UTF-8
            /// (which doesn't mean it has to be invalid itself).
            /// There **must not** be any decodable code points before reaching a decoding error while decoding `code_units`.
            ///
            /// @pre `code_units` have to be invalid UTF-8. Moreover, `code_units` have to invalid in such a way
            /// that the decoder is able to tell they're invalid before running out of the `code_units` to consume,
            /// i.e., the error cannot be caused by unexpectedly reaching the end of the input.
            ///
            /// @warning This function heavily relies on it's preconditions. Make sure they are properly followed.
            ///
            [[nodiscard]] constexpr std::uint8_t get_error_length(std::span<std::uint8_t> code_units) noexcept
            {
                const auto leading_byte = code_units.front();
                const auto width        = char_width_from_leading_byte(leading_byte);

                // Note: many of the branches are omitted because of the preconditions that `code_units` have to be invalid in a specific way.

                switch (width)
                {
                // case 1 should never trigger under the specified preconditions
                // case 2 is handled by the default case already
                case 3: {
                    const auto second_byte = code_units[1uz];

                    switch (leading_byte)
                    {
                    case 0xE0: {
                        return (second_byte >= 0xA0U && second_byte <= 0xBFU) ? 2 : 1;
                    }
                    case 0xE1: [[fallthrough]];
                    case 0xE2: [[fallthrough]];
                    case 0xE3: [[fallthrough]];
                    case 0xE4: [[fallthrough]];
                    case 0xE5: [[fallthrough]];
                    case 0xE6: [[fallthrough]];
                    case 0xE7: [[fallthrough]];
                    case 0xE8: [[fallthrough]];
                    case 0xE9: [[fallthrough]];
                    case 0xEA: [[fallthrough]];
                    case 0xEB: [[fallthrough]];
                    case 0xEC: {
                        return (second_byte >= 0x80U && second_byte <= 0xBFU) ? 2 : 1;
                    }
                    case 0xED: {
                        return (second_byte >= 0x80U && second_byte <= 0x9FU) ? 2 : 1;
                    }
                    case 0xEE: [[fallthrough]];
                    case 0xEF: {
                        return (second_byte >= 0x80U && second_byte <= 0xBFU) ? 2 : 1;
                    }
                    default: {
                        return 1;
                    }
                    }
                }
                case 4: {
                    const auto second_byte = code_units[1uz];

                    switch (leading_byte)
                    {
                    case 0xF0: {
                        if (second_byte < 0x90U || second_byte > 0xBFU)
                            return 1;
                        else
                            break;
                    }
                    case 0xF1: [[fallthrough]];
                    case 0xF2: [[fallthrough]];
                    case 0xF3: {
                        if (second_byte < 0x80U || second_byte > 0xBFU)
                            return 1;
                        else
                            break;
                    }
                    case 0xF4: {
                        if (second_byte < 0x80U || second_byte > 0x8FU)
                            return 1;
                        else
                            break;
                    }
                    default: {
                        return 1;
                    }
                    }

                    if (is_leading_byte(code_units[2uz]))
                        return 2;

                    return 3;
                }
                default: {
                    // Handles invalid leading bytes and invalid 2-byte-long sequences.

                    // Note: invalid 2-byte-long sequences are always caused by
                    // either the leading byte being invalid (overlong error being detected early),
                    // or the second byte not being a continuation byte.
                    // In both of these cases the number of bytes to skip would be 1.

                    return 1;
                }
                }
            }
        } // namespace utf8

        /// @brief `std::monostate` without having to include `std::variant`.
        ///
        struct monostate
        {
        };

        class basic_ustring_impl
        {
        public:
            template<unicode_encoding Encoding, typename Container, typename Range>
            [[nodiscard]] static constexpr std::expected<basic_ustring<Encoding, Container>, utf8_error> from_utf8(Range&& range)
            {
                using result_type   = basic_ustring<Encoding, Container>;
                using error_type    = utf8_error;
                using expected_type = std::expected<result_type, error_type>;

                // If `Range` is the underlying `container_type`, then instead of inserting code units one by one in the loop,
                // we push them all at once at the end. This allows us to move from an rvalue reference to the
                // underlying container.
                // Note: the underlying container type is guaranteed to be `std::ranges::contiguous_range`,
                // which means it cannot be a single pass range.
                static constexpr bool is_range_the_underlying_container_type = std::same_as<Container, std::remove_cvref_t<Range>>;

                // We only use the `result` variable if `is_range_the_underlying_container_type` is `false`.
                std::conditional_t<!is_range_the_underlying_container_type, result_type, monostate> result;

                // Don't reserve if Range is the underlying container type as this makes the potential move later pointless.
                if constexpr (!is_range_the_underlying_container_type && ranges::approximately_sized_range<Range> && reservable_container<Container>)
                {
                    result.template reserve_for_transcoding_from<unicode_encoding::utf8>(ranges::reserve_hint(range));
                }

                auto       it       = std::ranges::begin(range);
                const auto sentinel = std::ranges::end(range);

                std::uint32_t state = utf8::dfa::state::accept;
                std::uint32_t current_code_point;

                std::size_t valid_up_to = 0;

                // If Range isn't bidirectional we must cache at least 3 previous code units to calculate `error_length`.
                std::array<std::uint8_t, 3> previous_code_units{0, 0, 0};

                for (std::size_t index = 0; it != sentinel; ++index, ++it)
                {
                    const std::uint8_t current_code_unit = std::bit_cast<std::uint8_t>(*it);

                    std::uint32_t type = utf8::dfa::character_class_from_byte[current_code_unit];

                    if constexpr (Encoding != unicode_encoding::utf8)
                    {
                        // If the result encoding isn't UTF-8 we need to transcode
                        current_code_point = (state != utf8::dfa::state::accept) ? (current_code_unit & 0x3FU) | (current_code_point << 6)
                                                                                 : (0xFF >> type) & (current_code_unit);
                    }

                    state = utf8::dfa::state_transition_table[state + type];

                    if (state == utf8::dfa::state::reject)
                    {
                        std::array<std::uint8_t, 4> invalid_code_units;

                        const std::size_t invalid_code_units_last_index = index - valid_up_to;
                        const std::size_t invalid_code_units_length     = invalid_code_units_last_index + 1;

                        invalid_code_units[invalid_code_units_last_index] = current_code_unit;

                        if constexpr (std::ranges::bidirectional_range<Range>)
                        {
                            // Range is bidirectional - go back to the beginning of the current code point.

                            for (std::size_t index = invalid_code_units_last_index; (index--) > 0;)
                            {
                                --it;
                                invalid_code_units[index] = std::bit_cast<std::uint8_t>(*it);
                            }
                        }
                        else
                        {
                            // Range isn't bidirectional - use the cached previous code units.

                            for (std::size_t iteration = 0, index = invalid_code_units_last_index; (index--) > 0; ++iteration)
                            {
                                invalid_code_units[index] = previous_code_units[iteration];
                            }
                        }

                        const std::uint8_t error_length =
                            utf8::get_error_length(std::span<std::uint8_t>{invalid_code_units.data(), invalid_code_units_length});

                        return expected_type{std::unexpect, utf8_error{.valid_up_to = valid_up_to, .error_length = error_length}};
                    }

                    if (state == utf8::dfa::state::accept)
                    {
                        valid_up_to = index + 1;

                        if constexpr (Encoding != unicode_encoding::utf8)
                        {
                            // If we're transcoding, we do it here and append the resulting code units here.
                            // This is because we need the state to be `accept` to make sure we got the whole code point.
                            // If we're not transcoding, we can append a code unit in each loop iteration regardless of the state
                            // as long as it's not `reject` (which we do below).

                            result.push_back(uchar::from_unchecked(current_code_point));
                        }
                    }

                    if constexpr (Encoding == unicode_encoding::utf8 && !is_range_the_underlying_container_type)
                    {
                        // Since `Encoding == unicode_encoding::utf8`, we're not transcoding.
                        // Since `Range` isn't the underlying `container_type`, insert the code unit here;
                        // otherwise do it at the end, after the loop.

                        result.push_back_code_unit(current_code_unit);
                    }

                    if constexpr (!std::ranges::bidirectional_range<Range>)
                    {
                        // If Range isn't bidirectional we must cache at least 3 previous code units to calculate `error_length`.

                        previous_code_units[2] = previous_code_units[1];
                        previous_code_units[1] = previous_code_units[0];
                        previous_code_units[0] = current_code_unit;
                    }
                }

                // Check if the range ended in the middle of a code point.

                if (state != utf8::dfa::state::accept)
                {
                    return expected_type{std::unexpect, utf8_error{.valid_up_to = valid_up_to, .error_length = {std::nullopt}}};
                }

                if constexpr (is_range_the_underlying_container_type)
                    return expected_type{std::in_place, result_type{from_container, std::forward<Range>(range)}};
                else
                {
                    if constexpr (requires(Container& c) { c.shrink_to_fit(); })
                    {
                        result.shrink_to_fit();
                    }

                    return expected_type{std::in_place, std::move(result)};
                }
            }

            template<unicode_encoding Encoding, typename Container, typename Range>
                requires code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, encoding::utf8>
            [[nodiscard]] static constexpr basic_ustring<Encoding, Container> transcode_from_utf8_unchecked(Range&& range)
            {
                using result_type = basic_ustring<Encoding, Container>;

                result_type result;

                if constexpr (ranges::approximately_sized_range<Range> && reservable_container<Container>)
                {
                    result.template reserve_for_transcoding_from<unicode_encoding::utf8>(ranges::reserve_hint(range));
                }

                auto       it       = std::ranges::begin(range);
                const auto sentinel = std::ranges::end(range);

                std::uint32_t state = utf8::dfa::state::accept;
                std::uint32_t current_code_point;

                for (; it != sentinel; ++it)
                {
                    const std::uint8_t current_code_unit = std::bit_cast<std::uint8_t>(*it);

                    std::uint32_t type = utf8::dfa::character_class_from_byte[current_code_unit];

                    current_code_point = (state != utf8::dfa::state::accept) ? (current_code_unit & 0x3FU) | (current_code_point << 6)
                                                                             : (0xFF >> type) & (current_code_unit);

                    state = utf8::dfa::state_transition_table[state + type];

                    if (state == utf8::dfa::state::accept)
                    {
                        result.push_back(uchar::from_unchecked(current_code_point));
                    }
                }

                if constexpr (requires(Container& c) { c.shrink_to_fit(); })
                {
                    result.shrink_to_fit();
                }

                return result;
            }

            template<unicode_encoding Encoding, typename Container, typename Range>
                requires code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, static_cast<encoding>(Encoding)> &&
                         (!std::same_as<Container, std::remove_cvref_t<Range>>) // there is another overload for this case below
            [[nodiscard]] static constexpr basic_ustring<Encoding, Container> utfx_from_utfx_unchecked(Range&& range)
            {
                using result_type = basic_ustring<Encoding, Container>;
                using size_type   = result_type::size_type;

                result_type result;

                if constexpr (ranges::approximately_sized_range<Range> && reservable_container<Container>)
                {
                    result.reserve(static_cast<size_type>(ranges::reserve_hint(range)));
                }

                auto       it       = std::ranges::begin(range);
                const auto sentinel = std::ranges::end(range);

                for (; it != sentinel; ++it)
                    result.push_back_code_unit(*it);

                return result;
            }

            // If Range is the underlying container type, we just construct the string from the container.
            template<unicode_encoding Encoding, typename Container, typename Range>
                requires std::same_as<Container, std::remove_cvref_t<Range>>
            [[nodiscard]] static constexpr basic_ustring<Encoding, Container> utfx_from_utfx_unchecked(Range&& container)
            {
                return {from_container, std::forward<Range>(container)};
            }
        };
    } // namespace impl

    // Doxygen doesn't handle well static method definitions outside of class definitions
    /// @cond

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, encoding::utf8>
    [[nodiscard]] constexpr std::expected<basic_ustring<E, C>, utf8_error> basic_ustring<E, C>::from_utf8(Range&& range)
    {
        return impl::basic_ustring_impl::from_utf8<E, container_type>(std::forward<Range>(range));
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, encoding::utf8>
    [[nodiscard]] constexpr basic_ustring<E, C> basic_ustring<E, C>::from_utf8_unchecked(Range&& range)
    {
        if constexpr (E == unicode_encoding::utf8)
        {
            return impl::basic_ustring_impl::utfx_from_utfx_unchecked<E, container_type>(std::forward<Range>(range));
        }
        else
        {
            return impl::basic_ustring_impl::transcode_from_utf8_unchecked<E, container_type>(std::forward<Range>(range));
        }
    }

    /// @endcond
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_STRING_IMPL_HPP