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

#include <string>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string.h>
#include <memory>

namespace bzapi
{
    class database;

    class response
    {
    public:
        virtual ~response() = default;

        // consumer
        int get_signal_id(int theirs)
        {
            this->their_id = theirs;
            return this->my_id;
        }

        bool is_ready()
        {
            return this->ready;
        }

        std::string get_result()
        {
            if (ready)
            {
                return this->result_str;
            }

            return {};
        }

        std::shared_ptr<database> get_db()
        {
            return this->db;
        }

        // producer
        void set_result(const std::string& result)
        {
            this->result_str = result;
        }

        void set_ready()
        {
            this->set_error(0);
        }

        void set_error(int error)
        {
            this->ready = true;
            this->signal(error);
        }

        void set_db(std::shared_ptr<database> db_ptr)
        {
            this->db = db_ptr;
        }

    protected:
        int my_id = 0;
        int their_id = 0;
        std::string result_str;
        std::atomic<bool> ready = false;
        std::shared_ptr<database> db;

        virtual void signal(int error) = 0;
    };

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
            local.sin_addr.s_addr = INADDR_ANY;
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
            their_addr.sin_family = AF_INET;
            their_addr.sin_port = htons(their_id);
            their_addr.sin_addr.s_addr = INADDR_LOOPBACK;
            sendto(sock, &error, sizeof(error), 0, (sockaddr*)&their_addr, sizeof(their_addr));
        }

        int sock;
    };
}
