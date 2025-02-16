#pragma once
#ifndef UNI_CPP_CHAR_HPP
#define UNI_CPP_CHAR_HPP

#include "config.hpp"

#include <cstdint>
#include <optional>
#include <type_traits>
#include <compare>
#include <limits>
#include <bit>

namespace upp
{
	// Check if 'value' is in ASCII range (0 to 0x7F, inclusive)
	[[nodiscard]] constexpr bool is_valid_ascii(const std::uint8_t value) noexcept
	{
		return value < 0x80;
	}

	// Check if 'value' is a valid Unicode scalar value:
	// "The set of Unicode scalar values consists of the
	// ranges 0 to 0xD7FF and 0xE000 to 0x10FFFF, inclusive."
	[[nodiscard]] constexpr bool is_valid_usv(const std::uint32_t value) noexcept
	{
		// read: https://doc.rust-lang.org/src/core/char/convert.rs.html#233
		return (value ^ 0xD800U) - 0x800U < 0x10F800U;
	}

	class ascii_char;
	class uchar;

	class ascii_char
	{
	public:
		constexpr ascii_char() noexcept
			: m_value(0)
		{
		}

		constexpr ascii_char(const ascii_char&) noexcept = default;

		constexpr ascii_char& operator=(const ascii_char&) noexcept = default;

		[[nodiscard]] static constexpr ascii_char substitute_character() noexcept
		{
			return ascii_char(std::uint8_t{0x1A});
		}

		[[nodiscard]] static constexpr std::optional<ascii_char> from(const std::uint8_t value) noexcept
		{
			if (is_valid_ascii(value))
				return std::optional<ascii_char>(ascii_char(value));

			return std::optional<ascii_char>();
		}
		[[nodiscard]] static constexpr std::optional<ascii_char> from(const char value) noexcept
		{
			return ascii_char::from(std::bit_cast<std::uint8_t>(value));
		}
		[[nodiscard]] static constexpr std::optional<ascii_char> from(const char8_t value) noexcept
		{
			return ascii_char::from(static_cast<std::uint8_t>(value));
		}

		[[nodiscard]] static constexpr ascii_char from_lossy(const std::uint8_t value) noexcept
		{
			if (is_valid_ascii(value))
				return ascii_char(value);

			return ascii_char::substitute_character();
		}
		[[nodiscard]] static constexpr ascii_char from_lossy(const char value) noexcept
		{
			return ascii_char::from_lossy(std::bit_cast<std::uint8_t>(value));
		}
		[[nodiscard]] static constexpr ascii_char from_lossy(const char8_t value) noexcept
		{
			return ascii_char::from_lossy(static_cast<std::uint8_t>(value));
		}

		[[nodiscard]] constexpr bool operator==(const ascii_char rhs) const noexcept
		{
			return m_value == rhs.m_value;
		}
		[[nodiscard]] constexpr std::strong_ordering operator<=>(const ascii_char rhs) const noexcept
		{
			return m_value <=> rhs.m_value;
		}

		[[nodiscard]] constexpr std::uint8_t value() const noexcept
		{
			return m_value;
		}

	private:
		friend uchar;

		constexpr ascii_char(const std::uint8_t value) noexcept
			: m_value(value)
		{
		}

	private:
		std::uint8_t m_value;
	};

	using achar = ascii_char;

	class uchar
	{
	public:
		constexpr uchar() noexcept
			: m_value(0)
		{
		}
		explicit constexpr uchar(const ascii_char ch) noexcept
			: m_value(static_cast<std::uint32_t>(ch.value()))
		{
		}

		constexpr uchar(const uchar&) noexcept = default;

		constexpr uchar& operator=(const uchar&) noexcept = default;

		[[nodiscard]] constexpr bool operator==(const uchar rhs) noexcept
		{
			return m_value == rhs.m_value;
		}
		[[nodiscard]] constexpr std::strong_ordering operator<=>(const uchar rhs) noexcept
		{
			return m_value <=> rhs.m_value;
		}

		[[nodiscard]] static constexpr uchar replacement_character() noexcept
		{
			return uchar(std::uint32_t{0xFFFD});
		}

		[[nodiscard]] static constexpr std::optional<uchar> from(const std::uint32_t value) noexcept
		{
			if (is_valid_usv(value))
				return std::optional<uchar>(uchar(value));

			return std::optional<uchar>();
		}
		[[nodiscard]] static constexpr std::optional<uchar> from(const char32_t value) noexcept
		{
			return uchar::from(static_cast<std::uint32_t>(value));
		}

		[[nodiscard]] static constexpr uchar from_lossy(const std::uint32_t value) noexcept
		{
			if (is_valid_usv(value))
				return uchar(value);

			return uchar::replacement_character();
		}
		[[nodiscard]] static constexpr uchar from_lossy(const char32_t value) noexcept
		{
			return uchar::from_lossy(static_cast<std::uint32_t>(value));
		}

		[[nodiscard]] constexpr std::uint32_t value() const noexcept
		{
			return m_value;
		}

		[[nodiscard]] constexpr bool is_ascii() const noexcept
		{
			return m_value < 0x80;
		}

		[[nodiscard]] constexpr std::optional<ascii_char> as_ascii() const noexcept
		{
			if (is_ascii())
				return std::optional<ascii_char>(ascii_char(static_cast<std::uint8_t>(m_value)));

			return std::optional<ascii_char>();
		}
		[[nodiscard]] constexpr ascii_char as_ascii_lossy() const noexcept
		{
			if (is_ascii())
				return ascii_char(static_cast<std::uint8_t>(m_value));

			return ascii_char::substitute_character();
		}

		[[nodiscard]] constexpr std::size_t length_utf8() const noexcept
		{
			if (m_value < 0x80)
				return 1;
			else if (m_value < 0x800)
				return 2;
			else if (m_value < 0x10'000)
				return 3;
			else
				return 4;
		}
		[[nodiscard]] constexpr std::size_t length_utf16() const noexcept
		{
			if ((m_value & 0xFFFF) == m_value)
				return 1;
			else
				return 2;
		}

	private:
		constexpr uchar(const std::uint32_t value) noexcept
			: m_value(value)
		{
		}

	private:
		std::uint32_t m_value;
	};

	inline namespace literals
	{
		[[nodiscard]] consteval ascii_char operator""_ac(const unsigned long long int value)
		{
			if (value > static_cast<unsigned long long int>(std::numeric_limits<std::uint8_t>::max()) ||
				!is_valid_ascii(static_cast<std::uint8_t>(value)))
			{
				// Note: this function is consteval, meaning this will cause a compile-time error
				throw "Invalid ASCII value";
			}

			return ascii_char::from(static_cast<std::uint8_t>(value)).value();
		}

		[[nodiscard]] consteval ascii_char operator""_ac(const char8_t value)
		{
			if (!is_valid_ascii(static_cast<std::uint8_t>(value)))
			{
				// Note: this function is consteval, meaning this will cause a compile-time error
				throw "Invalid ASCII value";
			}

			return ascii_char::from(static_cast<std::uint8_t>(value)).value();
		}

		[[nodiscard]] consteval uchar operator""_uc(const unsigned long long int value)
		{
			if (value > static_cast<unsigned long long int>(std::numeric_limits<std::uint32_t>::max()) ||
				!is_valid_usv(static_cast<std::uint32_t>(value)))
			{
				// Note: this function is consteval, meaning this will cause a compile-time error
				throw "Invalid Unicode scalar value";
			}

			return uchar::from(static_cast<std::uint32_t>(value)).value();
		}

		[[nodiscard]] consteval uchar operator""_uc(const char32_t value)
		{
			if (!is_valid_usv(static_cast<std::uint32_t>(value)))
			{
				// Note: this function is consteval, meaning this will cause a compile-time error
				throw "Invalid Unicode scalar value";
			}

			return uchar::from(static_cast<std::uint32_t>(value)).value();
		}
	} // namespace literals
} // namespace upp

template<>
struct std::hash<upp::ascii_char>
{
	[[nodiscard]] constexpr std::size_t operator()(const upp::ascii_char ch) const noexcept
	{
		return static_cast<std::size_t>(ch.value());
	}
};

template<>
struct std::hash<upp::uchar>
{
	[[nodiscard]] constexpr std::size_t operator()(const upp::uchar ch) const noexcept
	{
		return static_cast<std::size_t>(ch.value());
	}
};

#endif // UNI_CPP_CHAR_HPP