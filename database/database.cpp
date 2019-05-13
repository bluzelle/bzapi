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

database::database(const async_database& db)
: async_database(db)
{
}

std::string
database::create(const std::string& key, const std::string& value)
{
    auto resp = async_database::create(key, value);
    return resp->get_result();
}

std::string
database::read(const std::string& key)
{
    auto resp = async_database::read(key);
    return resp->get_result();
}

std::string
database::update(const std::string& key, const std::string& value)
{
    auto resp = async_database::update(key, value);
    return resp->get_result();
}

std::string
database::remove(const std::string& key)
{
    auto resp = async_database::remove(key);
    return resp->get_result();
}

std::string
database::quick_read(const std::string& key)
{
    auto resp = async_database::quick_read(key);
    return resp->get_result();
}

std::string
database::has(const std::string& key)
{
    auto resp = async_database::has(key);
    return resp->get_result();
}

std::string
database::keys()
{
    auto resp = async_database::keys();
    return resp->get_result();
}

std::string
database::size()
{
    auto resp = async_database::size();
    return resp->get_result();
}

std::string
database::expire(const std::string& key, expiry_t expiry)
{
    auto resp = async_database::expire(key, expiry);
    return resp->get_result();
}

std::string
database::persist(const std::string& key)
{
    auto resp = async_database::persist(key);
    return resp->get_result();
}

std::string
database::ttl(const std::string& key)
{
    auto resp = async_database::ttl(key);
    return resp->get_result();
}


std::string
database::swarm_status()
{
    return async_database::swarm_status();
}
