# EEBUS library main CMake file

cmake_minimum_required (VERSION 3.15)

set(PROJECT_NAME "eebus")

# Set consistent runtime library for all targets on Windows BEFORE project()
if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

include(ProcessorCount)
ProcessorCount(PROCESSORS_NUM)

set(CMAKE_ECLIPSE_MAKE_ARGUMENTS ${CMAKE_ECLIPSE_MAKE_ARGUMENTS} "--no-print-directory -j${PROCESSORS_NUM} -r -s" CACHE STRING "Make parameters")

# Check if the build run from Jenkins or locally 
project(${PROJECT_NAME} LANGUAGES C)

# TODO: Change STATIC to SHARED when libeebus is a part of SDK
add_library(${PROJECT_NAME} STATIC)

if(WIN32)
    # Set vcpkg installation paths
    set(VCPKG_ROOT $ENV{VCPKG_ROOT})
    set(VCPKG_INSTALLED "${VCPKG_ROOT}/installed/x64-windows")
    message(STATUS "Using vcpkg installed libraries from: ${VCPKG_INSTALLED}")
    
    # Add vcpkg to CMAKE_PREFIX_PATH so find_package can locate packages
    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED}")
    
    # Set proper runtime library for Windows to avoid LIBCMT conflicts
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

    # Add Bonjour SDK paths for Windows
    set(BONJOUR_SDK_HOME $ENV{BONJOUR_SDK_HOME})
    set(BONJOUR_LIB_PATH "${BONJOUR_SDK_HOME}/Lib/x64/dnssd.lib")
    if(EXISTS "${BONJOUR_SDK_HOME}" AND EXISTS "${BONJOUR_LIB_PATH}")
        target_include_directories(${PROJECT_NAME} PRIVATE "${BONJOUR_SDK_HOME}/Include")
        target_link_directories(${PROJECT_NAME} PUBLIC "${BONJOUR_SDK_HOME}/Lib/x64")
        message(STATUS "Found Bonjour SDK at: ${BONJOUR_SDK_HOME}")
        set(BONJOUR_FOUND TRUE)
    else()
        message(WARNING "Bonjour SDK not found at: ${BONJOUR_SDK_HOME}")
        message(WARNING "Please install Bonjour SDK for Windows from Apple Developer")
        set(BONJOUR_FOUND FALSE)
    endif()

    # Try to find pthreads via vcpkg with more specific library names
    find_library(PTHREAD_LIBRARY 
        NAMES pthreadVCE32 pthreadVCE3 pthreadVCE3 pthread
        PATHS "${VCPKG_INSTALLED}/lib" "${VCPKG_INSTALLED}/debug/lib"
        NO_DEFAULT_PATH)

    find_path(PTHREAD_INCLUDE_DIR 
        NAMES pthread.h
        PATHS "${VCPKG_INSTALLED}/include"
        NO_DEFAULT_PATH)

    if(PTHREAD_LIBRARY AND PTHREAD_INCLUDE_DIR)
        message(STATUS "Found pthread library: ${PTHREAD_LIBRARY}")
        message(STATUS "Found pthread include: ${PTHREAD_INCLUDE_DIR}")
        add_library(pthread_imported STATIC IMPORTED)
        set_target_properties(pthread_imported PROPERTIES IMPORTED_LOCATION "${PTHREAD_LIBRARY}")
        target_include_directories(pthread_imported INTERFACE "${PTHREAD_INCLUDE_DIR}")
        set(THREAD_LIBRARY pthread_imported)
    else()
        message(ERROR "Could not find pthread library!")
    endif()
endif()

find_package(libwebsockets CONFIG REQUIRED)

# Pick the correct libwebsockets target depending on how it was built
set(LIBWEBSOCKETS_TARGET "")
if(TARGET websockets_shared)
  set(LIBWEBSOCKETS_TARGET websockets_shared)
elseif(TARGET websockets)
  set(LIBWEBSOCKETS_TARGET websockets)
elseif(TARGET libwebsockets::websockets_shared)
  set(LIBWEBSOCKETS_TARGET libwebsockets::websockets_shared)
elseif(TARGET libwebsockets::websockets)
  set(LIBWEBSOCKETS_TARGET libwebsockets::websockets)
else()
  message(FATAL_ERROR "libwebsockets found, but no known CMake target exists (websockets/websockets_shared).")
endif()

find_package(OpenSSL REQUIRED)
find_package(cJSON REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

message(STATUS "TARGET_COMPILE_OPTIONS    " ${TARGET_COMPILE_OPTIONS})
message(STATUS "TARGET_DEFINITION_OPTIONS " ${TARGET_DEFINITION_OPTIONS})
message(STATUS "TARGET_LINK_OPTIONS       " ${TARGET_LINK_OPTIONS})

include(sources.cmake)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
    ${SOURCES}
    ${HEADERS}
)

# Add Windows-specific applink.c to have the OPENSSL_Applink() available (required for OpenSSL on Windows)
if(WIN32)
  target_sources(
    ${PROJECT_NAME}
    PUBLIC
      src/ship/tls_certificate/applink.c
  )
endif()

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Warn if there are unresolved symbols in libeebus.
# There should not be any unresolved symbols, and we don't want to
# wait until the executable is linked to find out.
if(WIN32)
  # Windows-specific linker options to avoid runtime library conflicts
  target_link_options(
    ${PROJECT_NAME}
    PRIVATE
      "/NODEFAULTLIB:LIBCMT"
  )
else()
  target_link_options(
    ${PROJECT_NAME}
    PRIVATE
      "-Wl,--no-undefined"
  )
endif()

if (APPLE)
  set(OPEN_SSL_LIBRARIES ${OPEN_SSL_LIBRARIES} OpenSSL::Crypto)
elseif(WIN32)
  # For Windows, we need to link both SSL and Crypto libraries
  # to ensure all OpenSSL functionality is available
  set(OPEN_SSL_LIBRARIES ${OPEN_SSL_LIBRARIES} libssl)
  set(OPEN_SSL_LIBRARIES ${OPEN_SSL_LIBRARIES} libcrypto)
  set(DNS_SD_LIB "dnssd")
else()
  set(DNS_SD_LIB "dns_sd")
  set(OPEN_SSL_LIBRARIES ${OPEN_SSL_LIBRARIES} crypto)
endif()

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
    ${THREAD_LIBRARY}
    cjson
    ${LIBWEBSOCKETS_TARGET}
    ${LIBWEBSOCKETS_DEP_LIBS}
    ${DNS_SD_LIB}
    ${OPEN_SSL_LIBRARIES}
)

add_subdirectory(examples/heat_pump)
add_subdirectory(examples/hems)
