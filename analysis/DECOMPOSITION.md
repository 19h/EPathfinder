# EPathfinder software decomposition

## System role

EPathfinder is the Jetson-resident outer-loop autonomy and integration service
for the examined Gerbera aircraft configuration. A separate flight controller
performs low-level stabilization and actuator control. The Jetson application
adds camera processing, navigation fusion, mission/road logic, network control,
telemetry, and optional target-sensor integration.

The application is organized around Qt objects and signal/slot boundaries. The
reconstructed interface inventory contains 57 Qt classes and 819 methods,
including 284 signals.

## Top-level orchestration

| Component | Responsibility | Direct dependencies |
|---|---|---|
| `main.cpp` | Process identity, logger setup, controller construction, request/status wiring, event loop | `ELogger`, `VehicleController`, `ClientController` |
| `ClientController` | TCP JSON service, per-client buffering, cookie-correlated responses, message dispatch | `QTcpServer`, `QTcpSocket`, `VehicleStatus` |
| `VehicleController` | Application-level lifecycle, status, configuration, and translation between client requests and vehicle operations | `EScout`, `EPathfinder`, subsystem controller interfaces |
| `EPathfinder` | Mode state, mission/target/road coordination, high-level control outputs, camera and launch state | `EScout`, MAVLink handlers, ELink, vision, VNav, interception |
| `EScout` | Time-aligned navigation/sensor histories and correction state | MAVLink, ELink, VNav, road runner, odometry, elevation data |

## Flight-controller and transport layer

`AbstractLink` defines the transport contract. Implementations cover:

- `SerialLink` for `/dev/ttyAP`, `/dev/ttyFC`, and optional backup/tablet
  endpoints;
- `UdpLink` for datagram-based autopilot or service paths;
- `TcpLink` for stream-based external endpoints.

`MavLinkCommunicator` multiplexes links and associates each link with MAVLink
channel and role identifiers. Its handler layer is divided by protocol family:

| Handler | Input/output role |
|---|---|
| `HeartbeatHandler` | Connection state, vehicle/mode/arm state, system status, parameters, IMU/range/flow/gimbal status |
| `AttitudeHandler` | Primary and backup attitude, AHRS correction |
| `GpsHandler` | GNSS fixes, satellite state, velocity, altitude |
| `WindHandler` | Wind estimates and updates |
| `RcChannelsHandler` | RC channel values and button/mode state |
| `TimeSyncHandler` | Flight-controller/system timestamp alignment |
| `ControlHandler` | RC override, mount/gimbal, mission-side control, and auxiliary channels |
| `PlanHandler` | JSON plans, MAVLink mission transfer, road graph, target images |
| `LogHandler` | Log enumeration and chunk transfer |

The retained deployment configuration maps the primary MAVLink stream to the
Jetson UART `/dev/ttyAP` at 460,800 bit/s. The flight-controller hardware model
is unknown.

## ELink subsystem

`ELinkCommunicator` frames and multiplexes the proprietary ELink protocol. Its
handler set separates arm, attitude, position, movement, gimbal, target,
status, and telemetry messages. `ELinkTelemetry` also provides UDP telemetry
and associated command/service channels.

The retained configuration maps the local ELink stream to `/dev/ttyFC` at
57,600 bit/s. Forensic evidence identifies `/dev/ttyFC` as USB CDC ACM and shows
that the persistent-device rule expects an STMicroelectronics USB virtual COM
interface. The attached ELink/controller PCB remains unknown.

## Camera and vision subsystem

The installed camera path is:

```text
HJ-ZOOM10-4K UVC camera
    → /dev/video0
    → 1,920 × 1,080 @ 30 frames/s MJPEG
    → Jetson hardware decoder
    → ThunderGaze version 5
    → EVision
    → angular target/road observations
```

Application camera classes provide two backend families:

- `CameraUSB` → `CameraV4L` → `CameraT205` implements the configured UVC/V4L2
  path, device-presence checks, zoom control, and a 21-point calibration table.
- `CameraZR10` implements the SIYI packet protocol over UDP or serial, gimbal
  polling, CRC, autofocus, auto-centering, and a 24-point zoom calibration
  table. It is supported but not proven installed in either examined aircraft.

`EVision` owns camera selection, ThunderGaze integration, the 9 × 9 image-angle
calibration grid, frame timing, ignored image bounds, target projection, and
road observations. ThunderGaze versions 1–5 preserve distinct message extents
and Qt interfaces, while proprietary version payload decoding remains
incomplete.

## Navigation and state fusion

`EScout` maintains bounded histories for:

- primary and backup position;
- primary and ELink attitude;
- raw GNSS and inertial estimates;
- wind and airspeed;
- range/ground-height state;
- VNav yaw and position corrections;
- road-map offsets and validation;
- exact-heading calibration;
- optional odometry.

It projects position and attitude to requested timestamps, selects navigation
sources, applies altitude and heading corrections, and exposes a unified state
surface to EPathfinder.

`VNav` is an independent UDP navigation interface. It sends current IMU state,
operating mode, plan segments, and road points; receives position, status,
targets, plan-check results, image targets, and AHRS data; and correlates AHRS
timestamps with a 512-sample yaw history.

## Road and mission subsystem

`PlanHandler` parses mission nodes, wind data, road nodes/links, and target
images, then manages mission transfer and road-map construction.

Road processing is divided into:

- record and map types (`RoadPoint`, `RoadItem`, `RoadElement`, `RoadMap`);
- graph analyzers (`RoadAnalyzer`, V2 and network variants);
- `RoadRunner`, which ingests timestamped road observations, projects them
  against the route graph, and maintains temporary/accepted offsets.

Primitive geometry and public state transitions are implemented. Ambiguous
junction selection and multi-frame confidence remain bounded approximations.

## Interception and optional ranging

`EInterceptor` accepts external target reports over TCP. It can also instantiate
`InterceptorLivox`, which uses Livox SDK 2.3.0 to receive point clouds and emit
clustered aircraft-relative coordinates.

Neither radar address nor an installed Livox sensor is present in the retained
configuration/evidence. These are optional interfaces, not part of the
confirmed two-aircraft bill of materials.

## Client, telemetry, and host services

`ClientController` listens on TCP port 8052, accepts concatenated JSON objects,
routes replies by cookie, and exposes status, configuration, plan, camera,
telemetry, mode, calibration, and maintenance requests.

`RemoteController` and the radar client are optional outbound TCP services.
`ELogger` provides application logs. `GPIOController`/`VehicleLed` represent two
indicators plus configured arm/execute-related discrete outputs; physical
electrical behavior is not implemented in the current reconstruction.

## Current executable versus production topology

The current reconstructed startup path instantiates:

```text
main
├── ELogger
├── VehicleController
│   ├── EScout
│   └── EPathfinder
└── ClientController / TCP :8052
```

All peripheral classes compile and have focused tests, but the current
`VehicleController::Init()` does not yet create the complete production
MAVLink, ELink, vision, VNav, Livox, radar, remote-control, or GPIO graph. This
is the principal orchestration gap, separate from the autonomous-control and
ThunderGaze algorithm gaps documented in
[RECOVERY_STATUS.md](../RECOVERY_STATUS.md).

## Source partition

The application component map comprises 61 authored source concepts, 53 Qt
meta-object generation units, 22 Livox SDK units, and supporting runtime code.
The complete source mapping is maintained in
[RECOVERED_UNITS.md](RECOVERED_UNITS.md).
