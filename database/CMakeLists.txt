add_library(
    database async_database_impl.cpp
    ../include/async_database.hpp
        db_dispatch_base.hpp
        db_dispatch.hpp
        db_dispatch.cpp
    database_impl.cpp
    async_database_impl.hpp
    database_impl.hpp
    ../include/boost_asio_beast.hpp )

add_dependencies(database proto boost)
target_include_directories(database PRIVATE ${BLUZELLE_STD_INCLUDES})

if (NOT OMIT_LIBRARY)
    add_subdirectory(test)
endif(NOT OMIT_LIBRARY)
