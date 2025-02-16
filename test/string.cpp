#include <catch2/catch_test_macros.hpp>

#include "uni_cpp/string.hpp"

#include <type_traits>
#include <cstddef>
#include <concepts>

TEST_CASE("", "[upp::ascii_string]")
{
	CHECK(std::is_same_v<upp::ascii_string::allocator_type, std::allocator<char>>);
	CHECK(std::is_same_v<upp::ascii_string::size_type, std::size_t>);
	CHECK(std::is_same_v<upp::ascii_string::difference_type, std::ptrdiff_t>);
	std::string("a", 1, 2);
}