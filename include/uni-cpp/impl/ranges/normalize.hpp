#ifndef UNI_CPP_IMPL_RANGES_NORMALIZE_HPP
#define UNI_CPP_IMPL_RANGES_NORMALIZE_HPP

/// @file
///
/// @brief Defines range adaptors for normalizing ranges of code points.
///

#include "base.hpp"
#include "approximately_sized_range.hpp"
#include "view_interface.hpp"

#include "../../uchar.hpp"
#include "../unicode_data/decomposition.hpp"
#include "../normalization_form.hpp"

#include "../small_vector.hpp"
#include "../inplace_vector.hpp"

#include <bit>
#include <optional>
#include <type_traits>

namespace upp::ranges
{
    namespace impl::norm
    {
        enum class decompose_view_kind : std::uint8_t
        {
            canonical,
            compatibility
        };

        template<std::ranges::view View, decompose_view_kind Kind>
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

            constexpr iterator<false> begin() { return iterator<false>(*this, std::ranges::begin(m_base)); }

            constexpr iterator<true> begin() const
                requires std::ranges::range<const View> && code_point_range<const View>
            {
                return iterator<true>(*this, std::ranges::begin(m_base));
            }

            constexpr sentinel<false> end() { return sentinel<false>(std::ranges::end(m_base)); }

            constexpr iterator<false> end()
                requires std::ranges::common_range<View>
            {
                return iterator<false>(*this, std::ranges::end(m_base));
            }

            constexpr sentinel<true> end() const
                requires std::ranges::range<const View> && code_point_range<const View>
            {
                return sentinel<true>(std::ranges::end(m_base));
            }

            constexpr iterator<true> end() const
                requires std::ranges::common_range<const View> && code_point_range<const View>
            {
                return iterator<true>(*this, std::ranges::end(m_base));
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
                    requires std::default_initializable<std::ranges::iterator_t<View>>
                = default;

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
                constexpr iterator(parent_t& parent, std::ranges::iterator_t<base_t> begin)
                    : m_current(std::move(begin))
                    , m_parent(std::addressof(parent))
                {
                    if (base() != end())
                    {
                        m_buffer = decomposition_of(*m_current);
                    }
                }

                constexpr std::ranges::sentinel_t<base_t> end() const { return std::ranges::end(m_parent->m_base); }

                [[nodiscard]] static constexpr auto decomposition_of(uchar code_point) noexcept
                {
                    if constexpr (Kind == decompose_view_kind::canonical)
                    {
                        return upp::impl::unicode_data::decomposition::lookup_decomposition<
                            upp::impl::unicode_data::decomposition::decomposition_kind::canonical>(code_point.value());
                    }
                    else if constexpr (Kind == decompose_view_kind::compatibility)
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

                    if (m_current != end())
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
                parent_t*                       m_parent  = nullptr;

                upp::impl::inplace_vector<uchar, 18> m_buffer{};
                std::uint8_t                         m_buffer_index = 0;

                template<std::ranges::view View2, decompose_view_kind Kind2>
                    requires code_point_range<View2>
                friend class decompose_view;
            };

            template<bool Const>
            class sentinel
            {
            private:
                using parent_t = impl::maybe_const<Const, decompose_view>;
                using base_t   = impl::maybe_const<Const, View>;

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

                template<std::ranges::view View2, decompose_view_kind Kind2>
                    requires code_point_range<View2>
                friend class decompose_view;
            };

        private:
            View m_base = View();
        };

        template<typename Range, decompose_view_kind Kind>
        decompose_view(Range&&, nontype_t<Kind>) -> decompose_view<std::views::all_t<Range>, Kind>;

        using signed_size_t = decltype(0z);

        template<std::ranges::view View>
            requires code_point_range<View>
        class canonically_order_view : public UNI_CPP_IMPL_VIEW_INTERFACE(canonically_order_view<View>)
        {
        private:
            template<bool>
            class iterator;

            template<bool>
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

            constexpr iterator<false> begin() { return iterator<false>(*this, std::ranges::begin(m_base)); }

            constexpr iterator<true> begin() const
                requires code_point_range<const View>
            {
                return iterator<true>(*this, std::ranges::begin(m_base));
            }

            constexpr sentinel<false> end() { return sentinel<false>(std::ranges::end(m_base)); }

            constexpr iterator<false> end()
                requires std::ranges::common_range<View>
            {
                return iterator<false>(*this, std::ranges::end(m_base));
            }

            constexpr sentinel<true> end() const
                requires code_point_range<const View>
            {
                return sentinel<true>(std::ranges::end(m_base));
            }

            constexpr iterator<true> end() const
                requires std::ranges::common_range<const View> && code_point_range<const View>
            {
                return iterator<true>(*this, std::ranges::end(m_base));
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
            template<bool Const>
            class iterator : public impl::input_iterator_category_impl<View>
            {
            private:
                using parent_t = impl::maybe_const<Const, canonically_order_view>;
                using base_t   = impl::maybe_const<Const, View>;

            public:
                using iterator_concept = decltype(impl::bidirectional_range_iterator_concept_impl<base_t>());

                using value_type      = uchar;
                using difference_type = std::ptrdiff_t;

            public:
                constexpr iterator()
                    requires std::default_initializable<std::ranges::iterator_t<View>>
                = default;

                constexpr const std::ranges::iterator_t<base_t>& base() const& noexcept { return m_current; }

                constexpr std::ranges::iterator_t<base_t> base() && { return std::move(m_current); }

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
                    requires std::ranges::bidirectional_range<base_t>
                {
                    if (m_buffer_index == 0z)
                        read_backwards();
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
                            m_buffer_index = s_buffer_index_at_sentinel;
                        }
                    }
                }

                constexpr std::ranges::iterator_t<base_t> begin() const
                    requires std::ranges::bidirectional_range<base_t>
                {
                    return std::ranges::begin(m_parent->m_base);
                }

                constexpr std::ranges::sentinel_t<base_t> end() const { return std::ranges::end(m_parent->m_base); }

                constexpr void advance_underlying()
                {
                    if constexpr (std::ranges::forward_range<base_t>)
                        m_current = *m_advance_to;

                    if (m_current != end())
                    {
                        read();
                    }
                    else
                    {
                        if constexpr (std::ranges::forward_range<base_t>)
                            m_buffer_index = 0z;
                        else
                            m_buffer_index = s_buffer_index_at_sentinel;
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

                constexpr void read_impl(std::ranges::iterator_t<base_t>& it, const std::ranges::sentinel_t<base_t>& last)
                {
                    m_buffer.clear();
                    m_buffer_index = 0z;

                    const uchar first = *it;

                    m_buffer.emplace_back(first);

                    ++it;

                    if (first.canonical_combining_class() == 0)
                    {
                        if constexpr (std::ranges::forward_range<base_t>)
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

                    if constexpr (std::ranges::forward_range<base_t>)
                        m_advance_to = std::move(it);
                }

                /// @brief Advances the underlying iterator the necessary amount and updates the buffer.
                ///
                constexpr void read()
                {
                    impl::iterator_guard<std::ranges::iterator_t<base_t>> guard{m_current, m_current};
                    read_impl(m_current, end());
                }

                constexpr void reverse_buffer() { std::ranges::reverse(m_buffer); }

                /// @brief Moves the underlying iterator backwards the necessary amount and updates the buffer.
                ///
                constexpr void read_backwards()
                    requires std::ranges::bidirectional_range<base_t>
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

                    const auto beg = begin();

                    for (; m_current != beg;)
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
                std::ranges::iterator_t<base_t> m_current = std::ranges::iterator_t<base_t>();
                parent_t*                       m_parent  = nullptr;

                upp::impl::small_vector<uchar, 32u> m_buffer{};
                signed_size_t                       m_buffer_index = 0z;

                std::optional<std::ranges::iterator_t<base_t>> m_advance_to;

            private:
                // NOLINTNEXTLINE(bugprone-signed-char-misuse)
                static constexpr signed_size_t s_buffer_index_at_sentinel = static_cast<signed_size_t>(impl::buffer_index_at_sentinel);

                template<std::ranges::view View2>
                    requires code_point_range<View2>
                friend class canonically_order_view;
            };

            template<bool Const>
            class sentinel
            {
            private:
                using parent_t = impl::maybe_const<Const, canonically_order_view>;
                using base_t   = impl::maybe_const<Const, View>;

                // NOLINTNEXTLINE(bugprone-signed-char-misuse)
                static constexpr signed_size_t s_buffer_index_at_sentinel = static_cast<signed_size_t>(impl::buffer_index_at_sentinel);

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
                    if constexpr (std::ranges::forward_range<base_t>)
                    {
                        return x.m_current == y.m_end;
                    }
                    else
                    {
                        return x.m_current == y.m_end && x.m_buffer_index == s_buffer_index_at_sentinel;
                    }
                }

            private:
                constexpr explicit sentinel(std::ranges::sentinel_t<base_t> end)
                    : m_end{end}
                {
                }

                template<std::ranges::view View2>
                    requires code_point_range<View2>
                friend class canonically_order_view;
            };

        private:
            View m_base = View();
        };

        template<std::ranges::view View>
            requires code_point_range<View>
        class canonically_compose_view : public UNI_CPP_IMPL_VIEW_INTERFACE(canonically_compose_view<View>)
        {
        private:
            template<bool>
            class iterator;

            template<bool>
            class sentinel;

        public:
            canonically_compose_view()
                requires std::default_initializable<View>
            = default;

            constexpr explicit canonically_compose_view(View base)
                : m_base(std::move(base))
            {
            }

            constexpr View base() const&
                requires std::copy_constructible<View>
            {
                return m_base;
            }

            constexpr View base() && { return std::move(m_base); }

            constexpr iterator<false> begin() { return iterator<false>(*this, std::ranges::begin(m_base)); }

            constexpr iterator<true> begin() const
                requires code_point_range<const View>
            {
                return iterator<true>(*this, std::ranges::begin(m_base));
            }

            constexpr sentinel<false> end() { return sentinel<false>(std::ranges::end(m_base)); }

            constexpr iterator<false> end()
                requires std::ranges::common_range<View>
            {
                return iterator<false>(*this, std::ranges::end(m_base));
            }

            constexpr sentinel<true> end() const
                requires code_point_range<const View>
            {
                return sentinel<true>(std::ranges::end(m_base));
            }

            constexpr iterator<true> end() const
                requires std::ranges::common_range<const View> && code_point_range<const View>
            {
                return iterator<true>(*this, std::ranges::end(m_base));
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
                using parent_t = impl::maybe_const<Const, canonically_compose_view>;
                using base_t   = impl::maybe_const<Const, View>;

            public:
                using iterator_concept = decltype(impl::bidirectional_range_iterator_concept_impl<base_t>());

                using value_type      = uchar;
                using difference_type = std::ptrdiff_t;

            public:
                constexpr iterator()
                    requires std::default_initializable<std::ranges::iterator_t<View>>
                = default;

                constexpr const std::ranges::iterator_t<base_t>& base() const& noexcept { return m_current; }

                constexpr std::ranges::iterator_t<base_t> base() && { return std::move(m_current); }

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
                    requires std::ranges::bidirectional_range<base_t>
                {
                    if (m_buffer_index == 0z)
                        read_backwards();
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
                            m_buffer_index = s_buffer_index_at_sentinel;
                        }
                    }
                }

                constexpr std::ranges::iterator_t<base_t> begin() const
                    requires std::ranges::bidirectional_range<base_t>
                {
                    return std::ranges::begin(m_parent->m_base);
                }

                constexpr std::ranges::sentinel_t<base_t> end() const { return std::ranges::end(m_parent->m_base); }

                constexpr void advance_underlying()
                {
                    if constexpr (std::ranges::forward_range<base_t>)
                        m_current = *m_advance_to;

                    if (m_current != end())
                    {
                        read();
                    }
                    else
                    {
                        if constexpr (std::ranges::forward_range<base_t>)
                            m_buffer_index = 0z;
                        else
                            m_buffer_index = s_buffer_index_at_sentinel;
                    }
                }

                /// @pre The expression `m_buffer[0].canonical_combining_class() == 0` does not invoke UB and evaluates to `true`.
                ///
                constexpr void canonically_compose_the_buffer()
                {
                    auto last_starter_it = m_buffer.begin();
                    auto it              = m_buffer.begin() + 1;

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

                constexpr void read_impl(std::ranges::iterator_t<base_t>& it, const std::ranges::sentinel_t<base_t>& last)
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

                        if constexpr (std::ranges::forward_range<base_t>)
                            m_advance_to = std::move(it);

                        return;
                    }

                    for (; it != last; ++it)
                    {
                        const uchar code_point = *it;

                        if (code_point.canonical_combining_class() == 0 && code_point.nfc_quick_check() == quick_check::yes)
                            break;

                        m_buffer.emplace_back(code_point);
                    }

                    canonically_compose_the_buffer();

                    if constexpr (std::ranges::forward_range<base_t>)
                        m_advance_to = std::move(it);
                }

                /// @brief Advances the underlying iterator the necessary amount and updates the buffer.
                ///
                constexpr void read()
                {
                    impl::iterator_guard<std::ranges::iterator_t<base_t>> guard{m_current, m_current};
                    read_impl(m_current, end());
                }

                constexpr void reverse_buffer() { std::ranges::reverse(m_buffer); }

                /// @brief Moves the underlying iterator backwards the necessary amount and updates the buffer.
                ///
                constexpr void read_backwards()
                    requires std::ranges::bidirectional_range<base_t>
                {
                    m_buffer.clear();
                    m_advance_to = m_current;

                    std::optional<std::ranges::iterator_t<base_t>> last_starter;
                    std::size_t                                    code_points_since_last_starter = 0uz;

                    const auto beg = begin();

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
                            if (code_point.nfc_quick_check() == quick_check::yes)
                            {
                                reverse_buffer();
                                canonically_compose_the_buffer();

                                m_buffer_index = m_buffer.size() - 1uz;

                                return;
                            }

                            code_points_since_last_starter = 0uz;
                            last_starter                   = m_current;
                        }
                        else
                            ++code_points_since_last_starter;

                    } while (m_current != beg);

                    // No starter with `qc == yes` found.
                    // Try to stop at the last encountered starter.

                    if (last_starter)
                    {
                        m_current = *last_starter;
                        m_buffer.resize(m_buffer.size() - code_points_since_last_starter);

                        reverse_buffer();
                        canonically_compose_the_buffer();

                        m_buffer_index = m_buffer.size() - 1uz;
                    }
                    else
                    {
                        // No starter found. There is no need to canonically compose the buffer.

                        reverse_buffer();
                        m_buffer_index = m_buffer.size() - 1uz;
                    }
                }

            private:
                std::ranges::iterator_t<base_t> m_current = std::ranges::iterator_t<base_t>();
                parent_t*                       m_parent  = nullptr;

                upp::impl::small_vector<uchar, 32u> m_buffer{};
                signed_size_t                       m_buffer_index = 0z;

                std::optional<std::ranges::iterator_t<base_t>> m_advance_to;

            private:
                // NOLINTNEXTLINE(bugprone-signed-char-misuse)
                static constexpr signed_size_t s_buffer_index_at_sentinel = static_cast<signed_size_t>(impl::buffer_index_at_sentinel);

                template<std::ranges::view View2>
                    requires code_point_range<View2>
                friend class canonically_compose_view;
            };

            template<bool Const>
            class sentinel
            {
            private:
                using parent_t = impl::maybe_const<Const, canonically_compose_view>;
                using base_t   = impl::maybe_const<Const, View>;

                // NOLINTNEXTLINE(bugprone-signed-char-misuse)
                static constexpr signed_size_t s_buffer_index_at_sentinel = static_cast<signed_size_t>(impl::buffer_index_at_sentinel);

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
                    if constexpr (std::ranges::forward_range<base_t>)
                    {
                        return x.m_current == y.m_end;
                    }
                    else
                    {
                        return x.m_current == y.m_end && x.m_buffer_index == s_buffer_index_at_sentinel;
                    }
                }

            private:
                constexpr explicit sentinel(std::ranges::sentinel_t<base_t> end)
                    : m_end{end}
                {
                }

                template<std::ranges::view View2>
                    requires code_point_range<View2>
                friend class canonically_compose_view;
            };

        private:
            View m_base = View();
        };

        template<typename View, decompose_view_kind Kind>
        struct decomposing_normalization_traits
        {
        private:
            using view_t = canonically_order_view<decompose_view<View, Kind>>;

            using iter_t       = std::ranges::iterator_t<view_t&>;
            using const_iter_t = std::ranges::iterator_t<const view_t&>;

            using sent_t       = std::ranges::sentinel_t<view_t&>;
            using const_sent_t = std::ranges::sentinel_t<const view_t&>;

        public:
            [[nodiscard]] static constexpr auto base_projection(const view_t& view) { return view.base().base(); }
            [[nodiscard]] static constexpr auto base_projection(view_t&& view) { return std::move(view).base().base(); }

            [[nodiscard]] static constexpr const auto& iterator_base_projection(const iter_t& it) { return it.base().base(); }
            [[nodiscard]] static constexpr auto        iterator_base_projection(iter_t&& it) { return std::move(it).base().base(); }

            [[nodiscard]] static constexpr const auto& iterator_base_projection(const const_iter_t& it) { return it.base().base(); }
            [[nodiscard]] static constexpr auto        iterator_base_projection(const_iter_t&& it) { return std::move(it).base().base(); }

            [[nodiscard]] static constexpr auto sentinel_base_projection(const sent_t& sent) { return sent.base().base(); }
            [[nodiscard]] static constexpr auto sentinel_base_projection(const const_sent_t& sent) { return sent.base().base(); }
        };

        template<typename View, decompose_view_kind Kind>
        struct composing_normalization_traits
        {
        private:
            using view_t = canonically_compose_view<canonically_order_view<decompose_view<View, Kind>>>;

            using iter_t       = std::ranges::iterator_t<view_t&>;
            using const_iter_t = std::ranges::iterator_t<const view_t&>;

            using sent_t       = std::ranges::sentinel_t<view_t&>;
            using const_sent_t = std::ranges::sentinel_t<const view_t&>;

        public:
            [[nodiscard]] static constexpr auto base_projection(const view_t& view) { return view.base().base().base(); }
            [[nodiscard]] static constexpr auto base_projection(view_t&& view) { return std::move(view).base().base().base(); }

            [[nodiscard]] static constexpr const auto& iterator_base_projection(const iter_t& it) { return it.base().base().base(); }
            [[nodiscard]] static constexpr auto        iterator_base_projection(iter_t&& it) { return std::move(it).base().base().base(); }

            [[nodiscard]] static constexpr const auto& iterator_base_projection(const const_iter_t& it) { return it.base().base().base(); }
            [[nodiscard]] static constexpr auto        iterator_base_projection(const_iter_t&& it) { return std::move(it).base().base().base(); }

            [[nodiscard]] static constexpr auto sentinel_base_projection(const sent_t& sent) { return sent.base().base().base(); }
            [[nodiscard]] static constexpr auto sentinel_base_projection(const const_sent_t& sent) { return sent.base().base().base(); }
        };

        template<typename View>
        using nfd_traits = decomposing_normalization_traits<View, decompose_view_kind::canonical>;

        template<typename View>
        using nfkd_traits = decomposing_normalization_traits<View, decompose_view_kind::compatibility>;

        template<typename View>
        using nfc_traits = composing_normalization_traits<View, decompose_view_kind::canonical>;

        template<typename View>
        using nfkc_traits = composing_normalization_traits<View, decompose_view_kind::compatibility>;

        template<typename View, normalization_form Form>
        using normalize_view_traits =
            std::conditional_t<Form == normalization_form::nfd, nfd_traits<View>,
                               std::conditional_t<Form == normalization_form::nfc, nfc_traits<View>,
                                                  std::conditional_t<Form == normalization_form::nfkd, nfkd_traits<View>, nfkc_traits<View>>>>;

        template<typename View>
        using nfd_base = canonically_order_view<decompose_view<View, decompose_view_kind::canonical>>;

        template<typename View>
        using nfkd_base = canonically_order_view<decompose_view<View, decompose_view_kind::compatibility>>;

        template<typename View>
        using nfc_base = canonically_compose_view<canonically_order_view<decompose_view<View, decompose_view_kind::canonical>>>;

        template<typename View>
        using nfkc_base = canonically_compose_view<canonically_order_view<decompose_view<View, decompose_view_kind::compatibility>>>;

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

        static constexpr auto s_decompose_view_kind =
            compatibility_normalization_form<Form> ? impl::norm::decompose_view_kind::compatibility : impl::norm::decompose_view_kind::canonical;

        using view_1 = impl::norm::decompose_view<View, s_decompose_view_kind>;
        using view_2 = impl::norm::canonically_order_view<view_1>;

        struct dummy_t
        {
        };

        template<typename = dummy_t>
        using view_3 = impl::norm::canonically_compose_view<view_2>;

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
            : base_t(view_2(view_1(std::move(base))))
        {
        }

        constexpr normalize_view(dummy_t, View base)
            requires(Form == normalization_form::nfc || Form == normalization_form::nfkc)
            : base_t(view_3<dummy_t>(view_2(view_1(std::move(base)))))
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

#endif // UNI_CPP_IMPL_RANGES_NORMALIZE_HPP