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

        const std::shared_ptr<db_dispatch_base> db_impl;
        const std::shared_ptr<swarm_base> swarm;
        const uuid_t uuid;

        static void translate_swarm_response(const database_response& db_response, const boost::system::error_code& ec
            , std::shared_ptr<mutable_response> resp
            , std::function<void(const database_response& response)> handler);

        void send_message_with_basic_response(database_msg& msg, std::shared_ptr<mutable_response> resp);
    };
}
