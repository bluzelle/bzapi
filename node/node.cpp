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

#include <node.hpp>

using namespace bzapi;

node::node(std::shared_ptr<bzn::asio::io_context_base> io_context
    , std::shared_ptr<bzn::beast::websocket_base> ws_factory
    , const std::string& host
    , uint16_t port)
: io_context(std::move(io_context)), ws_factory(std::move(ws_factory))
{
    this->endpoint = this->make_tcp_endpoint(host, port);
}

node::~node()
{
}

void
node::register_message_handler(node_message_handler msg_handler)
{
    this->handler = msg_handler;
}

void
node::send_message(const char *msg, size_t len, completion_handler_t callback)
{
    boost::asio::mutable_buffers_1 write_buffer((void*)msg, len);

    if (!this->connected)
    {
        this->connect([&](auto ec)
        {
            if (ec)
            {
                callback(ec);
                return;
            }

            this->send(write_buffer, callback, true);
            return;
        });
    }
    else
    {
        this->send(write_buffer, callback, false);
    }
}

boost::asio::ip::tcp::endpoint
node::make_tcp_endpoint(const std::string& host, uint16_t port)
{
    return boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(host), port);
}

void
node::connect(completion_handler_t callback)
{
    std::shared_ptr<bzn::asio::tcp_socket_base> socket = this->io_context->make_unique_tcp_socket();
    socket->async_connect(this->endpoint, [&](auto ec)
    {
        // TODO: save connection latency...

        if (ec)
        {
            // failed to connect
            callback(ec);
            return;
        }

        this->connected = true;

        // set tcp_nodelay option
        boost::system::error_code option_ec;
        socket->get_tcp_socket().set_option(boost::asio::ip::tcp::no_delay(true), option_ec);
        if (option_ec)
        {
            //LOG(error) << "failed to set socket option: " << option_ec.message();
        }

        this->websocket = ws_factory->make_unique_websocket_stream(socket->get_tcp_socket());
        this->websocket->async_handshake(this->endpoint.address().to_string(), "/", [&](auto ec)
        {
            if (ec)
            {
                // connect failed
                callback(ec);
                return;
            }

            callback(ec);
            this->receive();
        });
    });
}

void
node::send(boost::asio::mutable_buffers_1 buffer, completion_handler_t callback, bool is_retry)
{
    // need to wrap this in a strand...

    this->websocket->async_write(buffer, [&](auto ec, auto /*bytes*/)
    {
        if (ec == boost::beast::websocket::error::closed || ec == boost::asio::error::eof)
        {
            this->connected = false;

            // try to reconnect once
            if (!is_retry)
            {
                this->connect([&](auto ec)
                {
                    if (ec)
                    {
                        callback(ec);
                        return;
                    }

                    this->send(buffer, callback, true);
                    return;
                });
            }
        }
        else
        {
            callback(ec);
        }
    });
}

void
node::receive()
{
    auto buffer = std::make_shared<boost::beast::multi_buffer>();
    this->websocket->async_read(*buffer, [this, buffer](auto ec, auto /*bytes_transferred*/)
    {
        if (ec)
        {
            this->close();
            return;
        }

        std::stringstream ss;
        ss << boost::beast::buffers(buffer->data());
        std::string str = ss.str();

        if (this->handler(str.c_str(), str.length()))
        {
            this->close();
        }
        else
        {
            this->receive();
        }
    });
}

void
node::close()
{
    if (this->websocket && this->websocket->is_open())
    {
        this->connected = false;

        // ignoring close errors for now
        this->websocket->async_close(boost::beast::websocket::close_code::normal, [](auto) {});
    }
}