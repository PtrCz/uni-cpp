#ifndef UNI_CPP_IMPL_RANGES_TRANSCODE_HPP
#define UNI_CPP_IMPL_RANGES_TRANSCODE_HPP

/// @file
///
/// @brief Defines a range adaptor for transcoding ranges between text encodings.
///

#include "base.hpp"
#include "approximately_sized_range.hpp"
#include "view_interface.hpp"
#include "valid_code_unit_range.hpp"

#include "../../uchar.hpp"
#include "../../encoding.hpp"

#include "../inplace_vector.hpp"

#include <memory>
#include <bit>
#include <expected>

namespace upp::ranges
{
    /// @brief Defines the error-handling policy for @ref upp::ranges::transcode_view "transcode_view" and @ref upp::views::transcode "views::transcode".
    ///
    enum class transcode_view_kind : std::uint8_t
    {
        /// @brief Transcodes valid sequences.
        ///
        /// The underlying view type must satisfy @ref upp::ranges::valid_code_unit_range "valid_code_unit_range".
        ///
        valid,

        /// @brief Propagates transcoding errors using `std::expected`.
        ///
        /// When this is used, the `value_type` of the `transcode_view` range is `std::expected<ToType, error_type>`.
        ///
        expected,

        /// @brief Performs lossy transcoding.
        ///
        /// Replaces invalid sequences with the replacement character.
        ///
        lossy,
    };

    namespace impl::transcode_view_impl
    {
        /// @brief Buffer index value used in non-forward ranges to indicate that the transcoding iterator is at the sentinel.
        ///
        /// For non-forward ranges, the underlying iterator being at the sentinel doesn't necessarily mean that the `transcode_view` iterator is.
        /// That's because, for non-forward ranges, the underlying iterator is **after** the current code units being transcoded.
        /// If the underlying iterator is at the sentinel, the `transcode_view` could be still transcoding the last sequence of code units, or it could actually be at the sentinel.
        ///
        /// This value is used to distinguish between those two cases. It signals that the transcoding iterator is actually at the sentinel.
        ///
        inline constexpr std::int8_t buffer_index_at_sentinel = -1;

        template<typename>
        struct iterator_category_impl
        {
        };

        template<typename Range>
            requires std::ranges::forward_range<Range>
        struct iterator_category_impl<Range>
        {
        private:
            [[nodiscard]] static consteval auto impl() noexcept
            {
                using category = std::iterator_traits<std::ranges::iterator_t<Range>>::iterator_category;

                if constexpr (std::derived_from<category, std::bidirectional_iterator_tag>)
                {
                    return std::bidirectional_iterator_tag{};
                }
                else if constexpr (std::derived_from<category, std::forward_iterator_tag>)
                {
                    return std::forward_iterator_tag{};
                }
                else
                {
                    return category{};
                }
            }

        public:
            using iterator_category = decltype(impl());
        };

        template<typename It>
        struct iterator_guard
        {
            constexpr iterator_guard(It&, It&) {}
        };

        /// @brief RAII guard for iterator rollback of forward iterators.
        ///
        template<typename It>
            requires std::forward_iterator<It>
        struct iterator_guard<It>
        {
            constexpr ~iterator_guard() { current = std::move(original); }

            It& current;
            It  original;
        };
    } // namespace impl::transcode_view_impl

    /// @brief A lazy view that transcodes text to different UTF encodings.
    ///
    /// @tparam View Underlying view type. If the `Kind` template parameter is specified as `valid`, then the `View` type must satisfy
    ///         `valid_code_unit_range<View, SourceEncoding>`.
    ///
    /// @tparam TargetEncoding Must be a UTF encoding. This view cannot transcode to ASCII.
    ///
    /// @tparam Kind Specifies the error handling strategy. Depending on the `Kind` template parameter, the range has a different `value_type`.
    ///         For `Kind == transcode_view_kind::expected`, the `value_type` is `std::expected<ToType, typename encoding_traits<SourceEncoding>::error_type>`.
    ///         Otherwise, the `value_type` is `ToType`.
    ///
    /// @tparam ToType Target code unit type. As an example, if `TargetEncoding` is UTF-16, then this type could be
    ///         `char16_t`, `std::uint16_t`, `[...]`. By default, this type is specified as `typename encoding_traits<TargetEncoding>::default_code_unit_type`.
    ///
    /// @note Users should use the @ref upp::views::transcode "views::transcode" range adaptor family as opposed to using this type directly.
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<std::ranges::view View, encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind,
             code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
        requires unicode_encoding<TargetEncoding> && code_unit_input_range<View, SourceEncoding> &&
                 (Kind != transcode_view_kind::valid || valid_code_unit_range<View, SourceEncoding>) && std::same_as<ToType, std::remove_cv_t<ToType>>
    class transcode_view : public UNI_CPP_IMPL_VIEW_INTERFACE(transcode_view<View, SourceEncoding, TargetEncoding, Kind, ToType>)
    {
    private:
        template<bool>
        class iterator;

        template<bool>
        class sentinel;

    private:
        /// @brief `true` iff the transcoding guarantees that each element from the underlying view results in exactly one output element.
        ///
        static constexpr bool s_preserves_element_count = [] -> bool {
            if constexpr (valid_code_unit_range<View, SourceEncoding> && SourceEncoding == TargetEncoding) // NOLINTNEXTLINE(bugprone-branch-clone)
            {
                // Underlying range is well-formed and we are not transcoding.
                return true;
            }
            else if constexpr (valid_code_unit_range<View, encoding::ascii>)
            {
                // Underlying range is valid ASCII. Valid ASCII code units always map 1:1 to every UTF encoding.
                return true;
            }
            else if constexpr (SourceEncoding == encoding::ascii)
            {
                if constexpr (Kind == transcode_view_kind::lossy)
                {
                    // Lossy transcoding from ASCII.
                    // Valid ASCII code units map to the same amount of code units in a given UTF encoding.
                    // Invalid ASCII code units map to Unicode replacement characters.
                    // In UTF-8, this would expand the output, as the replacement character is 3 code units in UTF-8.
                    // In UTF-16 and UTF-32, this does not affect the range size, as the replacement character is encoded using a single code unit.

                    return TargetEncoding != encoding::utf8;
                }
                else
                {
                    // 1 valid ASCII code unit maps to 1 code unit.
                    // 1 invalid ASCII code unit maps to 1 std::unexpected.
                    return true;
                }
            }
            else if constexpr (SourceEncoding == encoding::utf32 && TargetEncoding == encoding::utf32)
            {
                // 1 valid code unit maps to 1 code unit.
                // 1 invalid code unit maps to 1 replacement character (1 code unit), or 1 std::unexpected.
                return true;
            }

            return false;
        }();

    public:
        /// @brief Default constructor.
        ///
        transcode_view()
            requires std::default_initializable<View>
        = default;

        /// @brief Constructs the `transcode_view` from the underlying view.
        ///
        constexpr explicit transcode_view(View base)
            : m_base(std::move(base))
        {
        }

        /// @brief Constructs the `transcode_view` from the underlying view.
        ///
        /// Tagged constructor for CTAD.
        ///
        constexpr transcode_view(View base, encoding_tag_t<SourceEncoding>, encoding_tag_t<TargetEncoding>, nontype_t<Kind>, ToType)
            : m_base(std::move(base))
        {
        }

        /// @brief Returns a copy of the underlying view.
        ///
        constexpr View base() const&
            requires std::copy_constructible<View>
        {
            return m_base;
        }

        /// @brief Returns the underlying view by moving it.
        ///
        constexpr View base() && { return std::move(m_base); }

        /// @brief Returns an iterator to the beginning of the range.
        ///
        constexpr iterator<false> begin() { return iterator<false>(*this, std::ranges::begin(m_base)); }

        /// @brief Returns an iterator to the beginning of the range.
        ///
        constexpr iterator<true> begin() const
            requires std::ranges::range<const View> && code_unit_input_range<const View, SourceEncoding>
        {
            return iterator<true>(*this, std::ranges::begin(m_base));
        }

        /// @brief Returns a sentinel marking the end of the range.
        ///
        constexpr sentinel<false> end() { return sentinel<false>(std::ranges::end(m_base)); }

        /// @brief Returns an iterator marking the end of the range.
        ///
        constexpr iterator<false> end()
            requires std::ranges::common_range<View>
        {
            return iterator<false>(*this, std::ranges::end(m_base));
        }

        /// @brief Returns a sentinel marking the end of the range.
        ///
        constexpr sentinel<true> end() const
            requires std::ranges::range<const View> && code_unit_input_range<const View, SourceEncoding>
        {
            return sentinel<true>(std::ranges::end(m_base));
        }

        /// @brief Returns an iterator marking the end of the range.
        ///
        constexpr iterator<true> end() const
            requires std::ranges::common_range<const View> && code_unit_input_range<const View, SourceEncoding>
        {
            return iterator<true>(*this, std::ranges::end(m_base));
        }

        /// @brief Checks if the range is empty.
        ///
        constexpr bool empty()
            requires impl::range_supports_empty<View>
        {
            return std::ranges::empty(m_base);
        }

        /// @brief Checks if the range is empty.
        ///
        constexpr bool empty() const
            requires impl::range_supports_empty<const View> && code_unit_input_range<const View, SourceEncoding>
        {
            return std::ranges::empty(m_base);
        }

        /// @brief Returns the size of the range.
        ///
        /// @note This method is only available when the transcoding is guaranteed to preserve the size of the underlying view,
        /// e.g. UTF-32 → UTF-32.
        ///
        constexpr auto size()
            requires std::ranges::sized_range<View> && s_preserves_element_count
        {
            return std::ranges::size(m_base);
        }

        /// @brief Returns the size of the range.
        ///
        /// @note This method is only available when the transcoding is guaranteed to preserve the size of the underlying view,
        /// e.g. UTF-32 → UTF-32.
        ///
        constexpr auto size() const
            requires std::ranges::sized_range<const View> && code_unit_input_range<const View, SourceEncoding> && s_preserves_element_count
        {
            return std::ranges::size(m_base);
        }

        /// @brief Returns an approximate size of the range.
        ///
        constexpr auto reserve_hint()
            requires approximately_sized_range<View>
        {
            return ranges::reserve_hint(m_base);
        }

        /// @brief Returns an approximate size of the range.
        ///
        constexpr auto reserve_hint() const
            requires approximately_sized_range<const View> && code_unit_input_range<const View, SourceEncoding>
        {
            return ranges::reserve_hint(m_base);
        }

    private:
        template<bool Const>
        class iterator : public impl::transcode_view_impl::iterator_category_impl<View>
        {
        private:
            using parent_t = impl::maybe_const<Const, transcode_view>;
            using base_t   = impl::maybe_const<Const, View>;

            using error_type = encoding_traits<SourceEncoding>::error_type;

            [[nodiscard]] static consteval auto iterator_concept_impl() noexcept
            {
                if constexpr (std::ranges::bidirectional_range<base_t>)
                {
                    return std::bidirectional_iterator_tag{};
                }
                else if constexpr (std::ranges::forward_range<base_t>)
                {
                    return std::forward_iterator_tag{};
                }
                else if constexpr (std::ranges::input_range<base_t>)
                {
                    return std::input_iterator_tag{};
                }
            }

        public:
            using iterator_concept = decltype(iterator_concept_impl());

            /// @brief Value type of the iterator.
            ///
            /// Depending on the @p Kind template parameter, the `value_type` is:
            /// - `std::expected<ToType, error_type>` if `Kind == transcode_view_kind::expected`,
            /// - `ToType` otherwise.
            ///
            using value_type      = std::conditional_t<Kind == transcode_view_kind::expected, std::expected<ToType, error_type>, ToType>;
            using reference_type  = value_type;
            using difference_type = std::ptrdiff_t;

        public:
            /// @brief Default constructor.
            ///
            constexpr iterator()
                requires std::default_initializable<std::ranges::iterator_t<View>>
            = default;

            /// @brief Returns a `const` reference to the underlying iterator.
            ///
            /// The underlying iterator is at the beginning of the current code point for forward ranges,
            /// and it is at the beginning of the next code point for non-forward ranges.
            ///
            constexpr const std::ranges::iterator_t<base_t>& base() const& noexcept { return m_current; }

            /// @brief Returns the underlying iterator by moving it.
            ///
            /// The underlying iterator is at the beginning of the current code point for forward ranges,
            /// and it is at the beginning of the next code point for non-forward ranges.
            ///
            constexpr std::ranges::iterator_t<base_t> base() && { return std::move(m_current); }

            /// @brief Dereferences the iterator.
            ///
            constexpr value_type operator*() const
            {
                if constexpr (Kind == transcode_view_kind::expected)
                {
                    if (m_success.has_value())
                    {
                        return value_type{std::in_place, m_buffer[m_buffer_index]};
                    }
                    else
                        return value_type{std::unexpect, m_success.error()};
                }
                else
                    return m_buffer[m_buffer_index];
            }

            /// @brief Advances the output by one element/code unit.
            ///
            constexpr iterator& operator++()
            {
                if constexpr (Kind == transcode_view_kind::expected)
                {
                    if (!m_success)
                    {
                        advance_code_point();
                    }
                    else
                        advance_one();
                }
                else
                    advance_one();

                return *this;
            }

            /// @brief Advances the output by one element/code unit.
            ///
            constexpr auto operator++(int)
            {
                if constexpr (std::is_same_v<iterator_concept, std::input_iterator_tag>)
                {
                    ++*this;
                    return;
                }
                else
                {
                    auto temp = *this;
                    ++*this;
                    return temp;
                }
            }

            /// @brief Advances the output by one element/code unit backwards.
            ///
            constexpr iterator& operator--()
                requires std::ranges::bidirectional_range<base_t>
            {
                if constexpr (Kind == transcode_view_kind::expected)
                {
                    if (!m_success || m_buffer_index == 0)
                    {
                        read_reverse();
                    }
                    else
                        --m_buffer_index;
                }
                else
                {
                    if (m_buffer_index == 0)
                        read_reverse();
                    else
                        --m_buffer_index;
                }

                return *this;
            }

            /// @brief Advances the output by one element/code unit backwards.
            ///
            constexpr iterator operator--(int)
                requires std::ranges::bidirectional_range<base_t>
            {
                auto temp = *this;
                --*this;
                return temp;
            }

            /// @brief Compares two iterators.
            ///
            friend constexpr bool operator==(const iterator& lhs, const iterator& rhs)
                requires std::equality_comparable<std::ranges::iterator_t<base_t>>
            {
                return lhs.m_current == rhs.m_current && lhs.m_buffer_index == rhs.m_buffer_index;
            }

        private:
            constexpr iterator(parent_t& parent, std::ranges::iterator_t<base_t> begin)
                : m_current(std::move(begin))
                , m_parent(std::addressof(parent))
            {
                if (base() != end())
                {
                    read();
                }
                else
                {
                    if constexpr (!std::ranges::forward_range<base_t>)
                    {
                        m_buffer_index = impl::transcode_view_impl::buffer_index_at_sentinel;
                    }
                }
            }

            constexpr std::ranges::iterator_t<base_t> begin() const
                requires std::ranges::bidirectional_range<base_t>
            {
                return std::ranges::begin(m_parent->m_base);
            }

            constexpr std::ranges::sentinel_t<base_t> end() const { return std::ranges::end(m_parent->m_base); }

            /// @brief Updates the buffer with `code_point` encoded in `TargetEncoding`.
            ///
            constexpr void update(uchar code_point)
            {
                m_buffer.clear();

                if constexpr (TargetEncoding == encoding::utf8)
                {
                    for (const char8_t code_unit : code_point.encode_utf8())
                    {
                        m_buffer.unchecked_push_back(std::bit_cast<ToType>(code_unit));
                    }
                }
                else if constexpr (TargetEncoding == encoding::utf16)
                {
                    for (const char16_t code_unit : code_point.encode_utf16())
                    {
                        m_buffer.unchecked_push_back(std::bit_cast<ToType>(code_unit));
                    }
                }
                else if constexpr (TargetEncoding == encoding::utf32)
                {
                    const auto encoded = std::bit_cast<ToType>(code_point.value());

                    m_buffer.unchecked_push_back(encoded);
                }
                else
                    static_assert(false);
            }

            /// @brief Advance to the next code point of the underlying view.
            ///
            constexpr void advance_code_point()
            {
                if constexpr (std::ranges::forward_range<base_t>)
                    std::advance(m_current, m_to_increment);

                if (m_current != end())
                {
                    read();
                }
                else
                {
                    if constexpr (std::ranges::forward_range<base_t>)
                        m_buffer_index = 0;
                    else
                        m_buffer_index = impl::transcode_view_impl::buffer_index_at_sentinel;
                }
            }

            /// @brief Advance by one element/code unit.
            ///
            constexpr void advance_one()
            {
                ++m_buffer_index;

                if (m_buffer_index == static_cast<std::int8_t>(m_buffer.size()))
                {
                    advance_code_point();
                }
            }

            struct read_result
            {
                std::expected<uchar, error_type> decoded;
                std::uint8_t                     to_increment;
            };

            static constexpr read_result read_ascii(std::ranges::iterator_t<base_t>& it)
                requires(SourceEncoding == encoding::ascii)
            {
                using expected_type = std::expected<uchar, error_type>;

                const std::uint8_t code_unit = std::bit_cast<std::uint8_t>(*it);
                ++it;

                if constexpr (valid_code_unit_range<View, encoding::ascii>)
                {
                    const auto code_point = static_cast<std::uint32_t>(code_unit);

                    return {.decoded = expected_type{std::in_place, uchar::from_unchecked(code_point)}, .to_increment = 1};
                }
                else
                {
                    if (is_valid_ascii(code_unit))
                    {
                        const auto code_point = static_cast<std::uint32_t>(code_unit);

                        return {.decoded = expected_type{std::in_place, uchar::from_unchecked(code_point)}, .to_increment = 1};
                    }
                    else
                        return {.decoded = expected_type{std::unexpect, ascii_error{}}, .to_increment = 1};
                }
            }

            static constexpr read_result read_utf8(std::ranges::iterator_t<base_t>& it, const std::ranges::sentinel_t<base_t>& last)
                requires(SourceEncoding == encoding::utf8)
            {
                using expected_type = std::expected<uchar, error_type>;

                std::uint32_t state = upp::impl::utf8::dfa::state::accept;
                std::uint32_t code_point;

                std::size_t index = 0;

                for (; it != last; ++index, ++it)
                {
                    const char8_t code_unit = std::bit_cast<char8_t>(*it);

                    const std::uint32_t type = upp::impl::utf8::dfa::character_class_from_byte[code_unit];

                    code_point =
                        (state != upp::impl::utf8::dfa::state::accept) ? (code_unit & 0x3FU) | (code_point << 6) : (0xFF >> type) & (code_unit);

                    const std::uint32_t previous_state = state;
                    state                              = upp::impl::utf8::dfa::state_transition_table[state + type];

                    if constexpr (!valid_code_unit_range<View, encoding::utf8>)
                    {
                        if (state == upp::impl::utf8::dfa::state::reject)
                        {
                            if (index == 0uz)
                                ++it;

                            const std::size_t invalid_code_units_length = index + 1uz;

                            const std::uint8_t error_length =
                                upp::impl::utf8::get_error_length_from_invalid_code_units_length(invalid_code_units_length);

                            const utf8_error_code error_code = upp::impl::utf8::get_error_code(previous_state, type);

                            return {
                                .decoded =
                                    expected_type{
                                        std::unexpect,
                                        utf8_error{.length = std::optional<std::uint8_t>{std::in_place, error_length}, .code = error_code}
                                    },
                                .to_increment = error_length
                            };
                        }
                    }

                    if (state == upp::impl::utf8::dfa::state::accept)
                    {
                        ++it;
                        return {
                            .decoded      = expected_type{std::in_place, uchar::from_unchecked(code_point)},
                            .to_increment = static_cast<std::uint8_t>(index + 1uz)
                        };
                    }
                }

                return {
                    .decoded =
                        expected_type{
                            std::unexpect,
                            utf8_error{.length = std::optional<std::uint8_t>{std::nullopt}, .code = utf8_error_code::truncated_sequence}
                        },
                    .to_increment = static_cast<std::uint8_t>(index)
                };
            }

            static constexpr read_result read_utf16(std::ranges::iterator_t<base_t>& it, const std::ranges::sentinel_t<base_t>& last)
                requires(SourceEncoding == encoding::utf16)
            {
                using expected_type = std::expected<uchar, error_type>;

                const std::uint16_t first_code_unit = std::bit_cast<std::uint16_t>(*it);
                ++it;

                if (upp::impl::utf16::is_surrogate(first_code_unit))
                {
                    if constexpr (valid_code_unit_range<View, encoding::utf16>)
                    {
                        const std::uint16_t second_code_unit = std::bit_cast<std::uint16_t>(*it);
                        ++it;

                        std::uint32_t code_point = upp::impl::utf16::decode_valid_surrogate_pair(first_code_unit, second_code_unit);

                        return {.decoded = expected_type{std::in_place, uchar::from_unchecked(code_point)}, .to_increment = 2};
                    }
                    else
                    {
                        if (first_code_unit >= 0xDC00U)
                        {
                            return {
                                .decoded =
                                    expected_type{
                                        std::unexpect,
                                        utf16_error{
                                            .length = std::optional<std::uint8_t>{std::in_place, 1}, .code = utf16_error_code::unpaired_low_surrogate
                                        }
                                    },
                                .to_increment = 1
                            };
                        }

                        if (it == last)
                        {
                            return {
                                .decoded =
                                    expected_type{
                                        std::unexpect, utf16_error{.length = {std::nullopt}, .code = utf16_error_code::unpaired_high_surrogate}
                                    },
                                .to_increment = 1
                            };
                        }

                        const std::uint16_t second_code_unit = std::bit_cast<std::uint16_t>(*it);

                        if (second_code_unit < 0xDC00U || second_code_unit > 0xDFFFU)
                        {
                            return {
                                .decoded =
                                    expected_type{
                                        std::unexpect,
                                        utf16_error{
                                            .length = std::optional<std::uint8_t>{std::in_place, 1}, .code = utf16_error_code::unpaired_high_surrogate
                                        }
                                    },
                                .to_increment = 1
                            };
                        }

                        ++it;

                        std::uint32_t code_point = upp::impl::utf16::decode_valid_surrogate_pair(first_code_unit, second_code_unit);

                        return {.decoded = expected_type{std::in_place, uchar::from_unchecked(code_point)}, .to_increment = 2};
                    }
                }
                else
                {
                    return {.decoded = expected_type{std::in_place, uchar::from_unchecked(first_code_unit)}, .to_increment = 1};
                }
            }

            static constexpr read_result read_utf32(std::ranges::iterator_t<base_t>& it)
                requires(SourceEncoding == encoding::utf32)
            {
                using expected_type = std::expected<uchar, error_type>;

                const std::uint32_t code_unit = std::bit_cast<std::uint32_t>(*it);
                ++it;

                if constexpr (valid_code_unit_range<View, encoding::utf32>)
                {
                    return {.decoded = expected_type{std::in_place, uchar::from_unchecked(code_unit)}, .to_increment = 1};
                }
                else
                {
                    if (is_valid_usv(code_unit))
                    {
                        return {.decoded = expected_type{std::in_place, uchar::from_unchecked(code_unit)}, .to_increment = 1};
                    }
                    else
                    {
                        if (code_unit > upp::impl::max_usv)
                            return {.decoded = expected_type{std::unexpect, utf32_error{.code = utf32_error_code::out_of_range}}, .to_increment = 1};
                        else
                            return {
                                .decoded = expected_type{std::unexpect, utf32_error{.code = utf32_error_code::encoded_surrogate}}, .to_increment = 1
                            };
                    }
                }
            }

            /// @brief Updates `m_buffer`, `m_buffer_index` and `m_success` with the decoding result.
            ///
            template<bool Reverse>
            constexpr void process_decoding_result(const std::expected<uchar, error_type>& decoded)
            {
                const auto new_buffer_index = [&] -> std::int8_t {
                    if constexpr (Reverse)
                        return m_buffer.size() - 1;
                    else
                        return 0;
                };

                if constexpr (valid_code_unit_range<View, SourceEncoding>)
                {
                    update(*decoded);
                    m_buffer_index = new_buffer_index();
                }
                else
                {
                    if (decoded.has_value())
                    {
                        update(*decoded);
                        m_buffer_index = new_buffer_index();
                    }
                    else
                    {
                        if constexpr (Kind == transcode_view_kind::expected)
                        {
                            m_buffer_index = 0;
                        }
                        else
                        {
                            update(uchar::replacement_character());
                            m_buffer_index = new_buffer_index();
                        }

                        m_success = std::unexpected<error_type>{std::in_place, decoded.error()};
                    }
                }
            }

            /// @brief Decodes one code point from the underlying sequence.
            ///
            constexpr void read()
            {
                m_success.emplace();

                read_result result{[&] {
                    impl::transcode_view_impl::iterator_guard<std::ranges::iterator_t<base_t>> guard{m_current, m_current};

                    if constexpr (SourceEncoding == encoding::ascii)
                    {
                        return read_ascii(m_current);
                    }
                    else if constexpr (SourceEncoding == encoding::utf8)
                    {
                        return read_utf8(m_current, end());
                    }
                    else if constexpr (SourceEncoding == encoding::utf16)
                    {
                        return read_utf16(m_current, end());
                    }
                    else if constexpr (SourceEncoding == encoding::utf32)
                    {
                        return read_utf32(m_current);
                    }
                    else
                        static_assert(false);
                }()};

                m_to_increment = result.to_increment;

                process_decoding_result<false>(result.decoded);
            }

            constexpr read_result read_reverse_ascii()
                requires std::ranges::bidirectional_range<base_t> && (SourceEncoding == encoding::ascii)
            {
                --m_current;

                auto it{m_current};

                return read_ascii(it);
            }

            constexpr read_result read_reverse_utf8()
                requires std::ranges::bidirectional_range<base_t> && (SourceEncoding == encoding::utf8)
            {
                using expected_type = std::expected<uchar, error_type>;

                auto it{m_current};
                --it;

                std::uint8_t count = 1;

                if constexpr (valid_code_unit_range<View, encoding::utf8>)
                {
                    for (; upp::impl::utf8::is_continuation_byte(std::bit_cast<std::uint8_t>(*it)); ++count)
                        --it;
                }
                else
                {
                    for (; it != begin() && upp::impl::utf8::is_continuation_byte(std::bit_cast<std::uint8_t>(*it)) && count < 4; ++count)
                        --it;

                    if (upp::impl::utf8::is_continuation_byte(std::bit_cast<std::uint8_t>(*it)))
                    {
                        --m_current;
                        return {
                            .decoded =
                                expected_type{
                                    std::unexpect,
                                    utf8_error{
                                        .length = std::optional<std::uint8_t>{std::in_place, 1}, .code = utf8_error_code::unexpected_continuation_byte
                                    }
                                },
                            .to_increment = 1
                        };
                    }

                    std::uint8_t expected_count = upp::impl::utf8::char_width_from_leading_byte(std::bit_cast<std::uint8_t>(*it));

                    const bool is_first_byte_valid = (expected_count != 0);

                    if (!is_first_byte_valid)
                    {
                        const auto error_code = count == 1 ? utf8_error_code::invalid_leading_byte : utf8_error_code::unexpected_continuation_byte;

                        --m_current;
                        return {
                            .decoded =
                                expected_type{std::unexpect, utf8_error{.length = std::optional<std::uint8_t>{std::in_place, 1}, .code = error_code}},
                            .to_increment = 1
                        };
                    }

                    if (count > expected_count)
                    {
                        --m_current;
                        return {
                            .decoded =
                                expected_type{
                                    std::unexpect,
                                    utf8_error{
                                        .length = std::optional<std::uint8_t>{std::in_place, 1}, .code = utf8_error_code::unexpected_continuation_byte
                                    }
                                },
                            .to_increment = 1
                        };
                    }
                }

                auto       leading{it};
                const auto result = read_utf8(it, end());

                if constexpr (valid_code_unit_range<View, encoding::utf8>)
                {
                    m_current = leading;
                    return result;
                }
                else
                {
                    if (result.decoded.has_value() || result.decoded.error().code == utf8_error_code::truncated_sequence)
                    {
                        m_current = leading;
                        return result;
                    }
                    else
                    {
                        --m_current;

                        const auto err = count == 1 ? result.decoded.error()
                                                    : utf8_error{
                                                          .length = std::optional<std::uint8_t>{std::in_place, 1},
                                                          .code   = utf8_error_code::unexpected_continuation_byte
                                                      };

                        return {.decoded = expected_type{std::unexpect, err}, .to_increment = 1};
                    }
                }
            }

            constexpr read_result read_reverse_utf16()
                requires std::ranges::bidirectional_range<base_t> && (SourceEncoding == encoding::utf16)
            {
                using expected_type = std::expected<uchar, error_type>;

                const bool is_at_sentinel = (m_current == end());

                --m_current;
                const std::uint16_t code_unit = std::bit_cast<std::uint16_t>(*m_current);

                if (upp::impl::utf16::is_surrogate(code_unit))
                {
                    if constexpr (valid_code_unit_range<View, encoding::utf16>)
                    {
                        --m_current;
                        const std::uint16_t first_code_unit = std::bit_cast<std::uint16_t>(*m_current);

                        const auto code_point = uchar::from_unchecked(upp::impl::utf16::decode_valid_surrogate_pair(first_code_unit, code_unit));

                        return {.decoded = expected_type{std::in_place, code_point}, .to_increment = 2};
                    }
                    else
                    {
                        if (code_unit < 0xDC00U)
                        {
                            const auto error_length =
                                is_at_sentinel ? std::optional<std::uint8_t>{std::nullopt} : std::optional<std::uint8_t>{std::in_place, 1};

                            return {
                                .decoded =
                                    expected_type{
                                        std::unexpect, utf16_error{.length = error_length, .code = utf16_error_code::unpaired_high_surrogate}
                                    },
                                .to_increment = 1
                            };
                        }

                        if (m_current == begin())
                        {
                            return {
                                .decoded =
                                    expected_type{
                                        std::unexpect,
                                        utf16_error{
                                            .length = std::optional<std::uint8_t>{std::in_place, 1}, .code = utf16_error_code::unpaired_low_surrogate
                                        }
                                    },
                                .to_increment = 1
                            };
                        }

                        --m_current;
                        const std::uint16_t first_code_unit = std::bit_cast<std::uint16_t>(*m_current);

                        if (!upp::impl::utf16::is_high_surrogate(first_code_unit))
                        {
                            ++m_current;

                            return {
                                .decoded =
                                    expected_type{
                                        std::unexpect,
                                        utf16_error{
                                            .length = std::optional<std::uint8_t>{std::in_place, 1}, .code = utf16_error_code::unpaired_low_surrogate
                                        }
                                    },
                                .to_increment = 1
                            };
                        }

                        const auto code_point = uchar::from_unchecked(upp::impl::utf16::decode_valid_surrogate_pair(first_code_unit, code_unit));

                        return {.decoded = expected_type{std::in_place, code_point}, .to_increment = 2};
                    }
                }
                else
                {
                    return {.decoded = expected_type{std::in_place, uchar::from_unchecked(code_unit)}, .to_increment = 1};
                }
            }

            constexpr read_result read_reverse_utf32()
                requires std::ranges::bidirectional_range<base_t> && (SourceEncoding == encoding::utf32)
            {
                --m_current;

                auto it{m_current};

                return read_utf32(it);
            }

            /// @brief Decodes one code point from the underlying sequence backwards.
            ///
            constexpr void read_reverse()
                requires std::ranges::bidirectional_range<base_t>
            {
                m_success.emplace();

                read_result result{[&] {
                    if constexpr (SourceEncoding == encoding::ascii)
                    {
                        return read_reverse_ascii();
                    }
                    else if constexpr (SourceEncoding == encoding::utf8)
                    {
                        return read_reverse_utf8();
                    }
                    else if constexpr (SourceEncoding == encoding::utf16)
                    {
                        return read_reverse_utf16();
                    }
                    else if constexpr (SourceEncoding == encoding::utf32)
                    {
                        return read_reverse_utf32();
                    }
                    else
                        static_assert(false);
                }()};

                m_to_increment = result.to_increment;

                process_decoding_result<true>(result.decoded);
            }

        private:
            std::ranges::iterator_t<base_t> m_current = std::ranges::iterator_t<base_t>();
            parent_t*                       m_parent  = nullptr;

            upp::impl::inplace_vector<ToType, 4 / sizeof(ToType)> m_buffer{};

            std::int8_t  m_buffer_index = 0;
            std::uint8_t m_to_increment = 0; ///< Number of code units to advance the underlying iterator by.

            std::expected<void, error_type> m_success{}; ///< Holds error information about the last read operation.

            template<std::ranges::view View2, encoding SourceEncoding2, encoding TargetEncoding2, transcode_view_kind Kind2,
                     code_unit_type_for<TargetEncoding2> ToType2>
                requires unicode_encoding<TargetEncoding2> && code_unit_input_range<View2, SourceEncoding2> &&
                         (Kind2 != transcode_view_kind::valid || valid_code_unit_range<View2, SourceEncoding2>) &&
                         std::same_as<ToType2, std::remove_cv_t<ToType2>>
            friend class transcode_view;
        };

        template<bool Const>
        class sentinel
        {
        private:
            using parent_t = impl::maybe_const<Const, transcode_view>;
            using base_t   = impl::maybe_const<Const, View>;

            std::ranges::sentinel_t<base_t> m_end = std::ranges::sentinel_t<base_t>();

        public:
            /// @brief Default constructor.
            ///
            sentinel() = default;

            /// @brief Constructs a `const` sentinel from a non-`const` sentinel.
            ///
            constexpr explicit sentinel(sentinel<!Const> i)
                requires Const && std::convertible_to<std::ranges::sentinel_t<View>, std::ranges::sentinel_t<base_t>>
                : m_end{i.m_end}
            {
            }

            /// @brief Returns a copy of the underlying sentinel.
            ///
            constexpr std::ranges::sentinel_t<base_t> base() const { return m_end; }

            /// @brief Compares an iterator with a sentinel.
            ///
            template<bool OtherConst>
                requires std::sentinel_for<std::ranges::sentinel_t<base_t>, std::ranges::iterator_t<impl::maybe_const<OtherConst, View>>>
            friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y)
            {
                if constexpr (std::ranges::forward_range<base_t>)
                {
                    return x.m_current == y.m_end;
                }
                else
                {
                    return x.m_current == y.m_end && x.m_buffer_index == impl::transcode_view_impl::buffer_index_at_sentinel;
                }
            }

        private:
            constexpr explicit sentinel(std::ranges::sentinel_t<base_t> end)
                : m_end{end}
            {
            }

            template<std::ranges::view View2, encoding SourceEncoding2, encoding TargetEncoding2, transcode_view_kind Kind2,
                     code_unit_type_for<TargetEncoding2> ToType2>
                requires unicode_encoding<TargetEncoding2> && code_unit_input_range<View2, SourceEncoding2> &&
                         (Kind2 != transcode_view_kind::valid || valid_code_unit_range<View2, SourceEncoding2>) &&
                         std::same_as<ToType2, std::remove_cv_t<ToType2>>
            friend class transcode_view;
        };

    private:
        View m_base = View();
    };

    /// @cond

    template<typename Range, encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind, typename ToType>
    transcode_view(Range&&, encoding_tag_t<SourceEncoding>, encoding_tag_t<TargetEncoding>, nontype_t<Kind>, ToType)
        -> transcode_view<std::views::all_t<Range>, SourceEncoding, TargetEncoding, Kind, ToType>;

    template<typename View, encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind, typename ToType>
    inline constexpr bool enable_valid_code_unit_range<transcode_view<View, SourceEncoding, TargetEncoding, Kind, ToType>, TargetEncoding> =
        Kind != transcode_view_kind::expected;

    template<typename View, encoding SourceEncoding, transcode_view_kind Kind, typename ToType>
        requires valid_code_unit_range<View, encoding::ascii>
    inline constexpr bool enable_valid_code_unit_range<transcode_view<View, SourceEncoding, encoding::utf8, Kind, ToType>, encoding::ascii> =
        Kind != transcode_view_kind::expected;

    /// @endcond

    namespace impl
    {
        template<encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind, code_unit_type_for<TargetEncoding> ToType>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        struct transcode_utf_fn : public std::ranges::range_adaptor_closure<transcode_utf_fn<SourceEncoding, TargetEncoding, Kind, ToType>>
        {
        public:
            template<std::ranges::viewable_range Range>
                requires code_unit_input_range<Range, SourceEncoding> &&
                         (Kind != transcode_view_kind::valid || valid_code_unit_range<Range, SourceEncoding>)
            [[nodiscard]] constexpr auto operator()(Range&& range) const
            {
                return transcode_view<std::views::all_t<Range>, SourceEncoding, TargetEncoding, Kind, ToType>(
                    std::views::all(std::forward<Range>(range)));
            }
        };
    } // namespace impl

    namespace views
    {
        /// @brief Range adaptor that transcodes code unit ranges to different UTF encodings.
        ///
        /// Produces a lazy view that converts a sequence of code units from a source encoding into a target encoding
        /// using the provided error-handling policy.
        ///
        /// @tparam SourceEncoding The encoding to transcode from.
        /// @tparam TargetEncoding Must be a UTF encoding. This adaptor cannot transcode to ASCII.
        ///
        /// @tparam Kind Specifies the error handling strategy. Depending on the `Kind` template parameter, the resulting range has a different `value_type`.
        ///         For `Kind == transcode_view_kind::expected`, the `value_type` is `std::expected<ToType, typename encoding_traits<SourceEncoding>::error_type>`.
        ///         Otherwise, the `value_type` is `ToType`. If `Kind` is `transcode_view_kind::valid`, then the adapted range must satisfy
        ///         @ref upp::ranges::valid_code_unit_range "valid_code_unit_range<Range, SourceEncoding>". By default, `Kind` is `transcode_view_kind::valid`.
        ///
        /// @tparam ToType Target code unit type. As an example, if `TargetEncoding` is UTF-16, then this type could be
        ///         `char16_t`, `std::uint16_t`, `[...]`. By default, this type is specified as `typename encoding_traits<TargetEncoding>::default_code_unit_type`.
        ///
        /// @see transcode_view_kind
        ///
        template<encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind = transcode_view_kind::valid,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_utf_fn<SourceEncoding, TargetEncoding, Kind, ToType> transcode{};
    } // namespace views
} // namespace upp::ranges

#endif // UNI_CPP_IMPL_RANGES_TRANSCODE_HPP