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

#include <swarm/swarm_base.hpp>
#include <crypto/crypto_base.hpp>
#include <database/async_database_impl.hpp>
#include <boost_asio_beast.hpp>
#include <utils/peer_address.hpp>

namespace bzapi
{
class swarm_factory : public std::enable_shared_from_this<swarm_factory>
    {
    private:

        class swarm_registry
        {
        public:
            using node_id_t = std::string;
            using swarm_id_t = std::string;
            using node_map = std::map<node_id_t, bzn::peer_address_t>;
            using swarm_map = std::map<swarm_id_t, node_map>;

            void add_node(const swarm_id_t& swarm_id, const node_id_t& node_id, const bzn::peer_address_t& endpoint);
            std::vector<swarm_id_t> get_swarms();
            std::vector<std::pair<node_id_t, bzn::peer_address_t>> get_nodes(swarm_id_t swarm_id);

        private:
            swarm_map swarms;
        };


    public:
        swarm_factory(std::shared_ptr<bzn::asio::io_context_base> io_context
            , std::shared_ptr<bzn::beast::websocket_base> ws_factory
            , std::shared_ptr<crypto_base> crypto
            , const uuid_t& uuid);
        ~swarm_factory();

        void initialize(const std::string& esr_address, const std::string& url);
        void get_swarm(const uuid_t& uuid, std::function<void(std::shared_ptr<swarm_base>)>);
        void has_db(const uuid_t& uuid, std::function<void(db_error result)>);
        void create_db(const uuid_t& uuid, uint64_t max_size, bool random_evict, std::function<void(db_error, std::shared_ptr<swarm_base>)>);

        void temporary_set_default_endpoint(const endpoint_t& endpoint, const swarm_id_t& swarm_id);

        void update_swarm_registry();

    private:
        std::shared_ptr<bzn::asio::io_context_base> io_context;
        std::shared_ptr<bzn::beast::websocket_base> ws_factory;
        std::shared_ptr<crypto_base> crypto;
        const uuid_t my_uuid;
        std::map<uuid_t, std::weak_ptr<swarm_base>> uuids;
        std::map<endpoint_t, std::weak_ptr<swarm_base>> swarms;
        std::shared_ptr<node_factory_base> node_factory;
        std::set<std::pair<endpoint_t, swarm_id_t>> endpoints;
        std::shared_ptr<swarm_registry> swarm_reg;
        std::string esr_address;
        std::string esr_url;
        bool initialized = false;

        std::shared_ptr<swarm_base> get_default_swarm();
    };
}