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

#include <database/database_sync.hpp>

using namespace bzapi;

database_sync::database_sync(const database& db)
: database(db)
{
}

std::string
database_sync::create(const key_t& key, const value_t& value)
{
    auto resp = database::create(key, value);
    return resp->get_result();
}

std::string
database_sync::read(const key_t& key)
{
    auto resp = database::read(key);
    return resp->get_result();
}

std::string
database_sync::update(const key_t& key, const value_t& value)
{
    auto resp = database::update(key, value);
    return resp->get_result();
}

std::string
database_sync::remove(const key_t& key)
{
    auto resp = database::remove(key);
    return resp->get_result();
}

std::string
database_sync::quick_read(const key_t& key)
{
    auto resp = database::quick_read(key);
    return resp->get_result();
}

std::string
database_sync::has(const key_t& key)
{
    auto resp = database::has(key);
    return resp->get_result();
}

std::string
database_sync::keys()
{
    auto resp = database::keys();
    return resp->get_result();
}

std::string
database_sync::size()
{
    auto resp = database::size();
    return resp->get_result();
}

std::string
database_sync::expire(const key_t& key, expiry_t expiry)
{
    auto resp = database::expire(key, expiry);
    return resp->get_result();
}

std::string
database_sync::persist(const key_t& key)
{
    auto resp = database::persist(key);
    return resp->get_result();
}

std::string
database_sync::ttl(const key_t& key)
{
    auto resp = database::ttl(key);
    return resp->get_result();
}


std::string
database_sync::swarm_status()
{
    return database::swarm_status();
}
