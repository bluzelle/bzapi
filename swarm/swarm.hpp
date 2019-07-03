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

#include <swarm/swarm_base.hpp>
#include <crypto/crypto_base.hpp>
#include <node/node_base.hpp>

namespace bzapi
{
    // maintains knowledge of swarm state
    // picks node(s) to send messages to based on policy and swarm state
    // signs and serializes outgoing requests
    // deserializes, validates (signature and sender) incoming messages
    // dispatches incoming messages based on payload type
    class swarm : public swarm_base, public std::enable_shared_from_this<swarm>
    {
    public:
        swarm(std::shared_ptr<node_factory_base> node_factory
            , std::shared_ptr<bzn::beast::websocket_base> ws_factory
            , std::shared_ptr<bzn::asio::io_context_base> io_context
            , std::shared_ptr<crypto_base> crypto
            , const swarm_id_t& swarm_id
            , const uuid_t& uuid);

        ~swarm();

        // TODO: figure out how to make this better
        void add_nodes(const std::vector<std::pair<node_id_t, bzn::peer_address_t>>& nodes) override;

        void initialize(completion_handler_t handler) override;

        int send_request(std::shared_ptr<bzn_envelope> request, send_policy policy) override;

        bool register_response_handler(payload_t type, swarm_response_handler_t handler) override;

        std::string get_status() override;

        size_t honest_majority_size() override;

    private:
        struct node_info
        {
            std::shared_ptr<node_base> node;
            std::string host = {};
            uint16_t port = 0;
            std::chrono::steady_clock::time_point last_status_request_sent = {};
            std::chrono::microseconds last_status_duration = {};
            std::chrono::steady_clock::time_point last_message_sent = {};
            std::chrono::steady_clock::time_point last_message_received = {};
            std::shared_ptr<bzn::asio::steady_timer_base> status_timer = {};
        };

        std::shared_ptr<node_factory_base> node_factory;
        std::shared_ptr<bzn::beast::websocket_base> ws_factory;
        std::shared_ptr<bzn::asio::io_context_base> io_context;
        std::shared_ptr<crypto_base> crypto;
        swarm_id_t swarm_id;
        uuid_t my_uuid;
        std::shared_ptr<std::unordered_map<uuid_t, node_info>> nodes;
        std::unordered_map<payload_t, swarm_response_handler_t> response_handlers;

        bool init_called = false;
        uuid_t fastest_node;
        uuid_t primary_node;
        status_response last_status;
        completion_handler_t init_handler = nullptr;
        std::string private_key;
        std::shared_ptr<bzn::asio::steady_timer_base> timeout_timer;

        bool handle_status_response(const uuid_t& uuid, const bzn_envelope& response);

        void send_status_request(uuid_t node_uuid);

        bool handle_node_message(const std::string& uuid, const std::string& data);

        void send_node_request(std::shared_ptr<node_base> node, std::shared_ptr<bzn_envelope> request);

        void setup_client_timeout(std::shared_ptr<node_base> node, std::function<void(db_error)> callback);

        node_info add_node(const node_id_t& node_id, const bzn::peer_address_t& addr);
    };
}
