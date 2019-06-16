add_library(swarm swarm.cpp swarm.hpp swarm_base.hpp swarm_factory.hpp swarm_factory.cpp)
target_include_directories(swarm PRIVATE ${BLUZELLE_STD_INCLUDES} . ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(swarm node crypto proto ${Protobuf_LIBRARIES} ${OPENSSL_LIBRARIES} pthread)

if (NOT OMIT_LIBRARY)
    add_subdirectory(test)
endif(NOT OMIT_LIBRARY)
