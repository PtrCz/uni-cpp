#ifndef TEST_CATCH2_HPP
#define TEST_CATCH2_HPP

#include <catch2/catch_test_macros.hpp>

#define COMPILE_TIME_TEST(...)  \
    do                          \
    {                           \
        static_assert(          \
            [] {                \
                {               \
                    __VA_ARGS__ \
                }               \
                return true;    \
            }(),                \
            #__VA_ARGS__);      \
                                \
        [] {                    \
            {                   \
                __VA_ARGS__     \
            }                   \
            return true;        \
        }();                    \
                                \
    } while (false)

#define CTT_CHECK(...)          \
    do                          \
    {                           \
        if consteval            \
        {                       \
            if (__VA_ARGS__)    \
                ;               \
            else                \
                return false;   \
        }                       \
        else                    \
        {                       \
            CHECK(__VA_ARGS__); \
        }                       \
    } while (false)

#endif // TEST_CATCH2_HPP