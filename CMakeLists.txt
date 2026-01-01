cmake_minimum_required(VERSION 3.30)

project("uni-cpp" VERSION 0.3.0 LANGUAGES CXX)

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

configure_file(cmake/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/uni-cpp/version.hpp)

file(GLOB_RECURSE UNI_CPP_SOURCES CONFIGURE_DEPENDS "include/*.hpp")

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if (PROJECT_IS_TOP_LEVEL)
    find_program(CLANG_FORMAT_PATH clang-format)

    if (CLANG_FORMAT_PATH)
        add_custom_target(format
            COMMAND ${CLANG_FORMAT_PATH} --style=file:.clang-format -i ${UNI_CPP_SOURCES} ${UNI_CPP_TEST_SOURCES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running clang-format inplace"
            VERBATIM
        )
    else()
        message(WARNING "clang-format not found! 'format' target will not be available.")
    endif()

    find_package(Doxygen)
    find_package(Python COMPONENTS Interpreter)

    if(DOXYGEN_FOUND AND Python_FOUND AND CLANG_FORMAT_PATH)
        set(DOXYGEN_PREPROCESSED_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen_preprocessed/uni-cpp)

        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        set(DOXY_PREPROCESSED_FILES)

        foreach(src ${UNI_CPP_SOURCES})
            file(RELATIVE_PATH rel
                ${CMAKE_CURRENT_SOURCE_DIR}/include/uni-cpp
                ${src})

            set(out ${DOXYGEN_PREPROCESSED_DIR}/${rel})

            get_filename_component(out_dir ${out} DIRECTORY)

            add_custom_command(
                OUTPUT ${out}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
                COMMAND ${Python_EXECUTABLE}
                        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/preprocess-doxygen.py
                        ${src}
                        ${out}
                COMMAND ${CLANG_FORMAT_PATH} -i --style=file:${CMAKE_CURRENT_SOURCE_DIR}/.clang-format ${out}
                DEPENDS ${src} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/preprocess-doxygen.py
                COMMENT "Preprocessing ${rel} for Doxygen"
                VERBATIM
            )

            list(APPEND DOXY_PREPROCESSED_FILES ${out})
        endforeach()

        add_custom_target(doxygen_preprocess
            DEPENDS ${DOXY_PREPROCESSED_FILES}
        )

        add_custom_target(doxygen
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen_preprocessed
            COMMENT "Generating documentation using Doxygen"
            VERBATIM)

        add_dependencies(doxygen doxygen_preprocess)
    else()
        message(WARNING "doxygen, python or clang-format not found! 'doxygen' target will not be available.")
    endif()
endif()