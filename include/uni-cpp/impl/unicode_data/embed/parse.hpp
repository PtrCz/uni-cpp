#ifndef UNI_CPP_IMPL_UNICODE_DATA_EMBED_PARSE_HPP
#define UNI_CPP_IMPL_UNICODE_DATA_EMBED_PARSE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <concepts>
#include <bit>

namespace upp::impl::unicode_data::embed
{
    template<std::integral T, std::size_t ByteCount>
        requires(ByteCount % sizeof(T) == 0)
    [[nodiscard]] consteval std::array<T, ByteCount / sizeof(T)> parse(const std::array<std::uint8_t, ByteCount>& bytes) noexcept
    {
        constexpr std::size_t count = ByteCount / sizeof(T);

        std::array<T, count> out{};

        for (std::size_t i = 0; i < count; ++i)
        {
            using unsigned_t = std::make_unsigned_t<T>;

            unsigned_t value{};

            const std::size_t base = i * sizeof(T);

            for (std::size_t j = 0; j < sizeof(T); ++j)
            {
                value |= static_cast<unsigned_t>(static_cast<unsigned_t>(bytes[base + j]) << (8U * j));
            }

            out[i] = std::bit_cast<T>(value);
        }

        return out;
    }
} // namespace upp::impl::unicode_data::embed

#endif // UNI_CPP_IMPL_UNICODE_DATA_EMBED_PARSE_HPP