# Hardware and interface architecture

This document maps the confirmed Gerbera companion-computer hardware to the
EPathfinder software components. It distinguishes physical observations,
configured production interfaces, optional backends, and the current
reconstruction's startup graph.

## Physical topology

| From | To | Physical/logical interface | Application owner | Status |
|---|---|---|---|---|
| Jetson Orin NX P3767 | LEETOP carrier | 260-contact module connector | Linux/JetPack platform | Confirmed photographically; carrier marked PN `900-14887-0000`, Rev. 2.1 |
| LEETOP carrier | Active cooling assembly | Fan header and module heat spreader | Linux thermal control | Fan and heat spreader confirmed; fan model unknown |
| LEETOP carrier M.2 Key E | Intel 8265NGW | PCIe Wi-Fi plus USB Bluetooth | Linux `iwlwifi`/Bluetooth stacks | Confirmed photographically and in kernel enumeration |
| LEETOP carrier M.2 Key M | 128 GB 2242 NVMe | PCIe Gen3 ×2 through MAXIO MAP1202 | Linux storage stack | Confirmed; unit-12702 device identifies as KingSpec NE-128 2242 |
| Jetson Orin NX | Unidentified primary flight controller | Tegra UART `ttyTHS1` as `/dev/ttyAP`; 460,800 bit/s in the retained configuration | `SerialLink` → `MavLinkCommunicator` → MAVLink handlers | Configured production path; attached board unknown |
| Jetson Orin NX | Unidentified ELink controller/modem | USB CDC ACM as `/dev/ttyFC`; 57,600 bit/s; rule expects ST VCP `0483:5740` | `SerialLink` → `ELinkCommunicator` → ELink handlers | Configured production path; attached board unknown |
| Jetson USB host | HJ-ZOOM10-4K camera | UVC 1.10, VID:PID `32e4:9415`, `/dev/video0` | `CameraUSB` → `CameraV4L` → `CameraT205` | Confirmed installed and selected |
| Jetson media engine | Camera stream | 1,920 × 1,080, 30 frames/s, MJPEG hardware decode | ThunderGaze process and `EVision` | Confirmed operating mode |
| Jetson PCIe | Intel Wireless-AC 8265 | PCIe for Wi-Fi; USB function for integrated Bluetooth | Linux network stack; ELink/network services | Confirmed hardware |
| Jetson PCIe | Realtek Gigabit Ethernet controller | PCI ID `10ec:8168`, `r8168` | Linux network stack; UDP/TCP transports | Confirmed family |
| Jetson PCIe | 128 GB NVMe | MAXIO MAP1202, PCIe Gen3 ×2 | Operating system, application, maps, logs, vision data | Confirmed; unit-specific retail identity differs |
| Jetson USB host | Four-port USB 2.0 hub | High-speed USB | Camera and USB serial expansion | Hub confirmed; downstream port topology unknown |
| Jetson GPIO namespace | Indicator and arm/execute-related external circuits | Configured line numbers 11, 12, 29, 31, and 33 | `GPIOController`, `VehicleLed` | Logical configuration confirmed; electrical mapping and loads unknown; writes are inert in this reconstruction |

The retained settings also define an optional backup serial link at 57,600
bit/s, but its device path is empty.

## Carrier interfaces and actual population

The physical board is marked LEETOP PN `900-14887-0000`, Rev. 2.1. Its layout
matches the LEETOP A603 V2.1 carrier, while the retained operating system uses
NVIDIA's P3768-0000 board-support configuration. These are compatible facts:
P3768 describes the software configuration; LEETOP identifies the physical
board. The current LEETOP catalog lists A603 PN `900-44887-0000`, so the exact
commercial SKU remains high-confidence rather than confirmed.

| Carrier facility | A603 V2.1 capability | Deployment evidence |
|---|---|---|
| Jetson module connector | Orin NX/Orin Nano, 260 contacts | Populated by the confirmed P3767-0000 Orin NX 16 GB |
| M.2 Key E | PCIe, 2230 | Populated by Intel 8265NGW; antenna connectors are visible, but antenna type and routing are unknown |
| M.2 Key M | NVMe, 2242 | Populated by a 128 GB MAXIO MAP1202-based device; unit 12702 identifies as KingSpec NE-128 2242 |
| Ethernet | 10/100/1,000 Mbit/s RJ45 | Realtek `10ec:8168` enumerated; cable and link partner are unknown |
| USB | Two USB 3 Type-A receptacles, USB 3 ZIF, USB 2 Micro-AB | UVC camera, four-port USB 2 hub, and CDC ACM device enumerated; connector-to-device and hub-port mapping are unknown |
| CSI camera | One CSI connector | No operating CSI camera established; the configured IMX219 probe failed with `-121` |
| Expansion | 40-position and 14-position headers carrying UART, CAN, GPIO, I²C, SPI, and I²S functions | Tegra UART and five configured discrete-line numbers are known; photographs do not establish their connector pins or attached circuits |
| DC input | Vendor-specified 9–20 V DC, 60 W carrier input capability | Connector is visible; aircraft supply voltage, regulator chain, battery, and power wiring are unknown |
| Fan header | Active-cooling output | Fan is installed; exact fan electrical data are unknown |

The configuration names discrete lines 11 and 12 for two indicators, line 33
for an arm-related pull-up, and lines 29 and 31 for execute-related pull-down
and pull-up functions. These integers are retained software configuration.
Without the original GPIO library semantics or continuity measurements, they
must not be equated to physical header positions, SoC GPIO indices, or voltage
levels.

## Network and local-service wiring

| Interface | Direction relative to EPathfinder | Retained configuration | Purpose | Hardware certainty |
|---|---|---|---|---|
| Client TCP service | Inbound/outbound | Any IPv4, TCP port 8052 | JSON status, plan, mode, camera, telemetry, maintenance, and control requests | Runs on confirmed Jetson network interfaces |
| VNav | UDP bidirectional | Receive 12070, transmit 12071; enabled | Navigation status, AHRS/yaw correction, plan validation, road points, targets, and odometry state | Software/configuration confirmed; separate processor or process identity unknown |
| ELink telemetry | UDP bidirectional | Base UDP port 5005 plus associated service channels | Vehicle telemetry, remote commands, plan/status exchange, launcher/configurator discovery | Protocol/configuration confirmed; radio/modem model unknown |
| Vision/ThunderGaze | Local process plus UDP | Version 5; executable and configuration paths under `/home/jetson/thunder_gaze` | Hardware-decoded video analysis, target and road detections, RTSP/module state | Process/configuration confirmed; proprietary payload fields remain incomplete in this reconstruction |
| Radar station | Outbound TCP client | Address empty, port 12030 | External target reports | Disabled by retained configuration; hardware absent or unproven |
| Remote controller | Outbound TCP client | Address empty, port 12030 | Enable/disable and roll/pitch/throttle input | Disabled by retained configuration; hardware absent or unproven |
| Odometry | Inbound UDP | Port 0 | External odometry corrections | Disabled by retained configuration |
| Tablet | Serial | Device path empty, 57,600 bit/s | Tablet JSON transport | Disabled; reconstruction does not open this link |

The retained ELink endpoint address and vehicle identifiers are deployment
configuration, not hardware part numbers. They are not used to infer a modem,
radio, antenna, or network-provider model.

## Primary sensor/control loop

```mermaid
sequenceDiagram
    participant FC as Flight controller
    participant MAV as MAVLink handlers
    participant Scout as EScout
    participant Vision as ThunderGaze / EVision
    participant PF as EPathfinder
    participant Link as ELink / client telemetry

    FC->>MAV: attitude, GNSS, airspeed, RC, system and mission telemetry
    MAV->>Scout: timestamped position, attitude and sensor events
    Vision->>Scout: road, odometry and heading-related observations
    Vision->>PF: target and image-angle observations
    Scout->>PF: fused position, attitude, validity, wind and offsets
    Link->>PF: plan, remote mode and target state
    PF->>MAV: high-level control, mission, parameter and gimbal requests
    MAV->>FC: MAVLink traffic
    PF->>Link: mode, position, target and status telemetry
```

The flight controller remains responsible for low-level stabilization and
physical actuator output. The Jetson runs the outer navigation/vision/control
layer.

## Camera and vision path

1. The installed UVC camera exposes `/dev/video0` and streams 1080p30 MJPEG.
2. The Jetson hardware media path decodes the stream.
3. ThunderGaze version 5 performs object/road processing and exchanges its
   results with `EVision`.
4. `EVision` applies a 9 × 9 angular calibration grid over a 1,920 × 1,080
   frame, a 40 ms frame delay, and the configured ignored image region.
5. Camera/gimbal history is time-aligned to detections so pixel positions can
   be projected into yaw/pitch observations.
6. Results feed road matching, target state, EScout corrections, and
   EPathfinder modes.

The physical sensor, lens, motorized zoom mechanism, and gimbal assembly behind
the `HJ-ZOOM10-4K` USB descriptor remain unknown.

## Navigation and control layers

- **MAVLink/ArduPilot layer:** heartbeat, system status, attitude, GNSS, wind,
  RC channels, time synchronization, airspeed, range, optical flow, gimbal,
  parameters, logs, and missions.
- **EScout:** temporal histories and fusion of primary/backup position,
  attitude, GNSS validity, inertial projection, ELink data, VNav corrections,
  road offsets, elevations, wind, range, and heading corrections.
- **EPathfinder:** vehicle mode state, mission progression, target/road state,
  launch state, camera state, and high-level control requests.
- **ELink:** independent telemetry and command path carrying position,
  attitude, plan, mode, target, video-state, and remote-control information.
- **VNav:** independent UDP navigation/plan-validation path with AHRS and yaw
  correlation.

## Optional software backends not proven installed

| Backend | Supported hardware family | Evidence boundary |
|---|---|---|
| `CameraZR10` | SIYI ZR10 Ethernet/UART three-axis zoom camera | Class/protocol support only; recovered units used the USB UVC camera |
| `InterceptorLivox` | Livox devices supported by SDK 2.3.0, including Tele-15, Mid-70, Avia, and Horizon families | SDK and adapter present; no installed unit or broadcast code identified |
| `EInterceptor` radar path | TCP target-report source | Address unset; device model unknown |
| `RemoteController` | TCP remote-control source | Address unset; device model unknown |
| Backup MAVLink link | Any serial endpoint accepted by `SerialLink` | Device path unset |
| Tablet link | Serial JSON terminal | Device path unset and current implementation incomplete |

## Current reconstructed startup graph

The reconstructed executable currently creates this live object graph:

```text
QCoreApplication
├── ELogger
├── VehicleController
│   ├── EScout
│   └── EPathfinder
└── ClientController
    └── TCP server :8052
```

MAVLink, ELink, camera/vision, VNav, Livox, radar, remote-control, and GPIO
classes are compiled and exercised independently, but are not yet constructed
by the current `VehicleController::Init()`. The production topology described
above is therefore an integration target supported by the recovered interfaces,
not a claim that every peripheral is active when the reconstructed executable
starts.

## Unresolved physical wiring

Storage and the supplied photographs cannot establish the operational connector
pinouts, wire colors, actual supply voltage, level shifting, isolation,
grounding, antenna routing, GPIO numbering convention, or the external circuits
connected to arm/execute lines. Those require continuity measurements, harness
labels, or a complete installed-system teardown.

## Primary carrier references

- [LEETOP A603 product specification](https://www.leetop.top/products/a603-carrier-board-for-orin-nx-orin-nano)
- [LEETOP A603 Carrier Board V2.1 manual](https://files.seeedstudio.com/Leetop_A603_Carrier_Board_V2.1%200228.pdf)
