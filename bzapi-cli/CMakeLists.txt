add_executable(bzapi-cli main.cpp)
add_dependencies(bzapi-cli boost jsoncpp)
target_include_directories(bzapi-cli PRIVATE ${BLUZELLE_STD_INCLUDES})
target_link_libraries(bzapi-cli bzapi)
