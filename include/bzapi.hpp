// Copyright (C) 2018 Bluzelle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License, version 3,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include "response.hpp"
#include "database.hpp"

/// bzapi is a C++ API that a client can use to connect to the Bluzelle SwarmDB distributed database & cache.
///
namespace bzapi
{
    /// Initialize the bzapi library - call prior to any other method.
    /// @param public_key - client elliptic curve key / user id
    /// @param private_key - client private key used for signing
    /// @param endpoint - initial swarm endpoint (deprecated)
    /// @param swarm_id - id of initial swarm (deprecated)
    /// @return - true if successful, otherwise false
    bool initialize(const std::string& public_key, const std::string& private_key
        , const std::string& endpoint, const std::string& swarm_id);

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
    /// @return - a database object whose methods execute sychronously,
    /// or a nullptr if an error occurred
    std::shared_ptr<database> create_db(const std::string& uuid);

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
    /// @return - response that can be queried for the result of the operation
    std::shared_ptr<response> async_create_db(const std::string& uuid);

    /// Open an existing database with the given uuid on the network.
    /// Create a new database with the given uuid on the network.
    /// This method executes asynchronously and provides access to a
    /// database object whose methods execute asynchronously.
    /// @param uuid - identity of the database to open
    /// @return - response that can be queried for the result of the operation
    std::shared_ptr<response> async_open_db(const std::string& uuid);

    /// Get the numerical identifier for the most recent error that occurred.
    /// @return - error number
    int get_error();

    /// Get the text description of the most recent error that occurred.
    /// @return - error message
    std::string get_error_str();
}