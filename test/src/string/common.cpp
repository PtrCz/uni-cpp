#include <catch2/catch_test_macros.hpp>

#include <uni-cpp/string.hpp>

#include <type_traits>
#include <concepts>

#include "evil_allocator.hpp"

template<typename Callable>
void run_for_each_string_type_template(const Callable& callable)
{
    callable.template operator()<upp::basic_ascii_string, char>();
    callable.template operator()<upp::basic_utf8_string, char8_t>();
    callable.template operator()<upp::basic_utf16_string, char16_t>();
    callable.template operator()<upp::basic_utf32_string, char32_t>();
}

template<typename Callable>
void run_for_each_string_type(const Callable& callable)
{
    run_for_each_string_type_template([&]<template<typename> typename StringTemplate, typename CodeUnitType>() {
        callable.template operator()<StringTemplate<std::allocator<CodeUnitType>>>();
    });
}

TEST_CASE("string type traits", "[string types]")
{
    run_for_each_string_type([&]<typename StringType>() {
        STATIC_CHECK(std::default_initializable<StringType>);
        STATIC_CHECK(std::is_nothrow_default_constructible_v<StringType>);
        STATIC_CHECK(std::destructible<StringType>);
        STATIC_CHECK(std::copy_constructible<StringType>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<StringType>);
        // STATIC_CHECK(std::is_nothrow_move_assignable_v<StringType>);
    });
}

TEST_CASE("string constructors", "[string types]")
{
    run_for_each_string_type_template([&]<template<typename> typename StringTemplate, typename CodeUnitType>() {
        using StringType = StringTemplate<std::allocator<CodeUnitType>>;

        StringType empty;
        StringType with_allocator{std::allocator<typename StringType::code_unit_type>()};

        StringType copy = empty;
        StringType copy_with_allocator{copy, std::allocator<typename StringType::code_unit_type>()};

        StringType move = std::move(copy);
        StringType move_with_allocator{std::move(empty), std::allocator<typename StringType::code_unit_type>()};

        CHECK_THROWS(StringTemplate<upp_test::evil_allocator<CodeUnitType, true>>{});
        CHECK_NOTHROW(StringTemplate<upp_test::evil_allocator<CodeUnitType, false>>{});
    });
}