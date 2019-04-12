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

#include <swarm/swarm_factory.hpp>
#include <swarm/swarm.hpp>
#include <node/node_factory.hpp>

using namespace bzapi;

swarm_factory::swarm_factory(std::shared_ptr<bzn::asio::io_context_base> io_context
    , std::shared_ptr<bzn::beast::websocket_base> ws_factory
    , std::shared_ptr<crypto_base> crypto)
: io_context(std::move(io_context)), ws_factory(std::move(ws_factory)), crypto(std::move(crypto))
{
    node_factory = std::make_shared<::node_factory>();
}

swarm_factory::~swarm_factory()
{

}

void
swarm_factory::get_swarm(const uuid_t& uuid, std::function<void(std::shared_ptr<swarm_base>)> callback)
{
    auto it = this->uuids.find(uuid);
    if (it != this->uuids.end())
    {
        auto swarm = it->second.lock();
        if (swarm)
        {
            callback(swarm);
            return;
        }
    }

//    std::shared_ptr<swarm> the_swarm = std::make_shared<swarm>(this->node_factory, this->io_context, this->crypto
//        , this->tmp_default_endpoint);
//    this->swarms[uuid] = the_swarm;
//
//    return the_swarm;
}

void
swarm_factory::temporary_set_default_endpoint(const endpoint_t& endpoint)
{
    this->endpoints.insert(endpoint);
    this->swarms[endpoint] =
        std::make_shared<swarm>(this->node_factory, this->ws_factory, this->io_context, this->crypto, endpoint);
}

void
swarm_factory::has_db(const uuid_t& uuid, std::function<void(db_error result)> callback)
{
    if (this->uuids.find(uuid) != this->uuids.end())
    {
        callback(db_error::success);
        return;
    }

    auto count = this->swarms.size();
    for (const auto& elem : this->swarms)
    {
        auto sw = elem.second.lock();
        if (!sw)
        {
            sw = std::make_shared<swarm>(this->node_factory, this->ws_factory, this->io_context, this->crypto, elem.first);
            this->swarms[elem.first] = sw;
        }

        // TODO: can the shared_ptr be captured by reference?
//        sw->has_uuid(uuid, [uuid, callback, sw, this](auto res)
        sw->has_uuid(uuid, [&](auto res)
        {
            count--;
            if (res)
            {
                this->uuids[uuid] = sw;
                callback(db_error::success);
            }
            else
            {
                if (!count)
                {
                    callback(db_error::no_database);
                }
            }
        });
    }
}