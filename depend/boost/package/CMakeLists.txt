#
# Copyright (C) 2019 Bluzelle
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.10)

project(boost CXX)

include(ExternalProject)
include(ProcessorCount)

if (NOT DEFINED PKG_VER)
  message(FATAL_ERROR "usage: -DPKG_VER=x.y.z [-DPKG_HASH=<sha256>]")
endif()

if(PKG_HASH)
  set(URL_HASH_OPTION SHA256=${PKG_HASH})
else()
  message(WARNING "PKG_HASH not specified! Downloaded package will not be validated.")
endif()

# default to debug builds
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} PKG_BUILD_TYPE)
set(PREFIX ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE})

set(BOOST_TARBALL "boost_${PKG_VER}")
string(REPLACE "." "_" BOOST_TARBALL ${BOOST_TARBALL})
string(APPEND BOOST_TARBALL ".tar.gz")
set(BOOST_LIBS "chrono,program_options,random,regex,system,thread,log,serialization")
set(URL https://dl.bintray.com/boostorg/release/${PKG_VER}/source/${BOOST_TARBALL})

ProcessorCount(NPROC)

message(STATUS "${PROJECT_NAME} -- ${PKG_BUILD_TYPE} ${PKG_VER} (${BOOST_LIBS})")
message(STATUS "${PROJECT_NAME} -- URL: ${URL}")
message(STATUS "${PROJECT_NAME} -- URL_HASH: ${PKG_HASH}")

# Boost built with PIC flag...
ExternalProject_Add(boost
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/boost
    URL ${URL}
    URL_HASH ${URL_HASH_OPTION}
    TIMEOUT 120
    CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/boost/src/boost/bootstrap.sh --prefix=${PREFIX} --with-libraries=${BOOST_LIBS}
    BUILD_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/boost/src/boost/b2" variant=${PKG_BUILD_TYPE} cxxflags=-fPIC cflags=-fPIC link=static visibility=global -j${NPROC} install
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE true
    DOWNLOAD_NO_PROGRESS true
    )

set(CPACK_GENERATOR TGZ)
string(REPLACE "." ";" VERSION_LIST ${PKG_VER})
list(GET VERSION_LIST 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET VERSION_LIST 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET VERSION_LIST 2 CPACK_PACKAGE_VERSION_PATCH)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME}-${PKG_BUILD_TYPE})
string(TOLOWER ${CMAKE_SYSTEM_NAME} CPACK_SYSTEM_NAME)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE}/ DESTINATION ${PKG_BUILD_TYPE}/)

include(CPack)
