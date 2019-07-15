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

#include <swarm/swarm_factory.hpp>
#include <swarm/swarm.hpp>
#include <swarm/esr.hpp>
#include <node/node_factory.hpp>
#include <utils/esr_peer_info.hpp>
#include <random>

namespace bzapi
{
    extern std::shared_ptr<db_dispatch_base> get_db_dispatcher();
}

namespace
{
    uint64_t MAX_RETRY{5};
}

using namespace bzapi;

swarm_factory::swarm_factory(std::shared_ptr<bzn::asio::io_context_base> io_context
    , std::shared_ptr<bzn::beast::websocket_base> ws_factory
    , std::shared_ptr<crypto_base> crypto
    , std::shared_ptr<esr_base> esr
    , const uuid_t& uuid)
: io_context(std::move(io_context)), ws_factory(std::move(ws_factory)), crypto(std::move(crypto)), esr(std::move(esr))
    , my_uuid(uuid), node_factory(std::make_shared<::node_factory>())
{
}

void
swarm_factory::initialize(const std::string& esr_addr, const std::string& url)
{
    this->esr_address = esr_addr;
    this->esr_url = url;
    this->swarm_reg = std::make_shared<swarm_registry>();
    this->update_swarm_registry();
    this->initialized = true;
}

void
swarm_factory::initialize(const swarm_id_t& default_swarm, const std::vector<std::pair<node_id_t, bzn::peer_address_t>>& nodes)
{
    auto sw_reg = std::make_shared<swarm_registry>();
    for (const auto& node : nodes)
    {
        sw_reg->add_node(default_swarm, node.first, node.second);
    }

    this->swarm_reg = sw_reg;
    this->initialized = true;
}

void
swarm_factory::has_db(const uuid_t& uuid, std::function<void(db_error, std::shared_ptr<swarm_base>)> callback)
{
    assert(this->initialized);

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
        get_db_dispatcher()->has_uuid(sw, uuid
            , [weak_this = weak_from_this(), uuid, sw_id = elem, count, callback, sw](auto err)
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
    assert (this->initialized);

    this->has_db(db_uuid, [callback, weak_this = weak_from_this(), db_uuid, max_size, random_evict](auto /*err*/, auto sw)
    {
        if (sw)
        {
            callback(db_error::already_exists, nullptr);
            return;
        }

        auto strong_this = weak_this.lock();
        if (strong_this)
        {
            strong_this->do_create_db(db_uuid, max_size, random_evict
                , std::min(MAX_RETRY, static_cast<uint64_t>(strong_this->swarm_reg->get_swarms().size() - 1)), callback);
        }
    });
}

void
swarm_factory::do_create_db(const uuid_t& db_uuid, uint64_t max_size, bool random_evict, uint64_t retry
    , std::function<void(db_error, std::shared_ptr<swarm_base>)> callback)
{
    this->select_swarm_for_size(max_size, retry, [weak_this = weak_from_this(), db_uuid, max_size
        , random_evict, retry, callback](auto sw_id)
    {
        if (sw_id.empty())
        {
            callback(db_error::no_space, nullptr);
            return;
        }

        auto strong_this = weak_this.lock();
        if (strong_this)
        {
            auto sw = strong_this->get_or_create_swarm(sw_id);
            get_db_dispatcher()->create_uuid(sw, db_uuid, max_size, random_evict
                , [callback, sw, sw_id, weak_this, db_uuid, max_size, random_evict, retry](auto err)
                {
                    if (err == db_error::success)
                    {
                        auto strong_this = weak_this.lock();
                        if (strong_this)
                        {
                            try
                            {
                                strong_this->swarm_dbs[db_uuid] = sw_id;
                            }
                            CATCHALL();
                        }
                        callback(err, sw);
                    }
                    else
                    {
                        if (retry > 0)
                        {
                            auto strong_this = weak_this.lock();
                            if (strong_this)
                            {
                                strong_this->do_create_db(db_uuid, max_size, random_evict, retry - 1, callback);
                            }
                        }
                        else
                        {
                            callback(err, nullptr);
                        }
                    }
                });
        }
    });
}

void
swarm_factory::update_swarm_registry()
{
    if (!this->esr_address.empty() && !this->esr_url.empty())
    {
        auto sw_reg = std::make_shared<swarm_registry>();
        auto swarm_list = this->esr->get_swarm_ids(esr_address, esr_url);
        for (const auto& sw_id : swarm_list)
        {
            auto node_list = this->esr->get_peer_ids(sw_id, esr_address, esr_url);
            for (const auto& node : node_list)
            {
                auto ep = this->esr->get_peer_info(sw_id, node, esr_address, esr_url);
                sw_reg->add_node(sw_id, node, ep);
                auto strong_sw = this->swarm_reg->get_swarm(sw_id).lock();
                if (strong_sw)
                {
                    sw_reg->set_swarm(sw_id, strong_sw);
                }
            }
        }

        this->swarm_reg = sw_reg;
    }
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
            , swarm_id, this->my_uuid, nodes);
        this->swarm_reg->set_swarm(swarm_id, swm);
        return swm;
    }

    return std::shared_ptr<swarm_base>{sw};
}

void
swarm_factory::swarm_registry::add_node(const swarm_id_t& swarm_id, const node_id_t& node_id, const bzn::peer_address_t& endpoint)
{
    auto info = this->swarms[swarm_id];
    info.nodes.emplace(std::make_pair(node_id, endpoint));
    this->swarms[swarm_id] = std::move(info);
}

std::vector<swarm_id_t>
swarm_factory::swarm_registry::get_swarms()
{
    std::vector<swarm_id_t> res;
    for (const auto& sw : this->swarms)
    {
        res.push_back(sw.first);
    }

    return res;
}

std::vector<std::pair<node_id_t, bzn::peer_address_t>>
swarm_factory::swarm_registry::get_nodes(const swarm_id_t& swarm_id)
{
    std::vector<std::pair<node_id_t, bzn::peer_address_t>> res;
    auto it = this->swarms.find(swarm_id);
    if (it != this->swarms.end())
    {
        for (const auto& node : it->second.nodes)
        {
            res.emplace_back(std::make_pair(node.first, node.second));
        }
    }

    return res;
}

std::weak_ptr<swarm_base>
swarm_factory::swarm_registry::get_swarm(const swarm_id_t& swarm_id)
{
    auto it = this->swarms.find(swarm_id);
    return it != this->swarms.end() ? it->second.swarm : std::weak_ptr<swarm_base>();
}

void
swarm_factory::swarm_registry::set_swarm(const swarm_id_t& swarm_id, std::shared_ptr<swarm_base> swarm)
{
    auto it = this->swarms.find(swarm_id);
    assert(it != this->swarms.end());
    it->second.swarm = swarm;
}

void
swarm_factory::select_swarm_for_size(uint64_t /*size*/, uint64_t hint
    , std::function<void(const std::string& swarm_id)> callback)
{
    // for now, we will pick the next swarm from our list. Later we may check each swarm's status and look
    // for the one with the most uncommitted space, which would be an async operation (hence the callback)
    auto sw_list = this->swarm_reg->get_swarms();
    assert(hint < sw_list.size());
    callback(sw_list[hint]);
}
