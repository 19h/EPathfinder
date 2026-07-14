cmake_minimum_required(VERSION 3.21)

project(EPathfinderReconstruction
    VERSION 0.1.0
    DESCRIPTION "Evidence-driven reconstruction of EPathfinder"
    LANGUAGES CXX)

configure_file(vision_calibration.json vision_calibration.json COPYONLY)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

option(EPATHFINDER_BUILD_TESTS "Build reconstruction tests" ON)
option(EPATHFINDER_USE_VENDORED_LIVOX
    "Build and link the supplied Livox SDK source" ON)
option(EPATHFINDER_USE_VENDORED_MAVLINK
    "Expose the evidence-matched MAVLink C library headers" ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Core Gui Network Positioning SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core Gui Network Positioning SerialPort)

if(EPATHFINDER_USE_VENDORED_LIVOX)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/Livox-SDK/sdk_core/CMakeLists.txt")
        message(FATAL_ERROR
            "EPATHFINDER_USE_VENDORED_LIVOX is ON but vendor/Livox-SDK is absent")
    endif()
    # Livox SDK 2.3.0 predates CMake 4's removal of compatibility below 3.5.
    # Set the policy floor in the parent without modifying the evidence-matched
    # vendored source.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    add_subdirectory(vendor/Livox-SDK EXCLUDE_FROM_ALL)
    # The SDK promotes warnings to errors. Its bundled fmt v5 triggers modern
    # AppleClang/libc++ deprecation diagnostics, so retain the warnings but
    # remove only -Werror from the vendored target without altering vendor/.
    get_target_property(_livox_compile_options livox_sdk_static COMPILE_OPTIONS)
    set(_livox_compile_options_without_werror "")
    foreach(_option IN LISTS _livox_compile_options)
        string(REPLACE "-Werror" "" _option_without_werror "${_option}")
        list(APPEND _livox_compile_options_without_werror "${_option_without_werror}")
    endforeach()
    set_property(TARGET livox_sdk_static PROPERTY COMPILE_OPTIONS
        "${_livox_compile_options_without_werror}")
endif()

if(EPATHFINDER_USE_VENDORED_MAVLINK)
    if(NOT EXISTS
       "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mavlink-c-library-v2/ardupilotmega/mavlink.h")
        message(FATAL_ERROR
            "EPATHFINDER_USE_VENDORED_MAVLINK is ON but the vendored headers are absent")
    endif()
    add_library(epathfinder_mavlink INTERFACE)
    target_include_directories(epathfinder_mavlink INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mavlink-c-library-v2")
endif()

add_library(epathfinder_core STATIC
    include/camera/camera.hpp
    include/camera/camera_t205.hpp
    include/camera/camera_types.hpp
    include/camera/camera_usb.hpp
    include/camera/camera_v4l.hpp
    include/camera/camera_zr10.hpp
    include/camera/gimbal_transform.hpp
    include/core/angles.hpp
    include/core/crc.hpp
    include/core/emath.hpp
    include/core/kalman.hpp
    include/core/pixel_mapper.hpp
    include/controller/client_controller.hpp
    include/controller/controller_types.hpp
    include/controller/vehicle_controller.hpp
    include/elink/elink_abstract_handler.hpp
    include/elink/elink_arm_handler.hpp
    include/elink/elink_attitude_handler.hpp
    include/elink/elink_communicator.hpp
    include/elink/elink_flower_handler.hpp
    include/elink/elink_gimbal_handler.hpp
    include/elink/elink_interceptor_handler.hpp
    include/elink/elink_movement_handler.hpp
    include/elink/elink_position_handler.hpp
    include/elink/elink_shumodav_handler.hpp
    include/elink/elink_status_handler.hpp
    include/led/vehicle_led.hpp
    include/logging/elogger.hpp
    include/pathfinder/pathfinder.hpp
    include/interception/einterceptor.hpp
    include/link/abstract_link.hpp
    include/link/serial_link.hpp
    include/link/tcp_link.hpp
    include/link/udp_link.hpp
    include/scout/escout.hpp
    include/scout/scout_types.hpp
    include/remote/remote_controller.hpp
    include/road/road_types.hpp
    include/road/roads.hpp
    include/road/road_runner.hpp
    include/vision/thunder_gaze.hpp
    include/vision/vision_types.hpp
    include/vision/vision.hpp
    include/vnav/vnav.hpp
    src/camera/camera.cpp
    src/camera/camera_t205.cpp
    src/camera/camera_usb.cpp
    src/camera/camera_v4l.cpp
    src/camera/camera_zr10.cpp
    src/camera/gimbal_transform.cpp
    src/core/angles.cpp
    src/core/crc.cpp
    src/core/emath.cpp
    src/core/kalman.cpp
    src/core/pixel_mapper.cpp
    src/controller/client_controller.cpp
    src/controller/vehicle_controller.cpp
    src/elink/elink_abstract_handler.cpp
    src/elink/elink_arm_handler.cpp
    src/elink/elink_attitude_handler.cpp
    src/elink/elink_communicator.cpp
    src/elink/elink_flower_handler.cpp
    src/elink/elink_gimbal_handler.cpp
    src/elink/elink_interceptor_handler.cpp
    src/elink/elink_movement_handler.cpp
    src/elink/elink_position_handler.cpp
    src/elink/elink_shumodav_handler.cpp
    src/elink/elink_status_handler.cpp
    src/led/vehicle_led.cpp
    src/logging/elogger.cpp
    src/pathfinder/pathfinder.cpp
    src/interception/einterceptor.cpp
    src/link/abstract_link.cpp
    src/link/serial_link.cpp
    src/link/tcp_link.cpp
    src/link/udp_link.cpp
    src/scout/escout.cpp)
target_sources(epathfinder_core PRIVATE src/road/road_types.cpp)
target_sources(epathfinder_core PRIVATE
    src/road/roads.cpp
    src/road/road_runner.cpp)
target_sources(epathfinder_core PRIVATE
    src/vision/thunder_gaze.cpp
    src/vision/thunder_gaze_factory.cpp
    src/vision/thunder_gaze_versions.cpp)
target_sources(epathfinder_core PRIVATE src/vision/vision.cpp)
target_sources(epathfinder_core PRIVATE src/remote/remote_controller.cpp)
target_sources(epathfinder_core PRIVATE src/vnav/vnav.cpp)

if(EPATHFINDER_USE_VENDORED_MAVLINK)
    target_sources(epathfinder_core PRIVATE
        include/mavlink/abstract_handler.hpp
        include/mavlink/attitude_handler.hpp
        include/mavlink/control_handler.hpp
        include/mavlink/gps_handler.hpp
        include/mavlink/heartbeat_handler.hpp
        include/mavlink/log_handler.hpp
        include/mavlink/mavlink_communicator.hpp
        include/mavlink/plan_handler.hpp
        include/mavlink/rc_channels_handler.hpp
        include/mavlink/time_sync_handler.hpp
        include/mavlink/wind_handler.hpp
        include/elink/elink_telemetry.hpp
        src/mavlink/abstract_handler.cpp
        src/mavlink/attitude_handler.cpp
        src/mavlink/control_handler.cpp
        src/mavlink/gps_handler.cpp
        src/mavlink/heartbeat_handler.cpp
        src/mavlink/log_handler.cpp
        src/mavlink/mavlink_communicator.cpp
        src/mavlink/plan_handler.cpp
        src/mavlink/rc_channels_handler.cpp
        src/mavlink/time_sync_handler.cpp
        src/mavlink/wind_handler.cpp
        src/elink/elink_telemetry.cpp)
endif()

target_include_directories(epathfinder_core PUBLIC include)
target_link_libraries(epathfinder_core PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Positioning
    Qt${QT_VERSION_MAJOR}::SerialPort)

if(EPATHFINDER_USE_VENDORED_LIVOX)
    target_sources(epathfinder_core PRIVATE
        include/lidar/interceptor_lidar.hpp
        include/lidar/interceptor_livox.hpp
        include/lidar/lds_lidar.hpp
        src/lidar/interceptor_lidar.cpp
        src/lidar/interceptor_livox.cpp
        src/lidar/lds_lidar.cpp)
    target_link_libraries(epathfinder_core PUBLIC livox_sdk_static)
    target_compile_definitions(epathfinder_core PUBLIC
        EPATHFINDER_HAS_LIVOX_SDK=1)
endif()

if(EPATHFINDER_USE_VENDORED_MAVLINK)
    target_link_libraries(epathfinder_core PUBLIC epathfinder_mavlink)
    target_compile_definitions(epathfinder_core PUBLIC
        EPATHFINDER_HAS_MAVLINK=1)
endif()

target_compile_options(epathfinder_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>)

add_executable(epathfinder_reconstructed src/main.cpp)
target_link_libraries(epathfinder_reconstructed PRIVATE epathfinder_core)

if(EPATHFINDER_BUILD_TESTS)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(epathfinder_core_tests tests/core_tests.cpp)
    target_link_libraries(epathfinder_core_tests PRIVATE epathfinder_core)
    add_test(NAME epathfinder_core_tests COMMAND epathfinder_core_tests)
    add_executable(epathfinder_meta_dump tools/meta_dump.cpp)
    target_link_libraries(epathfinder_meta_dump PRIVATE epathfinder_core)
    add_test(NAME epathfinder_metaobject_test
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_metaobjects.py
            --inventory
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/generated/metaobjects.json
            --dump
            $<TARGET_FILE:epathfinder_meta_dump>)
endif()
