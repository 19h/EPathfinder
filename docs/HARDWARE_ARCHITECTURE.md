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
| Jetson Orin NX module | Texas Instruments INA3221 | I²C address `0x40`; `VDD_IN`, `VDD_CPU_GPU_CV`, and `VDD_SOC`; each configured for a 5 mΩ shunt | Kernel `ina3221` driver and `jtop` | Confirmed active module power monitor; does not identify propulsion power hardware |
| Jetson Orin NX | Historical `MATEKH743` autopilot target; exact PCB unknown | Tegra UART `ttyTHS1` as `/dev/ttyAP`; 460,800 bit/s in the retained configuration | `SerialLink` → `MavLinkCommunicator` → MAVLink handlers | Interface confirmed. Shared deleted telemetry identifies an earlier ArduPilot/ArduPlane 4.5.7 `MATEKH743` target with custom tag `45INAV06`; cloning prevents assignment of a PCB/revision to either aircraft |
| Jetson Orin NX | Unidentified ELink controller/peripheral | USB CDC ACM as `/dev/ttyFC`; 57,600 bit/s; rule expects ST VCP `0483:5740` | `SerialLink` → `ELinkCommunicator` → ELink handlers | Confirmed live path: unit 12674 received six arm-test on/off cycles. Shared deleted factory/test residue identifies an earlier matching peer as `Mordor` / `Flight adapter`, revision 2.00; deployed PCB and MCU unknown |
| Jetson USB host | HJ-ZOOM10-4K camera | UVC 1.10, Ailipu-assigned VID:PID `32e4:9415`, `/dev/video0` | `CameraUSB` → `CameraV4L` → `CameraT205` | Confirmed installed and selected; deleted factory/test descriptors add manufacturer string `HJ` and revision 4.19 but cannot assign a historical serial to either aircraft; sensor/lens unknown |
| Jetson media engine | Camera stream | 1,920 × 1,080, 30 frames/s, MJPEG hardware decode | ThunderGaze process and `EVision` | Confirmed operating mode |
| Jetson PCIe | Intel Wireless-AC 8265 | PCIe for Wi-Fi; USB function for integrated Bluetooth | Linux network stack; ELink/network services | Confirmed hardware |
| Jetson PCIe | Realtek Gigabit Ethernet controller | PCI ID `10ec:8168`, `r8168` | Linux network stack; UDP/TCP transports | Confirmed family |
| Jetson PCIe | 128 GB NVMe | MAXIO MAP1202, PCIe Gen3 ×2 | Operating system, application, maps, logs, vision data | Confirmed; unit-specific retail identity differs |
| Jetson USB host | Four-port USB 2.0 hub | High-speed USB | Camera and USB serial expansion | Hub confirmed. Shared deleted factory/test residue records `05e3:0608`, product `USB2.0 Hub`, revision 60.90; exact deployed controller IC and downstream physical mapping remain unknown |
| Jetson GPIO namespace | Unproven indicator and arm/execute-related circuits | Configured line numbers 11, 12, 29, 31, and 33 | `GPIOController`, `VehicleLed` | Settings confirmed. In both the historical 3613 and original recovered unit-12702 3793M executables the LED methods immediately return and arm/execute methods only log; there are no GPIO writes. Unit 12674's 3815M binary is unavailable |

The retained settings also define an optional backup serial link at 57,600
bit/s, but its device path is empty.

## Provisioned vehicle envelope

Both recovered deployments repeatedly start with internal vehicle enum 9. In
the unit-12702 executable, enum 9 is `SH10_5`; its vehicle-information record is
serialized as `TANDEM`, `ATTACK`, `BEV`, `weight=17.0`, and `payload=5.0`.
These are the production application's selected airframe-layout, role,
propulsion-class, and catalog fields. They are stronger than unused feature
strings, but remain provisioning evidence rather than independent measurement
of the physical airframe.

Unit 12674's executable is absent. It did, however, log enum 9 under the same
reported `3793M` build as the recovered executable and retained enum 9 under
the later `3815M` build. Applying the `SH10_5` record to that unit is therefore
a strong cross-version inference, not byte-for-byte confirmation.

`BEV` is distinct from the executable's `GAS` and `TJE` engine classes, so the
selected architecture is battery-electric. The retained throttle command
range is 1500–1900 with a base of 1650, and version 9 enables the software's
reversible-throttle path. The motor, propeller, ESC, motor count, battery,
power distribution, and physical reverse capability remain unidentified.

The catalog does not state units for weight 17.0 or payload 5.0. `ATTACK` and
the payload value must not be converted into a claim about an installed
warhead. The physical manufacturer, dimensions, tandem-wing geometry,
construction materials, and servo arrangement also remain unknown. `GERBERA`
is a separate executable enum (99), not the selected enum; Gerbera naming in
this repository comes from the forensic case context.

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
| Expansion | 40-position and 14-position headers carrying UART, CAN, GPIO, I²C, SPI, and I²S functions | Tegra UART is known. Five discrete-line settings survive, but the recovered unit-12702 executable does not drive them; photographs do not establish connector pins or attached circuits |
| DC input | Vendor-specified 9–20 V DC, 60 W carrier input capability | Connector is visible; a shared historical test reports about 24.7–24.8 V vehicle/battery telemetry, which cannot have fed this input directly without conversion. Supply rail, regulator chain, battery, and wiring remain unknown |
| Fan header | Active-cooling output | Fan is installed; exact fan electrical data are unknown |
| Power monitor | TI INA3221, three current/bus-voltage channels over I²C | Active at address `0x40`; software exposes it through hwmon. Recovered labels are `VDD_IN`, `VDD_CPU_GPU_CV`, and `VDD_SOC`, each with a 5 mΩ device-tree shunt value; NVIDIA identifies these as module-power domains |

The configuration names discrete line 11 for the green indicator, line 12 for
red, line 33 for an arm-related pull-up, and lines 29 and 31 for
execute-related pull-down and pull-up functions. These integers are retained
software configuration. In the original recovered unit-12702 executable, the
`GPIOController` constructor ignores its final three integer arguments and
both `VehicleLed` methods return
without performing work. `GPIOController::armTestOn`, `armTestOff`, `armOn`,
and `selfDestruction` only log and return; the binary has no GPIO library,
sysfs path, or gpiod access. They must not be equated to driven physical header
positions, SoC GPIO indices, voltage levels, or attached terminal hardware.
The historical 3613 executable has the same implementation, establishing
version persistence rather than a one-off reconstruction artifact. The later
unit-12674 3815M executable is absent, so its implementation cannot be checked
directly.

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
    participant FC as Historical MATEKH743 target / flight controller
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
4. The deployed empty `camera_ip` setting selects the `CameraT205` USB/V4L2
   object. Its 21-point calibration models full field of view from 60.0° ×
   32.3° at 0% zoom to 9.0° × 5.0° at 100%, while V4L2
   `zoom_absolute` controls drive the camera. `EVision` also loads a 9 × 9
   JSON grid, a 40 ms frame delay, and an ignored image region; code tracing
   shows that the grid's 120.02° × 71.30° central-axis span is the
   no-camera fallback path, not the deployed USB-camera projection model.
5. Camera/gimbal history is time-aligned to detections so pixel positions can
   be projected into yaw/pitch observations.
6. Results feed road matching, target state, EScout corrections, and
   EPathfinder modes.

A shared historical session enumerates absolute zoom 0–736, absolute focus
0–289, continuous autofocus, automatic exposure, and automatic white balance
on `/dev/video0`; it reads zoom 155 and commands the 736 endpoint. The same
session carries MAVLink gimbal-attitude traffic from component 154 with device
IDs 1, 2, and 3. Neither the control ranges nor those protocol IDs identify a
sensor, lens, or gimbal product.

USB-IF assigns VID `32e4` to Ailipu Technology Co., Ltd. This constrains the
registered USB-interface vendor but not the downstream optical assembly. The
physical sensor, lens, motorized zoom mechanism, and gimbal assembly behind the
`HJ-ZOOM10-4K` USB descriptor remain unknown. The internal `CameraT205` class
name and its approximately 6.5–6.7× angular narrowing are calibration evidence,
not a camera model or focal-length specification.

## Flight controller and terminal interface boundary

The primary autopilot is connected through the Jetson-native UART `/dev/ttyAP`,
so it provides no direct USB enumeration. A deleted EPathfinder session shared
exactly by both APP images does retain its MAVLink `AUTOPILOT_VERSION` response:
ArduPilot/ArduPlane version `0x040507ff` (4.5.7, stable-version class), custom
tag `45INAV06`, board version `0x03f50000`, vendor/product values `1209:5740`,
and ChibiOS hash `6a85082c`. ArduPilot's board registry maps the encoded APJ ID
1013 to `AP_HW_MATEKH743`; the official target uses an STM32H743xx MCU.

This is a target-family identification, not an exact PCB or per-aircraft
installation. Matek uses `MATEKH743` for H743-WING, WLITE, SLIM, and MINI
products across multiple revisions. The historical fragment is byte-identical
at the same physical offset in both cloned images, and its UID field is zero.
No exact board serial, form factor, or revision can therefore be assigned.

The session contains live primary inertial and magnetic data, a healthy-compass
transition, absolute pressure around 993.3 hPa, and differential pressure. It
also shows the autopilot GPS input with no fix while ELink reports a separate
25–27-satellite solution at approximately 10 Hz. The 62 surviving ELink fixes
cover only 0.111 m north-to-south by 0.025 m east-to-west in 6.105 seconds,
with mean reported ground speed 0.046 m/s. The controller is disarmed, throttle
remains zero, relative altitude remains close to zero, and its distance-sensor
altitude is zero throughout. This is a stationary integration/test session, not
a flight trace. Standard `MATEKH743` definitions span ICM20602,
MPU6000, ICM42605, and ICM42688P inertial variants, have no onboard compass,
and commonly probe DPS310 pressure hardware. Those definitions narrow the
candidate family; they do not prove an IMU, external compass, barometer,
differential-pressure sensor, or GNSS receiver model in either aircraft.

The ELink fixes center on `55.692162906, 49.250045682`. That point is about
565 m inside the current OpenStreetMap boundary for the Kazan Higher Tank
Command School training range near Usady and lies on an isolated rectangular
structure in the supplied satellite view. Continuous receiver/local counters,
changing satellite/velocity data, and centimetre-scale jitter strongly favor
an active stationary GNSS feed, although replay cannot be excluded. Since the
same deleted bytes occur at the same physical offset in both cloned APP images,
the location is evidence about a common historical commissioning/master-image
session, not the location history of either deployed aircraft. Full evidence
and geographic caveats are in
[the forensic hardware report](FORENSIC_HARDWARE.md#elink-gnss-geolocation-and-session-state).

The STM32 VCP identity `0483:5740` belongs to the separate `/dev/ttyFC` ELink
path. Unit 12674 recorded six incoming arm-test on/off cycles, establishing a
responding external ELink channel. That is not enough to call the peer a fuze:
its PCB, MCU, downstream electrical load, initiator, payload, and any warhead
remain unknown. ST publishes this VID:PID as a reusable virtual-COM example,
so it cannot identify a particular STM32 family or board. A deleted shared
factory/test boot records one matching device's self-reported manufacturer and
product as `Mordor` / `Flight adapter`, plus serial
`180028000747333031313638`. Because both UDA captures contain the exact same
deleted bytes, that serial cannot be assigned to either deployed aircraft and
does not identify the main UART-connected autopilot.

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
pinouts, wire colors, actual rail voltages, level shifting, isolation, grounding,
antenna routing, GPIO numbering convention, or the external circuits that might
have been intended for the configured arm/execute lines. A shared historical
session reports about 24.7–24.8 V at vehicle-telemetry level, not where or how
that voltage was distributed. The unit-12702 executable performs no writes on
the configured GPIO paths. Establishing any external circuit requires
continuity measurements, harness labels, or a complete installed-system
teardown.

## Primary references

- [NVIDIA Orin Nano/NX platform power-monitor documentation](https://docs.nvidia.com/jetson/archives/r36.4.4/DeveloperGuide/SD/PlatformPowerAndPerformance/JetsonOrinNanoSeriesJetsonOrinNxSeriesAndJetsonAgxOrinSeries.html)
- [LEETOP A603 product specification](https://www.leetop.top/products/a603-carrier-board-for-orin-nx-orin-nano)
- [LEETOP A603 Carrier Board V2.1 manual](https://files.seeedstudio.com/Leetop_A603_Carrier_Board_V2.1%200228.pdf)
- [USB-IF assigned USB vendor IDs](https://www.usb.org/sites/default/files/usb_vids_080223.pdf)
- [MAVLink `AUTOPILOT_VERSION` field definitions](https://mavlink.io/en/messages/common.html#AUTOPILOT_VERSION)
- [ArduPilot APJ board-ID registry](https://raw.githubusercontent.com/ArduPilot/ardupilot/master/Tools/AP_Bootloader/board_types.txt)
- [ArduPilot `MATEKH743` hardware definition](https://raw.githubusercontent.com/ArduPilot/ardupilot/master/libraries/AP_HAL_ChibiOS/hwdef/MatekH743/hwdef.dat)
- [Matek H743 target and sensor matrix](https://www.mateksys.com/?p=5159)
- [OpenStreetMap KVVKU training-range boundary](https://www.openstreetmap.org/way/1445987734)
- [STMicroelectronics USB VCP example using `0483:5740`](https://www.st.com/resource/en/user_manual/um1021-stm32f105xx-stm32f107xx-stm32f2xx-and-stm32f4xx-usb-onthego-host-and-device-library-stmicroelectronics.pdf)
- [Texas Instruments INA3221 product specification](https://www.ti.com/product/INA3221)
