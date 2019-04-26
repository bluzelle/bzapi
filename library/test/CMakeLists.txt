set(test_srcs library_test.cpp ../../mocks/mock_boost_asio_beast.hpp)
#set(test_libs ${Protobuf_LIBRARIES} ${OPENSSL_LIBRARIES} database crypto proto swarm node bzapi)
set(test_libs ${OPENSSL_LIBRARIES} database crypto swarm node bzapi)
add_gmock_test(library)