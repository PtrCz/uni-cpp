#include "../bugspray.hpp"

#include <uni-cpp/string.hpp>
#include <uni-cpp/encoding.hpp>

#include <type_traits>
#include <concepts>
#include <string>
#include <string_view>
#include <ranges>

#include "utility.hpp"
#include "evil_allocator.hpp"
#include "ranges.hpp"
#include "../utf.hpp"

TEST_CASE("string type traits", "[string types]")
{
    upp_test::run_for_each_string_type([&]<typename StringType>() {
        CHECK(std::default_initializable<StringType>);
        CHECK(std::is_nothrow_default_constructible_v<StringType>);
        CHECK(std::destructible<StringType>);
        CHECK(std::copy_constructible<StringType>);
        CHECK(std::is_nothrow_move_constructible_v<StringType>);
        // CHECK(std::is_nothrow_move_assignable_v<StringType>);
    });
}
EVAL_TEST_CASE("string type traits");

TEST_CASE("string constructors", "[string types]")
{
    upp_test::run_for_each_string_type_template([&]<template<typename> typename StringTemplate, typename CodeUnitType>() {
        using StringType = StringTemplate<std::basic_string<CodeUnitType>>; // NOLINT(readability-identifier-naming)

        StringType empty;
        StringType with_allocator{std::allocator<typename StringType::code_unit_type>()};

        CHECK(empty.underlying().empty());

        StringType copy = empty;
        StringType copy_with_allocator{copy, std::allocator<typename StringType::code_unit_type>()};

        StringType move = std::move(copy);
        StringType move_with_allocator{std::move(empty), std::allocator<typename StringType::code_unit_type>()};

        using evil_std_string = std::basic_string<CodeUnitType, std::char_traits<CodeUnitType>, upp_test::evil_allocator<CodeUnitType>>;

        RUNTIME_CHECK_THROWS(StringTemplate<evil_std_string>{});
        CHECK_FALSE(noexcept(StringTemplate<evil_std_string>{}));
    });
}
EVAL_TEST_CASE("string constructors");