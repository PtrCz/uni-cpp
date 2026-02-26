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
#include <optional>

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

        /// @brief Get the number of bytes to skip in a lossy decoding of UTF-8.
        ///
        /// @param invalid_code_units_length Length of the invalid code unit sequence of a single code point that failed to decode.
        ///
        /// The code units mentioned in this function are supposed to be only of a single code point that was found to be invalid.
        /// The first code unit should be a leading byte (a byte starting a new code point), and the last byte of the mentioned code unit sequence
        /// should be the first byte to cause a decoding error.
        ///
        /// That is, the length is supposed to be the number of code units the decoder has processed since the last time it has fully decoded a valid code point.
        /// The length should include the byte that caused the error.
        ///
        /// It's also important that the last byte of the invalid code unit sequence must be the FIRST byte that caused a decoding error.
        ///
        [[nodiscard]] constexpr std::uint8_t get_error_length_from_invalid_code_units_length(std::uint8_t invalid_code_units_length) noexcept
        {
            // Every error in UTF-8 other than 'Too Short' can be detected either in the first or the second byte of a code point.
            // If `invalid_code_units_length` is less than 3, the error was detected in either the first or the second byte (which includes all errors other than 'Too Short').
            // For all errors other than 'Too Short', the error length is 1.
            // For all 'Too Short' errors caused by all continuation bytes missing, the error length is 1 too, and all such sequences would have a length of 2.

            // For the left 'Too Short' errors, the error length can always be calculated by subtracting the byte that caused the error from the sequence length.
            // This works, because the result is the length of the sequence that unexpectedly ended, which is the error length for the 'Too Short' errors.

            if (invalid_code_units_length < 3)
                return 1;
            else
                return invalid_code_units_length - 1;
        }
    } // namespace impl::utf8
} // namespace upp

#endif // UNI_CPP_IMPL_UTF8_HPP