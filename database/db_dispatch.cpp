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

#include <database/db_dispatch.hpp>
#include <boost/format.hpp>

using namespace bzapi;

namespace
{
    const std::chrono::milliseconds REQUEST_RETRY_TIME{std::chrono::milliseconds(1500)};
    const std::chrono::milliseconds BROADCAST_RETRY_TIME{std::chrono::milliseconds(3000)};
}

db_dispatch::db_dispatch(std::shared_ptr<bzn::asio::io_context_base> io_context)
    : io_context(std::move(io_context))
{
}

void
db_dispatch::send_message_to_swarm(std::shared_ptr<swarm_base> swarm, uuid_t db_uuid, database_msg& msg
    , send_policy policy, db_response_handler_t handler)
{
    auto nonce = this->next_nonce++;

    auto header = new database_header;
    header->set_db_uuid(db_uuid);
    header->set_nonce(nonce);
    msg.set_allocated_header(header);

    auto env = std::make_shared<bzn_envelope>();
    env->set_database_msg(msg.SerializeAsString());
    swarm->sign_and_date_request(*env, policy);

    // store message info
    msg_info info;
    info.swarm = swarm;
    info.request = env;
    info.handler = handler;
    this->setup_client_timeout(nonce, info);
    this->setup_request_policy(info, policy, nonce);

    this->messages[nonce] = info;

    LOG(debug) << "Sending database request for message " << nonce;
    this->register_swarm_handler(swarm);
    swarm->send_request(*env, policy);
}

void
db_dispatch::setup_request_policy(msg_info& info, send_policy policy, nonce_t nonce)
{
    info.policy = policy;

    // consider splitting into send_policy and failure_policy
    if (policy != send_policy::fastest)
    {
        info.responses_required = info.swarm->honest_majority_size();
        info.retry_timer = this->io_context->make_unique_steady_timer();
        info.retry_timer->expires_from_now(REQUEST_RETRY_TIME);
        info.retry_timer->async_wait([weak_this = weak_from_this(), nonce](const auto& ec)
        {
            auto strong_this = weak_this.lock();
            if (strong_this)
            {
                strong_this->handle_request_timeout(ec, nonce);
            }
        });
    }
    else
    {
        info.responses_required = 1;
    }
}

void
db_dispatch::handle_request_timeout(const boost::system::error_code& ec, nonce_t nonce)
{
    if (ec == boost::asio::error::operation_aborted)
    {
        return;
    }

    if (ec)
    {
        LOG(error) << "handle_request_timeout error: " << ec.message();
        return;
    }

    static uint64_t timeouts;
    std::cout << ++timeouts << " timeouts" << std::endl;

    auto i = this->messages.find(nonce);
    if (i == this->messages.end())
    {
        // message has already been processed
        LOG(debug) << "Ignoring timeout for already processed message: " << nonce;
        return;
    }

    auto info = i->second;
    LOG(debug) << boost::format("request timeout for message %1% - %2% of required %3% responses received")
                  % nonce % info.responses.size() % info.responses_required;

    info.retry_timer->expires_from_now(BROADCAST_RETRY_TIME);
    info.retry_timer->async_wait([weak_this = weak_from_this(), nonce](const auto& ec2)
    {
        auto strong_this = weak_this.lock();
        if (strong_this)
        {
            strong_this->handle_request_timeout(ec2, nonce);
        }
    });

    // broadcast the retry
    info.swarm->send_request(*info.request, send_policy::broadcast);
}

bool
db_dispatch::handle_swarm_response(const bzn_envelope& response)
{
    database_response db_response;
    if (!db_response.ParseFromString(response.database_response()))
    {
        LOG(error) << "Failed to parse database response from swarm: " << response.DebugString().substr(0, MAX_MESSAGE_SIZE);
        return true;
    }

    auto nonce = db_response.header().nonce();
    LOG(debug) << "Got response for message " << nonce;

    auto i = this->messages.find(nonce);
    if (i == this->messages.end())
    {
        LOG(debug) << "Ignoring db response for unknown or already processed message: " << nonce;
        return false;
    }

    // all responses apart from quickreads require a signature
    // TODO: this isn't ideal if we want to enable/disable signatures globally
    if (!db_response.has_quick_read() && response.signature().empty())
    {
        LOG(debug) << "Dropping unsigned response for message: " << nonce;
        return false;
    }

    auto& info = i->second;
    info.responses[response.sender()] = std::move(db_response);
    if (this->qualify_response(info, response.sender()))
    {
        // TODO: how do subscription responses work here?
        boost::system::error_code ec;
        info.handler(info.responses[response.sender()], ec);
        LOG(debug) << "Done processing db response for message " << nonce;
        this->messages.erase(nonce);
    }

    return false;
}

bool
db_dispatch::qualify_response(bzapi::db_dispatch::msg_info &info, const uuid_t& sender) const
{
    auto num_responses = info.responses.size();
    if (num_responses < info.responses_required)
    {
        LOG(debug) << boost::format("%1% of %2% responses received") % num_responses % info.responses_required;
        return false;
    }

    if (info.responses_required == 1)
    {
        assert(num_responses == 1);
        LOG(debug) << boost::format("%1% of %2% responses received") % num_responses % info.responses_required;
        return true;
    }

    if (info.responses_required > 1)
    {
        size_t matches = 0;
        for (const auto& r : info.responses)
        {
            if (this->responses_are_equal(r.second, info.responses[sender]))
            {
                if (++matches >= info.responses_required)
                {
                    LOG(debug) << boost::format("%1% of %2% matching responses received") % matches % info.responses_required;
                    return true;
                }
            }
            else
            {
                LOG(warning) << "Non-matching database responses received from " << sender << " vs. " << r.first;
            }
        }
    }

    return false;
}

bool
db_dispatch::responses_are_equal(const database_response& r1, const database_response& r2) const
{
    return r1.response_case() == r2.response_case() &&
           r1.SerializeAsString() == r2.SerializeAsString();
}

void
db_dispatch::setup_client_timeout(nonce_t nonce, msg_info& info)
{
    info.timeout_timer = this->io_context->make_unique_steady_timer();
    info.timeout_timer->expires_from_now(std::chrono::milliseconds {std::chrono::seconds(get_timeout())});
    info.timeout_timer->async_wait([weak_this = weak_from_this(), nonce](const auto& ec)
    {
        if (!ec)
        {
            auto strong_this = weak_this.lock();
            if (strong_this)
            {
                auto i = strong_this->messages.find(nonce);
                if (i != strong_this->messages.end())
                {
                    LOG(warning) << "Request timeout querying swarm";
                    database_error error;
                    error.set_message("Request timeout");
                    database_response response;
                    response.set_allocated_error(new database_error(error));
                    i->second.handler(response, boost::system::error_code{});
                    strong_this->messages.erase(nonce);
                }
            }
        }
    });
}

void
db_dispatch::has_uuid(std::shared_ptr<swarm_base> swarm, uuid_t db_uuid, std::function<void(db_error)> callback)
{
    bzn_envelope env;
    database_msg request;
    database_header header;
    request.set_allocated_header(new database_header(header));
    request.set_allocated_has_db(new database_has_db());

    this->register_swarm_handler(swarm);
    this->send_message_to_swarm(swarm, db_uuid, request, send_policy::normal, [db_uuid, callback](auto response, auto err)
    {
        if (err)
        {
            callback(db_error::database_error);
        }
        else
        {
            // TODO: need to check for has_error
            if (response.has_db().uuid() != db_uuid)
            {
                LOG(error) << "Invalid uuid response to has_db: " << response.has_db().uuid();
                callback(db_error::database_error);
            }
            else
            {
                callback(response.has_db().has() ? db_error::success : db_error::no_database);
            }
        }
    });
}

void
db_dispatch::create_uuid(std::shared_ptr<swarm_base> swarm, uuid_t db_uuid, uint64_t max_size, bool random_evict, std::function<void(db_error)> callback)
{
    bzn_envelope env;
    database_msg request;
    database_header header;
    request.set_allocated_header(new database_header(header));
    database_create_db create;
    create.set_max_size(max_size);
    create.set_eviction_policy(random_evict ? database_create_db_eviction_policy_type_RANDOM
        : database_create_db_eviction_policy_type_NONE);
    request.set_allocated_create_db(new database_create_db(create));

    this->register_swarm_handler(swarm);
    this->send_message_to_swarm(swarm, db_uuid, request, send_policy::normal, [db_uuid, callback](auto response, auto err)
    {
        if (err)
        {
            callback(db_error::database_error);
        }
        else
        {
            if (response.has_error())
            {
                callback(db_error::database_error);
            }
            else
            {
                callback(db_error::success);
            }
        }
    });
}

void
db_dispatch::register_swarm_handler(std::shared_ptr<swarm_base> swarm)
{
    swarm->register_response_handler(bzn_envelope::kDatabaseResponse
        , [weak_this = weak_from_this()](const uuid_t& /*uuid*/, const bzn_envelope& env)
        {
            auto strong_this = weak_this.lock();
            if (strong_this)
            {
                return strong_this->handle_swarm_response(env);
            }
            else
            {
                return true;
            }
        });
}