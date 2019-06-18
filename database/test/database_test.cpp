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
#include <mocks/mock_db_impl.hpp>
#include <mocks/mock_swarm.hpp>
#include <database/database_impl.hpp>
#include <json/value.h>
#include <json/reader.h>

using namespace testing;
using namespace bzapi;

namespace
{
}

class database_test : public Test
{
public:

protected:
    std::shared_ptr<mock_db_impl> dbi = std::make_shared<mock_db_impl>();
    std::shared_ptr<mock_swarm> sw = std::make_shared<mock_swarm>();
    std::shared_ptr<async_database_impl> adb = std::make_shared<async_database_impl>(dbi, sw, "db_uuid");
    database_impl db{adb};
};


TEST_F(database_test, test_open)
{
    EXPECT_CALL(*this->sw, initialize(_)).WillOnce(Invoke([](auto handler) { handler(boost::system::error_code{}); }));
    adb->open([](const auto& ec)
    {
        EXPECT_EQ(ec.value(), 0);
    });
}

TEST_F(database_test, test_create)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_create());
        EXPECT_EQ(msg.create().key(), "key");
        EXPECT_EQ(msg.create().value(), "value");
        EXPECT_EQ(msg.create().expire(), 0u);
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        handler(response, boost::system::error_code{});
    }));

    auto result = db.create("key", "value", 0);
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
}


TEST_F(database_test, test_read)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_read());
        EXPECT_EQ(msg.read().key(), "key");
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        database_read_response read_response;
        read_response.set_key("key");
        read_response.set_value("value");
        response.set_allocated_read(new database_read_response(read_response));
        handler(response, boost::system::error_code{});
    }));

    auto result = db.read("key");
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
    EXPECT_EQ(resp_json["value"].asString(), "value");
}

TEST_F(database_test, test_update)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_update());
        EXPECT_EQ(msg.update().key(), "key");
        EXPECT_EQ(msg.update().value(), "value");
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        handler(response, boost::system::error_code{});
    }));

    auto result = db.update("key", "value");
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
}

TEST_F(database_test, test_remove)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_delete_());
        EXPECT_EQ(msg.delete_().key(), "key");
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        handler(response, boost::system::error_code{});
    }));

    auto result = db.remove("key");
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
}

TEST_F(database_test, test_quick_read)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_quick_read());
        EXPECT_EQ(msg.quick_read().key(), "key");
        EXPECT_EQ(policy, send_policy::fastest);

        database_response response;
        database_quick_read_response read_response;
        read_response.set_key("key");
        read_response.set_value("value");
        response.set_allocated_quick_read(new database_quick_read_response(read_response));
        handler(response, boost::system::error_code{});
    }));

    auto result = db.quick_read("key");
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
    EXPECT_EQ(resp_json["value"].asString(), "value");
}

TEST_F(database_test, test_has)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_has());
        EXPECT_EQ(msg.has().key(), "key");
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        database_has_response has_response;
        has_response.set_has(true);
        response.set_allocated_has(new database_has_response(has_response));
        handler(response, boost::system::error_code{});
    }));

    auto result = db.has("key");
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
}

TEST_F(database_test, test_keys)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_keys());
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        database_keys_response keys_response;
        keys_response.add_keys("key1");
        keys_response.add_keys("key2");
        response.set_allocated_keys(new database_keys_response(keys_response));
        handler(response, boost::system::error_code{});
    }));

    auto result = db.keys();
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["keys"].size(), 2u);
    EXPECT_EQ(resp_json["keys"][0], "key1");
    EXPECT_EQ(resp_json["keys"][1], "key2");
}

TEST_F(database_test, test_size)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_size());
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        database_size_response size_response;
        size_response.set_bytes(123);
        size_response.set_keys(12);
        size_response.set_remaining_bytes(321);
        size_response.set_max_size(999);
        response.set_allocated_size(new database_size_response(size_response));
        handler(response, boost::system::error_code{});
    }));

    auto result = db.size();
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
    EXPECT_EQ(resp_json["bytes"].asInt(), 123);
    EXPECT_EQ(resp_json["keys"].asInt(), 12);
    EXPECT_EQ(resp_json["remaining_bytes"].asInt(), 321);
    EXPECT_EQ(resp_json["max_size"].asInt(), 999);
}

TEST_F(database_test, test_expire)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_expire());
        EXPECT_EQ(msg.expire().key(), "key");
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        handler(response, boost::system::error_code{});
    }));

    auto result = db.expire("key", 123);
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
}

TEST_F(database_test, test_persist)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_persist());
        EXPECT_EQ(msg.persist().key(), "key");
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        handler(response, boost::system::error_code{});
    }));

    auto result = db.persist("key");
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
}

TEST_F(database_test, test_ttl)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_ttl());
        EXPECT_EQ(msg.ttl().key(), "key");
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        database_ttl_response ttl_response;
        ttl_response.set_key("key");
        ttl_response.set_ttl(123);
        response.set_allocated_ttl(new database_ttl_response(ttl_response));
        handler(response, boost::system::error_code{});
    }));

    auto result = db.ttl("key");
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
    EXPECT_EQ(resp_json["key"].asString(), "key");
    EXPECT_EQ(resp_json["ttl"].asInt(), 123);
}

TEST_F(database_test, test_writers)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_writers());
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        database_writers_response writers_response;
        writers_response.add_writers("writer_1");
        writers_response.add_writers("writer_2");
        response.set_allocated_writers(new database_writers_response(writers_response));
        handler(response, boost::system::error_code{});
    }));

    auto result = db.writers();
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
    EXPECT_EQ(resp_json["writers"].size(), 2u);
    EXPECT_EQ(resp_json["writers"][0], "writer_1");
    EXPECT_EQ(resp_json["writers"][1], "writer_2");
}

TEST_F(database_test, test_add_writer)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_add_writers());
        EXPECT_EQ(msg.add_writers().writers_size(), 1);
        EXPECT_EQ(msg.add_writers().writers(0), "test_writer");
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        handler(response, boost::system::error_code{});
    }));

    auto result = db.add_writer("test_writer");
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), true);
}

TEST_F(database_test, test_remove_writer)
{
    EXPECT_CALL(*dbi, send_message_to_swarm(_, _, _, _, _)).WillOnce(Invoke([](auto& /*sw*/, auto& /*uuid*/
        , database_msg& msg, auto policy, auto handler)
    {
        EXPECT_TRUE(msg.has_remove_writers());
        EXPECT_EQ(msg.remove_writers().writers_size(), 1);
        EXPECT_EQ(msg.remove_writers().writers(0), "test_writer");
        EXPECT_EQ(policy, send_policy::normal);

        database_response response;
        database_error err;
        err.set_message("Writer not found");
        response.set_allocated_error(new database_error(err));
        handler(response, boost::system::error_code{});
    }));

    auto result = db.remove_writer("test_writer");
    Json::Value resp_json;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(result, resp_json));
    EXPECT_EQ(resp_json["result"].asBool(), false);
    EXPECT_EQ(resp_json["error"].asString(), std::string("Writer not found"));
}
