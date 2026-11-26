#ifndef UNI_CPP_IMPL_STRING_CODE_UNITS_T_HPP
#define UNI_CPP_IMPL_STRING_CODE_UNITS_T_HPP

#include "../../encoding.hpp"

#include "../ranges/base.hpp"
#include "../ranges/valid_code_unit_range.hpp"

#include <span>
#include <stdexcept>

namespace upp::impl
{
    template<typename Container, encoding Encoding>
    class code_units_t;

    template<typename Container, encoding Encoding>
    [[nodiscard]] constexpr code_units_t<Container, Encoding> make_code_units_t(
        std::ranges::subrange<typename Container::const_iterator> code_units) noexcept;

    template<typename Container, encoding Encoding>
    class code_units_t
    {
    private:
        using const_it       = Container::const_iterator;
        using code_unit_type = Container::value_type;
        using subrange_t     = std::ranges::subrange<const_it>;
        using size_type      = std::ranges::range_size_t<subrange_t>;
        using reference      = const code_unit_type&;
        using pointer        = const code_unit_type*;

    public:
        code_units_t() = default;

        [[nodiscard]] constexpr const_it begin() const noexcept { return m_code_units.begin(); }
        [[nodiscard]] constexpr const_it end() const noexcept { return m_code_units.end(); }

        [[nodiscard]] constexpr reference front() const { return m_code_units.front(); }
        [[nodiscard]] constexpr reference back() const { return m_code_units.back(); }

        [[nodiscard]] constexpr reference at(size_type pos) const
        {
            if (pos >= m_code_units.size())
            {
                throw std::out_of_range{"uni-cpp: str.code_units().at(pos): pos >= this->size()"};
            }

            return m_code_units.data()[pos];
        }

        [[nodiscard]] constexpr reference operator[](size_type index) const
        {
            return m_code_units[static_cast<std::ranges::range_difference_t<subrange_t>>(index)];
        }

        [[nodiscard]] constexpr pointer data() const noexcept { return m_code_units.data(); }

        [[nodiscard]] constexpr bool empty() const noexcept { return m_code_units.empty(); }

        [[nodiscard]] constexpr size_type   size() const noexcept { return m_code_units.size(); }
        [[nodiscard]] constexpr std::size_t size_bytes() const noexcept { return m_code_units.size() * sizeof(code_unit_type); }

        [[nodiscard]] std::span<const std::byte> as_bytes() const noexcept { return std::as_bytes(std::span<const code_unit_type>{m_code_units}); }

    private:
        constexpr explicit code_units_t(subrange_t code_units) noexcept
            : m_code_units(std::move(code_units))
        {
        }

        friend constexpr code_units_t<Container, Encoding> make_code_units_t<Container, Encoding>(
            std::ranges::subrange<typename Container::const_iterator> code_units) noexcept;

    private:
        subrange_t m_code_units;
    };

    template<typename Container, encoding Encoding>
    [[nodiscard]] constexpr code_units_t<Container, Encoding> make_code_units_t(
        std::ranges::subrange<typename Container::const_iterator> code_units) noexcept
    {
        return code_units_t<Container, Encoding>{code_units};
    }
} // namespace upp::impl

/// @cond

template<typename Container, upp::encoding Encoding>
inline constexpr bool std::ranges::enable_borrowed_range<upp::impl::code_units_t<Container, Encoding>> = true;

template<typename Container, upp::encoding Encoding>
inline constexpr bool std::ranges::enable_view<upp::impl::code_units_t<Container, Encoding>> = true;

template<typename Container, upp::encoding Encoding>
inline constexpr bool upp::ranges::enable_valid_code_unit_range<upp::impl::code_units_t<Container, Encoding>, Encoding> = true;

/// @endcond

#endif // UNI_CPP_IMPL_STRING_CODE_UNITS_T_HPP