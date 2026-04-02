#ifndef UNI_CPP_IMPL_RANGES_AS_EXPECTED_RANGE_HPP
#define UNI_CPP_IMPL_RANGES_AS_EXPECTED_RANGE_HPP

/// @file
///
/// @brief Defines a range adaptor that transforms a range to a range of `std::expected` with the original ranges values.
///

#include "base.hpp"
#include "approximately_sized_range.hpp"
#include "view_interface.hpp"

#include "../../encoding.hpp"

#include <iterator>
#include <expected>

namespace upp::ranges::impl
{
    namespace as_expected_view_impl
    {
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

                if constexpr (std::same_as<category, std::contiguous_iterator_tag>)
                {
                    return std::random_access_iterator_tag{};
                }
                else
                {
                    return category{};
                }
            }

        public:
            using iterator_category = decltype(impl());
        };
    } // namespace as_expected_view_impl

    template<std::ranges::view View, typename ErrorType>
        requires std::ranges::input_range<View> && std::same_as<ErrorType, std::remove_cvref_t<ErrorType>>
    class as_expected_view : public UNI_CPP_IMPL_VIEW_INTERFACE(as_expected_view<View, ErrorType>)
    {
    private:
        template<bool>
        class iterator;

        template<bool>
        class sentinel;

    public:
        as_expected_view()
            requires std::default_initializable<View>
        = default;

        constexpr explicit as_expected_view(View base)
            : m_base(std::move(base))
        {
        }

        /// @details Tagged constructor for CTAD.
        ///
        constexpr as_expected_view(View base, type_tag_t<ErrorType>)
            : m_base(std::move(base))
        {
        }

        constexpr View base() const&
            requires std::copy_constructible<View>
        {
            return m_base;
        }

        constexpr View base() && { return std::move(m_base); }

        constexpr iterator<false> begin() { return iterator<false>(std::ranges::begin(m_base)); }

        constexpr iterator<true> begin() const
            requires std::ranges::input_range<const View>
        {
            return iterator<true>(std::ranges::begin(m_base));
        }

        constexpr sentinel<false> end() { return sentinel<false>(std::ranges::end(m_base)); }

        constexpr iterator<false> end()
            requires std::ranges::common_range<View>
        {
            return iterator<false>(std::ranges::end(m_base));
        }

        constexpr sentinel<true> end() const
            requires std::ranges::input_range<const View>
        {
            return sentinel<true>(std::ranges::end(m_base));
        }

        constexpr iterator<true> end() const
            requires std::ranges::common_range<const View> && std::ranges::input_range<const View>
        {
            return iterator<true>(std::ranges::end(m_base));
        }

        constexpr bool empty()
            requires impl::range_supports_empty<View>
        {
            return std::ranges::empty(m_base);
        }

        constexpr bool empty() const
            requires impl::range_supports_empty<const View> && std::ranges::input_range<const View>
        {
            return std::ranges::empty(m_base);
        }

        constexpr auto size()
            requires std::ranges::sized_range<View>
        {
            return std::ranges::size(m_base);
        }

        constexpr auto size() const
            requires std::ranges::sized_range<const View> && std::ranges::input_range<const View>
        {
            return std::ranges::size(m_base);
        }

        constexpr auto reserve_hint()
            requires approximately_sized_range<View>
        {
            return ranges::reserve_hint(m_base);
        }

        constexpr auto reserve_hint() const
            requires approximately_sized_range<const View> && std::ranges::input_range<const View>
        {
            return ranges::reserve_hint(m_base);
        }

    private:
        template<bool Const>
        class iterator : public impl::as_expected_view_impl::iterator_category_impl<View>
        {
        private:
            using parent_t = impl::maybe_const<Const, as_expected_view>;
            using base_t   = impl::maybe_const<Const, View>;

            [[nodiscard]] static consteval auto iterator_concept_impl() noexcept
            {
                if constexpr (std::ranges::random_access_range<base_t>)
                {
                    return std::random_access_iterator_tag{};
                }
                else if constexpr (std::ranges::bidirectional_range<base_t>)
                {
                    return std::bidirectional_iterator_tag{};
                }
                else if constexpr (std::ranges::forward_range<base_t>)
                {
                    return std::forward_iterator_tag{};
                }
                else
                    return std::input_iterator_tag{};
            }

        public:
            using iterator_concept = decltype(iterator_concept_impl());

            using value_type      = std::expected<std::remove_cvref_t<std::ranges::range_reference_t<base_t>>, ErrorType>;
            using reference_type  = value_type;
            using difference_type = std::ranges::range_difference_t<base_t>;

        public:
            constexpr iterator()
                requires std::default_initializable<std::ranges::iterator_t<View>>
            = default;

            constexpr iterator(iterator<!Const> i)
                requires Const && std::convertible_to<std::ranges::iterator_t<View>, std::ranges::iterator_t<base_t>>
                : m_current(std::move(i.m_current))
            {
            }

            constexpr const std::ranges::iterator_t<base_t>& base() const& noexcept { return m_current; }

            constexpr std::ranges::iterator_t<base_t> base() && { return std::move(m_current); }

            constexpr value_type operator*() const { return value_type{std::in_place, *m_current}; }

            constexpr iterator& operator++()
            {
                ++m_current;
                return *this;
            }

            constexpr void     operator++(int) { ++m_current; }
            constexpr iterator operator++(int)
                requires std::ranges::forward_range<base_t>
            {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            constexpr iterator& operator--()
                requires std::ranges::bidirectional_range<base_t>
            {
                --m_current;
                return *this;
            }
            constexpr iterator operator--(int)
                requires std::ranges::bidirectional_range<base_t>
            {
                auto tmp = *this;
                --*this;
                return tmp;
            }

            constexpr iterator& operator+=(difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                m_current += n;
                return *this;
            }
            constexpr iterator& operator-=(difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                m_current -= n;
                return *this;
            }

            constexpr value_type operator[](difference_type n) const
                requires std::ranges::random_access_range<base_t>
            {
                return value_type{std::in_place, m_current[n]};
            }

            friend constexpr bool operator==(const iterator& x, const iterator& y)
                requires std::equality_comparable<std::ranges::iterator_t<base_t>>
            {
                return x.m_current == y.m_current;
            }

            friend constexpr bool operator<(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<base_t>
            {
                return x.m_current < y.m_current;
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
                requires std::ranges::random_access_range<base_t> && std::three_way_comparable<std::ranges::iterator_t<base_t>>
            {
                return x.m_current <=> y.m_current;
            }

            friend constexpr iterator operator+(iterator i, difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                return iterator{i.m_current + n};
            }
            friend constexpr iterator operator+(difference_type n, iterator i)
                requires std::ranges::random_access_range<base_t>
            {
                return iterator{i.m_current + n};
            }

            friend constexpr iterator operator-(iterator i, difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                return iterator{i.m_current - n};
            }

            friend constexpr difference_type operator-(const iterator& x, const iterator& y)
                requires std::sized_sentinel_for<std::ranges::iterator_t<base_t>, std::ranges::iterator_t<base_t>>
            {
                return x.m_current - y.m_current;
            }

        private:
            constexpr iterator(std::ranges::iterator_t<base_t> current)
                : m_current(std::move(current))
            {
            }

            template<std::ranges::view View2, typename ErrorType2>
                requires std::ranges::input_range<View2> && std::same_as<ErrorType2, std::remove_cvref_t<ErrorType2>>
            friend class as_expected_view;

        private:
            std::ranges::iterator_t<base_t> m_current = std::ranges::iterator_t<base_t>();
        };

        template<bool Const>
        class sentinel
        {
        private:
            using parent_t = impl::maybe_const<Const, as_expected_view>;
            using base_t   = impl::maybe_const<Const, View>;

        public:
            sentinel() = default;

            constexpr explicit sentinel(sentinel<!Const> i)
                requires Const && std::convertible_to<std::ranges::sentinel_t<View>, std::ranges::sentinel_t<base_t>>
                : m_end(i.m_end)
            {
            }

            constexpr std::ranges::sentinel_t<base_t> base() const { return m_end; }

            template<bool OtherConst>
                requires std::sentinel_for<std::ranges::sentinel_t<base_t>, std::ranges::iterator_t<impl::maybe_const<OtherConst, View>>>
            friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y)
            {
                return x.m_current == y.m_end;
            }

            template<bool OtherConst>
                requires std::sized_sentinel_for<std::ranges::sentinel_t<base_t>, std::ranges::iterator_t<impl::maybe_const<OtherConst, View>>>
            friend constexpr std::ranges::range_difference_t<impl::maybe_const<OtherConst, View>> operator-(const iterator<OtherConst>& x,
                                                                                                            const sentinel&             y)
            {
                return x.m_current - y.m_end;
            }

            template<bool OtherConst>
                requires std::sized_sentinel_for<std::ranges::sentinel_t<base_t>, std::ranges::iterator_t<impl::maybe_const<OtherConst, View>>>
            friend constexpr std::ranges::range_difference_t<impl::maybe_const<OtherConst, View>> operator-(const sentinel&             y,
                                                                                                            const iterator<OtherConst>& x)
            {
                return y.m_end - x.m_current;
            }

        private:
            constexpr explicit sentinel(std::ranges::sentinel_t<base_t> end)
                : m_end{end}
            {
            }

            friend sentinel<!Const>;

            template<std::ranges::view View2, typename ErrorType2>
                requires std::ranges::input_range<View2> && std::same_as<ErrorType2, std::remove_cvref_t<ErrorType2>>
            friend class as_expected_view;

        private:
            std::ranges::sentinel_t<base_t> m_end = std::ranges::sentinel_t<base_t>();
        };

    private:
        View m_base = View();
    };

    /// @cond

    template<typename Range, typename ErrorType>
    as_expected_view(Range&&, type_tag_t<ErrorType>) -> as_expected_view<std::views::all_t<Range>, ErrorType>;

    /// @endcond

    template<typename ErrorType>
        requires std::same_as<ErrorType, std::remove_cvref_t<ErrorType>>
    struct as_expected_range_fn : public std::ranges::range_adaptor_closure<as_expected_range_fn<ErrorType>>
    {
    public:
        template<std::ranges::viewable_range Range>
            requires std::ranges::input_range<Range>
        [[nodiscard]] constexpr auto operator()(Range&& range) const
        {
            return as_expected_view(std::forward<Range>(range), type_tag<ErrorType>);
        }
    };

    /// @brief Makes a range of `std::expected` values from the adapted range.
    ///
    /// Each value from the adapted range is mapped to a `std::expected` value with the same value.
    /// The `std::expected` type is `std::expected<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, ErrorType>`.
    ///
    template<typename ErrorType>
        requires std::same_as<ErrorType, std::remove_cvref_t<ErrorType>>
    inline constexpr impl::as_expected_range_fn<ErrorType> as_expected_range{};
} // namespace upp::ranges::impl

/// @cond

template<typename View, typename ErrorType>
inline constexpr bool std::ranges::enable_borrowed_range<upp::ranges::impl::as_expected_view<View, ErrorType>> = std::ranges::borrowed_range<View>;

/// @endcond

#endif // UNI_CPP_IMPL_RANGES_AS_EXPECTED_RANGE_HPP