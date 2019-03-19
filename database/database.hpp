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

#include <database/db_base.hpp>
#include <database/db_impl_base.hpp>
#include <swarm/swarm_base.hpp>

namespace bzapi
{
    using nonce_t = uint64_t;

    class database : public db_base, public std::enable_shared_from_this<database>
    {
    public:

        database(std::shared_ptr<db_impl_base> db_impl);

        void open(completion_handler_t handler);

        void create(const key_t& key, const value_t& value, void_handler_t handler);
        void read(const key_t& key, value_handler_t handler);
        void update(const key_t& key, const value_t& value, void_handler_t handler);
        void remove(const key_t& key, void_handler_t handler);

        void quick_read(const key_t& key, value_handler_t handler);
        void has(const key_t& key, value_handler_t handler);
        void keys(vector_handler_t handler);
        void size(value_handler_t handler);
        void expire(const key_t& key, expiry_t expiry, void_handler_t handler);
        void persist(const key_t& key, void_handler_t handler);
        void ttl(const key_t& key, value_handler_t handler);

        std::string swarm_status();

    private:

        std::shared_ptr<db_impl_base> db_impl;

        void translate_swarm_response(const database_response& response, const boost::system::error_code& ec
            , std::function<void(const database_response& response, db_error err, const std::string& msg)> handler);

    };
}