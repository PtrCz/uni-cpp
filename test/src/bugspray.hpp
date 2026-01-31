#ifndef TEST_BUGSPRAY_HPP
#define TEST_BUGSPRAY_HPP

#if defined(__clang__)

// We cannot push diagnostic and then pop it, because the warning/error is coming from
// macro expansion of a bugspray macro, and that warning is generated where the macro is expanded
// and not where it is defined.
#pragma clang diagnostic ignored "-Wc2y-extensions"

#endif

#include <bugspray/bugspray.hpp>

#define CHECK_FALSE(...)   CHECK(!(__VA_ARGS__))
#define REQUIRE_FALSE(...) REQUIRE(!(__VA_ARGS__))

#define RUNTIME_CHECK_THROWS(...)      \
    do                                 \
    {                                  \
        if !consteval                  \
        {                              \
            CHECK_THROWS(__VA_ARGS__); \
        }                              \
    } while (false)

#define RUNTIME_REQUIRE_THROWS(...)      \
    do                                   \
    {                                    \
        if !consteval                    \
        {                                \
            REQUIRE_THROWS(__VA_ARGS__); \
        }                                \
    } while (false)

#define RUNTIME_CHECK_THROWS_AS(...)      \
    do                                    \
    {                                     \
        if !consteval                     \
        {                                 \
            CHECK_THROWS_AS(__VA_ARGS__); \
        }                                 \
    } while (false)

#define RUNTIME_REQUIRE_THROWS_AS(...)      \
    do                                      \
    {                                       \
        if !consteval                       \
        {                                   \
            REQUIRE_THROWS_AS(__VA_ARGS__); \
        }                                   \
    } while (false)

#endif // TEST_BUGSPRAY_HPP