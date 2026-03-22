#ifndef UNI_CPP_IMPL_ENCODING_UTF8_HPP
#define UNI_CPP_IMPL_ENCODING_UTF8_HPP

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
    enum class utf8_error_code : std::uint8_t
    {
        truncated_sequence,
        unexpected_continuation_byte,
        overlong,
        encoded_surrogate,
        out_of_range,
        invalid_leading_byte,
    };

    struct utf8_error
    {
        std::optional<std::uint8_t> length;
        utf8_error_code             code;

        [[nodiscard]] constexpr bool operator==(const utf8_error&) const noexcept = default;
    };

    struct from_utf8_error
    {
        std::size_t valid_up_to;
        utf8_error  error;

        [[nodiscard]] constexpr bool operator==(const from_utf8_error&) const noexcept = default;
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
            /// @code
            /// 0x00 - 0x7F              --> 0     --- ASCII
            /// 0x80 - 0x8F              --> 1     --- CONT_A
            /// 0xC2 - 0xDF              --> 2     --- LEAD_2
            /// 0xE1 - 0xEC, 0xEE - 0xEF --> 3     --- LEAD_3
            /// 0xED                     --> 4     --- LEAD_3_ED
            /// 0xF4                     --> 5     --- LEAD_4_MAX
            /// 0xF1 - 0xF3              --> 6     --- LEAD_4
            /// 0xA0 - 0xBF              --> 7     --- CONT_B
            /// 0xC0 - 0xC1, 0xF5 - 0xFF --> 8     --- INVALID
            /// 0x90 - 0x9F              --> 9     --- CONT_C
            /// 0xE0                     --> 10    --- LEAD_3_E0
            /// 0xF0                     --> 11    --- LEAD_4_F0
            /// @endcode
            ///
            inline constexpr std::array<std::uint8_t, 256> character_class_from_byte{
                // clang-format off
                0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // [0x00, 0x1F]
                0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // [0x20, 0x3F]
                0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // [0x40, 0x5F]
                0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // [0x60, 0x7F]
                1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9,  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // [0x80, 0x9F]
                7,  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // [0xA0, 0xBF]
                8,  8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // [0xC0, 0xDF]
                10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, // [0xE0, 0xFF]
                // clang-format on
            };

            /// @brief Transition table that maps a sum of a state of the automaton and a character class to a new state.
            ///
            /// @code
            /// 0  --- ACCEPT
            /// 12 --- REJECT
            /// 24 --- EXPECT1
            /// 36 --- EXPECT2
            /// 48 --- EXPECT_E0
            /// 60 --- EXPECT_ED
            /// 72 --- EXPECT_F0
            /// 84 --- EXPECT3
            /// 96 --- EXPECT_F4
            /// @endcode
            ///
            inline constexpr std::array<std::uint8_t, 108> state_transition_table{
                // clang-format off
                0,  12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, // ACCEPT       + CHARACTER CLASS
                12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, // REJECT       + CHARACTER CLASS
                12, 0,  12, 12, 12, 12, 12, 0,  12, 0,  12, 12, // EXPECT1      + CHARACTER CLASS
                12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, // EXPECT2      + CHARACTER CLASS
                12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, // EXPECT_E0    + CHARACTER CLASS
                12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, // EXPECT_ED    + CHARACTER CLASS
                12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, // EXPECT_F0    + CHARACTER CLASS
                12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, // EXPECT3      + CHARACTER CLASS
                12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, // EXPECT_F4    + CHARACTER CLASS
                // clang-format on
            };
        } // namespace dfa

        inline constexpr std::array<std::uint8_t, 256> char_width_from_leading_byte_table{
            // clang-format off
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // [0x00, 0x1F]
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // [0x20, 0x3F]
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // [0x40, 0x5F]
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // [0x60, 0x7F]
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // [0x80, 0x9F]
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // [0xA0, 0xBF]
            0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // [0xC0, 0xDF]
            3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // [0xE0, 0xFF]
            // clang-format on
        };

        /// @return `0` on an invalid `leading_byte`, otherwise the byte length of a UTF-8-encoded code point starting with `leading_byte`.
        ///
        [[nodiscard]] constexpr std::uint8_t char_width_from_leading_byte(char8_t leading_byte) noexcept
        {
            return char_width_from_leading_byte_table[static_cast<std::size_t>(leading_byte)];
        }

        /// @brief Checks whether `byte` is valid when it appears at the beginning of a UTF-8 sequence.
        ///
        [[nodiscard]] constexpr bool is_valid_first_byte(char8_t byte) noexcept
        {
            return char_width_from_leading_byte(byte) != 0;
        }

        /// @brief Checks whether `byte` is a UTF-8 continuation byte.
        ///
        [[nodiscard]] constexpr bool is_continuation_byte(char8_t byte) noexcept
        {
            return std::bit_cast<std::int8_t>(byte) < -64;
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

        namespace impl::error_code
        {
            // Filler values for the table below.
            // Given that `get_error_code` is always called with invalid sequences, these will never get returned.

            inline constexpr utf8_error_code not_an_error = static_cast<utf8_error_code>(0xFF);
            inline constexpr utf8_error_code sticky_error = static_cast<utf8_error_code>(0xFE);

            /// @brief Table mapping `STATE + CHARACTER_CLASS` indexes to a UTF-8 error code.
            ///
            inline constexpr std::array<utf8_error_code, 108> error_code_table{
                // clang-format off

                // ACCEPT       + CHARACTER CLASS
                not_an_error,                          utf8_error_code::unexpected_continuation_byte, not_an_error,                        not_an_error,
                not_an_error,                          not_an_error,                                  not_an_error,                        utf8_error_code::unexpected_continuation_byte,
                utf8_error_code::invalid_leading_byte, utf8_error_code::unexpected_continuation_byte, not_an_error,                        not_an_error,

                // REJECT       + CHARACTER CLASS
                sticky_error,                          sticky_error,                                  sticky_error,                        sticky_error,
                sticky_error,                          sticky_error,                                  sticky_error,                        sticky_error,
                sticky_error,                          sticky_error,                                  sticky_error,                        sticky_error,

                // EXPECT1      + CHARACTER CLASS
                utf8_error_code::truncated_sequence,   not_an_error,                                  utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,
                utf8_error_code::truncated_sequence,   utf8_error_code::truncated_sequence,           utf8_error_code::truncated_sequence, not_an_error,
                utf8_error_code::truncated_sequence,   not_an_error,                                  utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,

                // EXPECT2      + CHARACTER CLASS
                utf8_error_code::truncated_sequence,   not_an_error,                                  utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,
                utf8_error_code::truncated_sequence,   utf8_error_code::truncated_sequence,           utf8_error_code::truncated_sequence, not_an_error,
                utf8_error_code::truncated_sequence,   not_an_error,                                  utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,

                // EXPECT_E0    + CHARACTER CLASS
                utf8_error_code::truncated_sequence,   utf8_error_code::overlong,                     utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,
                utf8_error_code::truncated_sequence,   utf8_error_code::truncated_sequence,           utf8_error_code::truncated_sequence, not_an_error,
                utf8_error_code::truncated_sequence,   utf8_error_code::overlong,                     utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,

                // EXPECT_ED    + CHARACTER CLASS
                utf8_error_code::truncated_sequence,   not_an_error,                                  utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,
                utf8_error_code::truncated_sequence,   utf8_error_code::truncated_sequence,           utf8_error_code::truncated_sequence, utf8_error_code::encoded_surrogate,
                utf8_error_code::truncated_sequence,   not_an_error,                                  utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,

                // EXPECT_F0    + CHARACTER CLASS
                utf8_error_code::truncated_sequence,   utf8_error_code::overlong,                     utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,
                utf8_error_code::truncated_sequence,   utf8_error_code::truncated_sequence,           utf8_error_code::truncated_sequence, not_an_error,
                utf8_error_code::truncated_sequence,   not_an_error,                                  utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,

                // EXPECT3      + CHARACTER CLASS
                utf8_error_code::truncated_sequence,   not_an_error,                                  utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,
                utf8_error_code::truncated_sequence,   utf8_error_code::truncated_sequence,           utf8_error_code::truncated_sequence, not_an_error,
                utf8_error_code::truncated_sequence,   not_an_error,                                  utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,

                // EXPECT_F4    + CHARACTER CLASS
                utf8_error_code::truncated_sequence,   not_an_error,                                  utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,
                utf8_error_code::truncated_sequence,   utf8_error_code::truncated_sequence,           utf8_error_code::truncated_sequence, utf8_error_code::out_of_range,
                utf8_error_code::truncated_sequence,   utf8_error_code::out_of_range,                 utf8_error_code::truncated_sequence, utf8_error_code::truncated_sequence,

                // clang-format on
            };
        } // namespace impl::error_code

        /// @brief Returns a UTF-8 error code for a last non-reject state and a character class that caused the state to be reject.
        ///
        /// `dfa::state_transition_table[last_non_reject_state + character_class]` should result in `dfa::state::reject`,
        /// but `last_non_reject_state` shouldn't be `dfa::state::reject` itself.
        ///
        [[nodiscard]] constexpr utf8_error_code get_error_code(std::uint32_t last_non_reject_state, std::uint32_t character_class) noexcept
        {
            return impl::error_code::error_code_table[last_non_reject_state + character_class];
        }
    } // namespace impl::utf8
} // namespace upp

#endif // UNI_CPP_IMPL_ENCODING_UTF8_HPP