
add_library(bzapi SHARED library.cpp library.hpp ../include/boost_asio_beast.hpp ../database/db_base.hpp response.hpp)
add_dependencies(boost jsoncpp openssl)
target_include_directories(bzapi PRIVATE . ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(bzapi crypto proto swarm node database ${Protobuf_LIBRARIES} ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} ${OPENSSL_LIBRARIES} pthread)

add_subdirectory(test)
