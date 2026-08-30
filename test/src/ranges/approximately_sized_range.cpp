#include "../catch2.hpp"

#include <uni-cpp/ranges.hpp>

#include <string_view>

TEST_CASE("upp::ranges::approximately_sized_range concept & upp::ranges::reserve_hint", "[ranges]")
{
    CONSTEXPR_AND_RUNTIME_TEST()
    {
        using namespace std::string_view_literals;

        auto as_utf16 = u8"H\u00E9llo"sv | upp::views::transcode_lossy_utf8_to_utf16;

        CRTT_STATIC_CHECK(!std::ranges::sized_range<decltype(as_utf16)>);
        CRTT_STATIC_CHECK(upp::ranges::approximately_sized_range<decltype(as_utf16)>);

        CRTT_CHECK(std::ranges::distance(as_utf16) == 5);
        CRTT_CHECK(upp::ranges::reserve_hint(as_utf16) == 6);
    };
}