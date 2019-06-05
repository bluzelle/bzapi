add_library(utils esr_peer_info.cpp esr_peer_info.hpp http_req.cpp http_req.hpp peer_address.hpp)
target_include_directories(utils PRIVATE . ${PROJECT_SOURCE_DIR})
target_link_libraries(utils ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
