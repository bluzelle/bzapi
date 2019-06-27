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
#include <memory>

namespace bzapi
{
    class async_database;

    /// response object used to determine the outcome of an asynchronous
    /// bzapi swarmDB operation. The caller can be notified of a result by calling
    // set_signal_id() with the port of a UDP socket which will be written to when
    // a result or error is available, at which time get_result() will not block.
    // Alternatively, the caller can call get_result() at any time, which will block
    // until the result or error is available.
    class response
    {
    public:
        virtual ~response() = default;

        /// Set the udp localhost port to be notified when the result is ready to read.
        /// @param signal_id - UDP port number to notify on
        /// @result - UDP port the notification will come from
        virtual int set_signal_id(int signal_id) = 0;

        /// Retrieve the result of the asynchronous operation.
        /// The result is a JSON structure with fields dependent on the operation in question.
        /// In general it will at least contain "result" or "error" fields.
        /// @result - JSON result structure
        virtual std::string get_result() = 0;

        /// Retrieve the database object that is the result of either an async_create_db
        /// or async_open_db call.
        /// @return - database object or nullptr
        virtual std::shared_ptr<async_database> get_db() = 0;

        /// Retrieve the error id for the asynchrounous operation.
        /// @return - error value, or zero if successful
        virtual int get_error() = 0;
    };
}
