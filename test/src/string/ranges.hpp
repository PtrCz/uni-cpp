#ifndef TEST_STRING_RANGES_HPP
#define TEST_STRING_RANGES_HPP

#include <cstddef>
#include <iterator>
#include <ranges>
#include <string_view>

namespace upp_test
{
    /// @brief `std::basic_string_view` adaptor that intentionally models `input_range` and only `input_range`.
    ///
    /// This type is useful for testing functions that should work for `input_range`s that don't model `forward_range`.
    ///
    template<typename T>
    class string_input_range : public std::ranges::view_base
    {
    public:
        class iterator
        {
        public:
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using iterator_category = std::input_iterator_tag;
            using iterator_concept  = std::input_iterator_tag;

            constexpr iterator() noexcept                = default;
            constexpr iterator(const iterator&) noexcept = default;

            constexpr explicit iterator(const T* p) noexcept
                : m_ptr(p)
            {
            }

            [[nodiscard]] constexpr value_type operator*() const noexcept { return *m_ptr; }

            constexpr iterator& operator++() noexcept
            {
                ++m_ptr;
                return *this;
            }

            constexpr void operator++(int) noexcept { ++*this; }

            [[nodiscard]] constexpr bool operator==(const iterator& other) const noexcept { return m_ptr == other.m_ptr; }

        private:
            const T* m_ptr;
        };

    public:
        constexpr explicit string_input_range(std::basic_string_view<T> view) noexcept
            : m_view{view}
        {
        }

        [[nodiscard]] constexpr iterator begin() const noexcept
        {
            return iterator{m_view.data()}; // NOLINT(bugprone-suspicious-stringview-data-usage)
        }

        [[nodiscard]] constexpr iterator end() const noexcept { return iterator{m_view.data() + m_view.size()}; }

    private:
        std::basic_string_view<T> m_view;
    };

    static_assert(std::ranges::input_range<string_input_range<char>>);
    static_assert(!std::ranges::forward_range<string_input_range<char>>);
} // namespace upp_test

#endif // TEST_STRING_RANGES_HPP