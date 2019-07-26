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
#include <include/boost_asio_beast.hpp>
#include <crypto/crypto_base.hpp>
#include <database/async_database_impl.hpp>
#include <swarm/swarm_base.hpp>
#include <swarm/esr_base.hpp>
#include <utils/peer_address.hpp>

namespace bzapi
{
    class swarm_factory : public std::enable_shared_from_this<swarm_factory>
    {
    public:

        swarm_factory(std::shared_ptr<bzn::asio::io_context_base> io_context
            , std::shared_ptr<bzn::beast::websocket_base> ws_factory
            , std::shared_ptr<crypto_base> crypto
            , std::shared_ptr<esr_base> esr
            , const uuid_t& uuid);

        void initialize(const std::string& esr_address, const std::string& url);
        void initialize(const swarm_id_t& default_swarm, const std::vector<std::pair<node_id_t, bzn::peer_address_t>>& nodes);

        void has_db(const uuid_t& uuid, std::function<void(db_error, std::shared_ptr<swarm_base>)>);
        void create_db(const uuid_t& uuid, uint64_t max_size, bool random_evict, std::function<void(db_error, std::shared_ptr<swarm_base>)>);

    private:
        class swarm_registry
        {
        public:
            using swarm_id_t = std::string;
            using node_map = std::map<node_id_t, bzn::peer_address_t>;
            struct swarm_info
            {
                node_map nodes;
                std::weak_ptr<swarm_base> swarm;
            };

            void add_node(const swarm_id_t& swarm_id, const node_id_t& node_id, const bzn::peer_address_t& endpoint);
            std::weak_ptr<swarm_base> get_swarm(const swarm_id_t& swarm_id);
            void set_swarm(const swarm_id_t& swarm_id, std::shared_ptr<swarm_base> swarm);
            std::vector<swarm_id_t> get_swarms();
            std::vector<std::pair<node_id_t, bzn::peer_address_t>> get_nodes(const swarm_id_t& swarm_id);

        private:
            std::map<swarm_id_t, swarm_info> swarms;
        };

        const std::shared_ptr<bzn::asio::io_context_base> io_context;
        const std::shared_ptr<bzn::beast::websocket_base> ws_factory;
        const std::shared_ptr<crypto_base> crypto;
        const std::shared_ptr<esr_base> esr;
        const uuid_t my_uuid;
        const std::shared_ptr<node_factory_base> node_factory;

        std::string esr_address;
        std::string esr_url;
        bool initialized = false;
        std::shared_ptr<swarm_registry> swarm_reg;
        std::map<uuid_t, swarm_id_t> swarm_dbs;

        std::shared_ptr<swarm_base> get_or_create_swarm(const swarm_id_t& swarm_id);
        void update_swarm_registry();
        void select_swarm_for_size(uint64_t size, uint64_t hint, std::function<void(const std::string& swarm_id)> callback);
        void do_create_db(const uuid_t& uuid, uint64_t max_size, bool random_evict, uint64_t retry, std::function<void(db_error, std::shared_ptr<swarm_base>)>);
    };
}
