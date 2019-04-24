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
#include <mocks/mock_node_factory.hpp>
#include <mocks/mock_node.hpp>
#include <swarm/swarm.hpp>
#include <node/node_base.hpp>
#include <json/json.h>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <crypto/null_crypto.hpp>

using namespace testing;
using namespace bzapi;

namespace
{
    const std::string SWARM_VERSION{".."};
    const std::string SWARM_GIT_COMMIT{".."};
    const std::string UPTIME{"1:03:01"};
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
        , "ws://127.0.0.1:0", "my_uuid");
    std::vector<std::shared_ptr<mock_node>> mock_nodes;

    std::map<uint16_t, node_meta> nodes;
    uuid_t primary_node;

    void init()
    {
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
        pbft_status["primary"]["uuid"] = this->primary_node;

        Json::Value peer_index;
        for (const auto& p : this->nodes)
        {
            Json::Value peer;
            peer["host"] = "127.0.0.1";
            peer["port"] = p.first;
            peer["uuid"] = "node_" + std::to_string(p.first);
            peer_index.append(peer);
        }

        pbft_status["peer_index"] = peer_index;
        Json::Value module_status;
        module_status["pbft"] = pbft_status;
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

        EXPECT_CALL(*meta.node, send_message(_, _, _)).Times(AtLeast(1))
            .WillRepeatedly(Invoke([this, meta](auto /*msg*/, auto /*len*/, auto callback)
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
                    auto env_str = env.SerializeAsString();
                    ASSERT_NE(node.handler, nullptr);
                    EXPECT_EQ(node.handler(env_str.c_str(), env_str.length()), false);
                });

                boost::system::error_code ec;
                callback(ec);
            }));

        this->nodes[meta.id] = meta;
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

    this->the_swarm->initialize([](auto& /*ec*/){});
    boost::this_thread::sleep_for(boost::chrono::seconds(1));

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

    EXPECT_CALL(*meta.node, send_message(_, _, _)).Times(AtLeast(1))
        .WillRepeatedly(Invoke([&meta](auto /*msg*/, auto /*len*/, auto /*callback*/)
        {
            // schedule status response
            auto node = meta.node;

            bzn_envelope env;
            env.set_status_response("A bunch of garbage");
            auto env_str = env.SerializeAsString();
            EXPECT_EQ(meta.handler(env_str.c_str(), env_str.length()), true);
        }));

    auto l = [](auto& /*ec*/){};
    this->the_swarm->initialize(l);
}
