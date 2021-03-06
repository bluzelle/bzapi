//
// Copyright (C) 2019 Bluzelle
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include "response.hpp"
#include <memory>


namespace bzapi
{
    /// The async_database class provides access to a swarmDB database for CRUD and maintenance
    /// operations. The methods of this class execute asynchronously and return a response object
    /// which can be used to fetch the result of the operation, which is expressed in a
    /// JSON structure detailed below.
    class async_database
    {
    public:
        /// get the current status of the database's swarm
        /// @return - JSON structure containing status information
        virtual std::string swarm_status() = 0;

        /// create a key/value in the database.
        /// @param key - name of the key to create
        /// @param value - value to set the key to
        /// @param expiry - lifetime of key/value in seconds (0 = forever)
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 on success, or
        /// error - set to error message
        virtual std::shared_ptr<response> create(const std::string& key, const std::string& value, uint64_t expiry) = 0;

        /// get the value of a key in the database.
        /// @param key - name of the key to read
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 on success
        /// key - set to the name of the key
        /// value - set to the value of the key, or
        /// error - set to error message
        virtual std::shared_ptr<response> read(const std::string& key) = 0;

        /// update the value of a key in the database.
        /// @param key - name of the key to update
        /// @param value - value to set the key to
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 on success, or
        /// error - set to error message
        virtual std::shared_ptr<response> update(const std::string& key, const std::string& value) = 0;

        /// remove a key and its value from the database.
        /// @param key - name of the key to remove
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 on success, or
        /// error - set to error message
        virtual std::shared_ptr<response> remove(const std::string& key) = 0;

        /// get the value of a key in the database without enforcing consensus
        /// @param key - name of the key to read
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 on success
        /// key - set to the name of the key
        /// value - set to the value of the key, or
        /// error - set to error message
        virtual std::shared_ptr<response> quick_read(const std::string& key) = 0;

        /// determine if a key exist in the database.
        /// @param key - name of the key to look for
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 if key exists, 0 if not, or
        /// error - set to error message
        virtual std::shared_ptr<response> has(const std::string& key) = 0;

        /// obtain a list of the keys in the database
        /// @return - response containing JSON structure with the following members:
        /// keys - array of strings containing the list of keys, or
        /// error - set to error message
        virtual std::shared_ptr<response> keys() = 0;

        /// obtain information about the utilization of the database
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 on success
        /// bytes - size in bytes of the database
        /// keys - number of keys in the database
        /// remaining_bytes - space left in the allocation for the database
        /// max_size - maximum allowed size for the database, or
        /// error - set to error message
        virtual std::shared_ptr<response> size() = 0;

        /// set the expiry time for a key/value in the database
        /// @param key - name of the key to set expiry for
        /// @param expiry - expiry time
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 for success, or
        /// error - set to error message
        virtual std::shared_ptr<response> expire(const std::string& key, uint64_t expiry) = 0;

        /// set a key/value in the database to be persistent
        /// @param key - name of the key to persist
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 for success, or
        /// error - set to error message
        virtual std::shared_ptr<response> persist(const std::string& key) = 0;

        /// get the expiry time for key/value in the database
        /// @param key - name of the key to get expiry for
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 for success
        /// key - name of key
        /// ttl - time to live for the key, or
        /// error - set to error message
        virtual std::shared_ptr<response> ttl(const std::string& key) = 0;

        /// obtain a list of the users with write access to the database
        /// @return - response containing JSON structure with the following members:
        /// writers - array of strings containing the list of writers, or
        /// error - set to error message
        virtual std::shared_ptr<response> writers() = 0;

        /// give a user write access to the database
        /// @param writer - identity of user to give write access to
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 for success
        /// error - set to error message
        virtual std::shared_ptr<response> add_writer(const std::string& writer) = 0;

        /// revoke a user's write access to the database
        /// @param writer - identity of user to remove write access from
        /// @return - response containing JSON structure with the following members:
        /// result - set to 1 for success
        /// error - set to error message
        virtual std::shared_ptr<response> remove_writer(const std::string& writer) = 0;

        virtual ~async_database() = default;
    };
}
