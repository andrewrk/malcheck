cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0046 NEW)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

project(malcheck C)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set(MALCHECK_VERSION_MAJOR 0)
set(MALCHECK_VERSION_MINOR 0)
set(MALCHECK_VERSION_PATCH 0)
set(MALCHECK_VERSION "${MALCHECK_VERSION_MAJOR}.${MALCHECK_VERSION_MINOR}.${MALCHECK_VERSION_PATCH}")
message("Configuring malcheck version ${MALCHECK_VERSION}")

set(MALCHECK_SOURCES
    "${CMAKE_SOURCE_DIR}/src/malcheck.c"
)

set(LIBMALCHECK_SOURCES
    "${CMAKE_SOURCE_DIR}/src/libmalcheck.c"
)

set(CONFIGURE_OUT_FILE "${CMAKE_BINARY_DIR}/config.h")
configure_file (
    "${CMAKE_SOURCE_DIR}/src/config.h.in"
    ${CONFIGURE_OUT_FILE}
)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wno-unused-variable -Wno-unused-but-set-variable")

set(EXE_CFLAGS "-std=c11 -Werror -Wall -Werror=strict-prototypes -Werror=old-style-definition -Werror=missing-prototypes")
set(LIB_CFLAGS "-std=c11 -Werror -Wall -Werror=strict-prototypes -Werror=old-style-definition")
set(TEST_CFLAGS "-std=c99 -Werror -Wall")

add_library(libmalcheck SHARED ${LIBMALCHECK_SOURCES})
set_target_properties(libmalcheck PROPERTIES
    OUTPUT_NAME malcheck
    SOVERSION ${MALCHECK_VERSION_MAJOR}
    VERSION ${MALCHECK_VERSION}
    COMPILE_FLAGS ${LIB_CFLAGS}
    LINKER_LANGUAGE C
)
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
)
target_link_libraries(libmalcheck LINK_PUBLIC
    dl
)
install(TARGETS libmalcheck DESTINATION lib)

add_executable(malcheck ${MALCHECK_SOURCES})
set_target_properties(malcheck PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS ${EXE_CFLAGS})
install(TARGETS malcheck DESTINATION bin)

add_executable(test_simple "test/simple.c")
set_target_properties(test_simple PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS ${TEST_CFLAGS})
