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

            sockaddr_in local = this->make_addr(0);
            if (bind(sock, (sockaddr*)&local, sizeof(local)) == -1)
            {
                throw std::runtime_error("unable to bind udp socket");
            }

            struct sockaddr_in my_addr;
            socklen_t addrlen = sizeof(my_addr);
            if (getsockname(sock, (struct sockaddr *)&my_addr, &addrlen) == 0
                && my_addr.sin_family == AF_INET && addrlen == sizeof(my_addr))
            {
                this->my_id = ntohs(my_addr.sin_port);
            }
            else
            {
                throw std::runtime_error("error determining local port");
            }
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
            if (this->their_id)
            {
                this->send_signal();
            }
            else
            {
                this->deferred_signal = true;
            }

        }

        void send_signal()
        {
            assert(this->their_id);
            struct sockaddr_in their_addr = this->make_addr(this->their_id);
            if (!sendto(this->sock, &this->error_val, sizeof(this->error_val), 0, (sockaddr*)&their_addr, sizeof(their_addr)))
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
            this->set_error(0);
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
            this->db = std::move(db_ptr);
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

        static sockaddr_in make_addr(uint16_t port)
        {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(sockaddr_in));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            return addr;
        }
    };
}
