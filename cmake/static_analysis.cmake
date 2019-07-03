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

add_custom_target(static_analysis)

# copy paste detection...
if (NOT DEFINED "PMD_EXE")
    find_program(PMD_EXE NAMES pmd run.sh)
endif()
message(STATUS "pmd: ${PMD_EXE}")

if (PMD_EXE)
    add_custom_target(cpd COMMAND ${CMAKE_SOURCE_DIR}/cmake/static_analysis.sh cpd ${PMD_EXE} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
    add_dependencies(static_analysis cpd)
endif()

# complexity analysis...
find_program(PMCCABE_EXE NAMES pmccabe)
message(STATUS "pmccabe: ${PMCCABE_EXE}")

if (PMCCABE_EXE)
    add_custom_target(pmccabe COMMAND ${CMAKE_SOURCE_DIR}/cmake/static_analysis.sh pmccabe ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
    add_dependencies(static_analysis pmccabe)
endif()
