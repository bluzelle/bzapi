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
#include <crypto/crypto.hpp>
#include <library/library.hpp>
#include <jsoncpp/src/jsoncpp/include/json/value.h>
#include <jsoncpp/src/jsoncpp/include/json/reader.h>
#include <library/udp_response.hpp>

using namespace testing;
using namespace bzapi;

namespace bzapi
{
    extern std::shared_ptr<bzn::asio::io_context_base> io_context;
    extern std::shared_ptr<swarm_factory> the_swarm_factory;
    extern std::shared_ptr<crypto_base> the_crypto;
    extern std::shared_ptr<bzn::beast::websocket_base> ws_factory;
}

namespace
{
    const std::string SWARM_VERSION{".."};
    const std::string SWARM_GIT_COMMIT{".."};
    const std::string UPTIME{"1:03:01"};
}

// this is kinda ugly, but we need to identify the node somehow
std::map<uint16_t, boost::asio::ip::tcp::socket*> sock_map;

uint16_t get_node_from_sock(const boost::asio::ip::tcp::socket& sock)
{
    for (auto& n : sock_map)
    {
        if (n.second== &sock)
        {
            return n.first - 50000;
        }
    }

    return 0;
}

// this class is used to save a node id so we can associate it with later calls
struct my_mock_tcp_socket : public bzn::asio::tcp_socket_base
{
    void async_connect(const boost::asio::ip::tcp::endpoint& ep, bzn::asio::connect_handler handler) override
    {
        id = ep.port();
        sock_map[id] = &this->socket;
        handler(boost::system::error_code{});
    }

    virtual boost::asio::ip::tcp::endpoint remote_endpoint() override
    {
        return boost::asio::ip::tcp::endpoint{};
    }

    virtual boost::asio::ip::tcp::socket& get_tcp_socket() override
    {
        return socket;
    }

    int id{};
    static boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket{io};
};

boost::asio::io_context my_mock_tcp_socket::io;

struct mock_websocket
{
    mock_websocket(uint16_t id = 0)
    : id(id) {}

    std::unique_ptr<bzn::beast::Mockwebsocket_stream_base> get()
    {
        auto websocket = std::make_unique<bzn::beast::Mockwebsocket_stream_base>();

        EXPECT_CALL(*websocket, async_handshake(_, _, _)).WillOnce(Invoke([&](auto, auto, auto lambda)
        {
            lambda(boost::system::error_code{});
        }));

        EXPECT_CALL(*websocket, async_read(_, _)).Times(AtLeast(1))
            .WillRepeatedly(Invoke([&](boost::beast::multi_buffer& buffer, auto cb)
            {
                read_handler = cb;
                read_buffer = &buffer;

                if (read_func)
                {
                    auto r = read_func;
                    read_func = nullptr;
                    r(buffer);
                }
            }));

        EXPECT_CALL(*websocket, async_write(_, _)).WillRepeatedly(Invoke([&](const boost::asio::mutable_buffers_1& buffer, auto cb)
        {
            if (write_func)
            {
                auto w = write_func;
                write_func = nullptr;
                w(buffer);
            }

            cb(boost::system::error_code{}, buffer.size());
        }));

//        EXPECT_CALL(*websocket, is_open()).Times(Exactly(1)).WillOnce(Return(false));

        return websocket;
    }

    void simulate_read(const std::string& message)
    {
        if (read_buffer)
        {
            boost::asio::buffer_copy(read_buffer->prepare(message.size()), boost::asio::buffer(message));
            read_buffer->commit(message.size());
            read_handler(boost::system::error_code{}, message.size());
        }
    }

    std::function<void(const boost::asio::mutable_buffers_1& buffer)> write_func;
    std::function<void(boost::beast::multi_buffer& buffer)> read_func;
    bzn::asio::read_handler read_handler;
    boost::beast::multi_buffer *read_buffer = nullptr;
    uint16_t id{};
    std::shared_ptr<bzn::asio::steady_timer_base> response_timer;
    completion_handler_t timer_callback;
};


class integration_test : public Test, public std::enable_shared_from_this<integration_test>
{
public:
    void initialize(uuid_t _uuid)
    {
        this->uuid = _uuid;
        mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        io_context = mock_io_context;
        the_crypto = std::make_shared<null_crypto>();
        mock_ws_factory = std::make_shared<bzn::beast::Mockwebsocket_base>();
        ws_factory = mock_ws_factory;
        the_swarm_factory = std::make_shared<swarm_factory>(io_context, ws_factory, the_crypto, this->uuid);
        auto swf = the_swarm_factory;
        swf->temporary_set_default_endpoint("ws://127.0.0.1:50000");
//        the_swarm_factory->temporary_set_default_endpoint("ws://127.0.0.1:50000");
    }

    void teardown()
    {
        the_swarm_factory = nullptr;
        io_context = nullptr;
        ws_factory = nullptr;
    }

    status_response make_status_response()
    {
        status_response srm;
        srm.set_swarm_version(SWARM_VERSION);
        srm.set_swarm_git_commit(SWARM_GIT_COMMIT);
        srm.set_uptime(UPTIME);

        Json::Value pbft_status;
        pbft_status["primary"]["uuid"] = this->primary_node;

        Json::Value peer_index;
        for (const auto& p : this->nodes)
        {
            Json::Value peer;
            peer["host"] = "127.0.0.1";
            peer["port"] = 50000 + p.second.id;
            peer["uuid"] = "node_" + std::to_string(p.second.id);
            peer_index.append(peer);
        }

        pbft_status["peer_index"] = peer_index;
        Json::Value module_status;
        module_status["pbft"] = pbft_status;
        srm.set_module_status_json(module_status.toStyledString());

        return srm;
    }

    void expect_has_db()
    {
//    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(AtLeast(1)).WillRepeatedly(Invoke([]()
//    {
//        return std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
//    }));

        EXPECT_CALL(*mock_io_context, make_unique_tcp_socket()).Times(Exactly(1)).WillOnce(Invoke([]()
        {
            auto tcp_sock = std::make_unique<my_mock_tcp_socket>();
            return tcp_sock;
        })).RetiresOnSaturation();

        EXPECT_CALL(*mock_ws_factory, make_unique_websocket_stream(_)).Times(Exactly(1)).WillOnce(Invoke([uuid = this->uuid](auto&)
        {
            static mock_websocket ws{0};

            ws.write_func = [uuid](const boost::asio::mutable_buffers_1& buffer)
            {
                bzn_envelope env;
                EXPECT_TRUE(env.ParseFromString(std::string(static_cast<const char *>(buffer.data()), buffer.size())));

                database_msg db_msg;
                EXPECT_TRUE(db_msg.ParseFromString(env.database_msg()));

                EXPECT_TRUE(db_msg.has_has_db());
                EXPECT_TRUE(db_msg.header().db_uuid() == uuid);

            };

            ws.read_func = [uuid](const auto& /*buffer*/)
            {
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
                boost::asio::buffer_copy(ws.read_buffer->prepare(message.size()), boost::asio::buffer(message));
                ws.read_buffer->commit(message.size());
                ws.read_handler(boost::system::error_code{}, message.size());
            };

            return ws.get();
        })).RetiresOnSaturation();
    }

    void expect_swarm_initialize()
    {
        EXPECT_CALL(*mock_io_context, make_unique_tcp_socket()).Times(Exactly(this->nodes.size())).WillRepeatedly(Invoke([]()
        {
            auto tcp_sock = std::make_unique<my_mock_tcp_socket>();
            return tcp_sock;
        }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(AtLeast(1)).WillRepeatedly(Invoke([]()
        {
            return std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
        }));

        for (uint16_t i = 0; i < this->nodes.size(); i++)
        {
            EXPECT_CALL(*mock_ws_factory, make_unique_websocket_stream(ResultOf(get_node_from_sock, Eq(i)))).Times(
                    Exactly(1))
                .WillOnce(Invoke([uuid = this->uuid, this](auto &sock) {
                    auto node_id = get_node_from_sock(sock);
                    mock_websocket &ws = this->nodes[node_id];

                    ws.write_func = [uuid](const boost::asio::mutable_buffers_1 &buffer)
                    {
                        bzn_envelope env;
                        EXPECT_TRUE(
                            env.ParseFromString(
                                std::string(static_cast<const char *>(buffer.data()), buffer.size())));

                        status_request sr;
                        EXPECT_TRUE(sr.ParseFromString(env.status_request()));
                    };

                    ws.read_func = [this, node_id, &ws](const auto & /*buffer*/)
                    {
                        status_response sr = this->make_status_response();
                        bzn_envelope env;
                        env.set_status_response(sr.SerializeAsString());
                        env.set_sender("node_" + std::to_string(node_id));
                        auto message = env.SerializeAsString();
                        boost::asio::buffer_copy(ws.read_buffer->prepare(message.size()), boost::asio::buffer(
                            message));
                        ws.read_buffer->commit(message.size());
                        ws.read_handler(boost::system::error_code{}, message.size());
                    };

                    return ws.get();
                }));
        }
    }

protected:
    struct node_meta
    {
        uint16_t id{};
        uint64_t latency{};
        std::shared_ptr<bzn::asio::steady_timer_base> response_timer;
        completion_handler_t timer_callback;
    };

    uuid_t uuid;
    uuid_t primary_node;
    std::map<uint16_t, mock_websocket> nodes;
    std::set<my_mock_tcp_socket*> sockets;
    std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context;
    std::shared_ptr<bzn::beast::Mockwebsocket_base> mock_ws_factory;
};

TEST_F(integration_test, test_has_db)
{
    uuid_t uuid{"my_uuid"};
    this->initialize(uuid);

    expect_has_db();

    auto response = has_db(uuid.c_str());
    EXPECT_TRUE(response->is_ready());
    auto resp = response->get_result();
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(resp, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);

    this->teardown();
}

TEST_F(integration_test, test_open_db)
{
    uuid_t uuid{"my_uuid"};
    this->initialize(uuid);

    for (size_t i = 0; i < 4; i++)
    {
        mock_websocket s{static_cast<uint16_t>(i)};
        this->nodes[i] = s;
    }
    this->primary_node = "node_0";

    // reverse order is intentional to match most recent expectations first
    expect_swarm_initialize();
    expect_has_db();

    auto response = open_db(uuid.c_str());
    EXPECT_TRUE(response->is_ready());
    auto resp = response->get_result();
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(resp, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
    EXPECT_NE(response->get_db(), nullptr);

    this->teardown();

}

TEST_F(integration_test, test_create)
{
    uuid_t uuid{"my_uuid"};
    this->initialize(uuid);

    for (size_t i = 0; i < 4; i++)
    {
        mock_websocket s{static_cast<uint16_t>(i)};
        this->nodes[i] = s;
    }
    this->primary_node = "node_0";

    // reverse order is intentional to match most recent expectations first
    expect_swarm_initialize();
    expect_has_db();

    auto response = open_db(uuid.c_str());
    ASSERT_TRUE(response->is_ready());
    auto db = response->get_db();
    ASSERT_TRUE(db != nullptr);

    int nonce  = 0;

    for (size_t i = 0; i < 4; i++)
    {
        this->nodes[i].write_func = [&nonce](const boost::asio::mutable_buffers_1& buffer)
        {
            bzn_envelope env;
            EXPECT_TRUE(env.ParseFromString(std::string(static_cast<const char *>(buffer.data()), buffer.size())));

            database_msg msg;
            EXPECT_TRUE(msg.ParseFromString(env.database_msg()));
            nonce = msg.header().nonce();

            const database_create& create = msg.create();
            EXPECT_TRUE(create.key() == "test_key");
            EXPECT_TRUE(create.value() == "test_value");
        };
    }

    auto create_response = db->create("test_key", "test_value");

    for (size_t i = 0; i < 4; i++)
    {
        database_header header;
        header.set_nonce(nonce);
        database_response dr;
        dr.set_allocated_header(new database_header(header));

        bzn_envelope env2;
        env2.set_database_response(dr.SerializeAsString());
        env2.set_sender("node_" + std::to_string(i));
        auto message = env2.SerializeAsString();
        this->nodes[i].simulate_read(message);
    }

    EXPECT_TRUE(create_response->is_ready());
    auto resp = create_response->get_result();
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(resp, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
    EXPECT_EQ(create_response->get_db(), nullptr);

    this->teardown();

}

TEST_F(integration_test, test_read)
{
    uuid_t uuid{"my_uuid"};
    this->initialize(uuid);

    for (size_t i = 0; i < 4; i++)
    {
        mock_websocket s{static_cast<uint16_t>(i)};
        this->nodes[i] = s;
    }
    this->primary_node = "node_0";

    // reverse order is intentional to match most recent expectations first
    expect_swarm_initialize();
    expect_has_db();

    auto response = open_db(uuid.c_str());
    ASSERT_TRUE(response->is_ready());
    auto db = response->get_db();
    ASSERT_TRUE(db != nullptr);

    int nonce  = 0;

    for (size_t i = 0; i < 4; i++)
    {
        this->nodes[i].write_func = [&nonce](const boost::asio::mutable_buffers_1& buffer)
        {
            bzn_envelope env;
            EXPECT_TRUE(env.ParseFromString(std::string(static_cast<const char *>(buffer.data()), buffer.size())));

            database_msg msg;
            EXPECT_TRUE(msg.ParseFromString(env.database_msg()));
            nonce = msg.header().nonce();

            const database_read& read = msg.read();
            EXPECT_TRUE(read.key() == "test_key");
        };
    }

    auto create_response = db->read("test_key");

    for (size_t i = 0; i < 4; i++)
    {
        database_header header;
        header.set_nonce(nonce);
        database_response dr;
        dr.set_allocated_header(new database_header(header));

        database_read_response rr;
        rr.set_key("test_key");
        rr.set_value("test_value");
        dr.set_allocated_read(new database_read_response(rr));

        bzn_envelope env2;
        env2.set_database_response(dr.SerializeAsString());
        env2.set_sender("node_" + std::to_string(i));
        auto message = env2.SerializeAsString();
        this->nodes[i].simulate_read(message);
    }

    EXPECT_TRUE(create_response->is_ready());
    auto resp = create_response->get_result();
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(resp, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
    EXPECT_EQ(resp_json["key"].asString(), "test_key");
    EXPECT_EQ(resp_json["value"].asString(), "test_value");
    EXPECT_EQ(create_response->get_db(), nullptr);

//    for (size_t i = 0; i < 4; i++)
//    {
//        Mock::VerifyAndClearExpectations(this->nodes[i].))
//    }
    this->nodes.clear();

    this->teardown();
}

const char* priv_key = "-----BEGIN EC PRIVATE KEY-----\n"
                       "MHQCAQEEIBWDWE/MAwtXaFQp6d2Glm2Uj7ROBlDKFn5RwqQsDEbyoAcGBSuBBAAK\n"
                       "oUQDQgAEiykQ5A02u+02FR1nftxT5VuUdqLO6lvNoL5aAIyHvn8NS0wgXxbPfpuq\n"
                       "UPpytiopiS5D+t2cYzXJn19MQmnl/g==\n"
                       "-----END EC PRIVATE KEY-----";

const char* pub_key = "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEiykQ5A02u+02FR1nftxT5VuUdqLO6lvN\n"
                      "oL5aAIyHvn8NS0wgXxbPfpuqUPpytiopiS5D+t2cYzXJn19MQmnl/g==";

TEST_F(integration_test, signing_test)
{
    crypto c(priv_key);

    database_create_db db_msg;
    db_msg.set_eviction_policy(database_create_db::NONE);
    db_msg.set_max_size(0);

    database_header header;
    header.set_db_uuid(uuid);
    header.set_nonce(1);

    database_msg msg;
    msg.set_allocated_header(new database_header(header));
    msg.set_allocated_create_db(new database_create_db(db_msg));

    bzn_envelope env;
    env.set_sender(pub_key);
    env.set_database_msg(msg.SerializeAsString());

    c.sign(env);
    EXPECT_TRUE(c.verify(env));
}

TEST_F(integration_test, response_test)
{
    sockaddr_in local;
    memset(&local, 0, sizeof(sockaddr_in));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    local.sin_port = 0; //randomly selected port
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (bind(sock, (sockaddr*)&local, sizeof(local)) == -1)
    {
        throw std::runtime_error("unable to bind udp socket");
    }

    int my_id = 0;
    struct sockaddr_in sin;
    socklen_t addrlen = sizeof(sin);
    if (getsockname(sock, (struct sockaddr *)&sin, &addrlen) == 0
        && sin.sin_family == AF_INET && addrlen == sizeof(sin))
    {
        my_id = ntohs(sin.sin_port);
    }
    else
    {
        throw std::runtime_error("error determining local port");
    }

    udp_response resp;
    int their_id = resp.get_signal_id(my_id);
    (void) their_id;
    resp.set_ready();

    char buf[1024];
    auto res = recvfrom(sock, buf, 1024, 0, NULL, 0);
    std::cout << "received: " << res << " bytes" << std::endl;
}