#ifndef UNI_CPP_IMPL_UNICODE_DATA_CASE_MAPPING_HPP
#define UNI_CPP_IMPL_UNICODE_DATA_CASE_MAPPING_HPP

#include "../inplace_vector.hpp"

#include "data/case_mapping.hpp"
#include <utility>

namespace upp
{
    class uchar;
};

namespace upp::impl::unicode_data::case_mapping
{
    enum class case_mapping_type : std::uint8_t
    {
        lowercase = 0,
        uppercase = 1,
        titlecase = 2,
        casefold  = 3,
    };

    namespace impl
    {
        template<case_mapping_type MappingType>
        [[nodiscard]] consteval std::uint32_t greatest_code_point_with_mapping() noexcept
        {
            static constexpr std::array values{
                greatest_code_point_with_lowercase_mapping,
                greatest_code_point_with_uppercase_mapping,
                greatest_code_point_with_titlecase_mapping,
                greatest_code_point_with_casefold_mapping,
            };

            return values[std::to_underlying(MappingType)];
        }

        template<case_mapping_type MappingType>
        [[nodiscard]] constexpr std::uint16_t lookup_value_for_mapping_type(const std::uint32_t code_point) noexcept
        {
            const std::uint64_t value = lookup(code_point);

            static constexpr auto bit_offset = 0x10U * std::to_underlying(MappingType);

            return static_cast<std::uint16_t>((value >> bit_offset) & 0xFFFFU);
        }
    } // namespace impl

    template<case_mapping_type MappingType, typename UChar = upp::uchar>
    [[nodiscard]] constexpr inplace_vector<UChar, 3> lookup_case_mapping(const std::uint32_t code_point) noexcept
    {
        // Read `dev/docs/case_mapping_tables.md` to understand this function.

        if (code_point > impl::greatest_code_point_with_mapping<MappingType>())
            return inplace_vector<UChar, 3>{UChar::from_unchecked(code_point)}; // code point maps to itself

        const auto lookup_value = impl::lookup_value_for_mapping_type<MappingType>(code_point);

        // index is in the lower 15-bits and the MSB signifies whether the mapping is special
        const std::uint16_t index = lookup_value & 0x7FFFU;

        if (lookup_value & 0x8000U) // special mapping
        {
            const std::uint64_t special_mapping = impl::special_mappings[index];

            static constexpr std::uint64_t single_code_point_21bit_mask = 0b0001'1111'1111'1111'1111'1111;

            if (special_mapping & (1ULL << 63U)) // MSB is the length bit
            {
                return inplace_vector<UChar, 3>{
                    // clang-format off
                    UChar::from_unchecked(static_cast<std::uint32_t>( special_mapping         & single_code_point_21bit_mask)),
                    UChar::from_unchecked(static_cast<std::uint32_t>((special_mapping >> 21U) & single_code_point_21bit_mask)),
                    UChar::from_unchecked(static_cast<std::uint32_t>((special_mapping >> 42U) & single_code_point_21bit_mask))
                    // clang-format on
                };
            }
            else
            {
                return inplace_vector<UChar, 3>{
                    UChar::from_unchecked(static_cast<std::uint32_t>(special_mapping & single_code_point_21bit_mask)),
                    UChar::from_unchecked(static_cast<std::uint32_t>(special_mapping >> 21U))
                };
            }
        }
        else // simple mapping (1 to 1)
        {
            std::int32_t mapping_offset = impl::simple_mapping_offsets[index];

            if constexpr (MappingType == case_mapping_type::uppercase || MappingType == case_mapping_type::titlecase)
                mapping_offset = -mapping_offset; // uppercase and titlecase mappings use negated offsets

            return inplace_vector<UChar, 3>{UChar::from_unchecked(code_point + mapping_offset)};
        }
    }
} // namespace upp::impl::unicode_data::case_mapping

#endif // UNI_CPP_IMPL_UNICODE_DATA_CASE_MAPPING_HPP