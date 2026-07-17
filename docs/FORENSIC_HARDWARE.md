# Forensic hardware inventory

This document records hardware attributable to two recovered Gerbera aircraft
storage images and the supplied hardware photographs. It separates direct
operating-system observations, physical markings, vendor identification, and
unsupported software options.

Confidence terms:

- **Confirmed:** direct descriptor, kernel, firmware, or application-log
  evidence identifies the component or active mode.
- **Provisioned:** a retained setting is corroborated by runtime logs and the
  application's own type metadata. This identifies how the deployed software
  treated the aircraft, but is not independent physical inspection of the
  airframe.
- **High:** the observed configuration has a unique or strongly constrained
  vendor mapping, but a compatible clone cannot be excluded.
- **Historical:** a direct observation in deleted factory/test residue that is
  byte-identical in both images. It identifies the device present during that
  earlier boot, but cannot be assigned to aircraft 12702 or 12674.
- **Unknown:** the available evidence does not identify the physical part.

## Shared onboard bill of materials

| Component | Identification | Confidence and evidence |
|---|---|---|
| Computer module | NVIDIA Jetson Orin NX 16 GB, P3767-0000, photographed PN `900-13767-0000-000` and serial `1423223010849`; operating-system report includes `699-13767-0000-300`; Tegra234 SKU 211, eight CPU cores | Confirmed from platform identification in both images and module markings; NVIDIA maps P3767-0000 and `900-13767-0000-000` to Orin NX 16 GB |
| Physical carrier board | LEETOP, marked PN `900-14887-0000`, Rev. 2.1, date code `20230726` | Manufacturer, marked PN, and revision confirmed photographically. Connector layout and release timing match LEETOP A603 V2.1; commercial model is high-confidence because LEETOP's current A603 page instead lists PN `900-44887-0000` |
| Carrier software configuration | NVIDIA P3768-0000 Orin Nano reference-carrier configuration | Confirmed at `etc/nv_boot_control.conf:1` in both images. This is the active board-support configuration, not an identification of the physical carrier manufacturer |
| Cooling | Carrier-mounted active fan and module heat spreader | Confirmed photographically; the exact fan manufacturer and electrical rating are unknown |
| Jetson-module rail monitor | Texas Instruments INA3221 at I²C address `0x40` | Confirmed by the active `ina3221` driver, the `ti,ina3221` device-tree node, and the live hwmon path `/sys/bus/i2c/devices/1-0040`. Channels are `VDD_IN`, `VDD_CPU_GPU_CV`, and `VDD_SOC`, each configured with a 5,000 µΩ shunt; this is the module power monitor, not the aircraft propulsion battery |
| Camera descriptor | USB Video Class 1.10 `HJ-ZOOM10-4K`, VID:PID `32e4:9415` | Confirmed descriptor/interface; `var/log/dmesg:802` for unit 12702 and `var/log/dmesg:824` for unit 12674 |
| Active camera mode | 1,920 × 1,080 pixels, 30 frames/s, MJPEG, hardware decode | Confirmed in application logs at `var/log/syslog:2660523` and `var/log/syslog:2858051`; 4K capture was not observed |
| Wi-Fi/Bluetooth | Intel Dual Band Wireless-AC 8265, PCI ID `8086:24fd`, integrated Intel Bluetooth | Confirmed in both kernel logs. Intel specifies 2×2 Wi-Fi 5 on 2.4/5 GHz and Bluetooth 4.2 |
| Ethernet | Realtek RTL8111/8168/8411-family Gigabit Ethernet, PCI ID `10ec:8168`, `r8168` driver | Controller family confirmed; exact revision unavailable |
| NVMe controller | MAXIO MAP1202, PCI ID `1e4b:1202`, PCIe Gen3 ×2 negotiated | Confirmed in both kernel logs at `var/log/dmesg:437` |
| USB expansion | Four-port high-speed USB 2.0 hub | Confirmed in both deployed boots. Shared deleted factory/test residue identifies an earlier same-topology hub as `05e3:0608`, product `USB2.0 Hub`, revision 60.90; it does not prove the deployed hub's exact controller IC |
| Primary autopilot interface | Tegra UART `ttyTHS1`, persistent name `/dev/ttyAP` | Confirmed interface. `settings.json` assigns this path to the primary MAVLink connection |
| Historical primary autopilot target | ArduPilot/ArduPlane 4.5.7 stable-version class, custom tag `45INAV06`, APJ board ID 1013 (`MATEKH743`), ChibiOS hash `6a85082c` | Historical. An exact deleted EPathfinder telemetry fragment is shared by both APP images. This identifies an STM32H743-class Matek firmware target used in that earlier session, but not the exact H743-WING/WLITE/SLIM/MINI PCB/revision or the board installed in either aircraft |
| Secondary ELink/controller interface | USB CDC ACM, persistent name `/dev/ttyFC`; device rule expects STMicroelectronics VCP VID:PID `0483:5740` | Interface and rule confirmed. Unit 12674 received six ELink arm-test state cycles, establishing a live responding serial peer. Deleted factory/test residue identifies an earlier `0483:5740` peer as `Mordor` / `Flight adapter`, revision 2.00; the deployed PCB and MCU remain unknown |
| Deployed vehicle profile | Internal type `SH10_5`: `TANDEM`, role `ATTACK`, engine class `BEV`, catalog `weight=17.0`, `payload=5.0` | Directly mapped for unit 12702. Unit 12674 repeatedly logs the same enum, including under the same reported `3793M` build, but its executable is absent; the name/metadata transfer is therefore strong rather than byte-for-byte confirmed. Field units and physical manufacturer/model are not encoded |

## Photographic component evidence

| Observation | Result | Boundary |
|---|---|---|
| Module silkscreen and label | `NVIDIA CORP. SANTA CLARA Model: P3767`; PN `900-13767-0000-000`; serial `1423223010849` | Identifies the photographed module; the photograph alone does not map its serial to aircraft 12702 or 12674 |
| Carrier silkscreen | `LEETOP`, PN `900-14887-0000`, Rev. 2.1, `20230726` | Confirms the deployed physical carrier vendor and revision independently of excluded historical boot strings |
| Carrier layout | 260-contact module socket, M.2 Key E 2230, M.2 Key M 2242, active fan, RJ45, stacked USB, Micro-USB, CSI/ZIF and expansion headers, DC input | Matches the A603 V2.1 interface layout; exact A603 catalog SKU remains under the PN discrepancy above |
| Wireless module label | Intel `8265NGW`; WFM `00A5543FF84F` | Confirms the AC 8265 and matches aircraft 12674 Wi-Fi MAC `00:a5:54:3f:f8:4f` |
| Storage label | KingSpec `M.2 NVMe SSD NE 2242`, PCIe Gen3 ×2 | Confirms the photographed storage family; disk evidence supplies the 128 GB model/capacity assignment |

The photograph set does not, by itself, prove that every photographed loose or
removable component was installed in the same aircraft at the same time. Only
the Intel WFM address directly associates a photographed removable component
with aircraft 12674. Other unit assignments below come from the two storage
images.

## Unit-specific identifiers

| Aircraft identifier | Jetson module revision | Wi-Fi MAC | Ethernet MAC | Storage |
|---:|---|---|---|---|
| 12702 | L.2 | `00:a5:54:ad:37:ca` | `48:b0:2d:ea:cc:7e` | KingSpec NE-128 2242, 128 GB; device serial `0013863004134` |
| 12674 | M.1 | `00:a5:54:3f:f8:4f` | `48:b0:2d:ea:93:05` | 128 GB NVMe with MAXIO MAP1202; retail label and device serial unavailable |

Revision evidence is present at `var/log/syslog:2660362` for unit 12702 and
`var/log/syslog:2857873` for unit 12674.

## Shared deleted factory/test descriptors

The two 400 MiB UDA captures are byte-for-byte identical (SHA-256
`8f4866ede3aae2e356fb1ce2d6271b5ee8ab466424eb0006fb662cc1f9b0edaa`).
A deleted gzip stream carved at UDA offset 409,063,424 contains December 2024
factory/test boots under hostname `orindev`. Its Ethernet address and other
context do not match either deployed aircraft. The following are therefore
direct historical observations, not unit-specific serial assignments:

| Device in deleted boot | Full descriptor | Attribution boundary |
|---|---|---|
| ELink-side USB serial peer | VID:PID `0483:5740`, revision 2.00, product `Flight adapter`, manufacturer `Mordor`, serial `180028000747333031313638`; bound by `cdc_acm` as `ttyACM0` | Identifies the self-reported make/product and individual device in the earlier boot. The same VID:PID and hub port role match the later `/dev/ttyFC` design, but the residue cannot prove that either deployed aircraft contained that serial or exact PCB |
| UVC camera A | VID:PID `32e4:9415`, revision 4.19, product `HJ-ZOOM10-4K`, manufacturer `HJ`, serial `70381e10afe157e3`, UVC 1.10 | Confirms the manufacturer's USB string and one historical camera serial; it cannot be assigned to aircraft 12702 or 12674 |
| UVC camera B | Same interface identity, serial `80300e14afe157e3` | A second historical camera, also unassignable to either deployed aircraft |
| Four-port USB hub | VID:PID `05e3:0608`, revision 60.90, product `USB2.0 Hub` | USB-IF assigns VID `05e3` to Genesys Logic. The descriptor identifies the historical hub interface/vendor, not a unique hub-controller chip or the deployed units' hub serial |

This recovered descriptor set improves the interface attribution, especially
for the ELink-side peer, but does not turn the STM32-compatible USB identity
into the main autopilot identity. The primary flight controller remains on the
separate native Tegra UART `/dev/ttyAP`.

## Shared deleted flight-controller telemetry

An unallocated APP extent contains an EPathfinder session from 41.706 through
47.874 seconds after process start. The recoverable fragment begins at physical
byte offset 16,784,564,224, has length 1,040,384 bytes, and has SHA-256
`27891d7b82ac5d1fee71d7a4fce64b1e57230ef4012f8b2753ce34c1ff5e58d5`.
The fragment and its physical offset are identical in the GB1 and GB2 APP
images. That exact cloning makes it strong evidence for a historical
commissioning/test configuration, but prevents assignment to either aircraft.

At physical byte offset 16,784,589,833, the session prints a received MAVLink
`AUTOPILOT_VERSION` structure:

| Field | Recovered value | Interpretation |
|---|---:|---|
| `flight_sw_version` | `67438591` / `0x040507ff` | Semantic version 4.5.7; firmware-type byte `0xff`, the stable/official version class |
| `flight_custom_version` | `45INAV06` | Observed custom firmware tag; no longer merely an executable allowlist string |
| `board_version` | `66387968` / `0x03f50000` | ArduPilot writes `APJ_BOARD_ID << 16`; ID `0x03f5` / 1013 maps to `AP_HW_MATEKH743` |
| `vendor_id`, `product_id` | `4617`, `22336` / `1209:5740` | ArduPilot's generic USB VID/PID values; not a Matek product-model identifier |
| `os_custom_version` | `6a85082c` | ArduPilot/ChibiOS commit prefix; commit title `Remove lwip` |
| `UID` | `0` | Supplies no board serial identity |
| `uid2` | `38 00 41 00 0e 51 33 31 30 39 30 36` (12 logged bytes) | A raw historical MCU identifier survives, but it is cloned and does not encode the Matek retail PCB/revision or either aircraft's installed board |

The official `MATEKH743` definition targets an STM32H743xx MCU. Matek uses the
same target name for multiple H743-WING, WLITE, SLIM, and MINI products, so the
APJ ID is a target-family result rather than an exact retail SKU. The standard
target definitions and Matek's revision matrix span several inertial
combinations—ICM20602, MPU6000, ICM42605, and ICM42688P—and therefore do not
select one installed IMU from this log alone.

The same session materially narrows the attached sensor classes:

| Historical telemetry | What it establishes | Remaining boundary |
|---|---|---|
| Repeated `RAW_IMU` instance 0 with live acceleration, rotation, magnetic-field, and temperature values | A primary inertial stream and a magnetometer stream were active in the test | No `INS_ACC_ID`, `INS_GYR_ID`, or compass device-ID parameter survives; exact chips and board placement remain unknown |
| `compass status changed: true` and nonzero magnetic values | A compass was accepted as healthy | Standard Matek H743 target boards have no built-in compass, making an external compass the likely standard-target configuration; its receiver/module is unidentified |
| Repeated `SCALED_PRESSURE` with about 993.3 hPa absolute pressure and near-zero differential pressure | An absolute-pressure/barometer channel and a differential-pressure/airspeed channel were active | The standard target probes DPS310, while documented board variants/custom firmware permit alternatives; neither the barometer nor differential-pressure sensor model is proved |
| Flight-controller `RAW_GPS` repeatedly reports fix type 0 and zero satellites | No working GNSS solution reached the autopilot input during this short test | Absence of a fix does not prove absence of a receiver |
| Concurrent ELink `GLOBAL_POSITION` reports status 3 and 26 satellites with changing position/velocity | A separate ELink-side GNSS solution was live | Receiver, antenna, and whether this was the deployed aircraft's GNSS remain unknown |
| MAVLink system-status battery voltage about 24,807 mV; ELink voltage about 24,659 mV | A roughly 24.7–24.8 V electrical source was reported during the historical test | Compatible with a six-series pack, but chemistry, series count, capacity, BMS, regulator chain, and propulsion connection are not proved |

### ELink GNSS geolocation and session state

The surviving ELink trace is a stationary position/velocity/time session rather
than evidence of a flight. Sixty-two `GLOBAL_POSITION` reports cover 6.105
seconds at approximately 10 Hz. Their mean position is
`55.692162906, 49.250045682`; the complete horizontal envelope is only 0.111 m
north-to-south by 0.025 m east-to-west. Reported ground speed averages 0.046
m/s and never exceeds 0.127 m/s, while the altitude field ranges from 11,541
to 11,551. The original application multiplies that field by ten before
storing it in its millimetre coordinate representation, giving approximately
115.41–115.51 m.

The receiver-side `timeWeekMs` advances from 295,424,650 to 295,430,750 while
`localMs` advances from 42,600 to 48,700. Position, velocity, course, altitude,
and satellite count also change during the interval; only five consecutive
reports repeat an identical receiver epoch. This behavior strongly favors an
active stationary GNSS feed over a cached last-known coordinate. A deliberate
recorded or synthetic replay cannot be excluded from storage evidence alone,
but the fragment contains no HIL, simulation, replay, or GPS-injection marker.
The GPS time-of-week corresponds to Wednesday 10:03:44.650 through
10:03:50.750 under the conventional GPS-week origin. No week number survives,
so it supplies no calendar date.

Independent state evidence reinforces the stationary interpretation:

| Concurrent evidence | Observation | Interpretation |
|---|---|---|
| Flight-controller `RAW_GPS` | 234 reports have fix type 0, zero satellites, and zero position | The live ELink solution was a separate navigation path, not the autopilot's own GPS input |
| Vehicle state | One disarmed-state report; throttle is zero in all 118 surviving `VFR_HUD` reports | No propulsion event is present in the fragment |
| Relative altitude and range | `VFR_HUD` altitude remains between -0.16 and 0.35 m; all 62 ELink distance-sensor altitude reports are zero | The test article remained at its initialized ground/reference level |
| IMU and compass | Acceleration remains close to one gravity, gyro values remain close to zero, and magnetic values are stable | Consistent with a powered stationary article |
| Air data | Differential pressure varies near zero while reported airspeed varies from 0.48 to 4.40 m/s; reported `VFR_HUD` groundspeed closely follows airspeed despite the static GNSS fix | Airflow/pitot activity is not physical translation of the aircraft |

The field combination—fix status, satellite count, `1e-7`-degree latitude and
longitude, altitude, N/E/D velocity, ground speed, course, and GPS
time-of-week—is structurally close to a UBX `NAV-PVT` solution. In UBX,
fix-type 3 denotes a 3D fix and equivalent coordinate and N/E/D kinematic
quantities are exposed with defined integer scales.
That comparison supports a valid 3D-like, likely multi-constellation PVT feed,
but it does **not** identify a u-blox receiver: ELink firmware could normalize
another receiver's output into the same generic fields. The proprietary ELink
status enum and accuracy/differential-fix flags were not recovered, so status 3
must not be promoted to an RTK claim. The observed 25–27 satellites and stable
solution establish a functioning historical antenna path, not its antenna
type, connector, placement, or installation in either acquired aircraft.

The recovered coordinate shown in [the overview](../ref/shot1.png) and
[the enlarged satellite view](../ref/shot2.png) falls on an isolated
rectangular structure in heavily tracked terrain near Usady. The repository
copies are anonymized presentation derivatives: the geocoder form and duplicate
coordinate fields were cropped away, and all ancillary PNG metadata was
discarded. `shot1.png` is the exact source-pixel rectangle `(916,0)` through
`(2182,948)` and has SHA-256
`1255e3785fc9cfa6d1dc04f8949b9bfeb3a1e306d0ecaa568a09fbb2a66b9526`;
`shot2.png` is `(1100,0)` through `(2200,800)` and has SHA-256
`a91010bf0a16892e2ab6ee4d161bbc3e5982f667cc256ca6781395fbd238aa9e`.
No scaling, interpolation, blurring, content-aware editing, or generative image
processing was applied.

A point-in-polygon test against the current 71-node OpenStreetMap feature named
“Полигон Казанского высшего танкового командного училища” places the point
approximately 565 m inside the mapped KVVKU tank-school range/danger-area
boundary. It is approximately 503 m east of the separately mapped
“Учебные стенды КВВКУ” training-stands polygon. OpenStreetMap records that the
range boundary was added from EGRN/cadastral boundaries and Wikimapia; it is
useful corroboration, not an authoritative ownership determination.

Two independent sources support the broader site attribution without locating
this exact structure. A Republic of Tatarstan planning document, citing the
Russian Ministry of Defence, lists Military Town 34 at Usady. A 2017 Kazan
Federal University paper documents the KVVKU tankodrome in Kazan's
Privolzhsky District and UAV/GNSS positioning work there. Converting the
paper's first published UTM39N control point places that separate experiment
about 3.76 km northwest of the recovered coordinate; it corroborates the
facility, not a relationship between that DJI experiment and EPathfinder.

Public 30–90 m terrain models place the surrounding surface at approximately
103–105 m, while ELink reports about 115.5 m in the application's altitude
representation. Combined with the satellite marker falling on a structure,
this is compatible with an antenna/test article on a roof or tower roughly
ten metres above surrounding terrain. Terrain-model resolution, vertical
datum, absolute GNSS error, and unknown antenna height make that a low-confidence
interpretation; the structure cannot be identified as a tower, control post,
launcher, or any other specific installation from these images.

Taken together, the strongest bounded interpretation is a powered, stationary
commissioning or systems-integration session at a structure within the mapped
KVVKU training range. Because the entire deleted fragment is byte-identical at
the same physical offset in both APP images, it more likely records the history
of a source/master filesystem or common test article than two visits by the
acquired aircraft. It does not establish a launch, recovery, target, operator,
developer, or manufacturing location for either aircraft.

Recovered parameter reads (`COMPASS_CAL_FIT`, `COMPASS_AUTO_ROT`,
`COMPASS_AUTODEC`, `COMPASS_DEC`, and `ARSPD_RATIO`) corroborate configured
compass and airspeed paths but contain no device IDs. Searches of all
non-sparse ext4 blocks marked unallocated found no `INS_ACC_ID`, `INS_GYR_ID`,
`BARO1_DEVID`, or `COMPASS_DEV_ID` record and no checksum-valid raw MAVLink
`AUTOPILOT_VERSION` frame. The recovered version structure exists as
EPathfinder's formatted text log.

The fragment also captures `v4l2-ctl --list-ctrls` output from `/dev/video0`.
The historical UVC interface exposes absolute zoom 0–736 (read at 155 and then
commanded to 736), absolute focus 0–289 with continuous autofocus enabled,
exposure time 1–2500 with automatic exposure, and automatic white balance.
This confirms controllable focus and zoom behavior at the UVC interface. It
does not identify the sensor die, lens model, focal lengths, optical zoom ratio,
or mechanical assembly. Concurrent MAVLink component 154 traffic carries
`GIMBAL_DEVICE_ATTITUDE_STATUS` for device IDs 1, 2, and 3, but no vendor/model
or device-information response survives, so those numeric IDs are not mapped
to physical gimbal products.

## Storage identification

The unit-12702 device reports `NE-128 2242`. KingSpec's official NE-series
specification defines an M.2 2242 NVMe Rev. 1.3 device using PCIe Gen3 ×2 and
includes a 128 GB capacity. This makes the KingSpec identification strong. Unit
12674 exposes the same controller family and capacity but lacks a retained
retail model string or physical-label record; it is not assigned the KingSpec
brand in this document.

## Imaging interpretation

`HJ-ZOOM10-4K` is the device-reported USB product string, not a verified sensor
or optical assembly part number. USB-IF assigns VID `32e4` (decimal 13028) to
Ailipu Technology Co., Ltd.; this identifies the registered USB-interface
vendor, not necessarily the complete optical-module OEM. Deleted factory/test
logs add manufacturer string `HJ`, USB device revision 4.19, and two historical
serials, but the byte-identical residue cannot assign either serial to a
deployed aircraft. The observed deployed stream was 1080p30 MJPEG.

The deployed setting has an empty IP-camera address and selects `/dev/video0`.
The recovered executable consequently constructs its internal `CameraT205`
USB/V4L2 backend for the observed UVC camera. That backend contains a 21-point
zoom calibration and sends V4L2 `zoom_absolute` controls. Its effective **full
field-of-view model** runs from 60.0° horizontal × 32.3° vertical at 0% to
9.0° × 5.0° at 100%. The horizontal and vertical angular spans narrow by
about 6.67× and 6.46× respectively. These values are used as full spans: the
projection code multiplies them by normalized coordinates from -0.5 to +0.5.
They constrain the deployed software's calibrated zoom behavior, but do not
prove focal lengths, optical construction, or a literal 10× optical ratio.
`CameraT205` is an internal class name, not a recovered camera part number.

The shared deleted test session directly enumerates the camera's V4L2 controls:
absolute zoom 0–736, absolute focus 0–289, continuous autofocus, automatic
exposure, and automatic white balance. It reads zoom values 0, 155, and 736 and
issues a command for 736. These runtime observations prove a controllable
focus/zoom interface; they still do not supply a sensor or lens identifier.

The separate retained 9 × 9 JSON table spans approximately 120.02° horizontal
× 71.30° vertical on its central axes. Code tracing shows that table is used
by `EVision::pointAngles` only when no camera object exists. The deployed USB
path creates the `CameraT205` object, so the wider JSON grid is a retained
fallback calibration, not the operational field-of-view model for these
starts. No surviving artifact ties the interface to a particular Ailipu sensor
or lens assembly.

The application also contains a separate SIYI ZR10 network/serial backend. The
ZR10 class proves software compatibility only and is not evidence that a ZR10
was physically installed.

## Vehicle, propulsion, power, and terminal profile

Both deployed applications selected `plane_ver=9`. Unit 12702 records that
value in its retained setting and three starts. Unit 12674 records it in three
older `3793M` starts and eight later `3815M` starts. Reverse engineering the
unstripped unit-12702 executable, which also reports `3793M` (SHA-256
`f656cbf1bc7f8ee7d1d07cf8471f3a9733155c14f437d7aa432d9706aa3a6c2c`)
maps enum 9 to `SH10_5`. The corresponding six-field vehicle record is
`[9, 1, 0, 0, 17.0, 5.0]`; the executable's serializers and string converters
resolve those fields as:

| Field | Recovered value | Evidentiary boundary |
|---|---:|---|
| Internal name | `SH10_5` | Exact application profile name; no defensible public commercial-model match was found |
| Airframe type | `TANDEM` | Software airframe/layout classification; dimensions, structure, and materials remain unknown |
| Role | `ATTACK` | Software mission-role classification; not evidence of an installed terminal payload |
| Engine class | `BEV` | Battery-electric class, distinct from sibling `GAS` and `TJE` classes; motor, propeller, and controller remain unknown |
| Weight | `17.0` | Catalog value; the executable does not encode a unit |
| Payload | `5.0` | Catalog capacity/value; it is not evidence of a payload of that mass, much less a warhead |

`GERBERA` is a separate application enum (99); neither recovered deployment
selected it. Gerbera attribution therefore remains case context, while
`SH10_5` is directly established for unit 12702. It is a strong transfer to
unit 12674 because that unit logged enum 9 under the matching reported build
before retaining enum 9 after its update; the unit-12674 executable itself did
not survive for byte-level comparison.

The retained throttle values (`1500` minimum, `1650` base, `1900` maximum) are
consistent with an RC/PWM-style actuator command. The version-9 software also
enables a reversible-throttle control path. These facts constrain the control
interface but do not identify a motor, propeller, ESC, motor count, or prove
that the physical ESC implemented reverse operation.

Battery voltage is carried over ELink and low-battery logic compares its raw
value to `18200`. Millivolts, and hence approximately 18.2 V, is a plausible
interpretation, but the executable does not state the unit or whether this is
the propulsion or avionics supply. The shared deleted session independently
reports MAVLink battery voltage near 24,807 mV and ELink voltage near 24,659 mV.
Those values support the millivolt interpretation and are compatible with, but
do not prove, a roughly six-series lithium pack. They may also be test/replay
telemetry rather than a trace from either acquired aircraft. Chemistry, series
count, capacity, BMS, power-distribution board, converters, connectors, and
manufacturers remain unknown.

The Jetson module separately exposes a Texas Instruments INA3221 three-channel
current/bus-voltage monitor over I²C at address `0x40`. The recovered device
tree names its channels `VDD_IN` (total module power), `VDD_CPU_GPU_CV`, and
`VDD_SOC`, and configures each for a 5,000 µΩ (5 mΩ) shunt. NVIDIA's Orin
NX documentation independently identifies this as a monitor integrated on the
module. No recovered measurement or wiring map links it to the propulsion pack,
so it does not identify the aircraft battery, BMS, regulator chain, or
power-distribution hardware.

Unit 12674 records six incoming `ELINK_ARM_TEST` on/off state cycles. The
message handler logs only received state transitions and acknowledges an
enabled state, so this is evidence of a live external ELink arm-test channel,
not merely dormant code. The production rule identifies the peer through the
STM32 USB VCP identity `0483:5740`; ST's documentation shows that this VID:PID
is a reusable virtual-COM example across STM32 families. Deleted factory/test
residue records an earlier matching peer's own strings as manufacturer `Mordor`
and product `Flight adapter`, USB revision 2.00. That is a meaningful
self-description, but it still does not identify the deployed PCB or MCU model,
and the historical serial cannot be assigned to either aircraft. The evidence
does **not** establish a fuze board, initiator, explosive attachment, or warhead.
No runtime arm or self-destruction event survives, and `secret_boom_enabled` is
zero in the retained unit-12702 settings.

The five discrete settings are also not evidence of attached terminal
hardware. They are green line 11, red line 12, arm pull-up line 33, execute
pull-down line 29, and execute pull-up line 31. In the original unit-12702
executable, the `GPIOController` constructor uses only its first two integer
arguments and ignores the final three; `VehicleLed::init` and
`VehicleLed::setLedEnabled` are immediate returns. The four arm/execute-facing
methods `GPIOController::armTestOn`, `armTestOff`, `armOn`, and
`selfDestruction` write only Qt debug messages and return; the binary has no
GPIO library dependency or sysfs/gpiod access. Thus the line numbers 11, 12,
29, 31, and 33 are inert configuration in that recovered build. Comparative
disassembly of the historical 3613 executable yields the same immediate-return
LED methods, ignored final three constructor arguments, and debug-only
arm/execute methods. The implementation is therefore stable across those two
recovered versions, but unit 12674's 3815M executable is absent and cannot be
proven identical. None of this excludes terminal circuitry downstream of the
live ELink peer; it means the storage evidence does not identify or directly
drive such a circuit through these configured Jetson lines.

### Forensic source locations

- Unit 12702 profile setting: `home/jetson/EPathfinder_3794/settings.json:73`;
  runtime confirmations: `home/videoview/EPathfinder_3794_logs/_log_.log:3,6,10`.
- Unit 12674 runtime confirmations:
  `home/videoview/EPathfinder_3815_logs/_log_:3,6,11,28,32,38,42,46`.
- Unit 12674's earlier matching-build confirmations:
  `home/videoview/EPathfinder_3793_logs/_log_:20,24,27`.
- Type-name mapping: `vehicleEPType2String` at executable VA `0xe6488`, with
  `SH10_5` at file/VA `0x2458c0`. Vehicle record construction is in
  `EPathfinder::vehicleTypes` at VA `0xfaaa0`, specifically
  `0xfab08–0xfab30`; field serialization is in
  `VehicleController::getVehicleTypesMessage` at VA `0xb6138`.
- ELink arm-test runtime states:
  `home/videoview/EPathfinder_3815_logs/_log_:12–23`; incoming message handling
  is in `ELinkArmHandler::processMessage` at VA `0x31178`.
- Unit-12702 GPIO/LED behavior is in the original executable at
  `VehicleLed::init` VA `0x1587d0`, `VehicleLed::setLedEnabled` VA `0x1587d8`,
  the `GPIOController` constructor at VA `0x1589a0`,
  `GPIOController::armTestOn` VA `0x158d10`, `armTestOff` VA `0x158e38`,
  `armOn` VA `0x158f60`, and `selfDestruction` VA `0x159088`.
- Equivalent historical-3613 routines are at VAs `0x14eb98`, `0x14eba0`,
  `0x14ed68`, `0x14f0d8`, `0x14f200`, `0x14f328`, and `0x14f450` respectively.
- ELink USB enumeration: `var/log/dmesg:699` (unit 12702) and
  `var/log/dmesg:692` (unit 12674); persistent-name rule:
  `etc/udev/rules.d/12-flycontrol-link.rules:2`.
- Deleted full ELink descriptor and binding:
  `img/UDA/recovery/validated_carves/gzip_003_409063424.decompressed:242017–242021,242194`
  and duplicate boot lines `243348–243352,243520`. Full historical camera
  descriptors are at lines `36–41` and `242527–242532`.
- UVC enumeration: `var/log/dmesg:802` (unit 12702) and
  `var/log/dmesg:824` (unit 12674); operating pipeline:
  `var/log/syslog:2660523` and `var/log/syslog:2858051` respectively.
- Operational USB-camera calibration is reconstructed in
  `src/camera/camera_t205.cpp`; full-span use is visible in
  `src/vision/vision.cpp:567` and `src/vision/thunder_gaze.cpp:218`. The fallback
  grid dimensions are at `vision_calibration.json:2–5`.
- INA3221 device-tree identity, channel labels, and shunt configuration are at
  `img/A_KERNEL-DTB/recovery/strings.first5000.txt:1465–1472` and in the
  corresponding DTB node; active unit-12702
  evidence is at `var/log/syslog:2658222,2660388–2660389`, and active unit-12674
  evidence is at `var/log/syslog:2855766,2857899–2857900`.
- `AUTOPILOT_VERSION` decoding is in `HeartbeatHandler::processMessage` at VA
  `0x16cf58`; custom-tag comparisons occur at `0x16dd70–0x16ddc8`, with
  `45INAV06`/`45INAV07` at file offsets `0x24cbc8`/`0x24cbd8`.
- The shared deleted flight-controller log begins at APP physical byte offset
  16,784,564,224; its formatted `AUTOPILOT_VERSION` record begins at
  16,784,589,833. `tools/scan_ext4_free.py` performs the read-only ext4
  free-block/sparse-extent intersection and pattern/MAVLink-frame scan used to
  locate and independently search this residue.

## Flight-control interpretation

The primary `/dev/ttyAP` path carries the application's MAVLink/ArduPilot-facing
traffic. `/dev/ttyFC` is assigned to the proprietary ELink stack. A shared
deleted session now identifies the historical UART peer's target as
`MATEKH743`, running an ArduPilot/ArduPlane 4.5.7 stable-version build with
custom tag `45INAV06`. It also proves that inertial, magnetic, absolute-pressure,
and differential-pressure data paths were live in that session.

That result is deliberately narrower than an exact PCB identification.
`MATEKH743` is one ArduPilot target shared across multiple Matek H743 products
and revisions, and both disk images contain the same deleted bytes at the same
offset. It therefore cannot distinguish H743-WING from WLITE, SLIM, or MINI,
select an IMU/barometer revision, or establish the board independently in both
airframes. The UID field is zero, and the generic `1209:5740` values do not
close that gap.

The main autopilot is especially not identified by `0483:5740`: its production
path is the separate Tegra UART `/dev/ttyAP`, which has no USB descriptor. The
STM32 VCP path is `/dev/ttyFC` and is assigned to ELink. `Mordor` / `Flight
adapter` are the self-reported strings of a matching historical ELink-side
device, not the make/model of the main autopilot.

## RF and antenna interpretation

The Intel AC 8265 is configured and observed operating as a 2.4 GHz Wi-Fi
access point. Its dual-band 2×2 capability does not prove how many antenna
leads were populated. No artifact establishes antenna count, type, gain,
polarization, connector, cable, placement, or routing for Wi-Fi, GNSS, ELink,
or any other radio. Bluetooth firmware loads, but no paired-device record was
recovered.

## Exclusions

- Earlier `Orin Nano` and `leetop 603` boot entries are factory/test residue
  from cloned storage and are not used as deployment evidence. The LEETOP
  carrier identification above is independent: it comes from the supplied
  carrier photograph. The A603-family inference comes from its physical layout
  and the vendor manual, not from those historical entries.
- A Sony IMX219 device-tree configuration is present, but its I²C probe failed
  with error `-121`; it was not an operating installed camera in the examined
  unit.
- No Quectel cellular modem was detected.
- No installed Livox unit was established. Livox SDK support is an optional
  application capability.
- No installed SIYI ZR10 was established. Its backend is an alternative camera
  capability.
- The separate `mec` vision payload remains LUKS2-encrypted. No passphrase,
  plaintext type-10 unlock request, disk-backed swap, core dump, or usable
  pre-shred extent survives in the images; it therefore supplies no additional
  camera or terminal-hardware identification in this analysis.
- The recovered deployment can now be constrained to the `SH10_5` tandem,
  attack-role, battery-electric software profile and to a responding ELink
  arm-test channel. These do not identify the physical airframe manufacturer,
  construction material, propulsion parts, power parts, terminal payload,
  fuze, initiator, or warhead.
- The configured Jetson arm/execute line numbers do not close that gap: their
  methods are logging-only no-ops in the original recovered unit-12702 build
  and in the historical 3613 executable.
- The historical main flight-controller firmware target is now resolved to
  `MATEKH743`, ArduPilot/ArduPlane 4.5.7, custom tag `45INAV06`. The exact PCB
  SKU/revision and its per-aircraft installation remain unproved. GNSS receiver,
  IMU, compass, barometer, differential-pressure sensor, motor, propeller, ESC,
  battery, power distribution, antennas, camera sensor/lens, and terminal
  hardware make/models remain unknown. The INA3221 resolves only the Jetson
  module's rail-monitor IC; `Mordor` / `Flight adapter` resolves only a
  historical ELink-side USB self-description.

## Primary references

- [NVIDIA module/carrier mapping](https://docs.nvidia.com/jetson/archives/r36.4.3/DeveloperGuide/IN/QuickStart.html#jetson-modules-and-configurations)
- [NVIDIA Orin Nano/NX module part-number table](https://developer.download.nvidia.com/assets/embedded/secure/jetson/orin_nano/docs/Jetson-Orin-Nano-NX-CoV_DA-11856-001v01.pdf)
- [NVIDIA Orin Nano/NX platform power-monitor documentation](https://docs.nvidia.com/jetson/archives/r36.4.4/DeveloperGuide/SD/PlatformPowerAndPerformance/JetsonOrinNanoSeriesJetsonOrinNxSeriesAndJetsonAgxOrinSeries.html)
- [LEETOP A603 product specification](https://www.leetop.top/products/a603-carrier-board-for-orin-nx-orin-nano)
- [LEETOP A603 Carrier Board V2.1 manual](https://files.seeedstudio.com/Leetop_A603_Carrier_Board_V2.1%200228.pdf)
- [KingSpec NE 2242 specification](https://www.kingspec.com/product/m2-nvme-pcie-gen3-ssd-ne-2242mm.html)
- [Intel Wireless-AC 8265 specification](https://www.intel.com/content/www/us/en/products/sku/94150/intel-dual-band-wirelessac-8265/specifications.html)
- [USB-IF assigned USB vendor IDs](https://www.usb.org/sites/default/files/usb_vids_080223.pdf)
- [MAVLink `AUTOPILOT_VERSION` field definitions](https://mavlink.io/en/messages/common.html#AUTOPILOT_VERSION)
- [ArduPilot APJ board-ID registry](https://raw.githubusercontent.com/ArduPilot/ardupilot/master/Tools/AP_Bootloader/board_types.txt)
- [ArduPilot `MATEKH743` hardware definition](https://raw.githubusercontent.com/ArduPilot/ardupilot/master/libraries/AP_HAL_ChibiOS/hwdef/MatekH743/hwdef.dat)
- [ArduPilot version-report construction](https://raw.githubusercontent.com/ArduPilot/ardupilot/master/libraries/GCS_MAVLink/GCS_Common.cpp)
- [Matek H743 target and sensor matrix](https://www.mateksys.com/?p=5159)
- [ArduPilot/ChibiOS commit `6a85082c`](https://github.com/ArduPilot/ChibiOS/commit/6a85082c)
- [u-blox UBX `NAV-PVT` field and fix-type reference](https://content.u-blox.com/sites/default/files/documents/u-blox-F10-TIM-3.01_InterfaceDescription_UBX-23003447.pdf)
- [OpenStreetMap KVVKU training-range boundary](https://www.openstreetmap.org/way/1445987734)
- [OpenStreetMap KVVKU training stands](https://www.openstreetmap.org/way/365349083)
- [Republic of Tatarstan territorial waste-management plan listing Military Town 34 at Usady](https://kt.tatarstan.ru/file/pub/pub_4161353.pdf)
- [Kazan Federal University paper documenting UAV/GNSS work at the KVVKU tankodrome](https://kpfu.ru/portal/docs/F2125721560/159_4_est_8.pdf)
- [OpenTopoData SRTM90 elevation query for the recovered coordinate](https://api.opentopodata.org/v1/srtm90m?locations=55.6921625,49.2500455)
- [OpenTopoData ASTER30 elevation query for the recovered coordinate](https://api.opentopodata.org/v1/aster30m?locations=55.6921625,49.2500455)
- [Open-Elevation lookup for the recovered coordinate](https://api.open-elevation.com/api/v1/lookup?locations=55.6921625,49.2500455)
- [STMicroelectronics USB VCP example using `0483:5740`](https://www.st.com/resource/en/user_manual/um1021-stm32f105xx-stm32f107xx-stm32f2xx-and-stm32f4xx-usb-onthego-host-and-device-library-stmicroelectronics.pdf)
- [Texas Instruments INA3221 product specification](https://www.ti.com/product/INA3221)
