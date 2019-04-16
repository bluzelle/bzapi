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

#include <library/library.hpp>
#include <boost/asio.hpp>
#include <swarm/swarm.hpp>
#include <boost_asio_beast.hpp>
#include <swarm/swarm_factory.hpp>
#include <bzapi.hpp>
#include <crypto/crypto.hpp>
#include <database/database.hpp>
#include <database/db_impl.hpp>
#include <jsoncpp/src/jsoncpp/include/json/value.h>
#include <library/response.hpp>

namespace bzapi
{
    std::shared_ptr<bzn::asio::io_context_base> io_context;
    std::shared_ptr<swarm_factory> the_swarm_factory;
    std::shared_ptr<crypto_base> the_crypto;
    std::shared_ptr<bzn::beast::websocket_base> ws_factory;

    std::shared_ptr<response>
    make_response()
    {
        return std::make_shared<udp_response>();
    }

    bool
    initialize(const char *private_key, const char *endpoint)
    {
        try
        {
            io_context = std::make_shared<bzn::asio::io_context>();
            the_crypto = std::make_shared<crypto>(private_key);
            ws_factory = std::make_shared<bzn::beast::websocket>();
            the_swarm_factory = std::make_shared<swarm_factory>(io_context, ws_factory, the_crypto);
            the_swarm_factory->temporary_set_default_endpoint(endpoint);
        }
        catch(...)
        {
            return false;
        }

        return true;
    }

    std::shared_ptr<response>
    has_db(const char *uuid)
    {
        auto resp = make_response();
        the_swarm_factory->has_db(uuid, [resp](auto res)
        {
            Json::Value result;
            result["result"] = res == db_error::success ? 1 : 0;
            resp->set_result(result.toStyledString());
            resp->set_ready();
        });

        return resp;
    }

    std::shared_ptr<response>
    create_db(const char */*uuid*/)
    {

        return nullptr;
    }

    std::shared_ptr<response>
    remove_db(const char */*uuid*/)
    {
        return nullptr;
    }

    std::shared_ptr<response>
    open_db(const char *uuid)
    {
        auto resp = make_response();
        the_swarm_factory->has_db(uuid, [&](auto res)
        {
            if (res == db_error::success)
            {
                the_swarm_factory->get_swarm(uuid, [&](auto sw)
                {
                    if (sw)
                    {
                        auto dbi = std::make_shared<db_impl>(io_context, sw, uuid);
                        auto db = std::make_shared<database>(dbi);
                        db->open([&](auto ec)
                        {
                            if (ec)
                            {
                                LOG(error) << "Error initializing database: " << ec.message();
                                Json::Value result;
                                result["error"] = ec.message();
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
                        LOG(error) << "Error getting swarm for: " << std::string(uuid);
                        Json::Value result;
                        result["error"] = "Error getting swarm";
                        resp->set_error(static_cast<int>(db_error::no_database));
                    }
                });
            }
            else
            {
                LOG(debug) << "Failed to open database: " << std::string(uuid);
                Json::Value result;
                result["error"] = "UUID not found";
                resp->set_error(static_cast<int>(db_error::no_database));
            }
        });

        return resp;
    }
}