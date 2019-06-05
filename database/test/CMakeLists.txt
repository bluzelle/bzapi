set(test_srcs database_test.cpp db_impl_test.cpp ../../mocks/mock_boost_asio_beast.hpp ../../mocks/mock_swarm.hpp ../../mocks/mock_db_impl.hpp)
set(test_libs database crypto bzapi ${Protobuf_LIBRARIES} ${OPENSSL_LIBRARIES} boost_log)

add_gmock_test(database)
