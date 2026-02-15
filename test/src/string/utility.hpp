#ifndef TEST_STRING_UTILITY_HPP
#define TEST_STRING_UTILITY_HPP

#include <uni-cpp/string.hpp>

namespace upp_test
{
    template<typename Callable>
    constexpr void run_for_each_unicode_string_type_template(const Callable& callable)
    {
        callable.template operator()<upp::basic_utf8_string, char8_t>();
        callable.template operator()<upp::basic_utf16_string, char16_t>();
        callable.template operator()<upp::basic_utf32_string, char32_t>();
    }

    template<typename Callable>
    constexpr void run_for_each_string_type_template(const Callable& callable)
    {
        callable.template operator()<upp::basic_ascii_string, char>();
        run_for_each_unicode_string_type_template(callable);
    }

    template<typename Callable>
    constexpr void run_for_each_unicode_string_type(const Callable& callable)
    {
        run_for_each_unicode_string_type_template([&]<template<typename> typename StringTemplate, typename CodeUnitType>() {
            callable.template operator()<StringTemplate<std::basic_string<CodeUnitType>>>();
        });
    }

    template<typename Callable>
    constexpr void run_for_each_string_type(const Callable& callable)
    {
        run_for_each_string_type_template([&]<template<typename> typename StringTemplate, typename CodeUnitType>() {
            callable.template operator()<StringTemplate<std::basic_string<CodeUnitType>>>();
        });
    }
} // namespace upp_test

#endif // TEST_STRING_UTILITY_HPP