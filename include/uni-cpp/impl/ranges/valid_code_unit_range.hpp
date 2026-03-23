#ifndef UNI_CPP_IMPL_RANGES_VALID_CODE_UNIT_RANGE_HPP
#define UNI_CPP_IMPL_RANGES_VALID_CODE_UNIT_RANGE_HPP

/// @file
///
/// @brief Facilities for marking and detecting valid code unit ranges.
///

#include "base.hpp"
#include "approximately_sized_range.hpp"
#include "view_interface.hpp"

#include "../../encoding.hpp"

#include <cstdint>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>

/// @defgroup valid_code_unit_range Valid code unit ranges
///
/// @brief Facilities for marking and detecting valid code unit ranges.
///
/// These facilities allow functions and range adaptors to assume encoding correctness without
/// performing redundant validation, enabling more efficient processing and a more correct interface.
///
/// @note Constructing a range that models `upp::ranges::valid_code_unit_range` from an invalid
/// or incomplete sequence violates its invariant and results in undefined behaviour.
///
/// @headerfile "" <uni-cpp/ranges.hpp>
///

namespace upp::ranges
{
    /// @brief A view that marks its underlying view as a valid code unit sequence for the given `Encoding`.
    ///
    /// Acts as a lightweight view wrapper that signals that the underlying view
    /// is guaranteed to be a well-formed code unit sequence for the specified `Encoding`.
    ///
    /// The wrapper itself simply forwards all operations to the underlying view. It does not do anything else,
    /// but it is visible in the type system and it always satisfies the `upp::ranges::valid_code_unit_range` concept for the given `Encoding`.
    ///
    /// @note This type exists primarily as an implementation detail of `upp::views::mark_as_valid_encoding`.
    /// Using `mark_as_valid_encoding` is preferred over directly constructing a `valid_code_unit_view`, because it
    /// minimizes unnecessary template instantiations and preserves the original view type, which enables
    /// some extra optimizations in the uni-cpp library when possible.
    ///
    /// @pre The underlying view MUST be a valid and complete code unit sequence.
    /// If the underlying code unit sequence is invalid or incomplete (ends unexpectedly), the behaviour is undefined.
    ///
    /// @ingroup valid_code_unit_range
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<std::ranges::view View, encoding Encoding>
        requires code_unit_input_range<View, Encoding>
    class valid_code_unit_view : public UNI_CPP_IMPL_VIEW_INTERFACE(valid_code_unit_view<View, Encoding>)
    {
    public:
        /// @brief Default constructor.
        ///
        valid_code_unit_view()
            requires std::default_initializable<View>
        = default;

        /// @brief Constructs the view from an underlying view.
        ///
        /// @param base The underlying view to wrap.
        ///
        /// @pre The underlying view MUST be a valid and complete code unit sequence.
        /// If the underlying code unit sequence is invalid or incomplete (ends unexpectedly), the behaviour is undefined.
        ///
        constexpr explicit valid_code_unit_view(View base)
            : m_base(std::move(base))
        {
        }

        /// @brief Constructs the view from an underlying view.
        ///
        /// @param base The underlying view to wrap.
        ///
        /// @pre The underlying view MUST be a valid and complete code unit sequence.
        /// If the underlying code unit sequence is invalid or incomplete (ends unexpectedly), the behaviour is undefined.
        ///
        constexpr valid_code_unit_view(View base, encoding_tag_t<Encoding>)
            : m_base(std::move(base))
        {
        }

        /// @brief Returns a copy of the underlying view.
        ///
        constexpr View base() const&
            requires std::copy_constructible<View>
        {
            return m_base;
        }

        /// @brief Returns the underlying view by moving it.
        ///
        constexpr View base() && { return std::move(m_base); }

        /// @brief Returns an iterator to the beginning of the range.
        ///
        constexpr auto begin() { return std::ranges::begin(m_base); }

        /// @brief Returns an iterator to the beginning of the range.
        ///
        constexpr auto begin() const
            requires std::ranges::range<const View> && code_unit_input_range<const View, Encoding>
        {
            return std::ranges::begin(m_base);
        }

        /// @brief Returns an iterator/sentinel marking the end of the range.
        ///
        constexpr auto end() { return std::ranges::end(m_base); }

        /// @brief Returns an iterator/sentinel marking the end of the range.
        ///
        constexpr auto end() const
            requires std::ranges::range<const View> && code_unit_input_range<const View, Encoding>
        {
            return std::ranges::end(m_base);
        }

        /// @brief Returns the size of the range.
        ///
        constexpr auto size()
            requires std::ranges::sized_range<View>
        {
            return std::ranges::size(m_base);
        }

        /// @brief Returns the size of the range.
        ///
        constexpr auto size() const
            requires std::ranges::sized_range<const View> && code_unit_input_range<const View, Encoding>
        {
            return std::ranges::size(m_base);
        }

        /// @brief Returns an approximate size of the range.
        ///
        constexpr auto reserve_hint()
            requires approximately_sized_range<View>
        {
            return ranges::reserve_hint(m_base);
        }

        /// @brief Returns an approximate size of the range.
        ///
        constexpr auto reserve_hint() const
            requires approximately_sized_range<const View> && code_unit_input_range<const View, Encoding>
        {
            return ranges::reserve_hint(m_base);
        }

    private:
        View m_base = View();
    };

    /// @cond

    template<typename Range, encoding Encoding>
    valid_code_unit_view(Range&&, encoding_tag_t<Encoding>) -> valid_code_unit_view<std::views::all_t<Range>, Encoding>;

    /// @endcond

    /// @brief Template variable used to indicate whether a range type is guaranteed to be a well-formed code unit sequence in the given encoding.
    ///
    /// The primary template is defined as `false`.
    ///
    /// @par Specializations
    /// A program may specialize `enable_valid_code_unit_range` to `true` for cv-unqualified program-defined types which model `valid_code_unit_range`, and `false` for types which do not.
    /// Such specializations shall be usable in constant expression context and have type const bool.
    ///
    /// The library provides specializations of this template variable to properly propagate the result through [`std::views::all(range)`][1].
    /// That is, for any type `Range` where `enable_valid_code_unit_range<Range>` is `true`, the following is `true` as well:
    /// `enable_valid_code_unit_range<std::views::all_t<Range>>`.
    ///
    /// Furthermore, the following specializations are provided:
    /// - `enable_valid_code_unit_range<uchar::encode_utf8_t, encoding::utf8>` is `true`,
    /// - `enable_valid_code_unit_range<uchar::encode_utf16_t, encoding::utf16>` is `true`,
    /// - `enable_valid_code_unit_range<valid_code_unit_view<Encoding, View>, Encoding>` is `true`.
    /// - `enable_valid_code_unit_range<transcode_view<View, SourceEncoding, TargetEncoding, Kind, ToType>, TargetEncoding>` equals to `Kind != transcode_view_kind::expected`.
    ///   That's because the `transcode_view` always produces valid UTF, but if the `Kind` is `expected`, then the `value_type` of the `transcode_view` range is
    ///   `std::expected<ToType, error_type>`, which can't be a `valid_code_unit_range`, because it isn't even a `code_unit_range`.
    /// - The following extra specialization for `transcode_view` is provided if `valid_code_unit_range<View, encoding::ascii>` is satisfied:
    ///   `enable_valid_code_unit_range<transcode_view<View, SourceEncoding, encoding::utf8, Kind, ToType>, encoding::ascii> = Kind != transcode_view_kind::expected`.
    ///   It means that transcoding a valid ASCII range to UTF-8 results in a range that's valid ASCII as well.
    ///
    /// @par Specializing `enable_valid_code_unit_range` vs. using `views::mark_as_valid_encoding`
    ///     Specializing `enable_valid_code_unit_range` declares that the **type itself** guarantees a well-formed code unit sequence for the specified `Encoding`.
    ///     That is, every object of the type that satisfies its invariants represents a valid and complete code unit sequence.<br><br>
    ///     This is appropriate for types whose construction or semantics inherently enforce encoding validity.<br><br>
    ///     If a range type does not provide such a guarantee in general, but a specific range value is known to be valid,
    ///     the program should instead use the `upp::views::mark_as_valid_...` range adaptor family.
    ///     These adaptors construct a `valid_code_unit_view`, whose invariant states that the wrapped range is a valid sequence.
    ///     This converts a particular validated range value into a type that carries the required invariant.
    ///
    /// [1]: https://www.en.cppreference.com/w/cpp/ranges/all_view.html
    ///
    /// @ingroup valid_code_unit_range
    ///
    template<typename Range, encoding Encoding>
    inline constexpr bool enable_valid_code_unit_range = false;

    /// @cond

    // propagate through std::views::all

    template<typename Range, encoding Encoding>
    inline constexpr bool enable_valid_code_unit_range<std::ranges::owning_view<Range>, Encoding> =
        enable_valid_code_unit_range<std::remove_cvref_t<Range>, Encoding>;

    template<typename Range, encoding Encoding>
    inline constexpr bool enable_valid_code_unit_range<std::ranges::ref_view<Range>, Encoding> =
        enable_valid_code_unit_range<std::remove_cvref_t<Range>, Encoding>;

    // invariants

    template<encoding Encoding, typename View>
    inline constexpr bool enable_valid_code_unit_range<valid_code_unit_view<View, Encoding>, Encoding> = true;

    template<>
    inline constexpr bool enable_valid_code_unit_range<uchar::encode_utf8_t, encoding::utf8> = true;

    template<>
    inline constexpr bool enable_valid_code_unit_range<uchar::encode_utf16_t, encoding::utf16> = true;

    /// @endcond

    /// @brief Identifies ranges that are guaranteed to be well-formed code unit sequences in the given encoding.
    ///
    /// A range satisfies this concept if it models `code_unit_input_range` for the
    /// given `Encoding` and its type guarantees that every value represents
    /// a complete and well-formed code unit sequence.
    ///
    /// Such guarantees may arise in two ways:
    /// - The range type itself enforces the invariant and declares it by specializing `enable_valid_code_unit_range`.
    /// - The range is wrapped in a `valid_code_unit_view`, usually constructed by `upp::views::mark_as_valid_encoding`,
    ///   which introduces the invariant for the wrapped range. Note that constructing a `valid_code_unit_view` from an invalid
    ///   code unit sequence results in undefined behaviour.
    ///
    /// @note Every valid ASCII range is also a valid UTF-8 range, because ASCII is a subset of UTF-8.
    ///
    /// @ingroup valid_code_unit_range
    ///
    /// @headerfile "" <uni-cpp/ranges.hpp>
    ///
    template<typename Range, encoding Encoding>
    concept valid_code_unit_range = code_unit_input_range<Range, Encoding> &&
                                    (enable_valid_code_unit_range<std::remove_cvref_t<Range>, Encoding> ||
                                     (Encoding == encoding::utf8 && enable_valid_code_unit_range<std::remove_cvref_t<Range>, encoding::ascii>));

    namespace impl
    {
        template<encoding Encoding>
        struct mark_as_valid_encoding_fn : public std::ranges::range_adaptor_closure<mark_as_valid_encoding_fn<Encoding>>
        {
        public:
            template<std::ranges::viewable_range Range>
                requires code_unit_input_range<Range, Encoding>
            [[nodiscard]] constexpr auto operator()(Range&& range) const
            {
                if constexpr (valid_code_unit_range<std::views::all_t<Range>, Encoding>)
                    return std::views::all(std::forward<Range>(range));
                else
                    return valid_code_unit_view<std::views::all_t<Range>, Encoding>(std::views::all(std::forward<Range>(range)));
            }
        };
    } // namespace impl

    namespace views
    {
        /// @addtogroup valid_code_unit_range
        /// @{

        /// @brief Marks a range as a valid code unit range in the given `Encoding`.
        ///
        /// Template variable that once instantiated becomes a [RangeAdaptorClosureObject][1].
        ///
        /// @par Call signature (of the instantiated RangeAdaptorClosureObject)
        /// ```cpp
        /// template<std::ranges::viewable_range Range>
        ///     requires code_unit_input_range<Range, Encoding>
        /// constexpr valid_code_unit_range<Encoding> auto mark_as_valid_encoding(Range&& range);
        /// ```
        ///
        /// @return
        /// - `std::views::all(std::forward<Range>(range))` if `valid_code_unit_range<std::views::all_t<Range>, Encoding>` is `true`,
        /// - otherwise: `valid_code_unit_view<std::views::all_t<Range>, Encoding>(std::views::all(std::forward<Range>(range)))`.
        ///
        /// @par Example usage
        /// @code{.cpp}
        ///
        /// // Provided `utf8_range` names a valid UTF-8 code unit range...
        ///
        /// const auto valid_range = utf8_range | upp::views::mark_as_valid_encoding<upp::encoding::utf8>;
        ///
        /// @endcode
        ///
        /// @pre The `range` argument MUST be a valid code unit range in the given `Encoding`.
        /// If the code unit sequence is invalid or incomplete (ends unexpectedly), the behaviour is undefined.
        ///
        /// [1]: https://en.cppreference.com/w/cpp/named_req/RangeAdaptorClosureObject.html "RangeAdaptorClosureObject on cppreference.com"
        ///
        template<encoding Encoding>
        inline constexpr impl::mark_as_valid_encoding_fn<Encoding> mark_as_valid_encoding{};

        /// @brief Marks a code unit range as a valid ASCII range.
        ///
        /// [RangeAdaptorClosureObject](https://en.cppreference.com/w/cpp/named_req/RangeAdaptorClosureObject.html).
        ///
        /// @par Call signature
        /// ```cpp
        /// template<std::ranges::viewable_range Range>
        ///     requires code_unit_input_range<Range, encoding::ascii>
        /// constexpr valid_code_unit_range<encoding::ascii> auto mark_as_valid_ascii(Range&& range);
        /// ```
        ///
        /// @return
        /// - `std::views::all(std::forward<Range>(range))` if `valid_code_unit_range<std::views::all_t<Range>, encoding::ascii>` is `true`,
        /// - otherwise: `valid_code_unit_view<std::views::all_t<Range>, encoding::ascii>(std::views::all(std::forward<Range>(range)))`.
        ///
        /// @pre The `range` argument MUST be a valid ASCII code unit range.
        /// If the code unit sequence is invalid (contains values above `0x7F`), the behaviour is undefined.
        ///
        inline constexpr impl::mark_as_valid_encoding_fn<encoding::ascii> mark_as_valid_ascii{};

        /// @brief Marks a code unit range as a valid UTF-8 range.
        ///
        /// [RangeAdaptorClosureObject](https://en.cppreference.com/w/cpp/named_req/RangeAdaptorClosureObject.html).
        ///
        /// @par Call signature
        /// ```cpp
        /// template<std::ranges::viewable_range Range>
        ///     requires code_unit_input_range<Range, encoding::utf8>
        /// constexpr valid_code_unit_range<encoding::utf8> auto mark_as_valid_utf8(Range&& range);
        /// ```
        ///
        /// @return
        /// - `std::views::all(std::forward<Range>(range))` if `valid_code_unit_range<std::views::all_t<Range>, encoding::utf8>` is `true`,
        /// - otherwise: `valid_code_unit_view<std::views::all_t<Range>, encoding::utf8>(std::views::all(std::forward<Range>(range)))`.
        ///
        /// @pre The `range` argument MUST be a valid UTF-8 code unit range.
        /// If the code unit sequence is invalid or incomplete (ends unexpectedly), the behaviour is undefined.
        ///
        inline constexpr impl::mark_as_valid_encoding_fn<encoding::utf8> mark_as_valid_utf8{};

        /// @brief Marks a code unit range as a valid UTF-16 range.
        ///
        /// [RangeAdaptorClosureObject](https://en.cppreference.com/w/cpp/named_req/RangeAdaptorClosureObject.html).
        ///
        /// @par Call signature
        /// ```cpp
        /// template<std::ranges::viewable_range Range>
        ///     requires code_unit_input_range<Range, encoding::utf16>
        /// constexpr valid_code_unit_range<encoding::utf16> auto mark_as_valid_utf16(Range&& range);
        /// ```
        ///
        /// @return
        /// - `std::views::all(std::forward<Range>(range))` if `valid_code_unit_range<std::views::all_t<Range>, encoding::utf16>` is `true`,
        /// - otherwise: `valid_code_unit_view<std::views::all_t<Range>, encoding::utf16>(std::views::all(std::forward<Range>(range)))`.
        ///
        /// @pre The `range` argument MUST be a valid UTF-16 code unit range.
        /// If the code unit sequence is invalid or incomplete (ends unexpectedly), the behaviour is undefined.
        ///
        inline constexpr impl::mark_as_valid_encoding_fn<encoding::utf16> mark_as_valid_utf16{};

        /// @brief Marks a code unit range as a valid UTF-32 range.
        ///
        /// [RangeAdaptorClosureObject](https://en.cppreference.com/w/cpp/named_req/RangeAdaptorClosureObject.html).
        ///
        /// @par Call signature
        /// ```cpp
        /// template<std::ranges::viewable_range Range>
        ///     requires code_unit_input_range<Range, encoding::utf32>
        /// constexpr valid_code_unit_range<encoding::utf32> auto mark_as_valid_utf32(Range&& range);
        /// ```
        ///
        /// @return
        /// - `std::views::all(std::forward<Range>(range))` if `valid_code_unit_range<std::views::all_t<Range>, encoding::utf32>` is `true`,
        /// - otherwise: `valid_code_unit_view<std::views::all_t<Range>, encoding::utf32>(std::views::all(std::forward<Range>(range)))`.
        ///
        /// @pre The `range` argument MUST be a valid UTF-32 code unit range.
        /// If the code unit sequence is invalid (contains non-USVs), the behaviour is undefined.
        ///
        inline constexpr impl::mark_as_valid_encoding_fn<encoding::utf32> mark_as_valid_utf32{};

        /// @}
    } // namespace views
} // namespace upp::ranges

/// @cond

template<typename View, upp::encoding Encoding>
inline constexpr bool std::ranges::enable_borrowed_range<upp::ranges::valid_code_unit_view<View, Encoding>> =
    std::ranges::enable_borrowed_range<View>;

/// @endcond

#endif // UNI_CPP_IMPL_RANGES_VALID_CODE_UNIT_RANGE_HPP
