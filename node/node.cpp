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

#include <node/node.hpp>

using namespace bzapi;

namespace
{
    const uint64_t MAX_BACKOFF_TIME{1024};

    // TODO: Once we decide to use ssl between the client and the swarm then this will
    // be included in the ESR data along with peer validation on/off.
    const std::string WSS_ENABLED_ENV = "WSS_ENABLED";
}

node::node(std::shared_ptr<bzn::asio::io_context_base> io_context
    , std::shared_ptr<bzn::beast::websocket_base> ws_factory
    , const std::string& host
    , uint16_t port)
: io_context(std::move(io_context)), ws_factory(std::move(ws_factory)), endpoint(this->make_tcp_endpoint(host, port))
    , strand(this->io_context->make_unique_strand()), backoff_timer(this->io_context->make_unique_steady_timer())
{
    this->initialize_ssl_context();
}

void
node::initialize_ssl_context()
{
    // if defined and not zero then we'll use ssl...
    if (auto value = getenv(WSS_ENABLED_ENV.c_str()))
    {
        if (std::string(value) != "0")
        {
            LOG(info) << "WSS Enabled";

            this->client_ctx = std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
        }
    }
}

void
node::register_message_handler(node_message_handler msg_handler)
{
    this->handler = msg_handler;
}

void
node::back_off(bool value)
{
    if (value)
    {
        this->backoff_time = std::min(this->backoff_time ? 2 * this->backoff_time : 1, MAX_BACKOFF_TIME);
    }
    else
    {
        this->backoff_time = this->backoff_time > 1 ? this->backoff_time / 2 : 0;
    }
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

            if (auto strong_this = weak_this.lock())
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
    using namespace boost::asio;

    ip::tcp::resolver resolver(this->io_context->get_io_context());
    ip::tcp::resolver::query query(ip::tcp::v4(), host, std::to_string(port));
    auto iter = resolver.resolve(query);

    if (!iter.empty())
    {
        return *iter;
    }

    return {};
}

void
node::connect(const completion_handler_t& callback)
{
    this->strand->post([callback, weak_this = weak_from_this()]()
    {
        if (auto strong_this = weak_this.lock())
        {
            std::shared_ptr<bzn::asio::tcp_socket_base> socket = strong_this->io_context->make_unique_tcp_socket();
            socket->async_connect(strong_this->endpoint, strong_this->strand->wrap([weak_this, callback, socket](auto ec)
            {
                // TODO: save connection latency - KEP-1382

                if (ec)
                {
                    // failed to connect
                    callback(ec);
                    return;
                }

                if (auto strong_this = weak_this.lock())
                {
                    strong_this->connected = true;

                    // set tcp_nodelay option
                    boost::system::error_code option_ec;
                    socket->get_tcp_socket().set_option(boost::asio::ip::tcp::no_delay(true), option_ec);
                    if (option_ec)
                    {
                        LOG(warning) << "failed to set no_delay socket option: " << option_ec.message();
                    }

                    // use ssl?
                    if (strong_this->client_ctx)
                    {
                        strong_this->websocket = strong_this->ws_factory->make_websocket_secure_stream(
                            socket->get_tcp_socket(), *strong_this->client_ctx);
                    }
                    else
                    {
                        strong_this->websocket = strong_this->ws_factory->make_websocket_stream(
                            socket->get_tcp_socket());
                    }

                    strong_this->websocket->async_handshake(strong_this->endpoint.address().to_string(), "/"
                        , strong_this->strand->wrap([weak_this, callback](auto ec)
                        {
                            try
                            {
                                if (auto strong_this = weak_this.lock())
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
                        }));
                }
            }));
        }
    });
}

void
node::queued_send(const std::string& msg, bzn::asio::write_handler callback)
{
    this->strand->post([msg, callback, weak_this = weak_from_this()]()
    {
        if (auto strong_this = weak_this.lock())
        {
            strong_this->send_queue.push_back(std::make_shared<queued_message>(std::make_pair(msg, callback)));
            if (strong_this->send_queue.size() == 1)
            {
                strong_this->schedule_send();
            }
        }
    });
}

void
node::do_send()
{
    auto msg = this->send_queue.front();
    boost::asio::mutable_buffers_1 buffer((void *) msg->first.c_str(), msg->first.length());
    this->websocket->binary(true);
    this->websocket->async_write(buffer, this->strand->wrap(
        [weak_this = weak_from_this(), callback = msg->second]
            (const boost::system::error_code& ec, unsigned long bytes)
        {
            if (auto strong_this = weak_this.lock())
            {
                strong_this->send_queue.pop_front();
                if (!strong_this->send_queue.empty())
                {
                    strong_this->schedule_send();
                }
            }

            callback(ec, bytes);
        }));

}

void
node::schedule_send()
{
    if (this->backoff_time)
    {
        this->backoff_timer->expires_from_now(std::chrono::milliseconds{this->backoff_time});
        this->backoff_timer->async_wait(this->strand->wrap(
            [weak_this = weak_from_this()](const auto& ec)
            {
                if (auto strong_this = weak_this.lock(); !ec)
                {
                    assert(!strong_this->send_queue.empty());
                    strong_this->do_send();
                }
            }));
    }
    else
    {
        assert(!this->send_queue.empty());
        this->do_send();
    }
}

void
node::send(const std::string& msg, const completion_handler_t& callback, bool is_retry)
{
    this->queued_send(msg, [weak_this = weak_from_this(), callback, is_retry, msg](auto ec, auto /*bytes*/)
    {
        try
        {
            if (ec == boost::beast::websocket::error::closed || ec == boost::asio::error::eof
                || ec == boost::asio::error::operation_aborted)
            {
                if (auto strong_this = weak_this.lock())
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

                            if (auto strong_this = weak_this.lock())
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
    this->websocket->async_read(*buffer,
        this->strand->wrap([weak_this = weak_from_this(), buffer, ws = this->websocket](auto ec, auto /*bytes*/)
    {
        try
        {
            if (auto strong_this = weak_this.lock())
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
    }));
}

void
node::close()
{
    if (this->websocket && this->websocket->is_open())
    {
        this->connected = false;

        // hold onto reference to websocket to prevent boost exception if node is gone
        this->websocket->async_close(boost::beast::websocket::close_code::normal
            , this->strand->wrap([ws = this->websocket](auto)
        {
            // ignore close errors
        }));
    }
}
