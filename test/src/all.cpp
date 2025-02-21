#include <catch2/catch_test_macros.hpp>

#include <uni_cpp/all.hpp>
#include <ranges>
#include <print>
#include <string_view>

TEST_CASE("", "")
{
	using namespace upp::literals;

	for (const auto [index, ch] : std::ranges::enumerate_view(U'\u015A'_uc.encode_utf16()))
	{
		std::println("{}: {:X}", index, static_cast<std::uint64_t>(ch));
	}

	for (const auto upper : U'Ś'_uc.to_uppercase())
	{
		const auto utf8 = upper.encode_utf8();
		std::println("{}", std::string_view(reinterpret_cast<const char*>(utf8.data()), utf8.size()));
	}
}