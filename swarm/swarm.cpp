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
#include <boost/lexical_cast.hpp>
#include <json/json.h>
#include <bzapi.hpp>
#include <boost/format.hpp>
#include <proto/database.pb.h>

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
    , const endpoint_t& initial_endpoint
    , const uuid_t& uuid)
: node_factory(std::move(node_factory)), ws_factory(std::move(ws_factory)), io_context(std::move(io_context))
    , crypto(std::move(crypto)), initial_endpoint(initial_endpoint), my_uuid(uuid)
{
}

swarm::~swarm()
{
    node_factory = nullptr;
}

// TODO: this needs its own unit test
void
swarm::has_uuid(const uuid_t& uuid, std::function<void(bool)> callback)
{
    auto endpoint = this->parse_endpoint(initial_endpoint);
    auto uuid_node = node_factory->create_node(io_context, ws_factory, endpoint.first, endpoint.second);
    node_info info;
    info.node = uuid_node;
    info.host = endpoint.first;
    info.port = endpoint.second;
    this->nodes = std::make_shared<std::unordered_map<uuid_t, node_info>>();
    (*this->nodes)[uuid_t{"uuid_node"}] = info;

    // TODO: should this call a static method inside db_impl? Not ideal having the swarm
    // process a database message
    uuid_node->register_message_handler([uuid, callback](const char *data, uint64_t len)
    {
        bzn_envelope env;
        database_response response;
        database_has_db_response has_db;
        if (!env.ParseFromString(std::string(data, len)) || !response.ParseFromString(env.database_response()))
        {
            LOG(error) << "Dropping invalid response to has_db: " << std::string(data, MAX_MESSAGE_SIZE);
            callback(false);
            return true;
        }

        if (response.has_db().uuid() != uuid)
        {
            LOG(error) << "Invalid uuid response to has_db: " << std::string(data, MAX_MESSAGE_SIZE);
            callback(false);
            return true;
        }

        //node->register_message_handler([](const auto, auto){return true;});
        callback(response.has_db().has());
        return true;
    });

    bzn_envelope env;
    database_msg request;
    database_header header;
    header.set_db_uuid(uuid);
    header.set_nonce(1);
    request.set_allocated_header(new database_header(header));
    request.set_allocated_has_db(new database_has_db());
    env.set_database_msg(request.SerializeAsString());
    env.set_sender(my_uuid);
    env.set_timestamp(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count()));

    this->crypto->sign(env);
    auto message = env.SerializeAsString();
    uuid_node->send_message(message, [callback, uuid](const auto& ec)
    {
        if (ec)
        {
            LOG(error) << "Error sending has_db(" << uuid << ") request: " << ec.message();
            callback(false);
        }

        return true;
    });
}

// TODO: refactor
void
swarm::create_uuid(const uuid_t& uuid, std::function<void(bool)> callback)
{
    auto endpoint = this->parse_endpoint(initial_endpoint);
    auto uuid_node = node_factory->create_node(io_context, ws_factory, endpoint.first, endpoint.second);
    node_info info;
    info.node = uuid_node;
    info.host = endpoint.first;
    info.port = endpoint.second;
    this->nodes = std::make_shared<std::unordered_map<uuid_t, node_info>>();
    (*this->nodes)[uuid_t{"uuid_node"}] = info;

    // TODO: should this call a static method inside db_impl? Not ideal having the swarm
    // process a database message
    uuid_node->register_message_handler([weak_this = weak_from_this(), uuid, callback](const char *data, uint64_t len)->bool
    {
        auto strong_this = weak_this.lock();
        if (strong_this)
        {
            bzn_envelope env;
            database_response response;
            if (!env.ParseFromString(std::string(data, len)) || !response.ParseFromString(env.database_response()))
            {
                LOG(error) << "Dropping invalid response to has_db: " << std::string(data, MAX_MESSAGE_SIZE);
                callback(false);
                return true;
            }

            callback(!(response.has_error()));
        }

        return true;
    });

    database_create_db db_msg;
    db_msg.set_eviction_policy(database_create_db::NONE);
    db_msg.set_max_size(0);

    database_header header;
    header.set_db_uuid(uuid);
    header.set_nonce(1);

    database_msg msg;
    msg.set_allocated_header(new database_header(header));
    msg.set_allocated_create_db(new database_create_db(db_msg));

    bzn_envelope env;
    env.set_database_msg(msg.SerializeAsString());
    env.set_sender(my_uuid);
    env.set_timestamp(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count()));
    this->crypto->sign(env);

    auto message = env.SerializeAsString();
    uuid_node->send_message(message, [callback, uuid](const auto& ec)
    {
        if (ec)
        {
            LOG(error) << "Error sending has_db(" << uuid << ") request: " << ec.message();
            callback(false);
        }

        return true;
    });
}

// this should be void and send the result later when we have status
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
    this->nodes = std::make_shared<std::unordered_map<uuid_t, node_info>>();

    // we handle status messaages internally. don't hold reference to self...
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

    auto endpoint = this->parse_endpoint(initial_endpoint);
    auto initial_node = node_factory->create_node(this->io_context, this->ws_factory, endpoint.first, endpoint.second);
    initial_node->register_message_handler([weak_this = weak_from_this()](const char *data, uint64_t len)->bool
    {
        auto strong_this = weak_this.lock();
        if (strong_this)
        {
            return strong_this->handle_node_message(INITIAL_NODE, data, len);
        }
        else
        {
            return true;
        }
    });

    node_info info{initial_node, endpoint.first, endpoint.second};
    info.status_timer = this->io_context->make_unique_steady_timer();
    (*this->nodes)[INITIAL_NODE] = info;
    this->primary_node = INITIAL_NODE;
    this->fastest_node = INITIAL_NODE;

    this->send_status_request(INITIAL_NODE);
}

int
swarm::send_request(std::shared_ptr<bzn_envelope> request, send_policy policy)
{
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
        info["latency"] = i.second.last_status_duration.count();
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
        Json::Value swarm_status = module_status["pbft"];

        auto new_nodes = std::make_shared<std::unordered_map<uuid_t, node_info>>();
        std::vector<std::string> new_uuids;

        Json::Value peer_index = swarm_status["peer_index"];
        for (Json::ArrayIndex i = 0; i < peer_index.size(); i++)
        {
            Json::Value node = peer_index[i];
            auto node_uuid = node["uuid"].asString();
            auto info = (*this->nodes)[node_uuid];
            if (!info.node)
            {
                // check for initial_node
                if (uuid == INITIAL_NODE && node["host"].asString() == (*this->nodes)[INITIAL_NODE].host
                    && node["port"].asUInt() == (*this->nodes)[INITIAL_NODE].port)
                {
                    info = (*this->nodes)[INITIAL_NODE];

                    // re-register with it's real uuid
                    assert(info.node);

                    info.node->register_message_handler([weak_this = weak_from_this(), node_uuid](const char *data, uint64_t len)->bool
                    {
                        auto strong_this = weak_this.lock();
                        if (strong_this)
                        {
                            return strong_this->handle_node_message(node_uuid, data, len);
                        }
                        else
                        {
                            return true;
                        }
                    });
                }
                else
                {
                    // refactor
                    info.host = node["host"].asString();
                    info.port = node["port"].asUInt();
                    info.node = this->node_factory->create_node(this->io_context, this->ws_factory, info.host,
                        info.port);

                    info.node->register_message_handler([weak_this = weak_from_this(), node_uuid](const char *data, uint64_t len)->bool
                    {
                        auto strong_this = weak_this.lock();
                        if (strong_this)
                        {
                            return strong_this->handle_node_message(node_uuid, data, len);
                        }
                        else
                        {
                            return true;
                        }
                    });

                    info.status_timer = this->io_context->make_unique_steady_timer();
                    new_uuids.push_back(node_uuid);
                }
            }

            // is this the node we received this response from?
            if (node_uuid == uuid || (uuid == INITIAL_NODE && info.host == (*this->nodes)[INITIAL_NODE].host
                                      && info.port == (*this->nodes)[INITIAL_NODE].port))
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
        this->primary_node = swarm_status["primary"]["uuid"].asString();
        this->fastest_node = fastest_node_so_far;
        this->last_status = status;

        // kick off status requests for newly added nodes
        for (auto n : new_uuids)
        {
            this->send_status_request(n);
        }
    }

    if (this->init_handler)
    {
        this->init_handler(boost::system::error_code{});
        this->init_handler = nullptr;
    }

    return res;
}

std::pair<std::string, uint16_t>
swarm::parse_endpoint(const std::string& endpoint)
{
    // format should be ws://n.n.n.n:p
    // TODO: support for hostnames
    std::string addr;
    uint64_t port;

    auto offset = endpoint.find(':', 5);
    if (offset > endpoint.size() || endpoint.substr(0, 5) != "ws://")
    {
        throw(std::runtime_error("bad node endpoint: " + endpoint));
    }

    try
    {
        addr = endpoint.substr(5, offset - 5);
        port = boost::lexical_cast<uint16_t>(endpoint.substr(offset + 1).c_str());
    }
    catch (boost::bad_lexical_cast &)
    {
        throw(std::runtime_error("bad node endpoint: " + endpoint));
    }

    return std::make_pair(addr, port);
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
    env.set_status_request(req.SerializeAsString());
    auto msg = env.SerializeAsString();
    info.last_status_request_sent = std::chrono::steady_clock::now();
    info.last_message_sent = std::chrono::steady_clock::now();
    node->send_message(msg, [](auto& ec)
    {
        if (ec)
        {
            // TODO: this needs to be caught
            throw(std::runtime_error("Error sending status request to node: " + ec.message()));
        }
    });
}

bool
swarm::handle_node_message(const std::string& uuid, const char *data, uint64_t len)
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
    if (!env.ParseFromString(std::string(data, len)))
    {
        LOG(error) << "Dropping invalid message: " << std::string(data, MAX_MESSAGE_SIZE);
        return true;
    }

    // verify sender is on node list
    if (this->nodes->find(env.sender()) == this->nodes->end() && uuid != INITIAL_NODE)
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
