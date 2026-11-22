#ifndef UNI_CPP_IMPL_STRING_CHAR_INDICES_HPP
#define UNI_CPP_IMPL_STRING_CHAR_INDICES_HPP

#include "../ranges/base.hpp"
#include "../ranges/view_interface.hpp"
#include "../ranges/transcode.hpp"

#include "code_units_t.hpp"

#include <cstddef>
#include <type_traits>
#include <tuple>

namespace upp::ranges::impl
{
    template<encoding Encoding, typename CodeUnitType>
    class char_indices_view : public UNI_CPP_IMPL_VIEW_INTERFACE(char_indices_view<Encoding, CodeUnitType>)
    {
    private:
        using code_units_view_t = upp::impl::code_units_t<Encoding, CodeUnitType>;
        using code_units_it     = std::ranges::iterator_t<code_units_view_t>;
        using view_t =
            decode_view<code_units_view_t, Encoding, decode_view_kind::valid, std::conditional_t<Encoding == encoding::ascii, ascii_char, uchar>>;

        class iterator;

    public:
        char_indices_view() = default;

        constexpr explicit char_indices_view(code_units_view_t base)
            : m_base(view_t(std::move(base)))
        {
        }

        constexpr auto base() const& { return m_base.base(); }

        constexpr auto base() && { return std::move(m_base).base(); }

        constexpr iterator begin() const
        {
            auto it = m_base.begin();

            return iterator(it, it.base());
        }

        constexpr iterator end() const { return iterator(m_base.end(), m_base.begin().base()); }

        constexpr bool empty() const { return m_base.empty(); }

        constexpr auto size() const
            requires std::ranges::sized_range<const view_t>
        {
            return m_base.size();
        }

        constexpr auto reserve_hint() const { return m_base.reserve_hint(); }

    private:
        class iterator : public impl::input_iterator_category_impl<view_t>
        {
        private:
            using base_t          = const view_t;
            using underlying_it_t = std::ranges::iterator_t<base_t>;

        public:
            using iterator_concept = underlying_it_t::iterator_concept;
            using difference_type  = underlying_it_t::difference_type;
            using value_type       = std::tuple<std::size_t, std::ranges::range_value_t<base_t>>;

        private:
            using reference = std::tuple<std::size_t, std::ranges::range_reference_t<base_t>>;

        public:
            constexpr iterator() = default;

            constexpr const auto& base() const& { return m_current.base(); }

            constexpr auto base() && { return std::move(m_current).base(); }

            [[nodiscard]] constexpr std::size_t offset() const noexcept { return static_cast<std::size_t>(m_current.base() - m_begin); }

            constexpr reference operator*() const { return reference{offset(), *m_current}; }

            constexpr iterator& operator++()
            {
                ++m_current;
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
                --m_current;
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
                m_current += n;
                return *this;
            }
            constexpr iterator& operator-=(difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                m_current -= n;
                return *this;
            }

            constexpr reference operator[](difference_type n) const
                requires std::ranges::random_access_range<base_t>
            {
                auto it = m_current + n;

                return reference{static_cast<std::size_t>(it.base() - m_begin), *it};
            }

            friend constexpr bool operator==(const iterator& x, const iterator& y) { return x.m_current == y.m_current; }

            friend constexpr auto operator<=>(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<base_t> && std::three_way_comparable<underlying_it_t>
            {
                return x.m_current <=> y.m_current;
            }

            friend constexpr iterator operator+(iterator i, difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                return iterator{i.m_current + n, i.m_begin};
            }
            friend constexpr iterator operator+(difference_type n, iterator i)
                requires std::ranges::random_access_range<base_t>
            {
                return iterator{i.m_current + n, i.m_begin};
            }

            friend constexpr iterator operator-(iterator i, difference_type n)
                requires std::ranges::random_access_range<base_t>
            {
                return iterator{i.m_current - n, i.m_begin};
            }

            friend constexpr difference_type operator-(const iterator& x, const iterator& y)
                requires std::sized_sentinel_for<underlying_it_t, underlying_it_t>
            {
                return x.m_current - y.m_current;
            }

        private:
            constexpr iterator(underlying_it_t current, code_units_it begin)
                : m_current(std::move(current))
                , m_begin(std::move(begin))
            {
            }

            friend char_indices_view;

        private:
            underlying_it_t m_current = underlying_it_t();
            code_units_it   m_begin   = code_units_it();
        };

    private:
        view_t m_base = view_t();
    };
} // namespace upp::ranges::impl

template<upp::encoding Encoding, typename CodeUnitType>
inline constexpr bool std::ranges::enable_borrowed_range<upp::ranges::impl::char_indices_view<Encoding, CodeUnitType>> = true;

#endif // UNI_CPP_IMPL_STRING_CHAR_INDICES_HPP