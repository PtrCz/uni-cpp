#include "../bugspray.hpp"

#include <uni-cpp/string.hpp>
#include <uni-cpp/encoding.hpp>

#include <type_traits>
#include <concepts>
#include <string>
#include <string_view>
#include <ranges>

#include "evil_allocator.hpp"
#include "ranges.hpp"
#include "../utf8.hpp"

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

TEST_CASE("string type traits", "[string types]")
{
    run_for_each_string_type([&]<typename StringType>() {
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
    run_for_each_string_type_template([&]<template<typename> typename StringTemplate, typename CodeUnitType>() {
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

TEST_CASE("string from_utf8()", "[string types]")
{
    // `from_utf8` differs slightly in implementation depending on whether the argument is a bidirectional_range or not.
    // Here we're testing it on both kinds of ranges.

    using namespace std::string_literals;

    for (const auto& utf8_sequence : {u8"Hello, World!"s})
    {
        upp_test::string_input_range<char8_t> input_range{std::u8string_view{utf8_sequence}};

        const auto result             = upp::utf8_string::from_utf8(utf8_sequence);
        const auto input_range_result = upp::utf8_string::from_utf8(input_range);

        std::u8string utf8_sequence_copy{utf8_sequence};
        const auto    move_result = upp::utf8_string::from_utf8(std::move(utf8_sequence_copy));

        REQUIRE(result.has_value());
        REQUIRE(input_range_result.has_value());
        REQUIRE(move_result.has_value());

        CHECK(result->underlying() == utf8_sequence);
        CHECK(input_range_result->underlying() == utf8_sequence);
        CHECK(move_result->underlying() == utf8_sequence);

        // NOLINTNEXTLINE(bugprone-use-after-move)
        CHECK(utf8_sequence_copy.empty()); // Check if the underlying container has been moved from
    }

    for (const auto& [utf8_sequence, expected_error] : upp_test::utf8::error_test_cases())
    {
        upp_test::string_input_range<char8_t> input_range{std::u8string_view{utf8_sequence}};

        const auto result             = upp::utf8_string::from_utf8(utf8_sequence);
        const auto input_range_result = upp::utf8_string::from_utf8(input_range);

        REQUIRE_FALSE(result.has_value());
        REQUIRE_FALSE(input_range_result.has_value());

        CHECK(result.error() == expected_error);
        CHECK(input_range_result.error() == expected_error);
    }
}
EVAL_TEST_CASE("string from_utf8()");