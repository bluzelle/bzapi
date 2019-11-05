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

#include <include/bzapi.hpp>
#include <include/logger.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <json/json.h>
#include <iostream>
#include <unordered_map>


namespace
{
    // return codes
    const size_t SUCCESS = 0;
    const size_t ERROR_IN_COMMAND_LINE = 1;
    const size_t ERROR_UNHANDLED_EXCEPTION = 2;

    // command line args...
    const std::string CREATE_DB_MAX_SIZE = "create_db_max_size";
    const std::string CREATE_DB_RANDOM_EVICT = "create_db_random_evict";
    const std::string ADD_WRITER_PUBLIC_KEY = "add_writer_public_key";
    const std::string REMOVE_WRITER_PUBLIC_KEY = "remove_writer_public_key";
    const std::string CREATE_KEY = "create_key";
    const std::string CREATE_VALUE = "create_value";
    const std::string CREATE_EXPIRE = "create_expire";
    const std::string READ_KEY = "read_key";
    const std::string QREAD_KEY = "qread_key";
    const std::string UPDATE_KEY = "update_key";
    const std::string UPDATE_VALUE = "update_value";
    const std::string DELETE_KEY = "delete_key";
    const std::string TTL_KEY = "ttl_key";
    const std::string PERSIST_KEY = "persist_key";
    const std::string EXPIRE_KEY = "expire_key";
    const std::string EXPIRE_TTL = "expire_ttl";
    const std::string HAS_KEY = "has_key";
    const std::string HELP = "help";
    const std::string VERBOSE = "verbose";
    const std::string COMMAND = "command";
    const std::string UUID = "uuid";
    const std::string CONFIG = "config";

    // bzapi api...
    const std::string STATUS_CMD = "status";
    const std::string CREATE_DB_CMD = "create_db";
    const std::string HAS_DB_CMD = "has_db";
    const std::string WRITERS_CMD = "writers";
    const std::string ADD_WRITER_CMD = "add_writer";
    const std::string REMOVE_WRITER_CMD = "remove_writer";
    const std::string CREATE_CMD = "create";
    const std::string READ_CMD = "read";
    const std::string QREAD_CMD = "qread";
    const std::string UPDATE_CMD = "update";
    const std::string DELETE_CMD = "delete";
    const std::string HAS_CMD = "has";
    const std::string TTL_CMD = "ttl";
    const std::string PERSIST_CMD = "persist";
    const std::string EXPIRE_CMD = "expire";
    const std::string KEYS_CMD = "keys";
    const std::string SIZE_CMD = "size";

    const std::string BZAPI_CMDS =
        STATUS_CMD        + " | " +
        CREATE_DB_CMD     + " | " +
        HAS_DB_CMD        + " | " +
        WRITERS_CMD       + " | " +
        ADD_WRITER_CMD    + " | " +
        REMOVE_WRITER_CMD + " | " +
        CREATE_CMD        + " | " +
        READ_CMD          + " | " +
        QREAD_CMD         + " | " +
        UPDATE_CMD        + " | " +
        DELETE_CMD        + " | " +
        HAS_CMD           + " | " +
        TTL_CMD           + " | " +
        PERSIST_CMD       + " | " +
        EXPIRE_CMD        + " | " +
        KEYS_CMD          + " | " +
        SIZE_CMD;

    using bzapi_cmd_handler_t = bool(*)(boost::program_options::variables_map& vm, const Json::Value& config);

    // config keys...
    const std::string PUBLIC_KEY   = "public_key";
    const std::string PRIVATE_KEY  = "private_key";
    const std::string ESR_ADDRESS  = "esr_address";
    const std::string ETHEREUM_URL = "ethereum_url";
    const std::string TIMEOUT      = "timeout";


    class my_logger : public bzapi::logger
    {
        void log(const std::string& severity, const std::string& message)
        {
            std::cout << severity << ": " << message << std::endl;
        }
    };

    my_logger logger;
}


bool
load_config(const std::string& filename, Json::Value& config)
{
    std::ifstream file(filename);
    if (file.fail())
    {
        std::cerr << "Failed to read config file:  " << filename << " (" << strerror(errno) << ")\n";

        return false;
    }

    try
    {
        file >> config;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to parse config! (" << e.what() << ")";

        return false;
    }

    return true;
}


bool
initialize_bzapi(const Json::Value& config)
{
    if (config.isMember(TIMEOUT))
    {
        bzapi::set_timeout(config[TIMEOUT].asUInt64());
    }

    if (!bzapi::initialize(config[PUBLIC_KEY].asString(), config[PRIVATE_KEY].asString(), config[ESR_ADDRESS].asString(), config[ETHEREUM_URL].asString()))
    {
        std::cerr << "initialize failed: " << bzapi::get_error_str() << '\n';

        return false;
    }

    return true;
}


std::shared_ptr<bzapi::database>
open_db(boost::program_options::variables_map& vm, const Json::Value& config)
{
    std::shared_ptr<bzapi::database> db;

    if (initialize_bzapi(config))
    {
        db = bzapi::open_db(vm[UUID].as<std::string>());

        if (!db)
        {
            std::cerr << bzapi::get_error_str() << '\n';
        }
    }

    return db;
}


bool
handle_status(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (auto db = open_db(vm, config); db)
    {
        std::cout << db->swarm_status() << "\n";

        return true;
    }

    return false;
}


bool
handle_create_db(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (initialize_bzapi(config))
    {
        if (auto db = bzapi::create_db(vm[UUID].as<std::string>(), vm[CREATE_DB_MAX_SIZE].as<uint64_t>(),
                vm[CREATE_DB_RANDOM_EVICT].as<bool>()); db)
        {
            return true;
        }
    }

    std::cerr << bzapi::get_error_str() << '\n';

    return false;
}


bool
handle_has_db(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (initialize_bzapi(config))
    {
        if (bzapi::has_db(vm[UUID].as<std::string>()))
        {
            std::cout << "database: " << vm[UUID].as<std::string>() << " exists\n";

            return true;
        }
    }

    std::cerr << bzapi::get_error_str() << '\n';

    return false;
}


bool
handle_writer(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (auto db = open_db(vm, config); db)
    {
        std::cout << db->writers() << '\n';

        return true;
    }

    return false;
}


bool
handle_add_writer(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(ADD_WRITER_PUBLIC_KEY))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->add_writer(vm[ADD_WRITER_PUBLIC_KEY].as<std::string>()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << ADD_WRITER_PUBLIC_KEY << " not specified\n";
    }

    return false;
}


bool
handle_remove_writer(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(REMOVE_WRITER_PUBLIC_KEY))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->remove_writer(vm[REMOVE_WRITER_PUBLIC_KEY].as<std::string>()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << REMOVE_WRITER_PUBLIC_KEY << " not specified\n";
    }

    return false;
}


bool
handle_create(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(CREATE_KEY) && vm.count(CREATE_VALUE))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->create(vm[CREATE_KEY].as<std::string>(), vm[CREATE_VALUE].as<std::string>(), vm[CREATE_EXPIRE].as<uint64_t >()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << CREATE_KEY << " & " << CREATE_VALUE << " must be specified\n";
    }

    return false;
}


bool
handle_read(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(READ_KEY))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->read(vm[READ_KEY].as<std::string>()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << READ_KEY << " must be specified\n";
    }

    return false;
}


bool
handle_qread(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(QREAD_KEY))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->quick_read(vm[QREAD_KEY].as<std::string>()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << QREAD_KEY << " must be specified\n";
    }

    return false;
}


bool
handle_update(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(UPDATE_KEY) && vm.count(UPDATE_VALUE))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->update(vm[UPDATE_KEY].as<std::string>(), vm[UPDATE_VALUE].as<std::string>()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << UPDATE_KEY << " & " << UPDATE_VALUE << " must be specified\n";
    }

    return false;
}


bool
handle_delete(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(DELETE_KEY))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->remove(vm[DELETE_KEY].as<std::string>()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << DELETE_KEY << " must be specified\n";
    }

    return false;
}


bool
handle_has(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(HAS_KEY))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->has(vm[HAS_KEY].as<std::string>()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << HAS_KEY << " must be specified\n";
    }

    return false;
}


bool
handle_ttl(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(TTL_KEY))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->ttl(vm[TTL_KEY].as<std::string>()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << TTL_KEY << " must be specified\n";
    }

    return false;
}


bool
handle_persist(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(PERSIST_KEY))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->persist(vm[PERSIST_KEY].as<std::string>()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << PERSIST_KEY << " must be specified\n";
    }

    return false;
}


bool
handle_expire(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (vm.count(EXPIRE_KEY) && vm.count(EXPIRE_TTL))
    {
        if (auto db = open_db(vm, config); db)
        {
            std::cout << db->expire(vm[EXPIRE_KEY].as<std::string>(), vm[EXPIRE_TTL].as<uint64_t>()) << '\n';

            return true;
        }
    }
    else
    {
        std::cerr << EXPIRE_KEY << " & " << EXPIRE_TTL << " must be specified\n";
    }

    return false;
}


bool
handle_keys(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (auto db = open_db(vm, config); db)
    {
        std::cout << db->keys() << '\n';

        return true;
    }

    return false;
}


bool
handle_size(boost::program_options::variables_map& vm, const Json::Value& config)
{
    if (auto db = open_db(vm, config); db)
    {
        std::cout << db->size() << '\n';

        return true;
    }

    return false;
}


bool
process_program_options(boost::program_options::variables_map& vm, const Json::Value& config)
{
    const std::unordered_map<std::string, bzapi_cmd_handler_t> bzapi_cmd_handlers{
        {STATUS_CMD,        handle_status},
        {CREATE_DB_CMD,     handle_create_db},
        {HAS_DB_CMD,        handle_has_db},
        {WRITERS_CMD,       handle_writer},
        {ADD_WRITER_CMD,    handle_add_writer},
        {REMOVE_WRITER_CMD, handle_remove_writer},
        {CREATE_CMD,        handle_create},
        {READ_CMD,          handle_read},
        {QREAD_CMD,         handle_qread},
        {UPDATE_CMD,        handle_update},
        {DELETE_CMD,        handle_delete},
        {HAS_CMD,           handle_has},
        {TTL_CMD,           handle_ttl},
        {PERSIST_CMD,       handle_persist},
        {EXPIRE_CMD,        handle_expire},
        {KEYS_CMD,          handle_keys},
        {SIZE_CMD,          handle_size}
    };

    if (vm.count(VERBOSE))
    {
        bzapi::set_logger(&logger);
    }

    if (auto handler_it = bzapi_cmd_handlers.find(vm[COMMAND].as<std::string>()); handler_it != bzapi_cmd_handlers.end())
    {
        return (*(handler_it->second))(vm, config);
    }

    std::cerr << "Unknown command: " << vm[COMMAND].as<std::string>() << '\n';

    return false;
}


int
main(int argc, const char* argv[])
{
    namespace po = boost::program_options;

    po::variables_map vm;
    po::options_description description("bzapi options");

    try
    {
        description.add_options()
            ("help,h",   "display this help message")
            ("verbose,v","verbose logging")
            ("config,c", po::value<std::string>()->default_value("bzapi-cli.json"), "configuration file")
            ("command",  po::value<std::string>(), BZAPI_CMDS.c_str())
            ("uuid,u",   po::value<std::string>(), "database uuid");

        // some help as subparsers don't exists ("easily") in boost::program_options...
        po::options_description status(STATUS_CMD + " (Swarm status)");
        description.add(status);

        // const std::string& uuid, uint64_t max_size, bool random_evict);
        po::options_description create_db(CREATE_DB_CMD + " (Create database)");
        create_db.add_options()
            (CREATE_DB_MAX_SIZE.c_str(), po::value<uint64_t>()->default_value(0), "database max size")
            (CREATE_DB_RANDOM_EVICT.c_str(), po::value<bool>()->default_value(false), "database policy");
        description.add(create_db);

        po::options_description has_db(HAS_DB_CMD + " (Has database)");
        description.add(has_db);

        po::options_description writers(WRITERS_CMD + " (Database writers)");
        description.add(writers);

        po::options_description add_writer(ADD_WRITER_CMD + " (Add database writer)");
        add_writer.add_options()
            (ADD_WRITER_PUBLIC_KEY.c_str(), po::value<std::string>(), "writer's public key");
        description.add(add_writer);

        po::options_description remove_writer(REMOVE_WRITER_CMD + " (Remove database writer)");
        remove_writer.add_options()
            (REMOVE_WRITER_PUBLIC_KEY.c_str(), po::value<std::string>(), "writer's public key");
        description.add(remove_writer);

        po::options_description create(CREATE_CMD + " (Create k/v)");
        create.add_options()
            (CREATE_KEY.c_str(), po::value<std::string>(), "create to key")
            (CREATE_VALUE.c_str(), po::value<std::string>(), "created key value")
            (CREATE_EXPIRE.c_str(), po::value<uint64_t>()->default_value(0), "key's expiration in seconds");
        description.add(create);

        po::options_description read(READ_CMD + " (Read k/v)");
        read.add_options()
            (READ_KEY.c_str(), po::value<std::string>(), "key to read");
        description.add(read);

        po::options_description qread(QREAD_CMD + " (Quick read k/v)");
        qread.add_options()
            (QREAD_KEY.c_str(), po::value<std::string>(), "key to quick read");
        description.add(qread);

        po::options_description update(UPDATE_CMD + " (Update k/v)");
        update.add_options()
            (UPDATE_KEY.c_str(), po::value<std::string>(), "key to update")
            (UPDATE_VALUE.c_str(), po::value<std::string>(), "key's new value");
        description.add(update);

        po::options_description remove(DELETE_CMD + " (Delete k/v)");
        remove.add_options()
            (DELETE_KEY.c_str(), po::value<std::string>(), "key to delete");
        description.add(remove);

        po::options_description has(HAS_CMD + " (Determine whether a key exists within a database by uuid)");
        has.add_options()
            (HAS_KEY.c_str(), po::value<std::string>(), "key to check");
        description.add(has);

        po::options_description ttl(TTL_CMD + " (Get ttl for a key within a db by uuid)");
        ttl.add_options()
            (TTL_KEY.c_str(), po::value<std::string>(), "ttl for a key");
        description.add(ttl);

        po::options_description persist(PERSIST_CMD + " (Remove expiration for a key within a database by uuid)");
        persist.add_options()
            (PERSIST_KEY.c_str(), po::value<std::string>(), "remove ttl for a key");
        description.add(persist);

        po::options_description expire(EXPIRE_CMD + " (Set expire for a key within a database by uuid)");
        expire.add_options()
            (EXPIRE_KEY.c_str(), po::value<std::string>(), "key to add a ttl")
            (EXPIRE_TTL.c_str(), po::value<uint64_t>(), "set ttl for a key in seconds");
        description.add(expire);

        po::options_description size(SIZE_CMD + " (Determine the size of the database by uuid)");
        description.add(size);

        po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
        po::notify(vm);

        if (vm.count(HELP) || argc == 1)
        {
            std::cout << description;
            return SUCCESS;
        }

        Json::Value config;
        if (!load_config(vm[CONFIG].as<std::string>(), config))
        {
            return ERROR_IN_COMMAND_LINE;
        }

        if (!vm.count(UUID))
        {
            std::cerr << "Error: uuid not specified\n" << description << '\n';
            return ERROR_IN_COMMAND_LINE;
        }

        if (!vm.count(COMMAND))
        {
            std::cerr << "Error: command not specified\n" << description << '\n';
            return ERROR_IN_COMMAND_LINE;
        }

        if (process_program_options(vm, config))
        {
            bzapi::terminate();

            return SUCCESS;
        }
    }
    catch(std::exception& e)
    {
        std::cerr << "Unhandled Exception: " << e.what() << ", application will now exit" << std::endl;

        std::cerr << description;

        bzapi::terminate();

        return ERROR_UNHANDLED_EXCEPTION;
    }

    bzapi::terminate();

    return ERROR_IN_COMMAND_LINE;
}
