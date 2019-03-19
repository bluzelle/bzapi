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

#include <cstdint>
#include <boost_asio_beast.hpp>
#include <bzapi.hpp>

namespace bzapi
{
    using websocket = uint64_t;
    using node_message_handler = std::function<bool(const char *data, uint64_t len)>;

    // establishes and maintains connection with node
    // sends messages to node
    // receives incoming messages and forwards them to owner
    class node_base
    {
    public:

        virtual ~node_base() = default;

        virtual void register_message_handler(node_message_handler handler) = 0;
        virtual void send_message(const char *msg, size_t len, completion_handler_t callback) = 0;
   };
}