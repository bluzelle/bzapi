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

#include <database/db_impl_base.hpp>
#include <swarm/swarm_base.hpp>
#include <library/response.hpp>

using namespace bzapi;
namespace bzapi
{
    using uuid_t = std::string;
    using endpoint_t = std::string;
    using completion_handler_t = std::function<void(const boost::system::error_code &error)>;
    using key_t = std::string;
    using value_t = std::string;
    using expiry_t = uint64_t;

    enum class db_error
    {
        success = 0,
        connection_error,
        database_error,
        no_database
    };

    using nonce_t = uint64_t;

    class async_database : public std::enable_shared_from_this<async_database>
    {
    public:

        async_database(std::shared_ptr<db_impl_base> db_impl);
        virtual ~async_database() = default;

        void open(completion_handler_t handler);

        std::shared_ptr<response> create(const key_t& key, const value_t& value);
        std::shared_ptr<response> read(const key_t& key);
        std::shared_ptr<response> update(const key_t& key, const value_t& value);
        std::shared_ptr<response> remove(const key_t& key);

        std::shared_ptr<response> quick_read(const key_t& key);
        std::shared_ptr<response> has(const key_t& key);
        std::shared_ptr<response> keys();
        std::shared_ptr<response> size();
        std::shared_ptr<response> expire(const key_t& key, expiry_t expiry);
        std::shared_ptr<response> persist(const key_t& key);
        std::shared_ptr<response> ttl(const key_t& key);

        std::string swarm_status();

    private:

        enum class init_state {none, initializing, initialized} state{init_state::none};

        std::shared_ptr<db_impl_base> db_impl;

        static void translate_swarm_response(const database_response& db_response, const boost::system::error_code& ec
            , std::shared_ptr<response> resp
            , std::function<void(const database_response& response)> handler);

        void send_message_with_basic_response(database_msg& msg, std::shared_ptr<response> resp);
    };
}