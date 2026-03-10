#ifndef UNI_CPP_IMPL_RANGES_BASE_HPP
#define UNI_CPP_IMPL_RANGES_BASE_HPP

/// @file
///
/// @brief Ranges library base and internal macros.
///

#include "../../encoding.hpp"

#include <ranges>

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
    /// @brief Identifies types that are ranges of code units of a given encoding.
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<typename Range, encoding Encoding>
    concept code_unit_range = encoding_traits<Encoding>::template is_code_unit_range<Range>;

    /// @brief Identifies types that are input ranges of code units of a given encoding.
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<typename Range, encoding Encoding>
    concept code_unit_input_range = std::ranges::input_range<Range> && code_unit_range<Range, Encoding>;
} // namespace upp::ranges

#endif // UNI_CPP_IMPL_RANGES_BASE_HPP