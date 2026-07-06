#ifndef UNI_CPP_IMPL_UNICODE_DATA_HASH_HPP
#define UNI_CPP_IMPL_UNICODE_DATA_HASH_HPP

#include <cstdint>

namespace upp::impl::unicode_data
{
    [[nodiscard]] constexpr std::uint64_t splitmix64(std::uint64_t x) noexcept
    {
        x += 0x9E3779B97F4A7C15ULL;
        x = (x ^ (x >> 30U)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27U)) * 0x94D049BB133111EBULL;
        return x ^ (x >> 31U);
    }

    [[nodiscard]] constexpr std::uint64_t hash(const std::uint64_t value, const std::uint64_t d) noexcept
    {
        return splitmix64(value ^ splitmix64(d));
    }
} // namespace upp::impl::unicode_data

#endif // UNI_CPP_IMPL_UNICODE_DATA_HASH_HPP