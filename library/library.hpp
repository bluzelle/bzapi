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

#include <library/response.hpp>
#include <database/database.hpp>

namespace bzapi
{
    bool initialize(const std::string& public_key, const std::string& private_key
        , const std::string& endpoint, const std::string& swarm_id);

    void terminate();

    bool has_db(const std::string& uuid);

    std::shared_ptr<database> create_db(const std::string& uuid);

    std::shared_ptr<database> open_db(const std::string& uuid);

    std::shared_ptr<response> async_has_db(const std::string& uuid);

    std::shared_ptr<response> async_create_db(const std::string& uuid);

    std::shared_ptr<response> async_open_db(const std::string& uuid);

    int get_error();

    std::string get_error_str();
}