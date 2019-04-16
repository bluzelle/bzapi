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

#include <database/database.hpp>
#include <jsoncpp/src/jsoncpp/include/json/value.h>

using namespace bzapi;

namespace bzapi
{
    std::shared_ptr<response> make_response();
}

database::database(std::shared_ptr<db_impl_base> db_impl)
: db_impl(db_impl)
{
}

void
database::open(completion_handler_t handler)
{
    this->db_impl->initialize(handler);
}

// TODO: possibly move this into handle_swarm_response
void
database::translate_swarm_response(const database_response& response, const boost::system::error_code& ec
    , std::function<void(const database_response& response, db_error err, const std::string& msg)> handler)
{
    if (ec)
    {
        handler(response, db_error::connection_error, ec.message());
    }
    else
    {
        if (response.has_error())
        {
            handler(response, db_error::database_error, response.error().message());
        }
        else
        {
            handler(response, db_error::success, "");
        }
    }
}

std::shared_ptr<response>
database::create(const key_t& key, const value_t& value)
{
    auto resp = make_response();

    auto request = new database_create;
    request->set_key(key);
    request->set_value(value);

    database_msg msg;
    msg.set_allocated_create(request);

    this->db_impl->send_message_to_swarm(msg, send_policy::normal
        , std::bind(&database::translate_swarm_response, shared_from_this(), std::placeholders::_1
        , std::placeholders::_2, [resp](const database_response& /*response*/, auto err, const auto& msg)
        {
            Json::Value result;
            if (err == db_error::success)
            {
                result["result"] = 1;
                resp->set_result(result.toStyledString());
                resp->set_ready();
            }
            else
            {
                result["error"] = msg;
                resp->set_result(result.toStyledString());
                resp->set_error(static_cast<int>(err));
            }
        }));

    return resp;
}

std::shared_ptr<response>
database::read(const key_t& key)
{
    auto resp = make_response();

    auto request = new database_read;
    request->set_key(key);

    database_msg msg;
    msg.set_allocated_read(request);

    this->db_impl->send_message_to_swarm(msg, send_policy::normal
        , std::bind(&database::translate_swarm_response, shared_from_this(), std::placeholders::_1
        , std::placeholders::_2, [resp](const database_response &response, auto err, const auto &msg)
        {
            Json::Value result;
            if (err == db_error::success)
            {
                const database_read_response& read_resp = response.read();
                result["result"] = 1;
                result["key"] = read_resp.key();
                result["value"] = read_resp.value();
                resp->set_result(result.toStyledString());
                resp->set_ready();
            }
            else
            {
                result["error"] = msg;
                resp->set_result(result.toStyledString());
                resp->set_error(static_cast<int>(err));
            }
        }));

    return resp;
}

std::shared_ptr<response>
database::update(const key_t& /*key*/, const value_t& /*value*/)
{
    return nullptr;
}

std::shared_ptr<response>
database::remove(const key_t& /*key*/)
{
    return nullptr;
}

std::shared_ptr<response>
database::quick_read(const key_t& /*key*/)
{
    return nullptr;
}

std::shared_ptr<response>
database::has(const key_t& /*key*/)
{
    return nullptr;
}

std::shared_ptr<response>
database::keys()
{
    return nullptr;
}

std::shared_ptr<response>
database::size()
{
    return nullptr;
}

std::shared_ptr<response>
database::expire(const key_t& /*key*/, expiry_t /*expiry*/)
{
    return nullptr;
}

std::shared_ptr<response>
database::persist(const key_t& /*key*/)
{
    return nullptr;
}

std::shared_ptr<response>
database::ttl(const key_t& /*key*/)
{
    return nullptr;
}

