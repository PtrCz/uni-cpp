#ifndef TEST_RANGES_BASE_HPP
#define TEST_RANGES_BASE_HPP

#include <ranges>
#include <concepts>
#include <type_traits>

namespace upp_test
{
    namespace ranges
    {
        namespace views
        {
        }
    } // namespace ranges

    namespace views = ranges::views;
} // namespace upp_test

namespace upp_test::ranges
{
    namespace impl
    {
        struct equal_fn
        {
        public:
            template<std::ranges::input_range R1, std::ranges::input_range R2>
                requires std::same_as<std::remove_cvref_t<std::ranges::range_reference_t<R1>>,
                                      std::remove_cvref_t<std::ranges::range_reference_t<R2>>>
            [[nodiscard]] constexpr bool operator()(R1&& r1, R2&& r2) const
            {
                return std::ranges::equal(std::forward<R1>(r1), std::forward<R2>(r2));
            }
        };
    } // namespace impl

    inline constexpr impl::equal_fn equal{};
} // namespace upp_test::ranges

#endif // TEST_RANGES_BASE_HPP
