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

#include <include/database.hpp>
#include <database/async_database_impl.hpp>

namespace bzapi
{
    class database_impl : public database
    {
    public:
        database_impl(std::shared_ptr<async_database> db);

        std::string create(const std::string& key, const std::string& value, uint64_t expiry) override;
        std::string read(const std::string& key) override;
        std::string update(const std::string& key, const std::string& value) override;
        std::string remove(const std::string& key) override;

        std::string quick_read(const std::string& key) override;
        std::string has(const std::string& key) override;
        std::string keys() override;
        std::string size() override;
        std::string expire(const std::string& key, uint64_t expiry) override;
        std::string persist(const std::string& key) override;
        std::string ttl(const std::string& key) override;

        std::string writers() override;
        std::string add_writer(const std::string& writer) override;
        std::string remove_writer(const std::string& writer) override;

        std::string swarm_status() override;

    private:
        std::shared_ptr<async_database> db;
    };
}
