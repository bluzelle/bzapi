add_library(crypto
    crypto_base.hpp
    crypto.hpp
    crypto.cpp
    null_crypto.hpp
    )

add_dependencies(crypto openssl boost)
target_include_directories(crypto PRIVATE ${BLUZELLE_STD_INCLUDES})

if (BUILD_TESTS)
    add_subdirectory(test)
endif()
