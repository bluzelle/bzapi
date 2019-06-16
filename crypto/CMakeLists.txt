add_library(crypto
        crypto_base.hpp
        crypto.hpp
        crypto.cpp
        null_crypto.hpp)

target_link_libraries(crypto proto)
add_dependencies(crypto openssl)
target_include_directories(crypto PRIVATE ${BLUZELLE_STD_INCLUDES} ${PROTO_INCLUDE_DIR})
if (NOT OMIT_LIBRARY)
    add_subdirectory(test)
endif(NOT OMIT_LIBRARY)
