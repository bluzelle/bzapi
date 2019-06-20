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

#include <gmock/gmock.h>
#include <swarm/esr_base.hpp>

namespace bzapi
{
    class mock_esr : public esr_base
    {
    public:
        MOCK_METHOD2(get_swarm_ids, std::vector<std::string>(const std::string&, const std::string&));
        MOCK_METHOD3(get_peer_ids, std::vector<std::string>(const std::string&, const std::string&, const std::string&));
        MOCK_METHOD4(get_peer_info, bzn::peer_address_t(const std::string&, const std::string&, const std::string&, const std::string&));
    };
}
