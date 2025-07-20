cmake_minimum_required(VERSION 3.30)

project("uni-cpp" VERSION 0.2.0 LANGUAGES CXX)

set(UNI_CPP_UNICODE_VERSION "16.0.0")

message(STATUS "[uni-cpp] Unicode version: ${UNI_CPP_UNICODE_VERSION}")

string(REPLACE "." ";" UNI_CPP_UNICODE_VERSION_PARTS ${UNI_CPP_UNICODE_VERSION})
list(GET UNI_CPP_UNICODE_VERSION_PARTS 0 UNI_CPP_UNICODE_VERSION_MAJOR)
list(GET UNI_CPP_UNICODE_VERSION_PARTS 1 UNI_CPP_UNICODE_VERSION_MINOR)
list(GET UNI_CPP_UNICODE_VERSION_PARTS 2 UNI_CPP_UNICODE_VERSION_PATCH)

string(REPLACE "." ";" UNI_CPP_VERSION_PARTS ${PROJECT_VERSION})
list(GET UNI_CPP_VERSION_PARTS 0 UNI_CPP_VERSION_MAJOR)
list(GET UNI_CPP_VERSION_PARTS 1 UNI_CPP_VERSION_MINOR)
list(GET UNI_CPP_VERSION_PARTS 2 UNI_CPP_VERSION_PATCH)

configure_file(cmake/version.ixx.in ${CMAKE_CURRENT_SOURCE_DIR}/src/version.ixx)

file(GLOB_RECURSE UNI_CPP_SOURCES CONFIGURE_DEPENDS src/*.ixx)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_sources(${PROJECT_NAME}
    PUBLIC
    FILE_SET CXX_MODULES
    FILES ${UNI_CPP_SOURCES}
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if (PROJECT_IS_TOP_LEVEL)
    find_program(CLANG_FORMAT_PATH clang-format)

    if (CLANG_FORMAT_PATH)
        add_custom_target(format
            COMMAND ${CLANG_FORMAT_PATH} --style=file:.clang-format --i ${UNI_CPP_SOURCES} ${UNI_CPP_TEST_SOURCES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running clang-format inplace"
        )
    else()
        message(WARNING "clang-format not found! 'format' target will not be available.")
    endif()

    find_package(Doxygen)

    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        add_custom_target(doxygen
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating documentation using Doxygen"
            VERBATIM)
    else()
        message(WARNING "doxygen not found! 'doxygen' target will not be available.")
    endif()
endif()