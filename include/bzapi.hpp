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
#include "database.hpp"

/// bzapi is a C++ API that a client can use to connect to the Bluzelle SwarmDB distributed database & cache.
///
namespace bzapi
{
    /// Initialize the bzapi library for use with the public Bluzelle network - call prior to any other method.
    /// @param public_key - client elliptic curve key / user id
    /// @param private_key - client private key used for signing
    /// @param esr_address - Ethereum swarm registry address, e.g. D5B3d7C061F817ab05aF9Fab3b61EEe036e4f4fc
    /// @param url - URL of ethereum network, e.g. https://ropsten.infura.io
    /// @return - true if successful, otherwise false
    bool initialize(const std::string& public_key, const std::string& private_key
        , const std::string& esr_address, const std::string& url);

    /// Initialize the bzapi library for use with a local swarm - call prior to any other method.
    /// @param public_key - client elliptic curve key / user id
    /// @param private_key - client private key used for signing
    /// @param endpoint - initial swarm endpoint (deprecated)
    /// @param node_id - id of initial swarm endpoint (deprecated)
    /// @param swarm_id - id of initial swarm (deprecated)
    /// @return - true if successful, otherwise false
    bool initialize(const std::string& public_key, const std::string& private_key
        , const std::string& endpoint, const std::string& node_id, const std::string& swarm_id);

    /// Set the number of seconds after which a request wil time out
    /// @param seconds - length of time to wait for a response
    void set_timeout(uint64_t seconds);

    /// Close down the bzapi library. Should be called prior to exit to allow
    /// cleanup of library state.
    void terminate();

    /// Determine if the given database uuid exists on the network.
    /// This method blocks until the query is resolved.
    /// @param uuid - identity of the database to search for
    /// @return - true if database exists, otherwise false
    bool has_db(const std::string& uuid);

    /// Create a new database with the given uuid on the network.
    /// This method blocks until the query is resolved.
    /// @param uuid - identity of the database to create
    /// @param max_size - the maximum size of the database (0 for infinite)
    /// @param random_evict - use random eviction policy?
    /// @return - a database object whose methods execute sychronously,
    /// or a nullptr if an error occurred
    std::shared_ptr<database> create_db(const std::string& uuid, uint64_t max_size, bool random_evict);

    /// Open an existing database with the given uuid on the network.
    /// This method blocks until the query is resolved.
    /// @param uuid - identity of the database to open
    /// @return - a database object whose methods execute sychronously,
    /// or a nullptr if an error occurred
    std::shared_ptr<database> open_db(const std::string& uuid);

    /// Determine if the given database uuid exists on the network.
    /// This method executes asynchronously.
    /// @param uuid - identity of the database to search for
    /// @return - response that can be queried for the result of the operation
    std::shared_ptr<response> async_has_db(const std::string& uuid);

    /// Create a new database with the given uuid on the network.
    /// This method executes asynchronously and provides access to a
    /// database object whose methods execute asynchronously.
    /// @param uuid - identity of the database to create
    /// @param max_size - the maximum size of the database (0 for infinite)
    /// @param random_evict - use random eviction policy?
    /// @return - response that can be queried for the result of the operation
    std::shared_ptr<response> async_create_db(const std::string& uuid, uint64_t max_size, bool random_evict);

    /// Open an existing database with the given uuid on the network.
    /// Create a new database with the given uuid on the network.
    /// This method executes asynchronously and provides access to a
    /// database object whose methods execute asynchronously.
    /// @param uuid - identity of the database to open
    /// @return - response that can be queried for the result of the operation
    std::shared_ptr<response> async_open_db(const std::string& uuid);

    /// Get the numerical identifier for the most recent error that occurred
    /// after calling has_db(), create_db() or open_db()
    /// @return - error number
    int get_error();

    /// Get the text description of the most recent error that occurred
    /// after calling has_db(), create_db() or open_db()
    /// @return - error message
    std::string get_error_str();
}
