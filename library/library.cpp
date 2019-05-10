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
#include <library/udp_response.hpp>

namespace bzapi
{
    std::shared_ptr<bzn::asio::io_context_base> io_context;
    boost::asio::signal_set *signals = nullptr;
    std::thread *io_thread;
    std::shared_ptr<swarm_factory> the_swarm_factory;
    std::shared_ptr<crypto_base> the_crypto;
    std::shared_ptr<bzn::beast::websocket_base> ws_factory;

    std::shared_ptr<response>
    make_response()
    {
        return std::make_shared<udp_response>();
    }

    bool
    initialize(const std::string& public_key, const std::string& private_key, const std::string& endpoint)
    {
        try
        {
            io_context = std::make_shared<bzn::asio::io_context>();

            signals = new boost::asio::signal_set(io_context->get_io_context(), SIGINT);
            signals->async_wait([](const boost::system::error_code& error, int signal_number)
            {
                if (!error)
                {
                    std::cout << "signal received -- shutting down (" << signal_number << ")";
                    io_context->stop();
                }
                else
                {
                    std::cout << "Error: " << error.value() << ", " << error.category().name() << std::endl;
                }
            });

            io_thread = new std::thread([]()
            {
                std::cout << "running io_context" << std::endl;
                auto res = io_context->run();
                std::cout << "ran: " << res << std::endl;
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

        return true;
    }

    void
    terminate()
    {
        io_context->stop();
        io_thread->join();
    }

    std::shared_ptr<response>
    has_db(const std::string& uuid)
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

    std::shared_ptr<response>
    create_db(const std::string& uuid)
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
                        auto db = std::make_shared<database>(dbi);
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

    std::shared_ptr<response>
    open_db(const std::string& uuid)
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
                        auto db = std::make_shared<database>(dbi);
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

    namespace sync
    {
        bool
        has_db(const std::string& /*uuid*/)
        {
            return true;
        }

        bool
        create_db(const std::string& /*uuid*/)
        {
            return true;
        }

        bool
        open_db(const std::string& /*uuid*/)
        {
            return true;
        }
    }
}