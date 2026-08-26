project("uni-cpp-test" LANGUAGES CXX)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.hpp src/*.inl)

set(UNI_CPP_TEST_SOURCES ${TEST_SOURCES} PARENT_SCOPE)

add_executable(${PROJECT_NAME} ${TEST_SOURCES})

include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.0
)
FetchContent_MakeAvailable(catch2)

target_link_libraries(${PROJECT_NAME} PRIVATE "uni-cpp" Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(${PROJECT_NAME} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test_data")

# Enable extra warnings

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Increase the constexpr limits (for compile time tests)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /constexpr:steps1000000000 /constexpr:depth2048)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -fconstexpr-steps=1000000000 -fconstexpr-depth=2048)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -fconstexpr-ops-limit=1000000000 -fconstexpr-depth=2048)
endif()

# Make a clang-tidy target

set(CPP_TEST_SOURCES ${TEST_SOURCES})
list(FILTER CPP_TEST_SOURCES INCLUDE REGEX "\\.cpp$")

add_custom_target(clang-tidy
    COMMAND ${CMAKE_COMMAND} -E echo "Running clang-tidy on ${PROJECT_NAME}..."
    COMMAND clang-tidy --quiet --config-file=.clang-tidy ${CPP_TEST_SOURCES}
        -- -Iinclude/ "$<$<BOOL:$<TARGET_PROPERTY:Catch2::Catch2WithMain,INCLUDE_DIRECTORIES>>:-I$<JOIN:$<TARGET_PROPERTY:Catch2::Catch2WithMain,INCLUDE_DIRECTORIES>,;-I>>" # include directories
        -std=c++23 -fconstexpr-steps=1000000000 -fconstexpr-depth=2048
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../
    COMMENT "Static analysis with clang-tidy"
    COMMAND_EXPAND_LISTS
    VERBATIM
)