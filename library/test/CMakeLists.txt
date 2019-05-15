set(test_srcs library_test.cpp ../../mocks/mock_boost_asio_beast.hpp)
set(test_libs ${OPENSSL_LIBRARIES} database crypto swarm node _bzapi)
add_gmock_test(library)