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

#pragma once

#include <include/database.hpp>
#include <database/async_database_impl.hpp>

namespace bzapi
{
    class database_impl : public database
    {
    public:
        database_impl(std::shared_ptr<async_database> db);

        std::string create(const std::string& key, const std::string& value, uint64_t expiry) override;
        std::string read(const std::string& key) override;
        std::string update(const std::string& key, const std::string& value) override;
        std::string remove(const std::string& key) override;

        std::string quick_read(const std::string& key) override;
        std::string has(const std::string& key) override;
        std::string keys() override;
        std::string size() override;
        std::string expire(const std::string& key, uint64_t expiry) override;
        std::string persist(const std::string& key) override;
        std::string ttl(const std::string& key) override;

        std::string swarm_status();

    private:
        std::shared_ptr<async_database> db;
    };
}
