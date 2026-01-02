#ifndef UNI_CPP_IMPL_UTILITY_HPP
#define UNI_CPP_IMPL_UTILITY_HPP

#include <concepts>

namespace upp::impl
{
    template<typename T, typename... Ts>
    concept any_of = (std::same_as<T, Ts> || ...);
} // namespace upp::impl

#endif // UNI_CPP_IMPL_UTILITY_HPP