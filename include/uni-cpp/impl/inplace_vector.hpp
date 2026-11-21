#ifndef UNI_CPP_IMPL_INPLACE_VECTOR_HPP
#define UNI_CPP_IMPL_INPLACE_VECTOR_HPP

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <array>
#include <type_traits>
#include <iterator>
#include <memory>
#include <initializer_list>

namespace upp::impl::inplace_vector_impl
{
    template<typename T, std::uint8_t N>
    class inplace_vector;

    template<std::integral T, std::uint8_t N>
    class inplace_vector<T, N>
    {
    private:
        static constexpr std::size_t capacity{N};

    public:
        using value_type      = T;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference       = value_type&;
        using const_reference = const value_type&;
        using pointer         = value_type*;
        using const_pointer   = const value_type*;
        using iterator        = pointer;
        using const_iterator  = const_pointer;

    public:
        constexpr inplace_vector() noexcept
        {
            if consteval
            {
                // In constant evaluation the array can't be uninitialized.
                m_data.fill(T{});
            }
        }

        /// @pre `ilist.size() <= N`
        ///
        constexpr inplace_vector(std::initializer_list<T> ilist) noexcept
        {
            if consteval
            {
                // In constant evaluation the array can't be uninitialized.
                m_data.fill(T{});
            }

            for (const T value : ilist)
                unchecked_push_back(value);
        }

        [[nodiscard]] constexpr size_type size() const noexcept { return static_cast<size_type>(m_size); }
        [[nodiscard]] constexpr bool      empty() const noexcept { return m_size == 0; }

        [[nodiscard]] constexpr pointer       data() noexcept { return m_data.data(); }
        [[nodiscard]] constexpr const_pointer data() const noexcept { return m_data.data(); }

        constexpr void clear() noexcept { m_size = 0; }

        constexpr reference unchecked_push_back(T value) noexcept { return m_data[static_cast<std::size_t>(m_size++)] = value; }

        [[nodiscard]] constexpr reference       operator[](size_type index) noexcept { return m_data[index]; }
        [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept { return m_data[index]; }

        [[nodiscard]] friend constexpr bool operator==(const inplace_vector& x, const inplace_vector& y) noexcept
        {
            if (x.size() != y.size())
                return false;

            return std::equal(x.cbegin(), x.cend(), y.cbegin());
        }

        [[nodiscard]] constexpr iterator       begin() noexcept { return m_data.data(); }
        [[nodiscard]] constexpr const_iterator begin() const noexcept { return m_data.data(); }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }

        [[nodiscard]] constexpr iterator       end() noexcept { return m_data.data() + m_size; }
        [[nodiscard]] constexpr const_iterator end() const noexcept { return m_data.data() + m_size; }
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

    private:
        std::array<T, capacity> m_data;
        std::uint8_t            m_size{};
    };

    template<typename T>
    concept char_like = requires(T ch) {
        { ch.value() } -> std::integral;
        { T::from_unchecked(ch.value()) } -> std::same_as<T>;
    };

    template<char_like T>
    union uninitialized {
        constexpr uninitialized() noexcept {}

        constexpr uninitialized(T val) noexcept
            : value{val}
        {
        }

        T value;
    };

    template<char_like T, std::uint8_t N>
    class inplace_vector<T, N>
    {
    private:
        static constexpr std::size_t capacity{N};

        using uninitialized_t = uninitialized<T>;

        template<bool Const>
        class iterator_impl
        {
        private:
            using uninitialized_pointer_t = std::conditional_t<Const, const uninitialized_t*, uninitialized_t*>;

        public:
            using iterator_concept  = std::contiguous_iterator_tag;
            using iterator_category = std::contiguous_iterator_tag;
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using pointer           = std::conditional_t<Const, const T*, T*>;
            using reference         = std::conditional_t<Const, const T&, T&>;

        public:
            constexpr iterator_impl() noexcept = default;

            /// @brief Constructs a `const` iterator from a non-`const` iterator.
            ///
            constexpr iterator_impl(iterator_impl<!Const> i) noexcept
                requires Const
                : m_pointer{i.m_pointer}
            {
            }

            [[nodiscard]] constexpr reference operator*() const noexcept { return m_pointer->value; }
            [[nodiscard]] constexpr pointer   operator->() const noexcept { return &m_pointer->value; }

            constexpr iterator_impl& operator++() noexcept
            {
                ++m_pointer;
                return *this;
            }
            constexpr iterator_impl operator++(int) noexcept
            {
                iterator_impl tmp = *this;
                ++*this;
                return tmp;
            }

            constexpr iterator_impl& operator--() noexcept
            {
                --m_pointer;
                return *this;
            }
            constexpr iterator_impl operator--(int) noexcept
            {
                iterator_impl tmp = *this;
                --*this;
                return tmp;
            }

            constexpr iterator_impl& operator+=(difference_type n) noexcept
            {
                m_pointer += n;
                return *this;
            }
            constexpr iterator_impl& operator-=(difference_type n) noexcept
            {
                m_pointer -= n;
                return *this;
            }

            [[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return (m_pointer + n)->value; }

            [[nodiscard]] friend constexpr bool operator==(iterator_impl x, iterator_impl y) noexcept { return x.m_pointer == y.m_pointer; }
            [[nodiscard]] friend constexpr auto operator<=>(iterator_impl x, iterator_impl y) noexcept { return x.m_pointer <=> y.m_pointer; }

            [[nodiscard]] friend constexpr iterator_impl operator+(iterator_impl i, difference_type n) noexcept
            {
                return iterator_impl{i.m_pointer + n};
            }
            [[nodiscard]] friend constexpr iterator_impl operator+(difference_type n, iterator_impl i) noexcept
            {
                return iterator_impl{i.m_pointer + n};
            }

            [[nodiscard]] friend constexpr iterator_impl operator-(iterator_impl i, difference_type n) noexcept
            {
                return iterator_impl{i.m_pointer - n};
            }

            [[nodiscard]] friend constexpr difference_type operator-(iterator_impl x, iterator_impl y) noexcept { return x.m_pointer - y.m_pointer; }

        private:
            explicit constexpr iterator_impl(uninitialized_pointer_t p) noexcept
                : m_pointer{p}
            {
            }

            friend inplace_vector;

        private:
            uninitialized_pointer_t m_pointer{};
        };

    public:
        using value_type      = T;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference       = value_type&;
        using const_reference = const value_type&;
        using pointer         = value_type*;
        using const_pointer   = const value_type*;
        using iterator        = iterator_impl<false>;
        using const_iterator  = iterator_impl<true>;

    public:
        constexpr inplace_vector() noexcept = default;

        /// @pre `ilist.size() <= N`
        ///
        constexpr inplace_vector(std::initializer_list<T> ilist) noexcept
        {
            for (T value : ilist)
                unchecked_push_back(value);
        }

        [[nodiscard]] constexpr size_type size() const noexcept { return static_cast<size_type>(m_size); }
        [[nodiscard]] constexpr bool      empty() const noexcept { return m_size == 0; }

        [[nodiscard]] constexpr pointer       data() noexcept { return &m_data[0].value; }
        [[nodiscard]] constexpr const_pointer data() const noexcept { return &m_data[0].value; }

        constexpr void clear() noexcept { m_size = 0; }

        constexpr reference unchecked_push_back(T value) noexcept
        {
            auto& storage_ref = m_data[static_cast<std::size_t>(m_size++)];

            std::construct_at(&storage_ref.value, value);

            return storage_ref.value;
        }

        [[nodiscard]] constexpr reference       operator[](size_type index) noexcept { return m_data[index].value; }
        [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept { return m_data[index].value; }

        [[nodiscard]] friend constexpr bool operator==(const inplace_vector& x, const inplace_vector& y) noexcept
        {
            if (x.size() != y.size())
                return false;

            return std::equal(x.cbegin(), x.cend(), y.cbegin());
        }

        [[nodiscard]] constexpr iterator       begin() noexcept { return iterator{m_data.data()}; }
        [[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{m_data.data()}; }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }

        [[nodiscard]] constexpr iterator       end() noexcept { return iterator{m_data.data() + m_size}; }
        [[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{m_data.data() + m_size}; }
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

    private:
        std::array<uninitialized_t, capacity> m_data;
        std::uint8_t                          m_size{};
    };
} // namespace upp::impl::inplace_vector_impl

namespace upp::impl
{
    template<typename T, std::size_t N>
        requires(std::integral<T> || inplace_vector_impl::char_like<T>) && (N > 0uz && N < 255uz)
    using inplace_vector = inplace_vector_impl::inplace_vector<T, static_cast<std::uint8_t>(N)>;
}

#endif // UNI_CPP_IMPL_INPLACE_VECTOR_HPP