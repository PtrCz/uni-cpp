#ifndef UNI_CPP_IMPL_UNICODE_DATA_DECOMPOSITION_HPP
#define UNI_CPP_IMPL_UNICODE_DATA_DECOMPOSITION_HPP

#include "../inplace_vector.hpp"

#include "data/decomposition.hpp"

namespace upp
{
    class uchar;
};

namespace upp::impl::unicode_data::decomposition
{
    enum class decomposition_kind : std::uint8_t
    {
        canonical,
        compatibility
    };

    namespace impl
    {
        template<decomposition_kind DecompositionKind>
        [[nodiscard]] constexpr std::uint32_t lookup_value_for_decomposition_kind(const std::uint32_t code_point) noexcept
        {
            static constexpr std::uint64_t mask = 0x7FFFFU;

            if constexpr (DecompositionKind == decomposition_kind::canonical)
                return static_cast<std::uint32_t>(lookup(code_point) & mask);
            else if constexpr (DecompositionKind == decomposition_kind::compatibility)
                return static_cast<std::uint32_t>((lookup(code_point) >> 19U) & mask);
        }
    } // namespace impl

    [[nodiscard]] constexpr bool is_precomposed_hangul_syllable(const std::uint32_t code_point) noexcept
    {
        return code_point >= 0xAC00U && code_point <= 0xD7A3U;
    }

    /// @pre `is_precomposed_hangul_syllable(code_point)` is `true`.
    ///
    template<typename UChar = upp::uchar>
    [[nodiscard]] constexpr inplace_vector<UChar, 18> hangul_syllable_decomposition(const std::uint32_t code_point) noexcept
    {
        static constexpr std::uint32_t s_base  = 0xAC00;
        static constexpr std::uint32_t l_base  = 0x1100;
        static constexpr std::uint32_t v_base  = 0x1161;
        static constexpr std::uint32_t t_base  = 0x11A7;
        static constexpr std::uint32_t t_count = 28;
        static constexpr std::uint32_t n_count = 588;

        const std::uint32_t s_index = code_point - s_base;

        const std::uint32_t l_index = s_index / n_count;
        const std::uint32_t v_index = (s_index % n_count) / t_count;
        const std::uint32_t t_index = s_index % t_count;

        const UChar l_part = UChar::from_unchecked(l_base + l_index);
        const UChar v_part = UChar::from_unchecked(v_base + v_index);

        if (t_index == 0)
        {
            return inplace_vector<UChar, 18>{l_part, v_part};
        }
        else
        {
            const UChar t_part = UChar::from_unchecked(t_base + t_index);

            return inplace_vector<UChar, 18>{l_part, v_part, t_part};
        }
    }

    template<decomposition_kind DecompositionKind, typename UChar = upp::uchar>
    [[nodiscard]] constexpr inplace_vector<UChar, 18> lookup_decomposition(const std::uint32_t code_point) noexcept
    {
        // See `dev/docs/decomposition_tables.md`

        const std::uint32_t encoded_value = impl::lookup_value_for_decomposition_kind<DecompositionKind>(code_point);

        const auto length = static_cast<std::uint8_t>(encoded_value >> 14U);

        if (length == 0U)
        {
            if (is_precomposed_hangul_syllable(code_point))
                return hangul_syllable_decomposition<UChar>(code_point);

            return inplace_vector<UChar, 18>{UChar::from_unchecked(code_point)};
        }

        const auto offset = encoded_value & 0x3FFFU;

        inplace_vector<UChar, 18> decomposition;

        for (std::uint8_t index = 0; index < length; ++index)
        {
            decomposition.unchecked_push_back(UChar::from_unchecked(impl::mappings[offset + index]));
        }

        return decomposition;
    }

    [[nodiscard]] constexpr std::uint8_t lookup_decomposition_type(const std::uint32_t code_point) noexcept
    {
        return static_cast<std::uint8_t>(impl::lookup(code_point) >> 38U);
    }
} // namespace upp::impl::unicode_data::decomposition

#endif // UNI_CPP_IMPL_UNICODE_DATA_DECOMPOSITION_HPP
