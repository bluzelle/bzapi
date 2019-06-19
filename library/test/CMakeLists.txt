set(test_srcs library_test.cpp)
set(test_libs bzapi ${Protobuf_LIBRARIES})

add_gmock_test(library)
