set(test_srcs database_test.cpp db_dispatch_test.cpp)
set(test_libs database crypto bzapi ${Protobuf_LIBRARIES})

add_gmock_test(database)
