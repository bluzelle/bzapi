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

include(GoogleTest)

function(add_gmock_test target)
    set(target ${target}_tests)
    add_executable(${target} ${test_srcs})
    add_dependencies(${target} googletest ${test_deps})
    target_link_libraries(${target} ${test_libs} ${GMOCK_BOTH_LIBRARIES} ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} ${OPENSSL_LIBRARIES} ${test_link} pthread)
    target_include_directories(${target} PRIVATE ${BLUZELLE_STD_INCLUDES} ${JSONCPP_INCLUDE_DIRS})
    gtest_discover_tests(${target})
    unset(test_srcs)
    unset(test_libs)
    unset(test_deps)
    unset(test_link)
endfunction()
