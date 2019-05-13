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
    class async_database;

    class response
    {
    public:
        response()
        {}

        virtual ~response()
        {}

        // consumer
        virtual int set_signal_id(int signal_id) = 0;

        virtual std::string get_result() = 0;

        virtual std::shared_ptr<async_database> get_db() = 0;

        // producer
        virtual void set_result(const std::string& result) = 0;

        virtual void set_ready() = 0;

        virtual void set_error(int error) = 0;

        virtual int get_error() = 0;

        virtual void set_db(std::shared_ptr<async_database> db_ptr) = 0;

    };
}
