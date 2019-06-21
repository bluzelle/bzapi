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
#include <deque>

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

        void register_message_handler(node_message_handler msg_handler) override;
        void send_message(const std::string& msg, completion_handler_t callback) override;

    private:
        const std::shared_ptr<bzn::asio::io_context_base> io_context;
        const std::shared_ptr<bzn::beast::websocket_base> ws_factory;
        const boost::asio::ip::tcp::endpoint endpoint;
        std::shared_ptr<bzn::asio::strand_base> strand;

        node_message_handler handler;
        bool connected = false;
        std::shared_ptr<bzn::beast::websocket_stream_base> websocket;
        std::mutex send_mutex;

        using queued_message = std::pair<std::string, bzn::asio::write_handler>;
        std::deque<std::shared_ptr<queued_message>> send_queue;

        boost::asio::ip::tcp::endpoint make_tcp_endpoint(const std::string& host, uint16_t port);
        void connect(const completion_handler_t& callback);
        void send(const std::string& msg, const completion_handler_t& callback, bool is_retry = false);
        void receive();
        void close();

        void queued_send(const std::string& msg, bzn::asio::write_handler callback);
        void do_send();
    };
}
