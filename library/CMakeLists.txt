add_library(_bzapi SHARED library.cpp library.hpp ../database/db_base.hpp response.hpp udp_response.hpp ../bzapi.hpp)
target_include_directories(_bzapi PRIVATE . ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(_bzapi crypto proto swarm node database ${Protobuf_LIBRARIES} ${JSONCPP_LIBRARIES} ${OPENSSL_LIBRARIES} pthread)

add_subdirectory(test)
