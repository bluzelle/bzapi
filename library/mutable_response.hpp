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

#include <include/response.hpp>

namespace bzapi
{
    class async_database;

    class mutable_response : public response
    {
    public:
        virtual void set_result(const std::string& result) = 0;

        virtual void set_ready() = 0;

        virtual void set_error(int error) = 0;

        virtual void set_db(std::shared_ptr<async_database> db_ptr) = 0;

    };
}
