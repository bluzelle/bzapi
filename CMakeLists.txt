cmake_minimum_required(VERSION 3.13)

project(bzapi CXX)

include(cmake/bzapi_version.cmake)

message("Build version ${PROJECT_VERSION}")

option(STATIC_LIBRARY "Compiling a static library" OFF)
option(BUILD_TESTS "Build unit tests" ON)

# ensure type is set
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# use ccache if available...
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# output dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

# compiler options...
set(CMAKE_CXX_STANDARD 17)
add_compile_options("-fdiagnostics-color=auto")
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# todo: remove -Wno-implicit-fallthrough once CI moves past gcc 7.4.0...
set(warnings "-Wno-deprecated-declarations -Wall -Wextra -Werror -Wpedantic -Wno-implicit-fallthrough")
if (APPLE)
    set(warnings "${warnings} -Wno-extended-offsetof")
else()
    # for beast and gcc release builds...
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set(warnings "${warnings} -Wno-maybe-uninitialized")
    endif()

    # todo: these may no longer be required...
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.2)
        message(STATUS "GCC ${CMAKE_CXX_COMPILER_VERSION} warning workaround: -Wno-deprecated-copy -Wno-sign-compare (Still needed?)")
        set(warnings "${warnings} -Wno-deprecated-copy -Wno-sign-compare")
    endif()
endif()

set(CMAKE_CXX_FLAGS ${warnings})
set(CMAKE_C_FLAGS   ${warnings})

if(NOT BUILD_DEPEND)
    add_subdirectory(depend)
else()
    include(cmake/boost.cmake)
    if (BUILD_TESTS)
        include(cmake/googletest.cmake)
    endif()
    include(cmake/openssl.cmake)
    if (NOT USE_JSONCPP_SO)
        include(cmake/jsoncpp.cmake)
    endif()
endif()

# use jsoncpp shared library instead?
if (USE_JSONCPP_SO)
    include(FindPkgConfig)
    pkg_search_module(JSONCPP REQUIRED jsoncpp)
    include_directories(SYSTEM ${JSONCPP_INCLUDE_DIRS})
    link_directories(${JSONCPP_LIBRARY_DIRS})
endif()

include(cmake/add_gmock_test.cmake)

# find packages
#set(Protobuf_USE_STATIC_LIBS ON) # not built with PIC!
include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()

# coverage?
if (ENABLE_GCOV)
    if (NOT APPLE)
        set(COVERAGE_COMPILER_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage" CACHE INTERNAL "")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${COVERAGE_COMPILER_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILER_FLAGS}")
        message(STATUS "Appending code coverage compiler flags: ${COVERAGE_COMPILER_FLAGS}")
    endif()
endif()

add_subdirectory(proto)

set(BLUZELLE_STD_INCLUDES ${Boost_INCLUDE_DIRS} ${GTEST_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIRS} ${PROTO_INCLUDE_DIR})

add_subdirectory(crypto)
add_subdirectory(database)
add_subdirectory(library)
add_subdirectory(node)
add_subdirectory(swarm)
add_subdirectory(utils)

if (NOT STATIC_LIBRARY)
    add_subdirectory(bzapi-cli)
    if (NOT APPLE)
        add_subdirectory(pkg)
    endif()
endif()

include(cmake/static_analysis.cmake)
