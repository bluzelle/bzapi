set(test_srcs library_test.cpp)
set(test_libs ${OPENSSL_LIBRARIES} database crypto swarm node bzapi boost_log)
add_gmock_test(library)
