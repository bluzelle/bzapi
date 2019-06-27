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
