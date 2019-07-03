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
#include <mocks/mock_node_factory.hpp>
#include <mocks/mock_node.hpp>
#include <swarm/swarm.hpp>
#include <node/node_base.hpp>
#include <json/json.h>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <crypto/null_crypto.hpp>
#include <proto/database.pb.h>

using namespace testing;
using namespace bzapi;

namespace
{
    const std::string SWARM_VERSION{".."};
    const std::string SWARM_GIT_COMMIT{".."};
    const std::string UPTIME{"1:03:01"};
    const std::string SWARM_ID{"my_swarm"};
}

class swarm_test : public Test
{
public:

protected:

    struct node_meta
    {
        uint16_t id{};
        std::shared_ptr<mock_node> node;
        uint64_t latency{};
        node_message_handler handler;
        std::shared_ptr<bzn::asio::steady_timer_base> response_timer;
        completion_handler_t timer_callback;
    };

    std::shared_ptr<bzn::asio::io_context> real_io_context = std::make_shared<bzn::asio::io_context>();
    std::shared_ptr<boost::asio::signal_set> signals = nullptr;
    std::shared_ptr<std::thread> io_thread = nullptr;

    std::shared_ptr<mock_node_factory> node_factory = std::make_shared<mock_node_factory>();
    std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
    std::shared_ptr<crypto_base> crypto = std::make_shared<null_crypto>();
    std::shared_ptr<bzn::beast::websocket_base> ws_factory = std::make_shared<bzn::beast::Mockwebsocket_base>();
    std::shared_ptr<swarm_base> the_swarm = std::make_shared<swarm>(node_factory, ws_factory, mock_io_context, crypto
        , SWARM_ID, "my_uuid");

    std::map<uint16_t, node_meta> nodes;
    bzapi::uuid_t primary_node;

    void init()
    {
        // TODO: replace with work
        this->signals = std::make_shared<boost::asio::signal_set>(this->real_io_context->get_io_context()
            , SIGINT);
        this->signals->async_wait([&](const boost::system::error_code& error, int signal_number)
        {
            if (!error)
            {
                std::cout << "signal received -- shutting down (" << signal_number << ")";
                this->real_io_context->stop();
            }
            else
            {
                std::cout << "Error: " << error.value() << ", " << error.category().name() << std::endl;
            }
        });

        this->io_thread = std::make_shared<std::thread>([&]()
        {
            this->real_io_context->run();
        });
    }

    status_response make_status_response()
    {
        status_response srm;
        srm.set_swarm_version(SWARM_VERSION);
        srm.set_swarm_git_commit(SWARM_GIT_COMMIT);
        srm.set_uptime(UPTIME);

        Json::Value pbft_status;
        pbft_status["status"]["primary"]["uuid"] = this->primary_node;

        Json::Value peer_index;
        for (const auto& p : this->nodes)
        {
            Json::Value peer;
            peer["host"] = "127.0.0.1";
            peer["port"] = p.first;
            peer["uuid"] = "node_" + std::to_string(p.first);
            peer_index.append(peer);
        }

        pbft_status["status"]["peer_index"] = peer_index;
        Json::Value module_status;
        module_status["module"][0] = pbft_status;
        srm.set_module_status_json(module_status.toStyledString());

        return srm;
    }

    void add_node(uint16_t node_id, uint64_t latency)
    {
        node_meta meta;
        meta.id = node_id;
        meta.node = std::make_shared<mock_node>();
        meta.latency = latency;
        meta.response_timer = this->real_io_context->make_unique_steady_timer();

        EXPECT_CALL(*meta.node, register_message_handler(_)).Times(AtLeast(1))
            .WillRepeatedly(Invoke([this, meta](const auto& handler)
            {
                this->nodes[meta.id].handler = handler;
            }));

        EXPECT_CALL(*meta.node, send_message(ResultOf(is_status, Eq(true)), _)).Times(AtLeast(0))
            .WillRepeatedly(Invoke([this, meta](auto /*msg*/, auto callback)
            {
                // schedule status response
                auto node = this->nodes[meta.id];
                node.response_timer->expires_from_now(static_cast<std::chrono::milliseconds>(node.latency));
                node.response_timer->async_wait([this, node](const auto& ec)
                {
                    ASSERT_EQ(ec, boost::system::error_code{});

                    bzn_envelope env;
                    env.set_status_response(this->make_status_response().SerializeAsString());
                    env.set_sender("node_" + std::to_string(node.id));
                    env.set_swarm_id(SWARM_ID);
                    auto env_str = env.SerializeAsString();
                    ASSERT_NE(node.handler, nullptr);
                    EXPECT_EQ(node.handler(env_str), false);
                });

                boost::system::error_code ec;
                callback(ec);
            }));

        this->nodes[meta.id] = meta;
    }

    static bool
    is_status(const std::string& msg)
    {
        bzn_envelope env;
        return (env.ParseFromString(msg) && env.payload_case() == bzn_envelope::kStatusRequest);
    }
};


TEST_F(swarm_test, test_swarm_node_management)
{
    this->init();

    this->add_node(0, 200);
    this->add_node(1, 110);
    this->add_node(2, 100);
    this->add_node(3, 130);
    this->primary_node = "node_1";

    // status timer
    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillRepeatedly(Invoke([this]
    {
        static size_t count = 0;
        size_t n = count;
        count++;

        auto timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();
        EXPECT_CALL(*timer, expires_from_now(_)).Times(AtLeast(1));
        EXPECT_CALL(*timer, async_wait(_)).WillRepeatedly(Invoke([this, n](auto handler)
        {
            this->nodes[n].timer_callback = handler;
        }));
        return timer;
    }));

    EXPECT_CALL(*node_factory, create_node(_, _, _, _)).Times(Exactly(this->nodes.size()))
        .WillRepeatedly(Invoke([self = this](auto, auto, auto, auto)
        {
            static size_t count = 0;
            return self->nodes[count++].node;
        }));

    this->the_swarm->add_nodes({std::make_pair<node_id_t, bzn::peer_address_t>("node_0", {"127.0.0.1", 0, 0, "", ""})});

    std::promise<int> prom;
    this->the_swarm->initialize([&prom](auto& /*ec*/){prom.set_value(1);});
    prom.get_future().get();
    boost::this_thread::sleep_for(boost::chrono::seconds(2));

    auto status_str = this->the_swarm->get_status();
    Json::Value status;
    std::stringstream(status_str) >> status;
    EXPECT_EQ(status["fastest_node"].asString(), "node_2");
    EXPECT_EQ(status["primary_node"].asString(), this->primary_node);
    EXPECT_EQ(status["nodes"].size(), this->nodes.size());

    // remove a node
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(nodes[2].node.get()));
    this->nodes.erase(2);

    // kick off a new status request from swarm
    this->nodes[0].timer_callback(boost::system::error_code{});
    boost::this_thread::sleep_for(boost::chrono::seconds(1));

    // make sure the status has been updated correctly
    status_str = this->the_swarm->get_status();
    Json::Value status2;
    std::stringstream(status_str) >> status2;
    EXPECT_EQ(status2["fastest_node"].asString(), "node_1");
    EXPECT_EQ(status2["primary_node"].asString(), this->primary_node);
    EXPECT_EQ(status2["nodes"].size(), this->nodes.size());

    this->real_io_context->stop();
    this->io_thread->join();

    for (auto& n : this->nodes)
    {
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(n.second.node.get()));
    }
}

TEST_F(swarm_test, test_send_policy)
{
    this->init();

    this->add_node(0, 20);
    this->add_node(1, 110);
    this->primary_node = "node_1";

    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillRepeatedly(Invoke([this]
    {
        static size_t count = 0;
        size_t n = count;
        count++;

        auto timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();
        EXPECT_CALL(*timer, expires_from_now(_)).Times(AtLeast(1));
        EXPECT_CALL(*timer, async_wait(_)).WillRepeatedly(Invoke([this, n](auto handler)
        {
            this->nodes[n].timer_callback = handler;
        }));
        return timer;
    }));

    EXPECT_CALL(*node_factory, create_node(_, _, _, _)).Times(Exactly(this->nodes.size()))
        .WillRepeatedly(Invoke([self = this](auto, auto, auto, auto)
        {
            static size_t count = 0;
            return self->nodes[count++].node;
        }));

    this->the_swarm->add_nodes({std::make_pair<node_id_t, bzn::peer_address_t>("node_0", {"127.0.0.1", 0, 0, "", ""})});

    std::promise<int> prom;
    this->the_swarm->initialize([&prom](auto& /*ec*/){prom.set_value(1);});
    prom.get_future().get();
    boost::this_thread::sleep_for(boost::chrono::seconds(2));

    auto status_str = this->the_swarm->get_status();
    Json::Value status;
    std::stringstream(status_str) >> status;
    EXPECT_EQ(status["fastest_node"].asString(), "node_0");
    EXPECT_EQ(status["primary_node"].asString(), this->primary_node);
    EXPECT_EQ(status["nodes"].size(), this->nodes.size());

    auto env = std::make_shared<bzn_envelope>();
    database_msg request;
    database_header header;
    header.set_db_uuid("my_uuid");
    header.set_nonce(1);
    request.set_allocated_header(new database_header(header));
    request.set_allocated_has_db(new database_has_db());
    env->set_database_msg(request.SerializeAsString());

    auto respond = [](node_meta& meta)
    {
        auto node = meta.node;
        bzn_envelope env;
        database_response response;
        database_has_db_response has_response;
        has_response.set_has(true);
        has_response.set_uuid("my_uuid");
        response.set_allocated_has_db(new database_has_db_response(has_response));
        env.set_database_response(response.SerializeAsString());
        env.set_sender("node_" + std::to_string(meta.id));
        env.set_swarm_id(SWARM_ID);
        auto env_str = env.SerializeAsString();
        ASSERT_NE(meta.handler, nullptr);
        EXPECT_EQ(meta.handler(env_str), false);
    };

    size_t called = 0;
    the_swarm->register_response_handler(bzn_envelope::PayloadCase::kDatabaseResponse,
        [&called](auto&, const auto&)
        {
            called++;
            return false;
        });

    // normal policy - should go through primary
    auto& meta0 = this->nodes[1];
    EXPECT_CALL(*meta0.node, send_message(ResultOf(is_status, Eq(false)), _)).Times(Exactly(1))
        .WillRepeatedly(Invoke([&meta0, respond](auto /*msg*/, auto callback)
        {
            callback(boost::system::error_code{});
            respond(meta0);
        })).RetiresOnSaturation();
    the_swarm->send_request(env, send_policy::normal);
    EXPECT_EQ(called, 1u);

    // fastest policy - should go through fastest node
    auto meta1 = this->nodes[0];
    EXPECT_CALL(*meta1.node, send_message(ResultOf(is_status, Eq(false)), _)).Times(Exactly(1))
        .WillRepeatedly(Invoke([&meta1, respond](auto /*msg*/, auto callback)
        {
            callback(boost::system::error_code{});
            respond(meta1);
        })).RetiresOnSaturation();
    the_swarm->send_request(env, send_policy::fastest);
    EXPECT_EQ(called, 2u);

    // broadcast - should go to both
    auto meta2 = this->nodes[0];
    EXPECT_CALL(*meta2.node, send_message(ResultOf(is_status, Eq(false)), _)).Times(Exactly(1))
        .WillRepeatedly(Invoke([&meta2, respond](auto /*msg*/, auto callback)
        {
            callback(boost::system::error_code{});
            respond(meta2);
        }));
    auto meta3 = this->nodes[1];
    EXPECT_CALL(*meta3.node, send_message(ResultOf(is_status, Eq(false)), _)).Times(Exactly(1))
        .WillRepeatedly(Invoke([&meta3, respond](auto /*msg*/, auto callback)
        {
            callback(boost::system::error_code{});
            respond(meta3);
        }));
    the_swarm->send_request(env, send_policy::broadcast);
    EXPECT_EQ(called, 4u);


    this->real_io_context->stop();
    this->io_thread->join();

    for (auto& n : this->nodes)
    {
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(n.second.node.get()));
    }
}

TEST_F(swarm_test, test_bad_status)
{
    node_meta meta;
    meta.id = 0;
    meta.node = std::make_shared<mock_node>();
    meta.latency = 200;
    meta.response_timer = this->real_io_context->make_unique_steady_timer();

    this->primary_node = "node_0";

    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillRepeatedly(Invoke([] {
        return std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base >>();
    }));

    EXPECT_CALL(*node_factory, create_node(_, _, _, _)).WillRepeatedly(Invoke([&meta](auto, auto, auto, auto) {
        return meta.node;
    }));

    EXPECT_CALL(*meta.node, register_message_handler(_)).Times(AtLeast(1))
        .WillRepeatedly(Invoke([&meta](const auto& handler)
        {
            meta.handler = handler;
        }));

    EXPECT_CALL(*meta.node, send_message(_, _)).Times(AtLeast(1))
        .WillRepeatedly(Invoke([&meta](auto /*msg*/, auto /*callback*/)
        {
            // schedule status response
            auto node = meta.node;

            bzn_envelope env;
            env.set_status_response("A bunch of garbage");
            auto env_str = env.SerializeAsString();
            EXPECT_EQ(meta.handler(env_str), true);
        }));

    auto l = [](auto& /*ec*/){};
    this->the_swarm->add_nodes({std::make_pair<node_id_t, bzn::peer_address_t>("node_0", {"127.0.0.1", 0, 0, "", ""})});
    this->the_swarm->initialize(l);
}
