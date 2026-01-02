// Configured using cmake.
//
// BEFORE YOU EDIT THIS FILE:
// Make sure you are editing the file with the .in extenstion in the uni-cpp/cmake directory;
// otherwise, everything you change will be overwritten the moment cmake is run!

#ifndef UNI_CPP_VERSION_HPP
#define UNI_CPP_VERSION_HPP

#include <cstdint>
#include <compare>

/// @file
/// @brief Provides version information for the uni-cpp library and the Unicode version it supports.

namespace upp
{
    /// @brief Represents a semantic version (major.minor.patch).
    ///
    /// @headerfile "" <uni-cpp/version.hpp>
    ///
    struct version_t
    {
    public:
        /// The major version number.
        std::uint16_t major;
        /// The minor version number.
        std::uint16_t minor;
        /// The patch version number.
        std::uint16_t patch;

        /// @brief Composes the version components into a 64-bit integer.
        ///
        /// @return The resulting value is `(major << 32) | (minor << 16) | patch`.
        ///
        [[nodiscard]] constexpr std::uint64_t compose() const noexcept
        {
            std::uint64_t result = static_cast<std::uint64_t>(major) << 32;
            result |= static_cast<std::uint64_t>(minor) << 16;
            result |= static_cast<std::uint64_t>(patch);

            return result;
        }

        /// @brief Equality comparison operator.
        ///
        /// @return `true` if all version components are equal; otherwise, `false`.
        ///
        [[nodiscard]] constexpr bool operator==(const version_t&) const noexcept = default;

        /// @brief Three-way comparison operator.
        ///
        /// Equivalent to `lhs.compose() <=> rhs.compose()`.
        ///
        [[nodiscard]] constexpr std::strong_ordering operator<=>(version_t other) const noexcept { return compose() <=> other.compose(); }
    };

    /// @brief The version of the uni-cpp library.
    ///
    inline constexpr version_t version{.major = 0, .minor = 3, .patch = 0};

    /// @brief The Unicode standard version supported by this library.
    ///
    inline constexpr version_t unicode_version{.major = 16, .minor = 0, .patch = 0};
} // namespace upp

#endif // UNI_CPP_VERSION_HPP
