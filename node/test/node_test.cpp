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

#include <gtest/gtest.h>
#include <mocks/mock_boost_asio_beast.hpp>
#include <node/node.hpp>

using namespace testing;

boost::asio::io_context real_io_context;

class node_test : public Test
{
public:
    void init_test()
    {
        EXPECT_CALL(*io_context, get_io_context()).Times(AtLeast(1)).WillRepeatedly(ReturnRef(real_io_context));

        EXPECT_CALL(*io_context, make_unique_tcp_socket()).Times(Exactly(2))
            .WillRepeatedly(Invoke([&]()
        {
            auto tcp_sock = std::make_unique<bzn::asio::mock_tcp_socket_base>();

            EXPECT_CALL(*tcp_sock, async_connect(_, _)).WillOnce(Invoke([&](auto, auto lambda)
            {
                lambda(boost::system::error_code{});
            }));

            static boost::asio::io_context io;
            static boost::asio::ip::tcp::socket socket(io);
            EXPECT_CALL(*tcp_sock, get_tcp_socket()).WillRepeatedly(ReturnRef(socket));

            return tcp_sock;
        }));

        EXPECT_CALL(*io_context, make_unique_strand()).WillRepeatedly(Invoke(
            []()
            {
                auto strand = std::make_unique<bzn::asio::mock_strand_base>();
                EXPECT_CALL(*strand, wrap(A<bzn::asio::close_handler>())).WillRepeatedly(ReturnArg<0>());
                EXPECT_CALL(*strand, wrap(A<bzn::asio::write_handler>())).WillRepeatedly(ReturnArg<0>());
                EXPECT_CALL(*strand, post(_)).WillRepeatedly(Invoke([](auto func)
                {
                    func();
                }));
                return strand;
            }));

        this->node = std::make_shared<bzapi::node>(io_context, ws_factory, "127.0.0.1", 80);
    }

protected:
    std::shared_ptr<bzapi::node> node;
    std::shared_ptr<bzn::asio::mock_io_context_base> io_context = std::make_shared<bzn::asio::mock_io_context_base>();
    std::shared_ptr<bzn::beast::mock_websocket_base> ws_factory = std::make_shared<bzn::beast::mock_websocket_base>();
};

TEST_F(node_test, test_send_and_receive_message)
{
    for (size_t clean_close = 0; clean_close < 2; clean_close++)
    {
        init_test();

        std::string test_str{"hello world"};
        std::string test_resp{"bonjour le monde"};
        boost::beast::multi_buffer *read_buffer = nullptr;
        bzn::asio::read_handler read_cb;
        std::string request;

        EXPECT_CALL(*ws_factory, make_websocket_stream(_)).Times(Exactly(2))
            .WillOnce(Invoke([&](auto& /*sock*/)
            {
                auto websocket = std::make_unique<bzn::beast::mock_websocket_stream_base>();

                EXPECT_CALL(*websocket, binary(_)).Times(AtLeast(1));

                EXPECT_CALL(*websocket, async_handshake(_, _, _)).WillOnce(Invoke([&](auto, auto, auto lambda)
                {
                    lambda(boost::system::error_code{});
                }));

                EXPECT_CALL(*websocket, async_read(_, _)).Times(Exactly(1)).WillOnce(Invoke([&](auto& buffer, auto cb)
                {
                    // save the read info for later
                    read_buffer = &buffer;
                    read_cb = cb;

                }));

                if (clean_close)
                {
                    // in this case, the connection is gracefully closed and will be automatically re-opened
                    EXPECT_CALL(*websocket, async_write(_, _)).WillOnce(Invoke([&](const auto& buffer, auto cb)
                    {
                        request = std::string(static_cast<char *>(buffer.data()), buffer.size());

                        cb(boost::system::error_code{}, test_str.size());
                    }));

                    // we intend to close the socket below
                    EXPECT_CALL(*websocket, is_open()).WillOnce(Return(true));
                    EXPECT_CALL(*websocket, async_close(_, _));
                }
                else
                {
                    // in this case, the connection will be left open but we'll fail the next write, causing a re-connect
                    EXPECT_CALL(*websocket, async_write(_, _)).Times(Exactly(2))
                        .WillOnce(Invoke([&](const auto& buffer, auto cb)
                        {
                            request = std::string(static_cast<char *>(buffer.data()), buffer.size());

                            cb(boost::system::error_code{}, test_str.size());
                        }))
                        .WillOnce(Invoke([&](const auto& /*buffer*/, auto cb)
                        {
                            cb(boost::system::error_code{boost::beast::websocket::error::closed}, 0);
                        }));

                    // don't close the socket - next write will be forced to fail
                    EXPECT_CALL(*websocket, is_open()).WillOnce(Return(false));
                }

                return websocket;
            }))
            .WillOnce(Invoke([&](auto& /*sock*/)
            {
                auto websocket = std::make_unique<bzn::beast::mock_websocket_stream_base>();

                EXPECT_CALL(*websocket, binary(_)).Times(AtLeast(1));

                EXPECT_CALL(*websocket, async_handshake(_, _, _)).WillOnce(Invoke([&](auto, auto, auto lambda)
                {
                    lambda(boost::system::error_code{});
                }));

                EXPECT_CALL(*websocket, async_read(_, _)).Times(Exactly(1)).WillOnce(Invoke([&](auto& buffer, auto cb)
                {
                    // save the read info for later
                    read_buffer = &buffer;
                    read_cb = cb;

                }));

                EXPECT_CALL(*websocket, async_write(_, _)).WillOnce(Invoke([&](const auto& buffer, auto cb)
                {
                    request = std::string(static_cast<char *>(buffer.data()), buffer.size());

                    cb(boost::system::error_code{}, test_str.size());
                }));

                return websocket;
            }));

        // send the "request"
        this->node->send_message(test_str, [&](auto ec)
        {
            EXPECT_EQ(ec, boost::system::errc::success);
        });

        EXPECT_EQ(request, test_str);

        std::string response;
        this->node->register_message_handler([&](const std::string& data) -> bool
        {
            response = data;

            // force the connection to be closed
            return true;
        });

        // fake the "response"
        size_t n = boost::asio::buffer_copy(read_buffer->prepare(test_resp.size()), boost::asio::buffer(test_resp));
        read_buffer->commit(n);
        read_cb(boost::system::error_code{}, n);

        EXPECT_EQ(response, test_resp);

        // send another "request" to test re-open of connection
        this->node->send_message(test_str, [&](auto ec)
        {
            EXPECT_EQ(ec, boost::system::errc::success);
        });

        EXPECT_EQ(request, test_str);
    }
}
