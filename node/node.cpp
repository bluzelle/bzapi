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

#include <node/node.hpp>

using namespace bzapi;

node::node(std::shared_ptr<bzn::asio::io_context_base> io_context
    , std::shared_ptr<bzn::beast::websocket_base> ws_factory
    , const std::string& host
    , uint16_t port)
: io_context(std::move(io_context)), ws_factory(std::move(ws_factory)), endpoint(this->make_tcp_endpoint(host, port))
{
}

void
node::register_message_handler(node_message_handler msg_handler)
{
    this->handler = msg_handler;
}

void
node::send_message(const std::string& msg, completion_handler_t callback)
{
    if (!this->connected)
    {
        this->connect([weak_this = weak_from_this(), callback, msg](auto ec)
        {
            if (ec)
            {
                callback(ec);
                return;
            }

            auto strong_this = weak_this.lock();
            if (strong_this)
            {
                strong_this->send(msg, callback, true);
                return;
            }
        });
    }
    else
    {
        this->send(msg, callback, false);
    }
}

boost::asio::ip::tcp::endpoint
node::make_tcp_endpoint(const std::string& host, uint16_t port)
{
    boost::asio::ip::tcp::resolver resolver(this->io_context->get_io_context());
    boost::asio::ip::tcp::resolver::query query(host, std::to_string(port));
    auto iter = resolver.resolve(query);
    boost::asio::ip::tcp::endpoint ep;

    std::for_each(iter, {}, [&ep](auto& it)
    {
        if (it.endpoint().address().is_v4())
        {
            ep = it.endpoint();
        }
    });

    return ep;
}

void
node::connect(const completion_handler_t& callback)
{
    std::shared_ptr<bzn::asio::tcp_socket_base> socket = this->io_context->make_unique_tcp_socket();
    socket->async_connect(this->endpoint, [weak_this = weak_from_this(), callback, socket](auto ec)
    {
        // TODO: save connection latency - KEP-1382

        if (ec)
        {
            // failed to connect
            callback(ec);
            return;
        }

        auto strong_this = weak_this.lock();
        if (strong_this)
        {
            strong_this->connected = true;

            // set tcp_nodelay option
            boost::system::error_code option_ec;
            socket->get_tcp_socket().set_option(boost::asio::ip::tcp::no_delay(true), option_ec);
            if (option_ec)
            {
                LOG(warning) << "failed to set no_delay socket option: " << option_ec.message();
            }

            strong_this->websocket = strong_this->ws_factory->make_unique_websocket_stream(
                socket->get_tcp_socket());
            strong_this->websocket->async_handshake(strong_this->endpoint.address().to_string(), "/"
                , [weak_this, callback](auto ec)
                {
                    try
                    {
                        auto strong_this = weak_this.lock();
                        if (strong_this)
                        {
                            if (ec)
                            {
                                LOG(warning) << "websocket handshake failure: " << ec.message();
                                callback(ec);
                                return;
                            }

                            callback(ec);
                            strong_this->receive();
                        }
                    }
                    CATCHALL();
                });
        }
    });
}

void
node::send(const std::string& msg, const completion_handler_t& callback, bool is_retry)
{
    boost::asio::mutable_buffers_1 buffer((void*)msg.c_str(), msg.length());

    // guard against multiple threads trying to send on the same socket at the same time
    // this can happen if a status request is sent on an asio thread at the same time as an API request is issued
    auto send_lock = std::make_shared<std::unique_lock<std::mutex>>(this->send_mutex);

    this->websocket->binary(true);
    this->websocket->async_write(buffer, [weak_this = weak_from_this(), callback, is_retry, msg, send_lock](auto ec, auto /*bytes*/)
    {
        try
        {
            send_lock->unlock();
            if (ec == boost::beast::websocket::error::closed || ec == boost::asio::error::eof)
            {
                auto strong_this = weak_this.lock();
                if (strong_this)
                {
                    strong_this->connected = false;

                    // try to reconnect once
                    if (!is_retry)
                    {
                        strong_this->connect([weak_this, callback, msg](auto ec)
                        {
                            if (ec)
                            {
                                callback(ec);
                                return;
                            }

                            auto strong_this = weak_this.lock();
                            if (strong_this)
                            {
                                strong_this->send(msg, callback, true);
                                return;
                            }
                        });
                    }
                }
            }
            else
            {
                callback(ec);
            }
        }
        CATCHALL();
    });
}

void
node::receive()
{
    auto buffer = std::make_shared<boost::beast::multi_buffer>();
    this->websocket->async_read(*buffer, [weak_this = weak_from_this(), buffer, ws = this->websocket](auto ec, auto /*bytes*/)
    {
        try
        {
            auto strong_this = weak_this.lock();
            if (strong_this)
            {
                if (ec)
                {
                    strong_this->close();
                    return;
                }

                std::stringstream ss;
                ss << boost::beast::make_printable(buffer->data());
                std::string str = ss.str();

                if (strong_this->handler(str))
                {
                    strong_this->close();
                }
                else
                {
                    strong_this->receive();
                }
            }
        }
        CATCHALL();
    });
}

void
node::close()
{
    if (this->websocket && this->websocket->is_open())
    {
        this->connected = false;

        // hold onto reference to websocket to prevent boost exception if node is gone
        this->websocket->async_close(boost::beast::websocket::close_code::normal, [ws = this->websocket](auto)
        {
            // ignore close errors
        });
    }
}
