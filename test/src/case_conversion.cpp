#include <catch2/catch_test_macros.hpp>

import std;
import uni_cpp;
import test_data;

TEST_CASE("Lowercase conversion & lowercase mappings", "[case conversion][upp::uchar]")
{
    upp_test::test_using_data_from_file("lowercase_mappings.txt", [](upp::uchar ch) {
        return ch.to_lowercase() | std::views::transform([](upp::uchar mapping) { return mapping.value(); });
    });
}

TEST_CASE("Uppercase conversion & uppercase mappings", "[case conversion][upp::uchar]")
{
    upp_test::test_using_data_from_file("uppercase_mappings.txt", [](upp::uchar ch) {
        return ch.to_uppercase() | std::views::transform([](upp::uchar mapping) { return mapping.value(); });
    });
}

TEST_CASE("Titlecase conversion & titlecase mappings", "[case conversion][upp::uchar]")
{
    upp_test::test_using_data_from_file("titlecase_mappings.txt", [](upp::uchar ch) {
        return ch.to_titlecase() | std::views::transform([](upp::uchar mapping) { return mapping.value(); });
    });
}