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

include(ExternalProject)

macro(add_external_project NAME VERSION)
    message(STATUS "${NAME} -- ${CMAKE_SYSTEM_NAME} ${CMAKE_BUILD_TYPE} ${VERSION}")

    add_subdirectory(${NAME})

    string(TOLOWER ${CMAKE_SYSTEM_NAME} PKG_SYSTEM_NAME)

    if("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        set(PKG_BUILD_TYPE "release")
    else()
        string(TOLOWER ${CMAKE_BUILD_TYPE} PKG_BUILD_TYPE)
    endif()

    ExternalProject_Add(${NAME}
        URL file://${CMAKE_SOURCE_DIR}/depend/${NAME}/package/${NAME}-${PKG_BUILD_TYPE}-${VERSION}-${PKG_SYSTEM_NAME}.tar.gz
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        PREFIX ${CMAKE_BINARY_DIR}/${NAME}
        INSTALL_COMMAND ""
        )
endmacro()

add_external_project(boost 1.70.0)
add_external_project(openssl 1.1.1)

if (BUILD_TESTS)
    add_external_project(googletest 1.8.0)
endif()

if (NOT USE_JSONCPP_SO)
    add_external_project(jsoncpp 1.8.4)
endif()

