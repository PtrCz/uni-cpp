#ifndef TEST_UTILITY_HPP
#define TEST_UTILITY_HPP

#include <uni-cpp/encoding.hpp>

#include <cstdint>
#include <string_view>
#include <stdexcept>
#include <bit>

namespace upp_test
{
    template<typename Callable>
    constexpr void run_for_each_unicode_encoding(const Callable& callable)
    {
        callable.template operator()<upp::encoding::utf8>();
        callable.template operator()<upp::encoding::utf16>();
        callable.template operator()<upp::encoding::utf32>();
    }

    template<typename Callable>
    constexpr void run_for_each_encoding(const Callable& callable)
    {
        callable.template operator()<upp::encoding::ascii>();
        run_for_each_unicode_encoding(callable);
    }

    namespace impl
    {
        template<upp::encoding Encoding>
        constexpr std::basic_string_view<typename upp::encoding_traits<Encoding>::default_code_unit_type> string_literal_impl(
            std::string_view, std::u8string_view, std::u16string_view, std::u32string_view) noexcept;

        template<>
        constexpr std::string_view string_literal_impl<upp::encoding::ascii>(std::string_view ascii_literal, std::u8string_view, std::u16string_view,
                                                                             std::u32string_view) noexcept
        {
            return ascii_literal;
        }
        template<>
        constexpr std::u8string_view string_literal_impl<upp::encoding::utf8>(std::string_view, std::u8string_view utf8_literal, std::u16string_view,
                                                                              std::u32string_view) noexcept
        {
            return utf8_literal;
        }
        template<>
        constexpr std::u16string_view string_literal_impl<upp::encoding::utf16>(std::string_view, std::u8string_view,
                                                                                std::u16string_view utf16_literal, std::u32string_view) noexcept
        {
            return utf16_literal;
        }
        template<>
        constexpr std::u32string_view string_literal_impl<upp::encoding::utf32>(std::string_view, std::u8string_view, std::u16string_view,
                                                                                std::u32string_view utf32_literal) noexcept
        {
            return utf32_literal;
        }
    } // namespace impl

    inline namespace literals
    {
        inline namespace int_literals
        {
            [[nodiscard]] constexpr char operator""_ch(const unsigned long long int value)
            {
                if (value > 0xFFull)
                {
                    throw std::invalid_argument("Invalid 8-bit value");
                }

                return std::bit_cast<char>(static_cast<std::uint8_t>(value));
            }

            [[nodiscard]] constexpr std::uint8_t operator""_u8(const unsigned long long int value)
            {
                if (value > 0xFFull)
                {
                    throw std::invalid_argument("Invalid unsigned 8-bit value");
                }

                return static_cast<std::uint8_t>(value);
            }

            [[nodiscard]] constexpr std::uint32_t operator""_u32(const unsigned long long int value)
            {
                if (value > 0xFF'FF'FF'FFull)
                {
                    throw std::invalid_argument("Invalid unsigned 32-bit value");
                }

                return static_cast<std::uint32_t>(value);
            }
        } // namespace int_literals

        /// @brief Workaround for MSVC's broken consteval functions.
        ///
        inline namespace char_literals
        {
            [[nodiscard]] constexpr upp::ascii_char operator""_ac(const unsigned long long int value)
            {
                if (value > 0xFFull || !upp::is_valid_ascii(static_cast<std::uint8_t>(value)))
                {
                    throw std::invalid_argument("Invalid ASCII value");
                }

                return upp::ascii_char::from_unchecked(static_cast<std::uint8_t>(value));
            }

            [[nodiscard]] constexpr upp::ascii_char operator""_ac(const char8_t value)
            {
                if (!upp::is_valid_ascii(static_cast<std::uint8_t>(value)))
                {
                    throw std::invalid_argument("Invalid ASCII value");
                }

                return upp::ascii_char::from_unchecked(static_cast<std::uint8_t>(value));
            }

            [[nodiscard]] constexpr upp::uchar operator""_uc(const unsigned long long int value)
            {
                if (value > 0xFF'FF'FF'FFull || !upp::is_valid_usv(static_cast<std::uint32_t>(value)))
                {
                    throw std::invalid_argument("Invalid Unicode scalar value");
                }

                return upp::uchar::from_unchecked(static_cast<std::uint32_t>(value));
            }

            [[nodiscard]] constexpr upp::uchar operator""_uc(const char32_t value)
            {
                if (!upp::is_valid_usv(static_cast<std::uint32_t>(value)))
                {
                    throw std::invalid_argument("Invalid Unicode scalar value");
                }

                return upp::uchar::from_unchecked(static_cast<std::uint32_t>(value));
            }
        } // namespace char_literals

        /// @brief Workaround for clang's broken UD string literals.
        ///
        inline namespace string_literals
        {
            [[nodiscard]] constexpr std::string operator""_s(const char* data, std::size_t size)
            {
                std::string str;
                str.assign(data, size);
                return str;
            }

            [[nodiscard]] constexpr std::u8string operator""_s(const char8_t* data, std::size_t size)
            {
                std::u8string str;
                str.assign(data, size);
                return str;
            }

            [[nodiscard]] constexpr std::u16string operator""_s(const char16_t* data, std::size_t size)
            {
                std::u16string str;
                str.assign(data, size);
                return str;
            }

            [[nodiscard]] constexpr std::u32string operator""_s(const char32_t* data, std::size_t size)
            {
                std::u32string str;
                str.assign(data, size);
                return str;
            }
        } // namespace string_literals
    } // namespace literals
} // namespace upp_test

/// Returns the string_view `literal` encoded in `encoding`.
///
#define TEST_STRING_LITERAL(encoding, literal) (::upp_test::impl::string_literal_impl<encoding>(literal, u8##literal, u##literal, U##literal))

#endif // TEST_UTILITY_HPP