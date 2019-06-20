set(test_srcs swarm_test.cpp swarm_factory_test.cpp ../../mocks/mock_node_factory.hpp ../../mocks/mock_node.hpp ../../crypto/null_crypto.hpp)
set(test_libs swarm crypto proto bzapi ${Protobuf_LIBRARIES})

add_gmock_test(swarm)
