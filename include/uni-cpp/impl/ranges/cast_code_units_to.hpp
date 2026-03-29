#ifndef UNI_CPP_IMPL_RANGES_CAST_CODE_UNITS_TO_HPP
#define UNI_CPP_IMPL_RANGES_CAST_CODE_UNITS_TO_HPP

/// @file
///
/// @brief Defines a range adaptor that casts code units from one type to another.
///

#include "base.hpp"
#include "approximately_sized_range.hpp"
#include "view_interface.hpp"
#include "valid_code_unit_range.hpp"

#include "../../encoding.hpp"

#include <iterator>
#include <concepts>
#include <bit>

namespace upp::ranges
{
    namespace impl::cast_code_units_to_view_impl
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
    } // namespace impl::cast_code_units_to_view_impl

    /// @brief A view that casts code units from the underlying range's element type to `ToType`.
    ///
    /// `std::bit_cast`s each code unit from the underlying view to the target code unit type.
    ///
    /// @tparam View Underlying view type.
    ///
    /// @tparam ToType Target code unit type.
    ///
    /// @note Users should use the @ref upp::views::cast_code_units_to "views::cast_code_units_to" range adaptor instead of using this type directly.
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<std::ranges::view View, code_unit_type ToType>
        requires code_unit_range<View> && (sizeof(ToType) == sizeof(std::remove_cvref_t<std::ranges::range_reference_t<View>>)) &&
                 std::same_as<ToType, std::remove_cv_t<ToType>>
    class cast_code_units_to_view : public UNI_CPP_IMPL_VIEW_INTERFACE(cast_code_units_to_view<View, ToType>)
    {
    private:
        template<bool>
        class iterator;

        template<bool>
        class sentinel;

        static constexpr bool const_enabled =
            code_unit_range<const View> &&
            std::same_as<std::remove_cvref_t<std::ranges::range_reference_t<View>>, std::remove_cvref_t<std::ranges::range_reference_t<const View>>>;

    public:
        /// @brief Default constructor.
        ///
        cast_code_units_to_view()
            requires std::default_initializable<View>
        = default;

        /// @brief Constructs the `cast_code_units_to_view` from the underlying view.
        ///
        constexpr explicit cast_code_units_to_view(View base)
            : m_base(std::move(base))
        {
        }

        /// @brief Constructs the `cast_code_units_to_view` from the underlying view.
        ///
        /// Tagged constructor for CTAD.
        ///
        constexpr cast_code_units_to_view(View base, type_tag_t<ToType>)
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
        constexpr iterator<false> begin() { return iterator<false>(std::ranges::begin(m_base)); }

        /// @brief Returns an iterator to the beginning of the range.
        ///
        constexpr iterator<true> begin() const
            requires const_enabled
        {
            return iterator<true>(std::ranges::begin(m_base));
        }

        /// @brief Returns a sentinel marking the end of the range.
        ///
        constexpr sentinel<false> end() { return sentinel<false>(std::ranges::end(m_base)); }

        /// @brief Returns an iterator marking the end of the range.
        ///
        constexpr iterator<false> end()
            requires std::ranges::common_range<View>
        {
            return iterator<false>(std::ranges::end(m_base));
        }

        /// @brief Returns a sentinel marking the end of the range.
        ///
        constexpr sentinel<true> end() const
            requires const_enabled
        {
            return sentinel<true>(std::ranges::end(m_base));
        }

        /// @brief Returns an iterator marking the end of the range.
        ///
        constexpr iterator<true> end() const
            requires std::ranges::common_range<const View> && const_enabled
        {
            return iterator<true>(std::ranges::end(m_base));
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
            requires impl::range_supports_empty<const View> && const_enabled
        {
            return std::ranges::empty(m_base);
        }

        /// @brief Returns the size of the range.
        ///
        constexpr auto size()
            requires std::ranges::sized_range<View>
        {
            return std::ranges::size(m_base);
        }

        /// @brief Returns the size of the range.
        ///
        constexpr auto size() const
            requires std::ranges::sized_range<const View> && const_enabled
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
            requires approximately_sized_range<const View> && const_enabled
        {
            return ranges::reserve_hint(m_base);
        }

    private:
        template<bool Const>
        class iterator : public impl::cast_code_units_to_view_impl::iterator_category_impl<View>
        {
        private:
            using parent_t = impl::maybe_const<Const, cast_code_units_to_view>;
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

            using value_type      = ToType;
            using reference_type  = value_type;
            using difference_type = std::ranges::range_difference_t<base_t>;

        public:
            /// @brief Default constructor.
            ///
            constexpr iterator()
                requires std::default_initializable<std::ranges::iterator_t<View>>
            = default;

            /// @brief Constructs a `const` iterator from a non-`const` iterator.
            ///
            constexpr iterator(iterator<!Const> i)
                requires Const && std::convertible_to<std::ranges::iterator_t<View>, std::ranges::iterator_t<base_t>>
                : m_current(std::move(i.m_current))
            {
            }

            /// @brief Returns a `const` reference to the underlying iterator.
            ///
            constexpr const std::ranges::iterator_t<base_t>& base() const& noexcept { return m_current; }

            /// @brief Returns the underlying iterator by moving it.
            ///
            constexpr std::ranges::iterator_t<base_t> base() && { return std::move(m_current); }

            /// @brief Dereferences the iterator.
            ///
            constexpr value_type operator*() const noexcept(noexcept(*m_current)) { return std::bit_cast<ToType>(*m_current); }

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
                return std::bit_cast<ToType>(m_current[n]);
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

            template<std::ranges::view View2, code_unit_type ToType2>
                requires code_unit_range<View2> && (sizeof(ToType2) == sizeof(std::remove_cvref_t<std::ranges::range_reference_t<View2>>)) &&
                         std::same_as<ToType2, std::remove_cv_t<ToType2>>
            friend class cast_code_units_to_view;

        private:
            std::ranges::iterator_t<base_t> m_current = std::ranges::iterator_t<base_t>();
        };

        template<bool Const>
        class sentinel
        {
        private:
            using parent_t = impl::maybe_const<Const, cast_code_units_to_view>;
            using base_t   = impl::maybe_const<Const, View>;

        public:
            /// @brief Default constructor.
            ///
            sentinel() = default;

            /// @brief Constructs a `const` sentinel from a non-`const` sentinel.
            ///
            constexpr explicit sentinel(sentinel<!Const> i)
                requires Const && std::convertible_to<std::ranges::sentinel_t<View>, std::ranges::sentinel_t<base_t>>
                : m_end(i.m_end)
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
                return x.m_current == y.m_end;
            }

            /// @brief Returns the distance between the iterator `x` and the sentinel `y`.
            ///
            template<bool OtherConst>
                requires std::sized_sentinel_for<std::ranges::sentinel_t<base_t>, std::ranges::iterator_t<impl::maybe_const<OtherConst, View>>>
            friend constexpr std::ranges::range_difference_t<impl::maybe_const<OtherConst, View>> operator-(const iterator<OtherConst>& x,
                                                                                                            const sentinel&             y)
            {
                return x.m_current - y.m_end;
            }

            /// @brief Returns the distance between the sentinel `y` and the iterator `x`.
            ///
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

            template<std::ranges::view View2, code_unit_type ToType2>
                requires code_unit_range<View2> && (sizeof(ToType2) == sizeof(std::remove_cvref_t<std::ranges::range_reference_t<View2>>)) &&
                         std::same_as<ToType2, std::remove_cv_t<ToType2>>
            friend class cast_code_units_to_view;

        private:
            std::ranges::sentinel_t<base_t> m_end = std::ranges::sentinel_t<base_t>();
        };

    private:
        View m_base = View();
    };

    /// @cond

    template<typename Range, typename ToType>
    cast_code_units_to_view(Range&&, type_tag_t<ToType>) -> cast_code_units_to_view<std::views::all_t<Range>, ToType>;

    template<typename View, typename ToType, encoding Encoding>
    inline constexpr bool enable_valid_code_unit_range<cast_code_units_to_view<View, ToType>, Encoding> =
        enable_valid_code_unit_range<std::remove_cvref_t<View>, Encoding>;

    /// @endcond

    namespace impl
    {
        template<code_unit_type ToType>
            requires std::same_as<ToType, std::remove_cv_t<ToType>>
        struct cast_code_units_to_fn : public std::ranges::range_adaptor_closure<cast_code_units_to_fn<ToType>>
        {
        public:
            template<std::ranges::viewable_range Range>
                requires code_unit_range<Range> && (sizeof(ToType) == sizeof(std::remove_cvref_t<std::ranges::range_reference_t<Range>>))
            [[nodiscard]] constexpr auto operator()(Range&& range) const
            {
                if constexpr (std::same_as<ToType, std::remove_cvref_t<std::ranges::range_reference_t<Range>>>)
                {
                    return std::views::all(std::forward<Range>(range));
                }
                else
                {
                    return cast_code_units_to_view(std::forward<Range>(range), type_tag<ToType>);
                }
            }
        };
    } // namespace impl

    namespace views
    {
        /// @brief Casts the code unit type of a code unit range to `ToType`.
        ///
        /// If the element type of the adapted range is `ToType`, it simply returns `std::views::all` of it.
        /// Otherwise, it returns `ranges::cast_code_units_to_view` which `std::bit_cast`s each element from
        /// the underlying range to the target code unit type (`ToType`).
        ///
        /// @tparam ToType Target code unit type. Must have the same size as the code unit type of the adapted range.
        ///
        /// @par Example
        ///
        /// @code{.cpp}
        ///
        /// // `str` is a range of `char`
        /// std::string_view str = ...;
        ///
        /// // `as_char8_t` is a range of `char8_t`
        /// auto as_char8_t = str | upp::views::cast_code_units_to<char8_t>;
        ///
        /// @endcode
        ///
        template<code_unit_type ToType>
            requires std::same_as<ToType, std::remove_cv_t<ToType>>
        inline constexpr impl::cast_code_units_to_fn<ToType> cast_code_units_to{};
    } // namespace views
} // namespace upp::ranges

/// @cond

template<typename View, typename ToType>
inline constexpr bool std::ranges::enable_borrowed_range<upp::ranges::cast_code_units_to_view<View, ToType>> = std::ranges::borrowed_range<View>;

/// @endcond

#endif // UNI_CPP_IMPL_RANGES_CAST_CODE_UNITS_TO_HPP