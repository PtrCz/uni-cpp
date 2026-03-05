#ifndef UNI_CPP_IMPL_STRING_STRING_IMPL_HPP
#define UNI_CPP_IMPL_STRING_STRING_IMPL_HPP

/// @file
///
/// @brief Implementations of string types
///

#include "fwd.hpp"
#include "string.hpp"

#include "../ranges.hpp"

#include <cstdint>
#include <type_traits>

namespace upp
{
    namespace impl
    {
        class basic_ustring_impl
        {
        public:
            template<unicode_encoding SourceEncoding, typename ErrorType, unicode_encoding TargetEncoding, typename Container,
                     std::ranges::input_range Range>
                requires code_unit_range<Range, static_cast<encoding>(SourceEncoding)>
            [[nodiscard]] static constexpr std::expected<basic_ustring<TargetEncoding, Container>, ErrorType> from_utf(Range&& range)
            {
                using string_type            = basic_ustring<TargetEncoding, Container>;
                using expected_type          = std::expected<string_type, ErrorType>;
                using traits_type            = encoding_traits<static_cast<encoding>(SourceEncoding)>;
                using default_code_unit_type = traits_type::default_code_unit_type;

                if constexpr (TargetEncoding != SourceEncoding)
                {
                    // Transcode.

                    string_type result;

                    if constexpr (impl::ranges::approximately_sized_range<Range> && reservable_container<Container>)
                    {
                        result.template reserve_for_transcoding_from<SourceEncoding>(impl::ranges::reserve_hint(range));
                    }

                    auto expected = traits_type::decode_range(std::forward<Range>(range), [&](uchar code_point) { result.push_back(code_point); });

                    if (!expected.has_value())
                    {
                        return expected_type{std::unexpect, std::move(expected).error()};
                    }

                    if constexpr (requires(Container& c) { c.shrink_to_fit(); })
                    {
                        result.shrink_to_fit();
                    }

                    return expected_type{std::in_place, std::move(result)};
                }
                else if constexpr (std::same_as<Container, std::remove_cvref_t<Range>>)
                {
                    // Validate. Use the underlying container's copy/move constructor.

                    auto expected = traits_type::validate_range(range);

                    if (!expected.has_value())
                    {
                        return expected_type{std::unexpect, std::move(expected).error()};
                    }

                    return expected_type{std::in_place, string_type{impl::from_container, std::forward<Range>(range)}};
                }
                else
                {
                    // Validate.

                    string_type result;

                    if constexpr (impl::ranges::approximately_sized_range<Range> && reservable_container<Container>)
                    {
                        result.template reserve_for_transcoding_from<SourceEncoding>(impl::ranges::reserve_hint(range));
                    }

                    auto expected = traits_type::validate_range(std::forward<Range>(range),
                                                                [&](default_code_unit_type code_unit) { result.push_back_code_unit(code_unit); });

                    if (!expected.has_value())
                    {
                        return expected_type{std::unexpect, std::move(expected).error()};
                    }

                    return expected_type{std::in_place, std::move(result)};
                }
            }

            template<unicode_encoding SourceEncoding, unicode_encoding TargetEncoding, typename Container, std::ranges::input_range Range>
                requires code_unit_range<Range, static_cast<encoding>(SourceEncoding)>
            [[nodiscard]] static constexpr basic_ustring<TargetEncoding, Container> from_utf_lossy(Range&& range)
            {
                using string_type = basic_ustring<TargetEncoding, Container>;
                using traits_type = encoding_traits<static_cast<encoding>(SourceEncoding)>;

                string_type result;

                if constexpr (impl::ranges::approximately_sized_range<Range> && reservable_container<Container>)
                {
                    result.template reserve_for_transcoding_from<SourceEncoding>(impl::ranges::reserve_hint(range));
                }

                traits_type::decode_range_lossy(std::forward<Range>(range), [&](uchar code_point) { result.push_back(code_point); });

                if constexpr (requires(Container& c) { c.shrink_to_fit(); })
                {
                    result.shrink_to_fit();
                }

                return result;
            }

            template<unicode_encoding SourceEncoding, unicode_encoding TargetEncoding, typename Container, std::ranges::input_range Range>
                requires code_unit_range<Range, static_cast<encoding>(SourceEncoding)>
            [[nodiscard]] static constexpr basic_ustring<TargetEncoding, Container> from_utf_unchecked(Range&& range)
            {
                using string_type = basic_ustring<TargetEncoding, Container>;
                using traits_type = encoding_traits<static_cast<encoding>(SourceEncoding)>;

                if constexpr (TargetEncoding == SourceEncoding)
                {
                    return utfx_from_utfx_unchecked<TargetEncoding, Container>(std::forward<Range>(range));
                }
                else
                {
                    string_type result;

                    if constexpr (impl::ranges::approximately_sized_range<Range> && reservable_container<Container>)
                    {
                        result.template reserve_for_transcoding_from<SourceEncoding>(impl::ranges::reserve_hint(range));
                    }

                    traits_type::decode_range_unchecked(std::forward<Range>(range), [&](uchar code_point) { result.push_back(code_point); });

                    if constexpr (requires(Container& c) { c.shrink_to_fit(); })
                    {
                        result.shrink_to_fit();
                    }

                    return result;
                }
            }

            template<unicode_encoding Encoding, typename Container, typename Range>
                requires code_unit_range<Range, static_cast<encoding>(Encoding)> &&
                         (!std::same_as<Container, std::remove_cvref_t<Range>>) // there is another overload for this case below
            [[nodiscard]] static constexpr basic_ustring<Encoding, Container> utfx_from_utfx_unchecked(Range&& range)
            {
                using result_type = basic_ustring<Encoding, Container>;
                using size_type   = result_type::size_type;

                result_type result;

                if constexpr (ranges::approximately_sized_range<Range> && reservable_container<Container>)
                {
                    result.reserve(static_cast<size_type>(ranges::reserve_hint(range)));
                }

                auto       it       = std::ranges::begin(range);
                const auto sentinel = std::ranges::end(range);

                for (; it != sentinel; ++it)
                    result.push_back_code_unit(*it);

                return result;
            }

            // If Range is the underlying container type, we just construct the string from the container.
            template<unicode_encoding Encoding, typename Container, typename Range>
                requires std::same_as<Container, std::remove_cvref_t<Range>>
            [[nodiscard]] static constexpr basic_ustring<Encoding, Container> utfx_from_utfx_unchecked(Range&& container)
            {
                return {from_container, std::forward<Range>(container)};
            }
        };
    } // namespace impl

    // Doxygen doesn't handle well static method definitions outside of class definitions
    /// @cond

    template<string_compatible_container<encoding::ascii> Container>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::ascii>
    [[nodiscard]] constexpr std::expected<basic_ascii_string<Container>, ascii_error> basic_ascii_string<Container>::from_ascii(Range&& range)
    {
        using expected_type = std::expected<basic_ascii_string<Container>, ascii_error>;

        if constexpr (std::same_as<Container, std::remove_cvref_t<Range>>)
        {
            // Use the underlying container's copy/move constructor.

            auto expected = traits_type::validate_range(range);

            if (!expected.has_value())
            {
                return expected_type{std::unexpect, std::move(expected).error()};
            }

            return expected_type{std::in_place, basic_ascii_string{impl::from_container, std::forward<Range>(range)}};
        }
        else
        {
            basic_ascii_string result;

            if constexpr (impl::ranges::approximately_sized_range<Range> && reservable_container<Container>)
            {
                result.reserve(static_cast<size_type>(impl::ranges::reserve_hint(range)));
            }

            auto expected = traits_type::validate_range(std::forward<Range>(range), [&](char ch) { result.push_back_code_unit(ch); });

            if (!expected.has_value())
            {
                return expected_type{std::unexpect, std::move(expected).error()};
            }

            return expected_type{std::in_place, std::move(result)};
        }
    }

    template<string_compatible_container<encoding::ascii> Container>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::ascii>
    [[nodiscard]] constexpr basic_ascii_string<Container> basic_ascii_string<Container>::from_ascii_lossy(Range&& range)
    {
        basic_ascii_string result;

        if constexpr (impl::ranges::approximately_sized_range<Range> && reservable_container<Container>)
        {
            result.reserve(static_cast<size_type>(impl::ranges::reserve_hint(range)));
        }

        traits_type::decode_range_lossy(std::forward<Range>(range), [&](ascii_char ch) { result.push_back(ch); });

        return result;
    }

    template<string_compatible_container<encoding::ascii> Container>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::ascii>
    [[nodiscard]] constexpr basic_ascii_string<Container> basic_ascii_string<Container>::from_ascii_unchecked(Range&& range)
    {
        if constexpr (std::same_as<Container, std::remove_cvref_t<Range>>)
        {
            return {impl::from_container, std::forward<Range>(range)};
        }
        else
        {
            basic_ascii_string result;

            if constexpr (impl::ranges::approximately_sized_range<Range> && reservable_container<Container>)
            {
                result.reserve(static_cast<size_type>(impl::ranges::reserve_hint(range)));
            }

            auto       it       = std::ranges::begin(range);
            const auto sentinel = std::ranges::end(range);

            for (; it != sentinel; ++it)
                result.push_back_code_unit(*it);

            return result;
        }
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::utf8>
    [[nodiscard]] constexpr std::expected<basic_ustring<E, C>, utf8_error> basic_ustring<E, C>::from_utf8(Range&& range)
    {
        return impl::basic_ustring_impl::from_utf<unicode_encoding::utf8, utf8_error, E, C>(std::forward<Range>(range));
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::utf8>
    [[nodiscard]] constexpr basic_ustring<E, C> basic_ustring<E, C>::from_utf8_lossy(Range&& range)
    {
        return impl::basic_ustring_impl::from_utf_lossy<unicode_encoding::utf8, E, C>(std::forward<Range>(range));
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::utf8>
    [[nodiscard]] constexpr basic_ustring<E, C> basic_ustring<E, C>::from_utf8_unchecked(Range&& range)
    {
        return impl::basic_ustring_impl::from_utf_unchecked<unicode_encoding::utf8, E, C>(std::forward<Range>(range));
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::utf16>
    [[nodiscard]] constexpr std::expected<basic_ustring<E, C>, utf16_error> basic_ustring<E, C>::from_utf16(Range&& range)
    {
        return impl::basic_ustring_impl::from_utf<unicode_encoding::utf16, utf16_error, E, C>(std::forward<Range>(range));
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::utf16>
    [[nodiscard]] constexpr basic_ustring<E, C> basic_ustring<E, C>::from_utf16_lossy(Range&& range)
    {
        return impl::basic_ustring_impl::from_utf_lossy<unicode_encoding::utf16, E, C>(std::forward<Range>(range));
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::utf16>
    [[nodiscard]] constexpr basic_ustring<E, C> basic_ustring<E, C>::from_utf16_unchecked(Range&& range)
    {
        return impl::basic_ustring_impl::from_utf_unchecked<unicode_encoding::utf16, E, C>(std::forward<Range>(range));
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::utf32>
    [[nodiscard]] constexpr std::expected<basic_ustring<E, C>, utf32_error> basic_ustring<E, C>::from_utf32(Range&& range)
    {
        return impl::basic_ustring_impl::from_utf<unicode_encoding::utf32, utf32_error, E, C>(std::forward<Range>(range));
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::utf32>
    [[nodiscard]] constexpr basic_ustring<E, C> basic_ustring<E, C>::from_utf32_lossy(Range&& range)
    {
        return impl::basic_ustring_impl::from_utf_lossy<unicode_encoding::utf32, E, C>(std::forward<Range>(range));
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_range<Range, encoding::utf32>
    [[nodiscard]] constexpr basic_ustring<E, C> basic_ustring<E, C>::from_utf32_unchecked(Range&& range)
    {
        return impl::basic_ustring_impl::from_utf_unchecked<unicode_encoding::utf32, E, C>(std::forward<Range>(range));
    }

    /// @endcond
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_STRING_IMPL_HPP