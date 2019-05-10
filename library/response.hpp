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
#include <stdexcept>
#include <string.h>
#include <memory>
#include <functional>

namespace bzapi
{
    class database;

    class response
    {
    public:
        response()
        {}

        virtual ~response()
        {}

        virtual void exec(std::function<void(void)> func)
        { exec_func = func; }

        // consumer
        virtual int get_signal_id(int theirs) = 0;

        bool is_ready()
        {
            return this->ready;
        }

        virtual std::string get_result() = 0;

        std::shared_ptr<database> get_db()
        {
            return this->db;
        }

        // producer
        virtual void set_result(const std::string& result) = 0;

        virtual void set_ready() = 0;

        virtual void set_error(int error) = 0;

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
        std::function<void(void)> exec_func;

        virtual void signal(int error) = 0;
    };
}
