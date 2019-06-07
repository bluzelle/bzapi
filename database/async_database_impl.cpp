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

#include <database/async_database_impl.hpp>
#include <json/value.h>

using namespace bzapi;

namespace bzapi
{
    std::shared_ptr<mutable_response> make_response();
}

async_database_impl::async_database_impl(std::shared_ptr<db_impl_base> db_impl, std::shared_ptr<swarm_base> swarm, uuid_t uuid)
: db_impl(db_impl), swarm(swarm), uuid(uuid)
{
    std::cout << "here" << std::endl;
}

async_database_impl::~async_database_impl()
{
}

void
async_database_impl::open(completion_handler_t handler)
{
    if (this->state != init_state::none)
    {
        return;
    }

    this->state = init_state::initializing;
    this->swarm->initialize([weak_this = weak_from_this(), handler](const auto& ec)
    {
        if (!ec)
        {
            auto strong_this = weak_this.lock();
            if (strong_this)
            {
                strong_this->state = init_state::initialized;
            }
        }

        handler(ec);
    });
}

void
async_database_impl::translate_swarm_response(const database_response& db_response, const boost::system::error_code& ec
    , std::shared_ptr<mutable_response> resp
    , std::function<void(const database_response& response)> handler)
{
    Json::Value result;
    if (ec)
    {
        result["error"] = ec.message();
        resp->set_result(result.toStyledString());
        resp->set_error(static_cast<int>(db_error::connection_error));
    }
    else
    {
        if (db_response.has_error())
        {
            result["error"] = db_response.error().message();
            resp->set_result(result.toStyledString());

            // TODO: propagate other error codes (e.g. timeout) here
            resp->set_error(static_cast<int>(db_error::database_error));
        }
        else
        {
            handler(db_response);
        }
    }
}

std::string
async_database_impl::swarm_status()
{
    return this->db_impl->swarm_status();
}

void
async_database_impl::send_message_with_basic_response(database_msg& msg, std::shared_ptr<mutable_response> resp)
{
    this->db_impl->send_message_to_swarm(this->swarm, this->uuid, msg, send_policy::normal
        , [resp](const database_response &response, const boost::system::error_code &error)
    {
        translate_swarm_response(response, error, resp, [resp](const database_response &/*response*/)
        {
            Json::Value result;
            result["result"] = 1;
            resp->set_result(result.toStyledString());
            resp->set_ready();
        });
    });
}

std::shared_ptr<response>
async_database_impl::create(const std::string& key, const std::string& value, uint64_t expiry)
{
    try
    {
        auto resp = make_response();
        auto request = new database_create;
        request->set_key(key);
        request->set_value(value);
        request->set_expire(expiry);

        database_msg msg;
        msg.set_allocated_create(request);

        send_message_with_basic_response(msg, resp);

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::read(const std::string& key)
{
    try
    {
        auto resp = make_response();
        auto request = new database_read;
        request->set_key(key);

        database_msg msg;
        msg.set_allocated_read(request);

        this->db_impl->send_message_to_swarm(this->swarm, this->uuid, msg, send_policy::normal
            , [resp](const database_response &response, const boost::system::error_code &error)
            {
                translate_swarm_response(response, error, resp, [resp](const database_response &response)
                {
                    Json::Value result;
                    const database_read_response &read_resp = response.read();
                    result["result"] = 1;
                    result["key"] = read_resp.key();
                    result["value"] = read_resp.value();
                    resp->set_result(result.toStyledString());
                    resp->set_ready();
                });
            });

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::update(const std::string& key, const std::string& value)
{
    try
    {
        auto resp = make_response();
        auto request = new database_update;
        request->set_key(key);
        request->set_value(value);

        database_msg msg;
        msg.set_allocated_update(request);

        send_message_with_basic_response(msg, resp);

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::remove(const std::string& key)
{
    try
    {
        auto resp = make_response();
        auto request = new database_delete;
        request->set_key(key);

        database_msg msg;
        msg.set_allocated_delete_(request);

        send_message_with_basic_response(msg, resp);

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::quick_read(const std::string& key)
{
    try
    {
        auto resp = make_response();
        auto request = new database_read;
        request->set_key(key);

        database_msg msg;
        msg.set_allocated_quick_read(request);

        this->db_impl->send_message_to_swarm(this->swarm, this->uuid, msg, send_policy::fastest
            , [resp](const database_response &response, const boost::system::error_code &ec)
            {
                Json::Value result;
                if (ec)
                {
                    result["error"] = ec.message();
                    resp->set_result(result.toStyledString());
                    resp->set_error(static_cast<int>(db_error::connection_error));
                }
                else
                {
                    const database_quick_read_response &rr = response.quick_read();
                    if (!rr.error().empty())
                    {
                        result["error"] = rr.error();
                        resp->set_result(result.toStyledString());
                        resp->set_error(static_cast<int>(db_error::database_error));
                    }
                    else
                    {
                        result["result"] = 1;
                        result["key"] = rr.key();
                        result["value"] = rr.value();
                        resp->set_result(result.toStyledString());
                        resp->set_ready();
                    }
                }
            });

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::has(const std::string& key)
{
    try
    {
        auto resp = make_response();
        auto request = new database_has;
        request->set_key(key);

        database_msg msg;
        msg.set_allocated_has(request);

        this->db_impl->send_message_to_swarm(this->swarm, this->uuid, msg, send_policy::normal
            , [resp](const database_response &response, const boost::system::error_code &error)
            {
                translate_swarm_response(response, error, resp, [resp](const database_response &response)
                {
                    Json::Value result;
                    result["result"] = response.has().has();
                    resp->set_result(result.toStyledString());
                    resp->set_ready();
                });
            });

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::keys()
{
    try
    {
        auto resp = make_response();
        auto request = new database_request;

        database_msg msg;
        msg.set_allocated_keys(request);

        this->db_impl->send_message_to_swarm(this->swarm, this->uuid, msg, send_policy::normal
            , [resp](const database_response &response, const boost::system::error_code &ec)
            {
                translate_swarm_response(response, ec, resp, [resp](const database_response &response)
                {
                    Json::Value result;
                    const database_keys_response &keys_resp = response.keys();
                    Json::Value keys;
                    for (int i = 0; i < keys_resp.keys_size(); i++)
                    {
                        keys.append(keys_resp.keys(i));
                    }
                    result["keys"] = keys;

                    resp->set_result(result.toStyledString());
                    resp->set_ready();
                });
            });

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::size()
{
    try
    {
        auto resp = make_response();
        auto request = new database_request;

        database_msg msg;
        msg.set_allocated_size(request);

        this->db_impl->send_message_to_swarm(this->swarm, this->uuid, msg, send_policy::normal
            , [resp](const database_response &response, const boost::system::error_code &ec)
            {
                translate_swarm_response(response, ec, resp, [resp](const database_response &response)
                {
                    Json::Value result;
                    const database_size_response &size_resp = response.size();
                    result["result"] = 1;
                    result["bytes"] = static_cast<Json::Value::UInt64>(size_resp.bytes());
                    result["keys"] = size_resp.keys();
                    result["remaining_bytes"] = static_cast<Json::Value::UInt64>(size_resp.remaining_bytes());
                    result["max_size"] = static_cast<Json::Value::UInt64>(size_resp.max_size());
                    resp->set_result(result.toStyledString());
                    resp->set_ready();
                });
            });

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::expire(const std::string& key, expiry_t expiry)
{
    try
    {
        auto resp = make_response();
        auto request = new database_expire;
        request->set_key(key);
        request->set_expire(expiry);

        database_msg msg;
        msg.set_allocated_expire(request);

        send_message_with_basic_response(msg, resp);

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::persist(const std::string& key)
{
    try
    {
        auto resp = make_response();
        auto request = new database_read;
        request->set_key(key);

        database_msg msg;
        msg.set_allocated_persist(request);

        send_message_with_basic_response(msg, resp);

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::ttl(const std::string& key)
{
    try
    {
        auto resp = make_response();
        auto request = new database_read;
        request->set_key(key);

        database_msg msg;
        msg.set_allocated_ttl(request);

        this->db_impl->send_message_to_swarm(this->swarm, this->uuid, msg, send_policy::normal
            , [resp](const database_response &response, const boost::system::error_code &ec)
            {
                translate_swarm_response(response, ec, resp, [resp](const database_response &response)
                {
                    Json::Value result;
                    const database_ttl_response &read_resp = response.ttl();
                    result["result"] = 1;
                    result["key"] = read_resp.key();
                    result["ttl"] = static_cast<Json::Value::UInt64>(read_resp.ttl());
                    resp->set_result(result.toStyledString());
                    resp->set_ready();
                });
            });

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::writers()
{
    try
    {
        auto resp = make_response();
        auto request = new database_request;

        database_msg msg;
        msg.set_allocated_writers(request);

        this->db_impl->send_message_to_swarm(this->swarm, this->uuid, msg, send_policy::normal
            , [resp](const database_response &response, const boost::system::error_code &ec)
            {
                translate_swarm_response(response, ec, resp, [resp](const database_response &response)
                {
                    Json::Value result;
                    const database_writers_response &writers_resp = response.writers();
                    Json::Value writers;
                    for (int i = 0; i < writers_resp.writers_size(); i++)
                    {
                        writers.append(writers_resp.writers(i));
                    }

                    result["result"] = 1;
                    result["writers"] = writers;

                    resp->set_result(result.toStyledString());
                    resp->set_ready();
                });
            });

        return resp;

    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::add_writer(const std::string& writer)
{
    try
    {
        auto resp = make_response();
        auto request = new database_writers;
        request->add_writers(writer);

        database_msg msg;
        msg.set_allocated_add_writers(request);
        send_message_with_basic_response(msg, resp);

        return resp;
    }
    CATCHALL();
    return nullptr;
}

std::shared_ptr<response>
async_database_impl::remove_writer(const std::string& writer)
{
    try
    {
        auto resp = make_response();
        auto request = new database_writers;
        request->add_writers(writer);

        database_msg msg;
        msg.set_allocated_remove_writers(request);
        send_message_with_basic_response(msg, resp);

        return resp;
    }
    CATCHALL();
    return nullptr;
}
