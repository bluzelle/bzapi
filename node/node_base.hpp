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
#include <cstdint>


namespace bzapi
{
    using websocket = uint64_t;
    using node_message_handler = std::function<bool(const std::string& data)>;

    // establishes and maintains connection with node
    // sends messages to node
    // receives incoming messages and forwards them to owner
    class node_base
    {
    public:

        virtual ~node_base() = default;

        virtual void register_message_handler(node_message_handler handler) = 0;
        virtual void send_message(const std::string& msg, completion_handler_t callback) = 0;
        virtual void back_off(bool value) = 0;
   };
}
