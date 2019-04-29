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

#include <gmock/gmock.h>
#include <database/db_impl_base.hpp>

namespace bzapi
{
    class mock_db_impl : public db_impl_base
    {
    public:
        MOCK_METHOD1(initialize, void(completion_handler_t));
        MOCK_METHOD3(send_message_to_swarm, void(database_msg&, send_policy, db_response_handler_t));
        MOCK_METHOD0(swarm_status, std::string(void));
    };
}