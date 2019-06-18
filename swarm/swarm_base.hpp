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

#include <proto/bluzelle.pb.h>
#include <boost_asio_beast.hpp>
#include <node/node_factory_base.hpp>
#include <proto/status.pb.h>
#include <bluzelle.hpp>
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

        virtual void add_nodes(const std::vector<std::pair<node_id_t, bzn::peer_address_t>>& nodes) = 0;

        virtual int send_request(std::shared_ptr<bzn_envelope> request, send_policy policy) = 0;

        virtual bool register_response_handler(payload_t type, swarm_response_handler_t handler) = 0;

        virtual std::string get_status() = 0;

        virtual size_t honest_majority_size() = 0;
    };
}