#include "catch2.hpp"

#include "test_data.hpp"

#include <uni-cpp/uchar.hpp>
#include <cstdint>
#include <ranges>

TEST_CASE("UTF-8 encoding", "[UTF encoding][upp::uchar]")
{
    upp_test::test_using_data_from_file("utf_8_encoding.txt", [](upp::uchar ch) {
        return ch.encode_utf8() | std::views::transform([](char8_t value) { return static_cast<std::uint8_t>(value); });
    });
}

TEST_CASE("UTF-16 encoding", "[UTF encoding][upp::uchar]")
{
    upp_test::test_using_data_from_file("utf_16_encoding.txt", [](upp::uchar ch) {
        return ch.encode_utf16() | std::views::transform([](char16_t value) { return static_cast<std::uint16_t>(value); });
    });
}