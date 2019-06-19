add_library(node
    node_base.hpp
    node.cpp node.hpp
    ../include/boost_asio_beast.hpp
    node_factory_base.hpp
    node_factory.hpp
    node_factory.cpp
    )

add_dependencies(node boost)
target_include_directories(node PRIVATE ${BLUZELLE_STD_INCLUDES})

if (NOT OMIT_LIBRARY)
    add_subdirectory(test)
endif(NOT OMIT_LIBRARY)

