set(test_srcs node_test.cpp)
set(test_libs node crypto proto ${Protobuf_LIBRARIES})

add_gmock_test(node)
