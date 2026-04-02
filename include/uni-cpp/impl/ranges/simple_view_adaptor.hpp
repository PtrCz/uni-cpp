#ifndef UNI_CPP_IMPL_RANGES_SIMPLE_VIEW_ADAPTOR_HPP
#define UNI_CPP_IMPL_RANGES_SIMPLE_VIEW_ADAPTOR_HPP

/// @file
///
/// @brief Provides a base class for defining simple view adaptors.
///

#include "base.hpp"
#include "approximately_sized_range.hpp"
#include "view_interface.hpp"

#include <iterator>
#include <type_traits>
#include <utility>
#include <concepts>

namespace upp::ranges::impl
{
    namespace simple_view_adaptor_impl
    {
        template<typename T>
        using with_reference = T&;

        template<typename T>
        concept can_reference = requires { typename with_reference<T>; };
    } // namespace simple_view_adaptor_impl

    /// @brief Customizable base class for defining a simple view adaptor.
    ///
    /// This type provides a reusable implementation for simple view adaptors, for example,
    /// view adaptors which transform values on a 1-to-1 basis. It is customizable via the @p Traits
    /// template parameter, which is a type, acting as a namespace, that contains all of the
    /// functions corresponding to the actions which the user wants to customize.
    ///
    /// ### `Traits` template parameter
    ///
    /// A class type describing adaptor customization points. It serves as a stateless customization namespace.
    /// Here is a list of optionally recognized members:
    ///
    /// - `transform_element`. If provided, it will be called with the result of dereferening the underlying iterator.
    ///   The result of the `transform_element` function will be used as the value of dereferencing the iterator of this view.
    ///   It maps values from the underlying range.
    ///   If the `transform_element` function is provided for the `std::ranges::range_reference_t<View>` type,
    ///   it should be defined for the `std::ranges::range_reference_t<const View>` type as well,
    ///   as long as `std::ranges::input_range<const View>` is `true`.
    ///
    /// - `base_projection`. If provided, the result of the `.base()` members of this view will be projected through it.
    ///   That is, the return value of `.base()` will be `base_projection(m_base)` where `m_base` can be `const&` or `&&`.
    ///   The `const&` overload should make a copy of the base view, and the rvalue overload should construct the base view by moving it.
    ///
    /// - `iterator_base_projection`. If provided, the result of the `.base()` members of the iterator type of this view will be
    ///   projected through it. That is, the return value of `.begin().base()` will be `iterator_base_projection(m_current)`
    ///   where `m_current` can be `const&` or `&&`. Importantly, the `const&` overload should return a `const&` of the base iterator.
    ///   The rvalue overload should return the base iterator by moving it.
    ///
    /// - `sentinel_base_projection`. If provided, the result of the `.base()` member of the sentinel type of this view will be
    ///   projected through it. That is, the return value of `.end().base()` (for non-`common_range`s) will be `sentinel_base_projection(m_end)`.
    ///   Unlike the other projection functions, this one has only a single overload --- a copying one.
    ///
    /// All of these are optional and not every one of them must be provided. These functions should be defined as `static`.
    /// They can be template functions (where the template parameters are deduced from the functions arguments) and can have multiple overloads defined.
    ///
    /// The `base_projection` functions are useful for defining a single simple view composed of multiple `simple_view_adaptor`s, where
    /// the public API exposes it only as a single view. That way, even though the type is composed from multiple adaptors, the `.base()`
    /// method can return the type that is the base view as far as the public API of that view is concerned.
    ///
    /// @tparam Traits Customizes the behavior of this adaptor.
    ///
    /// @tparam View The underlying view. This is **NOT** the `Derived` class.
    ///         It is the `View` template parameter of the `Derived` class.
    ///
    /// It provides all of the standard range operations available for the underlying range.
    ///
    /// @note This type should only be used as a base class for a view adaptor. It should not be used directly.
    ///
    template<typename Traits, std::ranges::view View>
        requires std::is_class_v<Traits> && std::same_as<Traits, std::remove_cv_t<Traits>> && std::ranges::input_range<View>
    class simple_view_adaptor : public UNI_CPP_IMPL_VIEW_INTERFACE(simple_view_adaptor<Traits, View>)
    {
    private:
        template<bool Const>
        class iterator;

        template<bool Const>
        class sentinel;

        template<typename Arg>
        static constexpr bool is_transform_element_invocable_with = requires(Arg&& arg) {
            { Traits::transform_element(std::forward<Arg>(arg)) } -> simple_view_adaptor_impl::can_reference;
        };

        static_assert(!std::ranges::input_range<const View> || is_transform_element_invocable_with<std::ranges::range_reference_t<View>> ==
                                                                   is_transform_element_invocable_with<std::ranges::range_reference_t<const View>>);

        static constexpr bool has_transform_element = is_transform_element_invocable_with<std::ranges::range_reference_t<View>>;

        static constexpr bool has_base_projection = requires { Traits::base_projection(std::declval<const View&>()); };

        template<bool Const>
        using base_t = maybe_const<Const, View>;

    public:
        /// @brief Default constructor.
        ///
        simple_view_adaptor()
            requires std::default_initializable<View>
        = default;

        /// @brief Constructs the `simple_view_adaptor` from the underlying view.
        ///
        constexpr explicit simple_view_adaptor(View base)
            : m_base(std::move(base))
        {
        }

        /// @brief Returns a copy of the underlying view.
        ///
        constexpr auto base() const&
            requires std::copy_constructible<View>
        {
            if constexpr (has_base_projection)
            {
                return Traits::base_projection(m_base);
            }
            else
                return m_base;
        }

        /// @brief Returns the underlying view by moving it.
        ///
        constexpr auto base() &&
        {
            if constexpr (has_base_projection)
            {
                return Traits::base_projection(std::move(m_base));
            }
            else
                return std::move(m_base);
        }

        /// @brief Returns an iterator to the beginning of the range.
        ///
        constexpr iterator<false> begin() { return iterator<false>(std::ranges::begin(m_base)); }

        /// @brief Returns an iterator to the beginning of the range.
        ///
        constexpr iterator<true> begin() const
            requires std::ranges::input_range<const View>
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
            requires std::ranges::input_range<const View>
        {
            return sentinel<true>(std::ranges::end(m_base));
        }

        /// @brief Returns an iterator marking the end of the range.
        ///
        constexpr iterator<true> end() const
            requires std::ranges::common_range<const View> && std::ranges::input_range<const View>
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
            requires impl::range_supports_empty<const View>
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
            requires std::ranges::sized_range<const View>
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
            requires approximately_sized_range<const View>
        {
            return ranges::reserve_hint(m_base);
        }

    private:
        template<bool>
        struct iterator_category_impl
        {
        };

        template<bool Const>
            requires std::ranges::forward_range<base_t<Const>>
        struct iterator_category_impl<Const>
        {
        private:
            [[nodiscard]] static consteval auto impl() noexcept
            {
                using base_t = simple_view_adaptor::base_t<Const>;

                using category = std::iterator_traits<std::ranges::iterator_t<base_t>>::iterator_category;

                if constexpr (has_transform_element)
                {
                    using result_t = decltype(Traits::transform_element(std::declval<std::ranges::range_reference_t<base_t>>()));

                    if constexpr (std::is_reference_v<result_t>)
                    {
                        if constexpr (std::derived_from<category, std::contiguous_iterator_tag>)
                        {
                            return std::random_access_iterator_tag{};
                        }
                        else
                            return category{};
                    }
                    else
                        return std::input_iterator_tag{};
                }
                else
                {
                    if constexpr (std::derived_from<category, std::contiguous_iterator_tag>)
                    {
                        return std::random_access_iterator_tag{};
                    }
                    else
                        return category{};
                }
            }

        public:
            using iterator_category = decltype(impl());
        };

        // Note: can't use `std::conditional_t` for this, because `Traits::transform_element` is a conditionally valid expression.
        template<bool Const, bool HasTransformElement>
        struct value_type_impl
        {
        private:
            using base_t = impl::maybe_const<Const, View>;

        public:
            using value_type = std::remove_cvref_t<decltype(Traits::transform_element(std::declval<std::ranges::range_reference_t<base_t>>()))>;
        };

        template<bool Const>
        struct value_type_impl<Const, false>
        {
        private:
            using base_t = impl::maybe_const<Const, View>;

        public:
            using value_type = std::ranges::range_value_t<base_t>;
        };

        template<bool Const>
        class iterator : public iterator_category_impl<Const>, public value_type_impl<Const, has_transform_element>
        {
        private:
            using parent_t = impl::maybe_const<Const, simple_view_adaptor>;
            using base_t   = impl::maybe_const<Const, View>;

            static constexpr bool has_iterator_base_projection =
                requires { Traits::iterator_base_projection(std::declval<const std::ranges::iterator_t<base_t>&>()); };

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
            constexpr const auto& base() const&
            {
                if constexpr (has_iterator_base_projection)
                {
                    using result_t = decltype(Traits::iterator_base_projection(m_current));

                    static_assert(std::same_as<result_t, std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<result_t>>>>,
                                  "`Traits::iterator_base_projection`s `const&` overload must return a const reference");

                    return Traits::iterator_base_projection(m_current);
                }
                else
                    return m_current;
            }

            /// @brief Returns the underlying iterator by moving it.
            ///
            constexpr auto base() &&
            {
                if constexpr (has_iterator_base_projection)
                {
                    return Traits::iterator_base_projection(std::move(m_current));
                }
                else
                    return std::move(m_current);
            }

            /// @brief Dereferences the iterator.
            ///
            constexpr decltype(auto) operator*() const
            {
                if constexpr (has_transform_element)
                {
                    return Traits::transform_element(*m_current);
                }
                else
                    return *m_current;
            }

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

            constexpr decltype(auto) operator[](difference_type n) const
                requires std::ranges::random_access_range<base_t>
            {
                if constexpr (has_transform_element)
                {
                    return Traits::transform_element(m_current[n]);
                }
                else
                    return m_current[n];
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

            template<typename Traits2, std::ranges::view View2>
                requires std::is_class_v<Traits2> && std::same_as<Traits2, std::remove_cv_t<Traits2>> && std::ranges::input_range<View2>
            friend class simple_view_adaptor;

        private:
            std::ranges::iterator_t<base_t> m_current = std::ranges::iterator_t<base_t>();
        };

        template<bool Const>
        class sentinel
        {
        private:
            using parent_t = impl::maybe_const<Const, simple_view_adaptor>;
            using base_t   = impl::maybe_const<Const, View>;

            static constexpr bool has_sentinel_base_projection =
                requires { Traits::sentinel_base_projection(std::declval<std::ranges::sentinel_t<base_t>>()); };

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
            constexpr auto base() const
            {
                if constexpr (has_sentinel_base_projection)
                {
                    return Traits::sentinel_base_projection(m_end);
                }
                else
                    return m_end;
            }

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

            template<typename Traits2, std::ranges::view View2>
                requires std::is_class_v<Traits2> && std::same_as<Traits2, std::remove_cv_t<Traits2>> && std::ranges::input_range<View2>
            friend class simple_view_adaptor;

        private:
            std::ranges::sentinel_t<base_t> m_end = std::ranges::sentinel_t<base_t>();
        };

    private:
        View m_base = View();
    };
} // namespace upp::ranges::impl

#endif // UNI_CPP_IMPL_RANGES_SIMPLE_VIEW_ADAPTOR_HPP