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

#include <node/node_factory.hpp>
#include <node/node.hpp>

using namespace bzapi;

std::shared_ptr<node_base>
node_factory::create_node(std::shared_ptr<bzn::asio::io_context_base> io_context
    , std::shared_ptr<bzn::beast::websocket_base> ws_factory
    , const std::string& host, uint16_t port)
{
    return std::make_shared<node>(io_context, ws_factory, host, port);
}
