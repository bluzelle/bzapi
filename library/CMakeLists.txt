add_library(bzapi SHARED library.cpp library.hpp ../include/boost_asio_beast.hpp ../database/db_base.hpp response.hpp udp_response.hpp ../bzapi.hpp)
target_include_directories(bzapi PRIVATE . ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(bzapi crypto proto swarm node database ${JSONCPP_LIBRARIES} ${OPENSSL_LIBRARIES} pthread)

add_subdirectory(test)
