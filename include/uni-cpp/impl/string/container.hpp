#ifndef UNI_CPP_IMPL_STRING_CONTAINER_HPP
#define UNI_CPP_IMPL_STRING_CONTAINER_HPP

#include "../ranges/approximately_sized_range.hpp"

#include <concepts>
#include <ranges>
#include <utility>
#include <initializer_list>

namespace upp
{
    /// @brief Concept for identifying container types.
    ///
    /// The requirements of this concept are similar to those of [C++'s named requirements](https://en.cppreference.com/w/cpp/named_req/Container).
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<typename C>
    concept container =
        requires(C c, const C cc) {
            typename C::value_type;
            typename C::iterator;
            typename C::const_iterator;
            typename C::size_type;

            { c.begin() } -> std::same_as<typename C::iterator>;
            { c.end() } -> std::same_as<typename C::iterator>;
            { cc.begin() } -> std::same_as<typename C::const_iterator>;
            { cc.end() } -> std::same_as<typename C::const_iterator>;

            { c.size() } -> std::same_as<typename C::size_type>;
            { c.empty() } -> std::same_as<bool>;
        } && std::unsigned_integral<typename C::size_type> && std::default_initializable<C> && std::copy_constructible<C> &&
        std::assignable_from<C&, const C> && std::equality_comparable<C>;

    /// @brief Concept for identifying contiguous sequence container types.
    ///
    /// The requirements of this concept are similar to those of C++'s named requirements for
    /// [SequenceContainer](https://en.cppreference.com/w/cpp/named_req/SequenceContainer.html)s and
    /// [ContiguousContainer](https://en.cppreference.com/w/cpp/named_req/ContiguousContainer.html)s.
    ///
    /// @headerfile "" <uni-cpp/string.hpp>
    ///
    template<typename C>
    concept contiguous_sequence_container = container<C> && std::ranges::contiguous_range<C> &&

                                            std::constructible_from<C, typename C::iterator, typename C::iterator> &&
                                            std::constructible_from<C, std::initializer_list<typename C::value_type>> &&

                                            requires(C c, const C cc, typename C::size_type n, typename C::value_type v, typename C::iterator it,
                                                     typename C::const_iterator cit, std::initializer_list<typename C::value_type> il) {
                                                { c.assign(n, v) };
                                                { c.assign(it, it) };
                                                { c.assign(il) };

                                                { c.insert(cit, v) } -> std::same_as<typename C::iterator>;
                                                { c.insert(cit, n, v) } -> std::same_as<typename C::iterator>;
                                                { c.insert(cit, it, it) } -> std::same_as<typename C::iterator>;

                                                { c.erase(cit) } -> std::same_as<typename C::iterator>;
                                                { c.erase(cit, cit) } -> std::same_as<typename C::iterator>;

                                                { c.clear() };

                                                { c[n] } -> std::same_as<typename C::value_type&>;
                                                { cc[n] } -> std::same_as<const typename C::value_type&>;

                                                { c.data() } -> std::same_as<typename C::value_type*>;
                                                { cc.data() } -> std::same_as<const typename C::value_type*>;
                                            };

    template<typename C>
    concept reservable_container = container<C> && requires(C& c, const C& cc, typename C::size_type n) {
        c.reserve(n);
        { cc.capacity() } -> std::same_as<decltype(n)>;
        { cc.max_size() } -> std::same_as<decltype(n)>;
    };

    namespace impl
    {
        /// @brief Operations on contiguous sequence containers.
        ///
        namespace csc
        {
        }
    } // namespace impl

    namespace impl::csc
    {
        template<contiguous_sequence_container C, typename V>
        constexpr void push_back(C& cont, V&& value)
        {
            if constexpr (requires { cont.push_back(std::forward<V>(value)); })
            {
                cont.push_back(std::forward<V>(value));
            }
            else if constexpr (requires { cont.emplace_back(std::forward<V>(value)); })
            {
                cont.emplace_back(std::forward<V>(value));
            }
            else
            {
                cont.insert(std::as_const(cont).end(), std::forward<V>(value));
            }
        }

        template<typename R>
        concept can_common = requires(R&& rg) { std::views::common(std::forward<R>(rg)); };

        template<typename>
        struct common_iter
        {
            using type = void; // Must be a valid type; is not used
        };

        template<can_common R>
        struct common_iter<R>
        {
            using type = std::ranges::iterator_t<decltype(std::views::common(std::declval<R>()))>;
        };

        template<typename C>
        concept uses_allocator = requires(const C cont) {
            typename C::allocator_type;
            { cont.get_allocator() } -> std::same_as<typename C::allocator_type>;
        };

        template<contiguous_sequence_container C, std::ranges::input_range R, typename... Args>
        [[nodiscard]] constexpr C construct_from_range(R&& rg, Args&&... args)
        {
            // Similar to std::ranges::to, but uses ranges::reserve_hint in C++23

            using common_iter_t = common_iter<R&&>::type;

            if constexpr (std::constructible_from<C, R&&, Args...>)
            {
                return C(std::forward<R>(rg), std::forward<Args>(args)...);
            }
            else if constexpr (std::constructible_from<C, std::from_range_t, R&&, Args...>)
            {
                return C(std::from_range, std::forward<R>(rg), std::forward<Args>(args)...);
            }
            else if constexpr (std::constructible_from<C, std::ranges::iterator_t<R&>, std::ranges::sentinel_t<R&>, Args...>)
            {
                return C(std::ranges::begin(rg), std::ranges::end(rg), std::forward<Args>(args)...);
            }
            else if constexpr (can_common<R&&> && std::constructible_from<C, common_iter_t, common_iter_t, Args...>)
            {
                auto common = std::views::common(std::forward<R>(rg));

                return C(std::ranges::begin(common), std::ranges::end(common), std::forward<Args>(args)...);
            }
            else
            {
                // rg is at most an input_range

                C cont(std::forward<Args>(args)...);

                if constexpr (ranges::approximately_sized_range<R> && reservable_container<C>)
                {
                    cont.reserve(static_cast<typename C::size_type>(ranges::reserve_hint(rg)));
                }

                for (auto&& value : std::forward<R>(rg))
                    push_back(cont, std::forward<decltype(value)>(value));

                return cont;
            }
        }

        template<contiguous_sequence_container C, std::ranges::input_range R>
        constexpr typename C::iterator insert_range(C& cont, typename C::const_iterator pos, R&& rg)
        {
            using common_iter_t = common_iter<R&&>::type;

            if constexpr (requires { cont.insert_range(pos, std::forward<R>(rg)); })
            {
                return cont.insert_range(pos, std::forward<R>(rg));
            }
            else if constexpr (requires(std::ranges::iterator_t<R&> it, std::ranges::sentinel_t<R&> sent) { cont.insert(pos, it, sent); })
            {
                return cont.insert(pos, std::ranges::begin(rg), std::ranges::end(rg));
            }
            else if constexpr (can_common<R&&> && requires(common_iter_t it) { cont.insert(pos, it, it); })
            {
                auto common = std::views::common(std::forward<R>(rg));

                return cont.insert(pos, std::ranges::begin(common), std::ranges::end(common));
            }
            else
            {
                // Note: We must collect the range here.
                //       We can't try to insert each element ourselves, it has to be an "atomic" operation on the container itself,
                //       otherwise we couldn't provide the strong exception safety guarantee.
                //       This is a rare case anyway, given all of the branches above.

                if constexpr (uses_allocator<C>)
                {
                    auto collected = construct_from_range<C, R&&, typename C::allocator_type>(std::forward<R>(rg), cont.get_allocator());

                    return cont.insert(pos, collected.begin(), collected.end());
                }
                else
                {
                    auto collected = construct_from_range<C, R&&>(std::forward<R>(rg));

                    return cont.insert(pos, collected.begin(), collected.end());
                }
            }
        }

        template<contiguous_sequence_container C, std::ranges::input_range R>
        constexpr void append_range(C& cont, R&& rg)
        {
            using common_iter_t = common_iter<R&&>::type;

            if constexpr (requires { cont.append_range(std::forward<R>(rg)); })
            {
                cont.append_range(std::forward<R>(rg));
            }
            else if constexpr (requires(std::ranges::iterator_t<R&> it, std::ranges::sentinel_t<R&> sent) { cont.append(it, sent); })
            {
                cont.append(std::ranges::begin(rg), std::ranges::end(rg));
            }
            else if constexpr (can_common<R&&> && requires(common_iter_t it) { cont.append(it, it); })
            {
                auto common = std::views::common(std::forward<R>(rg));

                cont.append(std::ranges::begin(common), std::ranges::end(common));
            }
            else
            {
                insert_range(cont, std::as_const(cont).end(), std::forward<R>(rg));
            }
        }

        template<contiguous_sequence_container C, std::ranges::input_range R>
        constexpr void assign_range(C& cont, R&& rg)
        {
            using common_iter_t = common_iter<R&&>::type;

            if constexpr (requires { cont.assign_range(std::forward<R>(rg)); })
            {
                cont.assign_range(std::forward<R>(rg));
            }
            else if constexpr (requires(std::ranges::iterator_t<R&> it, std::ranges::sentinel_t<R&> sent) { cont.assign(it, sent); })
            {
                cont.assign(std::ranges::begin(rg), std::ranges::end(rg));
            }
            else if constexpr (can_common<R&&> && requires(common_iter_t it) { cont.assign(it, it); })
            {
                auto common = std::views::common(std::forward<R>(rg));

                cont.assign(std::ranges::begin(common), std::ranges::end(common));
            }
            else
            {
                // Note: We must collect the range here.
                //       We can't try to insert each element ourselves, it has to be an "atomic" operation on the container itself,
                //       otherwise we couldn't provide the strong exception safety guarantee.
                //       This is a rare case anyway, given all of the branches above.

                if constexpr (uses_allocator<C>)
                {
                    auto collected = construct_from_range<C, R&&, typename C::allocator_type>(std::forward<R>(rg), cont.get_allocator());

                    cont.assign(collected.begin(), collected.end());
                }
                else
                {
                    auto collected = construct_from_range<C, R&&>(std::forward<R>(rg));

                    cont.assign(collected.begin(), collected.end());
                }
            }
        }

        template<std::ranges::range R>
        [[nodiscard]] constexpr decltype(auto) to_viewable_range(R&& rg)
        {
            if constexpr (std::ranges::viewable_range<R>)
            {
                return std::forward<R>(rg);
            }
            else
            {
                return std::ranges::ref_view{rg};
            }
        }
    } // namespace impl::csc
} // namespace upp

#endif // UNI_CPP_IMPL_STRING_CONTAINER_HPP