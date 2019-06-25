cmake_minimum_required(VERSION 3.10)

project(bzapi CXX)

# for now...
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 1)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

option(OMIT_LIBRARY "Omit compiling the shared library" OFF)

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
endif(APPLE)

set(CMAKE_CXX_FLAGS ${warnings})
set(CMAKE_C_FLAGS   ${warnings})

if(NOT BUILD_DEPEND)
    add_subdirectory(depend)
else()
    include(cmake/boost.cmake)
    include(cmake/googletest.cmake)
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
add_subdirectory(node)
add_subdirectory(swarm)
add_subdirectory(utils)

if (NOT OMIT_LIBRARY)
    add_subdirectory(library)
    if (NOT APPLE)
        add_subdirectory(pkg)
    endif()
endif(NOT OMIT_LIBRARY)

include(cmake/static_analysis.cmake)
