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

#include <include/bluzelle.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>

// todo: this file needs a better name!

namespace bzn::asio
{
    // types...
    using  accept_handler = std::function<void(const boost::system::error_code& ec)>;
    using    read_handler = std::function<void(const boost::system::error_code& ec, size_t bytes_transfered)>;
    using   write_handler = std::function<void(const boost::system::error_code& ec, size_t bytes_transfered)>;
    using connect_handler = std::function<void(const boost::system::error_code& ec)>;
    using   close_handler = std::function<void(const boost::system::error_code& ec)>;
    using    wait_handler = std::function<void(const boost::system::error_code& ec)>;
    using            task = std::function<void()>;

    ///////////////////////////////////////////////////////////////////////////
    // mockable interfaces...

    class tcp_socket_base
    {
    public:
        virtual ~tcp_socket_base() = default;

        virtual void async_connect(const boost::asio::ip::tcp::endpoint& ep, bzn::asio::connect_handler handler) = 0;

        virtual boost::asio::ip::tcp::endpoint remote_endpoint() = 0;

        virtual boost::asio::ip::tcp::socket& get_tcp_socket() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class udp_socket_base
    {
    public:
        virtual ~udp_socket_base() = default;

        virtual void async_send_to(const boost::asio::const_buffer& msg, boost::asio::ip::udp::endpoint ep, bzn::asio::write_handler handler) = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class tcp_acceptor_base
    {
    public:
        virtual ~tcp_acceptor_base() = default;

        virtual void async_accept(bzn::asio::tcp_socket_base& socket, bzn::asio::accept_handler handler) = 0;

        virtual boost::asio::ip::tcp::acceptor& get_tcp_acceptor() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class steady_timer_base
    {
    public:
        virtual ~steady_timer_base() = default;

        virtual void async_wait(wait_handler handler) = 0;

        virtual std::size_t expires_from_now(const std::chrono::milliseconds& expiry_time) = 0;

        virtual void cancel() = 0;

        virtual boost::asio::steady_timer& get_steady_timer() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class strand_base
    {
    public:
        virtual ~strand_base() = default;

        virtual bzn::asio::write_handler wrap(write_handler handler) = 0;

        virtual bzn::asio::close_handler wrap(close_handler handler) = 0;

        virtual bzn::asio::task wrap(bzn::asio::task action) = 0;

        virtual void post(bzn::asio::task action) = 0;

        virtual boost::asio::io_context::strand& get_strand() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class io_context_base
    {
    public:
        virtual ~io_context_base() = default;

        virtual std::unique_ptr<bzn::asio::tcp_acceptor_base> make_unique_tcp_acceptor(const boost::asio::ip::tcp::endpoint& ep) = 0;

        virtual std::unique_ptr<bzn::asio::tcp_socket_base> make_unique_tcp_socket() = 0;

        virtual std::unique_ptr<bzn::asio::tcp_socket_base> make_unique_tcp_socket(bzn::asio::strand_base& ctx) = 0;

        virtual std::unique_ptr<bzn::asio::udp_socket_base> make_unique_udp_socket() = 0;

        virtual std::unique_ptr<bzn::asio::steady_timer_base> make_unique_steady_timer() = 0;

        virtual std::unique_ptr<bzn::asio::strand_base> make_unique_strand() = 0;

        virtual void post(bzn::asio::task) = 0;

        virtual boost::asio::io_context::count_type run() = 0;


        virtual void stop() = 0;

        virtual boost::asio::io_context& get_io_context() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    // the real thing...

    class udp_socket final : public udp_socket_base
    {
    public:
        explicit udp_socket(boost::asio::io_context& io_context)
            : socket(io_context)
        {
            this->socket.open(boost::asio::ip::udp::v4());
        }

        void async_send_to(const boost::asio::const_buffer& msg, boost::asio::ip::udp::endpoint ep, bzn::asio::write_handler handler)
        {
            this->socket.async_send_to(msg, ep, handler);
        }

    private:
        boost::asio::ip::udp::socket socket;
    };

    ///////////////////////////////////////////////////////////////////////////

    class tcp_socket final : public tcp_socket_base
    {
    public:
        explicit tcp_socket(boost::asio::io_context& io_context)
            : socket(io_context)
        {
        }

        explicit tcp_socket(bzn::asio::strand_base& ctx)
            : socket(ctx.get_strand())
        {
        }

        void async_connect(const boost::asio::ip::tcp::endpoint& ep, bzn::asio::connect_handler handler) override
        {
            this->socket.async_connect(ep, handler);
        }

        boost::asio::ip::tcp::endpoint remote_endpoint() override
        {
            return this->socket.remote_endpoint();
        }

        boost::asio::ip::tcp::socket& get_tcp_socket() override
        {
            return this->socket;
        }

    private:
        boost::asio::ip::tcp::socket socket;
    };

    ///////////////////////////////////////////////////////////////////////////

    class tcp_acceptor final : public tcp_acceptor_base
    {
    public:
        explicit tcp_acceptor(boost::asio::io_context& io_context, const boost::asio::ip::tcp::endpoint& ep)
            : acceptor(io_context, ep)
        {
        }

        void async_accept(bzn::asio::tcp_socket_base& socket, bzn::asio::accept_handler handler) override
        {
            this->acceptor.async_accept(socket.get_tcp_socket(), std::move(handler));
        }

        boost::asio::ip::tcp::acceptor& get_tcp_acceptor() override
        {
            return this->acceptor;
        }

    private:
        boost::asio::ip::tcp::acceptor acceptor;
    };

    ///////////////////////////////////////////////////////////////////////////

    class steady_timer final : public steady_timer_base
    {
    public:
        explicit steady_timer(boost::asio::io_context& io_context)
            : timer(io_context)
        {
        }

        void async_wait(wait_handler handler) override
        {
            this->timer.async_wait(handler);
        }

        std::size_t expires_from_now(const std::chrono::milliseconds& expiry_time) override
        {
            return this->timer.expires_from_now(expiry_time);
        }

        void cancel() override
        {
            this->timer.cancel();
        }

        boost::asio::steady_timer& get_steady_timer() override
        {
            return this->timer;
        }

    private:
        boost::asio::steady_timer timer;
    };

    ///////////////////////////////////////////////////////////////////////////

    class strand final : public strand_base
    {
    public:
        explicit strand(boost::asio::io_context& io_context)
            : s(io_context)
        {
        }

        bzn::asio::write_handler wrap(write_handler handler) override
        {
            return this->s.wrap(std::move(handler));
        }

        bzn::asio::close_handler wrap(close_handler handler) override
        {
            return this->s.wrap(std::move(handler));
        }

        bzn::asio::task wrap(bzn::asio::task action) override
        {
            return this->s.wrap(std::move(action));
        }

        void post(bzn::asio::task action) override
        {
            this->s.post(action);
        }

        boost::asio::io_context::strand& get_strand() override
        {
            return this->s;
        }

    private:
        boost::asio::io_context::strand s;
    };

    ///////////////////////////////////////////////////////////////////////////

    class io_context final : public io_context_base
    {
    public:
        std::unique_ptr<bzn::asio::tcp_acceptor_base> make_unique_tcp_acceptor(const boost::asio::ip::tcp::endpoint& ep) override
        {
            return std::make_unique<bzn::asio::tcp_acceptor>(this->io_context, ep);
        }

        std::unique_ptr<bzn::asio::tcp_socket_base> make_unique_tcp_socket() override
        {
            return std::make_unique<bzn::asio::tcp_socket>(this->io_context);
        }

        std::unique_ptr<bzn::asio::tcp_socket_base> make_unique_tcp_socket(bzn::asio::strand_base& ctx) override
        {
            return std::make_unique<bzn::asio::tcp_socket>(ctx);
        }

        std::unique_ptr<bzn::asio::udp_socket_base> make_unique_udp_socket() override
        {
            return std::make_unique<bzn::asio::udp_socket>(this->io_context);
        }

        std::unique_ptr<bzn::asio::steady_timer_base> make_unique_steady_timer() override
        {
            return std::make_unique<bzn::asio::steady_timer>(this->io_context);
        }

        std::unique_ptr<bzn::asio::strand_base> make_unique_strand() override
        {
            return std::make_unique<bzn::asio::strand>(this->io_context);
        }

        void post(bzn::asio::task func) override
        {
            boost::asio::post(this->io_context, func);
        }

        boost::asio::io_context::count_type run() override
        {
            return this->io_context.run();
        }

        void stop() override
        {
            this->io_context.stop();
        }

        boost::asio::io_context& get_io_context() override
        {
            return this->io_context;
        }

    private:
        boost::asio::io_context io_context;
    };

} // bzn::asio


namespace bzn::beast
{
    // types...
    using handshake_handler = std::function<void(const boost::system::error_code& ec)>;
    using read_handler  = std::function<void(const boost::beast::error_code& ec, std::size_t bytes_transferred)>;
    using write_handler = std::function<void(const boost::beast::error_code& ec, std::size_t bytes_transferred)>;
    using close_handler = std::function<void(const boost::system::error_code& ec)>;

    ///////////////////////////////////////////////////////////////////////////
    // mockable interfaces...

    class http_socket_base
    {
    public:
        virtual ~http_socket_base() = default;

        virtual boost::asio::ip::tcp::socket& get_socket() = 0;

        virtual void async_read(boost::beast::flat_buffer& buffer, boost::beast::http::request<boost::beast::http::dynamic_body>& request, bzn::beast::read_handler handler) = 0;

        virtual void async_write(boost::beast::http::response<boost::beast::http::dynamic_body>& response, bzn::beast::write_handler handler) = 0;

        virtual void close() = 0;
    };

    class http_socket final : public http_socket_base
    {
    public:
        explicit http_socket(boost::asio::ip::tcp::socket socket)
            : socket(std::move(socket))
        {
        }

        boost::asio::ip::tcp::socket& get_socket() override
        {
            return this->socket;
        }

        void async_read(boost::beast::flat_buffer& buffer, boost::beast::http::request<boost::beast::http::dynamic_body>& request, bzn::beast::read_handler handler) override
        {
            boost::beast::http::async_read(this->socket, buffer, request, std::move(handler));
        }

        void async_write(boost::beast::http::response<boost::beast::http::dynamic_body>& response, bzn::beast::write_handler handler) override
        {
            boost::beast::http::async_write(this->socket, response, std::move(handler));
        }

        void close() override
        {
            this->socket.close();
        }

    private:
        boost::asio::ip::tcp::socket socket;
    };

    ///////////////////////////////////////////////////////////////////////////

    class websocket_stream_base
    {
    public:
        virtual ~websocket_stream_base() = default;

        virtual void async_accept(bzn::asio::accept_handler handler) = 0;

        virtual void async_read(boost::beast::multi_buffer& buffer, bzn::asio::read_handler handler) = 0;

        virtual void async_write(const boost::asio::mutable_buffers_1& buffer, bzn::asio::write_handler handler) = 0;

        virtual size_t write(const boost::asio::mutable_buffers_1& buffer, boost::beast::error_code& ec) = 0;

        virtual void async_close(boost::beast::websocket::close_code reason, bzn::beast::close_handler handler) = 0;

        virtual void async_handshake(const std::string& host, const std::string& target, bzn::beast::handshake_handler handler) = 0;

        virtual void binary(bool bin) = 0;

        virtual bool is_open() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class websocket_stream final : public websocket_stream_base
    {
    public:
        explicit websocket_stream(boost::asio::ip::tcp::socket socket)
            : websocket(std::move(socket))
        {
        }

        void async_accept(bzn::asio::accept_handler handler) override
        {
            this->websocket.async_accept(handler);
        }

        void async_read(boost::beast::multi_buffer& buffer, bzn::asio::read_handler handler) override
        {
            this->websocket.async_read(buffer, handler);
        }

        void async_write(const boost::asio::mutable_buffers_1& buffer, bzn::asio::write_handler handler) override
        {
            this->websocket.async_write(buffer, handler);
        }

        size_t write(const boost::asio::mutable_buffers_1& buffer, boost::beast::error_code& ec) override
        {
            return this->websocket.write(buffer, ec);
        }

        void async_close(boost::beast::websocket::close_code reason, bzn::beast::close_handler handler) override
        {
            this->websocket.async_close(reason, handler);
        }

        void async_handshake(const std::string& host, const std::string& target, bzn::beast::handshake_handler handler) override
        {
            this->websocket.async_handshake(host, target, handler);
        }

        bool is_open() override
        {
            return this->websocket.is_open();
        }

        void binary(bool bin) override
        {
            this->websocket.binary(bin);
        }

    private:
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> websocket;
    };



    class websocket_secure_stream final : public websocket_stream_base, public std::enable_shared_from_this<websocket_secure_stream>
    {
    public:
        explicit websocket_secure_stream(boost::asio::ip::tcp::socket socket, boost::asio::ssl::context& ctx)
            : websocket(std::move(socket), ctx)
        {
            // todo: add peer validation
        }

        void async_accept(bzn::asio::accept_handler handler) override
        {
            boost::beast::get_lowest_layer(this->websocket).expires_after(std::chrono::seconds(30));

            this->websocket.next_layer().async_handshake(
                boost::asio::ssl::stream_base::server,
                [self = shared_from_this(), handler](auto ec)
                {
                    if (ec)
                    {
                        LOG(error) << "server ssl handshake failed: " << ec.message();
                        return;
                    }

                    boost::beast::get_lowest_layer(self->websocket).expires_never();

                    self->websocket.async_accept(handler);
                });
        }

        void async_read(boost::beast::multi_buffer& buffer, bzn::asio::read_handler handler) override
        {
            this->websocket.async_read(buffer, handler);
        }

        void async_write(const boost::asio::mutable_buffers_1& buffer, bzn::asio::write_handler handler) override
        {
            this->websocket.async_write(buffer, handler);
        }

        size_t write(const boost::asio::mutable_buffers_1& buffer, boost::beast::error_code& ec) override
        {
            return this->websocket.write(buffer, ec);
        }

        void async_close(boost::beast::websocket::close_code reason, bzn::beast::close_handler handler) override
        {
            this->websocket.async_close(reason, handler);
        }

        void async_handshake(const std::string& host, const std::string& target, bzn::beast::handshake_handler handler) override
        {
            boost::beast::get_lowest_layer(this->websocket).expires_after(std::chrono::seconds(30));

            this->websocket.next_layer().async_handshake(
                boost::asio::ssl::stream_base::client,
                [self = shared_from_this(), host, target, handler](auto ec)
                {
                    if (ec)
                    {
                        LOG(error) << "client ssl handshake failed: " << ec.message();
                        return;
                    }

                    boost::beast::get_lowest_layer(self->websocket).expires_never();

                    self->websocket.async_handshake(host, target, handler);
                });
        }

        bool is_open() override
        {
            return this->websocket.is_open();
        }

        void binary(bool bin) override
        {
            this->websocket.binary(bin);
        }

    private:
        boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> websocket;
    };
    //

    ///////////////////////////////////////////////////////////////////////////

    class websocket_base
    {
    public:
        virtual ~websocket_base() = default;

        virtual std::unique_ptr<bzn::beast::websocket_stream_base> make_websocket_stream(boost::asio::ip::tcp::socket& socket) = 0;

        virtual std::shared_ptr<bzn::beast::websocket_stream_base> make_websocket_secure_stream(boost::asio::ip::tcp::socket& socket,
            boost::asio::ssl::context& ctx) = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class websocket final : public websocket_base
    {
    public:
        std::unique_ptr<bzn::beast::websocket_stream_base> make_websocket_stream(boost::asio::ip::tcp::socket& socket) override
        {
            return std::make_unique<bzn::beast::websocket_stream>(std::move(socket));
        }

        std::shared_ptr<bzn::beast::websocket_stream_base> make_websocket_secure_stream(boost::asio::ip::tcp::socket& socket,
            boost::asio::ssl::context& ctx) override
        {
            return std::make_shared<bzn::beast::websocket_secure_stream>(std::move(socket), ctx);
        }
    };

} // bzn::beast
