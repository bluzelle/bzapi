set(test_srcs crypto_test.cpp)
set(test_libs crypto proto ${Protobuf_LIBRARIES})

add_gmock_test(crypto)
