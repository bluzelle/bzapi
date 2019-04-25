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

namespace bzapi {
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

        std::shared_ptr<bzapi::database> get_db()
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

        void set_db(std::shared_ptr<bzapi::database> db_ptr)
        {
            this->db = db_ptr;
        }

    protected:
        int my_id = 0;
        int their_id = 0;
        std::string result_str;
        std::atomic<bool> ready = false;
        std::shared_ptr<bzapi::database> db;

        virtual void signal(int error) = 0;
    };
}