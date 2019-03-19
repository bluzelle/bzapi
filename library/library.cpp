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

#include <library.hpp>
#include <boost/asio.hpp>
#include <swarm/swarm.hpp>
#include <boost_asio_beast.hpp>
#include <swarm/swarm_factory.hpp>
#include <bzapi.hpp>
#include <crypto/crypto.hpp>
#include <library/response.hpp>
#include <database/db_base.hpp>

namespace bzapi
{
    std::shared_ptr<bzn::asio::io_context> io_context;
    std::shared_ptr<swarm_factory> the_swarm_factory;
    std::shared_ptr<crypto> the_crypto;

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
            the_swarm_factory = std::make_shared<swarm_factory>(io_context, the_crypto);
            the_swarm_factory->temporary_set_default_endpoint(endpoint);
        }
        catch(...)
        {
            return false;
        }

        return true;
    }

    std::shared_ptr<response>
    has_db(const char */*uuid*/)
    {
        auto resp = make_response();

        return nullptr;
    }

    future*
    create_db(const char */*uuid*/)
    {
        return nullptr;
    }

    future*
    remove_db(const char */*uuid*/)
    {
        return nullptr;
    }

    std::shared_ptr<response>
    open_db(const char */*uuid*/)
    {
        return nullptr;
    }
}