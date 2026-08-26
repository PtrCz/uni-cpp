#include "../catch2.hpp"

#include <uni-cpp/string.hpp>
#include <uni-cpp/encoding.hpp>

#include <type_traits>
#include <concepts>
#include <string>
#include <string_view>
#include <ranges>

#include "utility.hpp"
#include "ranges.hpp"

TEST_CASE("string type traits", "[string types]")
{
    upp_test::run_for_each_string_type([&]<typename StringType>() {
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
    upp_test::run_for_each_string_type([&]<typename StringType>() {
        StringType empty;
        StringType with_allocator{std::allocator<typename StringType::code_unit_type>()};

        CHECK(empty.underlying().empty());

        StringType copy = empty;
        StringType copy_with_allocator{copy, std::allocator<typename StringType::code_unit_type>()};

        StringType move = std::move(copy);
        StringType move_with_allocator{std::move(empty), std::allocator<typename StringType::code_unit_type>()};
    });
}