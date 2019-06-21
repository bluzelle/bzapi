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
#include <swarm/swarm_base.hpp>

namespace bzapi
{
    class mock_swarm : public swarm_base
    {
    public:
        MOCK_METHOD2(has_uuid, void(const uuid_t& uuid, std::function<void(db_error)> callback));
        MOCK_METHOD4(create_uuid, void(const uuid_t& uuid, uint64_t max_size, bool random_evict, std::function<void(db_error)> callback));
        MOCK_METHOD1(initialize, void(completion_handler_t));
        MOCK_METHOD2(sign_and_date_request, void(bzn_envelope&, send_policy));
        MOCK_METHOD2(send_request, int(const bzn_envelope&, send_policy));
        MOCK_METHOD2(register_response_handler, bool(payload_t, swarm_response_handler_t));
        MOCK_METHOD0(get_status, std::string(void));
        MOCK_METHOD0(honest_majority_size, size_t(void));
    };
}
