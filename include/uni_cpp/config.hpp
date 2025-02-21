#pragma once
#ifndef UNI_CPP_CONFIG_HPP
#define UNI_CPP_CONFIG_HPP

#if !defined(__cplusplus) || (__cplusplus < 202002L && !defined(_MSVC_LANG)) || (defined(_MSVC_LANG) && _MSVC_LANG < 202002L)
#error "C++20 or higher is required"
#endif

#include <version>

#if !defined(__cpp_lib_expected) || (__cpp_lib_expected < 202211L)
#error "Standard <expected> support is required"
#endif

#include <cstdio>
#include <cstdlib>
#include <source_location>

#include "version.hpp"

namespace upp::impl
{
	[[noreturn]] inline void assert_fail(const char* cond, const std::source_location& loc = std::source_location::current())
	{
		std::fprintf(stderr, "%s:%u: %s: Assertion `%s` failed.\n", loc.file_name(), static_cast<unsigned>(loc.line()), loc.function_name(), cond);
		std::abort();
	}
} // namespace upp::impl

#if defined(UNI_CPP_FORCE_ENABLE_ASSERT) || !defined(NDEBUG)
#define UNI_CPP_ASSERT(...) ((__VA_ARGS__) ? ((void)0) : ::upp::impl::assert_fail(#__VA_ARGS__))
#else
#define UNI_CPP_ASSERT(...) ((void)0)
#endif

#if !defined(__cpp_deleted_function) || (__cpp_deleted_function < 202403L)
#define UNI_CPP_DELETE(reason) delete
#else
#define UNI_CPP_DELETE(reason) delete (reason)
#endif

#endif // UNI_CPP_CONFIG_HPP