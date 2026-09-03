#ifndef UNI_CPP_IMPL_RANGES_CONDITIONAL_RANGE_ELEM_T_HPP
#define UNI_CPP_IMPL_RANGES_CONDITIONAL_RANGE_ELEM_T_HPP

#include <ranges>
#include <type_traits>

namespace upp::ranges::impl
{
    template<typename>
    struct conditional_range_elem
    {
        using type = void;
    };

    template<std::ranges::range R>
    struct conditional_range_elem<R>
    {
        using type = std::remove_cvref_t<std::ranges::range_reference_t<R>>;
    };

    template<typename R>
    using conditional_range_elem_t = conditional_range_elem<R>::type;
} // namespace upp::ranges::impl

#endif // UNI_CPP_IMPL_RANGES_CONDITIONAL_RANGE_ELEM_T_HPP