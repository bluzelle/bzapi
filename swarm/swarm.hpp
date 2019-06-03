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
            , std::shared_ptr<crypto_base> crypto, const endpoint_t& initial_endpoint
            , const swarm_id_t& swarm_id
            , const uuid_t& uuid);

        ~swarm();

        void has_uuid(const uuid_t& uuid, std::function<void(db_error)> callback) override;

        void create_uuid(const uuid_t& uuid, std::function<void(db_error)> callback) override;

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
        endpoint_t initial_endpoint;
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

        std::pair<std::string, uint16_t> parse_endpoint(const std::string& endpoint);

        bool handle_status_response(const uuid_t& uuid, const bzn_envelope& response);

        void send_status_request(uuid_t node_uuid);

        bool handle_node_message(const std::string& uuid, const std::string& data);

        void send_node_request(std::shared_ptr<node_base> node, std::shared_ptr<bzn_envelope> request);

        void setup_client_timeout(std::shared_ptr<node_base> node, std::function<void(db_error)> callback);

    };
}