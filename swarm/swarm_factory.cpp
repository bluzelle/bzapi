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
swarm_factory::get_swarm(const uuid_t& /*uuid*/, std::function<void(std::shared_ptr<swarm_base>)> callback)
{
#if 1
    if (initialized)
    {

    }
    else
    {
        auto sw = get_default_swarm();
        callback(sw);
    }

    return;

#else // TODO: fix swarm sharing
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
#endif
}

void
swarm_factory::create_db(const uuid_t& uuid, uint64_t max_size, bool random_evict
    , std::function<void(db_error, std::shared_ptr<swarm_base>)> callback)
{
#if 1
    auto sw = get_default_swarm();
    sw->create_uuid(uuid, max_size, random_evict, [callback, sw, weak_this = weak_from_this(), uuid](auto res)
    {
        if (res == db_error::success)
        {
            callback(res, sw);
        }
        else
        {
            // TODO: needs error code/message
            callback(res, nullptr);
        }
    });


#else // TODO: fix swarm sharing
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
                //strong_this->uuids[uuid] = sw;
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
#endif
}

void
swarm_factory::temporary_set_default_endpoint(const endpoint_t& endpoint, const swarm_id_t& swarm_id)
{
    this->endpoints.insert(std::make_pair(endpoint, swarm_id));
}

std::shared_ptr<swarm_base>
swarm_factory::get_default_swarm()
{
#if 1

    auto ep = this->endpoints.begin();
    if (ep == this->endpoints.end())
    {
        LOG(error) << "No swarm endpoints configured";
        return nullptr;
    }

    auto sw = std::make_shared<swarm>(this->node_factory, this->ws_factory, this->io_context, this->crypto
        , ep->first, ep->second, my_uuid);
    return sw;


#else // TODO: fix swarm sharing
    auto sw_info = this->swarms.begin();
    if (sw_info == this->swarms.end())
    {
        LOG(error) << "No swarm endpoints configured";
        return nullptr;
    }

    auto sw = std::make_shared<swarm>(this->node_factory, this->ws_factory, this->io_context, this->crypto
        , sw_info->first, my_uuid);
    return sw;

    auto sw = sw_info->second.lock();
    if (!sw)
    {
        sw = std::make_shared<swarm>(this->node_factory, this->ws_factory, this->io_context, this->crypto
            , sw_info->first, my_uuid);
        this->swarms[sw_info->first] = sw;
    }

    return sw;
#endif
}

void
swarm_factory::has_db(const uuid_t& uuid, std::function<void(db_error result)> callback)
{
#if 1

    auto sw = this->get_default_swarm();
    sw->has_uuid(uuid, [weak_this = weak_from_this(), uuid, callback, sw](auto res)
    {
        callback(res);
    });


#else // TODO: fix swarm sharing
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
//            this->swarms[elem.first] = sw;
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
#endif
}

void
swarm_factory::update_swarm_registry()
{
    auto sw_reg = std::make_shared<swarm_registry>();
    auto swarm_list = bzn::utils::esr::get_swarm_ids(esr_address, esr_url);
    for (auto sw : swarm_list)
    {
        auto node_list = bzn::utils::esr::get_peer_ids(sw, esr_address, esr_url);
        for (auto node : node_list)
        {
            auto ep = bzn::utils::esr::get_peer_info(sw, node, esr_address, esr_url);
            sw_reg->add_node(sw, node, ep);
        }
    }

    // lock here?
    this->swarm_reg = sw_reg;
}

void
swarm_factory::swarm_registry::add_node(const swarm_id_t& swarm_id, const node_id_t& node_id, const bzn::peer_address_t& endpoint)
{
    this->swarms[swarm_id].insert(std::make_pair(node_id, endpoint));
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

std::vector<std::pair<swarm_factory::swarm_registry::node_id_t, bzn::peer_address_t>>
swarm_factory::swarm_registry::get_nodes(swarm_id_t swarm_id)
{
    std::vector<std::pair<node_id_t, bzn::peer_address_t>> res;
    auto it = this->swarms.find(swarm_id);
    if (it != this->swarms.end())
    {
        for (auto node : it->second)
        {
            res.push_back(std::make_pair(node.first, node.second));
        }
    }

    return res;
}

