#ifndef UNI_CPP_IMPL_SMALL_VECTOR_HPP
#define UNI_CPP_IMPL_SMALL_VECTOR_HPP

#include "gch/small_vector.hpp"

namespace upp::impl
{
    template<typename T, unsigned InlineCapacity, typename Allocator = std::allocator<T>>
    using small_vector = gch::small_vector<T, InlineCapacity, Allocator>;
}

#endif // UNI_CPP_IMPL_SMALL_VECTOR_HPP