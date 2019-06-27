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

include(ExternalProject)
include(ProcessorCount)


# Prevent travis gcc crashes...
if (DEFINED ENV{TRAVIS})
    set(BUILD_FLAGS -j8)
else()
    ProcessorCount(N)
    if(NOT N EQUAL 0)
        set(BUILD_FLAGS -j${N})
    endif()
endif()

# platform detection
if (APPLE)
    set(OPENSSL_BUILD_PLATFORM darwin64-x86_64-cc)
else()
    set(OPENSSL_BUILD_PLATFORM linux-generic64)
endif()

ExternalProject_Add(openssl
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/openssl
    URL https://www.openssl.org/source/openssl-1.1.1.tar.gz
    URL_HASH SHA256=2836875a0f89c03d0fdf483941512613a50cfb421d6fd94b9f41d7279d586a3d
    TIMEOUT 30
    INSTALL_COMMAND ""
    DOWNLOAD_NO_PROGRESS true
    CONFIGURE_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/openssl/src/openssl/Configure" ${OPENSSL_BUILD_PLATFORM}
    BUILD_COMMAND make ${BUILD_FLAGS}
    )

set_property(DIRECTORY PROPERTY CLEAN_NO_CUSTOM ${CMAKE_CURRENT_BINARY_DIR}/openssl)

ExternalProject_Get_Property(openssl source_dir)
ExternalProject_Get_Property(openssl binary_dir)

set(OPENSSL_INCLUDE_DIR ${source_dir}/include ${binary_dir}/include)
set(OPENSSL_LIBRARIES ${binary_dir}/libssl.a ${binary_dir}/libcrypto.a dl pthread)
