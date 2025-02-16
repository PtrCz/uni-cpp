#pragma once
#ifndef UNI_CPP_STRING_HPP
#define UNI_CPP_STRING_HPP

#include "config.hpp"

#include "char.hpp"

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <string>
#include <initializer_list>
#include <memory>
#include <memory_resource>
#include <span>
#include <bit>
#include <expected>
#include <cstddef>

namespace upp
{
	class uchar;

	namespace impl
	{
		template<typename T>
		concept allocator_concept = requires(T& alloc) {
			typename T::value_type;
			alloc.deallocate(alloc.allocate(std::size_t{1}), std::size_t{1});
		};

		template<typename R, typename T>
		concept container_compatible_range = std::ranges::input_range<R> && std::convertible_to<std::ranges::range_reference_t<R>, T>;
	} // namespace impl

	template<impl::allocator_concept alloc_type = std::allocator<char>>
	class basic_ascii_string;

	template<impl::allocator_concept alloc_type = std::allocator<char8_t>>
	class basic_utf8_string;

	template<impl::allocator_concept alloc_type = std::allocator<char16_t>>
	class basic_utf16_string;

	template<impl::allocator_concept alloc_type = std::allocator<char32_t>>
	class basic_utf32_string;

	template<impl::allocator_concept alloc_type = std::allocator<char8_t>>
	using basic_ustring = basic_utf8_string<alloc_type>;

	class ascii_string_view;
	class utf8_string_view;
	class utf16_string_view;
	class utf32_string_view;

	template<typename T>
	struct is_unicode_string : std::false_type
	{
	};

	template<impl::allocator_concept alloc_type>
	struct is_unicode_string<basic_utf8_string<alloc_type>> : std::true_type
	{
	};

	template<impl::allocator_concept alloc_type>
	struct is_unicode_string<basic_utf16_string<alloc_type>> : std::true_type
	{
	};

	template<impl::allocator_concept alloc_type>
	struct is_unicode_string<basic_utf32_string<alloc_type>> : std::true_type
	{
	};

	template<typename T>
	constexpr bool is_unicode_string_v = is_unicode_string<T>::value;

	template<typename T>
	concept unicode_string = is_unicode_string<T>::value;

	template<typename T>
	struct is_unicode_string_view : std::false_type
	{
	};

	template<>
	struct is_unicode_string_view<utf8_string_view> : std::true_type
	{
	};

	template<>
	struct is_unicode_string_view<utf16_string_view> : std::true_type
	{
	};

	template<>
	struct is_unicode_string_view<utf32_string_view> : std::true_type
	{
	};

	template<typename T>
	constexpr bool is_unicode_string_view_v = is_unicode_string_view<T>::value;

	template<typename T>
	concept unicode_string_view = is_unicode_string_view<T>::value;

	namespace impl
	{
		template<impl::allocator_concept alloc_type, typename char_type, typename storage_char_type>
		class basic_string_crtp_base
		{
		public:
			using allocator_type  = alloc_type;
			using size_type		  = std::allocator_traits<allocator_type>::size_type;
			using difference_type = std::allocator_traits<allocator_type>::difference_type;

			template<typename Self>
			[[nodiscard]] constexpr std::span<const std::byte> as_bytes(this Self&& self) noexcept
			{
				return std::span<const std::byte>(static_cast<const std::byte*>(self.m_string.data()),
												  static_cast<const std::byte*>(self.m_string.data() + self.m_string.size()));
			}

		private:
		};
	} // namespace impl

#define UNI_CPP_COMMON_STRING_INTERFACE(basic_string_type)
#define UNI_CPP_COMMON_STRING_INTERFACEA(basic_string_type)                                                                                         \
public:                                                                                                                                         \
	using allocator_type  = base::allocator_type;                                                                                               \
	using size_type		  = base::size_type;                                                                                                    \
	using difference_type = base::difference_type;                                                                                              \
                                                                                                                                                \
	constexpr basic_string_type() noexcept(std::is_nothrow_default_constructible_v<allocator_type>)                                             \
		: basic_string_type(allocator_type())                                                                                                   \
	{                                                                                                                                           \
	}                                                                                                                                           \
	explicit constexpr basic_string_type(const allocator_type& alloc) noexcept                                                                  \
	{                                                                                                                                           \
	}                                                                                                                                           \
                                                                                                                                                \
	constexpr basic_string_type(size_type count, ascii_char ch, const allocator_type& alloc = allocator_type())                                 \
	{                                                                                                                                           \
	}                                                                                                                                           \
                                                                                                                                                \
	template<std::input_iterator input_iter>                                                                                                    \
	constexpr basic_string_type(input_iter begin_it, input_iter end_it, const allocator_type& alloc = allocator_type())                         \
	{                                                                                                                                           \
	}                                                                                                                                           \
                                                                                                                                                \
	template<impl::container_compatible_range R>                                                                                                \
	constexpr basic_string_type(std::from_range_t, R&& rg, const allocator_type& = allocator_type())                                            \
	{                                                                                                                                           \
	}                                                                                                                                           \
                                                                                                                                                \
	constexpr basic_string_type(const char* s, size_type count, const allocator_type& alloc = allocator_type()) = delete;                       \
	constexpr basic_string_type(const char* s, const allocator_type& alloc = allocator_type())					= delete;                       \
                                                                                                                                                \
	constexpr basic_string_type(std::nullptr_t) = delete;                                                                                       \
                                                                                                                                                \
	template<typename string_view_like>                                                                                                         \
	explicit constexpr basic_string_type(const string_view_like& t, const allocator_type& alloc = allocator_type())                             \
	{                                                                                                                                           \
	}                                                                                                                                           \
                                                                                                                                                \
	template<class StringViewLike>                                                                                                              \
	basic_string_type(const StringViewLike& t, size_type pos, size_type count, const Allocator& alloc = Allocator());                           \
                                                                                                                                                \
	constexpr basic_string_type(const basic_string_type& other)                                                                                 \
	{                                                                                                                                           \
	}                                                                                                                                           \
	constexpr basic_string_type(basic_string_type&& other) noexcept                                                                             \
	{                                                                                                                                           \
	}                                                                                                                                           \
                                                                                                                                                \
	constexpr basic_string_type(const basic_string_type& other, const allocator_type& alloc)                                                    \
	{                                                                                                                                           \
	}                                                                                                                                           \
	constexpr basic_string_type(basic_string_type&& other, const allocator_type& alloc)                                                         \
	{                                                                                                                                           \
	}                                                                                                                                           \
                                                                                                                                                \
	constexpr basic_string_type(const basic_string_type& other, size_type pos, const allocator_type& alloc = allocator_type())                  \
	{                                                                                                                                           \
	}                                                                                                                                           \
	constexpr basic_string_type(basic_string_type&& other, size_type pos, const allocator_type& alloc = allocator_type())                       \
	{                                                                                                                                           \
	}                                                                                                                                           \
                                                                                                                                                \
	constexpr basic_string_type(const basic_string_type& other, size_type pos, size_type count, const allocator_type& alloc = allocator_type()) \
	{                                                                                                                                           \
	}                                                                                                                                           \
                                                                                                                                                \
	constexpr basic_string_type(basic_string_type&& other, size_type pos, size_type count, const allocator_type& alloc = allocator_type())      \
	{                                                                                                                                           \
	}                                                                                                                                           \
                                                                                                                                                \
	constexpr basic_string_type(std::initializer_list<ascii_char> ilist, const allocator_type& alloc = allocator_type())                        \
	{                                                                                                                                           \
	}

#define UNI_CPP_COMMON_UNICODE_STRING_INTERFACE(basic_string_type) UNI_CPP_COMMON_STRING_INTERFACE(basic_string_type)

	template<impl::allocator_concept alloc_type>
	class basic_ascii_string : public impl::basic_string_crtp_base<alloc_type, ascii_char, char>
	{
	private:
		using base = impl::basic_string_crtp_base<alloc_type, ascii_char, char>;
		friend base;

	public:
		UNI_CPP_COMMON_STRING_INTERFACE(basic_ascii_string);

	private:
		std::basic_string<char, std::char_traits<char>, alloc_type> m_string;
	};

	template<impl::allocator_concept alloc_type>
	class basic_utf8_string : public impl::basic_string_crtp_base<alloc_type, uchar, char8_t>
	{
	private:
		using base = impl::basic_string_crtp_base<alloc_type, uchar, char8_t>;
		friend base;

	public:
		UNI_CPP_COMMON_UNICODE_STRING_INTERFACE(basic_utf8_string);

	private:
		std::basic_string<char8_t, std::char_traits<char8_t>, alloc_type> m_string;
	};

	template<impl::allocator_concept alloc_type>
	class basic_utf16_string : public impl::basic_string_crtp_base<alloc_type, uchar, char16_t>
	{
	private:
		using base = impl::basic_string_crtp_base<alloc_type, uchar, char16_t>;
		friend base;

	public:
		UNI_CPP_COMMON_UNICODE_STRING_INTERFACE(basic_utf16_string);

	private:
		std::basic_string<char16_t, std::char_traits<char16_t>, alloc_type> m_string;
	};

	template<impl::allocator_concept alloc_type>
	class basic_utf32_string : public impl::basic_string_crtp_base<alloc_type, uchar, char32_t>
	{
	private:
		using base = impl::basic_string_crtp_base<alloc_type, uchar, char32_t>;
		friend base;

	public:
		UNI_CPP_COMMON_UNICODE_STRING_INTERFACE(basic_utf32_string);

	private:
		std::basic_string<char32_t, std::char_traits<char32_t>, alloc_type> m_string;
	};

#undef UNI_CPP_COMMON_UNICODE_STRING_INTERFACE
#undef UNI_CPP_COMMON_STRING_INTERFACE

	class ascii_string_view
	{
	public:
	private:
	};

	class utf8_string_view
	{
	public:
	private:
	};

	class utf16_string_view
	{
	public:
	private:
	};

	class utf32_string_view
	{
	public:
	private:
	};

	using ascii_string = basic_ascii_string<>;
	using utf8_string  = basic_utf8_string<>;
	using utf16_string = basic_utf16_string<>;
	using utf32_string = basic_utf32_string<>;
	using ustring	   = basic_ustring<>;
	using ustring_view = utf8_string_view;

	namespace pmr
	{
		using ascii_string = basic_ascii_string<std::pmr::polymorphic_allocator<char>>;
		using utf8_string  = basic_utf8_string<std::pmr::polymorphic_allocator<char8_t>>;
		using utf16_string = basic_utf16_string<std::pmr::polymorphic_allocator<char16_t>>;
		using utf32_string = basic_utf32_string<std::pmr::polymorphic_allocator<char32_t>>;
		using ustring	   = basic_ustring<std::pmr::polymorphic_allocator<char8_t>>;
	} // namespace pmr
} // namespace upp

#endif // UNI_CPP_STRING_HPP