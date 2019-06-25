# gather system info...
find_program(LSB_RELEASE_EXECUTABLE NAMES lsb_release)

if (LSB_RELEASE_EXECUTABLE)
    execute_process(COMMAND lsb_release -c -s OUTPUT_VARIABLE CODENAME        OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND lsb_release -i -s OUTPUT_VARIABLE DISTRIBUTOR_ID  OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND lsb_release -r -s OUTPUT_VARIABLE RELEASE         OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    message(FATAL_ERROR "lsb_release not found!")
endif()

string(TOLOWER ${DISTRIBUTOR_ID} DISTRIBUTOR_ID)
if (NOT CPACK_GENERATOR)
    if (DISTRIBUTOR_ID STREQUAL fedora)
        set(CPACK_GENERATOR RPM)
        set(PACKAGE_LIB_DIR lib64)
        set(CPACK_SYSTEM_NAME x86_64)
        set(DISTRO_RELEASE fc${RELEASE})
    else()
        set(CPACK_GENERATOR DEB)
        set(PACKAGE_LIB_DIR lib/x86_64-linux-gnu)
        set(CPACK_SYSTEM_NAME amd64)
        set(DISTRO_RELEASE ${DISTRIBUTOR_ID}_${RELEASE})
    endif()
endif()

# todo: handle rpm on other distos...

message(STATUS "Building: ${PROJECT_VERSION} for ${DISTRIBUTOR_ID} ${RELEASE} -- ${CPACK_GENERATOR} packaging" )

set(BZAPI_LIBRARY_NAME                libbzapi)
set(CPACK_PACKAGE_VENDOR              "Bluzelle Networks")
set(CPACK_PACKAGE_CONTACT             devops@bluzelle.com)
set(CPACK_PACKAGE_NAME                ${BZAPI_LIBRARY_NAME})
set(CPACK_PACKAGE_VERSION             ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR       ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR       ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH       ${PROJECT_VERSION_PATCH})

set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}_${DISTRO_RELEASE}.${CPACK_SYSTEM_NAME})

if (CPACK_GENERATOR MATCHES "DEB")
    # todo: fill out extended description...
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY  "Bluzelle SwarmDB Client Runtime Library\n Something really interesting needs to go here to make lintian happy.")
    set(CPACK_DEBIAN_COMPRESSION_TYPE      xz)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE  ${CPACK_SYSTEM_NAME})
    set(CPACK_DEBIAN_PACKAGE_DEPENDS       "libc6, libstdc++6, libprotobuf17")
    if (USE_JSONCPP_SO)
        string(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS ", libjsoncpp1")
    endif()
    set(CPACK_DEBIAN_PACKAGE_PRIORITY      optional)
    set(CPACK_DEBIAN_PACKAGE_SECTION       database)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER    "DevOps <debops@bluzelle.com>")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${PROJECT_SOURCE_DIR}/pkg/debian/triggers;${PROJECT_SOURCE_DIR}/pkg/debian/shlibs;${PROJECT_SOURCE_DIR}/pkg/postinst)

    # changelog format: date -R
    execute_process(COMMAND gzip -n -9 -c ${PROJECT_SOURCE_DIR}/pkg/debian/changelog WORKING_DIRECTORY ${PROJECT_BINARY_DIR} OUTPUT_FILE ${PROJECT_BINARY_DIR}/changelog.gz)
    execute_process(COMMAND chmod 644 ${PROJECT_SOURCE_DIR}/pkg/debian/triggers)
    execute_process(COMMAND chmod 644 ${PROJECT_SOURCE_DIR}/pkg/debian/shlibs)
    execute_process(COMMAND chmod 755 ${PROJECT_SOURCE_DIR}/pkg/postinst)

    install(FILES ${PROJECT_SOURCE_DIR}/LICENSE              DESTINATION /usr/share/doc/${BZAPI_LIBRARY_NAME})
    install(FILES ${PROJECT_SOURCE_DIR}/pkg/debian/copyright DESTINATION /usr/share/doc/${BZAPI_LIBRARY_NAME})
    install(FILES ${PROJECT_BINARY_DIR}/changelog.gz         DESTINATION /usr/share/doc/${BZAPI_LIBRARY_NAME})
endif()

if (CPACK_GENERATOR MATCHES "RPM")
    # changelog format: date "+%a %b %d %Y"
    set(CPACK_RPM_CHANGELOG_FILE           ${PROJECT_SOURCE_DIR}/pkg/rpm/changelog)
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Bluzelle SwarmDB Client Runtime Library")
    set(CPACK_RPM_COMPRESSION_TYPE         xz)
    set(CPACK_RPM_PACKAGE_GROUP            Development/Libraries)
    set(CPACK_RPM_PACKAGE_LICENSE          AGPLv3)
    set(CPACK_RPM_PACKAGE_REQUIRES         "glibc, protobuf-devel")
    if (USE_JSONCPP_SO)
        string(APPEND CPACK_RPM_PACKAGE_REQUIRES ", jsoncpp")
    endif()
    set(CPACK_RPM_PACKAGE_URL              https://github.com/bluzelle/bzapi)

    install(FILES ${PROJECT_SOURCE_DIR}/README.md DESTINATION /usr/share/doc/${CPACK_PACKAGE_NAME})
    install(FILES ${PROJECT_SOURCE_DIR}/LICENSE   DESTINATION /usr/share/licences/${CPACK_PACKAGE_NAME})

    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr/share/licences)
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr/lib64/)

    SET(CPACK_RPM_SPEC_MORE_DEFINE "%posttrans -p /sbin/ldconfig")
endif()

set_target_properties(bzapi PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set_target_properties(bzapi PROPERTIES PUBLIC_HEADER "../include/bzapi.hpp;../include/response.hpp;../include/database.hpp;../include/async_database.hpp;../include/logger.hpp")
install(TARGETS bzapi LIBRARY DESTINATION ${PACKAGE_LIB_DIR} PUBLIC_HEADER DESTINATION include/bzapi)

include(CPack)
