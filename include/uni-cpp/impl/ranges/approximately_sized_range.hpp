#ifndef UNI_CPP_IMPL_RANGES_APPROXIMATELY_SIZED_RANGE_HPP
#define UNI_CPP_IMPL_RANGES_APPROXIMATELY_SIZED_RANGE_HPP

/// @file
///
/// @brief Pre-C++26 implementation of [P2846R6](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2846r6.pdf).
///

#include "base.hpp"

#include <ranges>
#include <concepts>
#include <type_traits>
#include <iterator>
#include <cstddef>

namespace upp::ranges
{
    namespace impl::access
    {
        /// @brief Dummy range type defined for the integer_like concept below.
        ///
        /// Essentially integer_like should be defined as in https://eel.is/c++draft/iterator.concept.winc#def:integer-like.
        /// It mentions integer-class types, which there is no direct way to portably detect.
        ///
        /// However, there is a creative way to do it. Read https://eel.is/c++draft/range.prim.size.
        /// The important parts are:
        ///
        /// "Otherwise, if [...] auto(t.size()) is a valid expression of **integer-like type**, ranges​::​size(E) is expression-equivalent to auto(​t.size())." and
        /// "[Note 1: Diagnosable ill-formed cases above result in substitution failure when ranges​::​size(E) appears in the immediate context of a template instantiation. — end note]",
        ///
        /// which means that if there is a method size() in a range type R that returns T, T is an integer-like type if
        /// `requires(R& r) { std::ranges::size(r); }` or `std::ranges::sized_range<R>` is `true`.
        ///
        template<typename SizeType>
        struct dummy_sized_range
        {
            struct iterator
            {
                using value_type       = int;
                using reference        = int;
                using difference_type  = std::ptrdiff_t;
                using iterator_concept = std::input_iterator_tag;

                iterator& operator++();
                void      operator++(int);

                int  operator*() const;
                bool operator==(const iterator&) const;
            };

            iterator begin() const;
            iterator end() const;

            SizeType size() const;
        };

        /// @brief Implements the standard exposition-only [is-integer-like](https://eel.is/c++draft/iterator.concept.winc#def:integer-like) type trait.
        ///
        template<typename T>
        concept integer_like = std::ranges::sized_range<dummy_sized_range<std::remove_cv_t<T>>>;

        template<typename T>
        concept class_or_enum = std::is_class_v<T> || std::is_enum_v<T> || std::is_union_v<T>;

        template<typename T>
        concept ranges_size = requires(T& t) { std::ranges::size(t); };

        template<typename T>
        concept member_reserve_hint = requires(T& t) {
            { auto(t.reserve_hint()) } -> integer_like;
        };

        void reserve_hint() = delete;

        template<typename T>
        concept adl_reserve_hint = class_or_enum<std::remove_reference_t<T>> && requires(T& t) {
            { auto(reserve_hint(t)) } -> integer_like;
        };

        struct reserve_hint_t
        {
        public:
            template<typename R>
                requires ranges_size<R> || member_reserve_hint<R> || adl_reserve_hint<R>
            [[nodiscard]] constexpr auto operator()(R&& r) const noexcept(is_noexcept<R&>())
            {
                if constexpr (ranges_size<R>)
                    return std::ranges::size(r);
                else if constexpr (member_reserve_hint<R>)
                    return r.reserve_hint();
                else if constexpr (adl_reserve_hint<R>)
                    return reserve_hint(r);
            }

        private:
            template<typename R>
            [[nodiscard]] static constexpr bool is_noexcept()
            {
                if constexpr (ranges_size<R>)
                    return noexcept(std::ranges::size(std::declval<R&>()));
                else if constexpr (member_reserve_hint<R>)
                    return noexcept(auto(std::declval<R&>().reserve_hint()));
                else if constexpr (adl_reserve_hint<R>)
                    return noexcept(auto(reserve_hint(std::declval<R&>())));
            }
        };
    } // namespace impl::access

    UNI_CPP_IMPL_BEGIN_CPO_NAMESPACE

    /// @brief Estimates the number of elements in `range` in constant time.
    ///
    /// @par Call signature
    /// ```cpp
    /// template<typename Range>
    ///     requires // see below
    /// constexpr auto reserve_hint(Range&& range);
    /// ```
    ///
    /// Given a subexpression `E` with type `T`, let `t` be an lvalue that denotes the reified object for `E`.
    /// Then:
    /// - If [`std::ranges::size(E)`][1] is a valid expression, `upp::ranges::reserve_hint(E)` is expression-equivalent to [`std::ranges::size(E)`][1].
    /// - Otherwise, if `auto(t.reserve_hint())` is a valid expression of [integer-like type][2], `upp::ranges::reserve_hint(E)` is
    /// expression-equivalent to `auto(t.reserve_hint())`.
    /// - Otherwise, if `T` is a class or enumeration type and `auto(reserve_hint(t))` is a valid expression of [integer-like type][2] where
    /// the meaning of `reserve_hint` is established as-if by performing argument-dependent lookup only, then `upp::ranges::reserve_hint(E)` is expression-equivalent to that expression.
    /// - Otherwise, `upp::ranges::reserve_hint(E)` is ill-formed.
    ///
    /// Diagnosable ill-formed cases above result in substitution failure when `upp::ranges::reserve_hint(E)` appears in the immediate context of a template instantiation.
    ///
    /// @note Whenever `upp::ranges::reserve_hint(E)` is a valid expression, its type is [integer-like][2].
    ///
    /// The name `upp::ranges::reserve_hint` denotes a uni-cpp _customization point object_.
    ///
    /// Part of the uni-cpp pre-C++26 implementation of [P2846R6](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2846r6.pdf).
    ///
    /// @see upp::ranges::approximately_sized_range
    ///
    /// [1]: https://en.cppreference.com/w/cpp/ranges/size.html "std::ranges::size() on cppreference.com"
    /// [2]: https://eel.is/c++draft/iterator.concept.winc#def:integer-like "integer-like definition"
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    inline constexpr impl::access::reserve_hint_t reserve_hint{};

    UNI_CPP_IMPL_END_CPO_NAMESPACE

    /// @brief Determines whether the given `Range` type is a `std::ranges::range` that can approximate the number of its elements in amortized constant time.
    ///
    /// Part of the uni-cpp pre-C++26 implementation of [P2846R6](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2846r6.pdf).
    ///
    /// @see upp::ranges::reserve_hint
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<typename Range>
    concept approximately_sized_range = std::ranges::range<Range> && requires(Range& range) { upp::ranges::reserve_hint(range); };
} // namespace upp::ranges

#endif // UNI_CPP_IMPL_RANGES_APPROXIMATELY_SIZED_RANGE_HPP