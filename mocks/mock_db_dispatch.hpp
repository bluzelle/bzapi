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

#pragma once

#include <gmock/gmock.h>
#include <database/db_dispatch_base.hpp>

namespace bzapi
{
    class mock_db_dispatch : public db_dispatch_base
    {
    public:
        MOCK_METHOD3(has_uuid, void(std::shared_ptr<swarm_base>, uuid_t, std::function<void(db_error)>));
        MOCK_METHOD5(create_uuid, void(std::shared_ptr<swarm_base>, uuid_t, uint64_t, bool, std::function<void(db_error)>));
        MOCK_METHOD5(send_message_to_swarm, void(std::shared_ptr<swarm_base>, uuid_t, database_msg&, send_policy, db_response_handler_t));
        MOCK_METHOD0(swarm_status, std::string(void));
    };
}
