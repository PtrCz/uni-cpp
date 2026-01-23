project("uni-cpp-test" LANGUAGES CXX)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.hpp src/*.inl)

set(UNI_CPP_TEST_SOURCES ${TEST_SOURCES} PARENT_SCOPE)

add_executable(${PROJECT_NAME} ${TEST_SOURCES})

include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(catch2)

target_link_libraries(${PROJECT_NAME} PRIVATE "uni-cpp" Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(${PROJECT_NAME} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test_data")

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_CLANG_TIDY "clang-tidy;--config-file=${CMAKE_CURRENT_SOURCE_DIR}/../.clang-tidy"
)

# Enable extra warnings

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()