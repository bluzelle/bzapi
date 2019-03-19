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

using namespace bzapi;

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

void
database::create(const key_t& key, const value_t& value, void_handler_t handler)
{
    auto request = new database_create;
    request->set_key(key);
    request->set_value(value);

    database_msg msg;
    msg.set_allocated_create(request);

    this->db_impl->send_message_to_swarm(msg, send_policy::normal
        , std::bind(&database::translate_swarm_response, shared_from_this(), std::placeholders::_1
        , std::placeholders::_2, [handler](const auto&, auto err, const auto& msg)
        {
            handler(err, msg);
        }));
}

void
database::read(const key_t& key, value_handler_t handler)
{
    auto request = new database_read;
    request->set_key(key);

    database_msg msg;
    msg.set_allocated_read(request);

    this->db_impl->send_message_to_swarm(msg, send_policy::normal
        , std::bind(&database::translate_swarm_response, shared_from_this(), std::placeholders::_1
        , std::placeholders::_2, [handler](const auto &response, auto err, const auto &msg)
        {
            handler(response.has_error() ? "" : response.read().value(), err, msg);
        }));
}

void
database::update(const key_t& /*key*/, const value_t& /*value*/, void_handler_t /*handler*/)
{

}

void
database::remove(const key_t& /*key*/, void_handler_t /*handler*/)
{

}

void
database::quick_read(const key_t& /*key*/, value_handler_t /*handler*/)
{

}

void
database::has(const key_t& /*key*/, value_handler_t /*handler*/)
{

}

void
database::keys(vector_handler_t /*handler*/)
{

}

void
database::size(value_handler_t /*handler*/)
{

}

void
database::expire(const key_t& /*key*/, expiry_t /*expiry*/, void_handler_t /*handler*/)
{

}

void
database::persist(const key_t& /*key*/, void_handler_t /*handler*/)
{

}

void
database::ttl(const key_t& /*key*/, value_handler_t /*handler*/)
{

}

