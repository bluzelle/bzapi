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

#include <database/db_impl.hpp>
#include <boost/format.hpp>

using namespace bzapi;

namespace
{
    const std::chrono::milliseconds REQUEST_RETRY_TIME{std::chrono::milliseconds(1500)};
    const std::chrono::milliseconds BROADCAST_RETRY_TIME{std::chrono::milliseconds(3000)};
}

db_impl::db_impl(std::shared_ptr<bzn::asio::io_context_base> io_context
    , std::shared_ptr<swarm_base> swarm
    , uuid_t uuid)
    : io_context(io_context), swarm(swarm), uuid(uuid)
{
}

db_impl::~db_impl()
{
}

void
db_impl::initialize(completion_handler_t handler)
{
    this->swarm->register_response_handler(bzn_envelope::kDatabaseResponse
        , [weak_this = weak_from_this()](const uuid_t& /*uuid*/, const bzn_envelope& env)->bool
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

    this->swarm->initialize(handler);
}

void
db_impl::send_message_to_swarm(database_msg& msg, send_policy policy, db_response_handler_t handler)
{
    auto nonce = this->next_nonce++;

    auto header = new database_header;
    header->set_db_uuid(this->uuid);
    header->set_nonce(nonce);
    // what about point_of_contact and request_hash?

    msg.set_allocated_header(header);

    auto env = std::make_shared<bzn_envelope>();
    env->set_database_msg(msg.SerializeAsString());
    env->set_timestamp(this->now());

    // store message info
    msg_info info;
    info.request = env;
    this->setup_request_policy(info, policy, nonce);
    info.handler = handler;
    messages[nonce] = info;

    LOG(debug) << "Sending database request for message " << nonce;
    this->swarm->send_request(env, policy);
}

std::string
db_impl::swarm_status()
{
    return this->swarm->get_status();
}

uint64_t
db_impl::now() const
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

void
db_impl::setup_request_policy(msg_info& info, send_policy policy, nonce_t nonce)
{
    info.policy = policy;

    // should probably split into send_policy and failure_policy
    if (policy != send_policy::fastest)
    {
        info.responses_required = this->swarm->honest_majority_size();
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
db_impl::handle_request_timeout(const boost::system::error_code& ec, nonce_t nonce)
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
    this->swarm->send_request(info.request, send_policy::broadcast);
}

bool
db_impl::handle_swarm_response(const bzn_envelope& response)
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
    // TODO: this isn't ideal if we wan't to enable/disable signatures globally
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
db_impl::qualify_response(bzapi::db_impl::msg_info &info, const uuid_t& sender) const
{
    auto num_responses = info.responses.size();
    LOG(debug) << boost::format("%1% of %2% responses received") % num_responses % info.responses_required;

    if (num_responses < info.responses_required)
    {
        return false;
    }

    if (info.responses_required == 1)
    {
        assert(num_responses == 1);
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
db_impl::responses_are_equal(const database_response& r1, const database_response& r2) const
{
    // TODO: this needs to be made better
    return r1.response_case() == r2.response_case() &&
           r1.SerializeAsString() == r2.SerializeAsString();
}
