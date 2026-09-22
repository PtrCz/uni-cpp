#ifndef UNI_CPP_IMPL_STRING_CODE_UNITS_T_HPP
#define UNI_CPP_IMPL_STRING_CODE_UNITS_T_HPP

#include "../../encoding.hpp"

#include "../ranges/base.hpp"
#include "../ranges/valid_code_unit_range.hpp"

#include <span>
#include <stdexcept>

namespace upp::impl
{
    template<encoding Encoding, typename CodeUnitType>
    class code_units_t;

    template<encoding Encoding, typename CodeUnitType>
    [[nodiscard]] constexpr code_units_t<Encoding, CodeUnitType> make_code_units_t(std::span<const CodeUnitType> code_units) noexcept;

    template<encoding Encoding, typename CodeUnitType>
    class code_units_t
    {
    private:
        using span_t    = std::span<const CodeUnitType>;
        using size_type = span_t::size_type;
        using reference = const CodeUnitType&;
        using pointer   = const CodeUnitType*;

    public:
        code_units_t() = default;

        [[nodiscard]] constexpr auto begin() const noexcept { return m_code_units.begin(); }
        [[nodiscard]] constexpr auto end() const noexcept { return m_code_units.end(); }

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

        [[nodiscard]] constexpr reference operator[](size_type index) const { return m_code_units[index]; }

        [[nodiscard]] constexpr pointer data() const noexcept { return m_code_units.data(); }

        [[nodiscard]] constexpr bool empty() const noexcept { return m_code_units.empty(); }

        [[nodiscard]] constexpr size_type size() const noexcept { return m_code_units.size(); }
        [[nodiscard]] constexpr size_type size_bytes() const noexcept { return m_code_units.size_bytes(); }

        [[nodiscard]] std::span<const std::byte> as_bytes() const noexcept { return std::as_bytes(m_code_units); }

    private:
        constexpr explicit code_units_t(span_t code_units) noexcept
            : m_code_units(code_units)
        {
        }

        friend constexpr code_units_t<Encoding, CodeUnitType> make_code_units_t<Encoding, CodeUnitType>(
            std::span<const CodeUnitType> code_units) noexcept;

    private:
        span_t m_code_units;
    };

    template<encoding Encoding, typename CodeUnitType>
    [[nodiscard]] constexpr code_units_t<Encoding, CodeUnitType> make_code_units_t(std::span<const CodeUnitType> code_units) noexcept
    {
        return code_units_t<Encoding, CodeUnitType>{code_units};
    }
} // namespace upp::impl

/// @cond

template<upp::encoding Encoding, typename CodeUnitType>
inline constexpr bool std::ranges::enable_borrowed_range<upp::impl::code_units_t<Encoding, CodeUnitType>> = true;

template<upp::encoding Encoding, typename CodeUnitType>
inline constexpr bool std::ranges::enable_view<upp::impl::code_units_t<Encoding, CodeUnitType>> = true;

template<upp::encoding Encoding, typename CodeUnitType>
inline constexpr bool upp::ranges::enable_valid_code_unit_range<upp::impl::code_units_t<Encoding, CodeUnitType>, Encoding> = true;

/// @endcond

#endif // UNI_CPP_IMPL_STRING_CODE_UNITS_T_HPP