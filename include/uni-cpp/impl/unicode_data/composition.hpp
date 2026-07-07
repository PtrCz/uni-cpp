#ifndef UNI_CPP_IMPL_UNICODE_DATA_COMPOSITION_HPP
#define UNI_CPP_IMPL_UNICODE_DATA_COMPOSITION_HPP

#include <optional>

#include "data/composition_mapping.hpp"

namespace upp
{
    class uchar;
}

namespace upp::impl::unicode_data::composition_mapping
{
    template<typename UChar = upp::uchar>
    [[nodiscard]] constexpr std::optional<UChar> composition(const std::uint32_t code_point1, const std::uint32_t code_point2) noexcept
    {
        static constexpr std::uint32_t s_base = 0xAC00;
        static constexpr std::uint32_t l_base = 0x1100;
        static constexpr std::uint32_t v_base = 0x1161;
        static constexpr std::uint32_t t_base = 0x11A7;

        static constexpr std::uint32_t non_null_t_base = t_base + 1;

        static constexpr std::uint32_t l_count = 19;
        static constexpr std::uint32_t v_count = 21;
        static constexpr std::uint32_t t_count = 28;
        static constexpr std::uint32_t n_count = v_count * t_count;
        static constexpr std::uint32_t s_count = l_count * n_count;

        static constexpr std::uint32_t s_end = s_base + s_count;
        static constexpr std::uint32_t l_end = l_base + l_count;
        static constexpr std::uint32_t v_end = v_base + v_count;
        static constexpr std::uint32_t t_end = t_base + t_count;

        static constexpr std::uint32_t no_composition = 0x110000;

        const std::uint64_t key           = (static_cast<std::uint64_t>(code_point2) << 21U) | static_cast<std::uint64_t>(code_point1);
        const std::uint32_t encoded_value = impl::lookup(key);

        if (encoded_value != no_composition)
        {
            return {UChar::from_unchecked(encoded_value)};
        }

        if (l_base <= code_point1 && code_point1 < l_end && v_base <= code_point2 && code_point2 < v_end)
        {
            const std::uint32_t l_index  = code_point1 - l_base;
            const std::uint32_t v_index  = code_point2 - v_base;
            const std::uint32_t lv_index = l_index * n_count + v_index * t_count;

            return {UChar::from_unchecked(s_base + lv_index)};
        }

        if (s_base <= code_point1 && code_point1 < s_end)
        {
            const std::uint32_t s_index = code_point1 - s_base;

            const bool is_hangul_syllable_of_type_lv = s_index % t_count == 0;

            if (is_hangul_syllable_of_type_lv && non_null_t_base <= code_point2 && code_point2 < t_end)
            {
                const std::uint32_t t_index = code_point2 - t_base;

                return {UChar::from_unchecked(code_point1 + t_index)};
            }
        }

        return {};
    }
} // namespace upp::impl::unicode_data::composition_mapping

#endif // UNI_CPP_IMPL_UNICODE_DATA_COMPOSITION_HPP