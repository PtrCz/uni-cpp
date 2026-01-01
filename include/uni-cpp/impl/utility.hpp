#ifndef UNI_CPP_IMPL_UTILITY_HPP
#define UNI_CPP_IMPL_UTILITY_HPP

#include <type_traits>

namespace upp::impl
{
    template<typename T, typename... Ts>
    constexpr bool is_any_of = (std::is_same_v<T, Ts> || ...);
} // namespace upp::impl

#endif // UNI_CPP_IMPL_UTILITY_HPP