#ifndef UNI_CPP_IMPL_RANGES_NORMALIZE_HPP
#define UNI_CPP_IMPL_RANGES_NORMALIZE_HPP

/// @file
///
/// @brief Defines range adaptors for normalizing ranges of code points.
///

#include "base.hpp"
#include "approximately_sized_range.hpp"
#include "view_interface.hpp"
#include "iterator_cache.hpp"

#include "../../uchar.hpp"
#include "../unicode_data/decomposition.hpp"
#include "../normalization_form.hpp"
#include "../no_unique_address.hpp"

#include "../small_vector.hpp"

#include <bit>
#include <optional>

/// @defgroup normalization_range_adaptors Normalization range adaptors
///
/// @brief Provides range adaptors for normalizing code point ranges according to [UAX #15](https://www.unicode.org/reports/tr15).
///
/// @par Example:
///
/// @code{.cpp}
///
/// std::string_view username = /* ... */;
///
/// auto normalized_username = username | upp::views::mark_as_valid_utf8
///                                     | upp::views::decode_valid_utf8
///                                     | upp::views::normalize_to_nfc
///                                     | upp::views::encode_as_utf8;
///
/// auto user = database_query(normalized_username);
///
/// @endcode
///
/// @see @ref upp::ranges::views::normalize_to "views::normalize_to"
/// @see @ref upp::ranges::views::normalize_to_nfd "views::normalize_to_nfd"
/// @see @ref upp::ranges::views::normalize_to_nfc "views::normalize_to_nfc"
/// @see @ref upp::ranges::views::normalize_to_nfkd "views::normalize_to_nfkd"
/// @see @ref upp::ranges::views::normalize_to_nfkc "views::normalize_to_nfkc"
///
/// @headerfile "" <uni-cpp/ranges.hpp>
///

namespace upp::ranges
{
    /// @brief A lazy view adaptor that normalizes code points to the specified normalization form.
    ///
    /// @note This adaptor performs the normalization lazily, although normalizing an arbitrary code point
    /// sequence requires using buffers. These buffers are almost always on stack, but on malicious/meaningless
    /// input, these may run out of their huge SBO and allocate on heap. This will never occur in practice on
    /// real-world text, but specially-crafted input can make it happen.
    ///
    /// @note Users should use @ref upp::ranges::views::normalize_to "views::normalize_to" as opposed to using this type directly.
    ///
    /// @ingroup normalization_range_adaptors
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<std::ranges::view View, normalization_form Form>
        requires code_point_range<View>
    class normalize_view : public UNI_CPP_IMPL_VIEW_INTERFACE(normalize_view<View, Form>)
    {
    private:
        class iterator;
        class sentinel;

    public:
        /// @brief Default constructor.
        ///
        normalize_view()
            requires std::default_initializable<View>
        = default;

        /// @brief Constructs the `normalize_view` from the underlying view.
        ///
        constexpr explicit normalize_view(View base)
            : m_base(std::move(base))
        {
        }

        /// @brief Constructs the `normalize_view` from the underlying view.
        ///
        /// Tagged constructor for CTAD.
        ///
        constexpr normalize_view(View base, nontype_t<Form>)
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
        /// @note This view intentionally omits the const-qualified `begin()` method.
        ///       This range is non-const-iterable, because it might need to modify its internal state.
        ///
        constexpr iterator begin()
        {
            if constexpr (std::ranges::forward_range<View>)
            {
                if (m_cached_begin.has_value())
                    return *m_cached_begin;
            }

            auto get_it = [&] {
                if constexpr (std::ranges::bidirectional_range<View>)
                    return iterator(std::ranges::begin(m_base), std::ranges::begin(m_base), std::ranges::end(m_base));
                else
                    return iterator(std::ranges::begin(m_base), std::ranges::end(m_base));
            };

            if constexpr (std::ranges::forward_range<View>)
            {
                auto it = get_it();

                m_cached_begin.emplace(it);
                return it;
            }
            else
                return get_it();
        }

        /// @brief Returns a sentinel marking the end of the range.
        ///
        /// @note This view intentionally omits the const-qualified `end()` method.
        ///       This range is non-const-iterable, because it might need to modify its internal state.
        ///
        constexpr sentinel end() { return sentinel(std::ranges::end(m_base)); }

        /// @brief Returns an iterator marking the end of the range.
        ///
        /// @note This view intentionally omits the const-qualified `end()` method.
        ///       This range is non-const-iterable, because it might need to modify its internal state.
        ///
        constexpr iterator end()
            requires std::ranges::common_range<View>
        {
            if constexpr (std::ranges::bidirectional_range<View>)
                return iterator(std::ranges::end(m_base), std::ranges::begin(m_base), std::ranges::end(m_base));
            else
                return iterator(std::ranges::end(m_base), std::ranges::end(m_base));
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
            requires impl::range_supports_empty<const View> && code_point_range<const View>
        {
            return std::ranges::empty(m_base);
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
            requires approximately_sized_range<const View> && code_point_range<const View>
        {
            return ranges::reserve_hint(m_base);
        }

    private:
        using signed_size_t = decltype(0z);

        static constexpr bool s_canonical     = canonical_normalization_form<Form>;
        static constexpr bool s_compatibility = compatibility_normalization_form<Form>;

        static constexpr bool s_compose = Form == normalization_form::nfc || Form == normalization_form::nfkc;

        class iterator : public impl::input_iterator_category_impl<View>
        {
        public:
            using iterator_concept = decltype(impl::bidirectional_range_iterator_concept_impl<View>());
            using value_type       = uchar;
            using difference_type  = std::ptrdiff_t;

        public:
            /// @brief Default constructor.
            ///
            constexpr iterator()
                requires std::default_initializable<std::ranges::iterator_t<View>>
            = default;

            constexpr iterator(const iterator&)
                requires std::copyable<std::ranges::iterator_t<View>>
            = default;
            constexpr iterator(iterator&&) = default;

            constexpr iterator& operator=(const iterator&)
                requires std::copyable<std::ranges::iterator_t<View>>
            = default;
            constexpr iterator& operator=(iterator&&) = default;

            /// @brief Returns a `const` reference to the underlying iterator.
            ///
            constexpr const std::ranges::iterator_t<View>& base() const& noexcept
                requires std::ranges::forward_range<View>
            {
                return m_current;
            }

            /// @brief Returns the underlying iterator by moving it.
            ///
            constexpr std::ranges::iterator_t<View> base() &&
                requires std::ranges::forward_range<View>
            {
                return std::move(m_current);
            }

            constexpr value_type operator*() const { return m_buffer[std::bit_cast<std::size_t>(m_buffer_index)]; }

            constexpr iterator& operator++()
            {
                ++m_buffer_index;

                if (std::bit_cast<std::size_t>(m_buffer_index) == m_buffer.size())
                {
                    advance_underlying();
                }

                return *this;
            }

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

            constexpr iterator& operator--()
                requires std::ranges::bidirectional_range<View>
            {
                if (m_buffer_index == 0z)
                    read_backwards();
                else
                    --m_buffer_index;

                return *this;
            }

            constexpr iterator operator--(int)
                requires std::ranges::bidirectional_range<View>
            {
                auto temp = *this;
                --*this;
                return temp;
            }

            friend constexpr bool operator==(const iterator& lhs, const iterator& rhs)
                requires std::equality_comparable<std::ranges::iterator_t<View>>
            {
                return lhs.m_current == rhs.m_current && lhs.m_buffer_index == rhs.m_buffer_index;
            }

        private:
            constexpr iterator(std::ranges::iterator_t<View> current, std::ranges::iterator_t<View> begin, std::ranges::sentinel_t<View> end)
                requires std::ranges::bidirectional_range<View>
                : m_current(std::move(current))
                , m_begin(std::move(begin))
                , m_end(std::move(end))
            {
                if (m_current != m_end)
                    read();
            }

            constexpr iterator(std::ranges::iterator_t<View> current, std::ranges::sentinel_t<View> end)
                requires(!std::ranges::bidirectional_range<View>)
                : m_current(std::move(current))
                , m_end(std::move(end))
            {
                if (m_current != m_end)
                {
                    read();
                }
                else
                {
                    if constexpr (!std::ranges::forward_range<View>)
                        m_buffer_index = impl::buffer_index_at_sentinel<signed_size_t>;
                }
            }

            constexpr void advance_underlying()
            {
                if constexpr (std::ranges::forward_range<View>)
                    m_current = *m_advance_to;

                if (m_current != m_end)
                {
                    read();
                }
                else
                {
                    if constexpr (std::ranges::forward_range<View>)
                        m_buffer_index = 0z;
                    else
                        m_buffer_index = impl::buffer_index_at_sentinel<signed_size_t>;
                }
            }

            [[nodiscard]] static constexpr auto decomposition_of(uchar code_point) noexcept
            {
                if constexpr (s_canonical)
                {
                    return upp::impl::unicode_data::decomposition::lookup_decomposition<
                        upp::impl::unicode_data::decomposition::decomposition_kind::canonical>(code_point.value());
                }
                else if constexpr (s_compatibility)
                {
                    return upp::impl::unicode_data::decomposition::lookup_decomposition<
                        upp::impl::unicode_data::decomposition::decomposition_kind::compatibility>(code_point.value());
                }
                else
                    static_assert(false);
            }

            [[nodiscard]] static constexpr quick_check qc(uchar code_point) noexcept
            {
                if constexpr (Form == normalization_form::nfd)
                {
                    return code_point.nfd_quick_check();
                }
                else if constexpr (Form == normalization_form::nfc)
                {
                    return code_point.nfc_quick_check();
                }
                else if constexpr (Form == normalization_form::nfkd)
                {
                    return code_point.nfkd_quick_check();
                }
                else if constexpr (Form == normalization_form::nfkc)
                {
                    return code_point.nfkc_quick_check();
                }
                else
                    static_assert(false);
            }

            constexpr void fully_decompose_the_buffer()
            {
                using buffer_t = decltype(m_buffer);

                buffer_t new_buffer;

                // Note: this `reserve` call almost never actually allocates.
                // It only does so in the extremely rare case of m_buffer being larger than its in-place buffer.
                new_buffer.reserve(m_buffer.size());

                for (const uchar code_point : m_buffer)
                {
                    const auto decomposition = decomposition_of(code_point);

                    new_buffer.append(decomposition.begin(), decomposition.end());
                }

                m_buffer = std::move(new_buffer);
            }

            constexpr void canonically_order_the_buffer()
            {
                for (std::size_t i = 0uz; i < m_buffer.size();)
                {
                    const std::uint8_t first_ccc = m_buffer[i].canonical_combining_class();

                    if (first_ccc == 0)
                    {
                        ++i;
                        continue;
                    }

                    // Beginning of a non-starter run

                    const std::size_t beginning = i++;

                    bool is_canonically_ordered = true;

                    for (std::uint8_t prev_ccc = first_ccc; i < m_buffer.size(); ++i)
                    {
                        const std::uint8_t ccc = m_buffer[i].canonical_combining_class();

                        if (ccc == 0)
                            break;

                        if (is_canonically_ordered)
                            is_canonically_ordered = prev_ccc <= ccc;

                        prev_ccc = ccc;
                    }

                    if (!is_canonically_ordered)
                    {
                        // Stable insertion sort of non-starters

                        for (std::size_t j = beginning + 1uz; j < i; ++j)
                        {
                            const uchar        code_point  = m_buffer[j];
                            const std::uint8_t current_ccc = code_point.canonical_combining_class();

                            std::size_t k = j;

                            while (k > beginning)
                            {
                                const std::uint8_t prev_ccc = m_buffer[k - 1].canonical_combining_class();

                                if (prev_ccc <= current_ccc)
                                    break;

                                m_buffer[k] = m_buffer[k - 1];
                                --k;
                            }

                            m_buffer[k] = code_point;
                        }
                    }
                }
            }

            /// @pre `m_buffer` cannot be empty.
            ///
            constexpr void canonically_compose_the_buffer()
                requires(s_compose)
            {
                // Note: Since this function is called after decomposition, there may not be any starters in the buffer.
                // For example, if the buffer initially contained the U+0F73 starter, after full decomposition it contains [U+0F71, U+0F72],
                // both of which, are not starters. All we know is that the buffer cannot be empty.

                auto it = m_buffer.begin();

                while (true)
                {
                    if (it->canonical_combining_class() == 0)
                        break;

                    ++it;

                    if (it == m_buffer.end())
                    {
                        // If no starter was found, there is nothing to compose.
                        return;
                    }
                }

                auto last_starter_it = it++;

                std::uint8_t prev_ccc = 0;

                auto apply_composition = [&](uchar composite) {
                    *last_starter_it = composite;
                    it               = m_buffer.erase(it);
                };

                while (it != m_buffer.end())
                {
                    const uchar code_point = *it;

                    const std::uint8_t ccc = code_point.canonical_combining_class();

                    if (ccc == 0) // `code_point` is a starter itself
                    {
                        if (prev_ccc != 0) // if blocked
                        {
                            // Set `code_point` as the new last starter.

                            last_starter_it = it;
                            ++it;
                            prev_ccc = 0;
                            continue;
                        }

                        // not blocked

                        prev_ccc = 0;

                        if (const auto composition = uchar::composition(*last_starter_it, code_point);
                            composition) // has a composition with `*last_starter_it`
                        {
                            apply_composition(*composition);
                            continue;
                        }

                        // does not have a composition with `*last_starter_it`

                        last_starter_it = it;
                        ++it;
                        continue;
                    }

                    // `code_point` is a non-starter

                    if (prev_ccc >= ccc) // if blocked
                    {
                        ++it;
                        prev_ccc = ccc;
                        continue;
                    }

                    // not blocked

                    if (const auto composition = uchar::composition(*last_starter_it, code_point);
                        composition) // has a composition with `*last_starter_it`
                    {
                        apply_composition(*composition);
                        prev_ccc = (it - 1)->canonical_combining_class();

                        continue;
                    }

                    // does not have a composition with `*last_starter_it`

                    prev_ccc = ccc;
                    ++it;
                }
            }

            constexpr void read_impl(std::ranges::iterator_t<View>& it, const std::ranges::sentinel_t<View>& last)
            {
                m_buffer.clear();
                m_buffer_index = 0z;

                const uchar first = *it;
                ++it;

                m_buffer.emplace_back(first);

                if (first.canonical_combining_class() != 0)
                {
                    for (; it != last; ++it)
                    {
                        const uchar code_point = *it;

                        if (code_point.canonical_combining_class() == 0)
                            break;

                        m_buffer.emplace_back(code_point);
                    }

                    if constexpr (std::ranges::forward_range<View>)
                        m_advance_to = std::move(it);

                    fully_decompose_the_buffer();
                    canonically_order_the_buffer();

                    // Non-starters never compose, skip the composition step.

                    return;
                }

                for (; it != last; ++it)
                {
                    const uchar code_point = *it;

                    if (code_point.canonical_combining_class() == 0 && qc(code_point) == quick_check::yes)
                        break;

                    m_buffer.emplace_back(code_point);
                }

                if constexpr (std::ranges::forward_range<View>)
                    m_advance_to = std::move(it);

                if (m_buffer.size() == 1uz && qc(first) == quick_check::yes)
                {
                    // The buffer only has a single starter with qc == yes,
                    // and the next code point is also a starter with qc == yes,
                    // or there is no next code point (end of range).
                    // This is the most trivial case. Don't do anything.
                    return;
                }

                // The non-trivial case. Can't skip anything.

                fully_decompose_the_buffer();
                canonically_order_the_buffer();

                if constexpr (s_compose)
                    canonically_compose_the_buffer();
            }

            /// @brief Advances the underlying iterator the necessary amount and updates the buffer.
            ///
            constexpr void read()
            {
                impl::iterator_guard<std::ranges::iterator_t<View>> guard{m_current, m_current};
                read_impl(m_current, m_end);
            }

            constexpr void reverse_buffer() { std::ranges::reverse(m_buffer); }

            /// @brief Moves the underlying iterator backwards the necessary amount and updates the buffer.
            ///
            constexpr void read_backwards()
                requires std::ranges::bidirectional_range<View>
            {
                m_buffer.clear();
                m_advance_to = m_current;

                std::optional<std::ranges::iterator_t<View>> last_starter;
                std::size_t                                  code_points_since_last_starter = 0uz;

                // Stop at the first encountered starter with qc == yes.
                // If there isn't one, stop at the last encountered starter.
                // If there isn't one, stop at the last encountered code point.
                // This matches the behaviour of the forward reading function.

                do
                {
                    --m_current;

                    const uchar code_point = *m_current;

                    m_buffer.emplace_back(code_point);

                    if (code_point.canonical_combining_class() == 0)
                    {
                        if (qc(code_point) == quick_check::yes)
                        {
                            if (m_buffer.size() == 1uz)
                            {
                                // The buffer only has a single starter with qc == yes,
                                // and the next code point (going forward) is also a starter with qc == yes,
                                // or there is no next code point (end of range).
                                // This is the most trivial case. Don't do anything.

                                m_buffer_index = 0z;
                                return;
                            }

                            // The non-trivial case. Can't skip anything.

                            reverse_buffer();
                            fully_decompose_the_buffer();
                            canonically_order_the_buffer();

                            if constexpr (s_compose)
                                canonically_compose_the_buffer();

                            m_buffer_index = m_buffer.size() - 1uz;

                            return;
                        }

                        code_points_since_last_starter = 0uz;
                        last_starter                   = m_current;
                    }
                    else
                        ++code_points_since_last_starter;

                } while (m_current != m_begin);

                // No starter with `qc == yes` found.
                // Try to stop at the last encountered starter.

                if (last_starter)
                {
                    m_current = *last_starter;
                    m_buffer.resize(m_buffer.size() - code_points_since_last_starter);

                    reverse_buffer();
                    fully_decompose_the_buffer();
                    canonically_order_the_buffer();

                    if constexpr (s_compose)
                        canonically_compose_the_buffer();

                    m_buffer_index = m_buffer.size() - 1uz;
                }
                else
                {
                    // No starter found. There is no need to canonically compose the buffer.

                    reverse_buffer();
                    fully_decompose_the_buffer();
                    canonically_order_the_buffer();

                    m_buffer_index = m_buffer.size() - 1uz;
                }
            }

        private:
            std::ranges::iterator_t<View> m_current = std::ranges::iterator_t<View>();

            UNI_CPP_IMPL_NO_UNIQUE_ADDRESS
            upp::impl::maybe_present<std::ranges::bidirectional_range<View>, std::ranges::iterator_t<View>> m_begin = decltype(m_begin)();

            UNI_CPP_IMPL_NO_UNIQUE_ADDRESS std::ranges::sentinel_t<View> m_end = std::ranges::sentinel_t<View>();

            UNI_CPP_IMPL_NO_UNIQUE_ADDRESS
            upp::impl::maybe_present<std::ranges::forward_range<View>, std::optional<std::ranges::iterator_t<View>>> m_advance_to;

            upp::impl::small_vector<uchar, 32u> m_buffer{};
            signed_size_t                       m_buffer_index = 0z;

        private:
            friend normalize_view;
        };

        class sentinel
        {
        private:
            std::ranges::sentinel_t<View> m_end = std::ranges::sentinel_t<View>();

        public:
            /// @brief Default constructor.
            ///
            sentinel() = default;

            /// @brief Returns a copy of the underlying sentinel.
            ///
            constexpr std::ranges::sentinel_t<View> base() const { return m_end; }

            /// @brief Compares an iterator with a sentinel.
            ///
            friend constexpr bool operator==(const iterator& x, const sentinel& y)
            {
                if constexpr (std::ranges::forward_range<View>)
                {
                    return x.m_current == y.m_end;
                }
                else
                {
                    return x.m_current == y.m_end && x.m_buffer_index == impl::buffer_index_at_sentinel<signed_size_t>;
                }
            }

        private:
            constexpr explicit sentinel(std::ranges::sentinel_t<View> end)
                : m_end{end}
            {
            }

            friend normalize_view;
        };

    private:
        View m_base = View();

        UNI_CPP_IMPL_NO_UNIQUE_ADDRESS upp::impl::maybe_present<std::ranges::forward_range<View>, impl::non_propagating_cache<iterator>>
                                       m_cached_begin;
    };

    /// @cond

    template<typename Range, normalization_form Form>
    normalize_view(Range&&, nontype_t<Form>) -> normalize_view<std::views::all_t<Range>, Form>;

    /// @endcond

    namespace impl
    {
        namespace normalize_view_impl
        {
            template<typename>
            inline constexpr bool is_normalize_view = false;

            template<typename View, normalization_form Form>
            inline constexpr bool is_normalize_view<normalize_view<View, Form>> = true;

            template<typename>
            struct get_normalize_view_info;

            template<typename View, normalization_form Form>
            struct get_normalize_view_info<normalize_view<View, Form>>
            {
                static constexpr normalization_form form = Form;
            };
        } // namespace normalize_view_impl

        template<normalization_form Form>
        struct normalize_fn : public std::ranges::range_adaptor_closure<normalize_fn<Form>>
        {
        public:
            template<std::ranges::viewable_range Range>
                requires code_point_range<Range>
            [[nodiscard]] constexpr auto operator()(Range&& range) const
            {
                using range_t = std::remove_cvref_t<Range>;

                if constexpr (normalize_view_impl::is_normalize_view<range_t>)
                {
                    // See https://www.unicode.org/reports/tr15/#Design_Goals.

                    static constexpr normalization_form previous_form = normalize_view_impl::get_normalize_view_info<range_t>::form;

                    if constexpr (previous_form == Form)
                    {
                        return std::views::all(std::forward<Range>(range));
                    }
                    else
                    {
                        static constexpr bool is_previous_compatibility = is_compatibility_normalization_form(previous_form);

                        static constexpr bool is_this_compatibility = is_compatibility_normalization_form(Form);

                        static constexpr bool is_compatibility = is_previous_compatibility || is_this_compatibility;

                        static constexpr bool is_composition = Form == normalization_form::nfc || Form == normalization_form::nfkc;

                        static constexpr normalization_form resulting_normalization_form =
                            is_compatibility ? (is_composition ? normalization_form::nfkc : normalization_form::nfkd)
                                             : (is_composition ? normalization_form::nfc : normalization_form::nfd);

                        return normalize_view(std::forward<Range>(range).base(), nontype<resulting_normalization_form>);
                    }
                }
                else
                {
                    return normalize_view<std::views::all_t<Range>, Form>(std::views::all(std::forward<Range>(range)));
                }
            }
        };
    } // namespace impl

    namespace views
    {
        /// @addtogroup normalization_range_adaptors
        /// @{

        /// @brief Range adaptor that normalizes a sequence of code points to the desired Unicode normalization form.
        /// @tparam Form The desired normalization form.
        ///
        /// @par Example:
        ///
        /// @code{.cpp}
        ///
        /// std::string_view username = /* ... */;
        ///
        /// auto normalized_username = username | upp::views::mark_as_valid_utf8
        ///                                     | upp::views::decode_valid_utf8
        ///                                     | upp::views::normalize_to_nfc
        ///                                     | upp::views::encode_as_utf8;
        ///
        /// auto user = database_query(normalized_username);
        ///
        /// @endcode
        ///
        /// Chained uses of `normalize_to` in a range pipeline collapse into a single @ref upp::ranges::normalize_view "normalize_view"
        /// according to the rules presented in [UAX #15, Design Goals](https://www.unicode.org/reports/tr15/#Design_Goals).
        /// For example:
        ///
        /// @code{.cpp}
        /// str | upp::views::normalize_to_nfkc | upp::views::normalize_to_nfd
        /// @endcode
        ///
        /// collapses to a range equivalent to
        ///
        /// @code{.cpp}
        /// str | upp::views::normalize_to_nfkd
        /// @endcode
        ///
        /// @note This adaptor performs the normalization lazily, although normalizing an arbitrary code point
        /// sequence requires using buffers. These buffers are almost always on stack, but on malicious/meaningless
        /// input, these may run out of their huge SBO and allocate on heap. This will never occur in practice on
        /// real-world text, but specially-crafted input can make it happen.
        ///
        /// @see @ref upp::ranges::views::normalize_to_nfc "views::normalize_to_nfc", @ref upp::ranges::views::normalize_to_nfd "views::normalize_to_nfd"
        /// @see @ref upp::ranges::views::normalize_to_nfkc "views::normalize_to_nfkc", @ref upp::ranges::views::normalize_to_nfkd "views::normalize_to_nfkd"
        ///
        template<normalization_form Form>
        inline constexpr impl::normalize_fn<Form> normalize_to{};

        /// @brief Range adaptor that normalizes a sequence of code points to NFD.
        ///
        /// See @ref upp::ranges::views::normalize_to "views::normalize_to" documentation for more details.
        ///
        /// @see @ref upp::ranges::views::normalize_to "views::normalize_to"
        /// @see @ref upp::ranges::views::normalize_to_nfc "views::normalize_to_nfc"
        /// @see @ref upp::ranges::views::normalize_to_nfkd "views::normalize_to_nfkd"
        ///
        inline constexpr impl::normalize_fn<normalization_form::nfd> normalize_to_nfd{};

        /// @brief Range adaptor that normalizes a sequence of code points to NFC.
        ///
        /// See @ref upp::ranges::views::normalize_to "views::normalize_to" documentation for more details.
        ///
        /// @see @ref upp::ranges::views::normalize_to "views::normalize_to"
        /// @see @ref upp::ranges::views::normalize_to_nfd "views::normalize_to_nfd"
        /// @see @ref upp::ranges::views::normalize_to_nfkc "views::normalize_to_nfkc"
        ///
        inline constexpr impl::normalize_fn<normalization_form::nfc> normalize_to_nfc{};

        /// @brief Range adaptor that normalizes a sequence of code points to NFKD.
        ///
        /// See @ref upp::ranges::views::normalize_to "views::normalize_to" documentation for more details.
        ///
        /// @see @ref upp::ranges::views::normalize_to "views::normalize_to"
        /// @see @ref upp::ranges::views::normalize_to_nfd "views::normalize_to_nfd"
        /// @see @ref upp::ranges::views::normalize_to_nfkc "views::normalize_to_nfkc"
        ///
        inline constexpr impl::normalize_fn<normalization_form::nfkd> normalize_to_nfkd{};

        /// @brief Range adaptor that normalizes a sequence of code points to NFKC.
        ///
        /// See @ref upp::ranges::views::normalize_to "views::normalize_to" documentation for more details.
        ///
        /// @see @ref upp::ranges::views::normalize_to "views::normalize_to"
        /// @see @ref upp::ranges::views::normalize_to_nfc "views::normalize_to_nfc"
        /// @see @ref upp::ranges::views::normalize_to_nfkd "views::normalize_to_nfkd"
        ///
        inline constexpr impl::normalize_fn<normalization_form::nfkc> normalize_to_nfkc{};

        /// @}
    } // namespace views
} // namespace upp::ranges

/// @cond

template<typename View, upp::normalization_form Form>
inline constexpr bool std::ranges::enable_borrowed_range<upp::ranges::normalize_view<View, Form>> = std::ranges::enable_borrowed_range<View>;

/// @endcond

#endif // UNI_CPP_IMPL_RANGES_NORMALIZE_HPP