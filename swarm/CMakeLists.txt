add_library(
    swarm swarm.cpp
    swarm.hpp
    swarm_base.hpp
    swarm_factory.hpp
    swarm_factory.cpp)

add_dependencies(swarm boost)
target_include_directories(swarm PRIVATE ${BLUZELLE_STD_INCLUDES})
target_link_libraries(swarm proto )

if (NOT OMIT_LIBRARY)
    add_subdirectory(test)
endif(NOT OMIT_LIBRARY)
