#ifndef TEST_ENCODING_HPP
#define TEST_ENCODING_HPP

#include <uni-cpp/encoding.hpp>

#include "ascii.hpp"
#include "utf.hpp"

#include <ranges>
#include <string>
#include <vector>

namespace upp_test
{
    template<upp::encoding Encoding>
    struct valid_sequence
    {
    private:
        using string_type = std::basic_string<typename upp::encoding_traits<Encoding>::default_code_unit_type>;

    public:
        string_type    sequence;
        std::u8string  as_utf8;
        std::u16string as_utf16;
        std::u32string as_utf32;

        template<upp::encoding TargetEncoding, typename Self>
            requires upp::unicode_encoding<TargetEncoding>
        [[nodiscard]] constexpr auto&& encoded_as(this Self&& self) noexcept
        {
            if constexpr (TargetEncoding == upp::encoding::utf8)
                return std::forward<Self>(self).as_utf8;
            else if constexpr (TargetEncoding == upp::encoding::utf16)
                return std::forward<Self>(self).as_utf16;
            else if constexpr (TargetEncoding == upp::encoding::utf32)
                return std::forward<Self>(self).as_utf32;
        }
    };

    template<upp::encoding Encoding>
    constexpr auto valid_sequences()
    {
        using sequence_t = valid_sequence<Encoding>;

        if constexpr (Encoding == upp::encoding::ascii)
        {
            return upp_test::ascii::valid_sequences();
        }
        else
        {
            auto seqs = upp_test::utf::valid_sequences();

            return seqs | std::views::transform([](const std::ranges::range_value_t<decltype(seqs)>& seq) {
                       return sequence_t{
                           .sequence = seq.template encoded_as<Encoding>(),
                           .as_utf8  = seq.utf8_seq,
                           .as_utf16 = seq.utf16_seq,
                           .as_utf32 = seq.utf32_seq
                       };
                   }) |
                   std::ranges::to<std::vector<sequence_t>>();
        }
    }

    template<upp::encoding Encoding>
    constexpr auto invalid_sequences()
    {
        if constexpr (Encoding == upp::encoding::ascii)
        {
            return upp_test::ascii::invalid_sequences();
        }
        else
        {
            return upp_test::utf::invalid_sequences_for_encoding<Encoding>();
        }
    }
} // namespace upp_test

#endif // TEST_ENCODING_HPP
