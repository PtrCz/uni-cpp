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
#include "cast_code_units_to.hpp"

#include "../../uchar.hpp"
#include "../../encoding.hpp"

#include "../inplace_vector.hpp"
#include "as_expected_range.hpp"

#include <memory>
#include <bit>
#include <type_traits>
#include <expected>

/// @defgroup transcode_view Transcoding code unit ranges
///
/// @brief Provides range adaptors for transcoding between different UTF encodings.
///
/// Example:
///
/// @code{.cpp}
///
/// /// @brief Read a UTF-8 text file as TargetEncoding.
/// ///
/// template<upp::encoding TargetEncoding>
///     requires upp::unicode_encoding<TargetEncoding>
/// auto read_utf8_file_as(std::ifstream& in)
/// {
///     in >> std::noskipws;
///
///     auto src_view = std::ranges::istream_view<char>(in);
///
///     return src_view | upp::views::transcode_lossy_utf8_to<TargetEncoding>;
/// }
///
/// @endcode
///
/// @see transcoding_range_adaptors
/// @see transcode_view_kind
/// @see transcode_view
///
/// @headerfile "" <uni-cpp/ranges.hpp>
///

/// @defgroup decode_view Decoding code unit ranges
///
/// @brief Provides range adaptors for decoding code unit sequences.
///
/// Example:
///
/// @code{.cpp}
///
/// using namespace std::string_view_literals;
///
/// auto utf8_sequence = u8"नमस्ते"sv | upp::views::mark_as_valid_utf8;
///
/// // Iterate code points
///
/// for (upp::uchar code_point : utf8_sequence | upp::views::decode_valid_utf8)
/// {
///     std::print("U+{:04X} ", code_point.value());
/// }
///
/// @endcode
///
/// @see decoding_range_adaptors
/// @see decode_view_kind
/// @see decode_view
///
/// @headerfile "" <uni-cpp/ranges.hpp>
///

namespace upp::ranges
{
    /// @brief Defines the error-handling policy for @ref upp::ranges::transcode_view "transcode_view" and @ref transcoding_range_adaptors "transcoding range adaptors".
    ///
    /// @ingroup transcode_view
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
        /// To determine the number of malformed code units to replace with a single `std::unexpected` value,
        /// it uses the [Substitution of Maximal Subparts](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G66453) specification.
        ///
        expected,

        /// @brief Performs lossy transcoding.
        ///
        /// Replaces invalid sequences with the Unicode replacement character.
        /// To determine the number of malformed code units to replace with a single replacement character,
        /// it uses the [Substitution of Maximal Subparts](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G66453) specification.
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

        struct iterator_type_tag
        {
        };

        template<typename>
        struct get_transcode_view_subrange_info;
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
    /// @note Users should use the @ref transcoding_range_adaptors "transcoding range adaptors" as opposed to using this type directly.
    ///
    /// @ingroup transcode_view
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<std::ranges::view View, encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind,
             code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
        requires unicode_encoding<TargetEncoding> && code_unit_range_for<View, SourceEncoding> &&
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
        constexpr transcode_view(View base, encoding_tag_t<SourceEncoding>, encoding_tag_t<TargetEncoding>, nontype_t<Kind>, type_tag_t<ToType>)
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
            requires std::ranges::range<const View> && code_unit_range_for<const View, SourceEncoding>
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
            requires std::ranges::range<const View> && code_unit_range_for<const View, SourceEncoding>
        {
            return sentinel<true>(std::ranges::end(m_base));
        }

        /// @brief Returns an iterator marking the end of the range.
        ///
        constexpr iterator<true> end() const
            requires std::ranges::common_range<const View> && code_unit_range_for<const View, SourceEncoding>
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
            requires impl::range_supports_empty<const View> && code_unit_range_for<const View, SourceEncoding>
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
            requires std::ranges::sized_range<const View> && code_unit_range_for<const View, SourceEncoding> && s_preserves_element_count
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
            requires approximately_sized_range<const View> && code_unit_range_for<const View, SourceEncoding>
        {
            return ranges::reserve_hint(m_base);
        }

    private:
        template<bool Const>
        class iterator : public impl::transcode_view_impl::iterator_category_impl<View>, private impl::transcode_view_impl::iterator_type_tag
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
                    return {.decoded = uchar::from(code_unit), .to_increment = 1};
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

            template<typename>
            friend struct impl::transcode_view_impl::get_transcode_view_subrange_info;

            // for the implementation of `get_transcode_view_subrange_info`
            static constexpr auto kind            = Kind;
            static constexpr auto source_encoding = SourceEncoding;

            template<std::ranges::view View2, encoding SourceEncoding2, encoding TargetEncoding2, transcode_view_kind Kind2,
                     code_unit_type_for<TargetEncoding2> ToType2>
                requires unicode_encoding<TargetEncoding2> && code_unit_range_for<View2, SourceEncoding2> &&
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
                requires unicode_encoding<TargetEncoding2> && code_unit_range_for<View2, SourceEncoding2> &&
                         (Kind2 != transcode_view_kind::valid || valid_code_unit_range<View2, SourceEncoding2>) &&
                         std::same_as<ToType2, std::remove_cv_t<ToType2>>
            friend class transcode_view;
        };

    private:
        View m_base = View();
    };

    /// @cond

    template<typename Range, encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind, typename ToType>
    transcode_view(Range&&, encoding_tag_t<SourceEncoding>, encoding_tag_t<TargetEncoding>, nontype_t<Kind>, type_tag_t<ToType>)
        -> transcode_view<std::views::all_t<Range>, SourceEncoding, TargetEncoding, Kind, ToType>;

    template<typename View, encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind, typename ToType>
    inline constexpr bool enable_valid_code_unit_range<transcode_view<View, SourceEncoding, TargetEncoding, Kind, ToType>, TargetEncoding> =
        Kind != transcode_view_kind::expected;

    template<typename View, encoding SourceEncoding, transcode_view_kind Kind, typename ToType>
        requires valid_code_unit_range<View, encoding::ascii>
    inline constexpr bool enable_valid_code_unit_range<transcode_view<View, SourceEncoding, encoding::utf8, Kind, ToType>, encoding::ascii> =
        Kind != transcode_view_kind::expected;

    /// @endcond

    /// @brief Defines the error-handling policy for @ref upp::ranges::decode_view "decode_view" and @ref decoding_range_adaptors "decoding range adaptors".
    ///
    /// @ingroup decode_view
    ///
    using decode_view_kind = transcode_view_kind;

    namespace impl::decode_view_impl
    {
        template<typename View, encoding SourceEncoding, decode_view_kind Kind>
        struct variable_width_decoding_traits
        {
        private:
            static_assert(SourceEncoding == encoding::utf8 || SourceEncoding == encoding::utf16);

            using error_type = encoding_traits<SourceEncoding>::error_type;
            using expected_t = std::expected<uchar, error_type>;

            using transcode_view_t = ranges::transcode_view<View, SourceEncoding, encoding::utf32, Kind, std::uint32_t>;

            using iter_t       = std::ranges::iterator_t<transcode_view_t&>;
            using const_iter_t = std::ranges::iterator_t<const transcode_view_t&>;

            using sent_t       = std::ranges::sentinel_t<transcode_view_t&>;
            using const_sent_t = std::ranges::sentinel_t<const transcode_view_t&>;

        public:
            [[nodiscard]] static constexpr expected_t transform_element(std::expected<std::uint32_t, error_type> expected)
                requires(Kind == decode_view_kind::expected)
            {
                return expected.transform([](std::uint32_t code_point) static { return uchar::from_unchecked(code_point); });
            }

            [[nodiscard]] static constexpr uchar transform_element(std::uint32_t code_point)
                requires(Kind != decode_view_kind::expected)
            {
                return uchar::from_unchecked(code_point);
            }

            [[nodiscard]] static constexpr auto base_projection(const transcode_view_t& view) { return view.base(); }
            [[nodiscard]] static constexpr auto base_projection(transcode_view_t&& view) { return std::move(view).base(); }

            [[nodiscard]] static constexpr const auto& iterator_base_projection(const iter_t& it) { return it.base(); }
            [[nodiscard]] static constexpr auto        iterator_base_projection(iter_t&& it) { return std::move(it).base(); }

            [[nodiscard]] static constexpr const auto& iterator_base_projection(const const_iter_t& it) { return it.base(); }
            [[nodiscard]] static constexpr auto        iterator_base_projection(const_iter_t&& it) { return std::move(it).base(); }

            [[nodiscard]] static constexpr auto sentinel_base_projection(const sent_t& sent) { return sent.base(); }
            [[nodiscard]] static constexpr auto sentinel_base_projection(const const_sent_t& sent) { return sent.base(); }
        };

        template<typename View, encoding SourceEncoding, decode_view_kind Kind, typename ToType>
        struct fixed_width_decoding_traits
        {
        private:
            static_assert(SourceEncoding == encoding::ascii || SourceEncoding == encoding::utf32);

            using code_unit_t = std::remove_cvref_t<std::ranges::range_reference_t<View>>;

            using error_type = encoding_traits<SourceEncoding>::error_type;
            using expected_t = std::expected<ToType, error_type>;

        private:
            [[nodiscard]] static constexpr expected_t transform_expected_impl(code_unit_t code_unit)
            {
                if constexpr (SourceEncoding == encoding::ascii)
                {
                    const std::uint8_t ascii_code_unit = std::bit_cast<std::uint8_t>(code_unit);

                    if constexpr (std::same_as<ToType, ascii_char>)
                    {
                        return ascii_char::from(ascii_code_unit);
                    }
                    else
                    {
                        return ascii_char::from(ascii_code_unit).transform([](ascii_char ch) static { return uchar{ch}; });
                    }
                }
                else if constexpr (SourceEncoding == encoding::utf32)
                {
                    return uchar::from(std::bit_cast<std::uint32_t>(code_unit));
                }
                else
                    static_assert(false);
            }

            [[nodiscard]] static constexpr ToType transform_lossy_impl(code_unit_t code_unit)
            {
                if constexpr (SourceEncoding == encoding::ascii)
                {
                    const std::uint8_t ascii_code_unit = std::bit_cast<std::uint8_t>(code_unit);

                    if constexpr (std::same_as<ToType, ascii_char>)
                    {
                        return ascii_char::from_lossy(ascii_code_unit);
                    }
                    else
                    {
                        if (is_valid_ascii(ascii_code_unit))
                        {
                            return uchar::from_unchecked(static_cast<std::uint32_t>(ascii_code_unit));
                        }
                        else
                            return uchar::replacement_character();
                    }
                }
                else if constexpr (SourceEncoding == encoding::utf32)
                {
                    return uchar::from_lossy(std::bit_cast<std::uint32_t>(code_unit));
                }
                else
                    static_assert(false);
            }

            [[nodiscard]] static constexpr ToType transform_valid_impl(code_unit_t code_unit)
            {
                if constexpr (std::same_as<ToType, ascii_char>)
                {
                    return ascii_char::from_unchecked(std::bit_cast<std::uint8_t>(code_unit));
                }
                else if constexpr (std::same_as<ToType, uchar>)
                {
                    if constexpr (SourceEncoding == encoding::ascii)
                    {
                        return uchar::from_unchecked(static_cast<std::uint32_t>(std::bit_cast<std::uint8_t>(code_unit)));
                    }
                    else
                        return uchar::from_unchecked(std::bit_cast<std::uint32_t>(code_unit));
                }
                else
                    static_assert(false);
            }

        public:
            [[nodiscard]] static constexpr expected_t transform_element(code_unit_t code_unit)
                requires(Kind == decode_view_kind::expected)
            {
                if constexpr (valid_code_unit_range<View, SourceEncoding>)
                {
                    return expected_t{std::in_place, transform_valid_impl(code_unit)};
                }
                else
                    return transform_expected_impl(code_unit);
            }

            [[nodiscard]] static constexpr ToType transform_element(code_unit_t code_unit)
                requires(Kind == decode_view_kind::lossy)
            {
                if constexpr (valid_code_unit_range<View, SourceEncoding>)
                {
                    return transform_valid_impl(code_unit);
                }
                else
                    return transform_lossy_impl(code_unit);
            }

            [[nodiscard]] static constexpr ToType transform_element(code_unit_t code_unit)
                requires(Kind == decode_view_kind::valid)
            {
                return transform_valid_impl(code_unit);
            }
        };
    } // namespace impl::decode_view_impl

    /// @brief A lazy view that decodes code units to code points.
    ///
    /// @tparam View Underlying view type. If the `Kind` template parameter is specified as `valid`, then the `View` type must satisfy
    ///         `valid_code_unit_range<View, SourceEncoding>`.
    ///
    /// @tparam SourceEncoding The encoding of the code units from the underlying view.
    ///
    /// @tparam Kind Specifies the error handling strategy. Depending on the `Kind` template parameter, the range has a different `value_type`.
    ///         For `Kind == decode_view_kind::expected`, the `value_type` is `std::expected<ToType, typename encoding_traits<SourceEncoding>::error_type>`.
    ///         Otherwise, the `value_type` is `ToType`.
    ///
    /// @tparam ToType Target code point type. If the source encoding is ASCII, this type can be either `uchar` or `ascii_char`.
    ///         Otherwise, this type must be `uchar`.
    ///
    /// @note Users should use the @ref decoding_range_adaptors "decoding range adaptors" instead of using this type directly.
    ///
    /// @ingroup decode_view
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<std::ranges::view View, encoding SourceEncoding, decode_view_kind Kind, char_type ToType>
        requires code_unit_range_for<View, SourceEncoding> && (Kind != decode_view_kind::valid || valid_code_unit_range<View, SourceEncoding>) &&
                 (std::same_as<ToType, uchar> || SourceEncoding == encoding::ascii)
    class decode_view : public impl::simple_view_adaptor<impl::decode_view_impl::variable_width_decoding_traits<View, SourceEncoding, Kind>,
                                                         transcode_view<View, SourceEncoding, encoding::utf32, Kind, std::uint32_t>>
    {
    private:
        using transcode_view_t = transcode_view<View, SourceEncoding, encoding::utf32, Kind, std::uint32_t>;

        using base_t =
            impl::simple_view_adaptor<impl::decode_view_impl::variable_width_decoding_traits<View, SourceEncoding, Kind>, transcode_view_t>;

        static_assert(SourceEncoding == encoding::utf8 ||
                      SourceEncoding == encoding::utf16); // see partial specializations for ASCII and UTF-32 below

    public:
        /// @brief Default constructor.
        ///
        decode_view()
            requires std::default_initializable<View>
        = default;

        /// @brief Constructs the `decode_view` from the underlying view.
        ///
        constexpr explicit decode_view(View base)
            : base_t(transcode_view_t(std::move(base)))
        {
        }

        /// @brief Constructs the `decode_view` from the underlying view.
        ///
        /// Tagged constructor for CTAD.
        ///
        constexpr decode_view(View base, encoding_tag_t<SourceEncoding>, nontype_t<Kind>, type_tag_t<ToType>)
            : base_t(transcode_view_t(std::move(base)))
        {
        }
    };

    /// @cond

    template<std::ranges::view View, decode_view_kind Kind, char_type ToType>
        requires code_unit_range_for<View, encoding::ascii> && (Kind != decode_view_kind::valid || valid_code_unit_range<View, encoding::ascii>)
    class decode_view<View, encoding::ascii, Kind, ToType>
        : public impl::simple_view_adaptor<impl::decode_view_impl::fixed_width_decoding_traits<View, encoding::ascii, Kind, ToType>, View>
    {
    private:
        using base_t = impl::simple_view_adaptor<impl::decode_view_impl::fixed_width_decoding_traits<View, encoding::ascii, Kind, ToType>, View>;

    public:
        using base_t::base_t;

        constexpr decode_view(View base, encoding_tag_t<encoding::ascii>, nontype_t<Kind>, type_tag_t<ToType>)
            : base_t(std::move(base))
        {
        }
    };

    template<std::ranges::view View, decode_view_kind Kind>
        requires code_unit_range_for<View, encoding::utf32> && (Kind != decode_view_kind::valid || valid_code_unit_range<View, encoding::utf32>)
    class decode_view<View, encoding::utf32, Kind, uchar>
        : public impl::simple_view_adaptor<impl::decode_view_impl::fixed_width_decoding_traits<View, encoding::utf32, Kind, uchar>, View>
    {
    private:
        using base_t = impl::simple_view_adaptor<impl::decode_view_impl::fixed_width_decoding_traits<View, encoding::utf32, Kind, uchar>, View>;

    public:
        using base_t::base_t;

        constexpr decode_view(View base, encoding_tag_t<encoding::utf32>, nontype_t<Kind>, type_tag_t<uchar>)
            : base_t(std::move(base))
        {
        }
    };

    /// @endcond

    /// @cond

    template<typename Range, encoding SourceEncoding, decode_view_kind Kind, typename ToType>
    decode_view(Range&&, encoding_tag_t<SourceEncoding>, nontype_t<Kind>, type_tag_t<ToType>)
        -> decode_view<std::views::all_t<Range>, SourceEncoding, Kind, ToType>;

    /// @endcond

    namespace impl
    {
        namespace transcode_view_impl
        {
            template<typename>
            inline constexpr bool is_empty_view = false;

            template<typename T>
            inline constexpr bool is_empty_view<std::ranges::empty_view<T>> = true;

            template<typename>
            inline constexpr bool is_transcode_view = false;

            template<typename View, encoding SourceEncoding, encoding TargetEncoding, typename ToType, transcode_view_kind Kind>
            inline constexpr bool is_transcode_view<transcode_view<View, SourceEncoding, TargetEncoding, Kind, ToType>> = true;

            template<typename>
            struct get_transcode_view_info;

            template<typename View, encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind, typename ToType>
            struct get_transcode_view_info<transcode_view<View, SourceEncoding, TargetEncoding, Kind, ToType>>
            {
                static constexpr transcode_view_kind kind            = Kind;
                static constexpr encoding            source_encoding = SourceEncoding;
            };

            template<typename It>
            inline constexpr bool is_transcode_view_iterator = std::is_base_of_v<iterator_type_tag, It>;

            template<typename>
            inline constexpr bool is_transcode_view_subrange = false;

            template<typename It>
            inline constexpr bool is_transcode_view_subrange<std::ranges::subrange<It, It, std::ranges::subrange_kind::unsized>> =
                is_transcode_view_iterator<It>;

            template<typename It>
                requires is_transcode_view_iterator<It>
            struct get_transcode_view_subrange_info<std::ranges::subrange<It, It, std::ranges::subrange_kind::unsized>>
            {
                static constexpr transcode_view_kind kind            = It::kind;
                static constexpr encoding            source_encoding = It::source_encoding;
            };
        } // namespace transcode_view_impl

        template<encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind, code_unit_type_for<TargetEncoding> ToType>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        struct transcode_fn : public std::ranges::range_adaptor_closure<transcode_fn<SourceEncoding, TargetEncoding, Kind, ToType>>
        {
        public:
            template<std::ranges::viewable_range Range>
                requires code_unit_range_for<Range, SourceEncoding> &&
                         (Kind != transcode_view_kind::valid || valid_code_unit_range<Range, SourceEncoding>)
            [[nodiscard]] constexpr auto operator()(Range&& range) const
            {
                using range_t    = std::remove_cvref_t<Range>;
                using error_type = encoding_traits<SourceEncoding>::error_type;
                using expected_t = std::expected<ToType, error_type>;

                if constexpr (valid_code_unit_range<Range, TargetEncoding>)
                {
                    if constexpr (Kind == transcode_view_kind::expected)
                    {
                        return std::forward<Range>(range) | views::cast_code_units_to<ToType> | impl::as_expected_range<error_type>;
                    }
                    else
                        return std::forward<Range>(range) | views::cast_code_units_to<ToType>;
                }
                else if constexpr (transcode_view_impl::is_empty_view<range_t>)
                {
                    if constexpr (Kind == transcode_view_kind::expected)
                    {
                        return std::ranges::empty_view<expected_t>{};
                    }
                    else
                        return std::ranges::empty_view<ToType>{};
                }
                else if constexpr (transcode_view_impl::is_transcode_view<range_t>)
                {
                    using range_info = transcode_view_impl::get_transcode_view_info<range_t>;

                    if constexpr (Kind == transcode_view_kind::expected)
                    {
                        return impl::as_expected_range<error_type>(
                            transcode_view(std::forward<Range>(range).base(), encoding_tag<range_info::source_encoding>, encoding_tag<TargetEncoding>,
                                           nontype<range_info::kind>, type_tag<ToType>));
                    }
                    else
                    {
                        return transcode_view(std::forward<Range>(range).base(), encoding_tag<range_info::source_encoding>,
                                              encoding_tag<TargetEncoding>, nontype<range_info::kind>, type_tag<ToType>);
                    }
                }
                else if constexpr (transcode_view_impl::is_transcode_view_subrange<range_t>)
                {
                    using range_info = transcode_view_impl::get_transcode_view_subrange_info<range_t>;

                    if constexpr (Kind == transcode_view_kind::expected)
                    {
                        return impl::as_expected_range<error_type>(
                            transcode_view(std::ranges::subrange(range.begin().base(), range.end().base()), encoding_tag<range_info::source_encoding>,
                                           encoding_tag<TargetEncoding>, nontype<range_info::kind>, type_tag<ToType>));
                    }
                    else
                    {
                        return transcode_view(std::ranges::subrange(range.begin().base(), range.end().base()),
                                              encoding_tag<range_info::source_encoding>, encoding_tag<TargetEncoding>, nontype<range_info::kind>,
                                              type_tag<ToType>);
                    }
                }
                else
                {
                    return transcode_view<std::views::all_t<Range>, SourceEncoding, TargetEncoding, Kind, ToType>(
                        std::views::all(std::forward<Range>(range)));
                }
            }
        };

        template<encoding SourceEncoding, decode_view_kind Kind, char_type ToType>
            requires(std::same_as<ToType, uchar> || SourceEncoding == encoding::ascii)
        struct decode_fn : public std::ranges::range_adaptor_closure<decode_fn<SourceEncoding, Kind, ToType>>
        {
        public:
            template<std::ranges::viewable_range Range>
                requires code_unit_range_for<Range, SourceEncoding> &&
                         (Kind != decode_view_kind::valid || valid_code_unit_range<Range, SourceEncoding>)
            [[nodiscard]] constexpr auto operator()(Range&& range) const
            {
                using range_t    = std::remove_cvref_t<Range>;
                using error_type = encoding_traits<SourceEncoding>::error_type;
                using expected_t = std::expected<ToType, error_type>;

                if constexpr (transcode_view_impl::is_empty_view<range_t>)
                {
                    if constexpr (Kind == decode_view_kind::expected)
                    {
                        return std::ranges::empty_view<expected_t>{};
                    }
                    else
                        return std::ranges::empty_view<ToType>{};
                }
                else if constexpr (transcode_view_impl::is_transcode_view<range_t>)
                {
                    using range_info = transcode_view_impl::get_transcode_view_info<range_t>;

                    static constexpr bool is_valid_ascii_range = valid_code_unit_range<Range, encoding::ascii>;

                    static constexpr encoding decode_view_source_encoding = is_valid_ascii_range ? upp::encoding::ascii : range_info::source_encoding;

                    if constexpr (SourceEncoding == encoding::ascii && !is_valid_ascii_range)
                    {
                        // Cannot optimize this case any further.
                        // Consider `0x80` as an input:
                        //
                        // 0x80 --> transcode_lossy_ascii_to_utf8 --> 0xEF 0xBF 0xBD --> decode_lossy_ascii --> 0x1A 0x1A 0x1A
                        //
                        // and
                        //
                        // 0x80 --> decode_lossy_ascii --> 0x1A
                        //
                        // both produce different results.

                        return decode_view<std::views::all_t<Range>, SourceEncoding, Kind, ToType>(std::views::all(std::forward<Range>(range)));
                    }
                    else if constexpr (Kind == decode_view_kind::expected)
                    {
                        return impl::as_expected_range<error_type>(decode_view(std::forward<Range>(range).base(),
                                                                               encoding_tag<decode_view_source_encoding>, nontype<range_info::kind>,
                                                                               type_tag<ToType>));
                    }
                    else
                    {
                        return decode_view(std::forward<Range>(range).base(), encoding_tag<decode_view_source_encoding>, nontype<range_info::kind>,
                                           type_tag<ToType>);
                    }
                }
                else
                {
                    return decode_view<std::views::all_t<Range>, SourceEncoding, Kind, ToType>(std::views::all(std::forward<Range>(range)));
                }
            }
        };
    } // namespace impl

    namespace views
    {
        /// @addtogroup transcode_view
        /// @{

        /// @defgroup transcoding_range_adaptors Transcoding range adaptors
        ///
        /// @brief Range adaptors for transcoding between UTF encodings.
        ///
        /// Provides a set of lazy range adaptors that transcode between encodings using different error-handling policies.
        ///
        /// There are three kinds of transcoding range adaptors: `valid`, `expected`, and `lossy`. Their meaning
        /// is described in @ref transcode_view_kind.
        ///
        /// There are multiple versions of those adaptors -- some take the source encoding as a template parameter, and other
        /// have the source encoding baked into the adaptors name. The same goes for the target encoding and the error-handling policy.
        ///
        /// There are @ref generic_transcoding_range_adaptors "generic adaptors": `transcode`, `transcode_valid`, `transcode_expected`, `transcode_lossy`;
        /// and pre-instantiated adaptors, such as: `transcode_valid_utf8_to_utf16`, `transcode_lossy_utf16_to_utf32`, `[...]`.
        /// There is also a mix of those two: `transcode_utf8_to`, `transcode_valid_utf16_to`, `[...]`.
        ///
        /// The name format of these adaptors is <code>transcode[_<i>kind</i>][_<i>source-encoding</i>_to[_<i>target-encoding</i>]]</code>
        /// except that the adaptors where the _target-encoding_ is provided but the _kind_ isn't, are not provided.
        /// _kind_ is one of: `valid`, `expected`, `lossy`. _source-encoding_ is one of: `ascii`, `utf8`, `utf16`, `utf32`.
        /// _target-encoding_ is one of: `utf8`, `utf16`, `utf32`.
        ///
        /// Here are a few examples:
        ///
        /// @code{.cpp}
        ///
        /// /// @brief Read a UTF-8 text file as TargetEncoding.
        /// ///
        /// template<upp::encoding TargetEncoding>
        ///     requires upp::unicode_encoding<TargetEncoding>
        /// auto read_utf8_file_as(std::ifstream& in)
        /// {
        ///     in >> std::noskipws;
        ///
        ///     auto src_view = std::ranges::istream_view<char>(in);
        ///
        ///     return src_view | upp::views::transcode_lossy_utf8_to<TargetEncoding>;
        /// }
        ///
        /// @endcode
        ///
        /// @code{.cpp}
        ///
        /// /// @brief Count code points in the given code unit sequence.
        /// ///
        /// template<upp::encoding Encoding, typename Range>
        ///     requires upp::ranges::valid_code_unit_range<Range, Encoding>
        /// std::size_t count_code_points(Range&& code_units)
        /// {
        ///     auto code_points = code_units | upp::views::transcode_valid<Encoding, upp::encoding::utf32>;
        ///
        ///     return std::ranges::distance(code_points);
        /// }
        ///
        /// @endcode
        ///
        /// @code{.cpp}
        ///
        /// /// @pre @p str is valid UTF-8.
        /// ///
        /// void foo(std::string_view str)
        /// {
        ///     // ...
        ///
        ///     #ifdef OS_WINDOWS
        ///
        ///     win32::some_windows_function(
        ///         str
        ///             | upp::views::mark_as_valid_utf8
        ///             | upp::views::transcode_valid_utf8_to_utf16
        ///             | std::ranges::to<std::u16string>()
        ///     );
        ///
        ///     #endif
        ///
        ///     // ...
        /// }
        ///
        /// @endcode
        ///
        /// @{

        /// @defgroup generic_transcoding_range_adaptors Generic transcoding adaptors
        ///
        /// @brief Generic range adaptors that transcode between different UTF encodings.
        ///
        /// There are four of those generic adaptors:
        ///
        /// - `transcode` is the most generic of them all. Not only does it take `SourceEncoding` and `TargetEncoding`
        ///   as a template parameter, it takes an error-handling strategy (`transcode_view_kind`) as well. The target code unit
        ///   type can be specified too (`char` for ASCII, `std::uint32_t` for UTF-32, etc.). The rest of these adaptors
        ///   are a more specific version of this one.
        ///
        /// - `transcode_valid` guarantees that no transcoding errors happen during the transcoding. For this reason, the
        ///   original range of code units must satisfy @ref valid_code_unit_range "valid_code_unit_range<SourceEncoding>".
        ///
        /// - `transcode_expected` has a value type of `std::expected<ToType, error_type>` where `ToType` is the target code unit
        ///   type, and `error_type` is `upp::encoding_traits<SourceEncoding>::error_type`. That is, each element of the range is
        ///   either a code unit or an error value containing details about the error. It can be used to implement a custom
        ///   error-handling policy.
        ///
        /// - `transcode_lossy` is similar to `transcode_valid`, but it doesn't require the original code unit sequence to be well-formed.
        ///   Instead, it replaces ill-formed code unit subsequences with the Unicode replacement character.
        ///
        /// Here is an example of using one of these adaptors in a generic context:
        ///
        /// @code{.cpp}
        ///
        /// /// @brief Count code points in the given code unit sequence.
        /// ///
        /// template<upp::encoding Encoding, typename Range>
        ///     requires upp::ranges::valid_code_unit_range<Range, Encoding>
        /// std::size_t count_code_points(Range&& code_units)
        /// {
        ///     auto code_points = code_units | upp::views::transcode_valid<Encoding, upp::encoding::utf32>;
        ///
        ///     return std::ranges::distance(code_points);
        /// }
        ///
        /// @endcode
        ///
        /// @note These adaptors were made to be used in generic/template contexts. For non-generic contexts,
        /// users might prefer to use the pre-instantiated range adaptors, such as @ref transcode_valid_utf16_to_utf8.
        ///
        /// @{

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
        ///         @ref upp::ranges::valid_code_unit_range "valid_code_unit_range<Range, SourceEncoding>".
        ///
        /// @tparam ToType Target code unit type. As an example, if `TargetEncoding` is UTF-16, then this type could be
        ///         `char16_t`, `std::uint16_t`, `[...]`. By default, this type is specified as `typename encoding_traits<TargetEncoding>::default_code_unit_type`.
        ///
        /// @see transcode_view_kind
        /// @see views::transcode_valid, views::transcode_expected, views::transcode_lossy
        ///
        template<encoding SourceEncoding, encoding TargetEncoding, transcode_view_kind Kind,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<SourceEncoding, TargetEncoding, Kind, ToType> transcode{};

        /// @brief Range adaptor that transcodes well-formed code unit ranges to different UTF encodings.
        ///
        /// Produces a lazy view that transcodes a well-formed code unit sequence from a source encoding into a target encoding.
        ///
        /// When adapting a code unit range using this adaptor, the range must satisfy @ref upp::ranges::valid_code_unit_range "valid_code_unit_range<Range, SourceEncoding>".
        /// The reason for this is to guarantee at the type-system level that the transcoding cannot fail.
        ///
        /// @tparam SourceEncoding The encoding to transcode from.
        /// @tparam TargetEncoding Must be a UTF encoding. This adaptor cannot transcode to ASCII.
        ///
        /// @tparam ToType Target code unit type. As an example, if `TargetEncoding` is UTF-16, then this type could be
        ///         `char16_t`, `std::uint16_t`, `[...]`. By default, this type is specified as `typename encoding_traits<TargetEncoding>::default_code_unit_type`.
        ///
        /// @see views::transcode, views::transcode_expected, views::transcode_lossy
        ///
        template<encoding SourceEncoding, encoding TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<SourceEncoding, TargetEncoding, transcode_view_kind::valid, ToType> transcode_valid{};

        /// @brief Range adaptor that transcodes code unit ranges to different UTF encodings using `std::expected` to propagate transcoding errors.
        ///
        /// Produces a view of `std::expected` values containing either code units in the `TargetEncoding` or errors caused by ill-formed
        /// code unit sequences in the transcoded sequence.
        ///
        /// This adaptor is used over `transcode_lossy` whenever a detailed error description is needed. This can be, for example,
        /// whenever the user wants to use a custom error-handling policy.
        ///
        /// @tparam SourceEncoding The encoding to transcode from.
        /// @tparam TargetEncoding Must be a UTF encoding. This adaptor cannot transcode to ASCII.
        ///
        /// @tparam ToType Target code unit type. As an example, if `TargetEncoding` is UTF-16, then this type could be
        ///         `char16_t`, `std::uint16_t`, `[...]`. By default, this type is specified as `typename encoding_traits<TargetEncoding>::default_code_unit_type`.
        ///
        /// @see views::transcode, views::transcode_valid, views::transcode_lossy
        ///
        template<encoding SourceEncoding, encoding TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<SourceEncoding, TargetEncoding, transcode_view_kind::expected, ToType> transcode_expected{};

        /// @brief Range adaptor that transcodes code unit ranges to different UTF encoding replacing ill-formed subsequences with U+FFFD.
        ///
        /// Produces a lazy view that transcodes potentially-ill-formed code unit sequences from one encoding to another.
        ///
        /// It replaces ill-formed code unit sequences with the Unicode replacement character (U+FFFD).
        /// It uses the [Substitution of Maximal Subparts](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G66453) specification
        /// to determine the number of malformed code units to replace with a single replacement character.
        ///
        /// @tparam SourceEncoding The encoding to transcode from.
        /// @tparam TargetEncoding Must be a UTF encoding. This adaptor cannot transcode to ASCII.
        ///
        /// @tparam ToType Target code unit type. As an example, if `TargetEncoding` is UTF-16, then this type could be
        ///         `char16_t`, `std::uint16_t`, `[...]`. By default, this type is specified as `typename encoding_traits<TargetEncoding>::default_code_unit_type`.
        ///
        /// @see views::transcode, views::transcode_valid, views::transcode_expected
        ///
        template<encoding SourceEncoding, encoding TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<SourceEncoding, TargetEncoding, transcode_view_kind::lossy, ToType> transcode_lossy{};

        /// @}

        /// @defgroup ascii_transcoding_range_adaptors ASCII transcoding adaptors
        ///
        /// @brief Range adaptors that transcode ASCII to UTF encodings using the specified error-handling policy.
        ///
        /// See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        ///
        /// @{

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding TargetEncoding, transcode_view_kind Kind,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::ascii, TargetEncoding, Kind, ToType> transcode_ascii_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::ascii, TargetEncoding, transcode_view_kind::valid, ToType> transcode_valid_ascii_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::ascii, TargetEncoding, transcode_view_kind::expected, ToType> transcode_expected_ascii_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::ascii, TargetEncoding, transcode_view_kind::lossy, ToType> transcode_lossy_ascii_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::ascii, encoding::utf8, transcode_view_kind::valid, char8_t> transcode_valid_ascii_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::ascii, encoding::utf16, transcode_view_kind::valid, char16_t> transcode_valid_ascii_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::ascii, encoding::utf32, transcode_view_kind::valid, char32_t> transcode_valid_ascii_to_utf32{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::ascii, encoding::utf8, transcode_view_kind::expected, char8_t>
            transcode_expected_ascii_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::ascii, encoding::utf16, transcode_view_kind::expected, char16_t>
            transcode_expected_ascii_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::ascii, encoding::utf32, transcode_view_kind::expected, char32_t>
            transcode_expected_ascii_to_utf32{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::ascii, encoding::utf8, transcode_view_kind::lossy, char8_t> transcode_lossy_ascii_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::ascii, encoding::utf16, transcode_view_kind::lossy, char16_t> transcode_lossy_ascii_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::ascii, encoding::utf32, transcode_view_kind::lossy, char32_t> transcode_lossy_ascii_to_utf32{};

        /// @}

        /// @defgroup utf8_transcoding_range_adaptors UTF-8 transcoding adaptors
        ///
        /// @brief Range adaptors that transcode UTF-8 to different UTF encodings using the specified error-handling policy.
        ///
        /// See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        ///
        /// @{

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding TargetEncoding, transcode_view_kind Kind,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf8, TargetEncoding, Kind, ToType> transcode_utf8_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf8, TargetEncoding, transcode_view_kind::valid, ToType> transcode_valid_utf8_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf8, TargetEncoding, transcode_view_kind::expected, ToType> transcode_expected_utf8_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf8, TargetEncoding, transcode_view_kind::lossy, ToType> transcode_lossy_utf8_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf8, encoding::utf8, transcode_view_kind::valid, char8_t> transcode_valid_utf8_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf8, encoding::utf16, transcode_view_kind::valid, char16_t> transcode_valid_utf8_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf8, encoding::utf32, transcode_view_kind::valid, char32_t> transcode_valid_utf8_to_utf32{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf8, encoding::utf8, transcode_view_kind::expected, char8_t> transcode_expected_utf8_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf8, encoding::utf16, transcode_view_kind::expected, char16_t>
            transcode_expected_utf8_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf8, encoding::utf32, transcode_view_kind::expected, char32_t>
            transcode_expected_utf8_to_utf32{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf8, encoding::utf8, transcode_view_kind::lossy, char8_t> transcode_lossy_utf8_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf8, encoding::utf16, transcode_view_kind::lossy, char16_t> transcode_lossy_utf8_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf8, encoding::utf32, transcode_view_kind::lossy, char32_t> transcode_lossy_utf8_to_utf32{};

        /// @}

        /// @defgroup utf16_transcoding_range_adaptors UTF-16 transcoding adaptors
        ///
        /// @brief Range adaptors that transcode UTF-16 to different UTF encodings using the specified error-handling policy.
        ///
        /// See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        ///
        /// @{

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding TargetEncoding, transcode_view_kind Kind,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf16, TargetEncoding, Kind, ToType> transcode_utf16_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf16, TargetEncoding, transcode_view_kind::valid, ToType> transcode_valid_utf16_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf16, TargetEncoding, transcode_view_kind::expected, ToType> transcode_expected_utf16_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf16, TargetEncoding, transcode_view_kind::lossy, ToType> transcode_lossy_utf16_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf16, encoding::utf8, transcode_view_kind::valid, char8_t> transcode_valid_utf16_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf16, encoding::utf16, transcode_view_kind::valid, char16_t> transcode_valid_utf16_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf16, encoding::utf32, transcode_view_kind::valid, char32_t> transcode_valid_utf16_to_utf32{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf16, encoding::utf8, transcode_view_kind::expected, char8_t>
            transcode_expected_utf16_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf16, encoding::utf16, transcode_view_kind::expected, char16_t>
            transcode_expected_utf16_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf16, encoding::utf32, transcode_view_kind::expected, char32_t>
            transcode_expected_utf16_to_utf32{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf16, encoding::utf8, transcode_view_kind::lossy, char8_t> transcode_lossy_utf16_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf16, encoding::utf16, transcode_view_kind::lossy, char16_t> transcode_lossy_utf16_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf16, encoding::utf32, transcode_view_kind::lossy, char32_t> transcode_lossy_utf16_to_utf32{};

        /// @}

        /// @defgroup utf32_transcoding_range_adaptors UTF-32 transcoding adaptors
        ///
        /// @brief Range adaptors that transcode UTF-32 to different UTF encodings using the specified error-handling policy.
        ///
        /// See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        ///
        /// @{

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding TargetEncoding, transcode_view_kind Kind,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf32, TargetEncoding, Kind, ToType> transcode_utf32_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf32, TargetEncoding, transcode_view_kind::valid, ToType> transcode_valid_utf32_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf32, TargetEncoding, transcode_view_kind::expected, ToType> transcode_expected_utf32_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        template<encoding                           TargetEncoding,
                 code_unit_type_for<TargetEncoding> ToType = typename encoding_traits<TargetEncoding>::default_code_unit_type>
            requires unicode_encoding<TargetEncoding> && std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::transcode_fn<encoding::utf32, TargetEncoding, transcode_view_kind::lossy, ToType> transcode_lossy_utf32_to{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf32, encoding::utf8, transcode_view_kind::valid, char8_t> transcode_valid_utf32_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf32, encoding::utf16, transcode_view_kind::valid, char16_t> transcode_valid_utf32_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf32, encoding::utf32, transcode_view_kind::valid, char32_t> transcode_valid_utf32_to_utf32{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf32, encoding::utf8, transcode_view_kind::expected, char8_t>
            transcode_expected_utf32_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf32, encoding::utf16, transcode_view_kind::expected, char16_t>
            transcode_expected_utf32_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf32, encoding::utf32, transcode_view_kind::expected, char32_t>
            transcode_expected_utf32_to_utf32{};

        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf32, encoding::utf8, transcode_view_kind::lossy, char8_t> transcode_lossy_utf32_to_utf8{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf32, encoding::utf16, transcode_view_kind::lossy, char16_t> transcode_lossy_utf32_to_utf16{};
        /// @details See @ref transcoding_range_adaptors "Transcoding Range Adaptors documentation".
        inline constexpr impl::transcode_fn<encoding::utf32, encoding::utf32, transcode_view_kind::lossy, char32_t> transcode_lossy_utf32_to_utf32{};

        /// @}
        /// @}
        /// @}

        /// @addtogroup decode_view
        /// @{

        /// @defgroup decoding_range_adaptors Decoding range adaptors
        ///
        /// @brief Range adaptors for decoding code units into code points.
        ///
        /// Provides a set of lazy range adaptors that decode code units to code points using different error-handling policies.
        ///
        /// There are three kinds of decoding range adaptors: `valid`, `expected`, and `lossy`. Their meaning
        /// is described in @ref decode_view_kind.
        ///
        /// There are multiple versions of those adaptors -- some take the source encoding as a template parameter, and other
        /// have the source encoding baked into the adaptors name. The same goes for the error-handling policy.
        ///
        /// There are @ref generic_decoding_range_adaptors "generic adaptors": `decode`, `decode_valid`, `decode_expected`, `decode_lossy`;
        /// and pre-instantiated adaptors, such as: `decode_valid_utf8`, `decode_lossy_utf16`, `decode_valid_ascii_to_uchars`, `[...]`.
        ///
        /// For decoding ASCII, the code point type can be specified as either `ascii_char` or `uchar`. The pre-instantiated adaptors
        /// for ASCII reflect this --- there are <code>decode_<i>kind</i>_ascii</code> adaptors which decode to `ascii_char`s, and there are
        /// <code>decode_<i>kind</i>_ascii_to_uchars</code> adaptors which decode to `uchar`s.
        ///
        /// The lossy decoding uses a different fallback character depending on the target code point type specified.
        /// It uses `uchar::replacement_character()` for `uchar` and `ascii_char::substitute_character()` for `ascii_char`.
        ///
        /// Here is an example:
        ///
        /// @code{.cpp}
        ///
        /// using namespace std::string_view_literals;
        ///
        /// auto utf8_sequence = u8"नमस्ते"sv | upp::views::mark_as_valid_utf8;
        ///
        /// // Iterate code points
        ///
        /// for (upp::uchar code_point : utf8_sequence | upp::views::decode_valid_utf8)
        /// {
        ///     std::print("U+{:04X} ", code_point.value());
        /// }
        ///
        /// @endcode
        ///
        /// @{

        /// @defgroup generic_decoding_range_adaptors Generic decoding adaptors
        ///
        /// @brief Generic range adaptors that decode ranges of code units.
        ///
        /// There are four of those generic adaptors:
        ///
        /// - `decode` is the most generic of them all. It takes the `SourceEncoding` and the error-handling strategy (`decode_view_kind`)
        ///   as template parameters. The target code point type can be specified too. The rest of these adaptors
        ///   are a more specific version of this one.
        ///
        /// - `decode_valid` guarantees that no decoding errors happen during the decoding. For this reason, the
        ///   original range of code units must satisfy @ref valid_code_unit_range "valid_code_unit_range<SourceEncoding>".
        ///
        /// - `decode_expected` has a value type of `std::expected<ToType, error_type>` where `ToType` is the target code point
        ///   type, and `error_type` is `upp::encoding_traits<SourceEncoding>::error_type`. That is, each element of the range is
        ///   either a code point or an error value containing details about the error. It can be used to implement a custom
        ///   error-handling policy.
        ///
        /// - `decode_lossy` is similar to `decode_valid`, but it doesn't require the original code unit sequence to be well-formed.
        ///   Instead, it replaces ill-formed code unit subsequences with a fallback character.
        ///
        /// @{

        /// @brief Range adaptor that decodes code units to code points (`uchar`s or `ascii_char`s).
        ///
        /// Produces a lazy view that converts a sequence of code units from a source encoding into a sequence
        /// of code points using the provided error-handling policy.
        ///
        /// When decoding a UTF encoding, the code point type (`ToType`) is `uchar`. When decoding ASCII,
        /// the code point type (`ToType`) can be specified as either `uchar` or `ascii_char`.
        ///
        /// @tparam SourceEncoding The encoding to decode from.
        ///
        /// @tparam Kind Specifies the error handling strategy. Depending on the `Kind` template parameter, the resulting range has a different `value_type`.
        ///         For `Kind == decode_view_kind::expected`, the `value_type` is `std::expected<ToType, typename encoding_traits<SourceEncoding>::error_type>`.
        ///         Otherwise, the `value_type` is `ToType`. If `Kind` is `decode_view_kind::valid`, then the adapted range must satisfy
        ///         @ref upp::ranges::valid_code_unit_range "valid_code_unit_range<Range, SourceEncoding>".
        ///
        /// @tparam ToType Code point type to decode to. If the source encoding is ASCII, this type can be either `uchar` or `ascii_char`.
        ///         Otherwise, this type must be `uchar`. By default, this type is specified as `uchar`.
        ///
        /// @see decode_view_kind
        /// @see views::decode_valid, views::decode_expected, views::decode_lossy
        ///
        template<encoding SourceEncoding, decode_view_kind Kind, char_type ToType = uchar>
            requires(std::same_as<ToType, uchar> || SourceEncoding == encoding::ascii)
        inline constexpr impl::decode_fn<SourceEncoding, Kind, ToType> decode{};

        /// @brief Range adaptor that decodes well-formed code unit ranges to code points.
        ///
        /// Produces a lazy view that decodes a well-formed code unit sequence into a sequence of code points.
        ///
        /// When adapting a code unit range using this adaptor, the range must satisfy @ref upp::ranges::valid_code_unit_range "valid_code_unit_range<Range, SourceEncoding>".
        /// The reason for this is to guarantee at the type-system level that the decoding cannot fail.
        ///
        /// @tparam SourceEncoding The encoding to decode from.
        ///
        /// @tparam ToType Code point type to decode to. If the source encoding is ASCII, this type can be either `uchar` or `ascii_char`.
        ///         Otherwise, this type must be `uchar`. By default, this type is specified as `uchar`.
        ///
        /// @see views::decode, views::decode_expected, views::decode_lossy
        ///
        template<encoding SourceEncoding, char_type ToType = uchar>
            requires(std::same_as<ToType, uchar> || SourceEncoding == encoding::ascii)
        inline constexpr impl::decode_fn<SourceEncoding, decode_view_kind::valid, ToType> decode_valid{};

        /// @brief Range adaptor that decodes code unit ranges to code points using `std::expected` to propagate decoding errors.
        ///
        /// Produces a view of `std::expected` values containing either code points decoded from the original code unit sequence,
        /// or errors caused by ill-formed code unit sequences in the original sequence.
        ///
        /// This adaptor is used over `decode_lossy` whenever a detailed error description is needed. This can be, for example,
        /// whenever the user wants to use a custom error-handling policy.
        ///
        /// @tparam SourceEncoding The encoding to decode from.
        ///
        /// @tparam ToType Code point type to decode to. If the source encoding is ASCII, this type can be either `uchar` or `ascii_char`.
        ///         Otherwise, this type must be `uchar`. By default, this type is specified as `uchar`.
        ///
        /// @see views::decode, views::decode_valid, views::decode_lossy
        ///
        template<encoding SourceEncoding, char_type ToType = uchar>
            requires(std::same_as<ToType, uchar> || SourceEncoding == encoding::ascii)
        inline constexpr impl::decode_fn<SourceEncoding, decode_view_kind::expected, ToType> decode_expected{};

        /// @brief Range adaptor that decodes code unit ranges to code points, replacing ill-formed subsequences with a fallback character.
        ///
        /// Produces a lazy view that decodes potentially-ill-formed code unit sequences to a sequence of code points.
        ///
        /// It replaces ill-formed code unit sequences with a fallback character.
        ///
        /// The fallback character depends on the specified `ToType`: U+FFFD for `uchar` and `0x1A` for `ascii_char`.
        ///
        /// It uses the [Substitution of Maximal Subparts](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G66453) specification
        /// to determine the number of malformed code units to replace with a single fallback character.
        ///
        /// @tparam SourceEncoding The encoding to decode from.
        ///
        /// @tparam ToType Code point type to decode to. If the source encoding is ASCII, this type can be either `uchar` or `ascii_char`.
        ///         Otherwise, this type must be `uchar`. By default, this type is specified as `uchar`.
        ///
        /// @see views::decode, views::decode_valid, views::decode_expected
        ///
        template<encoding SourceEncoding, char_type ToType = uchar>
            requires(std::same_as<ToType, uchar> || SourceEncoding == encoding::ascii)
        inline constexpr impl::decode_fn<SourceEncoding, decode_view_kind::lossy, ToType> decode_lossy{};

        /// @}

        /// @defgroup ascii_decoding_range_adaptors ASCII decoding adaptors
        ///
        /// @brief Range adaptors that decode ASCII code units to a range of code points using the specified error-handling policy.
        ///
        /// See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        ///
        /// @{

        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::ascii, decode_view_kind::valid, ascii_char> decode_valid_ascii{};
        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::ascii, decode_view_kind::expected, ascii_char> decode_expected_ascii{};
        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::ascii, decode_view_kind::lossy, ascii_char> decode_lossy_ascii{};

        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::ascii, decode_view_kind::valid, uchar> decode_valid_ascii_to_uchars{};
        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::ascii, decode_view_kind::expected, uchar> decode_expected_ascii_to_uchars{};
        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::ascii, decode_view_kind::lossy, uchar> decode_lossy_ascii_to_uchars{};

        /// @}

        /// @defgroup utf8_decoding_range_adaptors UTF-8 decoding adaptors
        ///
        /// @brief Range adaptors that decode UTF-8 code units to a range of `uchar`s using the specified error-handling policy.
        ///
        /// See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        ///
        /// @{

        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::utf8, decode_view_kind::valid, uchar> decode_valid_utf8{};
        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::utf8, decode_view_kind::expected, uchar> decode_expected_utf8{};
        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::utf8, decode_view_kind::lossy, uchar> decode_lossy_utf8{};

        /// @}

        /// @defgroup utf16_decoding_range_adaptors UTF-16 decoding adaptors
        ///
        /// @brief Range adaptors that decode UTF-16 code units to a range of `uchar`s using the specified error-handling policy.
        ///
        /// See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        ///
        /// @{

        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::utf16, decode_view_kind::valid, uchar> decode_valid_utf16{};
        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::utf16, decode_view_kind::expected, uchar> decode_expected_utf16{};
        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::utf16, decode_view_kind::lossy, uchar> decode_lossy_utf16{};

        /// @}

        /// @defgroup utf32_decoding_range_adaptors UTF-32 decoding adaptors
        ///
        /// @brief Range adaptors that decode UTF-32 code units to a range of `uchar`s using the specified error-handling policy.
        ///
        /// See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        ///
        /// @{

        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::utf32, decode_view_kind::valid, uchar> decode_valid_utf32{};
        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::utf32, decode_view_kind::expected, uchar> decode_expected_utf32{};
        /// @details See @ref decoding_range_adaptors "Decoding Range Adaptors documentation".
        inline constexpr impl::decode_fn<encoding::utf32, decode_view_kind::lossy, uchar> decode_lossy_utf32{};

        /// @}
        /// @}
        /// @}
    } // namespace views
} // namespace upp::ranges

#endif // UNI_CPP_IMPL_RANGES_TRANSCODE_HPP