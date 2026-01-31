#ifndef TEST_UTF8_HPP
#define TEST_UTF8_HPP

#include <uni-cpp/string.hpp>

#include <array>
#include <tuple>
#include <string>

namespace upp_test::utf8
{
    [[nodiscard]] constexpr auto error_test_cases()
    {
        return std::to_array<std::tuple<std::u8string, upp::utf8_error>>({
               {/*          input */ {0x8F}, // Too Long (0 bytes)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0x39, 0x8C}, // Too Long (1 byte)
                /* expected error */ {.valid_up_to = 1, .error_length = 1}

            }, {/*          input */ {0xC6, 0x84, 0x98}, // Too Long (2 bytes)
                /* expected error */ {.valid_up_to = 2, .error_length = 1}

            }, {/*          input */ {0xE0, 0xA0, 0x8C, 0x9E}, // Too Long (3 bytes)
                /* expected error */ {.valid_up_to = 3, .error_length = 1}

            }, {/*          input */ {0xF3, 0xA0, 0x81, 0x88, 0xBC}, // Too Long (4 bytes)
                /* expected error */ {.valid_up_to = 4, .error_length = 1}
            
            }, {/*          input */ {0xF8, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (5 bytes, U+E0061)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0xFC, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (6 bytes, U+E0061)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0xFE, 0x80, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (7 bytes, U+E0061)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0xFF, 0x80, 0x80, 0x80, 0x83, 0xA0, 0x81, 0xA1}, // 5+ Byte (8 bytes, U+E0061)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}
            
            }, {/*          input */ {0xC4}, // Too Short (at the end, should be 2-byte long, 1 byte missing, U+0104)
                /* expected error */ {.valid_up_to = 0, .error_length = std::nullopt}

            }, {/*          input */ {0xC4, u8'A'}, // Too Short (in the middle, should be 2-byte long, 1 byte missing, U+0104)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0xE1}, // Too Short (at the end, should be 3-byte long, 2 bytes missing, U+10C4)
                /* expected error */ {.valid_up_to = 0, .error_length = std::nullopt}

            }, {/*          input */ {0xE1, u8'0'}, // Too Short (in the middle, should be 3-byte long, 2 bytes missing, U+10C4)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}
            
            }, {/*          input */ {0xE1, 0x83}, // Too Short (at the end, should be 3-byte long, 1 byte missing, U+10C4)
                /* expected error */ {.valid_up_to = 0, .error_length = std::nullopt}

            }, {/*          input */ {0xE1, 0x83, u8' '}, // Too Short (in the middle, should be 3-byte long, 1 byte missing, U+10C4)
                /* expected error */ {.valid_up_to = 0, .error_length = 2}

            }, {/*          input */ {0xF3}, // Too Short (at the end, should be 4-byte long, 3 bytes missing, U+E0198)
                /* expected error */ {.valid_up_to = 0, .error_length = std::nullopt}

            }, {/*          input */ {0xF3, u8' '}, // Too Short (in the middle, should be 4-byte long, 3 bytes missing, U+E0198)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0xF3, 0xA0}, // Too Short (at the end, should be 4-byte long, 2 bytes missing, U+E0198)
                /* expected error */ {.valid_up_to = 0, .error_length = std::nullopt}

            }, {/*          input */ {0xF3, 0xA0, u8' '}, // Too Short (in the middle, should be 4-byte long, 2 bytes missing, U+E0198)
                /* expected error */ {.valid_up_to = 0, .error_length = 2}

            }, {/*          input */ {0xF3, 0xA0, 0x86}, // Too Short (at the end, should be 4-byte long, 1 byte missing, U+E0198)
                /* expected error */ {.valid_up_to = 0, .error_length = std::nullopt}

            }, {/*          input */ {0xF3, 0xA0, 0x86, u8' '}, // Too Short (in the middle, should be 4-byte long, 1 byte missing, U+E0198)
                /* expected error */ {.valid_up_to = 0, .error_length = 3}

            }, {/*          input */ {0xC0, 0xB6}, // Overlong (2-byte long, U+0036)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0xE0, 0x97, 0x9F}, // Overlong (3-byte long, U+05DF)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0xF0, 0x8F, 0xAC, 0xB4}, // Overlong (4-byte long, U+FB34)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0xF7, 0x8C, 0x9A, 0x8D}, // Too Large (detectable at 1st byte)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0xF4, 0x90, 0x91, 0xB1}, // Too Large (detectable at 2nd byte)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}

            }, {/*          input */ {0xED, 0xA1, 0xB4}, // Surrogate (0xD874)
                /* expected error */ {.valid_up_to = 0, .error_length = 1}
            },
        });
    }
} // namespace upp_test::utf8

#endif // TEST_UTF8_HPP