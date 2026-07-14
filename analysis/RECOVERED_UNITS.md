# Source-component coverage map

This table maps every identified application source concept to its compiling
counterpart. “Combined” means that the class boundary and Qt interface are
retained in a shared translation unit rather than a file with the same basename.

| # | Component unit | Reconstructed source | Boundary |
|---:|---|---|---|
| 1 | `roadelements.cpp` | `src/road/road_types.cpp` | Combined; record layouts and map primitives |
| 2 | `pathfinder.cpp` | `src/pathfinder/pathfinder.cpp` | Direct class; control-law scaffold |
| 3 | `main.cpp` | `src/main.cpp` | Direct orchestration |
| 4 | `pixel_mapper.cpp` | `src/core/pixel_mapper.cpp` | Direct |
| 5 | `thunder_gaze_factory.cpp` | `src/vision/thunder_gaze_factory.cpp` | Direct |
| 6 | `thunder_gaze_v4.cpp` | `src/vision/thunder_gaze_versions.cpp` | Combined |
| 7 | `thunder_gaze_v5.cpp` | `src/vision/thunder_gaze_versions.cpp` | Combined |
| 8 | `crc.cpp` | `src/core/crc.cpp` | Direct |
| 9 | `camera_t205.cpp` | `src/camera/camera_t205.cpp` | Direct |
| 10 | `camera_usb.cpp` | `src/camera/camera_usb.cpp` | Direct |
| 11 | `camera_v4l.cpp` | `src/camera/camera_v4l.cpp` | Direct |
| 12 | `camera_zr10.cpp` | `src/camera/camera_zr10.cpp` | Direct |
| 13 | `lds_lidar.cpp` | `src/lidar/lds_lidar.cpp` | Direct; vendored SDK |
| 14 | `elink_arm_handler.cpp` | `src/elink/elink_arm_handler.cpp` | Direct |
| 15 | `elink_flower_handler.cpp` | `src/elink/elink_flower_handler.cpp` | Direct |
| 16 | `elink_gimbal_handler.cpp` | `src/elink/elink_gimbal_handler.cpp` | Direct |
| 17 | `elink_interceptor_handler.cpp` | `src/elink/elink_interceptor_handler.cpp` | Direct |
| 18 | `elink_shumodav_handler.cpp` | `src/elink/elink_shumodav_handler.cpp` | Direct |
| 19 | `elink_status_handler.cpp` | `src/elink/elink_status_handler.cpp` | Direct |
| 20 | `elink_telemetry.cpp` | `src/elink/elink_telemetry.cpp` | Direct |
| 21 | `emath.cpp` | `src/core/emath.cpp`, `src/core/angles.cpp` | Split by function family |
| 22 | `interceptor.cpp` | `src/interception/einterceptor.cpp` | Renamed to recovered class |
| 23 | `interceptor_lidar.cpp` | `src/lidar/interceptor_lidar.cpp`, `src/lidar/interceptor_livox.cpp` | Split interface/adapter |
| 24 | `kalman.cpp` | `src/core/kalman.cpp` | Direct |
| 25 | `log.cpp` | `src/logging/elogger.cpp` | Renamed to recovered class |
| 26 | `gimbal_transform.cpp` | `src/camera/gimbal_transform.cpp` | Direct |
| 27 | `remote_controller.cpp` | `src/remote/remote_controller.cpp` | Direct |
| 28 | `roadrunner.cpp` | `src/road/road_runner.cpp` | Direct class; bounded heuristics |
| 29 | `roads.cpp` | `src/road/roads.cpp` | Direct analyzer hierarchy |
| 30 | `thunder_gaze.cpp` | `src/vision/thunder_gaze.cpp` | Direct base class |
| 31 | `thunder_gaze_v1.cpp` | `src/vision/thunder_gaze_versions.cpp` | Combined |
| 32 | `thunder_gaze_v2.cpp` | `src/vision/thunder_gaze_versions.cpp` | Combined |
| 33 | `thunder_gaze_v3.cpp` | `src/vision/thunder_gaze_versions.cpp` | Combined |
| 34 | `vehiclecontroller.cpp` | `src/controller/vehicle_controller.cpp` | Direct class; bounded host actions |
| 35 | `clientcontroller.cpp` | `src/controller/client_controller.cpp` | Direct class |
| 36 | `scout.cpp` | `src/scout/escout.cpp` | Direct class |
| 37 | `vision.cpp` | `src/vision/vision.cpp` | Direct class |
| 38 | `vehicleled.cpp` | `src/led/vehicle_led.cpp` | Combined with GPIO controller |
| 39 | `ledcontroller.cpp` | `src/led/vehicle_led.cpp` | Combined with LED state class |
| 40 | `elink_communicator.cpp` | `src/elink/elink_communicator.cpp` | Direct |
| 41 | `elink_abstract_handler.cpp` | `src/elink/elink_abstract_handler.cpp` | Direct |
| 42 | `elink_attitude_handler.cpp` | `src/elink/elink_attitude_handler.cpp` | Direct |
| 43 | `elink_position_handler.cpp` | `src/elink/elink_position_handler.cpp` | Direct |
| 44 | `elink_movement_handler.cpp` | `src/elink/elink_movement_handler.cpp` | Filename spelling normalized; Qt class name retains the observed spelling |
| 45 | `camera.cpp` | `src/camera/camera.cpp` | Direct |
| 46 | `abstract_link.cpp` | `src/link/abstract_link.cpp` | Direct |
| 47 | `serial_link.cpp` | `src/link/serial_link.cpp` | Direct |
| 48 | `udp_link.cpp` | `src/link/udp_link.cpp` | Direct |
| 49 | `tcp_link.cpp` | `src/link/tcp_link.cpp` | Direct |
| 50 | `abstract_handler.cpp` | `src/mavlink/abstract_handler.cpp` | Direct |
| 51 | `heartbeat_handler.cpp` | `src/mavlink/heartbeat_handler.cpp` | Direct |
| 52 | `mavlink_communicator.cpp` | `src/mavlink/mavlink_communicator.cpp` | Direct |
| 53 | `log_handler.cpp` | `src/mavlink/log_handler.cpp` | Direct |
| 54 | `wind_handler.cpp` | `src/mavlink/wind_handler.cpp` | Direct |
| 55 | `control_handler.cpp` | `src/mavlink/control_handler.cpp` | Direct |
| 56 | `rc_channels_handler.cpp` | `src/mavlink/rc_channels_handler.cpp` | Direct |
| 57 | `time_sync_handler.cpp` | `src/mavlink/time_sync_handler.cpp` | Direct |
| 58 | `attitude_handler.cpp` | `src/mavlink/attitude_handler.cpp` | Direct |
| 59 | `plan_handler.cpp` | `src/mavlink/plan_handler.cpp` | Direct |
| 60 | `gps_handler.cpp` | `src/mavlink/gps_handler.cpp` | Direct |
| 61 | `vnav.cpp` | `src/vnav/vnav.cpp` | Direct |

The 53 `moc_*.cpp` concepts are regenerated from the reconstructed `Q_OBJECT`
headers by CMake `AUTOMOC`. The 22 Livox units are compiled from
`vendor/Livox-SDK`; generated MAVLink code remains header-only under
`vendor/mavlink-c-library-v2`.
