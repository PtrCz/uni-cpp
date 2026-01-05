#include <catch2/catch_test_macros.hpp>

#include <uni-cpp/string.hpp>

#include <type_traits>
#include <concepts>

template<typename Callable>
void run_for_each_string_type(const Callable& callable)
{
    callable.template operator()<upp::ascii_string>();
    callable.template operator()<upp::utf8_string>();
    callable.template operator()<upp::utf16_string>();
    callable.template operator()<upp::utf32_string>();
}

TEST_CASE("string type traits", "[string types]")
{
    run_for_each_string_type([&]<typename StringType>() {
        STATIC_CHECK(std::default_initializable<StringType>);
        STATIC_CHECK(std::destructible<StringType>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<StringType>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<StringType>);
    });
}

TEST_CASE("string default constructor", "[string types]")
{
    run_for_each_string_type([&]<typename StringType>() {
        StringType empty;
        StringType with_allocator{std::allocator<typename StringType::code_unit_type>()};
    });
}