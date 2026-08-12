#ifndef UNI_CPP_IMPL_RANGES_ITERATOR_CACHE_HPP
#define UNI_CPP_IMPL_RANGES_ITERATOR_CACHE_HPP

#include "base.hpp"

#include <optional>
#include <memory>
#include <initializer_list>
#include <utility>

namespace upp::ranges::impl
{
    template<typename T>
        requires std::is_object_v<T>
    class non_propagating_cache
    {
    public:
        constexpr non_propagating_cache() noexcept = default;
        constexpr ~non_propagating_cache()         = default;

        constexpr non_propagating_cache(const non_propagating_cache&) noexcept {}

        constexpr non_propagating_cache(non_propagating_cache&& other) noexcept { other.reset(); }

        constexpr non_propagating_cache& operator=(const non_propagating_cache& other) noexcept
        {
            if (std::addressof(other) != this)
                reset();

            return *this;
        }

        constexpr non_propagating_cache& operator=(non_propagating_cache&& other) noexcept
        {
            reset();
            other.reset();

            return *this;
        }

        [[nodiscard]] constexpr bool has_value() const noexcept { return m_data.has_value(); }

        [[nodiscard]] constexpr T& operator*() & noexcept { return *m_data; }

        [[nodiscard]] constexpr const T& operator*() const& noexcept { return *m_data; }

        [[nodiscard]] constexpr T&& operator*() && noexcept { return std::move(*m_data); }

        [[nodiscard]] constexpr const T&& operator*() const&& noexcept { return std::move(*m_data); }

        constexpr void reset() noexcept { return m_data.reset(); }

        template<typename... Args>
        constexpr T& emplace(Args&&... args)
        {
            return m_data.emplace(std::forward<Args>(args)...);
        }

        template<typename U, typename... Args>
            requires std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>
        constexpr T& emplace(std::initializer_list<U> ilist, Args&&... args)
        {
            return m_data.emplace(ilist, std::forward<Args>(args)...);
        }

    private:
        std::optional<T> m_data;
    };

    template<std::ranges::forward_range Range>
    struct iterator_cache : protected non_propagating_cache<std::ranges::iterator_t<Range>>
    {
    private:
        using base = non_propagating_cache<std::ranges::iterator_t<Range>>;

    public:
        using base::has_value;

        [[nodiscard]] constexpr std::ranges::iterator_t<Range> get(const Range&) const { return **this; }

        constexpr void set(const Range&, const std::ranges::iterator_t<Range>& it) { base::emplace(it); }
    };

    // If `Range` is a `random_access_range`, don't bother storing the whole iterator.
    template<std::ranges::random_access_range Range>
        requires(sizeof(std::ranges::range_difference_t<Range>) <= sizeof(std::ranges::iterator_t<Range>))
    struct iterator_cache<Range>
    {
    public:
        constexpr iterator_cache() noexcept = default;
        constexpr ~iterator_cache()         = default;

        constexpr iterator_cache(const iterator_cache&) = default;

        constexpr iterator_cache(iterator_cache&& other) noexcept
        {
            m_offset       = other.m_offset;
            other.m_offset = -1;
        }

        constexpr iterator_cache& operator=(const iterator_cache&) = default;

        constexpr iterator_cache& operator=(iterator_cache&& other) noexcept
        {
            m_offset       = other.m_offset;
            other.m_offset = -1;

            return *this;
        }

        [[nodiscard]] constexpr bool has_value() const { return m_offset >= 0; }

        [[nodiscard]] constexpr std::ranges::iterator_t<Range> get(Range& range) const { return std::ranges::begin(range) + m_offset; }

        constexpr void set(Range& range, const std::ranges::iterator_t<Range>& it) { m_offset = it - std::ranges::begin(range); }

    private:
        std::ranges::range_difference_t<Range> m_offset = -1;
    };
} // namespace upp::ranges::impl

#endif // UNI_CPP_IMPL_RANGES_ITERATOR_CACHE_HPP