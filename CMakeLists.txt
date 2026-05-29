cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(GDVN VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	src/*.cpp src/*.hpp src/*.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (WIN32)
    set_source_files_properties(src/services/PvpRealtimeSocketService.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:BUILD_RELEASE>
        $<$<CONFIG:RelWithDebInfo>:BUILD_RELWITHDEBINFO>
)

if (PROJECT_IS_TOP_LEVEL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BI_EXPORTING)
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} $ENV{GEODE_SDK}/build)

set(USE_TLS ON CACHE BOOL "" FORCE)
set(USE_ZLIB OFF CACHE BOOL "" FORCE)
set(BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(USE_OPEN_SSL OFF CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)

if (APPLE)
    set(USE_SECURE_TRANSPORT ON CACHE BOOL "" FORCE)
    set(USE_MBED_TLS OFF CACHE BOOL "" FORCE)
else()
    set(USE_SECURE_TRANSPORT OFF CACHE BOOL "" FORCE)
    set(USE_MBED_TLS ON CACHE BOOL "" FORCE)

    CPMAddPackage(
        NAME mbedtls
        GITHUB_REPOSITORY Mbed-TLS/mbedtls
        GIT_TAG v2.28.9
        OPTIONS
            "ENABLE_PROGRAMS OFF"
            "ENABLE_TESTING OFF"
            "MBEDTLS_FATAL_WARNINGS OFF"
            "USE_STATIC_MBEDTLS_LIBRARY ON"
            "USE_SHARED_MBEDTLS_LIBRARY OFF"
            "DISABLE_PACKAGE_CONFIG_AND_INSTALL ON"
    )

    set(MBEDTLS_FOUND TRUE)
    set(MBEDTLS_INCLUDE_DIRS "${mbedtls_SOURCE_DIR}/include")
    set(MBEDTLS_LIBRARIES mbedtls mbedx509 mbedcrypto)
endif()

CPMAddPackage(
    NAME ixwebsocket
    GITHUB_REPOSITORY machinezone/IXWebSocket
    GIT_TAG v11.4.5
    OPTIONS
        "USE_TLS ON"
        "USE_ZLIB OFF"
        "BUILD_DEMO OFF"
        "BUILD_SHARED_LIBS OFF"
        "IXWEBSOCKET_INSTALL OFF"
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
target_link_libraries(${PROJECT_NAME} ixwebsocket)

setup_geode_mod(${PROJECT_NAME} EXTERNALS geode.node-ids:1.18.0)
