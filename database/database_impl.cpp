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

#include <database/database_impl.hpp>

using namespace bzapi;

database_impl::database_impl(std::shared_ptr<async_database> db)
: db(db)
{
}

std::string
database_impl::create(const std::string& key, const std::string& value, uint64_t expiry)
{
    auto resp = db->create(key, value, expiry);
    return resp->get_result();
}

std::string
database_impl::read(const std::string& key)
{
    auto resp = db->read(key);
    return resp->get_result();
}

std::string
database_impl::update(const std::string& key, const std::string& value)
{
    auto resp = db->update(key, value);
    return resp->get_result();
}

std::string
database_impl::remove(const std::string& key)
{
    auto resp = db->remove(key);
    return resp->get_result();
}

std::string
database_impl::quick_read(const std::string& key)
{
    auto resp = db->quick_read(key);
    return resp->get_result();
}

std::string
database_impl::has(const std::string& key)
{
    auto resp = db->has(key);
    return resp->get_result();
}

std::string
database_impl::keys()
{
    auto resp = db->keys();
    return resp->get_result();
}

std::string
database_impl::size()
{
    auto resp = db->size();
    return resp->get_result();
}

std::string
database_impl::expire(const std::string& key, expiry_t expiry)
{
    auto resp = db->expire(key, expiry);
    return resp->get_result();
}

std::string
database_impl::persist(const std::string& key)
{
    auto resp = db->persist(key);
    return resp->get_result();
}

std::string
database_impl::ttl(const std::string& key)
{
    auto resp = db->ttl(key);
    return resp->get_result();
}

std::string
database_impl::writers()
{
    auto resp = db->writers();
    return resp->get_result();
}

std::string
database_impl::add_writer(const std::string& writer)
{
    auto resp = db->add_writer(writer);
    return resp->get_result();
}

std::string
database_impl::remove_writer(const std::string& writer)
{
    auto resp = db->remove_writer(writer);
    return resp->get_result();
}

std::string
database_impl::swarm_status()
{
    return db->swarm_status();
}

