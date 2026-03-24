#ifndef TEST_RANGES_TO_INPUT_HPP
#define TEST_RANGES_TO_INPUT_HPP

#include <uni-cpp/ranges.hpp>

#include <ranges>
#include <iterator>
#include <concepts>

#include "base.hpp"

namespace upp_test::ranges
{
    namespace impl
    {
        template<typename R>
        concept simple_view =
            std::ranges::view<R> && std::ranges::range<const R> && std::same_as<std::ranges::iterator_t<R>, std::ranges::iterator_t<const R>> &&
            std::same_as<std::ranges::sentinel_t<R>, std::ranges::sentinel_t<const R>>;

        template<bool Const, typename T>
        using maybe_const = std::conditional_t<Const, const T, T>;
    } // namespace impl

    /// @brief Converts a view into a non-forward view.
    ///
    /// Pre-C++26 implementation of P3137R3.
    ///
    template<std::ranges::input_range View>
        requires std::ranges::view<View>
    class to_input_view : public std::ranges::view_interface<to_input_view<View>>
    {
    private:
        template<bool>
        class iterator;

    public:
        to_input_view()
            requires std::default_initializable<View>
        = default;

        constexpr explicit to_input_view(View base)
            : m_base(std::move(base))
        {
        }

        constexpr View base() const&
            requires std::copy_constructible<View>
        {
            return m_base;
        }

        constexpr View base() && { return std::move(m_base); }

        constexpr auto begin()
            requires(!impl::simple_view<View>)
        {
            return iterator<false>(std::ranges::begin(m_base));
        }

        constexpr auto begin() const
            requires std::ranges::range<const View>
        {
            return iterator<true>(std::ranges::begin(m_base));
        }

        constexpr auto end()
            requires(!impl::simple_view<View>)
        {
            return std::ranges::end(m_base);
        }

        constexpr auto end() const
            requires std::ranges::range<const View>
        {
            return std::ranges::end(m_base);
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
            requires upp::ranges::approximately_sized_range<View>
        {
            return upp::ranges::reserve_hint(m_base);
        }

        constexpr auto reserve_hint() const
            requires upp::ranges::approximately_sized_range<const View>
        {
            return upp::ranges::reserve_hint(m_base);
        }

    private:
        template<bool Const>
        class iterator
        {
        private:
            using base_t = impl::maybe_const<Const, View>;

        public:
            using difference_type  = std::ranges::range_difference_t<base_t>;
            using value_type       = std::ranges::range_value_t<base_t>;
            using iterator_concept = std::input_iterator_tag;

            iterator()
                requires std::default_initializable<std::ranges::iterator_t<base_t>>
            = default;

            iterator(iterator&&)            = default;
            iterator& operator=(iterator&&) = default;

            constexpr iterator(iterator<!Const> i)
                requires Const && std::convertible_to<std::ranges::iterator_t<View>, std::ranges::iterator_t<base_t>>
                : m_current(std::move(i.m_current))
            {
            }

            constexpr const std::ranges::iterator_t<base_t>& base() const& noexcept { return m_current; }

            constexpr std::ranges::iterator_t<base_t> base() && { return std::move(m_current); }

            constexpr decltype(auto) operator*() const { return *m_current; }

            constexpr iterator& operator++()
            {
                ++m_current;
                return *this;
            }

            constexpr void operator++(int) { ++*this; }

            friend constexpr bool operator==(const iterator& x, const std::ranges::sentinel_t<base_t>& y) { return x.m_current == y; }

            friend constexpr difference_type operator-(const std::ranges::sentinel_t<base_t>& y, const iterator& x)
                requires std::sized_sentinel_for<std::ranges::sentinel_t<base_t>, std::ranges::iterator_t<base_t>>
            {
                return y - x.m_current;
            }

            friend constexpr difference_type operator-(const iterator& x, const std::ranges::sentinel_t<base_t>& y)
                requires std::sized_sentinel_for<std::ranges::sentinel_t<base_t>, std::ranges::iterator_t<base_t>>
            {
                return x.m_current - y;
            }

            friend constexpr std::ranges::range_rvalue_reference_t<base_t> iter_move(const iterator& i) noexcept(
                noexcept(std::ranges::iter_move(i.m_current)))
            {
                return std::ranges::iter_move(i.m_current);
            }

            friend constexpr void iter_swap(const iterator& x, const iterator& y) noexcept(noexcept(std::ranges::iter_swap(x.m_current, y.m_current)))
                requires std::indirectly_swappable<std::ranges::iterator_t<base_t>>
            {
                std::ranges::iter_swap(x.m_current, y.m_current);
            }

        private:
            constexpr explicit iterator(std::ranges::iterator_t<base_t> current)
                : m_current(std::move(current))
            {
            }

            friend to_input_view;

        private:
            std::ranges::iterator_t<base_t> m_current = std::ranges::iterator_t<base_t>();
        };

    private:
        View m_base = View();
    };

    template<typename Range>
    to_input_view(Range&&) -> to_input_view<std::views::all_t<Range>>;

    namespace impl
    {
        template<typename Range>
        concept single_pass_range = requires(Range& rg) { std::views::all(rg); } && std::ranges::input_range<Range> &&
                                    !std::ranges::forward_range<Range> && !std::ranges::common_range<Range>;

        struct to_input_fn : public std::ranges::range_adaptor_closure<to_input_fn>
        {
        public:
            template<std::ranges::viewable_range Range>
                requires single_pass_range<Range> || (requires(Range& rg) { ranges::to_input_view{rg}; })
            [[nodiscard]] constexpr auto operator()(Range&& range) const
            {
                if constexpr (single_pass_range<Range>)
                    return std::views::all(std::forward<Range>(range));
                else
                    return ranges::to_input_view{std::forward<Range>(range)};
            }
        };
    } // namespace impl

    namespace views
    {
        /// @brief Converts a view into a non-forward view.
        ///
        /// Pre-C++26 implementation of P3137R3.
        ///
        inline constexpr impl::to_input_fn to_input{};
    } // namespace views
} // namespace upp_test::ranges

template<typename View>
inline constexpr bool std::ranges::enable_borrowed_range<upp_test::ranges::to_input_view<View>> = std::ranges::enable_borrowed_range<View>;

template<typename View, upp::encoding Encoding>
inline constexpr bool upp::ranges::enable_valid_code_unit_range<upp_test::ranges::to_input_view<View>, Encoding> =
    upp::ranges::valid_code_unit_range<View, Encoding>;

#endif // TEST_RANGES_TO_INPUT_HPP
