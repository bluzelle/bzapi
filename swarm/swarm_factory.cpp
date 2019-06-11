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
#include <utils/esr_peer_info.hpp>

// Note: the intention is for a swarm to serve multiple db_impl clients (swarm sharing)
// however the current code will allocate a new swarm for each db instance, even if
// it's the same uuid.
// This will be fixed in an upcoming sprint.


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
swarm_factory::initialize(const std::string& esr_address, const std::string& url)
{
    this->esr_address = esr_address;
    this->esr_url = url;
    this->update_swarm_registry();
    this->initialized = true;
}

void
swarm_factory::initialize(const swarm_id_t& default_swarm, const std::vector<std::pair<node_id_t, bzn::peer_address_t>>& nodes)
{
    auto sw_reg = std::make_shared<swarm_registry>();
    for (auto node : nodes)
    {
        sw_reg->add_node(default_swarm, node.first, node.second);
    }

    this->swarm_reg = sw_reg;
    this->initialized = true;
}

void
swarm_factory::get_swarm(const uuid_t& db_uuid, std::function<void(std::shared_ptr<swarm_base>)> callback)
{
    auto it = this->swarm_dbs.find(db_uuid);
    if (it != this->swarm_dbs.end())
    {
        auto swarm = this->get_or_create_swarm(it->second);
        callback(swarm);
    }

    // find the swarm if we don't have it
    this->has_db(db_uuid, [&](auto sw)
    {
        callback(sw);
    });
}

void
swarm_factory::has_db(const uuid_t& uuid, std::function<void(db_error, std::shared_ptr<swarm_base>)> callback)
{
    if (!this->initialized)
    {
        // how do we propagate the error?
        callback(db_error::uninitialized, nullptr);
        return;
    }

    auto it = this->swarm_dbs.find(uuid);
    if (it != this->swarm_dbs.end())
    {
        auto sw = this->get_or_create_swarm(it->second);
        callback(sw ? db_error::success : db_error::database_error, sw);
        return;
    }

    auto swarms = this->swarm_reg->get_swarms();
    auto count = std::make_shared<size_t>(swarms.size());
    for (const auto& elem : swarms)
    {
        auto sw = this->get_or_create_swarm(elem);
        sw->has_uuid(uuid, [weak_this = weak_from_this(), uuid, sw_id = elem, count, callback, sw](auto err)
        {
            (*count)--;
            if (err == db_error::success)
            {
                auto strong_this = weak_this.lock();
                if (strong_this)
                {
                    strong_this->swarm_dbs[uuid] = sw_id;
                }
                callback(db_error::success, sw);
            }
            else
            {
                if (!(*count))
                {
                    callback(db_error::no_database, nullptr);
                }
            }
        });
    }
}

void
swarm_factory::create_db(const uuid_t& db_uuid, uint64_t max_size, bool random_evict
    , std::function<void(db_error, std::shared_ptr<swarm_base>)> callback)
{
    if (!this->initialized)
    {
        // how do we propagate the error?
        callback(db_error::uninitialized, nullptr);
        return;
    }

    // first we need to make sure this uuid doesn't already exist in a swarm
    this->has_db(db_uuid, [callback, weak_this = weak_from_this(), db_uuid, max_size, random_evict](auto sw)
    {
        if (sw)
        {
            callback(db_error::already_exists, nullptr);
            return;
        }

        auto strong_this = weak_this.lock();
        if (strong_this)
        {
            // TODO: find best swarm for the new db
            // for now we will just use the first (only) swarm in our list
            auto swarms = strong_this->swarm_reg->get_swarms();
            auto sw_id = swarms.front();
            auto sw = strong_this->get_or_create_swarm(sw_id);

            sw->create_uuid(db_uuid, max_size, random_evict
                , [callback, sw, sw_id, weak_this, db_uuid](auto err)
                {
                    if (err == db_error::success)
                    {
                        auto strong_this = weak_this.lock();
                        if (strong_this)
                        {
                            strong_this->swarm_dbs[db_uuid] = sw_id;
                        }
                        callback(err, sw);
                    }
                    else
                    {
                        callback(err, nullptr);
                    }
                });
        }
    });

}

void
swarm_factory::update_swarm_registry()
{
    auto sw_reg = std::make_shared<swarm_registry>();
    auto swarm_list = bzn::utils::esr::get_swarm_ids(esr_address, esr_url);
    for (auto sw_id : swarm_list)
    {
        auto node_list = bzn::utils::esr::get_peer_ids(sw_id, esr_address, esr_url);
        for (auto node : node_list)
        {
            auto ep = bzn::utils::esr::get_peer_info(sw_id, node, esr_address, esr_url);
            sw_reg->add_node(sw_id, node, ep);
            sw_reg->set_swarm(sw_id, this->swarm_reg->get_swarm(sw_id));
        }
    }

    // lock here?
    this->swarm_reg = sw_reg;
}

std::shared_ptr<swarm_base>
swarm_factory::get_or_create_swarm(const swarm_id_t& swarm_id)
{
    auto sw = this->swarm_reg->get_swarm(swarm_id);
    if (!sw.lock())
    {
        auto nodes = this->swarm_reg->get_nodes(swarm_id);
        assert(!nodes.empty());
        auto swm = std::make_shared<swarm>(this->node_factory, this->ws_factory, this->io_context, this->crypto
            , nodes.front().second, swarm_id, this->my_uuid);
        this->swarm_reg->set_swarm(swarm_id, sw);
        return swm;
    }

    return std::shared_ptr<swarm_base>{sw};
}

// ====== swarm_registry ==============

void
swarm_factory::swarm_registry::add_node(const swarm_id_t& swarm_id, const node_id_t& node_id, const bzn::peer_address_t& endpoint)
{
    this->swarms[swarm_id].nodes.insert(std::make_pair(node_id, endpoint));
}

std::vector<swarm_id_t>
swarm_factory::swarm_registry::get_swarms()
{
    std::vector<swarm_id_t> res;
    for (auto sw : this->swarms)
    {
        res.push_back(sw.first);
    }

    return res;
}

std::vector<std::pair<node_id_t, bzn::peer_address_t>>
swarm_factory::swarm_registry::get_nodes(swarm_id_t swarm_id)
{
    std::vector<std::pair<node_id_t, bzn::peer_address_t>> res;
    auto it = this->swarms.find(swarm_id);
    if (it != this->swarms.end())
    {
        for (auto node : it->second.nodes)
        {
            res.push_back(std::make_pair(node.first, node.second));
        }
    }

    return res;
}

std::weak_ptr<swarm_base>
swarm_factory::swarm_registry::get_swarm(const swarm_id_t& swarm_id)
{
    auto it = this->swarms.find(swarm_id);
    assert(it != this->swarms.end());
    return it->second.swarm;
}

void
swarm_factory::swarm_registry::set_swarm(const swarm_id_t& swarm_id, std::weak_ptr<swarm_base> swarm)
{
    auto it = this->swarms.find(swarm_id);
    assert(it != this->swarms.end());
    it->second.swarm = swarm;
}
