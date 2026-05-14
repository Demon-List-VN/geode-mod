cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(GDVN VERSION 1.0.0)

if (APPLE)
    enable_language(OBJCXX)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	src/*.cpp src/*.hpp src/*.h
)

if (APPLE)
    list(APPEND SOURCES src/services/PvpRealtimeSocket.mm)
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCES}
        src/services/CheatGuard.cpp)

if (APPLE)
    set_source_files_properties(src/services/PvpRealtimeSocket.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
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

setup_geode_mod(${PROJECT_NAME} EXTERNALS geode.node-ids:1.18.0)
