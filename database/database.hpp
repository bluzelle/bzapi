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

#include <database/async_database.hpp>

namespace bzapi
{
    class database : private async_database
    {
    public:

        database(const async_database& db);

        std::string create(const std::string& key, const std::string& value);
        std::string read(const std::string& key);
        std::string update(const std::string& key, const std::string& value);
        std::string remove(const std::string& key);

        std::string quick_read(const std::string& key);
        std::string has(const std::string& key);
        std::string keys();
        std::string size();
        std::string expire(const std::string& key, expiry_t expiry);
        std::string persist(const std::string& key);
        std::string ttl(const std::string& key);

        std::string swarm_status();
    };
}
