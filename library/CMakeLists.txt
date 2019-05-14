add_library(bzapi SHARED library.cpp library.hpp ../include/boost_asio_beast.hpp ../database/db_base.hpp response.hpp udp_response.hpp ../bzapi.hpp)
include_directories(${PROTOBUF_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIRS})
set_source_files_properties(../swig/py/bzpy.i PROPERTIES CPLUSPLUS ON)
include_directories(bzapi ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/database)
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
target_include_directories(bzapi PRIVATE . ${JSONCPP_INCLUDE_DIRS})

target_link_libraries(bzapi crypto proto swarm node database ${JSONCPP_LIBRARIES} ${Protobuf_LIBRARIES} ${OPENSSL_LIBRARIES} pthread)
target_compile_features(bzapi PUBLIC cxx_std_17)

find_package(PythonLibs 3 REQUIRED)
find_package(PythonInterp ${PYTHONLIBS_VERSION_STRING} REQUIRED)

if(PYTHONLIBS_VERSION_STRING MATCHES "^2.*$" )
    set(CMAKE_SWIG_FLAGS -classic -c++)
else()
    set(CMAKE_SWIG_FLAGS -py3 -c++)
endif()


find_package(Boost REQUIRED)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

set(SWIG_MODULE_NAME bzpy)

add_compile_options("-fdiagnostics-color=auto")
add_compile_options("-fPIC")
add_compile_options("-pthread")
add_compile_options("-Wall")
add_compile_options("-Wextra")
add_compile_options("-Wno-unused-parameter")

include_directories(bzpy ${PYTHON_INCLUDE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

swig_add_library(bzpy TYPE SHARED LANGUAGE python SOURCES ../swig/py/bzpy.i)
swig_link_libraries(bzpy ${PYTHON_LIBRARIES} bzapi)
set_target_properties(_bzpy PROPERTIES SUFFIX ".so")

add_subdirectory(test)