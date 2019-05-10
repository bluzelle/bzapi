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

namespace bzapi
{
    // temporary?
    bool initialize(const std::string& public_key, const std::string& private_key, const std::string& endpoint);

    void terminate();

    std::shared_ptr<response> has_db(const std::string& uuid);

    std::shared_ptr<response> create_db(const std::string& uuid);

    std::shared_ptr<response> open_db(const std::string& uuid);

    namespace sync
    {
        bool has_db(const std::string& uuid);
        bool create_db(const std::string& uuid);
        bool open_db(const std::string& uuid);
    }
}