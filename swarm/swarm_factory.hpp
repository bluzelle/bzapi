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
#include <database/db_base.hpp>
#include <boost_asio_beast.hpp>

namespace bzapi
{
    class swarm_factory
    {
    public:
        swarm_factory(std::shared_ptr<bzn::asio::io_context_base> io_context
            , std::shared_ptr<bzn::beast::websocket_base> ws_factory
            , std::shared_ptr<crypto_base> crypto);
        ~swarm_factory();

        void has_db(const uuid_t& uuid, std::function<void(db_error result)>);
        void get_swarm(const uuid_t& uuid, std::function<void(std::shared_ptr<swarm_base>)>);

        void temporary_set_default_endpoint(const endpoint_t& endpoint);

    private:
        std::shared_ptr<bzn::asio::io_context_base> io_context;
        std::shared_ptr<bzn::beast::websocket_base> ws_factory;
        std::shared_ptr<crypto_base> crypto;
        std::map<uuid_t, std::weak_ptr<swarm_base>> uuids;
        std::map<endpoint_t, std::weak_ptr<swarm_base>> swarms;
        std::shared_ptr<node_factory_base> node_factory;
        std::set<endpoint_t> endpoints;
    };
}