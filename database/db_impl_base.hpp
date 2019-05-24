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

#include <proto/database.pb.h>
#include <bluzelle.hpp>
#include <swarm/swarm_base.hpp>

namespace bzapi
{
    using db_response_handler_t = std::function<void(const database_response &response, const boost::system::error_code &error)>;

    class db_impl_base
    {
    public:
        virtual ~db_impl_base() = default;
        virtual void initialize(completion_handler_t handler) = 0;
        virtual void send_message_to_swarm(database_msg& msg, send_policy policy, db_response_handler_t handler) = 0;
        virtual std::string swarm_status() = 0;
    };

}