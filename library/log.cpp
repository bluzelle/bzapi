//
// Created by paul on 5/24/19.
//

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