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

#include <library/response.hpp>
namespace bzapi
{
    class udp_response : public response
    {
    public:
        udp_response()
        {
            sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (!sock)
            {
                throw std::runtime_error("unable to create udp socket");
            }

            sockaddr_in local;
            memset(&local, 0, sizeof(sockaddr_in));
            local.sin_family = AF_INET;
            local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            local.sin_port = 0; //randomly selected port
            if (bind(sock, (sockaddr*)&local, sizeof(local)) == -1)
            {
                throw std::runtime_error("unable to bind udp socket");
            }

            struct sockaddr_in sin;
            socklen_t addrlen = sizeof(sin);
            if (getsockname(sock, (struct sockaddr *)&sin, &addrlen) == 0
                && sin.sin_family == AF_INET && addrlen == sizeof(sin))
            {
                my_id = ntohs(sin.sin_port);
            }
            else
            {
                throw std::runtime_error("error determining local port");
            }
        }

        void signal(int error) override
        {
            struct sockaddr_in their_addr;
            memset(&their_addr, 0, sizeof(sockaddr_in));
            their_addr.sin_family = AF_INET;
            their_addr.sin_port = htons(their_id);
            their_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            int res = sendto(sock, &error, sizeof(error), 0, (sockaddr*)&their_addr, sizeof(their_addr));
            if (!res)
            {
                LOG(error) << "Error: " << errno << " sending data to socket";
            }
        }

        int sock;
    };
}
