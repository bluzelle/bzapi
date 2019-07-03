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
#include <utils/peer_address.hpp>

namespace bzapi
{
    class esr_base
    {
    public:
        virtual ~esr_base() = default;

        virtual std::vector<std::string> get_swarm_ids(const std::string& esr_address, const std::string& url) = 0;
        virtual std::vector<std::string> get_peer_ids(const uuid_t& swarm_id, const std::string& esr_address
            , const std::string& url) = 0;
        virtual bzn::peer_address_t get_peer_info(const uuid_t& swarm_id, const std::string& peer_id
            , const std::string& esr_address, const std::string& url) = 0;
    };
}
