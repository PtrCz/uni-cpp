#include "../bugspray.hpp"

#include <uni-cpp/string.hpp>

#include <string_view>

#define TEST_LITERAL(prefix, literal, suffix) CHECK(prefix##literal##suffix.underlying() == prefix##literal##sv)

#define TEST_ASCII_LITERAL(literal)                               \
    do                                                            \
    {                                                             \
        CHECK(literal##_as.underlying() == literal##sv);          \
        CHECK(u8##literal##_a8s.underlying() == u8##literal##sv); \
    } while (false)

#define TEST_UNICODE_LITERAL(literal)      \
    do                                     \
    {                                      \
        TEST_LITERAL(u8, literal, _us);    \
        TEST_LITERAL(u8, literal, _utf8s); \
        TEST_LITERAL(u, literal, _utf16s); \
        TEST_LITERAL(U, literal, _utf32s); \
    } while (false)

TEST_CASE("User-defined ASCII string literals", "[string types]")
{
    using namespace std::string_view_literals;
    using namespace upp::string_literals;

    TEST_ASCII_LITERAL("");
    TEST_ASCII_LITERAL(" ");
    TEST_ASCII_LITERAL("y");
    TEST_ASCII_LITERAL("Hello");
    TEST_ASCII_LITERAL("ASCII ascii 123 !@#");
}
EVAL_TEST_CASE("User-defined ASCII string literals");

TEST_CASE("User-defined Unicode string literals", "[string types][Unicode string types]")
{
    using namespace std::string_view_literals;
    using namespace upp::string_literals;

    TEST_UNICODE_LITERAL("");
    TEST_UNICODE_LITERAL(" ");
    TEST_UNICODE_LITERAL("Hello");
    TEST_UNICODE_LITERAL("\U0010FFFF");
    TEST_UNICODE_LITERAL("\u0447\u0443\u0301\u0434\u043d\u043e");
    TEST_UNICODE_LITERAL("\u5b98\u8bdd");
    TEST_UNICODE_LITERAL("\u0076\u0079\u0073\u0076\u0065\u0064\u010d\u0065\u006e\u0069\u0065");
}
EVAL_TEST_CASE("User-defined Unicode string literals");