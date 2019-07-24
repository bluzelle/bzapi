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

ExternalProject_Add(jsoncpp
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp"
    URL https://github.com/open-source-parsers/jsoncpp/archive/1.8.4.tar.gz
    URL_HASH SHA256=c49deac9e0933bcb7044f08516861a2d560988540b23de2ac1ad443b219afdb6
    TIMEOUT 30
    INSTALL_COMMAND ""
    CMAKE_ARGS
    -DJSONCPP_WITH_PKGCONFIG_SUPPORT=OFF
    -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF
    -DJSONCPP_WITH_TESTS=OFF
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DOWNLOAD_NO_PROGRESS true
    )

set_property(DIRECTORY PROPERTY CLEAN_NO_CUSTOM
    "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp")

ExternalProject_Get_Property(jsoncpp source_dir)
set(JSONCPP_INCLUDE_DIRS ${source_dir}/include)
include_directories(${JSONCPP_INCLUDE_DIRS})

ExternalProject_Get_Property(jsoncpp binary_dir)
link_directories(${binary_dir}/src/lib_json/)

set(JSONCPP_LIBRARIES ${binary_dir}/src/lib_json/libjsoncpp.a)
