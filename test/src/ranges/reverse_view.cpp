#include "../catch2.hpp"

#include <uni-cpp/ranges.hpp>

#include "base.hpp"

#include <string_view>

TEST_CASE("reverse_view", "[ranges]")
{
    CONSTEXPR_AND_RUNTIME_TEST()
    {
        using namespace std::string_view_literals;

        CRTT_CHECK(upp_test::ranges::equal(std::ranges::empty_view<int>{} | upp::views::reverse, std::ranges::empty_view<int>{}));
        CRTT_CHECK(upp_test::ranges::equal("hello world"sv | upp::views::reverse, "dlrow olleh"sv));

        const auto sv = "abcdefghij"sv;

        CRTT_CHECK(upp_test::ranges::equal(sv | upp::views::reverse, sv | std::views::reverse));
        CRTT_CHECK(upp_test::ranges::equal(std::ranges::reverse_view{sv | upp::views::reverse}, sv));

        CRTT_CHECK(upp_test::ranges::equal(upp::ranges::reverse_view<upp::ranges::reverse_view<std::string_view>>{sv | upp::views::reverse}, sv));

        auto reversed_sv = sv | upp::views::reverse;

        const auto subrange = std::ranges::subrange{reversed_sv.begin() + 2, reversed_sv.begin() + 5};

        CRTT_CHECK(upp_test::ranges::equal(subrange | upp::views::reverse, "fgh"sv));
    };
}

TEST_CASE("views::reverse", "[ranges]")
{
    STATIC_CHECK(IS_EXPR_OF_TYPE(std::string_view{} | upp::views::reverse, upp::ranges::reverse_view<std::string_view>));

    STATIC_CHECK(IS_EXPR_OF_TYPE(std::string_view{} | upp::views::reverse | upp::views::reverse, std::string_view));
    STATIC_CHECK(IS_EXPR_OF_TYPE(std::string_view{} | std::views::reverse | upp::views::reverse, std::string_view));

    STATIC_CHECK(IS_EXPR_OF_TYPE(([] {
                                     auto reversed_sv = std::string_view{} | upp::views::reverse;

                                     const auto subrange = std::ranges::subrange{reversed_sv.begin(), reversed_sv.end(), 0uz};

                                     return subrange | upp::views::reverse;
                                 }()),
                                 std::ranges::subrange<std::string_view::iterator, std::string_view::iterator, std::ranges::subrange_kind::sized>));

    using decode_view_t = upp::ranges::decode_view<std::string_view, upp::encoding::utf8, upp::ranges::decode_view_kind::lossy, upp::uchar>;

    using decode_view_iterator_t = decltype(std::declval<decode_view_t&>().begin());

    STATIC_CHECK(IS_EXPR_OF_TYPE(([] {
                                     auto reversed = std::string_view{} | upp::views::decode_lossy_utf8 | std::views::reverse;

                                     const auto subrange = std::ranges::subrange{reversed.begin(), reversed.end()};

                                     return subrange | upp::views::reverse;
                                 }()),
                                 std::ranges::subrange<decode_view_iterator_t, decode_view_iterator_t, std::ranges::subrange_kind::unsized>));
}
