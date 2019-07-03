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

#include <string>


namespace bzn
{
    struct peer_address_t
    {
        peer_address_t(std::string host, uint16_t port, uint16_t http_port, std::string name, std::string uuid)
            : host(std::move(host))
            , port(port)
            , http_port(http_port)
            , name(std::move(name))
            , uuid(std::move(uuid))
        {
        };

        bool operator==(const peer_address_t& other) const
        {
            if (&other == this)
            {
                return true;
            }

            return this->host == other.host && this->port == other.port && this->http_port == other.http_port && this->uuid == other.uuid;
        }

        const std::string host;
        const uint16_t    port;
        const uint16_t    http_port;
        const std::string name;
        const std::string uuid;
    };
}
