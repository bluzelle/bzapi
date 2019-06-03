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

#include <boost/log/trivial.hpp>
#include <boost/system/error_code.hpp>
#include <boost/exception/all.hpp>
#include <string_view>
#include <libgen.h>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <list>
#include <string>

namespace bzapi::utils
{
    constexpr
    std::string_view basename(const std::string_view& path)
    {
        return path.substr(path.rfind('/') + 1);
    }
}

#define LOG(x) BOOST_LOG_TRIVIAL(x) << "(" << bzapi::utils::basename(__FILE__) << ":"  << __LINE__ << ") - "

#define CATCHALL(action) \
catch (boost::exception& be) \
{ \
    LOG(error) << "Exception caught " << boost::diagnostic_information(be); \
    action; \
} \
catch (std::exception& e) \
{ \
    LOG(error) << "Exception caught: " << e.what(); \
    action; \
} \
catch(...) \
{ \
    LOG(error) << "Unknown exception caught"; \
    action; \
}

namespace bzapi
{
    using uuid_t = std::string;
    using swarm_id_t = std::string;
    using endpoint_t = std::string;
    using completion_handler_t = std::function<void(const boost::system::error_code &error)>;
    using expiry_t = uint64_t;

    const uint16_t MAX_MESSAGE_SIZE = 1024;
    const uint16_t MAX_SHORT_MESSAGE_SIZE = 32;

    enum class db_error
    {
        success = 0,
        connection_error,
        database_error,
        timeout_error,
        no_database
    };

    uint64_t get_timeout();
    std::string get_error_str(db_error err);
}

