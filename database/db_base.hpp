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

#include <cstdint>
#include <boost_asio_beast.hpp>
#include <defs.hpp>
#include <proto/database.pb.h>
#include <library/response.hpp>

namespace bzapi
{
    enum class db_error
    {
        success = 0,
        connection_error,
        database_error,
        no_database
    };

    // provides crud+ access to a single database uuid
    class db_base
    {
    public:
        using void_handler_t = std::function<void(db_error error, const std::string& msg)>;
        using value_handler_t = std::function<void(const value_t& value, db_error error, const std::string& msg)>;
        using vector_handler_t = std::function<void(const std::vector<value_t>& value,  db_error error, const std::string& msg)>;

        virtual ~db_base() = default;



    };
}