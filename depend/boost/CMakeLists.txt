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

set(SOURCE_DIR ${CMAKE_BINARY_DIR}/boost/src/boost)

set(Boost_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE INTERNAL "")

set(Boost_LIBRARIES
    ${SOURCE_DIR}/lib/libboost_log.a
    ${SOURCE_DIR}/lib/libboost_program_options.a
    ${SOURCE_DIR}/lib/libboost_system.a
    ${SOURCE_DIR}/lib/libboost_thread.a
    pthread
    ${SOURCE_DIR}/lib/libboost_serialization.a
    ${SOURCE_DIR}/lib/libboost_date_time.a
    ${SOURCE_DIR}/lib/libboost_log_setup.a
    ${SOURCE_DIR}/lib/libboost_filesystem.a
    ${SOURCE_DIR}/lib/libboost_regex.a
    ${SOURCE_DIR}/lib/libboost_chrono.a
    ${SOURCE_DIR}/lib/libboost_atomic.a
    CACHE INTERNAL "")
