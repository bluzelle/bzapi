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
            , std::shared_ptr<crypto_base> crypto
            , swarm_id_t swarm_id
            , uuid_t uuid
            , const std::vector<std::pair<node_id_t, bzn::peer_address_t>>& node_list);

        void initialize(completion_handler_t handler) override;

        bool register_response_handler(payload_t type, swarm_response_handler_t handler) override;

        void sign_and_date_request(bzn_envelope& request, send_policy policy) override;

        int send_request(const bzn_envelope& request, send_policy policy) override;

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
            std::chrono::system_clock::time_point last_message_sent = {};
            std::chrono::system_clock::time_point last_message_received = {};
            std::shared_ptr<bzn::asio::steady_timer_base> status_timer = {};
        };
        using node_map = std::unordered_map<uuid_t, node_info>;

        const std::shared_ptr<node_factory_base> node_factory;
        const std::shared_ptr<bzn::beast::websocket_base> ws_factory;
        const std::shared_ptr<bzn::asio::io_context_base> io_context;
        const std::shared_ptr<crypto_base> crypto;
        const swarm_id_t swarm_id;
        const uuid_t my_uuid;

        bool init_called = false;
        completion_handler_t init_handler = nullptr;
        std::unordered_map<payload_t, swarm_response_handler_t> response_handlers;

        std::shared_ptr<node_map> nodes;
        uuid_t fastest_node;
        uuid_t primary_node;
        status_response last_status;
        std::mutex info_mutex;

        void add_nodes(const std::vector<std::pair<node_id_t, bzn::peer_address_t>>& node_list);
        void send_status_request(const uuid_t& node_uuid);
        void send_node_request(const std::shared_ptr<node_base>& node, const bzn_envelope& request);
        bool handle_status_response(const uuid_t& uuid, const bzn_envelope& response);
        bool handle_node_message(const std::string& uuid, const std::string& data);
        node_info add_node(const node_id_t& node_id, const bzn::peer_address_t& addr);
        std::shared_ptr<node_map> get_nodes();
    };
}