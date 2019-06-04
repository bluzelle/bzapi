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
    boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
    boost::asio::ip::tcp::endpoint endpoint;

    std::for_each(iter, {}, [&endpoint](auto& it)
    {
        if (it.endpoint().address().is_v4())
        {
            endpoint = it.endpoint();
        }
    });

    return endpoint;
}

void
node::connect(completion_handler_t callback)
{
    std::shared_ptr<bzn::asio::tcp_socket_base> socket = this->io_context->make_unique_tcp_socket();
    socket->async_connect(this->endpoint, [weak_this = weak_from_this(), callback, socket](auto ec)
    {
        try
        {
            // TODO: save connection latency...

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
                    LOG(error) << "failed to set socket option: " << option_ec.message();
                }

                strong_this->websocket = strong_this->ws_factory->make_unique_websocket_stream(
                    socket->get_tcp_socket());
                strong_this->websocket->async_handshake(strong_this->endpoint.address().to_string(), "/"
                    , [weak_this2 = std::weak_ptr<node>(strong_this), callback](auto ec)
                    {
                        try
                        {
                            auto strong_this2 = weak_this2.lock();
                            if (strong_this2)
                            {
                                if (ec)
                                {
                                    // connect failed
                                    callback(ec);
                                    return;
                                }

                                callback(ec);
                                //                    auto strong_this2 = weak_this2.lock();
                                if (strong_this2)
                                {
                                    strong_this2->receive();
                                }
                            }
                        }
                        catch(...){}
                    });
            }
        }
        CATCHALL();
    });
}

void
node::send(const std::string& msg, completion_handler_t callback, bool is_retry)
{
    boost::asio::mutable_buffers_1 buffer((void*)msg.c_str(), msg.length());

    this->websocket->binary(true);
    this->websocket->async_write(buffer, [weak_this = weak_from_this(), callback, is_retry, msg](auto ec, auto bytes)
    {
        try
        {
            if (ec == boost::beast::websocket::error::closed || ec == boost::asio::error::eof)
            {
                auto strong_this = weak_this.lock();
                if (strong_this)
                {
                    strong_this->connected = false;

                    // try to reconnect once
                    if (!is_retry)
                    {
                        strong_this->connect([weak_this2 = std::weak_ptr<node>(strong_this), callback, msg](auto ec)
                        {
                            if (ec)
                            {
                                callback(ec);
                                return;
                            }

                            auto strong_this2 = weak_this2.lock();
                            if (strong_this2)
                            {
                                strong_this2->send(msg, callback, true);
                                return;
                            }
                        });
                    }
                }
            }
            else
            {
                std::cout << bytes << " bytes written" << std::endl;
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
    this->websocket->async_read(*buffer, [weak_this = weak_from_this(), buffer, ws = this->websocket](auto ec, auto /*bytes_transferred*/)
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
                ss << boost::beast::buffers(buffer->data());
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

        // ignoring close errors for now
        this->websocket->async_close(boost::beast::websocket::close_code::normal, [ws = this->websocket](auto)
        {

        });
    }
}