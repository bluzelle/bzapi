find_package(PythonLibs 3 REQUIRED)

if(PYTHONLIBS_VERSION_STRING MATCHES "^2.*$" )
    set(CMAKE_SWIG_FLAGS -classic -c++)
else()
    set(CMAKE_SWIG_FLAGS -py3 -c++)
endif()

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

include_directories(bzapi ${PYTHON_INCLUDE_PATH})

set_source_files_properties(../swig/py/bzapi.i PROPERTIES CPLUSPLUS ON)
swig_add_library(bzapi TYPE SHARED LANGUAGE python SOURCES ../swig/py/bzapi.i library.cpp library.hpp ../database/db_base.hpp response.hpp udp_response.hpp ../bzapi.hpp)
swig_link_libraries(bzapi crypto proto swarm node database ${PYTHON_LIBRARIES})
set_target_properties(_bzapi PROPERTIES SUFFIX ".so")

add_subdirectory(test)