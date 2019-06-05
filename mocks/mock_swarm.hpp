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
#include <swarm/swarm_base.hpp>

namespace bzapi
{
    class mock_swarm : public swarm_base
    {
    public:
        MOCK_METHOD2(has_uuid, void(const uuid_t& uuid, std::function<void(db_error)> callback));
        MOCK_METHOD4(create_uuid, void(const uuid_t& uuid, uint64_t max_size, bool random_evict, std::function<void(db_error)> callback));
        MOCK_METHOD1(initialize, void(completion_handler_t));
        MOCK_METHOD1(add_nodes, void(const std::vector<std::pair<node_id_t, bzn::peer_address_t>>&));
        MOCK_METHOD2(send_request, int(std::shared_ptr<bzn_envelope>, send_policy));
        MOCK_METHOD2(register_response_handler, bool(payload_t, swarm_response_handler_t));
        MOCK_METHOD0(get_status, std::string(void));
        MOCK_METHOD0(honest_majority_size, size_t(void));
    };
}