#ifndef TEST_CATCH2_HPP
#define TEST_CATCH2_HPP

#if defined(__clang__)

// We cannot push diagnostic and then pop it, because the warning/error is coming from
// macro expansion of a catch2 macro, and that warning is generated where the macro is expanded
// and not where it is defined.
#pragma clang diagnostic ignored "-Wc2y-extensions"

#endif

#include <catch2/catch_test_macros.hpp>

#endif // TEST_CATCH2_HPP