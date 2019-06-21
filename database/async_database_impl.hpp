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

#include <include/bluzelle.hpp>
#include <include/async_database.hpp>
#include <database/db_dispatch_base.hpp>
#include <library/mutable_response.hpp>
#include <swarm/swarm_base.hpp>


namespace bzapi
{
    class db_dispatch_base;

    class async_database_impl : public async_database, public std::enable_shared_from_this<async_database_impl>
    {
    public:

        async_database_impl(std::shared_ptr<db_dispatch_base> db_impl, std::shared_ptr<swarm_base> swarm, uuid_t uuid);
        ~async_database_impl();

        void open(completion_handler_t handler);

        std::shared_ptr<response> create(const std::string& key, const std::string& value, uint64_t expiry) override;
        std::shared_ptr<response> read(const std::string& key) override;
        std::shared_ptr<response> update(const std::string& key, const std::string& value) override;
        std::shared_ptr<response> remove(const std::string& key) override;

        std::shared_ptr<response> quick_read(const std::string& key) override;
        std::shared_ptr<response> has(const std::string& key) override;
        std::shared_ptr<response> keys() override;
        std::shared_ptr<response> size() override;
        std::shared_ptr<response> expire(const std::string& key, uint64_t expiry) override;
        std::shared_ptr<response> persist(const std::string& key) override;
        std::shared_ptr<response> ttl(const std::string& key) override;

        std::shared_ptr<response> writers() override;
        std::shared_ptr<response> add_writer(const std::string& writer) override;
        std::shared_ptr<response> remove_writer(const std::string& writer) override;

        std::string swarm_status() override;

    private:

        enum class init_state {none, initializing, initialized} state{init_state::none};

        std::shared_ptr<db_dispatch_base> db_impl;
        std::shared_ptr<swarm_base> swarm;
        uuid_t uuid;

        static void translate_swarm_response(const database_response& db_response, const boost::system::error_code& ec
            , std::shared_ptr<mutable_response> resp
            , std::function<void(const database_response& response)> handler);

        void send_message_with_basic_response(database_msg& msg, std::shared_ptr<mutable_response> resp);
    };
}
