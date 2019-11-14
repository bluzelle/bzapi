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

#include <utils/esr_peer_info.hpp>
#include <gtest/gtest.h>

using namespace testing;

namespace
{
    const std::map<const char*, const char*> FAKE_SERVER_RESPONSES{{"https://cpr-dev.bluzelle.com/api/v1/swarms", R"({"testnet":[{"host":"127.0.0.1","name":"peer0","port":49150,"uuid":"MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEb21xjv3drO9cbYyHum7wDOBVdfKL5knuaTspWfYkvlZN3oVUVKUSJWaxaUEpj1/XXHx2Mbhd6v6JQ0aGncBy7Q=="},{"host":"127.0.0.1","name":"peer1","port":49151,"uuid":"MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAE8xJyKKG4+kQT5qFL6T8M7ee0tLzCJkUYmqB+bVC6WXupXa18PU4pLgc7T8N2VXxmGY47AdShMgExxckRNS5wXA=="},{"host":"127.0.0.1","name":"peer2","port":49152,"uuid":"MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEMg2LnYtqcudTFmDV6RGT6IGUvQOjhYP/o8j425Oiq6eDSX5q++mU/L8lZET8iPJquUXwpM1fDD4kuwjO6mV3AA=="},{"host":"127.0.0.1","name":"peer3","port":49153,"uuid":"MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAE1LwWPBMwPR1CoK7Yxbb2TW5BF8b3anPm834sulhibZeIB4hw/oG0YUTry5V9LGBTjl8WozHMr94YqOKDsa/+/w=="}],"testnet-2":[{"host":"127.0.0.1","name":"peer0","port":49150,"uuid":"MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEb21xjv3drO9cbYyHum7wDOBVdfKL5knuaTspWfYkvlZN3oVUVKUSJWaxaUEpj1/XXHx2Mbhd6v6JQ0aGncBy7Q=="},{"host":"127.0.0.1","name":"peer1","port":49151,"uuid":"MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAE8xJyKKG4+kQT5qFL6T8M7ee0tLzCJkUYmqB+bVC6WXupXa18PU4pLgc7T8N2VXxmGY47AdShMgExxckRNS5wXA=="},{"host":"127.0.0.1","name":"peer2","port":49152,"uuid":"MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEMg2LnYtqcudTFmDV6RGT6IGUvQOjhYP/o8j425Oiq6eDSX5q++mU/L8lZET8iPJquUXwpM1fDD4kuwjO6mV3AA=="},{"host":"127.0.0.1","name":"peer3","port":49153,"uuid":"MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAE1LwWPBMwPR1CoK7Yxbb2TW5BF8b3anPm834sulhibZeIB4hw/oG0YUTry5V9LGBTjl8WozHMr94YqOKDsa/+/w=="}],"testnetsnew":[],"swarmnew":[],"testnewnew":[],"sdfsdf":[],"unittestswarm":[],"thisisanewswarm":[]})"}};
}


class cpr_test : public Test
{
public:
    void SetUp() override
    {
        bzn::utils::esr::set_sync_method([](const auto& url, const auto& ) -> std::string
        {
            try
            {
                return FAKE_SERVER_RESPONSES.at(url.c_str());
            }
            catch (...)
            {
                return "";
            }
        });
    }


    void TearDown() override
    {
        bzn::utils::esr::set_sync_method(nullptr);
    }

};


TEST_F(cpr_test, test_get_swarm_ids)
{
    const std::vector<std::string> accepted_ids { "newswarmtest", "sdfsdf", "swarmnew", "testnet", "testnet-2", "testnetsnew", "testnewnew", "thisisanewswarm", "unittestswarm"};

    auto actual_ids{bzn::utils::esr::get_swarm_ids("cpr-dev.bluzelle.com")};
    std::sort(actual_ids.begin(), actual_ids.end());
    EXPECT_TRUE(std::equal(accepted_ids.begin(), accepted_ids.end(), actual_ids.begin()));
}


TEST_F(cpr_test, test_get_peer_ids )
{
    std::vector<std::string> accepted_testnet_2_node_ids
    {
        "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEb21xjv3drO9cbYyHum7wDOBVdfKL5knuaTspWfYkvlZN3oVUVKUSJWaxaUEpj1/XXHx2Mbhd6v6JQ0aGncBy7Q==",
        "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAE8xJyKKG4+kQT5qFL6T8M7ee0tLzCJkUYmqB+bVC6WXupXa18PU4pLgc7T8N2VXxmGY47AdShMgExxckRNS5wXA==",
        "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEMg2LnYtqcudTFmDV6RGT6IGUvQOjhYP/o8j425Oiq6eDSX5q++mU/L8lZET8iPJquUXwpM1fDD4kuwjO6mV3AA==",
        "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAE1LwWPBMwPR1CoK7Yxbb2TW5BF8b3anPm834sulhibZeIB4hw/oG0YUTry5V9LGBTjl8WozHMr94YqOKDsa/+/w=="
    };
    auto actual_node_ids{bzn::utils::esr::get_peer_ids("testnet-2", "cpr-dev.bluzelle.com")};

    ASSERT_EQ(accepted_testnet_2_node_ids.size(), actual_node_ids.size());

    std::sort(accepted_testnet_2_node_ids.begin(), accepted_testnet_2_node_ids.end());
    std::sort(actual_node_ids.begin(), actual_node_ids.end());
    EXPECT_TRUE(std::equal(accepted_testnet_2_node_ids.begin(), accepted_testnet_2_node_ids.end(), actual_node_ids.begin()));
}


TEST_F(cpr_test, test_get_peer_info)
{
    bzn::peer_address_t accepted("127.0.0.1", 49150, 8080, "peer0", R"(MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEb21xjv3drO9cbYyHum7wDOBVdfKL5knuaTspWfYkvlZN3oVUVKUSJWaxaUEpj1/XXHx2Mbhd6v6JQ0aGncBy7Q==)");
    auto actual{bzn::utils::esr::get_peer_info("testnet-2", accepted.uuid, "cpr-dev.bluzelle.com")};

    EXPECT_EQ(accepted.host, actual.host);
    EXPECT_EQ(accepted.port, actual.port);
    EXPECT_EQ(accepted.http_port, actual.http_port);
    EXPECT_EQ(accepted.name, actual.name);
    EXPECT_EQ(accepted.uuid, actual.uuid);
}

