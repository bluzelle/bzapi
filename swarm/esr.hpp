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


#include <swarm/esr_base.hpp>
#include <utils/esr_peer_info.hpp>

namespace bzapi
{
    class esr : public esr_base
    {
    public:
        std::vector<std::string> get_swarm_ids(const std::string &url) override
        {
            return bzn::utils::esr::get_swarm_ids(url);
        }

        std::vector<std::string> get_peer_ids(const uuid_t& swarm_id, const std::string& url) override
        {
            return bzn::utils::esr::get_peer_ids(swarm_id, url);
        }

        bzn::peer_address_t get_peer_info(const uuid_t &swarm_id, const std::string &peer_id, const std::string &url) override
        {
            return bzn::utils::esr::get_peer_info(swarm_id, peer_id, url);
        }
    };
}
