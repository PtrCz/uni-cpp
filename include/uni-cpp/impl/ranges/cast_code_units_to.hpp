#ifndef UNI_CPP_IMPL_RANGES_CAST_CODE_UNITS_TO_HPP
#define UNI_CPP_IMPL_RANGES_CAST_CODE_UNITS_TO_HPP

/// @file
///
/// @brief Defines a range adaptor that casts code units from one type to another.
///

#include "base.hpp"
#include "approximately_sized_range.hpp"
#include "view_interface.hpp"
#include "valid_code_unit_range.hpp"
#include "simple_view_adaptor.hpp"

#include "../../encoding.hpp"

#include <iterator>
#include <concepts>
#include <bit>

namespace upp::ranges
{
    namespace impl
    {
        template<typename View, typename ToType>
        struct cast_code_units_to_view_traits
        {
        private:
            using original_code_unit_type = std::remove_cvref_t<std::ranges::range_reference_t<View>>;

        public:
            [[nodiscard]] static constexpr ToType transform_element(const original_code_unit_type code_unit) noexcept
            {
                return std::bit_cast<ToType>(code_unit);
            }

            static constexpr void transform_element(const auto) = delete; // disable implicit conversions
        };
    } // namespace impl

    /// @brief A view that casts code units from the underlying range's element type to `ToType`.
    ///
    /// `std::bit_cast`s each code unit from the underlying view to the target code unit type.
    ///
    /// @tparam View Underlying view type.
    ///
    /// @tparam ToType Target code unit type.
    ///
    /// @note Users should use the @ref upp::views::cast_code_units_to "views::cast_code_units_to" range adaptor instead of using this type directly.
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<std::ranges::view View, code_unit_type ToType>
        requires code_unit_range<View> && (sizeof(ToType) == sizeof(std::remove_cvref_t<std::ranges::range_reference_t<View>>)) &&
                 std::same_as<ToType, std::remove_cv_t<ToType>>
    class cast_code_units_to_view : public impl::simple_view_adaptor<impl::cast_code_units_to_view_traits<View, ToType>, View>
    {
    private:
        using base_t = impl::simple_view_adaptor<impl::cast_code_units_to_view_traits<View, ToType>, View>;

    public:
        using base_t::base_t;

        /// @brief Constructs the `cast_code_units_to_view` from the underlying view.
        ///
        /// Tagged constructor for CTAD.
        ///
        constexpr cast_code_units_to_view(View base, type_tag_t<ToType>)
            : base_t(std::move(base))
        {
        }
    };

    /// @cond

    template<typename Range, typename ToType>
    cast_code_units_to_view(Range&&, type_tag_t<ToType>) -> cast_code_units_to_view<std::views::all_t<Range>, ToType>;

    template<typename View, typename ToType, encoding Encoding>
    inline constexpr bool enable_valid_code_unit_range<cast_code_units_to_view<View, ToType>, Encoding> =
        enable_valid_code_unit_range<std::remove_cvref_t<View>, Encoding>;

    /// @endcond

    namespace impl
    {
        template<code_unit_type ToType>
            requires std::same_as<ToType, std::remove_cv_t<ToType>>
        struct cast_code_units_to_fn : public std::ranges::range_adaptor_closure<cast_code_units_to_fn<ToType>>
        {
        public:
            template<std::ranges::viewable_range Range>
                requires code_unit_range<Range> && (sizeof(ToType) == sizeof(std::remove_cvref_t<std::ranges::range_reference_t<Range>>))
            [[nodiscard]] constexpr auto operator()(Range&& range) const
            {
                if constexpr (std::same_as<ToType, std::remove_cvref_t<std::ranges::range_reference_t<Range>>>)
                {
                    return std::views::all(std::forward<Range>(range));
                }
                else
                {
                    return cast_code_units_to_view(std::forward<Range>(range), type_tag<ToType>);
                }
            }
        };
    } // namespace impl

    namespace views
    {
        /// @brief Casts the code unit type of a code unit range to `ToType`.
        ///
        /// If the element type of the adapted range is `ToType`, it simply returns `std::views::all` of it.
        /// Otherwise, it returns `ranges::cast_code_units_to_view` which `std::bit_cast`s each element from
        /// the underlying range to the target code unit type (`ToType`).
        ///
        /// @tparam ToType Target code unit type. Must have the same size as the code unit type of the adapted range.
        ///
        /// @par Example
        ///
        /// @code{.cpp}
        ///
        /// // `str` is a range of `char`
        /// std::string_view str = ...;
        ///
        /// // `as_char8_t` is a range of `char8_t`
        /// auto as_char8_t = str | upp::views::cast_code_units_to<char8_t>;
        ///
        /// @endcode
        ///
        template<code_unit_type ToType>
            requires std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::cast_code_units_to_fn<ToType> cast_code_units_to{};
    } // namespace views
} // namespace upp::ranges

/// @cond

template<typename View, typename ToType>
inline constexpr bool std::ranges::enable_borrowed_range<upp::ranges::cast_code_units_to_view<View, ToType>> = std::ranges::borrowed_range<View>;

/// @endcond

#endif // UNI_CPP_IMPL_RANGES_CAST_CODE_UNITS_TO_HPP