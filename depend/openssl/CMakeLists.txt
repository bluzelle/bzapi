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

set(SOURCE_DIR ${CMAKE_BINARY_DIR}/openssl/src/openssl)

set(OPENSSL_INCLUDE_DIR ${SOURCE_DIR}/include CACHE INTERNAL "")

set(OPENSSL_LIBRARIES
    ${SOURCE_DIR}/lib/libssl.a
    ${SOURCE_DIR}/lib/libcrypto.a
    dl
    pthread
    CACHE INTERNAL "")
