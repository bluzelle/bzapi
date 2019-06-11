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

#include <swarm.hpp>
#include <json/json.h>
#include <bluzelle.hpp>
#include <boost/format.hpp>
#include <proto/database.pb.h>
#include <database/db_impl_base.hpp>
#include <utils/peer_address.hpp>

using namespace bzapi;

namespace
{
    const std::string INITIAL_NODE{"initial_node"};
    const std::chrono::seconds STATUS_REQUEST_TIME{std::chrono::seconds(60)};
}

swarm::swarm(std::shared_ptr<node_factory_base> node_factory
    , std::shared_ptr<bzn::beast::websocket_base> ws_factory
    , std::shared_ptr<bzn::asio::io_context_base> io_context
    , std::shared_ptr<crypto_base> crypto
    , const std::vector<std::pair<node_id_t, bzn::peer_address_t>>& initial_nodes
    , const swarm_id_t& swarm_id
    , const uuid_t& uuid)
: node_factory(std::move(node_factory)), ws_factory(std::move(ws_factory)), io_context(std::move(io_context))
    , crypto(std::move(crypto)), swarm_id(swarm_id), my_uuid(uuid)
{
    if (initial_nodes.empty())
    {
        throw(std::runtime_error("Attempt to create swarm with no nodes"));
    }

    this->nodes = std::make_shared<std::unordered_map<uuid_t, node_info>>();
    for (auto node : initial_nodes)
    {
        this->add_node(node.first, node.second);
    }

    // until we have status...
    this->primary_node = initial_nodes.front().first;
    this->fastest_node = this->primary_node;
}

swarm::~swarm()
{
    node_factory = nullptr;
}

void
swarm::initialize(completion_handler_t handler)
{
    if (this->init_called)
    {
        handler(boost::system::error_code{});
        return;
    }

    this->init_called = true;
    this->init_handler = handler;

    // we handle status messages internally
    this->register_response_handler(bzn_envelope::kStatusResponse
        , [weak_this = weak_from_this()](const uuid_t& uuid, const bzn_envelope& response)
        {
            auto strong_this = weak_this.lock();
            if (strong_this)
            {
                return strong_this->handle_status_response(uuid, response);
            }

            return true;
        });

    // request status from all nodes
    for (auto info : *(this->nodes))
    {
        this->send_status_request(info.first);
    }
}

int
swarm::send_request(std::shared_ptr<bzn_envelope> request, send_policy policy)
{
    // TODO: refactor the message so we don't need to break encapsulation like this
    if (request->payload_case() == bzn_envelope::PayloadCase::kDatabaseMsg)
    {
        database_msg db_msg;
        if (db_msg.ParseFromString(request->database_msg()))
        {
            auto db_header = new database_header(*db_msg.mutable_header());
            db_header->set_point_of_contact(policy == send_policy::fastest ?
                this->fastest_node : this->primary_node);
            db_msg.set_allocated_header(db_header);
            request->set_database_msg(db_msg.SerializeAsString());
        }
    }

    request->set_sender(my_uuid);
    request->set_swarm_id(swarm_id);
    request->set_timestamp(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count()));
    if (request->signature().empty())
    {
        this->crypto->sign(*request);
    }

    switch (policy)
    {
        case send_policy::normal:
        {
            auto it = this->nodes->find(this->primary_node);
            if (it == this->nodes->end())
            {
                // just use the first node we can find
                it = this->nodes->begin();
            }

            this->send_node_request(it->second.node, request);
        }
        break;

        case send_policy::fastest:
        {
            auto it = this->nodes->find(this->fastest_node);
            if (it == this->nodes->end())
            {
                // just use the first node we can find
                it = this->nodes->begin();
            }

            this->send_node_request(it->second.node, request);
        }
        break;

        case send_policy::broadcast:
        {
            for (auto& i : *(this->nodes))
            {
                this->send_node_request(i.second.node, request);
            }
        }
    }

    return 0;
}

void
swarm::send_node_request(std::shared_ptr<node_base> node, std::shared_ptr<bzn_envelope> request)
{
    std::string msg = request->SerializeAsString();
    node->send_message(msg, [](auto ec)
    {
        if (ec)
        {
            return true;
        }

        return false;
    });
}

bool
swarm::register_response_handler(payload_t type, swarm_response_handler_t handler)
{
    return this->response_handlers.insert(std::make_pair(type, handler)).second;
}

std::string
swarm::get_status()
{
    Json::Value status;
    status["swarm_version"] = this->last_status.swarm_version();
    status["swarm_git_commit"] = this->last_status.swarm_git_commit();
    status["uptime"] = this->last_status.uptime();
    status["fastest_node"] = this->fastest_node;
    status["primary_node"] = this->primary_node;

    Json::Value node_list;
    for (const auto &i : (*this->nodes))
    {
        Json::Value info;
        info["uuid"] = i.first;
        info["host"] = i.second.host;
        info["port"] = i.second.port;
        info["latency"] = static_cast<Json::Value::UInt64>(i.second.last_status_duration.count());
        // what to do about last send/receive?

        node_list.append(info);
    }
    status["nodes"] = node_list;

    return status.toStyledString();
}

bool
swarm::handle_status_response(const uuid_t& uuid, const bzn_envelope& response)
{
    bool res = false;

    // calculate time for retrieving status from this node
    std::chrono::microseconds this_node_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - (*this->nodes)[uuid].last_status_request_sent);

    // TODO: fix this
//    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::steady_clock::now()
//    - (*this->nodes)[uuid].last_status_request_sent);
    auto fastest_time = this_node_duration;
    auto fastest_node_so_far = uuid;

    status_response status;
    if (!status.ParseFromString(response.status_response()))
    {
        LOG(error) << "Unable to parse status response";
        res = true;
    }
    else
    {
        Json::Value module_status;
        std::stringstream(status.module_status_json()) >> module_status;

        // find the pbft module status (verify this exists first?)
        Json::Value swarm_status = module_status["module"][0];

        auto new_nodes = std::make_shared<std::unordered_map<uuid_t, node_info>>();
        std::vector<std::string> new_uuids;

        Json::Value peer_index = swarm_status["status"]["peer_index"];
        for (Json::ArrayIndex i = 0; i < peer_index.size(); i++)
        {
            Json::Value node = peer_index[i];
            auto node_uuid = node["uuid"].asString();
            auto info = (*this->nodes)[node_uuid];
            if (!info.node)
            {
                uint16_t port = node["port"].asUInt();
                this->add_node(node_uuid
                    , bzn::peer_address_t{node["host"].asString(), port, 0, "", node_uuid});
                new_uuids.push_back(node_uuid);
            }

            // is this the node we received this response from?
            if (node_uuid == uuid)
            {
                // store its response time
                info.last_status_duration = this_node_duration;

                // schedule another status request for this node
                info.status_timer->expires_from_now(STATUS_REQUEST_TIME);
                info.status_timer->async_wait([weak_this = weak_from_this(), node_uuid](auto ec)
                {
                    if (!ec)
                    {
                        auto strong_this = weak_this.lock();
                        if (strong_this)
                        {
                            strong_this->send_status_request(node_uuid);
                        }
                    }
                });
            }

            if (info.last_status_duration > static_cast<std::chrono::microseconds>(0) &&
                info.last_status_duration < fastest_time)
            {
                fastest_time = info.last_status_duration;
                fastest_node_so_far = node_uuid;
            }

            (*new_nodes)[node_uuid] = info;
        }

        this->nodes = new_nodes;
        this->primary_node = swarm_status["status"]["primary"]["uuid"].asString();
        this->fastest_node = fastest_node_so_far;
        this->last_status = status;

        // kick off status requests for newly added nodes
        for (const auto& n : new_uuids)
        {
            this->send_status_request(n);
        }
    }

    // initialization is done once we've received one status response
    if (this->init_handler)
    {
        this->init_handler(boost::system::error_code{});
        this->init_handler = nullptr;
    }

    return res;
}

void
swarm::send_status_request(uuid_t node_uuid)
{
    auto elem = this->nodes->find(node_uuid);
    if (elem == this->nodes->end())
    {
        // this node doesn't exist. It may have been erased
        return;
    }

    auto& info = elem->second;
    auto node = info.node;

    status_request req;
    bzn_envelope env;
    env.set_swarm_id(swarm_id);
    env.set_sender(my_uuid);
    env.set_status_request(req.SerializeAsString());
    env.set_swarm_id(swarm_id);
    env.set_sender(my_uuid);
    this->crypto->sign(env);
    auto msg = env.SerializeAsString();
    info.last_status_request_sent = std::chrono::steady_clock::now();
    info.last_message_sent = std::chrono::steady_clock::now();
    node->send_message(msg, [](auto& ec)
    {
        if (ec)
        {
            // TODO: this needs to be caught
            LOG(error) << "Error sending status request to node: " << ec.message();
            throw(std::runtime_error("Error sending status request to node: " + ec.message()));
        }
    });
}

bool
swarm::handle_node_message(const std::string& uuid, const std::string& data)
{
    auto it = this->nodes->find(uuid);
    if (it == this->nodes->end())
    {
        LOG(debug) << "Dropping message relayed by unknown node: " << uuid;
        return true;
    }

    auto info = it->second;
    info.last_message_received = std::chrono::steady_clock::now();

    bzn_envelope env;
    if (!env.ParseFromString(data))
    {
        LOG(error) << "Dropping invalid message: " << std::string(data, MAX_MESSAGE_SIZE);
        return true;
    }

    // verify sender is on node list
    // TODO: quickread responses don't have a sender????
    if (!env.sender().empty() && this->nodes->find(env.sender()) == this->nodes->end() && uuid != INITIAL_NODE)
    {
        LOG(debug) << "Dropping message from unknown sender: " << env.sender();
        return true;
    }

    // only verify signature if it exists. upper layer will check for existance of signature where required
    if (env.signature().length() && !this->crypto->verify(env))
    {
        LOG(error) << "Dropping message with invalid signature: " << env.DebugString().substr(0, MAX_MESSAGE_SIZE);
        return true;
    }

    auto it2 = this->response_handlers.find(env.payload_case());
    if (it2 == this->response_handlers.end())
    {
        LOG(error) << "Dropping message with unregistered type: " << env.DebugString().substr(0, MAX_MESSAGE_SIZE);
        return false;
    }

    return it2->second(uuid, env);
}

size_t
swarm::honest_majority_size()
{
    return (((this->nodes->size() - 1) / 3) * 2) + 1;
}

void
swarm::setup_client_timeout(std::shared_ptr<node_base> node, std::function<void(db_error)> callback)
{
    this->timeout_timer = this->io_context->make_unique_steady_timer();
    this->timeout_timer->expires_from_now(std::chrono::milliseconds {std::chrono::seconds(get_timeout())});
    this->timeout_timer->async_wait([node, callback](const auto& ec)
    {
        if (!ec)
        {
            LOG(warning) << "Request timeout querying swarm";
            callback(db_error::timeout_error);
            node->register_message_handler([](const auto){return true;});
        }
    });
}

void
swarm::add_node(const node_id_t& node_id, const bzn::peer_address_t& addr)
{
    node_info info;
    info.node = node_factory->create_node(io_context, ws_factory, addr.host, addr.port);
    info.host = addr.host;
    info.port = addr.port;
    info.status_timer = this->io_context->make_unique_steady_timer();
    this->nodes = std::make_shared<std::unordered_map<uuid_t, node_info>>();
    (*this->nodes)[node_id] = info;

    LOG(debug) << "status: adding node: " << info.host << ":" << info.port;

    info.node->register_message_handler([weak_this = weak_from_this()](const std::string& data)
    {
        auto strong_this = weak_this.lock();
        if (strong_this)
        {
            return strong_this->handle_node_message("uuid_node", data);
        }
        else
        {
            return true;
        }
    });
}
