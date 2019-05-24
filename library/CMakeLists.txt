add_library(bzapi SHARED library.cpp ../include/bzapi.hpp ../include/response.hpp udp_response.hpp mutable_response.hpp)
target_include_directories(bzapi PRIVATE . ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(bzapi crypto proto swarm node database ${Protobuf_LIBRARIES} ${JSONCPP_LIBRARIES} ${OPENSSL_LIBRARIES} pthread)

add_subdirectory(test)

set_target_properties(bzapi PROPERTIES PUBLIC_HEADER "../include/bzapi.hpp;../include/response.hpp;../include/database.hpp;../include/async_database.hpp")

INSTALL(TARGETS bzapi
        LIBRARY DESTINATION /usr/local/lib
        PUBLIC_HEADER DESTINATION /usr/local/include/bzapi/
        )
