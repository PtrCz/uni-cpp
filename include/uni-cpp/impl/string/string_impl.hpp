#ifndef UNI_CPP_IMPL_STRING_STRING_IMPL_HPP
#define UNI_CPP_IMPL_STRING_STRING_IMPL_HPP

/// @file
///
/// @brief Implementations of string types
///

#include "fwd.hpp"
#include "string.hpp"

#include "../ranges.hpp"
#include "../utf8.hpp"

#include <cstdint>
#include <type_traits>

namespace upp
{
    namespace impl
    {
        class basic_ustring_impl
        {
        public:
            template<unicode_encoding Encoding, typename Container, typename Range>
                requires code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, static_cast<encoding>(Encoding)> &&
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

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, encoding::utf8>
    [[nodiscard]] constexpr std::expected<basic_ustring<E, C>, utf8_error> basic_ustring<E, C>::from_utf8(Range&& range)
    {
        using expected_type = std::expected<basic_ustring<E, C>, utf8_error>;

        if constexpr (E != unicode_encoding::utf8)
        {
            // Transcode.

            basic_ustring result;

            if constexpr (impl::ranges::approximately_sized_range<Range> && reservable_container<C>)
            {
                result.template reserve_for_transcoding_from<unicode_encoding::utf8>(impl::ranges::reserve_hint(range));
            }

            auto expected = impl::utf8::decode_utf8(std::forward<Range>(range), [&](uchar code_point) { result.push_back(code_point); });

            if (!expected.has_value())
            {
                return expected_type{std::unexpect, std::move(expected).error()};
            }

            if constexpr (requires(C& c) { c.shrink_to_fit(); })
            {
                result.shrink_to_fit();
            }

            return expected_type{std::in_place, std::move(result)};
        }
        else if constexpr (std::same_as<C, std::remove_cvref_t<Range>>)
        {
            // Validate. Use the underlying container's move/copy constructor.

            auto expected = impl::utf8::validate_utf8(range);

            if (!expected.has_value())
            {
                return expected_type{std::unexpect, std::move(expected).error()};
            }

            return expected_type{std::in_place, basic_ustring{impl::from_container, std::forward<Range>(range)}};
        }
        else
        {
            // Validate.

            basic_ustring result;

            if constexpr (impl::ranges::approximately_sized_range<Range> && reservable_container<C>)
            {
                result.template reserve_for_transcoding_from<unicode_encoding::utf8>(impl::ranges::reserve_hint(range));
            }

            auto expected =
                impl::utf8::validate_utf8(std::forward<Range>(range), [&](std::uint8_t code_unit) { result.push_back_code_unit(code_unit); });

            if (!expected.has_value())
            {
                return expected_type{std::unexpect, std::move(expected).error()};
            }

            return expected_type{std::in_place, std::move(result)};
        }
    }

    template<unicode_encoding E, string_compatible_container<static_cast<encoding>(E)> C>
    template<std::ranges::input_range Range>
        requires code_unit_type_for<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, encoding::utf8>
    [[nodiscard]] constexpr basic_ustring<E, C> basic_ustring<E, C>::from_utf8_unchecked(Range&& range)
    {
        if constexpr (E == unicode_encoding::utf8)
        {
            return impl::basic_ustring_impl::utfx_from_utfx_unchecked<E, container_type>(std::forward<Range>(range));
        }
        else
        {
            basic_ustring result;

            if constexpr (impl::ranges::approximately_sized_range<Range> && reservable_container<C>)
            {
                result.template reserve_for_transcoding_from<unicode_encoding::utf8>(impl::ranges::reserve_hint(range));
            }

            impl::utf8::decode_utf8_unchecked(std::forward<Range>(range), [&](uchar code_point) { result.push_back(code_point); });

            if constexpr (requires(C& c) { c.shrink_to_fit(); })
            {
                result.shrink_to_fit();
            }

            return result;
        }
    }

    /// @endcond
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_STRING_IMPL_HPP