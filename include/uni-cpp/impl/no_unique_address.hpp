#ifndef UNI_CPP_IMPL_NO_UNIQUE_ADDRESS_HPP
#define UNI_CPP_IMPL_NO_UNIQUE_ADDRESS_HPP

#if defined(_MSC_VER) && !defined(__clang__)
#define UNI_CPP_IMPL_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define UNI_CPP_IMPL_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

namespace upp::impl
{
    struct empty_t
    {
    };
} // namespace upp::impl

#endif // UNI_CPP_IMPL_NO_UNIQUE_ADDRESS_HPP