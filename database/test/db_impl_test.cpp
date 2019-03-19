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
#include <mocks/mock_swarm.hpp>
#include <json/json.h>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <database/db_impl.hpp>

using namespace testing;
using namespace bzapi;

namespace
{
}

class db_impl_test : public Test
{
public:

protected:
    std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context  = std::make_shared<bzn::asio::Mockio_context_base>();
    std::shared_ptr<mock_swarm> swarm = std::make_shared<mock_swarm>();
    std::shared_ptr<db_impl> db = std::make_shared<db_impl>(mock_io_context, swarm, "my_db_uuid");

};


TEST_F(db_impl_test, basic_test)
{
    swarm_response_handler_t swarm_response_handler;
    completion_handler_t timer_callback;

    EXPECT_CALL(*swarm, initialize(_));
    EXPECT_CALL(*swarm, register_response_handler(_, _)).WillOnce(Invoke([&](auto, auto handler)
    {
        swarm_response_handler = handler;
        return true;
    }));
    db->initialize([](auto){});

    EXPECT_CALL(*swarm, send_request(_, _)).Times(Exactly(1)).WillOnce(Invoke([&](auto e, auto)
    {
        bzn_envelope env;
        database_msg request;
        EXPECT_TRUE(request.ParseFromString(e->database_msg()));
        database_response response;
        *response.mutable_header() = request.header();
        env.set_database_response(response.SerializeAsString());
        env.set_sender("node1");
        swarm_response_handler("node1", env);
        return 0;
    }));

    database_msg request;
    bool called = false;
    db->send_message_to_swarm(request, send_policy::fastest, [&](const auto& /*response*/, const auto& /*ec*/)
    {
        called = true;
    });

    EXPECT_TRUE(called);
}

TEST_F(db_impl_test, collation_and_timeout_test)
{
    swarm_response_handler_t swarm_response_handler;
    completion_handler_t timer_callback;

    EXPECT_CALL(*swarm, initialize(_));
    EXPECT_CALL(*swarm, register_response_handler(_, _)).WillOnce(Invoke([&](auto, auto handler)
    {
        swarm_response_handler = handler;
        return true;
    }));
    db->initialize([](auto){});

    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(AtLeast(1)).WillRepeatedly(Invoke([&]
    {
        auto timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();
        EXPECT_CALL(*timer, expires_from_now(_)).Times(AtLeast(1));
        EXPECT_CALL(*timer, async_wait(_)).WillRepeatedly(Invoke([&](auto handler)
        {
            timer_callback = handler;
        }));
        return timer;
    }));

    EXPECT_CALL(*swarm, honest_majority_size()).Times(Exactly(1)).WillOnce(Return(3));

    bool broadcasted = false;
    std::shared_ptr<bzn_envelope> envelope;
    EXPECT_CALL(*swarm, send_request(_, send_policy::normal)).Times(Exactly(1)).WillOnce(Invoke([&](auto, auto)
    {
        EXPECT_CALL(*swarm, send_request(_, send_policy::broadcast)).Times(Exactly(1)).WillOnce(Invoke([&](auto e, auto)
        {
            envelope = e;
            broadcasted = true;
            return 0;
        }));

        timer_callback(boost::system::error_code{});
        return 0;
    }));

    database_msg request;
    bool called = false;
    db->send_message_to_swarm(request, send_policy::normal, [&](const auto& /*response*/, const auto& /*ec*/)
    {
        ASSERT_EQ(called, false);
        called = true;
    });

    ASSERT_FALSE(called);
    ASSERT_TRUE(broadcasted);

    bzn_envelope env;
    ASSERT_TRUE(request.ParseFromString(envelope->database_msg()));
    database_response response;
    *response.mutable_header() = request.header();
    env.set_database_response(response.SerializeAsString());

    env.set_sender("node1");
    swarm_response_handler("node1", env);
    EXPECT_FALSE(called);

    env.set_sender("node2");
    swarm_response_handler("node2", env);
    EXPECT_FALSE(called);

    env.set_sender("node1");
    swarm_response_handler("node1", env);
    EXPECT_FALSE(called);

    env.set_sender("node2");
    swarm_response_handler("node2", env);
    EXPECT_FALSE(called);

    // send a different response. should not add to quorum
    bzn_envelope env2{env};
    database_error err;
    err.set_message("error");
    *response.mutable_error() = err;
    env2.set_database_response(response.SerializeAsString());
    env2.set_sender("node3");
    swarm_response_handler("node3", env2);
    EXPECT_FALSE(called);

    env.set_sender("node3");
    swarm_response_handler("node3", env);
    EXPECT_TRUE(called);

    // handler shouldn't be called twice (enforced in the callback)
    env.set_sender("node3");
    swarm_response_handler("node3", env);
}