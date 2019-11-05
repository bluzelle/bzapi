add_executable(bzapi-cli main.cpp)

if (USE_JSONCPP_SO)
    add_dependencies(bzapi-cli boost)
else()
    add_dependencies(bzapi-cli boost jsoncpp)
endif()

target_include_directories(bzapi-cli PRIVATE ${BLUZELLE_STD_INCLUDES})
target_link_libraries(bzapi-cli bzapi)
