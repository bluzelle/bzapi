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
#include <include/boost_asio_beast.hpp>
#include <node/node_factory_base.hpp>
#include <proto/bluzelle.pb.h>
#include <proto/status.pb.h>
#include <utils/peer_address.hpp>


namespace bzapi
{
    enum class send_policy
    {
        normal,
        fastest,
        broadcast
    };

    using payload_t = bzn_envelope::PayloadCase;
    using node = uint64_t;
    using swarm_response_handler_t = std::function<bool(const uuid_t& uuid, const bzn_envelope&)>;

    class swarm_base
    {
    public:
        virtual ~swarm_base() = default;

        virtual void initialize(completion_handler_t handler) = 0;

        virtual void sign_and_date_request(bzn_envelope& request, send_policy policy) = 0;

        virtual int send_request(const bzn_envelope& request, send_policy policy) = 0;

        virtual bool register_response_handler(payload_t type, swarm_response_handler_t handler) = 0;

        virtual std::string get_status() = 0;

        virtual size_t honest_majority_size() = 0;
    };
}
