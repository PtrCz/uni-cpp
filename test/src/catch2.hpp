#ifndef TEST_CATCH2_HPP
#define TEST_CATCH2_HPP

#include <catch2/catch_test_macros.hpp>

namespace upp_test::impl::crtt
{
    template<typename StaticAssert>
    struct constexpr_and_runtime_test
    {
        template<typename TestFunction>
        constexpr_and_runtime_test& operator=(TestFunction&&)
        {
            StaticAssert{}.template operator()<[] {
                bool success = true;
                TestFunction{}(success);

                return success;
            }()>();

            [[maybe_unused]] bool success = true;
            TestFunction{}(success);

            return *this;
        }
    };
} // namespace upp_test::impl::crtt

#define TEST_IMPL_CRTT_STRINGIFY_IMPL(x) #x
#define TEST_IMPL_CRTT_STRINGIFY(x)      TEST_IMPL_CRTT_STRINGIFY_IMPL(x)

#define CONSTEXPR_AND_RUNTIME_TEST()                                                                               \
    if (::upp_test::impl::crtt::constexpr_and_runtime_test<decltype([]<bool TestSuccess>() static constexpr {      \
            static_assert(TestSuccess, "constexpr test failed: " __FILE__ ":" TEST_IMPL_CRTT_STRINGIFY(__LINE__)); \
        })>                                                                                                        \
            crtt_impl_var{};                                                                                       \
        true)                                                                                                      \
    crtt_impl_var = [](bool& crtt_impl_success) static constexpr->void

#define CRTT_CHECK(...)                    \
    do                                     \
    {                                      \
        if consteval                       \
        {                                  \
            if (void(); __VA_ARGS__)       \
                ;                          \
            else                           \
                crtt_impl_success = false; \
        }                                  \
        else                               \
        {                                  \
            CHECK(__VA_ARGS__);            \
        }                                  \
    } while (false)

#define CRTT_ASSUME(...)                                                                                                                          \
    if ([&] {                                                                                                                                     \
            if (void(); __VA_ARGS__)                                                                                                              \
            {                                                                                                                                     \
                if !consteval                                                                                                                     \
                {                                                                                                                                 \
                    SUCCEED("CRTT_ASSUME( " #__VA_ARGS__ " )");                                                                                   \
                }                                                                                                                                 \
                                                                                                                                                  \
                return true;                                                                                                                      \
            }                                                                                                                                     \
            else                                                                                                                                  \
            {                                                                                                                                     \
                if consteval                                                                                                                      \
                {                                                                                                                                 \
                    crtt_impl_success = false;                                                                                                    \
                }                                                                                                                                 \
                else                                                                                                                              \
                {                                                                                                                                 \
                    FAIL_CHECK("\nCRTT_ASSUME(...) FAILED:\n\n  " __FILE__ ":" TEST_IMPL_CRTT_STRINGIFY(__LINE__) ":\nCRTT_ASSUME( " #__VA_ARGS__ \
                                                                                                                  " )");                          \
                }                                                                                                                                 \
                                                                                                                                                  \
                return false;                                                                                                                     \
            }                                                                                                                                     \
        }())

#endif // TEST_CATCH2_HPP