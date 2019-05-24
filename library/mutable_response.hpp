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

#include <include/response.hpp>

namespace bzapi
{
    class async_database;

    /// response object used to determine the outcome of an asynchronous
    /// bzapi swarmDB operation. The caller can be notified of a result by calling
    // set_signal_id() with the port of a UDP socket which will be written to when
    // a result or error is available, at which time get_result() will not block.
    // Alternatively, the caller can call get_result() at any time, which will block
    // until the result or error is available.
    class mutable_response : public response
    {
    public:
        virtual void set_result(const std::string& result) = 0;

        virtual void set_ready() = 0;

        virtual void set_error(int error) = 0;

        virtual void set_db(std::shared_ptr<async_database> db_ptr) = 0;

    };
}
