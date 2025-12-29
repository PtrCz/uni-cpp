export module uni_cpp.impl.unicode_data:case_conversion;

export import std;

import :data.case_conversion;

namespace upp::impl::unicode_data::case_conversion
{
    struct case_mapping
    {
        std::array<std::uint32_t, 3> code_points;
        std::uint8_t                 length;

        [[nodiscard]] static constexpr case_mapping single_code_point_mapping(const std::uint32_t code_point) noexcept
        {
            return case_mapping{.code_points = {code_point, 0, 0}, .length = 1};
        }
    };

    export {
        enum class case_mapping_type
        {
            lowercase = 0,
            uppercase = 1,
            titlecase = 2
        };
    }

    template<case_mapping_type MappingType>
    [[nodiscard]] consteval std::uint32_t greatest_code_point_with_mapping() noexcept
    {
        constexpr std::array values{greatest_code_point_with_lowercase_mapping, greatest_code_point_with_uppercase_mapping,
                                    greatest_code_point_with_titlecase_mapping};

        return values[std::to_underlying(MappingType)];
    }

    template<case_mapping_type MappingType>
    [[nodiscard]] consteval const auto& special_mappings() noexcept
    {
        // Note: each case returns a different type

        if constexpr (MappingType == case_mapping_type::lowercase)
            return special_lowercase_mappings;
        else if constexpr (MappingType == case_mapping_type::uppercase)
            return special_uppercase_mappings;
        else if constexpr (MappingType == case_mapping_type::titlecase)
            return special_titlecase_mappings;
    }

    template<case_mapping_type MappingType>
    [[nodiscard]] constexpr std::uint8_t lookup_value_for_mapping_type(const std::uint32_t code_point) noexcept
    {
        const std::uint32_t value = lookup(code_point);

        constexpr auto bit_offset = 8 * std::to_underlying(MappingType);

        return static_cast<std::uint8_t>((value >> bit_offset) & 0xFF);
    }

    export {

        template<case_mapping_type MappingType>
        [[nodiscard]] constexpr case_mapping lookup_case_mapping(const std::uint32_t code_point) noexcept
        {
            // Read `dev/docs/case_conversion_tables.md` to understand this function.

            if (code_point > greatest_code_point_with_mapping<MappingType>())
                return case_mapping::single_code_point_mapping(code_point); // code point maps to itself

            const auto lookup_value = lookup_value_for_mapping_type<MappingType>(code_point);

            // index is in the lower 7-bits and the MSB signifies whether the mapping is special
            const std::uint8_t index = lookup_value & 0b0111'1111;

            if (lookup_value & 0b1000'0000) // special mapping
            {
                const std::uint64_t special_mapping = special_mappings<MappingType>()[index];

                const auto length_bit = (special_mapping & (1ULL << 63)) >> 63; // MSB is the length bit

                constexpr std::uint64_t single_code_point_21bit_mask = 0b0001'1111'1111'1111'1111'1111;

                return case_mapping{
                    // clang-format off
                    .code_points = {
                        static_cast<std::uint32_t>( special_mapping        & single_code_point_21bit_mask),
                        static_cast<std::uint32_t>((special_mapping >> 21) & single_code_point_21bit_mask),
                        static_cast<std::uint32_t>((special_mapping >> 42) & single_code_point_21bit_mask)
                    },
                    // clang-format on
                    .length = static_cast<std::uint8_t>(length_bit + 2) // Add 2 to the length bit to get the mapping's length
                };
            }
            else // simple mapping (1 to 1)
            {
                std::int32_t mapping_offset = simple_mapping_offsets[index];

                if constexpr (MappingType == case_mapping_type::lowercase)
                    mapping_offset = -mapping_offset; // lowercase uses negated offsets

                return case_mapping::single_code_point_mapping(code_point + mapping_offset);
            }
        }
    }
} // namespace upp::impl::unicode_data::case_conversion