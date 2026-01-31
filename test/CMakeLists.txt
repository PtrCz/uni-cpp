project("uni-cpp-test" LANGUAGES CXX)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.hpp src/*.inl)

set(UNI_CPP_TEST_SOURCES ${TEST_SOURCES} PARENT_SCOPE)

add_executable(${PROJECT_NAME} ${TEST_SOURCES})

set(BUGSPRAY_DONT_USE_STD_VECTOR ON)
set(BUGSPRAY_DONT_USE_STD_STRING ON)

include(FetchContent)
FetchContent_Declare(
    bugspray
    GIT_REPOSITORY https://github.com/jan-moeller/bugspray.git
    GIT_TAG v0.4.1
)
FetchContent_MakeAvailable(bugspray)

target_link_libraries(${PROJECT_NAME} PRIVATE "uni-cpp" bugspray-with-main)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_CLANG_TIDY "clang-tidy;--config-file=${CMAKE_CURRENT_SOURCE_DIR}/../.clang-tidy"
)

# Enable extra warnings

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

set_target_properties(bugspray PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:bugspray,INTERFACE_INCLUDE_DIRECTORIES>)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_tests_properties(${PROJECT_NAME} PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test_data")