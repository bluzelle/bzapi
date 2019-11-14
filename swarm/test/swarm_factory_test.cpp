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
#include <mocks/mock_esr.hpp>
#include <crypto/null_crypto.hpp>
#include <swarm/swarm_factory.hpp>
#include <json/json.h>

using namespace testing;
using namespace bzapi;

namespace
{
    const std::string SWARM_ID{"my_swarm"};
}

class swarm_factory_test : public Test
{
public:
    swarm_factory_test()
    {
        the_esr = std::make_shared<mock_esr>();
        mock_ws_factory = std::make_shared<bzn::beast::mock_websocket_base>();
        mock_io_context = std::make_shared<bzn::asio::mock_io_context_base>();
        the_crypto = std::make_shared<null_crypto>();
    }

protected:
    std::shared_ptr<bzn::asio::mock_io_context_base> mock_io_context;
    std::shared_ptr<bzn::beast::mock_websocket_base> mock_ws_factory;
    std::shared_ptr<crypto_base> the_crypto;
    std::shared_ptr<mock_esr> the_esr;
};

TEST_F(swarm_factory_test, test_init_esr)
{
    auto swf = std::make_shared<swarm_factory>(mock_io_context, mock_ws_factory, the_crypto, the_esr, "my_uuid");

    EXPECT_CALL(*the_esr, get_swarm_ids(_)).WillOnce(Invoke([](auto)
    {
        return std::vector<std::string>{"swarm_1", "swarm_2"};
    }));
    EXPECT_CALL(*the_esr, get_peer_ids(_, _)).Times(Exactly(2))
        .WillRepeatedly(Invoke([](auto, auto)
        {
            return std::vector<std::string>{"node_1", "node_2"};
        }));
    EXPECT_CALL(*the_esr, get_peer_info(_, _, _)).Times(Exactly(4))
        .WillRepeatedly(Invoke([](auto, auto, auto)
        {
            static uint16_t id = 1;
            return bzn::peer_address_t{"127.0.0.1", id, 0, "", std::string{"node_"} + std::to_string(id)};
            id++;
        }));

    swf->initialize("address", "url");
}

#if 0

TEST_F(swarm_factory_test, test_create_uuid)
{
    this->add_node(0, 10);
    EXPECT_CALL(*node_factory, create_node(_, _, _, _)).Times(Exactly(1))
        .WillOnce(Invoke([self = this](auto, auto, auto, auto)
        {
            return self->nodes[0].node;
        }));

    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(Exactly(1)).WillOnce(Invoke([]()
    {
        return std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
    })).RetiresOnSaturation();;

    auto& meta = this->nodes[0];
    EXPECT_CALL(*meta.node, send_message(ResultOf(is_status, Eq(false)), _)).Times(Exactly(1))
        .WillRepeatedly(Invoke([this, &meta](auto /*msg*/, auto callback)
        {
            callback(boost::system::error_code{});

            auto node = meta.node;
            bzn_envelope env;
            database_response response;
            env.set_database_response(response.SerializeAsString());
            env.set_sender("node_" + std::to_string(meta.id));
            env.set_swarm_id(SWARM_ID);
            auto env_str = env.SerializeAsString();
            ASSERT_NE(meta.handler, nullptr);
            EXPECT_EQ(meta.handler(env_str), true);
        }));

    this->the_swarm->create_uuid("test_uuid", 0, false, [](db_error res)
    {
        EXPECT_EQ(res, db_error::success);
    });

    EXPECT_TRUE(Mock::VerifyAndClearExpectations(meta.node.get()));
}

#endif
