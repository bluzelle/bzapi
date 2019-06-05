set(test_srcs swarm_test.cpp ../../mocks/mock_node_factory.hpp ../../mocks/mock_node.hpp ../../crypto/null_crypto.hpp)
set(test_libs swarm crypto proto bzapi ${Protobuf_LIBRARIES} ${OPENSSL_LIBRARIES} boost_thread boost_log boost_chrono)

add_gmock_test(swarm)
