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
#include <defs.hpp>
#include <crypto/crypto.hpp>
#include <database/async_database.hpp>
#include <database/db_impl.hpp>
#include <json/value.h>
#include <library/udp_response.hpp>
#include <json/value.h>
#include <json/reader.h>

namespace bzapi
{
    std::shared_ptr<bzn::asio::io_context_base> io_context;
    std::shared_ptr<std::thread> io_thread;
    std::shared_ptr<swarm_factory> the_swarm_factory;
    std::shared_ptr<crypto_base> the_crypto;
    std::shared_ptr<bzn::beast::websocket_base> ws_factory;
    std::string error_str = "Not Initialized";
    int error_val = -1;
    bool initialized = false;

    std::shared_ptr<response>
    make_response()
    {
        return std::make_shared<udp_response>();
    }

    bool
    initialize(const std::string& public_key, const std::string& private_key, const std::string& endpoint)
    {
        if (!initialized)
        {
            try
            {
                io_context = std::make_shared<bzn::asio::io_context>();
                io_thread = std::make_shared<std::thread>([]()
                {
                    auto& io = io_context->get_io_context();
                    boost::asio::executor_work_guard<decltype(io.get_executor())> work{io.get_executor()};
                    auto res = io_context->run();
                    LOG(debug) << "Events run: " << res << std::endl;
                });

                the_crypto = std::make_shared<crypto>(private_key);
                ws_factory = std::make_shared<bzn::beast::websocket>();
                the_swarm_factory = std::make_shared<swarm_factory>(io_context, ws_factory, the_crypto, public_key);
                the_swarm_factory->temporary_set_default_endpoint(endpoint);
            }
            catch (...)
            {
                return false;
            }

            error_val = 0;
            error_str = "";
            initialized = true;
        }
        return true;
    }

    void
    terminate()
    {
        if (initialized)
        {
            io_context->stop();
            io_thread->join();

            initialized = false;
            error_str = "Not Initialized";
            error_val = -1;
        }
    }

    std::shared_ptr<response>
    async_has_db(const std::string& uuid)
    {
        if (initialized)
        {
            std::string uuidstr{uuid};
            auto resp = make_response();
            the_swarm_factory->has_db(uuid, [resp, uuidstr](auto res)
            {
                Json::Value result;
                result["result"] = res == db_error::success ? 1 : 0;
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

    std::shared_ptr<response>
    async_create_db(const std::string& uuid)
    {
        if (initialized)
        {
            std::string uuidstr{uuid};
            auto resp = make_response();
            the_swarm_factory->has_db(uuidstr, [resp, uuidstr](auto res)
            {
                if (res == db_error::no_database)
                {
                    the_swarm_factory->create_db(uuidstr, [uuidstr, resp](auto sw)
                    {
                        if (sw)
                        {
                            auto dbi = std::make_shared<db_impl>(io_context, sw, uuidstr);
                            auto db = std::make_shared<async_database>(dbi);
                            db->open([sw, resp, db, uuidstr](auto ec)
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
                            result["error"] = "Error creating database";
                            result["uuid"] = uuidstr;
                            resp->set_result(result.toStyledString());
                            resp->set_error(static_cast<int>(db_error::no_database));
                        }
                    });
                }
                else if (res == db_error::success)
                {
                    LOG(debug) << "Unable to create existing database: " << uuidstr;
                    Json::Value result;
                    result["error"] = "UUID already exists";
                    result["uuid"] = uuidstr;
                    resp->set_result(result.toStyledString());
                    resp->set_error(static_cast<int>(db_error::database_error));
                }
                else
                {
                    Json::Value result;
                    result["error"] = "Connection error";
                    result["uuid"] = uuidstr;
                    resp->set_result(result.toStyledString());
                    resp->set_error(static_cast<int>(db_error::connection_error));
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

    std::shared_ptr<response>
    async_open_db(const std::string& uuid)
    {
        if (initialized)
        {
            std::string uuidstr{uuid};
            auto resp = make_response();
            the_swarm_factory->has_db(uuidstr, [resp, uuidstr](auto res)
            {
                if (res == db_error::success)
                {
                    the_swarm_factory->get_swarm(uuidstr, [&](auto sw)
                    {
                        if (sw)
                        {
                            auto dbi = std::make_shared<db_impl>(io_context, sw, uuidstr);
                            auto db = std::make_shared<async_database>(dbi);
                            db->open([resp, db](auto ec)
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
                            LOG(error) << "Error getting swarm for: " << uuidstr;
                            Json::Value result;
                            result["error"] = "Error getting swarm";
                            result["uuid"] = uuidstr;
                            resp->set_error(static_cast<int>(db_error::no_database));
                        }
                    });
                }
                else
                {
                    LOG(debug) << "Failed to open database: " << uuidstr;
                    Json::Value result;
                    result["error"] = "UUID not found";
                    resp->set_error(static_cast<int>(db_error::no_database));
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

    bool
    has_db(const std::string& uuid)
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

    std::shared_ptr<database>
    create_db(const std::string& uuid)
    {
        if (initialized)
        {
            auto resp = async_create_db(uuid);
            auto result = resp->get_result();

            Json::Value json;
            std::stringstream(result) >> json;

            error_val = resp->get_error();
            error_str = json["error"].asString();

            return json["result"].asInt() ? std::make_shared<database>(*(resp->get_db()))
                : nullptr;
        }
        else
        {
            LOG(error) << "bzapi create_db method called before initialization";
            return nullptr;
        }
    }

    std::shared_ptr<database>
    open_db(const std::string& uuid)
    {
        if (initialized)
        {
            auto resp = async_open_db(uuid);
            auto result = resp->get_result();

            Json::Value json;
            std::stringstream(result) >> json;

            error_val = resp->get_error();
            error_str = json["error"].asString();

            return json["result"].asInt() ? std::make_shared<database>(*(resp->get_db()))
                : nullptr;
        }
        else
        {
            LOG(error) << "bzapi open_db method called before initialization";
            return nullptr;
        }
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
}