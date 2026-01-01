project("uni-cpp-test" LANGUAGES CXX)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.hpp src/*.inl)

set(UNI_CPP_TEST_SOURCES ${TEST_SOURCES} PARENT_SCOPE)

add_executable(${PROJECT_NAME} ${TEST_SOURCES})

include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(catch2)

target_link_libraries(${PROJECT_NAME} PRIVATE "uni-cpp" Catch2::Catch2WithMain)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_tests_properties(${PROJECT_NAME} PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test_data")