#ifndef TEST_ENCODING_MACROS_HPP
#define TEST_ENCODING_MACROS_HPP

#include <uni-cpp/encoding.hpp>

#include "../utility.hpp"

#include <vector>
#include <string_view>
#include <expected>

#define TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_IMPL(source_encoding, n, ...)         \
    .expected_utf##n##_transcoding_with_errors = [] {                                   \
        using namespace std::string_view_literals;                                      \
                                                                                        \
        [[maybe_unused]] static constexpr auto target_encoding = upp::encoding::utf##n; \
                                                                                        \
        using code_unit_type = char##n##_t;                                             \
        using error_type     = upp::encoding_traits<source_encoding>::error_type;       \
                                                                                        \
        std::vector<std::expected<char##n##_t, error_type>> result;                     \
                                                                                        \
        __VA_ARGS__                                                                     \
                                                                                        \
        return result;                                                                  \
    }()

#define TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS(source_encoding, ...)                   \
    TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_IMPL(source_encoding, 8, __VA_ARGS__),      \
        TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_IMPL(source_encoding, 16, __VA_ARGS__), \
        TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_IMPL(source_encoding, 32, __VA_ARGS__)

#define TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_APPEND_STRING_LITERAL(error_type, literal)                                     \
    do                                                                                                                           \
    {                                                                                                                            \
        result.append_range(TEST_STRING_LITERAL(target_encoding, literal) | std::views::transform([](code_unit_type code_unit) { \
                                return std::expected<code_unit_type, error_type>{std::in_place, code_unit};                      \
                            }));                                                                                                 \
    } while (false)

#define TEST_EXPECTED_UTF_TRANSCODING_WITH_ERRORS_APPEND_ERROR(error_type, ...)                  \
    do                                                                                           \
    {                                                                                            \
        result.push_back(std::expected<code_unit_type, error_type>{std::unexpect, __VA_ARGS__}); \
    } while (false)

#endif // TEST_ENCODING_MACROS_HPP