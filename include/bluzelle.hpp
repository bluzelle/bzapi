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
    using node_id_t = std::string;


    const uint16_t MAX_MESSAGE_SIZE = 1024;
    const uint16_t MAX_SHORT_MESSAGE_SIZE = 32;

    const std::string TIMESTAMP_ERROR_MSG{"INVALID TIMESTAMP"};
    const std::string TOO_LARGE_ERROR_MSG{"REQUEST TOO LARGE"};
    const std::string TOO_BUSY_ERROR_MSG{"SERVER TOO BUSY"};
    const std::string DUPLICATE_ERROR_MSG{"DUPLICATE REQUEST"};

    enum class db_error
    {
        success = 0,
        uninitialized,
        connection_error,
        database_error,
        timeout_error,
        already_exists,
        no_space,
        no_database
    };

    uint64_t get_timeout();
    std::string get_error_str(db_error err);
}

