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

#include <database/db_impl_base.hpp>

namespace bzapi
{
    // fills in header, sends and tracks outgoing database requests
    // handles incoming database responses
    // applies collation policy and forwards acceptable responses
    // handles response timeout and resend
    class db_impl : public db_impl_base, public std::enable_shared_from_this<db_impl>
    {
    public:
        db_impl(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<swarm_base> swarm, uuid_t uuid);
        ~db_impl();

        void initialize(completion_handler_t handler) override;

        void send_message_to_swarm(database_msg& msg, send_policy policy, db_response_handler_t handler) override;

        std::string swarm_status() override;

    private:
        using nonce_t = uint64_t;

        struct msg_info
        {
            std::shared_ptr<bzn_envelope> request;
            send_policy policy;
            uint64_t responses_required;
            std::shared_ptr<bzn::asio::steady_timer_base> retry_timer;
            std::shared_ptr<bzn::asio::steady_timer_base> timeout_timer;
            std::map<uuid_t, database_response> responses;
            db_response_handler_t handler;
        };

        std::shared_ptr<bzn::asio::io_context_base> io_context;
        std::shared_ptr<swarm_base> swarm;
        uuid_t uuid;
        nonce_t next_nonce = 1;
        std::map<nonce_t, msg_info> messages;

        void setup_request_policy(msg_info& info, send_policy policy, nonce_t nonce);
        uint64_t now() const;
        void handle_request_timeout(const boost::system::error_code& ec, nonce_t nonce);
        bool handle_swarm_response(/*const uuid_t& uuid, */const bzn_envelope& response);
        bool qualify_response(msg_info& info, const uuid_t& sender) const;
        bool responses_are_equal(const database_response& r1, const database_response& r2) const;
        void setup_client_timeout(nonce_t nonce, msg_info& info);
    };
}