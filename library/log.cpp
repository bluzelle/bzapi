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

#include <include/bluzelle.hpp>
#include <include/logger.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/core/null_deleter.hpp>
#include <iostream>

namespace
{
    bzapi::logger* the_logger = nullptr;
}

namespace bzapi
{
    namespace sinks = boost::log::sinks;
    namespace logging = boost::log;

    class log_pusher :
        public sinks::basic_sink_backend<sinks::combine_requirements<sinks::synchronized_feeding, sinks::flushing>::type>
    {
    private:
        std::string data;

    public:
        log_pusher() {}

        // The function consumes the log records that come from the frontend
        void consume(logging::record_view const& rec)
        {
            if (the_logger)
            {
                try
                {
                    std::stringstream sev;
                    sev << rec[logging::trivial::severity];
                    the_logger->log(sev.str(), *rec[logging::expressions::smessage]);
                }
                // can't use CATCHALL here as it calls LOG()
                catch(std::exception& e)
                {
                    std::cout << "Exception caught trying to log message: " << e.what() << std::endl;
                }
                catch(...)
                {
                    std::cout << "Unknown exception caught trying to log message" << std::endl;
                }
            }
        }

        // The function flushes the file
        void flush()
        {
        }
    };

    boost::shared_ptr<log_pusher> pusher;

    void init_logging()
    {
        pusher = boost::make_shared<log_pusher>();
        boost::shared_ptr< boost::log::core > core = boost::log::core::get();

        typedef sinks::synchronous_sink< log_pusher > sink_t;
        boost::shared_ptr< sink_t > sink(new sink_t(pusher));

        core->add_sink(sink);
    }

    void end_logging()
    {
        boost::shared_ptr< boost::log::core > core = boost::log::core::get();
        core->remove_all_sinks();
    }

    void set_logger(logger* logger)
    {
        the_logger = logger;
    }
}
