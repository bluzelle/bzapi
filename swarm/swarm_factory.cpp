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
    , std::shared_ptr<crypto_base> crypto
    , const uuid_t& uuid)
: io_context(std::move(io_context)), ws_factory(std::move(ws_factory)), crypto(std::move(crypto)), my_uuid(uuid)
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

    // find the swarm if we don't have it
    this->has_db(uuid, [&](auto result)
    {
        if (result != db_error::success)
        {
            callback(nullptr);
        }
        else
        {
            auto it = this->uuids.find(uuid);
            assert(it != this->uuids.end());
            auto swarm = it->second.lock();
            callback(swarm);
        }
    });

    // TODO: fix this
//    std::shared_ptr<swarm> the_swarm = std::make_shared<swarm>(this->node_factory, this->io_context, this->crypto
//        , this->tmp_default_endpoint);
//    this->swarms[uuid] = the_swarm;
//
//    return the_swarm;
}

void
swarm_factory::create_db(const uuid_t& uuid, std::function<void(std::shared_ptr<swarm_base>)> callback)
{
    // for now we will just use the first (only) swarm in our list
    // TODO: find best swarm for the new db

    auto sw_info = this->swarms.begin();
    if (sw_info == this->swarms.end())
    {
        LOG(error) << "No swarm endpoints configured";
        callback(nullptr);
        return;
    }

    auto sw = sw_info->second.lock();
    if (!sw)
    {
        sw = std::make_shared<swarm>(this->node_factory, this->ws_factory, this->io_context, this->crypto
            , sw_info->first, my_uuid);
        this->swarms[sw_info->first] = sw;
    }

    sw->create_uuid(uuid, [callback, sw, weak_this = weak_from_this(), uuid](auto res)
    {
        if (res)
        {
            auto strong_this = weak_this.lock();
            if (strong_this)
            {
                strong_this->uuids[uuid] = sw;
                callback(sw);
                return;
            }
        }
        else
        {
            // TODO: needs error code/message
            callback(nullptr);
        }
    });
}

void
swarm_factory::temporary_set_default_endpoint(const endpoint_t& endpoint)
{
    this->endpoints.insert(endpoint);
    this->swarms[endpoint] = std::make_shared<swarm>(this->node_factory, this->ws_factory, this->io_context
        , this->crypto, endpoint, my_uuid);
}

void
swarm_factory::has_db(const uuid_t& uuid, std::function<void(db_error result)> callback)
{
    if (this->uuids.find(uuid) != this->uuids.end())
    {
        callback(db_error::success);
        return;
    }

    // TODO: set a timer to retry if no response

    auto count = std::make_shared<size_t>(this->swarms.size());
    for (const auto& elem : this->swarms)
    {
        auto sw = elem.second.lock();
        if (!sw)
        {
            sw = std::make_shared<swarm>(this->node_factory, this->ws_factory, this->io_context, this->crypto
                , elem.first, my_uuid);
            this->swarms[elem.first] = sw;
        }

        sw->has_uuid(uuid, [weak_this = weak_from_this(), uuid, count, callback, sw](auto res)
        {
            (*count)--;
            if (res)
            {
                auto strong_this = weak_this.lock();
                if (strong_this)
                {
                    //strong_this->uuids[uuid] = weak_sw;
                    strong_this->uuids[uuid] = sw;
                    callback(db_error::success);
                }
            }
            else
            {
                if (!(*count))
                {
                    callback(db_error::no_database);
                }
            }
        });
    }
}