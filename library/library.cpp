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

#include <include/bluzelle.hpp>
#include <include/boost_asio_beast.hpp>
#include <crypto/crypto.hpp>
#include <database/database_impl.hpp>
#include <database/db_impl.hpp>
#include <include/bzapi.hpp>
#include <library/log.hpp>
#include <library/udp_response.hpp>
#include <swarm/swarm.hpp>
#include <swarm/swarm_factory.hpp>
#include <swarm/esr.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <json/reader.h>
#include <json/value.h>


namespace
{
    std::shared_ptr<bzn::asio::io_context_base> io_context;
    std::shared_ptr<std::thread> io_thread;
    std::shared_ptr<bzapi::swarm_factory> the_swarm_factory;
    std::shared_ptr<bzapi::crypto_base> the_crypto;
    std::shared_ptr<bzn::beast::websocket_base> ws_factory;
    std::string error_str = "Not Initialized";
    int error_val = -1;
    uint64_t DEFAULT_TIMEOUT = 30;
    uint64_t api_timeout = DEFAULT_TIMEOUT;
}

namespace bzapi
{
    // these need to be accessible to unit tssts
    std::shared_ptr<bzn::asio::io_context_base> io_context;
    std::shared_ptr<bzapi::swarm_factory> the_swarm_factory;
    std::shared_ptr<bzapi::crypto_base> the_crypto;
    std::shared_ptr<bzn::beast::websocket_base> ws_factory;
    std::shared_ptr<bzapi::db_impl_base> db_dispatcher;
    std::shared_ptr<bzapi::esr_base> the_esr{new bzapi::esr};
    bool initialized = false;

    std::shared_ptr<mutable_response>
    make_response()
    {
        return std::make_shared<udp_response>();
    }

    std::shared_ptr<bzapi::db_impl_base>
    get_db_dispatcher()
    {
        return db_dispatcher;
    }

    void
    do_bad_endpoint(const std::string& endpoint)
    {
        LOG(error) << "bad swarm node endpoint: " << endpoint;
        error_str = "Bad Endpoint";
        error_val = -1;
        throw(std::runtime_error("bad node endpoint: " + endpoint));
    }

    std::pair<std::string, uint16_t>
    parse_endpoint(const std::string& endpoint)
    {
        // format should be ws://n.n.n.n:p
        // TODO: support for hostnames
        std::string addr;
        uint64_t port;

        auto offset = endpoint.find(':', 5);
        if (offset > endpoint.size() || endpoint.substr(0, 5) != "ws://")
        {
            do_bad_endpoint(endpoint);
        }

        try
        {
            addr = endpoint.substr(5, offset - 5);
            port = boost::lexical_cast<uint16_t>(endpoint.substr(offset + 1).c_str());
        }
        catch (boost::bad_lexical_cast &)
        {
            do_bad_endpoint(endpoint);
        }

        return std::make_pair(addr, port);
    }

    void
    common_init(const std::string& public_key, const std::string& private_key)
    {
        init_logging();
        io_context = std::make_shared<bzn::asio::io_context>();
        io_thread = std::make_shared<std::thread>([]()
        {
            auto& io = io_context->get_io_context();
            boost::asio::executor_work_guard<decltype(io.get_executor())> work{io.get_executor()};
            auto res = io_context->run();
            LOG(debug) << "Events run: " << res << std::endl;
        });

        db_dispatcher = std::make_shared<db_impl>(io_context);
        the_crypto = std::make_shared<crypto>(private_key);
        ws_factory = std::make_shared<bzn::beast::websocket>();
        the_swarm_factory = std::make_shared<swarm_factory>(io_context, ws_factory, the_crypto, the_esr, public_key);

        error_val = 0;
        error_str = "";
    }

    bool
    initialize(const std::string& public_key, const std::string& private_key
        , const std::string& endpoint, const std::string& node_id, const std::string& swarm_id)
    {
        if (!initialized)
        {
            try
            {
                common_init(public_key, private_key);
                auto ep = parse_endpoint(endpoint);
                std::vector<std::pair<node_id_t, bzn::peer_address_t>> addrs;
                addrs.push_back(std::make_pair(node_id, bzn::peer_address_t{ep.first, ep.second, 0, "", ""}));
                the_swarm_factory->initialize(swarm_id, addrs);
            }
            CATCHALL(
                if (io_context)
                {
                    io_context->stop();
                    if (io_thread)
                    {
                        io_thread->join();
                    }
                }
                return false;
            );

            initialized = true;
            return true;
        }
        return false;
    }

    bool
    initialize(const std::string& public_key, const std::string& private_key
        , const std::string& esr_address, const std::string& url)
    {
        if (!initialized)
        {
            try
            {
                common_init(public_key, private_key);
                the_swarm_factory->initialize(esr_address, url);
            }
            CATCHALL(return false);

            initialized = true;
            return true;
        }
        return false;
    }

    void
    terminate()
    {
        try
        {
            if (initialized)
            {
                io_context->stop();
                io_thread->join();

                initialized = false;
                error_str = "Not Initialized";
                error_val = -1;

                end_logging();
                the_swarm_factory = nullptr;
            }
        }
        CATCHALL();
    }

    std::shared_ptr<response>
    async_has_db(const std::string& uuid)
    {
        try
        {
            if (initialized)
            {
                std::string uuidstr{uuid};
                auto resp = make_response();
                the_swarm_factory->has_db(uuid, [resp, uuidstr](auto /*err*/, auto res)
                {
                    Json::Value result;
                    result["result"] = res ? 1 : 0;
                    result["uuid"] = uuidstr;
                    resp->set_result(result.toStyledString());
                    resp->set_ready();
                });

                return resp;
            }
            else
            {
                LOG(error) << "bzapi async_has_db method called before initialization";
                return nullptr;
            }
        }
        CATCHALL();
        return nullptr;
    }

    std::shared_ptr<response>
    async_create_db(const std::string& uuid, uint64_t max_size, bool random_evict)
    {
        try
        {
            if (initialized)
            {
                std::string uuidstr{uuid};
                auto resp = make_response();
                the_swarm_factory->create_db(uuidstr, max_size, random_evict, [uuidstr, resp](auto res, auto sw)
                {
                    if (sw)
                    {
                        auto db = std::make_shared<async_database_impl>(db_dispatcher, sw, uuidstr);
                        db->open([sw, resp, db, uuidstr](auto ec)
                        {
                            if (ec)
                            {
                                LOG(error) << "Error initializing database: " << ec.message();
                                Json::Value result;
                                result["error"] = ec.message();
                                resp->set_result(result.toStyledString());
                                resp->set_error(static_cast<int>(db_error::connection_error));
                            }
                            else
                            {
                                Json::Value result;
                                result["result"] = 1;
                                result["uuid"] = uuidstr;
                                resp->set_result(result.toStyledString());
                                resp->set_db(db);
                                resp->set_ready();
                            }
                        });
                    }
                    else
                    {
                        LOG(error) << "Error creating database for: " << uuidstr;
                        Json::Value result;
                        result["error"] = get_error_str(res);
                        result["uuid"] = uuidstr;
                        resp->set_result(result.toStyledString());
                        resp->set_error(static_cast<int>(res));
                    }
                });

                return resp;
            }
            else
            {
                LOG(error) << "bzapi async_create_db method called before initialization";
                return nullptr;
            }
        }
        CATCHALL();
        return nullptr;
    }

    std::shared_ptr<response>
    async_open_db(const std::string& uuid)
    {
        try
        {
            if (initialized)
            {
                std::string uuidstr{uuid};
                auto resp = make_response();
                the_swarm_factory->has_db(uuidstr, [resp, uuidstr](auto /*err*/, auto sw)
                {
                    if (sw)
                    {
                        auto db = std::make_shared<async_database_impl>(db_dispatcher, sw, uuidstr);
                        db->open([resp, db](auto ec)
                        {
                            if (ec)
                            {
                                LOG(error) << "Error initializing database: " << ec.message();
                                Json::Value result;
                                result["error"] = ec.message();
                                resp->set_result(result.toStyledString());
                                resp->set_error(static_cast<int>(db_error::connection_error));
                            }
                            else
                            {
                                Json::Value result;
                                result["result"] = 1;
                                resp->set_result(result.toStyledString());
                                resp->set_db(db);
                                resp->set_ready();
                            }
                        });
                    }
                    else
                    {
                        LOG(debug) << "Failed to open database: " << uuidstr;
                        // TODO: catch error here
//                        Json::Value result;
//                        result["error"] = get_error_str(res);
//                        resp->set_result(result.toStyledString());
//                        resp->set_error(static_cast<int>(res));
                   }
                });

                return resp;
            }
            else
            {
                LOG(error) << "bzapi async_open_db method called before initialization";
                return nullptr;
            }
        }
        CATCHALL();
        return nullptr;
    }

    bool
    has_db(const std::string& uuid)
    {
        try
        {
            if (initialized)
            {
                auto resp = async_has_db(uuid);
                auto result = resp->get_result();

                Json::Value json;
                std::stringstream(result) >> json;

                error_val = resp->get_error();
                error_str = json["error"].asString();

                return json["result"].asInt();
            }
            else
            {
                LOG(error) << "bzapi has_db method called before initialization";
                return false;
            }
        }
        CATCHALL();
        return false;
    }

    std::shared_ptr<database>
    create_db(const std::string& uuid, uint64_t max_size, bool random_evict)
    {
        try
        {
            if (initialized)
            {
                auto resp = async_create_db(uuid, max_size, random_evict);
                auto result = resp->get_result();

                Json::Value json;
                std::stringstream(result) >> json;

                error_val = resp->get_error();
                error_str = json["error"].asString();

                return json["result"].asInt() ? std::make_shared<database_impl>(resp->get_db())
                    : nullptr;
            }
            else
            {
                LOG(error) << "bzapi create_db method called before initialization";
                return nullptr;
            }
        }
        CATCHALL();
        return nullptr;
    }

    std::shared_ptr<database>
    open_db(const std::string& uuid)
    {
        try
        {
            if (initialized)
            {
                auto resp = async_open_db(uuid);
                auto result = resp->get_result();

                Json::Value json;
                std::stringstream(result) >> json;

                error_val = resp->get_error();
                error_str = json["error"].asString();

                return json["result"].asInt() ? std::make_shared<database_impl>(resp->get_db())
                    : nullptr;
            }
            else
            {
                LOG(error) << "bzapi open_db method called before initialization";
                return nullptr;
            }
        }
        CATCHALL();
        return nullptr;
    }

    int
    get_error()
    {
        return error_val;
    }

    std::string
    get_error_str()
    {
        return error_str;
    }

    std::string
    get_error_str(db_error err)
    {
        switch (err)
        {
        case db_error::success:
            return "Success";
        case db_error::uninitialized:
            return "bzapi uninitialized";
        case db_error::connection_error:
            return "Connection error";
        case db_error::database_error:
            return "Database error";
        case db_error::timeout_error:
            return "Timeout error";
        case db_error::already_exists:
            return "Database already exists";
        case db_error::no_database:
            return "No database";
        }

        return "Unknown error";
    }

    void
    set_timeout(uint64_t seconds)
    {
        api_timeout = seconds;
    }

    uint64_t
    get_timeout()
    {
        return api_timeout;
    }
}
