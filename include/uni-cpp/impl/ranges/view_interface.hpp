#ifndef UNI_CPP_IMPL_RANGES_VIEW_INTERFACE_HPP
#define UNI_CPP_IMPL_RANGES_VIEW_INTERFACE_HPP

/// @file
///
/// @brief Custom implementation of `std::ranges::view_interface`.
///

#include "base.hpp"

#include <ranges>
#include <type_traits>
#include <concepts>
#include <iterator>
#include <memory>

namespace upp::ranges::impl
{
    /// @brief Custom implementation of `std::ranges::view_interface`.
    ///
    /// This library uses its own implementation of `view_interface`, because the existing implementation
    /// in Clang is semi-broken. If you're interested, this reddit post explains the problem:
    /// https://www.reddit.com/r/cpp_questions/comments/1rpf904/is_clangs_ranges_implementation_still_broken/
    ///
    /// Instead of choosing the (what I believe to be) fragile solution of adding `.size()` and `.data()` to every view type,
    /// we use our own implementation of `view_interface` which does not suffer from the `static_assert(view<...>)` problem.
    /// This seems more stable as it does not depend on the current Clang implementation. It also prevents other methods
    /// like `empty()` and `operator[]` from breaking the view types.
    ///
    /// This implementation has been heavily inspired by the libstdc++'s implementation.
    ///
    template<typename Derived>
        requires std::is_class_v<Derived> && std::same_as<Derived, std::remove_cv_t<Derived>>
    class view_interface : public std::ranges::view_base // NOLINT(bugprone-crtp-constructor-accessibility)
    {
    private:
        constexpr Derived& derived() noexcept { return static_cast<Derived&>(*this); }

        constexpr const Derived& derived() const noexcept { return static_cast<const Derived&>(*this); }

        static constexpr bool noexcept_helper(bool) noexcept; // not defined

        template<typename Range>
        static constexpr bool empty_impl(Range& rg) noexcept(noexcept(noexcept_helper(std::ranges::begin(rg) == std::ranges::end(rg))))
        {
            return std::ranges::begin(rg) == std::ranges::end(rg);
        }

        template<typename Range>
        static constexpr auto size_impl(Range& rg) noexcept(noexcept(std::ranges::end(rg) - std::ranges::begin(rg)))
        {
            return std::ranges::end(rg) - std::ranges::begin(rg);
        }

    public:
        constexpr bool empty() noexcept(noexcept(empty_impl(derived())))
            requires std::ranges::forward_range<Derived> && (!std::ranges::sized_range<Derived>)
        {
            return empty_impl(derived());
        }

        constexpr bool empty() const noexcept(noexcept(empty_impl(derived())))
            requires std::ranges::forward_range<const Derived> && (!std::ranges::sized_range<const Derived>)
        {
            return empty_impl(derived());
        }

        constexpr bool empty() noexcept(noexcept(std::ranges::size(derived()) == 0))
            requires std::ranges::sized_range<Derived>
        {
            return std::ranges::size(derived()) == 0;
        }

        constexpr bool empty() const noexcept(noexcept(std::ranges::size(derived()) == 0))
            requires std::ranges::sized_range<const Derived>
        {
            return std::ranges::size(derived()) == 0;
        }

        constexpr explicit operator bool() noexcept(noexcept(std::ranges::empty(derived())))
            requires requires { std::ranges::empty(derived()); }
        {
            return !std::ranges::empty(derived());
        }

        constexpr explicit operator bool() const noexcept(noexcept(std::ranges::empty(derived())))
            requires requires { std::ranges::empty(derived()); }
        {
            return !std::ranges::empty(derived());
        }

        constexpr auto data() noexcept(noexcept(std::ranges::begin(derived())))
            requires std::contiguous_iterator<std::ranges::iterator_t<Derived>>
        {
            return std::to_address(std::ranges::begin(derived()));
        }

        constexpr auto data() const noexcept(noexcept(std::ranges::begin(derived())))
            requires std::ranges::range<const Derived> && std::contiguous_iterator<std::ranges::iterator_t<const Derived>>
        {
            return std::to_address(std::ranges::begin(derived()));
        }

        constexpr auto size() noexcept(noexcept(size_impl(derived())))
            requires std::ranges::forward_range<Derived> &&
                     std::sized_sentinel_for<std::ranges::sentinel_t<Derived>, std::ranges::iterator_t<Derived>>
        {
            return size_impl(derived());
        }

        constexpr auto size() const noexcept(noexcept(size_impl(derived())))
            requires std::ranges::forward_range<const Derived> &&
                     std::sized_sentinel_for<std::ranges::sentinel_t<const Derived>, std::ranges::iterator_t<const Derived>>
        {
            return size_impl(derived());
        }

        constexpr decltype(auto) front()
            requires std::ranges::forward_range<Derived>
        {
            // ASSERT(!empty());
            return *std::ranges::begin(derived());
        }

        constexpr decltype(auto) front() const
            requires std::ranges::forward_range<const Derived>
        {
            // ASSERT(!empty());
            return *std::ranges::begin(derived());
        }

        constexpr decltype(auto) back()
            requires std::ranges::bidirectional_range<Derived> && std::ranges::common_range<Derived>
        {
            // ASSERT(!empty());
            return *std::ranges::prev(std::ranges::end(derived()));
        }

        constexpr decltype(auto) back() const
            requires std::ranges::bidirectional_range<const Derived> && std::ranges::common_range<const Derived>
        {
            // ASSERT(!empty());
            return *std::ranges::prev(std::ranges::end(derived()));
        }

        template<std::ranges::random_access_range Range = Derived>
        constexpr decltype(auto) operator[](std::ranges::range_difference_t<Range> index)
        {
            return std::ranges::begin(derived())[index];
        }

        template<std::ranges::random_access_range Range = const Derived>
        constexpr decltype(auto) operator[](std::ranges::range_difference_t<Range> index) const
        {
            return std::ranges::begin(derived())[index];
        }

        constexpr auto cbegin()
            requires std::ranges::input_range<Derived>
        {
            return std::ranges::cbegin(derived());
        }

        constexpr auto cbegin() const
            requires std::ranges::input_range<const Derived>
        {
            return std::ranges::cbegin(derived());
        }

        constexpr auto cend()
            requires std::ranges::input_range<Derived>
        {
            return std::ranges::cend(derived());
        }

        constexpr auto cend() const
            requires std::ranges::input_range<const Derived>
        {
            return std::ranges::cend(derived());
        }
    };
} // namespace upp::ranges::impl

// To make the documentation easier, we lie about the `view_interface` class to doxygen.

#ifndef UNI_CPP_IMPL_DOXYGEN

#define UNI_CPP_IMPL_VIEW_INTERFACE(...) ::upp::ranges::impl::view_interface<__VA_ARGS__>

#else

#define UNI_CPP_IMPL_VIEW_INTERFACE(...) ::std::ranges::view_interface<__VA_ARGS__>

#endif

#endif // UNI_CPP_IMPL_RANGES_VIEW_INTERFACE_HPP