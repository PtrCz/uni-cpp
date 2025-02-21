#pragma once
#ifndef UNI_CPP_CHAR_HPP
#define UNI_CPP_CHAR_HPP

#include "config.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <array>
#include <type_traits>
#include <utility>
#include <compare>
#include <limits>
#include <bit>

#if !defined(__cpp_size_t_suffix) && defined(__INTELLISENSE__)

inline namespace intellisense_fix
{
	consteval std::size_t operator"" uz(unsigned long long int val) noexcept
	{
		return static_cast<std::size_t>(val);
	}
} // namespace intellisense_fix

#endif

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

	template<typename T>
	concept char_type = std::is_same_v<T, ascii_char> || std::is_same_v<T, uchar>;

	namespace impl
	{
		// inplace_vector like class used by:
		// encode_utf8_t/encode_utf16_t and to_lowercase_t/to_uppercase_t
		template<typename T, std::size_t buffer_size>
		class inplace_vector_like
		{
		public:
			using value_type			 = T;
			using reference				 = T&;
			using const_reference		 = const T&;
			using iterator				 = const T*;
			using const_iterator		 = const T*;
			using reverse_iterator		 = std::reverse_iterator<iterator>;
			using const_reverse_iterator = std::reverse_iterator<const_iterator>;
			using difference_type		 = std::ptrdiff_t;
			using size_type				 = std::size_t;

			constexpr inplace_vector_like() noexcept
				: m_size(0U)
			{
			}

			constexpr inplace_vector_like(const inplace_vector_like&) noexcept = default;
			constexpr inplace_vector_like(inplace_vector_like&&) noexcept	   = default;

			constexpr ~inplace_vector_like() noexcept = default;

			constexpr inplace_vector_like& operator=(const inplace_vector_like&) noexcept = default;
			constexpr inplace_vector_like& operator=(inplace_vector_like&&) noexcept	  = default;

			constexpr const_iterator begin() const noexcept
			{
				return m_data.data();
			}
			constexpr const_iterator cbegin() const noexcept
			{
				return begin();
			}
			constexpr const_iterator end() const noexcept
			{
				return m_data.data() + m_size;
			}
			constexpr const_iterator cend() const noexcept
			{
				return end();
			}
			constexpr const_reverse_iterator rbegin() const noexcept
			{
				return const_reverse_iterator(end());
			}
			constexpr const_reverse_iterator crbegin() const noexcept
			{
				return rbegin();
			}
			constexpr const_reverse_iterator rend() const noexcept
			{
				return const_reverse_iterator(begin());
			}
			constexpr const_reverse_iterator crend() const noexcept
			{
				return rend();
			}

			constexpr size_type size() const noexcept
			{
				return static_cast<size_type>(m_size);
			}
			constexpr size_type max_size() const noexcept
			{
				return buffer_size;
			}

			constexpr const T* data() const noexcept
			{
				return m_data.data();
			}

			constexpr bool empty() const noexcept
			{
				return m_size == 0;
			}

			constexpr void swap(inplace_vector_like& other) noexcept
			{
				std::swap(*this, other);
			}

			constexpr bool operator==(const inplace_vector_like& other) const noexcept
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
			constexpr inplace_vector_like(std::array<T, buffer_size> data, std::uint32_t psize) noexcept
				: m_data(data)
				, m_size(psize)
			{
			}

			friend uchar;

		private:
			std::array<T, buffer_size> m_data;
			std::uint32_t			   m_size;
		};

		template<typename T, std::size_t buffer_size>
		class encode_utf : public inplace_vector_like<T, buffer_size>
		{
		private:
			using base = inplace_vector_like<T, buffer_size>;
			friend uchar;

		public:
			using base::base;
		};

		enum class to_case_enum : std::uint8_t
		{
			lower,
			upper
		};

		// 1st note: template on to_case_enum so that to_lowercase_t and to_uppercase_t are seperate types
		// 2nd note: T is always uchar, but needs to be template param because uchar is an incomplete type here
		template<to_case_enum, typename T = uchar>
		class to_case : public inplace_vector_like<T, 3>
		{
		private:
			using base = inplace_vector_like<T, 3>;
			friend uchar;

		public:
			using base::base;

			constexpr T simple_mapping() const noexcept
			{
				return *data();
			}
		};
	} // namespace impl

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
		using encode_utf8_t	 = impl::encode_utf<char8_t, 4>;
		using encode_utf16_t = impl::encode_utf<char16_t, 2>;

		using to_lowercase_t = impl::to_case<impl::to_case_enum::lower>;
		using to_uppercase_t = impl::to_case<impl::to_case_enum::upper>;

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

		[[nodiscard]] constexpr to_lowercase_t to_lowercase() const noexcept
		{
			std::array<uchar, 3> arr{*this};
			return to_lowercase_t(arr, 1U); // TODO
		}
		[[nodiscard]] constexpr to_uppercase_t to_uppercase() const noexcept
		{
			std::array<uchar, 3> arr{*this};
			return to_uppercase_t(arr, 1U); // TODO
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

		[[nodiscard]] constexpr encode_utf8_t encode_utf8() const noexcept
		{
			std::array<char8_t, 4> arr;
			const std::size_t	   size_utf8 = length_utf8();

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

			return encode_utf8_t(arr, static_cast<std::uint32_t>(size_utf8));
		}
		[[nodiscard]] constexpr encode_utf16_t encode_utf16() const noexcept
		{
			std::array<char16_t, 2> arr;
			const std::size_t		size_utf16 = length_utf16();

			if (size_utf16 == 1)
			{
				arr[0] = static_cast<char16_t>(m_value);
			}
			else // size_utf16 == 2
			{
				const std::uint32_t code = m_value - 0x10'000;

				arr[0] = static_cast<char16_t>(0xD800 | (code >> 10));
				arr[1] = static_cast<char16_t>(0xDC00 | (code & 0x3FF));
			}
			return encode_utf16_t(arr, static_cast<std::uint32_t>(size_utf16));
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
				throw std::invalid_argument("Invalid ASCII value");
			}

			return ascii_char::from(static_cast<std::uint8_t>(value)).value();
		}

		[[nodiscard]] consteval ascii_char operator""_ac(const char8_t value)
		{
			if (!is_valid_ascii(static_cast<std::uint8_t>(value)))
			{
				throw std::invalid_argument("Invalid ASCII value");
			}

			return ascii_char::from(static_cast<std::uint8_t>(value)).value();
		}

		[[nodiscard]] consteval uchar operator""_uc(const unsigned long long int value)
		{
			if (value > static_cast<unsigned long long int>(std::numeric_limits<std::uint32_t>::max()) ||
				!is_valid_usv(static_cast<std::uint32_t>(value)))
			{
				throw std::invalid_argument("Invalid Unicode scalar value");
			}

			return uchar::from(static_cast<std::uint32_t>(value)).value();
		}

		[[nodiscard]] consteval uchar operator""_uc(const char32_t value)
		{
			if (!is_valid_usv(static_cast<std::uint32_t>(value)))
			{
				throw std::invalid_argument("Invalid Unicode scalar value");
			}

			return uchar::from(static_cast<std::uint32_t>(value)).value();
		}
	} // namespace literals
} // namespace upp

template<upp::char_type T>
struct std::hash<T>
{
	[[nodiscard]] constexpr std::size_t operator()(const T ch) const noexcept
	{
		return static_cast<std::size_t>(ch.value());
	}
};

#endif // UNI_CPP_CHAR_HPP