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

#include <utils/http_req.hpp>
#include <utils/esr_peer_info.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>
#include <json/json.h>
#include <iostream>
#include <sstream>
#include <cassert>
#include <sstream>

using json_message = Json::Value;

namespace
{
    const std::string ERR_UNABLE_TO_PARSE_JSON_RESPONSE{"Unable to parse JSON response: "};

    json_message
    str_to_json(const std::string& json_str)
    {
        json_message json_msg;
        Json::CharReaderBuilder builder;
        Json::CharReader* reader = builder.newCharReader();
        std::string errors;
        if (!reader->parse(
            json_str.c_str()
            , json_str.c_str() + json_str.size()
            , &json_msg
            , &errors))
        {
            throw (std::runtime_error(ERR_UNABLE_TO_PARSE_JSON_RESPONSE + errors));
        }

        return json_msg;
    }
}


namespace bzn::utils::esr
{
    auto sync_req = bzn::utils::http::sync_req;

    void
    set_sync_method(std::string (*f)(const std::string&, const std::string&) = nullptr)
    {
        sync_req = f ? f : bzn::utils::http::sync_req;
    }


    std::vector<std::string>
    get_swarm_ids(const std::string &url)
    {
        const char* API_PATH{"/api/v1/swarms"};
        const auto response{bzn::utils::http::sync_req("https://" + url + API_PATH, "")};
        const auto json_response{str_to_json(response)};
        const auto swarm_ids_json{json_response.getMemberNames()};
        std::vector<std::string> swarm_ids;
        std::transform(
                swarm_ids_json.begin(), swarm_ids_json.end(), std::back_inserter(swarm_ids)
                , [](const std::string& id) { return id; });
        return swarm_ids;
    }


    std::vector<std::string>
    get_peer_ids(const bzapi::uuid_t &swarm_id, const std::string &url)
    {
        const char* API_PATH{"/api/v1/swarms"};
        const auto response{bzn::utils::http::sync_req("https://" + url + API_PATH, "")};
        const auto json_response{str_to_json(response)};
        const auto node_info = json_response[swarm_id];

        std::vector<std::string> peer_ids;
        std::transform(node_info.begin(), node_info.end(), std::back_inserter(peer_ids)
                , [](const auto& node) { return node["uuid"].asString(); });

        return peer_ids;
    }


    bzn::peer_address_t
    get_peer_info(const bzapi::uuid_t &swarm_id, const std::string &peer_id, const std::string &url)
    {
        const char* API_PATH{"/api/v1/swarms"};
        const auto response{bzn::utils::http::sync_req("https://" + url + API_PATH, "")};
        const auto json_response{str_to_json(response)};
        const auto node_info = json_response[swarm_id];

        for (const auto& n : node_info)
        {
            if ( peer_id == n["uuid"].asString() )
            {
                return bzn::peer_address_t(n["host"].asString(), uint16_t(n["port"].asInt()), uint16_t(8080), n["name"].asString(), n["uuid"].asString());
            }
        }

        return bzn::peer_address_t("", 0, 0, "", "");
    }
}

