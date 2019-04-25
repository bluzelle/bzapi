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

#include <cstdint>
#include <boost_asio_beast.hpp>
#include <node/node_base.hpp>

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
        void send(boost::asio::mutable_buffers_1 buffer, completion_handler_t callback, bool is_retry = false);
        void send(const std::string& msg, completion_handler_t callback, bool is_retry = false);
        void receive();
        void close();
    };
}
