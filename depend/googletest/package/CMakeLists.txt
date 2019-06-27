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

project(googletest CXX)

include(ExternalProject)

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
set(URL https://github.com/google/googletest/archive/release-${PKG_VER}.tar.gz)
set(PREFIX ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE})

message(STATUS "${PROJECT_NAME} -- ${PKG_BUILD_TYPE} ${PKG_VER}")
message(STATUS "${PROJECT_NAME} -- URL: ${URL}")
message(STATUS "${PROJECT_NAME} -- URL_HASH: ${PKG_HASH}")

ExternalProject_Add(googletest
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/googletest"
    URL ${URL}
    URL_HASH ${URL_HASH_OPTION}
    TIMEOUT 30
	CMAKE_ARGS
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
 	-DCMAKE_INSTALL_PREFIX=${PREFIX}
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
