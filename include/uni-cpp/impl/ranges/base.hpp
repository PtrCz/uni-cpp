#ifndef UNI_CPP_IMPL_RANGES_BASE_HPP
#define UNI_CPP_IMPL_RANGES_BASE_HPP

/// @file
///
/// @brief Ranges library base and internal macros.
///

#include "../../uchar.hpp"
#include "../../encoding.hpp"

#include "conditional_range_elem.hpp"

#include <cstddef>
#include <ranges>
#include <iterator>
#include <type_traits>
#include <concepts>

#ifndef UNI_CPP_IMPL_DOXYGEN

#define UNI_CPP_IMPL_BEGIN_CPO_NAMESPACE \
    inline namespace impl_cpo            \
    {
#define UNI_CPP_IMPL_END_CPO_NAMESPACE }

#else

#define UNI_CPP_IMPL_BEGIN_CPO_NAMESPACE
#define UNI_CPP_IMPL_END_CPO_NAMESPACE

#endif

// namespace structure

namespace upp
{
    /// Core namespace of the uni-cpp ranges library.
    namespace ranges
    {
        /// ASCII and Unicode range adaptors.
        namespace views
        {
        }
    } // namespace ranges

    namespace views = ranges::views;
} // namespace upp

namespace upp::ranges
{
    /// @brief Identifies types that are input ranges of code units of a given encoding.
    ///
    /// @see @ref upp::ranges::code_unit_range "code_unit_range"
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<typename Range, encoding Encoding>
    concept code_unit_range_for = encoding_traits<Encoding>::template is_code_unit_range<Range>;

    /// @brief Identifies types that are input ranges of code units of some encoding.
    ///
    /// @see @ref upp::ranges::code_unit_range_for "code_unit_range_for"
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<typename Range>
    concept code_unit_range = std::ranges::input_range<Range> && code_unit_type<impl::conditional_range_elem_t<Range>>;

    /// @brief Identifies types that are input ranges of code points (or more precisely, @ref upp::uchar "uchars").
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<typename Range>
    concept code_point_range = std::ranges::input_range<Range> && std::same_as<impl::conditional_range_elem_t<Range>, upp::uchar>;

    namespace impl
    {
        template<bool Const, typename T>
        using maybe_const = std::conditional_t<Const, const T, T>;

        template<typename Range>
        concept range_supports_empty = std::ranges::range<Range> && requires(Range& rg) { std::ranges::empty(rg); };

        /// @brief Buffer index value used in non-forward range adaptors to indicate that the transformation iterator is at the sentinel.
        ///
        /// For non-forward ranges, the underlying iterator being at the sentinel doesn't necessarily mean that the transformation iterator is.
        /// That's because, for non-forward ranges, the underlying iterator is **after** the current elements being transformed.
        /// If the underlying iterator is at the sentinel, the transforming view could be still transforming the last sequence of elements, or it could actually be at the sentinel.
        ///
        /// This value is used to distinguish between those two cases. It signals that the transformation iterator is actually at the sentinel.
        ///
        template<std::signed_integral T>
        inline constexpr T buffer_index_at_sentinel = static_cast<T>(-1);

        template<typename>
        struct input_iterator_category_impl
        {
        };

        template<typename Range>
            requires std::ranges::forward_range<Range>
        struct input_iterator_category_impl<Range>
        {
            using iterator_category = std::input_iterator_tag;
        };

        template<typename Base>
        [[nodiscard]] consteval auto bidirectional_range_iterator_concept_impl() noexcept
        {
            if constexpr (std::ranges::bidirectional_range<Base>)
            {
                return std::bidirectional_iterator_tag{};
            }
            else if constexpr (std::ranges::forward_range<Base>)
            {
                return std::forward_iterator_tag{};
            }
            else if constexpr (std::ranges::input_range<Base>)
            {
                return std::input_iterator_tag{};
            }
        }
    } // namespace impl
} // namespace upp::ranges

#endif // UNI_CPP_IMPL_RANGES_BASE_HPP