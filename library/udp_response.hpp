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

#include <library/mutable_response.hpp>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdexcept>
#include <string.h>
#include <functional>

namespace
{
    std::string ERROR_RESULT{"{ error: \"An exception occurred getting result\""};
}

namespace bzapi
{
    class udp_response : public mutable_response
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

        ~udp_response()
        {
        }

        int set_signal_id(int signal_id) override
        {
            std::scoped_lock<std::mutex> lock(this->mutex);

            this->their_id = signal_id;
            if (this->deferred_signal)
            {
                this->send_signal();
                this->deferred_signal = false;
            }

            return this->my_id;
        }

        void signal(int error)
        {
            std::scoped_lock<std::mutex> lock(this->mutex);

            this->error_val = error;
            if (their_id)
            {
                send_signal();
            }
            else
            {
                this->deferred_signal = true;
            }

        }

        void send_signal()
        {
            assert(their_id);
            struct sockaddr_in their_addr;
            memset(&their_addr, 0, sizeof(sockaddr_in));
            their_addr.sin_family = AF_INET;
            their_addr.sin_port = htons(their_id);
            their_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            int res = sendto(sock, &error_val, sizeof(error_val), 0, (sockaddr*)&their_addr, sizeof(their_addr));
            if (!res)
            {
                LOG(error) << "Error: " << errno << " sending data to socket";
            }
        }

        void set_result(const std::string& result) override
        {
            this->result_str = result;
        }

        void set_ready() override
        {
            this->prom.set_value(0);
            this->signal(0);
        }

        void set_error(int error) override
        {
            this->prom.set_value(error);
            this->signal(error);
        }

        int get_error() override
        {
            return this->error_val;
        }

        std::string get_result() override
        {
            try
            {
                this->prom.get_future().get();
                return this->result_str;
            }
            CATCHALL();
            return ERROR_RESULT;
        }

        void set_db(std::shared_ptr<async_database> db_ptr) override
        {
            this->db = db_ptr;
        }

        std::shared_ptr<async_database> get_db() override
        {
            return this->db;
        }

    private:
        int my_id = 0;
        int their_id = 0;
        std::string result_str;
        std::shared_ptr<async_database> db;
        int sock;
        int error_val = 0;
        bool deferred_signal = false;
        std::promise<int> prom;
        std::mutex mutex;
    };
}
