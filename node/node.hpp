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

        node_message_handler handler;
        bool connected = false;
        std::shared_ptr<bzn::beast::websocket_stream_base> websocket;
        std::mutex send_mutex;

        using queued_message = std::pair<boost::asio::mutable_buffers_1, bzn::asio::write_handler>;
        std::deque<std::shared_ptr<queued_message>> send_queue;

        boost::asio::ip::tcp::endpoint make_tcp_endpoint(const std::string& host, uint16_t port);
        void connect(const completion_handler_t& callback);
        void send(const std::string& msg, const completion_handler_t& callback, bool is_retry = false);
        void receive();
        void close();

        void queued_send(boost::asio::mutable_buffers_1 &buffer, bzn::asio::write_handler callback);
        void do_send(std::shared_ptr<queued_message> msg);
    };
}
