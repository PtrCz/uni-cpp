#ifndef UNI_CPP_IMPL_RANGES_REVERSE_HPP
#define UNI_CPP_IMPL_RANGES_REVERSE_HPP

/// @file
///
/// @brief Defines a faster alternative to `std::views::reverse`.
///

#include "base.hpp"
#include "approximately_sized_range.hpp"
#include "view_interface.hpp"
#include "iterator_cache.hpp"

#include "../no_unique_address.hpp"

namespace upp::ranges
{
    namespace impl::reverse_view_impl
    {
        struct iterator_type_tag
        {
        };
    } // namespace impl::reverse_view_impl

    template<std::ranges::view View>
        requires std::ranges::bidirectional_range<View>
    class reverse_view : public UNI_CPP_IMPL_VIEW_INTERFACE(reverse_view<View>)
    {
    private:
        template<bool>
        class iterator;

        struct begin_t
        {
        };
        struct end_t
        {
        };

        template<typename>
        static constexpr bool const_iterable_impl = false;

        template<typename ConstView>
            requires std::ranges::bidirectional_range<ConstView>
        static constexpr bool const_iterable_impl<ConstView> =
            std::ranges::common_range<ConstView> || (std::ranges::random_access_range<ConstView> &&
                                                     std::sized_sentinel_for<std::ranges::sentinel_t<ConstView>, std::ranges::iterator_t<ConstView>>);

    public:
        static constexpr bool const_iterable = const_iterable_impl<const View>;

    public:
        reverse_view()
            requires std::default_initializable<View>
        = default;

        constexpr explicit reverse_view(View base)
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

        constexpr iterator<false> begin()
        {
            if constexpr (s_cache_end)
            {
                if (m_cached_end.has_value())
                    return iterator<false>(begin_t{}, m_cached_end.get(m_base), std::ranges::begin(m_base));
            }

            auto get_end_it = [&] {
                if constexpr (std::ranges::common_range<View>)
                {
                    return std::ranges::end(m_base);
                }
                else
                    return std::ranges::next(std::ranges::begin(m_base), std::ranges::end(m_base));
            };

            if constexpr (s_cache_end)
            {
                auto end_it = get_end_it();
                m_cached_end.set(m_base, end_it);

                return iterator<false>(begin_t{}, std::move(end_it), std::ranges::begin(m_base));
            }
            else
                return iterator<false>(begin_t{}, get_end_it(), std::ranges::begin(m_base));
        }

        constexpr iterator<true> begin() const
            requires const_iterable
        {
            if constexpr (std::ranges::common_range<const View>)
            {
                return iterator<true>(begin_t{}, std::ranges::end(m_base), std::ranges::begin(m_base));
            }
            else
                return iterator<true>(begin_t{}, std::ranges::next(std::ranges::begin(m_base), std::ranges::end(m_base)), std::ranges::begin(m_base));
        }

        constexpr iterator<false> end() { return iterator<false>(end_t{}, std::ranges::begin(m_base), std::ranges::begin(m_base)); }

        constexpr iterator<true> end() const
            requires const_iterable
        {
            return iterator<true>(end_t{}, std::ranges::begin(m_base), std::ranges::begin(m_base));
        }

        constexpr bool empty()
            requires impl::range_supports_empty<View>
        {
            return std::ranges::empty(m_base);
        }

        constexpr bool empty() const
            requires impl::range_supports_empty<const View>
        {
            return std::ranges::empty(m_base);
        }

        constexpr auto size()
            requires std::ranges::sized_range<View>
        {
            return std::ranges::size(m_base);
        }

        constexpr auto size() const
            requires std::ranges::sized_range<const View>
        {
            return std::ranges::size(m_base);
        }

        constexpr auto reserve_hint()
            requires approximately_sized_range<View>
        {
            return ranges::reserve_hint(m_base);
        }

        constexpr auto reserve_hint() const
            requires approximately_sized_range<const View>
        {
            return ranges::reserve_hint(m_base);
        }

    private:
        static constexpr bool s_cache_end =
            !(std::ranges::common_range<View> ||
              (std::ranges::random_access_range<View> && std::sized_sentinel_for<std::ranges::sentinel_t<View>, std::ranges::iterator_t<View>>));

        template<typename>
        struct iterator_category_impl
        {
        };

        template<typename It>
            requires requires { typename std::iterator_traits<It>::iterator_category; }
        struct iterator_category_impl<It>
        {
            using iterator_category = decltype([] {
                using category = std::iterator_traits<It>::iterator_category;

                if constexpr (std::derived_from<category, std::random_access_iterator_tag>)
                    return std::random_access_iterator_tag{};
                else
                    return category{};
            }());
        };

        template<bool Const>
        class iterator : public iterator_category_impl<std::ranges::iterator_t<impl::maybe_const<Const, View>>>,
                         private impl::reverse_view_impl::iterator_type_tag
        {
        private:
            using base_t = impl::maybe_const<Const, View>;

            friend reverse_view;

        public:
            using iterator_type = std::ranges::iterator_t<base_t>;

            using iterator_concept = decltype([] {
                if constexpr (std::ranges::random_access_range<base_t>)
                    return std::random_access_iterator_tag{};
                else
                    return std::bidirectional_iterator_tag{};
            }());

            using value_type      = std::iter_value_t<iterator_type>;
            using difference_type = std::iter_difference_t<iterator_type>;
            using reference       = std::iter_reference_t<iterator_type>;

        public:
            constexpr iterator() = default;

            /// @brief Constructs a `const` iterator from a non-`const` iterator.
            ///
            constexpr iterator(iterator<!Const> it)
                requires Const && std::convertible_to<std::ranges::iterator_t<View>, iterator_type>
                : m_current(std::move(it.m_current))
                , m_begin(std::move(it.m_begin))
                , m_is_at_reverse_end(it.m_is_at_reverse_end)
            {
            }

            /// @brief Returns the underlying iterator pointing to the position one past the current element.
            ///
            [[nodiscard]] constexpr iterator_type base() const
            {
                if (m_is_at_reverse_end)
                    return m_current;

                return std::ranges::next(m_current);
            }

            constexpr reference operator*() const { return *m_current; }

            constexpr auto operator->() const
                requires std::is_pointer_v<iterator_type> || requires(const iterator_type it) { it.operator->(); }
            {
                if constexpr (std::is_pointer_v<iterator_type>)
                {
                    return m_current;
                }
                else
                    return m_current.operator->();
            }

            constexpr iterator& operator++()
            {
                if (m_current != m_begin)
                    --m_current;
                else
                    m_is_at_reverse_end = true;

                return *this;
            }

            constexpr iterator operator++(int)
            {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            constexpr iterator& operator--()
            {
                if (m_is_at_reverse_end)
                    m_is_at_reverse_end = false;
                else
                    ++m_current;

                return *this;
            }

            constexpr iterator operator--(int)
            {
                auto tmp = *this;
                --*this;
                return tmp;
            }

            constexpr iterator& operator+=(difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                if (n > 0)
                {
                    std::ranges::advance(m_current, -n + 1);

                    if (m_current == m_begin)
                    {
                        m_is_at_reverse_end = true;
                    }
                    else
                        --m_current;
                }
                else if (n < 0)
                {
                    if (m_is_at_reverse_end)
                    {
                        m_is_at_reverse_end = false;
                    }
                    else
                        ++m_current;

                    std::ranges::advance(m_current, -n - 1);
                }

                return *this;
            }

            constexpr iterator& operator-=(difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                return *this += -n;
            }

            constexpr reference operator[](difference_type n) const
                requires std::ranges::random_access_range<base_t>
            {
                return *(*this + n);
            }

            friend constexpr bool operator==(const iterator& x, const iterator& y)
            {
                return x.m_current == y.m_current && x.m_is_at_reverse_end == y.m_is_at_reverse_end;
            }

            friend constexpr bool operator<(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<base_t>
            {
                if (x.m_is_at_reverse_end)
                    return false;

                if (y.m_is_at_reverse_end)
                    return true;

                return x.m_current > y.m_current;
            }

            friend constexpr bool operator>(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<base_t>
            {
                return y < x;
            }

            friend constexpr bool operator<=(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<base_t>
            {
                return !(y < x);
            }

            friend constexpr bool operator>=(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<base_t>
            {
                return !(x < y);
            }

            friend constexpr auto operator<=>(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<base_t> && std::three_way_comparable<iterator_type>
            {
                using ordering_t = decltype(y.m_current <=> x.m_current);

                if (x.m_is_at_reverse_end)
                {
                    if (y.m_is_at_reverse_end)
                        return ordering_t{std::strong_ordering::equal};

                    return ordering_t{std::strong_ordering::greater};
                }

                if (y.m_is_at_reverse_end)
                    return ordering_t{std::strong_ordering::less};

                return y.m_current <=> x.m_current;
            }

            friend constexpr iterator operator+(iterator it, difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                it += n;
                return it;
            }

            friend constexpr iterator operator+(difference_type n, iterator it)
                requires std::ranges::random_access_range<base_t>
            {
                it += n;
                return it;
            }

            friend constexpr iterator operator-(iterator it, difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                it -= n;
                return it;
            }

            friend constexpr difference_type operator-(const iterator& x, const iterator& y)
                requires std::sized_sentinel_for<iterator_type, iterator_type>
            {
                if (x.m_is_at_reverse_end)
                {
                    if (y.m_is_at_reverse_end)
                        return 0;

                    return y.m_current - x.m_current + 1;
                }

                if (y.m_is_at_reverse_end)
                    return y.m_current - x.m_current - 1;

                return y.m_current - x.m_current;
            }

        private:
            constexpr iterator(begin_t, iterator_type current, iterator_type begin)
                : m_current(std::move(current))
                , m_begin(std::move(begin))
            {
                if (m_current != m_begin)
                {
                    --m_current;
                    m_is_at_reverse_end = false;
                }
            }

            constexpr iterator(end_t, iterator_type current, iterator_type begin)
                : m_current(std::move(current))
                , m_begin(std::move(begin))
            {
            }

        private:
            iterator_type m_current = iterator_type();
            iterator_type m_begin   = iterator_type();

            bool m_is_at_reverse_end = true;
        };

    private:
        View m_base = View();

        UNI_CPP_IMPL_NO_UNIQUE_ADDRESS upp::impl::maybe_present<s_cache_end, impl::iterator_cache<View>> m_cached_end;
    };

    /// @cond

    template<typename Range>
    reverse_view(Range&&) -> reverse_view<std::views::all_t<Range>>;

    /// @endcond

    namespace impl
    {
        namespace reverse_view_impl
        {
            template<typename>
            inline constexpr bool is_reverse_view = false;

            template<typename View>
            inline constexpr bool is_reverse_view<reverse_view<View>> = true;

            template<typename It>
            inline constexpr bool is_reverse_view_iterator = std::is_base_of_v<iterator_type_tag, It>;

            template<typename>
            inline constexpr bool is_reverse_view_subrange = false;

            template<typename It, std::ranges::subrange_kind Kind>
            inline constexpr bool is_reverse_view_subrange<std::ranges::subrange<It, It, Kind>> = is_reverse_view_iterator<It>;

            template<typename>
            inline constexpr bool is_std_reverse_view = false;

            template<typename View>
            inline constexpr bool is_std_reverse_view<std::ranges::reverse_view<View>> = true;

            template<typename>
            inline constexpr bool is_std_reverse_iterator_subrange = false;

            template<typename It, std::ranges::subrange_kind Kind>
            inline constexpr bool
                is_std_reverse_iterator_subrange<std::ranges::subrange<std::reverse_iterator<It>, std::reverse_iterator<It>, Kind>> = true;
        } // namespace reverse_view_impl

        struct reverse_fn : public std::ranges::range_adaptor_closure<reverse_fn>
        {
        public:
            template<std::ranges::viewable_range Range>
                requires std::ranges::bidirectional_range<Range>
            [[nodiscard]] constexpr auto operator()(Range&& range) const
            {
                using range_t = std::remove_cvref_t<Range>;

                if constexpr (reverse_view_impl::is_reverse_view<range_t> || reverse_view_impl::is_std_reverse_view<range_t>)
                {
                    return std::forward<Range>(range).base();
                }
                else if constexpr (reverse_view_impl::is_reverse_view_subrange<range_t> ||
                                   reverse_view_impl::is_std_reverse_iterator_subrange<range_t>)
                {
                    using iterator_t = decltype(std::ranges::begin(range).base());

                    if constexpr (std::ranges::sized_range<range_t>)
                    {
                        return std::ranges::subrange<iterator_t, iterator_t, std::ranges::subrange_kind::sized>{
                            range.end().base(), range.begin().base(), range.size()
                        };
                    }
                    else
                    {
                        return std::ranges::subrange<iterator_t, iterator_t, std::ranges::subrange_kind::unsized>{
                            range.end().base(), range.begin().base()
                        };
                    }
                }
                else
                    return reverse_view{std::forward<Range>(range)};
            }
        };
    } // namespace impl

    namespace views
    {
        inline constexpr impl::reverse_fn reverse;
    } // namespace views
} // namespace upp::ranges

/// @cond

template<typename View>
inline constexpr bool std::ranges::enable_borrowed_range<upp::ranges::reverse_view<View>> = std::ranges::enable_borrowed_range<View>;

/// @endcond

#endif // UNI_CPP_IMPL_RANGES_REVERSE_HPP