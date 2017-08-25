# This file has been adapted from https://github.com/jacketizer/libnmea/blob/master/CMakeLists.txt
# (MIT License  THANKS)

cmake_minimum_required (VERSION 2.8)
project(NeoGPS CXX)

option(NEO_GPS_BUILD_STATIC_LIB "Build static NeoGPS" ON)
option(NEO_GPS_BUILD_SHARED_LIB "Build shared NeoGPS" ON)
option(NEO_GPS_BUILD_EXAMPLES "Build examples" ON)
option(NEO_GPS_EXAMPLES_LINK_STATIC "Link examples statically" OFF)

if (NOT NEO_GPS_BUILD_STATIC_LIB AND NOT NEO_GPS_BUILD_SHARED_LIB)
    message(FATAL_ERROR "You must build either shared or static lib, or both")
endif()

if (NOT NEO_GPS_BUILD_SHARED_LIB)
    set(NEO_GPS_EXAMPLES_LINK_STATIC ON)
endif()

if (NOT NEO_GPS_BUILD_STATIC_LIB)
    message("Linking examples/unit tests to shared lib since NEO_GPS_BUILD_STATIC_LIB is turned off")
    set(NEO_GPS_EXAMPLES_LINK_STATIC OFF)
endif()

# Set some nicer output dirs.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

set(NEO_GPS_SRC
    src/DMS.cpp
    src/GPSTime.cpp
    src/Location.cpp
    src/NeoTime.cpp
    src/NMEAGPS.cpp
    src/Streamers.cpp
    src/ublox/ubxGPS.cpp
    src/ublox/ubxmsg.cpp
    src/ublox/ubxNMEA.cpp)

set(NEO_GPS_HDR
    src/CosaCompat.h
    src/DMS.h
    src/GPSfix_cfg.h
    src/GPSfix.h
    src/GPSport.h
    src/GPSTime.h
    src/Location.h
    src/NeoGPS_cfg.h
    src/NeoTime.h
    src/NMEAGPS_cfg.h
    src/NMEAGPS.h
    src/Platform.h
    src/Streamers.h
    src/ublox/ubx_cfg.h
    src/ublox/ubxGPS.h
    src/ublox/ubxmsg.h
    src/ublox/ubxNMEA.h)

source_group("Headers" FILES ${NEO_GPS_HDR})
source_group("Source" FILES ${NEO_GPS_SRC})

if (NEO_GPS_BUILD_STATIC_LIB)
    add_library(neogps STATIC ${NEO_GPS_SRC})
    target_link_libraries(neogps dl)
    install(TARGETS neogps DESTINATION lib)
endif()

if (NEO_GPS_BUILD_SHARED_LIB)
    if (POLICY CMP0042)
        cmake_policy(PUSH)
        cmake_policy(SET CMP0042 OLD)
    endif()

    add_library(neogps_shared SHARED ${NEO_GPS_SRC})
    target_link_libraries(neogps_shared dl)

    if (UNIX)
        set_target_properties(neogps_shared PROPERTIES OUTPUT_NAME neogps)
    endif()

    install(TARGETS neogps_shared DESTINATION lib)

    if (POLICY CMP0042)
        cmake_policy(POP)
    endif()
endif()

# And copy headers to build dir.
foreach (HDR ${PARSERS_HDRS})
    get_filename_component(HDR_NAME ${HDR} NAME_WE)
    message("${HDR}")
    configure_file(${HDR} ${PROJECT_BINARY_DIR}/include/neogps/${HDR_NAME}.h COPYONLY)
endforeach()

include_directories("${PROJECT_BINARY_DIR}/include/")

if (NEO_GPS_BUILD_EXAMPLES)
    # Find all example sources.
    file(GLOB EXAMPLE_SRCS
        RELATIVE "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/examples/*/*.cpp")

    foreach (EXAMPLE_SRC ${EXAMPLE_SRCS})
        get_filename_component(EXAMPLE_NAME ${EXAMPLE_SRC} NAME_WE)

        add_executable(${EXAMPLE_NAME} ${EXAMPLE_SRC})

        if (NEO_GPS_EXAMPLES_LINK_STATIC)
            target_link_libraries(${EXAMPLE_NAME} neogps)
        else()
            target_link_libraries(${EXAMPLE_NAME} neogps_shared)
        endif()
    endforeach()
endif()

