cmake_minimum_required(VERSION 3.30)

project("uni-cpp" VERSION 0.1.0 LANGUAGES CXX)

set(UNI_CPP_UNICODE_VERSION "16.0.0")

string(REPLACE "." ";" UNI_CPP_UNICODE_VERSION_PARTS ${UNI_CPP_UNICODE_VERSION})
list(GET UNI_CPP_UNICODE_VERSION_PARTS 0 UNI_CPP_UNICODE_VERSION_MAJOR)
list(GET UNI_CPP_UNICODE_VERSION_PARTS 1 UNI_CPP_UNICODE_VERSION_MINOR)
list(GET UNI_CPP_UNICODE_VERSION_PARTS 2 UNI_CPP_UNICODE_VERSION_PATCH)

string(REPLACE "." ";" UNI_CPP_VERSION_PARTS ${PROJECT_VERSION})
list(GET UNI_CPP_VERSION_PARTS 0 UNI_CPP_VERSION_MAJOR)
list(GET UNI_CPP_VERSION_PARTS 1 UNI_CPP_VERSION_MINOR)
list(GET UNI_CPP_VERSION_PARTS 2 UNI_CPP_VERSION_PATCH)

math(EXPR UNI_CPP_UNICODE_VERSION_NUMBER "(${UNI_CPP_UNICODE_VERSION_MAJOR} * 1000000) + (${UNI_CPP_UNICODE_VERSION_MINOR} * 1000) + ${UNI_CPP_UNICODE_VERSION_PATCH}" OUTPUT_FORMAT DECIMAL)
math(EXPR UNI_CPP_VERSION_NUMBER "(${UNI_CPP_VERSION_MAJOR} * 1000000) + (${UNI_CPP_VERSION_MINOR} * 1000) + ${UNI_CPP_VERSION_PATCH}" OUTPUT_FORMAT DECIMAL)

message("[uni-cpp] Unicode version used: ${UNI_CPP_UNICODE_VERSION}")

configure_file(cmake/unicode_version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/uni_cpp/impl/unicode_version.hpp)
configure_file(cmake/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/uni_cpp/impl/version.hpp)

set(UNI_CPP_HEADERS
    include/uni_cpp/all.hpp
    include/uni_cpp/char.hpp
    include/uni_cpp/config.hpp
    include/uni_cpp/string.hpp
    include/uni_cpp/version.hpp
    include/uni_cpp/impl/unicode_version.hpp
    include/uni_cpp/impl/version.hpp
    )

set(UNI_CPP_TEST_SOURCES
    test/src/all.cpp
    test/src/char.cpp
    test/src/string.cpp
    )

add_library(${PROJECT_NAME} INTERFACE ${UNI_CPP_HEADERS})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_26)

find_program(CLANG_FORMAT_PATH clang-format REQUIRED)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

add_custom_target(format
    COMMAND ${CLANG_FORMAT_PATH} --style=file:.clang-format --dry-run ${UNI_CPP_HEADERS} ${UNI_CPP_TEST_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format --dry-run. Use format-inplace target to confirm changes"
)

add_custom_target(format-inplace
    COMMAND ${CLANG_FORMAT_PATH} --style=file:.clang-format --i ${UNI_CPP_HEADERS} ${UNI_CPP_TEST_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format inplace"
)