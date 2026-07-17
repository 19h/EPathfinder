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
the propulsion or avionics supply. It is compatible with, but does not prove,
a roughly six-series lithium pack. Chemistry, series count, capacity, BMS,
power-distribution board, converters, connectors, and manufacturers remain
unknown.

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

## Flight-control interpretation

The primary `/dev/ttyAP` path carries the application's MAVLink/ArduPilot-facing
traffic. `/dev/ttyFC` is assigned to the proprietary ELink stack. These names
and protocols identify logical roles, not the attached controller boards.
ArduPilot compatibility does not identify a Pixhawk, Cube, Matek, CUAV, or any
other flight-controller family.

The application parses MAVLink `AUTOPILOT_VERSION`, including board, vendor,
product, UID, firmware-version, and custom-version fields. It treats the
eight-byte custom tags `45INAV06` and `45INAV07` as acceptable. No received
`AUTOPILOT_VERSION` record, actual custom tag, parameter dump, flight log, or
board UID survives in either image. Those strings are therefore a firmware
allowlist/capability, not identification of the installed firmware or flight
controller. MAVLink IMU, GNSS, magnetometer, pressure, and airspeed handlers
similarly establish data paths, not sensor chip models.

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
- The main flight-controller PCB, GNSS receiver, IMU, compass, barometer,
  motor, propeller, ESC, battery, power distribution, antennas, camera
  sensor/lens, and terminal hardware make/models remain unknown. The INA3221
  resolves only the Jetson module's rail-monitor IC; `Mordor` / `Flight
  adapter` resolves only a historical ELink-side USB self-description.

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
- [STMicroelectronics USB VCP example using `0483:5740`](https://www.st.com/resource/en/user_manual/um1021-stm32f105xx-stm32f107xx-stm32f2xx-and-stm32f4xx-usb-onthego-host-and-device-library-stmicroelectronics.pdf)
- [Texas Instruments INA3221 product specification](https://www.ti.com/product/INA3221)
