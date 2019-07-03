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

ExternalProject_Add(googletest
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/googletest"
    URL https://github.com/google/googletest/archive/release-1.8.0.tar.gz
    URL_HASH SHA256=58a6f4277ca2bc8565222b3bbd58a177609e9c488e8a72649359ba51450db7d8
    TIMEOUT 30
    INSTALL_COMMAND ""
    DOWNLOAD_NO_PROGRESS true
    )

set_property(DIRECTORY PROPERTY CLEAN_NO_CUSTOM
    "${CMAKE_CURRENT_BINARY_DIR}/googletest")

ExternalProject_Get_Property(googletest source_dir)
include_directories(${source_dir}/googlemock/include ${source_dir}/googletest/include)

ExternalProject_Get_Property(googletest binary_dir)
link_directories(${binary_dir}/googlemock)

set(GMOCK_BOTH_LIBRARIES gmock_main gmock)
