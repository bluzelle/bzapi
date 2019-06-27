add_library(bzapi SHARED
    library.cpp
    ../include/bzapi.hpp
    ../include/response.hpp
    udp_response.hpp
    mutable_response.hpp
    ../include/logger.hpp
    log.cpp
    log.hpp)

add_dependencies(bzapi boost)
target_include_directories(bzapi PRIVATE ${BLUZELLE_STD_INCLUDES})
target_link_libraries(bzapi database swarm node utils crypto ${JSONCPP_LIBRARIES} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${Protobuf_LIBRARIES})

if (NOT OMIT_LIBRARY)
    add_subdirectory(test)
endif(NOT OMIT_LIBRARY)
