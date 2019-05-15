add_library(database db_base.hpp async_database.cpp async_database.hpp ../include/boost_asio_beast.hpp db_impl_base.hpp db_impl.hpp db_impl.cpp database.cpp)
target_include_directories(database PRIVATE . ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(database ${Protobuf_LIBRARIES} ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} ${OPENSSL_LIBRARIES} pthread)
add_subdirectory(test)
