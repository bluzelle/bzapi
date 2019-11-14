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
#include <swarm/esr_base.hpp>

namespace bzapi
{
    class mock_esr : public esr_base
    {
    public:
        MOCK_METHOD1(get_swarm_ids, std::vector<std::string>(const std::string &));
        MOCK_METHOD2(get_peer_ids, std::vector<std::string>(const std::string&, const std::string&));
        MOCK_METHOD3(get_peer_info,bzn::peer_address_t(const uuid_t&, const std::string&, const std::string&));
    };
}
