#ifndef UNI_CPP_IMPL_NORMALIZATION_FORM_HPP
#define UNI_CPP_IMPL_NORMALIZATION_FORM_HPP

#include <cstdint>

namespace upp
{
    enum class normalization_form : std::uint8_t
    {
        nfd,
        nfc,
        nfkd,
        nfkc,
    };

    /// @brief Returns `true` if @p form is a canonical normalization form (NFD or NFC), otherwise `false`.
    ///
    [[nodiscard]] constexpr bool is_canonical_normalization_form(normalization_form form) noexcept
    {
        return form == normalization_form::nfd || form == normalization_form::nfc;
    }

    /// @brief Concept checking whether @p Form is a canonical normalization form (NFD or NFC).
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<normalization_form Form>
    concept canonical_normalization_form = is_canonical_normalization_form(Form);

    /// @brief Returns `true` if @p form is a compatibility normalization form (NFKD or NFKC), otherwise `false`.
    ///
    [[nodiscard]] constexpr bool is_compatibility_normalization_form(normalization_form form) noexcept
    {
        return form == normalization_form::nfkd || form == normalization_form::nfkc;
    }

    /// @brief Concept checking whether @p Form is a compatibility normalization form (NFKD or NFKC).
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<normalization_form Form>
    concept compatibility_normalization_form = is_compatibility_normalization_form(Form);
} // namespace upp

#endif // UNI_CPP_IMPL_NORMALIZATION_FORM_HPP