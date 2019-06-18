add_library(bzapi SHARED library.cpp ../include/bzapi.hpp ../include/response.hpp udp_response.hpp mutable_response.hpp ../include/logger.hpp log.cpp log.hpp)
add_dependencies(bzapi boost)
target_include_directories(bzapi PRIVATE ${BLUZELLE_STD_INCLUDES} . ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(bzapi crypto proto swarm node database ${Protobuf_LIBRARIES} ${JSONCPP_LIBRARIES} ${OPENSSL_LIBRARIES} pthread)

if (NOT OMIT_LIBRARY)
    add_subdirectory(test)
endif(NOT OMIT_LIBRARY)

set_target_properties(bzapi PROPERTIES PUBLIC_HEADER "../include/bzapi.hpp;../include/response.hpp;../include/database.hpp;../include/async_database.hpp;../include/logger.hpp")

INSTALL(TARGETS bzapi
        LIBRARY DESTINATION /usr/local/lib
        PUBLIC_HEADER DESTINATION /usr/local/include/bzapi/
        )
