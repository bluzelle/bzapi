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
#include <gmock/gmock.h>


// gmock_gen.py generated...

namespace bzn::asio {

    class mock_tcp_socket_base : public tcp_socket_base {
    public:
        MOCK_METHOD0(get_tcp_socket,
            boost::asio::ip::tcp::socket&());
        MOCK_METHOD2(async_connect,
            void(const boost::asio::ip::tcp::endpoint& ep, bzn::asio::connect_handler handler));
        MOCK_METHOD0(remote_endpoint,
            boost::asio::ip::tcp::endpoint());
    };

}  // namespace bzn::asio


namespace bzn::asio {

    class mock_udp_socket_base : public udp_socket_base {
    public:
        MOCK_METHOD3(async_send_to, void(const boost::asio::const_buffer& msg,
                           boost::asio::ip::udp::endpoint ep,
                           std::function<void(const boost::system::error_code&, size_t)> handler));
    };

}  // namespace bzn::asio


namespace bzn::asio {

    class mock_tcp_acceptor_base : public tcp_acceptor_base {
    public:
        MOCK_METHOD2(async_accept,
            void(bzn::asio::tcp_socket_base& socket, bzn::asio::accept_handler handler));
        MOCK_METHOD0(get_tcp_acceptor,
            boost::asio::ip::tcp::acceptor&());
    };

}  // namespace bzn::asio


namespace bzn::asio {

    class mock_steady_timer_base : public steady_timer_base {
    public:
        MOCK_METHOD1(async_wait,
            void(wait_handler handler));
        MOCK_METHOD1(expires_from_now,
            std::size_t(const std::chrono::milliseconds& expiry_time));
        MOCK_METHOD0(cancel,
            void());
        MOCK_METHOD0(get_steady_timer,
            boost::asio::steady_timer&());
    };

}  // namespace bzn::asio


namespace bzn::asio {

    class mock_strand_base : public strand_base {
    public:
        MOCK_METHOD1(wrap,
            bzn::asio::write_handler(write_handler handler));
        MOCK_METHOD1(wrap,
            bzn::asio::close_handler(close_handler handler));
        MOCK_METHOD1(wrap,
            bzn::asio::task(bzn::asio::task handler));
        MOCK_METHOD1(post,
            void(bzn::asio::task task));
        MOCK_METHOD0(get_strand,
            boost::asio::io_context::strand&());
    };

}  // namespace bzn::asio


namespace bzn::asio {

    class mock_io_context_base : public io_context_base {
    public:
        MOCK_METHOD1(make_unique_tcp_acceptor,
            std::unique_ptr<bzn::asio::tcp_acceptor_base>(const boost::asio::ip::tcp::endpoint& ep));
        MOCK_METHOD0(make_unique_tcp_socket,
            std::unique_ptr<bzn::asio::tcp_socket_base>());
        MOCK_METHOD1(make_unique_tcp_socket,
                std::unique_ptr<bzn::asio::tcp_socket_base>(bzn::asio::strand_base&));
        MOCK_METHOD0(make_unique_udp_socket,
                     std::unique_ptr<bzn::asio::udp_socket_base>());
        MOCK_METHOD0(make_unique_steady_timer,
            std::unique_ptr<bzn::asio::steady_timer_base>());
        MOCK_METHOD0(make_unique_strand,
            std::unique_ptr<bzn::asio::strand_base>());
        MOCK_METHOD1(post,
            void(bzn::asio::task));
        MOCK_METHOD0(run,
            boost::asio::io_context::count_type());
        MOCK_METHOD0(stop,
            void());
        MOCK_METHOD0(get_io_context,
            boost::asio::io_context&());
    };

}  // namespace bzn::asio


namespace bzn::beast {

    class mock_http_socket_base : public http_socket_base {
    public:
        MOCK_METHOD0(get_socket,
            boost::asio::ip::tcp::socket&());
        MOCK_METHOD3(async_read,
            void(boost::beast::flat_buffer& buffer, boost::beast::http::request<boost::beast::http::dynamic_body>& request, bzn::beast::read_handler handler));
        MOCK_METHOD2(async_write,
            void(boost::beast::http::response<boost::beast::http::dynamic_body>& response, bzn::beast::write_handler handler));
        MOCK_METHOD0(close,
            void());
    };

}  // namespace bzn::beast


namespace bzn::beast {

    class mock_websocket_stream_base : public websocket_stream_base {
    public:
        MOCK_METHOD0(get_websocket,
            boost::beast::websocket::stream<boost::asio::ip::tcp::socket>&());
        MOCK_METHOD1(async_accept,
            void(bzn::asio::accept_handler handler));
        MOCK_METHOD2(async_read,
            void(boost::beast::multi_buffer& buffer, bzn::asio::read_handler handler));
        MOCK_METHOD2(async_write,
            void(const boost::asio::mutable_buffers_1& buffer, bzn::asio::write_handler handler));
        MOCK_METHOD2(write,
            size_t(const boost::asio::mutable_buffers_1& buffer, boost::beast::error_code& ec));
        MOCK_METHOD2(async_close,
            void(boost::beast::websocket::close_code reason, bzn::beast::close_handler handler));
        MOCK_METHOD3(async_handshake,
            void(const std::string& host, const std::string& target, bzn::beast::handshake_handler handler));
        MOCK_METHOD0(is_open,
            bool());
        MOCK_METHOD1(binary,
            void(bool bin));
    };

}  // namespace bzn::beast


namespace bzn::beast {

    class mock_websocket_base : public websocket_base {
    public:
        MOCK_METHOD1(make_unique_websocket_stream,
            std::unique_ptr<bzn::beast::websocket_stream_base>(boost::asio::ip::tcp::socket& socket));
    };

}  // namespace bzn::beast
