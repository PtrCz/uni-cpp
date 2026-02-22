#ifndef UNI_CPP_IMPL_UTF8_HPP
#define UNI_CPP_IMPL_UTF8_HPP

/// @file
///
/// @brief UTF-8 validating and decoding helper functions.
///

#include <cstddef>
#include <cstdint>
#include <array>
#include <bit>
#include <span>

namespace upp
{
    struct utf8_error
    {
        std::size_t                 valid_up_to;
        std::optional<std::uint8_t> error_length;

        [[nodiscard]] constexpr bool operator==(const utf8_error&) const noexcept = default;
    };

    namespace impl::utf8
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
        [[nodiscard]] constexpr std::uint8_t char_width_from_leading_byte(char8_t leading_byte) noexcept
        {
            return char_width_from_leading_byte_table[static_cast<std::size_t>(leading_byte)];
        }

        /// @brief Checks whether `byte` is a UTF-8 leading byte (a code unit starting a new code point).
        ///
        [[nodiscard]] constexpr bool is_leading_byte(char8_t byte) noexcept
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
        [[nodiscard]] constexpr std::uint8_t get_error_length(std::span<char8_t> code_units) noexcept
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

        class previous_code_units_buffer
        {
        public:
            [[nodiscard]] constexpr std::array<char8_t, 4> get_n(std::size_t n) const noexcept
            {
                std::array<char8_t, 4> result;

                if consteval
                {
                    // In constant evaluation the array can't be partially uninitialized.
                    // At runtime it's fine, because the uninitialized part is never read.
                    result.fill(0);
                }

                for (std::size_t iteration = 0, index = n; (index--) > 0; ++iteration)
                {
                    result[index] = m_buffer[iteration];
                }

                return result;
            }

            constexpr void push(char8_t code_unit) noexcept
            {
                m_buffer[2] = m_buffer[1];
                m_buffer[1] = m_buffer[0];
                m_buffer[0] = code_unit;
            }

        private:
            std::array<char8_t, 3> m_buffer{0, 0, 0};
        };
    } // namespace impl::utf8
} // namespace upp

#endif // UNI_CPP_IMPL_UTF8_HPP