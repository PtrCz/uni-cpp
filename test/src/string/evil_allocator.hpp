#ifndef TEST_EVIL_ALLOCATOR_HPP
#define TEST_EVIL_ALLOCATOR_HPP

#include <stdexcept>
#include <cstddef>

namespace upp_test
{
    /// Evil allocator type used for testing
    template<typename T, bool ThrowFromConstructor>
    struct evil_allocator
    {
        using value_type      = T;
        using pointer         = T*;
        using const_pointer   = const T*;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;

        template<typename U>
        struct rebind
        {
            using other = evil_allocator<U, ThrowFromConstructor>;
        };

        evil_allocator()
        {
            if constexpr (ThrowFromConstructor)
            {
                throw std::runtime_error("evil_allocator constructor called");
            }
        }
        template<typename U, bool OtherThrowFromConstructor>
        evil_allocator(const evil_allocator<U, OtherThrowFromConstructor>&)
        {
            if constexpr (ThrowFromConstructor)
            {
                throw std::runtime_error("evil_allocator constructor called");
            }
        }

        T*   allocate(std::size_t size) { return new T[size]; }
        void deallocate(T* ptr, std::size_t) { delete[] ptr; }

        bool operator==(const evil_allocator&) const { return true; }
        bool operator!=(const evil_allocator&) const { return false; }
    };
} // namespace upp_test

#endif // TEST_EVIL_ALLOCATOR_HPP