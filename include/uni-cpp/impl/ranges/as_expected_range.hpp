#ifndef UNI_CPP_IMPL_RANGES_AS_EXPECTED_RANGE_HPP
#define UNI_CPP_IMPL_RANGES_AS_EXPECTED_RANGE_HPP

/// @file
///
/// @brief Defines a range adaptor that transforms a range to a range of `std::expected` with the original ranges values.
///

#include "base.hpp"
#include "approximately_sized_range.hpp"
#include "view_interface.hpp"
#include "simple_view_adaptor.hpp"

#include "../../encoding.hpp"

#include <iterator>
#include <expected>

namespace upp::ranges::impl
{
    template<typename View, typename ErrorType>
    struct as_expected_view_traits
    {
    private:
        using range_ref_t = std::ranges::range_reference_t<View>;
        using range_val_t = std::remove_cvref_t<range_ref_t>;

        using expected_t = std::expected<range_val_t, ErrorType>;

    public:
        template<typename T>
            requires std::same_as<range_val_t, std::remove_cvref_t<T>>
        [[nodiscard]] static constexpr expected_t transform_element(T&& value)
        {
            return expected_t{std::in_place, std::forward<T>(value)};
        }
    };

    template<std::ranges::view View, typename ErrorType>
        requires std::ranges::input_range<View> && std::same_as<ErrorType, std::remove_cvref_t<ErrorType>>
    class as_expected_view : public impl::simple_view_adaptor<as_expected_view_traits<View, ErrorType>, View>
    {
    private:
        using base_t = impl::simple_view_adaptor<as_expected_view_traits<View, ErrorType>, View>;

    public:
        using base_t::base_t;

        /// @details Tagged constructor for CTAD.
        ///
        constexpr as_expected_view(View base, type_tag_t<ErrorType>)
            : base_t(std::move(base))
        {
        }
    };

    /// @cond

    template<typename Range, typename ErrorType>
    as_expected_view(Range&&, type_tag_t<ErrorType>) -> as_expected_view<std::views::all_t<Range>, ErrorType>;

    /// @endcond

    template<typename ErrorType>
        requires std::same_as<ErrorType, std::remove_cvref_t<ErrorType>>
    struct as_expected_range_fn : public std::ranges::range_adaptor_closure<as_expected_range_fn<ErrorType>>
    {
    public:
        template<std::ranges::viewable_range Range>
            requires std::ranges::input_range<Range>
        [[nodiscard]] constexpr auto operator()(Range&& range) const
        {
            return as_expected_view(std::forward<Range>(range), type_tag<ErrorType>);
        }
    };

    /// @brief Makes a range of `std::expected` values from the adapted range.
    ///
    /// Each value from the adapted range is mapped to a `std::expected` value with the same value.
    /// The `std::expected` type is `std::expected<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, ErrorType>`.
    ///
    template<typename ErrorType>
        requires std::same_as<ErrorType, std::remove_cvref_t<ErrorType>>
    inline constexpr impl::as_expected_range_fn<ErrorType> as_expected_range{};
} // namespace upp::ranges::impl

/// @cond

template<typename View, typename ErrorType>
inline constexpr bool std::ranges::enable_borrowed_range<upp::ranges::impl::as_expected_view<View, ErrorType>> = std::ranges::borrowed_range<View>;

/// @endcond

#endif // UNI_CPP_IMPL_RANGES_AS_EXPECTED_RANGE_HPP