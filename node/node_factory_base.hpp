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

#include <node/node_base.hpp>

namespace bzapi
{
    class node_factory_base
    {
    public:
        virtual std::shared_ptr<node_base> create_node(std::shared_ptr<bzn::asio::io_context_base> io_context
            , std::shared_ptr<bzn::beast::websocket_base> ws_factory
            , const std::string& host, uint16_t port) = 0;

        virtual ~node_factory_base() = default;
    };
}
