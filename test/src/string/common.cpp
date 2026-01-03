#include <catch2/catch_test_macros.hpp>

#include <uni-cpp/string.hpp>

#include <type_traits>
#include <concepts>

TEST_CASE("string type traits", "[string types]")
{
    auto check_type_traits_for = [&]<typename StringType>() {
        STATIC_CHECK(std::default_initializable<StringType>);
        STATIC_CHECK(std::destructible<StringType>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<StringType>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<StringType>);
    };

    check_type_traits_for.template operator()<upp::ascii_string>();
    check_type_traits_for.template operator()<upp::utf8_string>();
    check_type_traits_for.template operator()<upp::utf16_string>();
    check_type_traits_for.template operator()<upp::utf32_string>();
}