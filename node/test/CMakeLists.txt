set(test_srcs node_test.cpp ../../mocks/mock_boost_asio_beast.hpp)
set(test_libs node crypto proto ${Protobuf_LIBRARIES} ${OPENSSL_LIBRARIES})

add_gmock_test(node)
