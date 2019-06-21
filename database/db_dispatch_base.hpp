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

#include <include/bluzelle.hpp>
#include <proto/database.pb.h>
#include <swarm/swarm_base.hpp>

namespace bzapi
{
    using db_response_handler_t = std::function<void(const database_response &response, const boost::system::error_code &error)>;

    class db_dispatch_base
    {
    public:
        virtual ~db_dispatch_base() = default;
        virtual void has_uuid(std::shared_ptr<swarm_base> swarm, uuid_t uuid, std::function<void(db_error)> callback) = 0;
        virtual void create_uuid(std::shared_ptr<swarm_base> swarm, uuid_t uuid, uint64_t max_size, bool random_evict, std::function<void(db_error)> callback) = 0;

        virtual void send_message_to_swarm(std::shared_ptr<swarm_base> swarm, uuid_t uuid, database_msg& msg, send_policy policy, db_response_handler_t handler) = 0;
    };

}
