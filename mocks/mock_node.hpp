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
#include <node/node_base.hpp>

namespace bzapi
{
    class mock_node : public node_base
    {
    public:

        MOCK_METHOD1(register_message_handler, void(node_message_handler handler));
        MOCK_METHOD2(send_message, void(const std::string& msg, completion_handler_t callback));
    };
}
