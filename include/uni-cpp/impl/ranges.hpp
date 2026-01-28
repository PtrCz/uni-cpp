#ifndef UNI_CPP_IMPL_RANGES_HPP
#define UNI_CPP_IMPL_RANGES_HPP

/// @file
///
/// @brief Pre-C++26 implementation of P2846R6.
///

#include <ranges>
#include <concepts>
#include <type_traits>

namespace upp::impl::ranges
{
    namespace impl::access
    {
        /// @brief Dummy range type defined for the integer_like concept below.
        ///
        /// Essentially integer_like should be defined as in https://eel.is/c++draft/iterator.concept.winc#def:integer-like.
        /// Now, std::integral<T> is easy and filtering out the cv bool is no problem either.
        /// But it also mentions integer-class types, which (AFAIK) there is no way to portably detect.
        ///
        /// However, there is a creative way to do it. Basically you need to read https://eel.is/c++draft/range.prim.size.
        /// The important parts are:
        ///
        /// "Otherwise, if [...] auto(t.size()) is a valid expression of **integer-like type**, ranges​::​size(E) is expression-equivalent to auto(​t.size())." and
        /// "[Note 1: Diagnosable ill-formed cases above result in substitution failure when ranges​::​size(E) appears in the immediate context of a template instantiation. — end note]",
        ///
        /// which means that if there is a method size() in a range type R that returns T, T is an integer-like type if
        /// `requires(R& r) { std::ranges::size(r); }` is `true`, or `std::ranges::sized_range<R>` is `true`.
        ///
        template<typename SizeType>
        struct dummy_sized_range
        {
            struct iterator
            {
                using difference_type = int;

                iterator& operator++();
                void      operator++(int);

                int  operator*() const;
                bool operator==(iterator) const;
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
        };
    } // namespace impl::access

    inline namespace impl_cpo
    {
        /// @brief Pre-C++26 implementation of P2846R6.
        ///
        inline constexpr ranges::impl::access::reserve_hint_t reserve_hint{};
    } // namespace impl_cpo

    /// @brief Pre-C++26 implementation of P2846R6.
    ///
    template<typename Range>
    concept approximately_sized_range = std::ranges::range<Range> && requires(Range& range) { ranges::reserve_hint(range); };
} // namespace upp::impl::ranges

#endif // UNI_CPP_IMPL_RANGES_HPP