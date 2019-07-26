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
: db(std::move(db))
{
}

std::string
database_impl::create(const std::string& key, const std::string& value, uint64_t expiry)
{
    return db->create(key, value, expiry)->get_result();
}

std::string
database_impl::read(const std::string& key)
{
    return db->read(key)->get_result();
}

std::string
database_impl::update(const std::string& key, const std::string& value)
{
    return db->update(key, value)->get_result();
}

std::string
database_impl::remove(const std::string& key)
{
    return db->remove(key)->get_result();
}

std::string
database_impl::quick_read(const std::string& key)
{
    return db->quick_read(key)->get_result();
}

std::string
database_impl::has(const std::string& key)
{
    return db->has(key)->get_result();
}

std::string
database_impl::keys()
{
    return db->keys()->get_result();
}

std::string
database_impl::size()
{
    return db->size()->get_result();
}

std::string
database_impl::expire(const std::string& key, expiry_t expiry)
{
    return db->expire(key, expiry)->get_result();
}

std::string
database_impl::persist(const std::string& key)
{
    return db->persist(key)->get_result();
}

std::string
database_impl::ttl(const std::string& key)
{
    return db->ttl(key)->get_result();
}

std::string
database_impl::writers()
{
    return db->writers()->get_result();
}

std::string
database_impl::add_writer(const std::string& writer)
{
    return db->add_writer(writer)->get_result();
}

std::string
database_impl::remove_writer(const std::string& writer)
{
    return db->remove_writer(writer)->get_result();
}

std::string
database_impl::swarm_status()
{
    return db->swarm_status();
}

