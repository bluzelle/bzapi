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

#include <gtest/gtest.h>
#include <mocks/mock_boost_asio_beast.hpp>
#include <swarm/swarm_factory.hpp>
#include <bzapi.hpp>
#include <crypto/null_crypto.hpp>
#include <library/library.hpp>
#include <jsoncpp/src/jsoncpp/include/json/value.h>
#include <jsoncpp/src/jsoncpp/include/json/reader.h>

using namespace testing;
using namespace bzapi;

namespace bzapi
{
    extern std::shared_ptr<bzn::asio::io_context_base> io_context;
    extern std::shared_ptr<swarm_factory> the_swarm_factory;
    extern std::shared_ptr<crypto_base> the_crypto;
    extern std::shared_ptr<bzn::beast::websocket_base> ws_factory;
}

class integration_test : public Test
{
public:
    void initialize()
    {
        mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        io_context = mock_io_context;
        the_crypto = std::make_shared<null_crypto>();
        mock_ws_factory = std::make_shared<bzn::beast::Mockwebsocket_base>();
        ws_factory = mock_ws_factory;
        the_swarm_factory = std::make_shared<swarm_factory>(io_context, ws_factory, the_crypto);
        the_swarm_factory->temporary_set_default_endpoint("ws://127.0.0.1:50000");
    }

    void teardown()
    {
        the_swarm_factory = nullptr;
        io_context = nullptr;
        ws_factory = nullptr;
    }

protected:
    std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context;
    std::shared_ptr<bzn::beast::Mockwebsocket_base> mock_ws_factory;
};

TEST_F(integration_test, test_has_db)
{
    uuid_t uuid{"my_uuid"};
    this->initialize();

//    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(AtLeast(1)).WillRepeatedly(Invoke([]()
//    {
//        return std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
//    }));

    EXPECT_CALL(*mock_io_context, make_unique_tcp_socket()).Times(AtLeast(1)).WillRepeatedly(Invoke([&]()
    {
        auto tcp_sock = std::make_unique<bzn::asio::Mocktcp_socket_base>();

        static boost::asio::io_context io;
        static boost::asio::ip::tcp::socket socket(io);
        EXPECT_CALL(*tcp_sock, get_tcp_socket()).WillRepeatedly(ReturnRef(socket));

        EXPECT_CALL(*tcp_sock, async_connect(_, _)).WillOnce(Invoke([&](auto, auto& cb)
        {
            cb(boost::system::error_code{});
        }));

        return tcp_sock;
    }));

    EXPECT_CALL(*mock_ws_factory, make_unique_websocket_stream(_)).Times(Exactly(1)).WillOnce(Invoke([&](auto&)
    {
        auto websocket = std::make_unique<bzn::beast::Mockwebsocket_stream_base>();

        EXPECT_CALL(*websocket, async_handshake(_, _, _)).WillOnce(Invoke([&](auto, auto, auto lambda)
        {
            lambda(boost::system::error_code{});
        }));

        static bzn::asio::read_handler read_handler;
        static boost::beast::multi_buffer *read_buffer;

        EXPECT_CALL(*websocket, async_read(_, _)).Times(Exactly(1))
            .WillOnce(Invoke([uuid](boost::beast::multi_buffer& buffer, auto cb)
            {
                read_handler = cb;
                read_buffer = &buffer;

                database_has_db_response has_db;
                has_db.set_uuid(uuid);
                has_db.set_has(true);

                database_header header;
                header.set_nonce(1);
                database_response response;
                response.set_allocated_has_db(new database_has_db_response(has_db));
                response.set_allocated_header(new database_header(header));

                bzn_envelope env2;
                env2.set_database_response(response.SerializeAsString());
                env2.set_sender("uuid1");
                auto message = env2.SerializeAsString();
                boost::asio::buffer_copy(read_buffer->prepare(message.size()), boost::asio::buffer(message));
                read_buffer->commit(message.size());
                read_handler(boost::system::error_code{}, message.size());
            }));

        EXPECT_CALL(*websocket, async_write(_, _)).WillOnce(Invoke([&](const boost::asio::mutable_buffers_1& buffer, auto cb)
        {
            bzn_envelope env;
            EXPECT_TRUE(env.ParseFromString(std::string(static_cast<const char *>(buffer.data()), buffer.size())));

            database_msg db_msg;
            EXPECT_TRUE(db_msg.ParseFromString(env.database_msg()));

            EXPECT_TRUE(db_msg.has_has_db());
            EXPECT_TRUE(db_msg.header().db_uuid() == uuid);
            cb(boost::system::error_code{}, buffer.size());

        }));

        EXPECT_CALL(*websocket, is_open()).Times(Exactly(1)).WillOnce(Return(false));

        return websocket;

    }));

    auto response = has_db(uuid.c_str());
    EXPECT_TRUE(response->is_ready());
    auto resp = response->get_result();
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(resp, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);

    this->teardown();
}