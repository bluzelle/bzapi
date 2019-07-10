set(LIBRARY_SOURCES
    ../include/bzapi.hpp
    ../include/logger.hpp
    ../include/response.hpp
    library.cpp
    log.cpp
    log.hpp
    mutable_response.hpp
    udp_response.hpp)

if (NOT STATIC_LIBRARY)
    add_library(bzapi SHARED ${LIBRARY_SOURCES})
else()
    add_library(bzapi STATIC ${LIBRARY_SOURCES})
endif()

add_dependencies(bzapi boost)
target_include_directories(bzapi PRIVATE ${BLUZELLE_STD_INCLUDES})
target_link_libraries(bzapi database swarm node utils crypto proto ${JSONCPP_LIBRARIES} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${Protobuf_LIBRARIES})

if (NOT STATIC_LIBRARY)
    add_subdirectory(test)
endif()
