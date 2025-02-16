cmake_minimum_required(VERSION 3.30)

project("uni-cpp-test" LANGUAGES CXX)

set(TEST_SOURCES)
foreach(SOURCE IN LISTS UNI_CPP_TEST_SOURCES)
    list(APPEND TEST_SOURCES "../${SOURCE}")
endforeach()

add_executable(${PROJECT_NAME} ${TEST_SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE UNI_CPP_FORCE_ENABLE_ASSERT)

include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(catch2)

target_link_libraries(${PROJECT_NAME} PRIVATE "uni-cpp" Catch2::Catch2WithMain)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})