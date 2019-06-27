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

set(SOURCE_DIR ${CMAKE_BINARY_DIR}/googletest/src/googletest)

set(GTEST_INCLUDE_DIR ${SOURCE_DIR}/include CACHE INTERNAL "")
set(GMOCK_INCLUDE_DIR ${SOURCE_DIR}/include CACHE INTERNAL "")

set(GMOCK_BOTH_LIBRARIES
    ${SOURCE_DIR}/lib/libgmock_main.a
    ${SOURCE_DIR}/lib/libgmock.a
    CACHE INTERNAL "")

set(GTEST_BOTH_LIBRARIES
    ${SOURCE_DIR}/lib/libgtest_main.a
    ${SOURCE_DIR}/lib/libgtest.a
    CACHE INTERNAL "")
