add_library(utils
    esr_peer_info.cpp
    esr_peer_info.hpp
    http_req.cpp
    http_req.hpp
    peer_address.hpp
    )

target_include_directories(utils PRIVATE ${BLUZELLE_STD_INCLUDES})
add_dependencies(utils boost openssl)

if (BUILD_TESTS)
    add_subdirectory(test)
endif()