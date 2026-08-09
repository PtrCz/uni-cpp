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
#include "../inplace_vector.hpp"

#include <bit>
#include <optional>
#include <type_traits>

namespace upp::ranges
{
    namespace impl::norm
    {
        enum class decomposition_kind : std::uint8_t
        {
            canonical,
            compatibility
        };

        template<std::ranges::view View, decomposition_kind Kind>
            requires code_point_range<View>
        class decompose_view : public UNI_CPP_IMPL_VIEW_INTERFACE(decompose_view<View, Kind>)
        {
        private:
            template<bool>
            class iterator;

            template<bool>
            class sentinel;

        public:
            decompose_view()
                requires std::default_initializable<View>
            = default;

            constexpr explicit decompose_view(View base)
                : m_base(std::move(base))
            {
            }

            /// Tagged constructor for CTAD.
            ///
            constexpr decompose_view(View base, nontype_t<Kind>)
                : m_base(std::move(base))
            {
            }

            constexpr View base() const&
                requires std::copy_constructible<View>
            {
                return m_base;
            }

            constexpr View base() && { return std::move(m_base); }

            constexpr iterator<false> begin() { return iterator<false>(std::ranges::begin(m_base), std::ranges::end(m_base)); }

            constexpr iterator<true> begin() const
                requires std::ranges::range<const View> && code_point_range<const View>
            {
                return iterator<true>(std::ranges::begin(m_base), std::ranges::end(m_base));
            }

            constexpr sentinel<false> end() { return sentinel<false>(std::ranges::end(m_base)); }

            constexpr iterator<false> end()
                requires std::ranges::common_range<View>
            {
                return iterator<false>(std::ranges::end(m_base), std::ranges::end(m_base));
            }

            constexpr sentinel<true> end() const
                requires std::ranges::range<const View> && code_point_range<const View>
            {
                return sentinel<true>(std::ranges::end(m_base));
            }

            constexpr iterator<true> end() const
                requires std::ranges::common_range<const View> && code_point_range<const View>
            {
                return iterator<true>(std::ranges::end(m_base), std::ranges::end(m_base));
            }

            constexpr bool empty()
                requires impl::range_supports_empty<View>
            {
                return std::ranges::empty(m_base);
            }

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
            template<bool Const>
            class iterator : public impl::input_iterator_category_impl<View>
            {
            private:
                using parent_t = impl::maybe_const<Const, decompose_view>;
                using base_t   = impl::maybe_const<Const, View>;

            public:
                using iterator_concept = decltype(impl::bidirectional_range_iterator_concept_impl<base_t>());

                using value_type      = uchar;
                using difference_type = std::ptrdiff_t;

            public:
                constexpr iterator()
                    requires std::default_initializable<std::ranges::iterator_t<base_t>>
                = default;

                constexpr iterator(const iterator&)
                    requires std::copyable<std::ranges::iterator_t<base_t>>
                = default;
                constexpr iterator(iterator&&) = default;

                constexpr iterator& operator=(const iterator&)
                    requires std::copyable<std::ranges::iterator_t<base_t>>
                = default;
                constexpr iterator& operator=(iterator&&) = default;

                constexpr const std::ranges::iterator_t<base_t>& base() const& noexcept { return m_current; }

                constexpr std::ranges::iterator_t<base_t> base() && { return std::move(m_current); }

                constexpr value_type operator*() const { return m_buffer[m_buffer_index]; }

                constexpr iterator& operator++()
                {
                    ++m_buffer_index;

                    if (m_buffer_index == static_cast<std::uint8_t>(m_buffer.size()))
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
                    requires std::ranges::bidirectional_range<base_t>
                {

                    if (m_buffer_index == 0)
                        advance_underlying_backwards();
                    else
                        --m_buffer_index;

                    return *this;
                }

                constexpr iterator operator--(int)
                    requires std::ranges::bidirectional_range<base_t>
                {
                    auto temp = *this;
                    --*this;
                    return temp;
                }

                friend constexpr bool operator==(const iterator& lhs, const iterator& rhs)
                    requires std::equality_comparable<std::ranges::iterator_t<base_t>>
                {
                    return lhs.m_current == rhs.m_current && lhs.m_buffer_index == rhs.m_buffer_index;
                }

            private:
                constexpr iterator(std::ranges::iterator_t<base_t> current, std::ranges::sentinel_t<base_t> end)
                    : m_current(std::move(current))
                    , m_end(std::move(end))
                {
                    if (m_current != m_end)
                    {
                        m_buffer = decomposition_of(*m_current);
                    }
                }

                [[nodiscard]] static constexpr auto decomposition_of(uchar code_point) noexcept
                {
                    if constexpr (Kind == decomposition_kind::canonical)
                    {
                        return upp::impl::unicode_data::decomposition::lookup_decomposition<
                            upp::impl::unicode_data::decomposition::decomposition_kind::canonical>(code_point.value());
                    }
                    else if constexpr (Kind == decomposition_kind::compatibility)
                    {
                        return upp::impl::unicode_data::decomposition::lookup_decomposition<
                            upp::impl::unicode_data::decomposition::decomposition_kind::compatibility>(code_point.value());
                    }
                    else
                        static_assert(false);
                }

                constexpr void advance_underlying()
                {
                    ++m_current;
                    m_buffer_index = 0;

                    if (m_current != m_end)
                    {
                        m_buffer = decomposition_of(*m_current);
                    }
                }

                constexpr void advance_underlying_backwards()
                    requires std::ranges::bidirectional_range<base_t>
                {
                    --m_current;

                    m_buffer       = decomposition_of(*m_current);
                    m_buffer_index = m_buffer.size() - 1;
                }

            private:
                std::ranges::iterator_t<base_t> m_current = std::ranges::iterator_t<base_t>();

                UNI_CPP_IMPL_NO_UNIQUE_ADDRESS std::ranges::sentinel_t<base_t> m_end = std::ranges::sentinel_t<base_t>();

                upp::impl::inplace_vector<uchar, 18> m_buffer{};
                std::uint8_t                         m_buffer_index = 0;

                template<std::ranges::view View2, decomposition_kind Kind2>
                    requires code_point_range<View2>
                friend class decompose_view;
            };

            template<bool Const>
            class sentinel
            {
            private:
                using base_t = impl::maybe_const<Const, View>;

                std::ranges::sentinel_t<base_t> m_end = std::ranges::sentinel_t<base_t>();

            public:
                sentinel() = default;

                /// @brief Constructs a `const` sentinel from a non-`const` sentinel.
                ///
                constexpr explicit sentinel(sentinel<!Const> i)
                    requires Const && std::convertible_to<std::ranges::sentinel_t<View>, std::ranges::sentinel_t<base_t>>
                    : m_end{i.m_end}
                {
                }

                constexpr std::ranges::sentinel_t<base_t> base() const { return m_end; }

                /// @brief Compares an iterator with a sentinel.
                ///
                template<bool OtherConst>
                    requires std::sentinel_for<std::ranges::sentinel_t<base_t>, std::ranges::iterator_t<impl::maybe_const<OtherConst, View>>>
                friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y)
                {
                    return x.m_current == y.m_end;
                }

            private:
                constexpr explicit sentinel(std::ranges::sentinel_t<base_t> end)
                    : m_end{end}
                {
                }

                template<std::ranges::view View2, decomposition_kind Kind2>
                    requires code_point_range<View2>
                friend class decompose_view;
            };

        private:
            View m_base = View();
        };

        template<typename Range, decomposition_kind Kind>
        decompose_view(Range&&, nontype_t<Kind>) -> decompose_view<std::views::all_t<Range>, Kind>;

        using signed_size_t = decltype(0z);

        template<std::ranges::view View>
            requires code_point_range<View>
        class canonically_order_view : public UNI_CPP_IMPL_VIEW_INTERFACE(canonically_order_view<View>)
        {
        private:
            class iterator;
            class sentinel;

        public:
            canonically_order_view()
                requires std::default_initializable<View>
            = default;

            constexpr explicit canonically_order_view(View base)
                : m_base(std::move(base))
            {
            }

            constexpr View base() const&
                requires std::copy_constructible<View>
            {
                return m_base;
            }

            constexpr View base() && { return std::move(m_base); }

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

            constexpr sentinel end() { return sentinel(std::ranges::end(m_base)); }

            constexpr iterator end()
                requires std::ranges::common_range<View>
            {
                if constexpr (std::ranges::bidirectional_range<View>)
                    return iterator(std::ranges::end(m_base), std::ranges::begin(m_base), std::ranges::end(m_base));
                else
                    return iterator(std::ranges::end(m_base), std::ranges::end(m_base));
            }

            constexpr bool empty()
                requires impl::range_supports_empty<View>
            {
                return std::ranges::empty(m_base);
            }

            constexpr bool empty() const
                requires impl::range_supports_empty<const View> && code_point_range<const View>
            {
                return std::ranges::empty(m_base);
            }

            constexpr auto size()
                requires std::ranges::sized_range<View>
            {
                return std::ranges::size(m_base);
            }

            constexpr auto size() const
                requires std::ranges::sized_range<const View> && code_point_range<const View>
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
                requires approximately_sized_range<const View> && code_point_range<const View>
            {
                return ranges::reserve_hint(m_base);
            }

        private:
            class iterator : public impl::input_iterator_category_impl<View>
            {
            public:
                using iterator_concept = decltype(impl::bidirectional_range_iterator_concept_impl<View>());
                using value_type       = uchar;
                using difference_type  = std::ptrdiff_t;

            public:
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

                constexpr const std::ranges::iterator_t<View>& base() const& noexcept
                    requires std::ranges::forward_range<View>
                {
                    return m_current;
                }

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

                /// @pre `m_buffer` should only contain code points with `ccc != 0`.
                ///
                [[nodiscard]] constexpr bool is_buffer_canonically_ordered() const
                {
                    std::uint8_t prev_ccc = 0;

                    for (const upp::uchar code_point : m_buffer)
                    {
                        const std::uint8_t ccc = code_point.canonical_combining_class();

                        if (prev_ccc > ccc)
                            return false;

                        prev_ccc = ccc;
                    }

                    return true;
                }

                /// @pre `m_buffer` should only contain code points with `ccc != 0`.
                ///
                constexpr void canonically_order_the_buffer()
                {
                    for (std::size_t i = 1uz; i < m_buffer.size(); ++i)
                    {
                        const uchar code_point = m_buffer[i];

                        const std::uint8_t ccc = code_point.canonical_combining_class();

                        std::size_t j = i;

                        while (j > 0uz)
                        {
                            const uchar prev_code_point = m_buffer[j - 1uz];

                            const std::uint8_t prev_ccc = prev_code_point.canonical_combining_class();

                            if (prev_ccc <= ccc)
                                break;

                            m_buffer[j] = prev_code_point;
                            --j;
                        }

                        m_buffer[j] = code_point;
                    }
                }

                constexpr void read_impl(std::ranges::iterator_t<View>& it, const std::ranges::sentinel_t<View>& last)
                {
                    m_buffer.clear();
                    m_buffer_index = 0z;

                    const uchar first = *it;

                    m_buffer.emplace_back(first);

                    ++it;

                    if (first.canonical_combining_class() == 0)
                    {
                        if constexpr (std::ranges::forward_range<View>)
                            m_advance_to = std::move(it);

                        return;
                    }

                    for (; it != last; ++it)
                    {
                        const uchar code_point = *it;

                        if (code_point.canonical_combining_class() == 0)
                            break;

                        m_buffer.emplace_back(code_point);
                    }

                    if (!is_buffer_canonically_ordered())
                    {
                        canonically_order_the_buffer();
                    }

                    if constexpr (std::ranges::forward_range<View>)
                        m_advance_to = std::move(it);
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

                    --m_current;

                    const uchar first_backwards = *m_current;

                    m_buffer.emplace_back(first_backwards);

                    if (first_backwards.canonical_combining_class() == 0)
                    {
                        m_buffer_index = 0z;
                        return;
                    }

                    for (; m_current != m_begin;)
                    {
                        --m_current;

                        const uchar code_point = *m_current;

                        if (code_point.canonical_combining_class() == 0)
                        {
                            ++m_current;
                            break;
                        }

                        m_buffer.emplace_back(code_point);
                    }

                    reverse_buffer();

                    if (!is_buffer_canonically_ordered())
                    {
                        canonically_order_the_buffer();
                    }

                    m_buffer_index = std::bit_cast<signed_size_t>(m_buffer.size() - 1uz);
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
                template<std::ranges::view View2>
                    requires code_point_range<View2>
                friend class canonically_order_view;
            };

            class sentinel
            {
            private:
                std::ranges::sentinel_t<View> m_end = std::ranges::sentinel_t<View>();

            public:
                sentinel() = default;

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

                template<std::ranges::view View2>
                    requires code_point_range<View2>
                friend class canonically_order_view;
            };

        private:
            View m_base = View();

            UNI_CPP_IMPL_NO_UNIQUE_ADDRESS upp::impl::maybe_present<std::ranges::forward_range<View>, non_propagating_cache<iterator>> m_cached_begin;
        };

        template<std::ranges::view View, decomposition_kind Kind>
            requires code_point_range<View>
        class to_nfc_view : public UNI_CPP_IMPL_VIEW_INTERFACE(to_nfc_view<View, Kind>)
        {
        private:
            class iterator;
            class sentinel;

        public:
            to_nfc_view()
                requires std::default_initializable<View>
            = default;

            constexpr explicit to_nfc_view(View base)
                : m_base(std::move(base))
            {
            }

            /// Tagged constructor for CTAD.
            ///
            constexpr to_nfc_view(View base, nontype_t<Kind>)
                : m_base(std::move(base))
            {
            }

            constexpr View base() const&
                requires std::copy_constructible<View>
            {
                return m_base;
            }

            constexpr View base() && { return std::move(m_base); }

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

            constexpr sentinel end() { return sentinel(std::ranges::end(m_base)); }

            constexpr iterator end()
                requires std::ranges::common_range<View>
            {
                if constexpr (std::ranges::bidirectional_range<View>)
                    return iterator(std::ranges::end(m_base), std::ranges::begin(m_base), std::ranges::end(m_base));
                else
                    return iterator(std::ranges::end(m_base), std::ranges::end(m_base));
            }

            constexpr bool empty()
                requires impl::range_supports_empty<View>
            {
                return std::ranges::empty(m_base);
            }

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
            class iterator : public impl::input_iterator_category_impl<View>
            {
            public:
                using iterator_concept = decltype(impl::bidirectional_range_iterator_concept_impl<View>());
                using value_type       = uchar;
                using difference_type  = std::ptrdiff_t;

            public:
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

                constexpr const std::ranges::iterator_t<View>& base() const& noexcept
                    requires std::ranges::forward_range<View>
                {
                    return m_current;
                }

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
                    if constexpr (Kind == decomposition_kind::canonical)
                    {
                        return upp::impl::unicode_data::decomposition::lookup_decomposition<
                            upp::impl::unicode_data::decomposition::decomposition_kind::canonical>(code_point.value());
                    }
                    else if constexpr (Kind == decomposition_kind::compatibility)
                    {
                        return upp::impl::unicode_data::decomposition::lookup_decomposition<
                            upp::impl::unicode_data::decomposition::decomposition_kind::compatibility>(code_point.value());
                    }
                    else
                        static_assert(false);
                }

                [[nodiscard]] static constexpr quick_check qc(uchar code_point) noexcept
                {
                    if constexpr (Kind == decomposition_kind::canonical)
                    {
                        return code_point.nfc_quick_check();
                    }
                    else if constexpr (Kind == decomposition_kind::compatibility)
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
                    // Unlike in canonically_order_view, the buffer may contain starters here.

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

                upp::impl::small_vector<uchar, 64u> m_buffer{};
                signed_size_t                       m_buffer_index = 0z;

            private:
                template<std::ranges::view View2, decomposition_kind Kind2>
                    requires code_point_range<View2>
                friend class to_nfc_view;
            };

            class sentinel
            {
            private:
                std::ranges::sentinel_t<View> m_end = std::ranges::sentinel_t<View>();

            public:
                sentinel() = default;

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

                template<std::ranges::view View2, decomposition_kind Kind2>
                    requires code_point_range<View2>
                friend class to_nfc_view;
            };

        private:
            View m_base = View();

            UNI_CPP_IMPL_NO_UNIQUE_ADDRESS upp::impl::maybe_present<std::ranges::forward_range<View>, non_propagating_cache<iterator>> m_cached_begin;
        };

        template<typename Range, decomposition_kind Kind>
        to_nfc_view(Range&&, nontype_t<Kind>) -> to_nfc_view<std::views::all_t<Range>, Kind>;

        template<typename View, decomposition_kind Kind>
        struct decomposing_normalization_traits
        {
        private:
            using view_t = canonically_order_view<decompose_view<View, Kind>>;

        public:
            template<typename BaseT>
            static constexpr bool provide_base_method_for_iterators = std::ranges::forward_range<BaseT>;

        public:
            [[nodiscard]] static constexpr auto base_projection(const view_t& view) { return view.base().base(); }
            [[nodiscard]] static constexpr auto base_projection(view_t&& view) { return std::move(view).base().base(); }

            template<typename It>
            [[nodiscard]] static constexpr decltype(auto) iterator_base_projection(It&& it)
            {
                return std::forward<It>(it).base().base();
            }

            template<typename Sent>
            [[nodiscard]] static constexpr auto sentinel_base_projection(const Sent& sent)
            {
                return sent.base().base();
            }
        };

        template<typename View, decomposition_kind Kind>
        struct composing_normalization_traits
        {
        private:
            using view_t = to_nfc_view<View, Kind>;

        public:
            template<typename BaseT>
            static constexpr bool provide_base_method_for_iterators = std::ranges::forward_range<BaseT>;

        public:
            [[nodiscard]] static constexpr auto base_projection(const view_t& view) { return view.base(); }
            [[nodiscard]] static constexpr auto base_projection(view_t&& view) { return std::move(view).base(); }

            template<typename It>
            [[nodiscard]] static constexpr decltype(auto) iterator_base_projection(It&& it)
            {
                return std::forward<It>(it).base();
            }

            template<typename Sent>
            [[nodiscard]] static constexpr auto sentinel_base_projection(const Sent& sent)
            {
                return sent.base();
            }
        };

        template<typename View>
        using nfd_traits = decomposing_normalization_traits<View, decomposition_kind::canonical>;

        template<typename View>
        using nfkd_traits = decomposing_normalization_traits<View, decomposition_kind::compatibility>;

        template<typename View>
        using nfc_traits = composing_normalization_traits<View, decomposition_kind::canonical>;

        template<typename View>
        using nfkc_traits = composing_normalization_traits<View, decomposition_kind::compatibility>;

        template<typename View, normalization_form Form>
        using normalize_view_traits =
            std::conditional_t<Form == normalization_form::nfd, nfd_traits<View>,
                               std::conditional_t<Form == normalization_form::nfc, nfc_traits<View>,
                                                  std::conditional_t<Form == normalization_form::nfkd, nfkd_traits<View>, nfkc_traits<View>>>>;

        template<typename View>
        using nfd_base = canonically_order_view<decompose_view<View, decomposition_kind::canonical>>;

        template<typename View>
        using nfkd_base = canonically_order_view<decompose_view<View, decomposition_kind::compatibility>>;

        template<typename View>
        using nfc_base = to_nfc_view<View, decomposition_kind::canonical>;

        template<typename View>
        using nfkc_base = to_nfc_view<View, decomposition_kind::compatibility>;

        template<typename View, normalization_form Form>
        using normalize_view_base =
            std::conditional_t<Form == normalization_form::nfd, nfd_base<View>,
                               std::conditional_t<Form == normalization_form::nfc, nfc_base<View>,
                                                  std::conditional_t<Form == normalization_form::nfkd, nfkd_base<View>, nfkc_base<View>>>>;
    } // namespace impl::norm

    template<std::ranges::view View, normalization_form Form>
        requires code_point_range<View>
    class normalize_view
        : public impl::simple_view_adaptor<impl::norm::normalize_view_traits<View, Form>, impl::norm::normalize_view_base<View, Form>>
    {
    private:
        using view_t = impl::norm::normalize_view_base<View, Form>;
        using base_t = impl::simple_view_adaptor<impl::norm::normalize_view_traits<View, Form>, view_t>;

        static constexpr auto s_decomposition_kind =
            compatibility_normalization_form<Form> ? impl::norm::decomposition_kind::compatibility : impl::norm::decomposition_kind::canonical;

        struct dummy_t
        {
        };

        template<typename = dummy_t>
        using nfd_view_1 = impl::norm::decompose_view<View, s_decomposition_kind>;

        template<typename Dummy = dummy_t>
        using nfd_view_2 = impl::norm::canonically_order_view<nfd_view_1<Dummy>>;

        template<typename = dummy_t>
        using nfc_view = impl::norm::to_nfc_view<View, s_decomposition_kind>;

    public:
        /// @brief Default constructor.
        ///
        normalize_view()
            requires std::default_initializable<View>
        = default;

        /// @brief Constructs the `normalize_view` from the underlying view.
        ///
        constexpr explicit normalize_view(View base)
            : normalize_view(dummy_t{}, std::move(base))
        {
        }

        /// @brief Constructs the `normalize_view` from the underlying view.
        ///
        /// Tagged constructor for CTAD.
        ///
        constexpr normalize_view(View base, nontype_t<Form>)
            : normalize_view(dummy_t{}, std::move(base))
        {
        }

    private:
        constexpr normalize_view(dummy_t, View base)
            requires(Form == normalization_form::nfd || Form == normalization_form::nfkd)
            : base_t(nfd_view_2<dummy_t>(nfd_view_1<dummy_t>(std::move(base))))
        {
        }

        constexpr normalize_view(dummy_t, View base)
            requires(Form == normalization_form::nfc || Form == normalization_form::nfkc)
            : base_t(nfc_view<dummy_t>(std::move(base)))
        {
        }
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
        template<normalization_form Form>
        inline constexpr impl::normalize_fn<Form> normalize_to{};

        inline constexpr impl::normalize_fn<normalization_form::nfd>  normalize_to_nfd;
        inline constexpr impl::normalize_fn<normalization_form::nfc>  normalize_to_nfc;
        inline constexpr impl::normalize_fn<normalization_form::nfkd> normalize_to_nfkd;
        inline constexpr impl::normalize_fn<normalization_form::nfkc> normalize_to_nfkc;
    } // namespace views
} // namespace upp::ranges

/// @cond

template<typename View, upp::normalization_form Form>
inline constexpr bool std::ranges::enable_borrowed_range<upp::ranges::normalize_view<View, Form>> = std::ranges::enable_borrowed_range<View>;

/// @endcond

#endif // UNI_CPP_IMPL_RANGES_NORMALIZE_HPP