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


};

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