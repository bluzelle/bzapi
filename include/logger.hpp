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

#include <string>

namespace bzapi
{
    /// The logger class is used to send log messages to the client application.
    /// Derive a class from logger and implement the log() function
    /// then register it using the set_logger function
    class logger
    {
    public:
        virtual ~logger() = default;
        virtual void log(const std::string& severity, const std::string& message) = 0;
    };

    /// Sets the object used to receive log messages.
    /// @param logger - pointer to the logger object to use
    void set_logger(logger* logger);
}