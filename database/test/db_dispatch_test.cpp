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

#include <include/bzapi.hpp>
#include <database/db_dispatch.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <json/json.h>
#include <mocks/mock_boost_asio_beast.hpp>
#include <mocks/mock_swarm.hpp>


using namespace testing;
using namespace bzapi;

namespace
{
}

class db_dispatch_test : public Test
{
public:

protected:
    std::shared_ptr<bzn::asio::mock_io_context_base> mock_io_context  = std::make_shared<bzn::asio::mock_io_context_base>();
    std::shared_ptr<mock_swarm> swarm = std::make_shared<mock_swarm>();
    std::shared_ptr<db_dispatch> db = std::make_shared<db_dispatch>(mock_io_context);

};


TEST_F(db_dispatch_test, basic_test)
{
    swarm_response_handler_t swarm_response_handler;
    completion_handler_t timer_callback;

    EXPECT_CALL(*swarm, register_response_handler(_, _))
    .Times(Exactly(2))
    .WillOnce(Invoke([&](auto, auto handler)
    {
        swarm_response_handler = handler;
        return true;
    }))
    .WillOnce(Return(true));

    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(Exactly(1))
        .WillOnce(Invoke([&]
        {
            return std::make_unique<NiceMock<bzn::asio::mock_steady_timer_base>>();
        }));

    EXPECT_CALL(*swarm, sign_and_date_request(_, _)).Times(Exactly(1));

    EXPECT_CALL(*swarm, send_request(_, _)).Times(Exactly(1)).WillOnce(Invoke([&](auto e, auto)
    {
        bzn_envelope env;
        database_msg request;
        EXPECT_TRUE(request.ParseFromString(e.database_msg()));
        database_response response;
        *response.mutable_header() = request.header();
        env.set_database_response(response.SerializeAsString());
        env.set_sender("node1");
        env.set_signature("xxx");
        swarm_response_handler("node1", env);
        return 0;
    }));

    database_msg request;
    bool called = false;
    db->send_message_to_swarm(this->swarm, "db_uuid", request, send_policy::fastest, [&](const auto& /*response*/, const auto& /*ec*/)
    {
        called = true;
    });

    EXPECT_TRUE(called);
}

TEST_F(db_dispatch_test, collation_and_timeout_test)
{
    swarm_response_handler_t swarm_response_handler;
    completion_handler_t timer_callback;

    EXPECT_CALL(*swarm, register_response_handler(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&](auto, auto handler)
        {
            swarm_response_handler = handler;
            return true;
        }))
        .WillOnce(Return(true));

    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(AtLeast(2))
    .WillOnce(Invoke([&]
    {
        return std::make_unique<NiceMock<bzn::asio::mock_steady_timer_base>>();
    }))
    .WillRepeatedly(Invoke([&]
    {
        auto timer = std::make_unique<bzn::asio::mock_steady_timer_base>();
        EXPECT_CALL(*timer, expires_from_now(_)).Times(AtLeast(1));
        EXPECT_CALL(*timer, async_wait(_)).WillRepeatedly(Invoke([&](auto handler)
        {
            timer_callback = handler;
        }));
        return timer;
    }));

    EXPECT_CALL(*swarm, honest_majority_size()).Times(Exactly(1)).WillOnce(Return(3));

    EXPECT_CALL(*swarm, sign_and_date_request(_, _)).Times(Exactly(1));

    bool broadcasted = false;
    bzn_envelope envelope;
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
    db->send_message_to_swarm(this->swarm, "db_uuid", request, send_policy::normal, [&](const auto& /*response*/, const auto& /*ec*/)
    {
        ASSERT_EQ(called, false);
        called = true;
    });

    ASSERT_FALSE(called);
    ASSERT_TRUE(broadcasted);

    bzn_envelope env;
    ASSERT_TRUE(request.ParseFromString(envelope.database_msg()));
    database_response response;
    *response.mutable_header() = request.header();
    env.set_database_response(response.SerializeAsString());
    env.set_signature("xxx");

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

TEST_F(db_dispatch_test, client_timeout_test)
{
    completion_handler_t timer_callback;

    bzapi::set_timeout(1);

    EXPECT_CALL(*swarm, register_response_handler(_, _))
        .Times(Exactly(2))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).Times(Exactly(1))
        .WillOnce(Invoke([&]
        {
            auto timer = std::make_unique<bzn::asio::mock_steady_timer_base>();
            EXPECT_CALL(*timer, expires_from_now(_)).Times(AtLeast(1)).WillOnce(Invoke([](auto time)
            {
                EXPECT_EQ(time, std::chrono::seconds(1));
                return 0;
            }));

            EXPECT_CALL(*timer, async_wait(_)).WillRepeatedly(Invoke([&](auto handler)
            {
                timer_callback = handler;
            }));
            return timer;
        }));

    EXPECT_CALL(*swarm, sign_and_date_request(_, _)).Times(Exactly(1));

    EXPECT_CALL(*swarm, send_request(_, _)).Times(Exactly(1)).WillOnce(Invoke([&](auto /*e*/, auto)
    {
        timer_callback(boost::system::error_code{});
        return 0;
    }));

    database_msg request;
    bool called = false;
    db->send_message_to_swarm(this->swarm, "db_uuid", request, send_policy::fastest, [&](const auto& response, const auto& /*ec*/)
    {
        called = true;

        EXPECT_TRUE(response.has_error());
        EXPECT_EQ(response.error().message(), std::string("Request timeout"));
    });

    EXPECT_TRUE(called);
}

