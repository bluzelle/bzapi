set(test_srcs crypto_test.cpp)
set(test_libs crypto proto ${Protobuf_LIBRARIES} ${OPENSSL_LIBRARIES} boost_log)

add_gmock_test(crypto)
