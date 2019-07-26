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

#include <database/db_dispatch_base.hpp>

namespace bzapi
{
    // fills in header, sends and tracks outgoing database requests
    // handles incoming database responses
    // applies collation policy and forwards acceptable responses
    // handles response timeout and resend
    class db_dispatch : public db_dispatch_base, public std::enable_shared_from_this<db_dispatch>
    {
    public:
        db_dispatch(std::shared_ptr<bzn::asio::io_context_base> io_context);

        void has_uuid(std::shared_ptr<swarm_base> swarm, uuid_t uuid, std::function<void(db_error)> callback) override;

        void create_uuid(std::shared_ptr<swarm_base> swarm, uuid_t uuid, uint64_t max_size, bool random_evict, std::function<void(db_error)> callback) override;

        void send_message_to_swarm(std::shared_ptr<swarm_base> swarm, uuid_t uuid, database_msg& msg, send_policy policy, db_response_handler_t handler) override;

    private:
        using nonce_t = uint64_t;

        struct msg_info
        {
            std::shared_ptr<swarm_base> swarm;
            std::shared_ptr<bzn_envelope> request;
            send_policy policy;
            uint64_t responses_required;
            std::shared_ptr<bzn::asio::steady_timer_base> retry_timer;
            std::shared_ptr<bzn::asio::steady_timer_base> timeout_timer;
            std::map<uuid_t, database_response> responses;
            db_response_handler_t handler;
        };

        const std::shared_ptr<bzn::asio::io_context_base> io_context;
        nonce_t next_nonce = 1;
        std::map<nonce_t, msg_info> messages;

        void setup_request_policy(msg_info& info, send_policy policy, nonce_t nonce);
        void handle_request_timeout(const boost::system::error_code& ec, nonce_t nonce);
        bool handle_swarm_response(const bzn_envelope& response);
        bool qualify_response(msg_info& info, const uuid_t& sender) const;
        bool responses_are_equal(const database_response& r1, const database_response& r2) const;
        void setup_client_timeout(nonce_t nonce, msg_info& info);
        void register_swarm_handler(std::shared_ptr<swarm_base> swarm);

    };
}
