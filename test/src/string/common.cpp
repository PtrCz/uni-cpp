#include "../catch2.hpp"

#include <uni-cpp/string.hpp>
#include <uni-cpp/encoding.hpp>

#include <type_traits>
#include <concepts>
#include <string>
#include <string_view>
#include <ranges>

#include "utility.hpp"
#include "ranges.hpp"

TEST_CASE("string type traits", "[string types]")
{
    upp_test::run_for_each_string_type([&]<typename StringType>() {
        STATIC_CHECK(std::default_initializable<StringType>);
        STATIC_CHECK(std::is_nothrow_default_constructible_v<StringType>);
        STATIC_CHECK(std::destructible<StringType>);
        STATIC_CHECK(std::copy_constructible<StringType>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<StringType>);
        // STATIC_CHECK(std::is_nothrow_move_assignable_v<StringType>);
    });
}

TEST_CASE("string constructors", "[string types]")
{
    upp_test::run_for_each_string_type([&]<typename StringType>() {
        StringType empty;
        StringType with_allocator{std::allocator<typename StringType::code_unit_type>()};

        CHECK(empty.underlying().empty());

        StringType copy = empty;
        StringType copy_with_allocator{copy, std::allocator<typename StringType::code_unit_type>()};

        StringType move = std::move(copy);
        StringType move_with_allocator{std::move(empty), std::allocator<typename StringType::code_unit_type>()};
    });
}

TEST_CASE("string assignment operators", "[string types]")
{
    using namespace upp::string_literals;

    upp::ascii_string ascii_str1 = "Hello!"_as;
    auto              ascii_str2 = std::move(ascii_str1);

    CHECK(ascii_str1.code_units().empty()); // NOLINT(bugprone-use-after-move)

    CHECK(ascii_str2 == "Hello!"_as);

    upp::ustring str1 = u8"Cześć!"_us;
    auto         str2 = str1;

    CHECK(str1 == u8"Cześć!"_us);
    CHECK(str1 == str2);
}

TEST_CASE("string operator==", "[string types]")
{
    using namespace upp::string_literals;

    CHECK(""_as == ""_as);
    CHECK("123"_as != "456"_as);
    CHECK("Cafe"_as == "Cafe"_as);

    CHECK(u8""_us == u8""_us);
    CHECK(u8"123"_us != u8""_us);
    CHECK(u8"Caf\u00E9"_us != u8"Cafe\u0301"_us);
    CHECK(u8"Caf\u00E9"_us == u8"Caf\u00E9"_us);
}

TEST_CASE("string .chars(), .char_indices()", "[string types][UTF encoding]")
{
    using namespace upp::char_literals;
    using namespace upp::string_literals;

    {
        const auto str = "Hey"_as;

        const auto chars = str.chars();

        CHECK(chars[0] == 'H'_ac);
        CHECK(chars[1] == 'e'_ac);
        CHECK(chars[2] == 'y'_ac);

        const auto char_indices = str.char_indices();

        CHECK(char_indices[0] == std::make_tuple(0uz, 'H'_ac));
        CHECK(char_indices[1] == std::make_tuple(1uz, 'e'_ac));
        CHECK(char_indices[2] == std::make_tuple(2uz, 'y'_ac));
    }

    {
        const auto str = u8"¿Qué?"_us;

        const auto chars    = str.chars();
        auto       chars_it = chars.begin();

        CHECK((*chars_it++) == U'¿'_uc);
        CHECK((*chars_it++) == U'Q'_uc);
        CHECK((*chars_it++) == U'u'_uc);
        CHECK((*chars_it++) == U'é'_uc);
        CHECK((*chars_it++) == U'?'_uc);

        CHECK(chars_it == chars.end());

        const auto char_indices    = str.char_indices();
        auto       char_indices_it = char_indices.begin();

        CHECK((*char_indices_it++) == std::make_tuple(0uz, U'¿'_uc));
        CHECK((*char_indices_it++) == std::make_tuple(2uz, U'Q'_uc));
        CHECK((*char_indices_it++) == std::make_tuple(3uz, U'u'_uc));
        CHECK((*char_indices_it++) == std::make_tuple(4uz, U'é'_uc));
        CHECK((*char_indices_it++) == std::make_tuple(6uz, U'?'_uc));

        CHECK(char_indices_it == char_indices.end());
    }
}