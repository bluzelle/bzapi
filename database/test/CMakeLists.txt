set(test_srcs database_test.cpp db_impl_test.cpp ../../mocks/mock_boost_asio_beast.hpp ../../mocks/mock_swarm.hpp ../../mocks/mock_db_impl.hpp)
set(test_libs database crypto proto ${Protobuf_LIBRARIES} ${OPENSSL_LIBRARIES})

add_gmock_test(database)
