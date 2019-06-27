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

#include <include/boost_asio_beast.hpp>
#include <node/node_base.hpp>
#include <cstdint>

using namespace bzn::asio;

namespace bzapi
{
    // establishes and maintains connection with node
    // sends messages to node
    // receives incoming messages and forwards them to owner
    class node : public node_base, public std::enable_shared_from_this<node>
    {
    public:
        node(std::shared_ptr<bzn::asio::io_context_base> io_context
            , std::shared_ptr<bzn::beast::websocket_base> ws_factory
            , const std::string& host, uint16_t port);
        ~node();

        void register_message_handler(node_message_handler msg_handler) override;
        void send_message(const std::string& msg, completion_handler_t callback) override;

    private:
        std::shared_ptr<bzn::asio::io_context_base> io_context;
        std::shared_ptr<bzn::beast::websocket_base> ws_factory;
        node_message_handler handler;
        boost::asio::ip::tcp::endpoint endpoint;
        bool connected = false;
        std::shared_ptr<bzn::beast::websocket_stream_base> websocket;

        boost::asio::ip::tcp::endpoint make_tcp_endpoint(const std::string& host, uint16_t port);
        void connect(completion_handler_t callback);
        void send(const std::string& msg, completion_handler_t callback, bool is_retry = false);
        void receive();
        void close();
    };
}
