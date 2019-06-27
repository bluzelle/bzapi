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

project(openssl CXX)

include(ExternalProject)
include(ProcessorCount)

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
set(URL https://www.openssl.org/source/openssl-${PKG_VER}.tar.gz)

message(STATUS "${PROJECT_NAME} -- ${PKG_BUILD_TYPE} ${PKG_VER}")
message(STATUS "${PROJECT_NAME} -- URL: ${URL}")
message(STATUS "${PROJECT_NAME} -- URL_HASH: ${PKG_HASH}")

# platform detection
if (APPLE)
    set(OPENSSL_BUILD_PLATFORM darwin64-x86_64-cc)
    set(OPENSSLDIR /usr/local/etc/openssl)
else()
    set(OPENSSL_BUILD_PLATFORM linux-generic64)
    set(OPENSSLDIR /etc/ssl)
endif()

ProcessorCount(NPROC)

ExternalProject_Add(openssl
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/openssl"
    URL ${URL}
    URL_HASH ${URL_HASH_OPTION}
    TIMEOUT 30
    DOWNLOAD_NO_PROGRESS true
	CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/openssl/src/openssl/Configure ${OPENSSL_BUILD_PLATFORM} --prefix=${PREFIX} --${PKG_BUILD_TYPE} --openssldir=${OPENSSLDIR}
	INSTALL_COMMAND make install_sw
    BUILD_COMMAND make -j${NPROC}
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
