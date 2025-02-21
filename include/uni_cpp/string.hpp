#pragma once
#ifndef UNI_CPP_STRING_HPP
#define UNI_CPP_STRING_HPP

#include "config.hpp"

#include "char.hpp"

#include <cstdint>
#include <memory>
#include <cstddef>
#include <span>
#include <string>
#include <iterator>
#include <utility>
#include <type_traits>

namespace upp
{
	namespace impl
	{
		template<typename T, typename value_type>
		concept allocator_for = requires(T& alloc) {
			typename T::value_type;
			alloc.deallocate(alloc.allocate(1uz), 1uz);
		} && std::is_same_v<typename T::value_type, value_type>;

		template<typename R, typename T>
		concept container_compatible_range = std::ranges::input_range<R> && std::convertible_to<std::ranges::range_reference_t<R>, T>;
	} // namespace impl

	template<impl::allocator_for<char8_t> alloc_type = std::allocator<char8_t>>
	class basic_ascii_string;

	template<impl::allocator_for<char8_t> alloc_type = std::allocator<char8_t>>
	class basic_utf8_string;

	template<impl::allocator_for<char16_t> alloc_type = std::allocator<char16_t>>
	class basic_utf16_string;

	template<impl::allocator_for<char32_t> alloc_type = std::allocator<char32_t>>
	class basic_utf32_string;

	class ascii_string_view;
	class utf8_string_view;
	class utf16_string_view;
	class utf32_string_view;

	namespace impl
	{
		template<typename T, typename... types>
		constexpr bool is_any_of = (std::is_same_v<T, types> || ...);

		template<typename>
		struct is_unicode_string : std::false_type
		{
		};

		template<impl::allocator_for<char8_t> alloc_type>
		struct is_unicode_string<basic_utf8_string<alloc_type>> : std::true_type
		{
		};

		template<impl::allocator_for<char16_t> alloc_type>
		struct is_unicode_string<basic_utf16_string<alloc_type>> : std::true_type
		{
		};

		template<impl::allocator_for<char32_t> alloc_type>
		struct is_unicode_string<basic_utf32_string<alloc_type>> : std::true_type
		{
		};

		template<typename>
		struct is_ascii_string : std::false_type
		{
		};

		template<impl::allocator_for<char8_t> alloc_type>
		struct is_ascii_string<basic_ascii_string<alloc_type>> : std::true_type
		{
		};
	} // namespace impl

	template<typename T>
	concept unicode_string_type = impl::is_unicode_string<T>::value;

	template<typename T>
	concept string_type = impl::is_ascii_string<T>::value || unicode_string_type<T>;

	template<typename T>
	concept unicode_string_view_type = impl::is_any_of<T, utf8_string_view, utf16_string_view, utf32_string_view>;

	template<typename T>
	concept string_view_type = std::is_same_v<T, ascii_string_view> || unicode_string_view_type<T>;

	namespace impl
	{
		enum class encoding : std::uint8_t
		{
			ascii,
			utf8,
			utf16,
			utf32
		};

		template<encoding>
		struct encoding_properties
		{
		};

		template<>
		struct encoding_properties<encoding::ascii>
		{
			using char_type		 = ascii_char;
			using code_unit_type = char8_t;
			using view_type		 = ascii_string_view;

			static constexpr bool is_variable_width = false;

			static constexpr code_unit_type encode(char_type ch) noexcept
			{
				return static_cast<code_unit_type>(ch.value());
			}
		};

		template<>
		struct encoding_properties<encoding::utf8>
		{
			using char_type		 = uchar;
			using code_unit_type = char8_t;
			using view_type		 = utf8_string_view;

			static constexpr bool is_variable_width = true;

			static constexpr uchar::encode_utf8_t encode(char_type ch) noexcept
			{
				return ch.encode_utf8();
			}
		};

		template<>
		struct encoding_properties<encoding::utf16>
		{
			using char_type		 = uchar;
			using code_unit_type = char16_t;
			using view_type		 = utf16_string_view;

			static constexpr bool is_variable_width = true;

			static constexpr uchar::encode_utf16_t encode(char_type ch) noexcept
			{
				return ch.encode_utf16();
			}
		};

		template<>
		struct encoding_properties<encoding::utf32>
		{
			using char_type		 = uchar;
			using code_unit_type = char32_t;
			using view_type		 = utf32_string_view;

			static constexpr bool is_variable_width = false;

			static constexpr code_unit_type encode(char_type ch) noexcept
			{
				return static_cast<code_unit_type>(ch.value());
			}
		};

		template<encoding enc, typename alloc_type>
		class basic_string_base
		{
		public:
			using code_unit_type  = encoding_properties<enc>::code_unit_type;
			using allocator_type  = alloc_type;
			using size_type		  = std::allocator_traits<alloc_type>::size_type;
			using difference_type = std::allocator_traits<alloc_type>::difference_type;
			using view_type		  = encoding_properties<enc>::view_type;
			using char_type		  = encoding_properties<enc>::char_type;

			constexpr basic_string_base() noexcept(std::is_nothrow_default_constructible_v<allocator_type>)
				: basic_string_base(allocator_type())
			{
			}
			explicit constexpr basic_string_base(const allocator_type& alloc) noexcept
				: m_string(alloc)
			{
			}
			constexpr basic_string_base(size_type count, char_type ch, const allocator_type& alloc = allocator_type())
				: m_string(alloc)
			{ // TODO
			}
			template<std::input_iterator I, std::sentinel_for<I> S>
				requires std::is_same_v<char_type, std::iter_value_t<I>>
			constexpr basic_string_base(I iter, S sentinel, const allocator_type& alloc = allocator_type())
				: m_string(alloc)
			{ // TODO: make sure that this has strong exception safety guarantee
				if constexpr (std::forward_iterator<I>)
				{
					// Note: for variable-width encodings this may underestimate the required space
					m_string.reserve(std::ranges::distance(iter, sentinel));
				}
				for (; iter != sentinel; ++iter)
					push_back(char_type(*iter));
			}

			template<container_compatible_range<char_type> R>
			constexpr basic_string_base(std::from_range_t, R&& range, const allocator_type& alloc = allocator_type())
				: basic_string_base(std::ranges::begin(range), std::ranges::end(range), alloc)
			{
			}

			constexpr basic_string_base(const char*, const allocator_type& = allocator_type())			 = UNI_CPP_DELETE(""); // TODO: message
			constexpr basic_string_base(const code_unit_type*, const allocator_type& = allocator_type()) = UNI_CPP_DELETE(""); // TODO: message

			explicit constexpr basic_string_base(view_type sv, const allocator_type& alloc = allocator_type())
			{ // TODO
			}
			constexpr basic_string_base(view_type sv, size_type pos, size_type count, const allocator_type& alloc = allocator_type())
			{ // TODO
			}

			constexpr basic_string_base(const basic_string_base& other)		= default;
			constexpr basic_string_base(basic_string_base&& other) noexcept = default;

			constexpr basic_string_base(const basic_string_base& other, const allocator_type& alloc)
				: m_string(other.m_string, alloc)
			{
			}
			constexpr basic_string_base(basic_string_base&& other, const allocator_type& alloc)
				: m_string(std::move(other.m_string), alloc)
			{
			}

			constexpr basic_string_base(const basic_string_base& other, size_type pos, const allocator_type& alloc = allocator_type())
			{ // TODO
			}
			constexpr basic_string_base(basic_string_base&& other, size_type pos, const allocator_type& alloc = allocator_type())
			{ // TODO
			}
			constexpr basic_string_base(const basic_string_base& other, size_type pos, size_type count,
										const allocator_type& alloc = allocator_type())
			{ // TODO
			}
			constexpr basic_string_base(basic_string_base&& other, size_type pos, size_type count, const allocator_type& alloc = allocator_type())
			{ // TODO
			}

			constexpr basic_string_base(std::initializer_list<char_type> ilist, const allocator_type& alloc = allocator_type())
			{ // TODO
			}

			[[nodiscard]] std::span<const std::byte> as_bytes() const noexcept
			{
				return std::span<const std::byte>{reinterpret_cast<const std::byte*>(m_string.data()),
												  reinterpret_cast<const std::byte*>(m_string.data() + m_string.size())};
			}

			[[nodiscard]] constexpr std::span<const code_unit_type> as_code_units() const noexcept
			{
				return std::span<const code_unit_type>{m_string.data(), m_string.size()};
			}

			constexpr void push_back(char_type ch)
			{
				if constexpr (encoding_properties<enc>::is_variable_width)
				{
					const auto encoded = encoding_properties<enc>::encode(ch);
					m_string.append(encoded.data(), static_cast<size_type>(encoded.size()));
				}
				else
				{
					m_string.push_back(encoding_properties<enc>::encode(ch));
				}
			}

			[[nodiscard]] constexpr view_type view() const noexcept
			{
				return view_type(); // TODO
			}

		protected:
			std::basic_string<code_unit_type, std::char_traits<code_unit_type>, alloc_type> m_string;
		};

		template<encoding enc, typename alloc_type>
		class basic_unicode_string_base : public basic_string_base<enc, alloc_type>
		{
		private:
			using base = basic_string_base<enc, alloc_type>;

		protected:
			using base::m_string;

		public:
			using base::base;
		};
	} // namespace impl

	template<impl::allocator_for<char8_t> alloc_type>
	class basic_ascii_string final : public impl::basic_string_base<impl::encoding::ascii, alloc_type>
	{
	private:
		using base = impl::basic_string_base<impl::encoding::ascii, alloc_type>;
		using base::m_string;

	public:
		using base::base;

		constexpr bool operator==(const basic_ascii_string&) const noexcept = default;

		[[nodiscard]] const char* c_str() const noexcept
		{
			return reinterpret_cast<const char*>(m_string.c_str());
		}
	};

	template<impl::allocator_for<char8_t> alloc_type>
	class basic_utf8_string final : public impl::basic_unicode_string_base<impl::encoding::utf8, alloc_type>
	{
	private:
		using base = impl::basic_unicode_string_base<impl::encoding::utf8, alloc_type>;
		using base::m_string;

	public:
		using base::base;

		[[nodiscard]] const char* c_str() const noexcept
		{
			return reinterpret_cast<const char*>(m_string.c_str());
		}
	};

	template<impl::allocator_for<char16_t> alloc_type>
	class basic_utf16_string final : public impl::basic_unicode_string_base<impl::encoding::utf16, alloc_type>
	{
	private:
		using base = impl::basic_unicode_string_base<impl::encoding::utf16, alloc_type>;
		using base::m_string;

	public:
		using base::base;
	};

	template<impl::allocator_for<char32_t> alloc_type>
	class basic_utf32_string final : public impl::basic_unicode_string_base<impl::encoding::utf32, alloc_type>
	{
	private:
		using base = impl::basic_unicode_string_base<impl::encoding::utf32, alloc_type>;
		using base::m_string;

	public:
		using base::base;
	};

	namespace impl
	{
		template<encoding enc>
		class string_view_base
		{
		public:
		protected:
			int m_view;
		};

		template<encoding enc>
		class unicode_string_view_base : public string_view_base<enc>
		{
		private:
			using base = string_view_base<enc>;

		protected:
			using base::m_view;

		public:
			using base::base;
		};
	} // namespace impl

	class ascii_string_view final : public impl::string_view_base<impl::encoding::ascii>
	{
	private:
		using base = impl::string_view_base<impl::encoding::ascii>;
		using base::m_view;

	public:
		template<impl::allocator_for<char8_t> alloc_type = std::allocator<char8_t>>
		using basic_string_type = basic_ascii_string<alloc_type>;
		using string_type		= basic_string_type<>;

		using base::base;
	};

	class utf8_string_view final : public impl::unicode_string_view_base<impl::encoding::utf8>
	{
	private:
		using base = impl::unicode_string_view_base<impl::encoding::utf8>;
		using base::m_view;

	public:
		template<impl::allocator_for<char8_t> alloc_type = std::allocator<char8_t>>
		using basic_string_type = basic_utf8_string<alloc_type>;
		using string_type		= basic_string_type<>;

		using base::base;
	};

	class utf16_string_view final : public impl::unicode_string_view_base<impl::encoding::utf16>
	{
	private:
		using base = impl::unicode_string_view_base<impl::encoding::utf16>;
		using base::m_view;

	public:
		template<impl::allocator_for<char16_t> alloc_type = std::allocator<char16_t>>
		using basic_string_type = basic_utf16_string<alloc_type>;
		using string_type		= basic_string_type<>;

		using base::base;
	};

	class utf32_string_view final : public impl::unicode_string_view_base<impl::encoding::utf32>
	{
	private:
		using base = impl::unicode_string_view_base<impl::encoding::utf32>;
		using base::m_view;

	public:
		template<impl::allocator_for<char32_t> alloc_type = std::allocator<char32_t>>
		using basic_string_type = basic_utf32_string<alloc_type>;
		using string_type		= basic_string_type<>;

		using base::base;
	};

	template<impl::allocator_for<char8_t> alloc_type = std::allocator<char8_t>>
	using basic_ustring = basic_utf8_string<alloc_type>;

	using ascii_string = basic_ascii_string<>;
	using utf8_string  = basic_utf8_string<>;
	using utf16_string = basic_utf16_string<>;
	using utf32_string = basic_utf32_string<>;
	using ustring	   = basic_ustring<>;
	using ustring_view = utf8_string_view;

	namespace pmr
	{
		using ascii_string = basic_ascii_string<std::pmr::polymorphic_allocator<char8_t>>;
		using utf8_string  = basic_utf8_string<std::pmr::polymorphic_allocator<char8_t>>;
		using utf16_string = basic_utf16_string<std::pmr::polymorphic_allocator<char16_t>>;
		using utf32_string = basic_utf32_string<std::pmr::polymorphic_allocator<char32_t>>;
		using ustring	   = basic_ustring<std::pmr::polymorphic_allocator<char8_t>>;
	} // namespace pmr

	inline namespace literals
	{
		[[nodiscard]] consteval ascii_string_view operator""_asv(const char8_t* str, std::size_t size)
		{
			return ascii_string_view(); // TODO
		}

		[[nodiscard]] consteval utf8_string_view operator""_u8sv(const char8_t* str, std::size_t size)
		{
			return utf8_string_view(); // TODO
		}

		[[nodiscard]] consteval utf16_string_view operator""_u16sv(const char16_t* str, std::size_t size)
		{
			return utf16_string_view(); // TODO
		}

		[[nodiscard]] consteval utf32_string_view operator""_u32sv(const char32_t* str, std::size_t size)
		{
			return utf32_string_view(); // TODO
		}

		[[nodiscard]] consteval utf8_string_view operator""_usv(const char8_t* str, std::size_t size)
		{
			return operator""_u8sv(str, size);
		}
	} // namespace literals
} // namespace upp

template<>
struct std::hash<upp::ascii_string_view>
{
	[[nodiscard]] constexpr std::size_t operator()(const upp::ascii_string_view view) const noexcept
	{
		return static_cast<std::size_t>(5); // TODO
	}
};

template<upp::unicode_string_view_type T>
struct std::hash<T>
{
	[[nodiscard]] constexpr std::size_t operator()(const T view) const noexcept
	{
		return static_cast<std::size_t>(5); // TODO
	}
};

template<upp::string_type T>
struct std::hash<T>
{
	[[nodiscard]] constexpr std::size_t operator()(const T& str) const noexcept
	{
		return std::hash<typename T::view_type>{}(str.view());
	}
};

#endif // UNI_CPP_STRING_HPP