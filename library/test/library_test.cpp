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

#include <include/bluzelle.hpp>
#include <crypto/crypto.hpp>
#include <crypto/null_crypto.hpp>
#include <database/db_dispatch.hpp>
#include <include/bzapi.hpp>
#include <include/logger.hpp>
#include <library/udp_response.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <mocks/mock_esr.hpp>
#include <swarm/swarm_factory.hpp>

#include <gtest/gtest.h>
#include <json/reader.h>
#include <json/value.h>
#include <random>

using namespace testing;
using namespace bzapi;

namespace bzapi
{
    extern std::shared_ptr<bzn::asio::io_context_base> io_context;
    extern std::shared_ptr<swarm_factory> the_swarm_factory;
    extern std::shared_ptr<crypto_base> the_crypto;
    extern std::shared_ptr<bzn::beast::websocket_base> ws_factory;
    extern std::shared_ptr<bzapi::db_dispatch_base> db_dispatcher;
    extern std::shared_ptr<bzapi::esr_base> the_esr;
    extern bool initialized;

    void init_logging();
    void end_logging();
}

namespace
{
    const std::string SWARM_VERSION{".."};
    const std::string SWARM_GIT_COMMIT{".."};
    const std::string UPTIME{"1:03:01"};

    const char* priv_key = "-----BEGIN EC PRIVATE KEY-----\n"
                           "MHQCAQEEIBWDWE/MAwtXaFQp6d2Glm2Uj7ROBlDKFn5RwqQsDEbyoAcGBSuBBAAK\n"
                           "oUQDQgAEiykQ5A02u+02FR1nftxT5VuUdqLO6lvNoL5aAIyHvn8NS0wgXxbPfpuq\n"
                           "UPpytiopiS5D+t2cYzXJn19MQmnl/g==\n"
                           "-----END EC PRIVATE KEY-----";

    const char* pub_key = "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEiykQ5A02u+02FR1nftxT5VuUdqLO6lvN\n"
                          "oL5aAIyHvn8NS0wgXxbPfpuqUPpytiopiS5D+t2cYzXJn19MQmnl/g==";

}

// this is kinda ugly, but we need to identify the node somehow
std::map<uint16_t, boost::asio::ip::tcp::socket*> sock_map;
boost::asio::io_context real_io_context;

uint16_t get_node_from_sock(const boost::asio::ip::tcp::socket& sock)
{
    for (auto& n : sock_map)
    {
        if (n.second == (&sock))
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
    boost::asio::ip::tcp::socket socket{real_io_context};
};

struct mock_websocket
{
    mock_websocket(uint16_t id = 0)
    : id(id) {}

    std::unique_ptr<bzn::beast::Mockwebsocket_stream_base> get(bool close = false)
    {
        auto websocket = std::make_unique<bzn::beast::Mockwebsocket_stream_base>();

        EXPECT_CALL(*websocket, async_handshake(_, _, _)).WillOnce(Invoke([](auto, auto, auto lambda)
        {
            lambda(boost::system::error_code{});
        }));

        EXPECT_CALL(*websocket, binary(_)).Times(AtLeast(1));

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

        if (close)
        {
            EXPECT_CALL(*websocket, is_open()).Times(Exactly(1)).WillOnce(Return(false));
        }

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

class my_logger : public bzapi::logger
{
    void log(const std::string& severity, const std::string& message)
    {
        std::cout << severity << ": " << message << std::endl;
    }
};

class integration_test : public Test
{
public:
    void initialize(bzapi::uuid_t _uuid)
    {
        bzapi::init_logging();
        bzapi::set_logger(&mylogger);

        this->uuid = _uuid;
        mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        EXPECT_CALL(*mock_io_context, get_io_context()).Times(AtLeast(1)).WillRepeatedly(ReturnRef(real_io_context));

        io_context = mock_io_context;
        db_dispatcher = std::make_shared<db_dispatch>(io_context);
        the_crypto = std::make_shared<null_crypto>();
        mock_ws_factory = std::make_shared<bzn::beast::Mockwebsocket_base>();
        ws_factory = mock_ws_factory;
        auto esr = std::make_shared<mock_esr>();
        the_swarm_factory = std::make_shared<swarm_factory>(mock_io_context, ws_factory, the_crypto, esr, this->uuid);
        std::vector<std::pair<node_id_t, bzn::peer_address_t>> addrs;
        addrs.push_back(std::make_pair(node_id_t{"node_0"}, bzn::peer_address_t{"127.0.0.1", 50000, 0, "node_0", "node_0"}));
        addrs.push_back(std::make_pair(node_id_t{"node_1"}, bzn::peer_address_t{"127.0.0.1", 50001, 0, "node_1", "node_1"}));
        addrs.push_back(std::make_pair(node_id_t{"node_2"}, bzn::peer_address_t{"127.0.0.1", 50002, 0, "node_2", "node_2"}));
        addrs.push_back(std::make_pair(node_id_t{"node_3"}, bzn::peer_address_t{"127.0.0.1", 50003, 0, "node_3", "node_3"}));
        the_swarm_factory->initialize("swarm_id", addrs);
        this->swarm_size = addrs.size();
        bzapi::initialized = true;
    }

    void teardown()
    {
        the_swarm_factory = nullptr;
        db_dispatcher = nullptr;
        io_context = nullptr;
        ws_factory = nullptr;
        this->node_websocks.clear();
        bzapi::end_logging();
        bzapi::initialized = false;
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
        for (const auto& p : this->node_websocks)
        {
            Json::Value peer;
            peer["host"] = "127.0.0.1";
            peer["port"] = 50000 + p.id;
            peer["uuid"] = "node_" + std::to_string(p.id);
            peer_index.append(peer);
        }

        pbft_status["peer_index"] = peer_index;
        Json::Value module_status;
        module_status["module"][0]["status"] = pbft_status;
        srm.set_module_status_json(module_status.toStyledString());

        return srm;
    }

    void expect_has_db(bool value = true)
    {
        // status timer for each node, plus client timeout, plus request timeout
        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(Exactly(swarm_size + 2)).WillRepeatedly(Invoke([]()
        {
            return std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
        })).RetiresOnSaturation();;

        EXPECT_CALL(*mock_io_context, make_unique_tcp_socket()).Times(Exactly(1)).WillOnce(Invoke([]()
        {
            auto tcp_sock = std::make_unique<bzn::asio::Mocktcp_socket_base>();

            EXPECT_CALL(*tcp_sock, async_connect(_, _)).Times(AtLeast(1))
                .WillRepeatedly(Invoke([](auto, auto callback)
                {
                    callback(boost::system::error_code{});
                }));

            static boost::asio::ip::tcp::socket socket{real_io_context};
            EXPECT_CALL(*tcp_sock, get_tcp_socket()).Times(AtLeast(1))
                .WillRepeatedly(ReturnRef(socket));

            return tcp_sock;
        })).RetiresOnSaturation();

        EXPECT_CALL(*mock_ws_factory, make_unique_websocket_stream(_)).Times(Exactly(1)).WillOnce(
            Invoke([uuid = this->uuid, value, num_nodes = swarm_size, &ws = this->node_websocks[0]](auto&)
        {
            ws.write_func = [uuid](const boost::asio::mutable_buffers_1& buffer)
            {
                bzn_envelope env;
                EXPECT_TRUE(env.ParseFromString(std::string(static_cast<const char *>(buffer.data()), buffer.size())));

                database_msg db_msg;
                EXPECT_TRUE(db_msg.ParseFromString(env.database_msg()));

                EXPECT_TRUE(db_msg.has_has_db());
                EXPECT_TRUE(db_msg.header().db_uuid() == uuid);
            };

            ws.read_func = [uuid, value, num_nodes, &ws](const auto& /*buffer*/)
            {
                database_has_db_response has_db;
                has_db.set_uuid(uuid);
                has_db.set_has(value);

                database_header header;
                header.set_nonce(1);
                database_response response;
                response.set_allocated_has_db(new database_has_db_response(has_db));
                response.set_allocated_header(new database_header(header));

                bzn_envelope env2;
                env2.set_database_response(response.SerializeAsString());
                env2.set_signature("xxx");
                for (size_t i = 0; i < num_nodes; i++)
                {
                    env2.set_sender(std::string{"node_"} + std::to_string(i));
                    auto message = env2.SerializeAsString();
                    boost::asio::buffer_copy(ws.read_buffer->prepare(message.size()), boost::asio::buffer(message));
                    ws.read_buffer->commit(message.size());
                    ws.read_handler(boost::system::error_code{}, message.size());
                }
            };

            return ws.get();
        })).RetiresOnSaturation();
    }

    void expect_create_db(bool succeed = true)
    {
        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(Exactly(2)).WillOnce(Invoke([]()
        {
            return std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
        })).WillOnce(Invoke([this, succeed]()
        {
            static int nonce  = 0;
            static int node_id = -1;

            for (size_t i = 0; i < 4; i++)
            {
                this->node_websocks[i].write_func = [this, i](const boost::asio::mutable_buffers_1& buffer)
                {
                    try
                    {
                        // remember which node sent us the request
                        node_id = i;

                        bzn_envelope env;
                        EXPECT_TRUE(
                            env.ParseFromString(std::string(static_cast<const char *>(buffer.data()), buffer.size())));
                        database_msg msg;
                        EXPECT_TRUE(msg.ParseFromString(env.database_msg()));
                        EXPECT_EQ(msg.header().db_uuid(), this->uuid);
                        nonce = msg.header().nonce();
                        EXPECT_TRUE(msg.has_create_db());
                    }
                    CATCHALL();
                };

                this->node_websocks[i].read_func = [this, succeed](const auto& /*buffer*/)
                {
                    try
                    {
                        for (size_t j = 0; j < 4; j++)
                        {
                            database_header header;
                            header.set_nonce(nonce);
                            database_response dr;
                            dr.set_allocated_header(new database_header(header));

                            bzn_envelope env2;
                            if (!succeed)
                            {
                                dr.mutable_error()->set_message("ACCESS_DENIED");
                            }

                            env2.set_database_response(dr.SerializeAsString());
                            env2.set_sender("node_" + std::to_string(j));
                            env2.set_signature("xxx");
                            auto message = env2.SerializeAsString();
                            this->node_websocks[node_id].simulate_read(message);
                        }
                    }
                    CATCHALL();
                };
            }

            return std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
        }))
        .RetiresOnSaturation();;
    }

    void expect_swarm_initialize()
    {
        uint16_t node_count = 0;

        EXPECT_CALL(*mock_io_context, make_unique_tcp_socket()).Times(Exactly(swarm_size - 1))
            .WillRepeatedly(Invoke([]()
        {
            auto tcp_sock = std::make_unique<bzn::asio::Mocktcp_socket_base>();

            EXPECT_CALL(*tcp_sock, async_connect(_, _)).Times(AtLeast(1))
                .WillRepeatedly(Invoke([](auto, auto callback)
                {
                    callback(boost::system::error_code{});
                }));

            static boost::asio::ip::tcp::socket socket{real_io_context};
            EXPECT_CALL(*tcp_sock, get_tcp_socket()).Times(AtLeast(1))
                .WillRepeatedly(ReturnRef(socket));

            return tcp_sock;
        }));

        EXPECT_CALL(*mock_ws_factory, make_unique_websocket_stream(_))
            .WillRepeatedly(Invoke([&node_count, uuid = this->uuid, this](auto &/*sock*/)
        {
            auto node_id = node_count++;
            mock_websocket &ws = this->node_websocks[node_id];

            ws.write_func = [uuid](const boost::asio::mutable_buffers_1 &buffer)
            {
                bzn_envelope env;
                EXPECT_TRUE(
                    env.ParseFromString(
                        std::string(static_cast<const char *>(buffer.data()), buffer.size())));

                status_request sr;
                EXPECT_TRUE(sr.ParseFromString(env.status_request()));
            };

            ws.read_func = [this, node_id, &ws](const auto& /*buffer*/)
            {
                status_response sr = this->make_status_response();
                bzn_envelope env;
                env.set_status_response(sr.SerializeAsString());
                env.set_sender("node_" + std::to_string(node_id));
                env.set_signature("xxx");
                auto message = env.SerializeAsString();
                boost::asio::buffer_copy(ws.read_buffer->prepare(message.size()), boost::asio::buffer(
                    message));
                ws.read_buffer->commit(message.size());
                ws.read_handler(boost::system::error_code{}, message.size());
            };

            return ws.get();
        }));
    }

    uint32_t generate_random_number(uint32_t min, uint32_t max)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dist(min, max);
        return dist(gen);
    }

    int create_socket(uint16_t& port)
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

        struct sockaddr_in sin;
        socklen_t addrlen = sizeof(sin);
        if (getsockname(sock, (struct sockaddr *)&sin, &addrlen) == 0
            && sin.sin_family == AF_INET && addrlen == sizeof(sin))
        {
            port = ntohs(sin.sin_port);
            return sock;
        }
        else
        {
            throw std::runtime_error("error determining local port");
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

    bzapi::uuid_t uuid;
    bzapi::uuid_t primary_node;
    std::vector<mock_websocket> node_websocks;
    std::set<my_mock_tcp_socket*> sockets;
    std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context;
    std::shared_ptr<bzn::beast::Mockwebsocket_base> mock_ws_factory;
    my_logger mylogger;
    size_t swarm_size;
};

TEST_F(integration_test, test_uninitialized)
{
    EXPECT_EQ(bzapi::async_has_db("test_uuid"), nullptr);
    EXPECT_EQ(bzapi::get_error(), -1);

    EXPECT_EQ(bzapi::async_create_db("test_uuid", 0, false), nullptr);
    EXPECT_EQ(bzapi::get_error(), -1);

    EXPECT_EQ(bzapi::async_open_db("test_uuid"), nullptr);
    EXPECT_EQ(bzapi::get_error(), -1);

    EXPECT_EQ(bzapi::has_db("test_uuid"), false);
    EXPECT_EQ(bzapi::get_error(), -1);

    EXPECT_EQ(bzapi::create_db("test_uuid", 0, false), nullptr);
    EXPECT_EQ(bzapi::get_error(), -1);

    EXPECT_EQ(bzapi::open_db("test_uuid"), nullptr);
    EXPECT_EQ(bzapi::get_error(), -1);
}

TEST_F(integration_test, test_initialize)
{
    EXPECT_EQ(bzapi::get_error(), -1);
    EXPECT_EQ(bzapi::get_error_str(), std::string{"Not Initialized"});

    bool result = bzapi::initialize(pub_key, priv_key, "ws://127.0.0.1:50000", "", "");
    EXPECT_TRUE(result);
    EXPECT_EQ(bzapi::get_error(), 0);
    EXPECT_EQ(bzapi::get_error_str(), std::string{""});

    bzapi::terminate();
    EXPECT_EQ(bzapi::get_error(), -1);
    EXPECT_EQ(bzapi::get_error_str(), std::string{"Not Initialized"});
}

TEST_F(integration_test, test_initialize_fails_with_bad_endpoint)
{
    EXPECT_EQ(bzapi::get_error(), -1);
    EXPECT_EQ(bzapi::get_error_str(), std::string{"Not Initialized"});

    bool result = bzapi::initialize(pub_key, priv_key, "127.0.0.1:50000", "", "");
    EXPECT_FALSE(result);
    EXPECT_EQ(bzapi::get_error(), -1);
    EXPECT_EQ(bzapi::get_error_str(), std::string{"Bad Endpoint"});

    result = bzapi::initialize(pub_key, priv_key, "ws://127.0.0.1", "", "");
    EXPECT_FALSE(result);
    EXPECT_EQ(bzapi::get_error(), -1);
    EXPECT_EQ(bzapi::get_error_str(), std::string{"Bad Endpoint"});
}

TEST_F(integration_test, test_initialize_esr)
{
    auto esr = std::make_shared<mock_esr>();
    bzapi::the_esr = esr;
    EXPECT_CALL(*esr, get_swarm_ids(_, _)).WillOnce(Invoke([](auto, auto)
    {
        return std::vector<std::string>{"swarm_1", "swarm_2"};
    }));
    EXPECT_CALL(*esr, get_peer_ids(_, _, _)).Times(Exactly(2))
        .WillRepeatedly(Invoke([](auto, auto, auto)
    {
        return std::vector<std::string>{"node_1", "node_2"};
    }));
    EXPECT_CALL(*esr, get_peer_info(_, _, _, _)).Times(Exactly(4))
        .WillRepeatedly(Invoke([](auto, auto, auto, auto)
        {
            static uint16_t id = 1;
            return bzn::peer_address_t{"127.0.0.1", id, 0, "", std::string{"node_"} + std::to_string(id)};
            id++;
        }));

    bool result = bzapi::initialize(pub_key, priv_key, "address", "url");
    EXPECT_TRUE(result);
    EXPECT_EQ(bzapi::get_error(), 0);
    EXPECT_EQ(bzapi::get_error_str(), std::string{""});

    bzapi::terminate();
    EXPECT_EQ(bzapi::get_error(), -1);
    EXPECT_EQ(bzapi::get_error_str(), std::string{"Not Initialized"});

    bzapi::the_esr = nullptr;
}

TEST_F(integration_test, test_has_db)
{
    bzapi::uuid_t uuid{"my_uuid"};
    this->initialize(uuid);

    for (size_t i = 0; i < 4; i++)
    {
        this->node_websocks.push_back(mock_websocket{static_cast<uint16_t>(i)});
    }
    this->primary_node = "node_0";
    expect_has_db();

    bool result = has_db(uuid);
    EXPECT_EQ(result, true);

    this->teardown();
}

TEST_F(integration_test, test_open_db)
{
    bzapi::uuid_t uuid{"my_uuid"};
    this->initialize(uuid);

    for (size_t i = 0; i < 4; i++)
    {
        this->node_websocks.push_back(mock_websocket{static_cast<uint16_t>(i)});
    }
    this->primary_node = "node_0";

    // reverse order is intentional to match most recent expectations first
    expect_swarm_initialize();
    expect_has_db();

    auto db = open_db(uuid);
    EXPECT_NE(db, nullptr);

    auto status_str = db->swarm_status();
    Json::Value status;
    std::stringstream(status_str) >> status;
    EXPECT_EQ(status["primary_node"].asString(), this->primary_node);
    EXPECT_EQ(status["nodes"].size(), this->node_websocks.size());

    this->teardown();
}

TEST_F(integration_test, test_create_db)
{
    bzapi::uuid_t uuid{"my_uuid"};
    this->initialize(uuid);

    for (size_t i = 0; i < 4; i++)
    {
        this->node_websocks.push_back(mock_websocket{static_cast<uint16_t>(i)});
    }
    this->primary_node = "node_0";

    // reverse order is intentional to match most recent expectations first
    expect_swarm_initialize();
    expect_create_db();
    expect_has_db(false);


    auto db = create_db(uuid, 0, false);
    EXPECT_NE(db, nullptr);

    this->teardown();
}

TEST_F(integration_test, test_cant_create_db)
{
    size_t MAX_RETRY{5}; // set in swarm_factory.cpp
    bzapi::uuid_t uuid{"my_uuid"};
    this->initialize(uuid);

    for (size_t i = 0; i < 4; i++)
    {
        this->node_websocks.push_back(mock_websocket{static_cast<uint16_t>(i)});
    }
    this->primary_node = "node_0";

    // reverse order is intentional to match most recent expectations first
    for (size_t i = 0; i < MAX_RETRY + 1; i++)
    {
        expect_create_db(false);
    }
    expect_has_db(false);


    auto db = create_db(uuid, 0, false);
    EXPECT_EQ(db, nullptr);

    this->teardown();
}

TEST_F(integration_test, test_create)
{
    bzapi::uuid_t uuid{"my_uuid"};
    this->initialize(uuid);

    for (size_t i = 0; i < 4; i++)
    {
        this->node_websocks.push_back(mock_websocket{static_cast<uint16_t>(i)});
    }
    this->primary_node = "node_0";

    // reverse order is intentional to match most recent expectations first
    expect_swarm_initialize();
    expect_has_db();

    auto response = async_open_db(uuid);
    response->set_signal_id(100);
    auto db = response->get_db();
    ASSERT_TRUE(db != nullptr);

    int nonce  = 0;
    int node_id = -1;

    for (size_t i = 0; i < 4; i++)
    {
        this->node_websocks[i].write_func = [&nonce, &node_id, i](const boost::asio::mutable_buffers_1& buffer)
        {
            // remember which node sent us the request
            node_id = i;

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

    // once for client timeout, once for request timeout
    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(Exactly(2)).WillRepeatedly(Invoke([]()
    {
        return std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
    })).RetiresOnSaturation();;

    auto create_response = db->create("test_key", "test_value", 0);
    create_response->set_signal_id(100);

    ASSERT_TRUE(node_id >= 0);

    for (size_t i = 0; i < 4; i++)
    {
        database_header header;
        header.set_nonce(nonce);
        database_response dr;
        dr.set_allocated_header(new database_header(header));

        bzn_envelope env2;
        env2.set_database_response(dr.SerializeAsString());
        env2.set_sender("node_" + std::to_string(i));
        env2.set_signature("xxx");
        auto message = env2.SerializeAsString();
        this->node_websocks[node_id].simulate_read(message);
    }

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
    bzapi::uuid_t uuid{"my_uuid"};
    this->initialize(uuid);

    for (size_t i = 0; i < 4; i++)
    {
        this->node_websocks.push_back(mock_websocket{static_cast<uint16_t>(i)});
    }
    this->primary_node = "node_0";

    // reverse order is intentional to match most recent expectations first
    expect_swarm_initialize();
    expect_has_db();

    auto response = async_open_db(uuid);
    response->set_signal_id(100);
    auto db = response->get_db();
    ASSERT_TRUE(db != nullptr);

    int nonce  = 0;
    int node_id = -1;

    for (size_t i = 0; i < 4; i++)
    {
        this->node_websocks[i].write_func = [&nonce, &node_id, i](const boost::asio::mutable_buffers_1& buffer)
        {
            node_id = i;

            bzn_envelope env;
            EXPECT_TRUE(env.ParseFromString(std::string(static_cast<const char *>(buffer.data()), buffer.size())));

            database_msg msg;
            EXPECT_TRUE(msg.ParseFromString(env.database_msg()));
            nonce = msg.header().nonce();

            const database_read& read = msg.read();
            EXPECT_TRUE(read.key() == "test_key");
        };
    }

    // once for client timeout, once for request timeout
    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(Exactly(2)).WillRepeatedly(Invoke([]()
    {
        return std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
    })).RetiresOnSaturation();;

    auto create_response = db->read("test_key");
    create_response->set_signal_id(100);

    ASSERT_TRUE(node_id >= 0);

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
        env2.set_signature("xxx");
        auto message = env2.SerializeAsString();
        this->node_websocks[node_id].simulate_read(message);
    }

    auto resp = create_response->get_result();
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(resp, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
    EXPECT_EQ(resp_json["key"].asString(), "test_key");
    EXPECT_EQ(resp_json["value"].asString(), "test_value");
    EXPECT_EQ(create_response->get_db(), nullptr);

    this->node_websocks.clear();
    Mock::VerifyAndClearExpectations(mock_io_context.get());
    this->teardown();
}

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
    uint16_t my_id = 0;
    int sock = create_socket(my_id);

    udp_response resp;
    int their_id = resp.set_signal_id(my_id);
    (void) their_id;
    resp.set_ready();

    char buf[1024];
    auto res = recvfrom(sock, buf, 1024, 0, NULL, 0);
    std::cout << "received: " << res << " bytes" << std::endl;
}

TEST_F(integration_test, blocking_response_test)
{
    udp_response resp;

    std::thread thr([&resp]()
    {
        sleep(2);
        resp.set_result("done");
        resp.set_ready();
    });

    EXPECT_EQ(resp.get_result(), std::string("done"));
    thr.join();
}

#if 0 // these tests need to be run manually with an active swarm
      // and need the node id to be set below to the first swarm member

namespace
{
    std::string NODE_ID{"MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEoqaq8sFA3tnGXBySuET0IdLgKXvmz+uCA2FL/DiTPOTpM3q/9kIscZ1I+Ryh2W+xpfFVkp2m9d6ZIp/XjQcHwg=="};
}

TEST_F(integration_test, live_test)
{
    uint16_t my_id = 0;
    int sock = create_socket(my_id);
    char buf[1024];

    auto rand = generate_random_number(0, 100000);
    std::string db_name = "testdb_" + std::to_string(rand);

    bzapi::set_logger(&mylogger);
    bool res = bzapi::initialize(pub_key, priv_key, "ws://localhost:50000", NODE_ID, "my_swarm");
//    bool res = bzapi::initialize(pub_key, priv_key, "ws://127.0.0.1:50000");
//    bool res = bzapi::initialize(pub_key, priv_key, "ws://75.96.163.85:51010");
    EXPECT_TRUE(res);

    auto resp = bzapi::async_create_db(db_name.data(), 0, false);
    resp->set_signal_id(my_id);
    recvfrom(sock, buf, 1024, 0, NULL, 0);

    Json::Value res_json;
    std::stringstream(resp->get_result()) >> res_json;
    auto result = res_json["result"].asInt();
    auto db_uuid = res_json["uuid"].asString();
    EXPECT_EQ(result, 1);
    EXPECT_EQ(db_uuid, db_name);

    auto db = resp->get_db();
    ASSERT_NE(db, nullptr);

    auto create_resp = db->create("test_key", "test_value", 0);
    create_resp->set_signal_id(my_id);
    recvfrom(sock, buf, 1024, 0, NULL, 0);

    Json::Value create_json;
    std::stringstream(create_resp->get_result()) >> create_json;
    EXPECT_EQ(create_json["result"].asInt(), 1);


//    auto exp_resp = db->expire("test_key", 5);
//    resp->set_signal_id(my_id);
//    recvfrom(sock, buf, 1024, 0, NULL, 0);
//
//    Json::Value exp_json;
//    std::stringstream(exp_resp->get_result()) >> exp_json;
//    EXPECT_EQ(exp_json["result"].asInt(), 1);

    auto read_resp = db->read("test_key");
    read_resp->set_signal_id(my_id);
    recvfrom(sock, buf, 1024, 0, NULL, 0);

    Json::Value read_json;
    std::stringstream(read_resp->get_result()) >> read_json;
    EXPECT_EQ(read_json["result"].asInt(), 1);
    EXPECT_TRUE(read_json["value"].asString() == "test_value");

    auto update_resp = db->update("test_key", "test_value2");
    update_resp->set_signal_id(my_id);
    recvfrom(sock, buf, 1024, 0, NULL, 0);

    Json::Value update_json;
    std::stringstream(update_resp->get_result()) >> update_json;
    EXPECT_EQ(update_json["result"].asInt(), 1);


    auto qread_resp = db->quick_read("test_key");
    qread_resp->set_signal_id(my_id);
    recvfrom(sock, buf, 1024, 0, NULL, 0);

    Json::Value qread_json;
    std::stringstream(qread_resp->get_result()) >> qread_json;
    EXPECT_EQ(qread_json["result"].asInt(), 1);
    EXPECT_TRUE(qread_json["value"].asString() == "test_value2");


    auto remove_resp = db->remove("test_key");
    remove_resp->set_signal_id(my_id);
    recvfrom(sock, buf, 1024, 0, NULL, 0);

    Json::Value remove_json;
    std::stringstream(remove_resp->get_result()) >> remove_json;
    EXPECT_EQ(read_json["result"].asInt(), 1);

    auto has_resp = db->has("test_key");
    has_resp->set_signal_id(my_id);
    recvfrom(sock, buf, 1024, 0, NULL, 0);

    Json::Value has_json;
    std::stringstream(has_resp->get_result()) >> has_json;
    EXPECT_EQ(has_json["result"].asInt(), 0);

    auto status = db->swarm_status();
    std::cout << status << std::endl;

    bzapi::terminate();
}

TEST_F(integration_test, blocking_live_test)
{
    auto rand = generate_random_number(0, 100000);
    std::string db_name = "testdb_" + std::to_string(rand);

    bool res = bzapi::initialize(pub_key, priv_key, "ws://localhost:50000", NODE_ID, "my_swarm");
//    bool res = bzapi::initialize(pub_key, priv_key, "ws://127.0.0.1:50000");
//    bool res = bzapi::initialize(pub_key, priv_key, "ws://75.96.163.85:51010");
    EXPECT_TRUE(res);

    auto resp = bzapi::async_create_db(db_name.data(), 0, false);
    Json::Value res_json;
    std::stringstream(resp->get_result()) >> res_json;
    auto result = res_json["result"].asInt();
    auto db_uuid = res_json["uuid"].asString();
    EXPECT_EQ(result, 1);
    EXPECT_EQ(db_uuid, db_name);

    auto db = resp->get_db();
    ASSERT_NE(db, nullptr);

    auto create_resp = db->create("test_key", "test_value", 0);
    Json::Value create_json;
    std::stringstream(create_resp->get_result()) >> create_json;
    EXPECT_EQ(create_json["result"].asInt(), 1);

//    auto exp_resp = db->expire("test_key", 5);
//    Json::Value exp_json;
//    std::stringstream(exp_resp->get_result()) >> exp_json;
//    EXPECT_EQ(exp_json["result"].asInt(), 1);

    auto read_resp = db->read("test_key");
    Json::Value read_json;
    std::stringstream(read_resp->get_result()) >> read_json;
    EXPECT_EQ(read_json["result"].asInt(), 1);
    EXPECT_TRUE(read_json["value"].asString() == "test_value");

    auto update_resp = db->update("test_key", "test_value2");
    Json::Value update_json;
    std::stringstream(update_resp->get_result()) >> update_json;
    EXPECT_EQ(update_json["result"].asInt(), 1);


    auto qread_resp = db->quick_read("test_key");
    Json::Value qread_json;
    std::stringstream(qread_resp->get_result()) >> qread_json;
    EXPECT_EQ(qread_json["result"].asInt(), 1);
    EXPECT_TRUE(qread_json["value"].asString() == "test_value2");


    auto remove_resp = db->remove("test_key");
    Json::Value remove_json;
    std::stringstream(remove_resp->get_result()) >> remove_json;
    EXPECT_EQ(read_json["result"].asInt(), 1);

    auto has_resp = db->has("test_key");
    Json::Value has_json;
    std::stringstream(has_resp->get_result()) >> has_json;
    EXPECT_EQ(has_json["result"].asInt(), 0);

    auto status = db->swarm_status();
    std::cout << status << std::endl;

    bzapi::terminate();
}

TEST_F(integration_test, sync_live_test)
{
    bzapi::set_timeout(10000);
    auto rand = generate_random_number(0, 100000);
    std::string db_name = "testdb_" + std::to_string(rand);

    bool res = bzapi::initialize(pub_key, priv_key, "ws://localhost:50000", NODE_ID, "my_swarm");
    bzapi::set_logger(&mylogger);

//    bool res = bzapi::initialize(pub_key, priv_key, "ws://54.215.183.100:51010", "testnet-dev");
//    bool res = bzapi::initialize(pub_key, priv_key, "ws://13.57.48.65:51010", "testnet-dev");
//    bool res = bzapi::initialize(pub_key, priv_key, "ws://13.56.191.86:50000", NODE_ID, "my_swarm");
//    bool res = bzapi::initialize(pub_key, priv_key, "ws://54.193.36.192:51010", "testnet-dev");



    //    bool res = bzapi::initialize(pub_key, priv_key, "ws://127.0.0.1:50000");
//    bool res = bzapi::initialize(pub_key, priv_key, "ws://75.96.163.85:51010");
    EXPECT_TRUE(res);

    auto db = bzapi::create_db(db_name.data(), 0, false);
    ASSERT_NE(db, nullptr);

    auto create_resp = db->create("test_key", "test_value", 0);
    Json::Value create_json;
    std::stringstream(create_resp) >> create_json;
    EXPECT_EQ(create_json["result"].asInt(), 1);

//    auto exp_resp = db->expire("test_key", 5);
//    Json::Value exp_json;
//    std::stringstream(exp_resp) >> exp_json;
//    EXPECT_EQ(exp_json["result"].asInt(), 1);

    auto read_resp = db->read("test_key");
    Json::Value read_json;
    std::stringstream(read_resp) >> read_json;
    EXPECT_EQ(read_json["result"].asInt(), 1);
    EXPECT_TRUE(read_json["value"].asString() == "test_value");

    auto update_resp = db->update("test_key", "test_value2");
    Json::Value update_json;
    std::stringstream(update_resp) >> update_json;
    EXPECT_EQ(update_json["result"].asInt(), 1);


    auto qread_resp = db->quick_read("test_key");
    Json::Value qread_json;
    std::stringstream(qread_resp) >> qread_json;
    EXPECT_EQ(qread_json["result"].asInt(), 1);
    EXPECT_TRUE(qread_json["value"].asString() == "test_value2");


    auto remove_resp = db->remove("test_key");
    Json::Value remove_json;
    std::stringstream(remove_resp) >> remove_json;
    EXPECT_EQ(read_json["result"].asInt(), 1);

    auto has_resp = db->has("test_key");
    Json::Value has_json;
    std::stringstream(has_resp) >> has_json;
    EXPECT_EQ(has_json["result"].asInt(), 0);

    auto status = db->swarm_status();
    std::cout << status << std::endl;

    bzapi::terminate();
}

#include <utils/esr_peer_info.hpp>

const std::string DEFAULT_SWARM_INFO_ESR_ADDRESS{"D5B3d7C061F817ab05aF9Fab3b61EEe036e4f4fc"};
const std::string ROPSTEN_URL{"https://ropsten.infura.io"};

TEST_F(integration_test, get_swarms_test)
{
    auto swarms = bzn::utils::esr::get_swarm_ids(DEFAULT_SWARM_INFO_ESR_ADDRESS, ROPSTEN_URL);
    for (auto sw : swarms)
    {
        if (sw.empty())
        {
            break;
        }

        std::cout << sw.size() << ": " << sw << std::endl;
    }
    EXPECT_TRUE(swarms.size() > 0);
}

#endif // these tests need to be run manually with an active swarm
