# Implementation status and evidence register

## Scope and completion boundary

This repository reconstructs the EPathfinder companion-computer application
used in two Gerbera-family aircraft. It covers the onboard Jetson application,
protocol adapters, state model, vision interfaces, and controller API. It does
not contain the flight-controller firmware image, ThunderGaze source, VNav
implementation, radio/modem firmware, camera firmware, propulsion control, or
physical payload electronics. Deleted telemetry identifies a historical
`MATEKH743` firmware target, but does not recover its source/binary or prove an
exact PCB in either aircraft.

All 61 identified application component concepts have compiling source
counterparts. “Implemented” is graded as follows:

- **Strong:** interface, layout, protocol, state transition, constants, or
  mathematics are reproduced and covered by executable tests.
- **Functional:** public behavior and principal state flow are implemented,
  while some internal algorithms remain bounded approximations.
- **Scaffold:** the complete class/interface surface exists, but material
  private behavior is incomplete and no equivalence claim is made.

## Platform identity

The forensic case identifies both aircraft as Gerbera-family unmanned aircraft.
The onboard application itself supports several vehicle roles and does not
self-identify an airframe. The Gerbera attribution is therefore case evidence,
not an inference from a class name.

Both storage images confirm the shared Jetson Orin NX 16 GB, UVC camera,
Intel/Realtek networking, MAXIO-backed NVMe, `/dev/ttyAP`, and `/dev/ttyFC`
architecture documented in [the forensic hardware inventory](docs/FORENSIC_HARDWARE.md).

## Additional forensic recovery

Two previously unallocated/deleted artifacts materially extend the evidence
base without changing the deployed-application reconstruction:

- An intact 12 MiB GNU tar stream was recovered from the GB1 APP image at raw
  offset 15,439,233,024
  (`img/APP/recovery/mec_leads/tar_probe/from_15439233024.bin`, SHA-256
  `071e73a8fdf5297595650f5c7cc9460606d8945aef36595544df37a698247927`).
  It contains the complete historical `EPathfinder_3613` package: a stripped
  AArch64 executable, settings, version/release files, and vision calibration,
  all dated 2023-05-11. The executable SHA-256 is
  `682fa57232ee939e18c3e98de3ad443d11560c5ba156512207bbcfbbf6df4531`.
  Its settings independently preserve the same `/dev/ttyAP` at 460,800 bit/s,
  `/dev/ttyFC` at 57,600 bit/s, and `/dev/video0` architecture. It is historical
  deployment residue and does not identify a current sensor, controller PCB,
  propulsion part, or payload. Comparative disassembly also shows the same
  GPIO behavior as the current unit-12702 executable: LED writes return
  immediately, the three arm/execute constructor arguments are ignored, and
  the four arm/execute-facing methods only log.
- The GB1 and GB2 400 MiB UDA captures are byte-identical (SHA-256
  `8f4866ede3aae2e356fb1ce2d6271b5ee8ab466424eb0006fb662cc1f9b0edaa`).
  A deleted gzip stream carved at offset 409,063,424 contains full factory/test
  USB descriptors absent from the later deployed logs. It records a
  `0483:5740` revision-2.00 device as manufacturer `Mordor`, product `Flight
  adapter`, serial `180028000747333031313638`; two `32e4:9415` revision-4.19
  cameras as manufacturer `HJ`, product `HJ-ZOOM10-4K`, serials
  `70381e10afe157e3` and `80300e14afe157e3`; and a `05e3:0608` revision-60.90
  four-port `USB2.0 Hub`. Because the residue is identical in both images and
  its boot identity differs from both aircraft, none of those historical
  serials is assigned to unit 12702 or 12674.
- A second shared deleted artifact was recovered from unallocated APP blocks.
  The 1,040,384-byte EPathfinder log fragment begins at physical byte offset
  16,784,564,224 and has SHA-256
  `27891d7b82ac5d1fee71d7a4fce64b1e57230ef4012f8b2753ce34c1ff5e58d5`.
  It is byte-for-byte identical at the same offset in both APP images. Its
  received MAVLink `AUTOPILOT_VERSION` record reports ArduPilot/ArduPlane 4.5.7,
  stable-version class, custom tag `45INAV06`, APJ board ID 1013
  (`MATEKH743`), generic ArduPilot values `1209:5740`, and ChibiOS hash
  `6a85082c`. Live inertial, magnetic, absolute-pressure,
  differential-pressure, battery, and separate ELink GNSS telemetry also
  survives. The session also enumerates UVC absolute zoom 0–736, absolute focus
  0–289, continuous autofocus, automatic exposure, and automatic white balance.
  The exact cloning makes these historical test facts, not two independent
  per-aircraft observations.
- The ELink GNSS trace contains 62 fixes over 6.105 seconds at approximately
  10 Hz. It centers on `55.692162906, 49.250045682`, varies by only 0.111 m
  north-to-south and 0.025 m east-to-west, and reports mean ground speed 0.046
  m/s. The controller is disarmed; throttle is zero in all surviving
  `VFR_HUD` records; relative altitude remains close to zero; and all ELink
  distance-sensor altitude reports are zero. This is a stationary test trace,
  not a flight. The coordinate is approximately 565 m inside the current
  OpenStreetMap boundary for the Kazan Higher Tank Command School training
  range near Usady. Continuous counters and changing position/velocity data
  favor a live GNSS feed, but replay remains possible. Because the fragment is
  cloned, this is historical test/master-image provenance and not proof that
  either acquired aircraft visited the location. See
  [the bounded geolocation analysis](docs/FORENSIC_HARDWARE.md#elink-gnss-geolocation-and-session-state).

The deleted full-descriptor log is retained at
`img/UDA/recovery/validated_carves/gzip_003_409063424.decompressed`
(SHA-256
`468675ba46446e187b182f9c93eb7c9e1706fa38a8969438ed0d3cbb3d47f98b`).
Descriptor lines are indexed in
[the hardware inventory](docs/FORENSIC_HARDWARE.md#forensic-source-locations).
The read-only unallocated-block scanner used for the APP recovery is retained
as `tools/scan_ext4_free.py`; it intersects ext4 free-block bitmaps with
non-sparse host extents, reports selected text patterns, and validates candidate
raw MAVLink `AUTOPILOT_VERSION` frames by CRC.

The 512 MiB ext4 journals were also dumped and searched independently. They
retain directory metadata for historical `EPathfinder_3378`, `_3579`, `_3613`,
and the deleted GB2 `_3815` package, plus an older 114-byte `cam.cfg`. Journal
copies show the `_3815` package's expected calibration, settings, release,
version, and executable entries, followed by filename-obscuring zero renames.
The corresponding inode generations/extents have been recycled: `cam.cfg`'s
former block and the `_3613_RELEASE.tgz` former 1,359,989-byte extent are now
all zero, and a whole-partition path scan found no second package archive. This
closes those journal leads without yielding another sensor, controller, camera,
propulsion, or payload identifier. The separate intact `_3613` TAR above is the
only complete historical application package recovered from unallocated data.

The encrypted `mec` payload was treated as a separate recovery boundary. Both
images contain 62,914,560,000-byte LUKS2 containers with byte-identical 16 MiB
header/keyslot regions and matching sampled ciphertext; the keyslot uses
Argon2i with about 938 MiB per guess. Reverse engineering shows that an
Honor/Huawei controller sent the literal passphrase transiently in plaintext
JSON `messageType` 10 over TCP 8052, after which local cleanup shredded the
unlock scripts and ran `fstrim`. No type-10 payload, passphrase, disk-backed
swap, hibernation image, core dump, or usable pre-shred OpenPGP extent survives
locally. Targeted candidates were unsuccessful, so blind local guessing is not
a proportionate path to additional hardware evidence. The full basis is in
`GERBERA_FORENSIC_REPORT.md`; a controller acquisition or a
preserved unlock packet capture would be required to cross this boundary.

## Component coverage

| Constituent | Implemented result | Grade / boundary |
|---|---|---|
| Core math | CRC, angle arithmetic, ray/plane intersection, Kalman state, pixel mapping, calibration matrices, and transform chain | Strong |
| Camera and transforms | Base camera state, USB/V4L2 backend, T205 calibration profile, ZR10 network/serial backend, gimbal and zoom histories | Strong for interfaces, packets, and tested state; installed camera internals remain unknown |
| Abstract links | Serial, UDP, and TCP transport contracts and buffering | Strong |
| MAVLink stack | Communicator plus heartbeat, wind, RC, time, attitude, GNSS, log, control, and mission handlers | Strong for protocol-valid traffic |
| ELink stack | Communicator and arm, attitude, flower, gimbal, interceptor, movement, position, status, and telemetry handlers | Strong for packet/state behavior |
| VNav | UDP navigation, target/AHRS traffic, plan checks, and 512-sample yaw correlation | Strong for protocol-valid traffic |
| Livox/lidar | Vendored Livox SDK 2.3.0, device adapter, and point clustering | Functional optional backend; installed hardware not established |
| Support | Logging, remote controller, interception client, and LED/GPIO state | Strong protocols; historical 3613 and original unit-12702 3793M binaries both make LED methods immediate returns and arm/execute methods logging-only no-ops |
| EScout | Position/attitude histories, timing transforms, GNSS/inertial/ELink state, wind, range, road offsets, heading correction, and odometry surfaces | Functional; large sensor-fusion paths are not statement-equivalent |
| Road records/map | Record layouts, graph structures, geodesic/local projections | Strong |
| Road analyzers/runner | Analyzer hierarchy, topology element production, nearest-road projection, and offset state | Functional; large matching/averaging heuristics remain bounded under A14 |
| ThunderGaze | Base and versions 1–5, message extents, settings/signals, point reduction, and angle conversion | Functional base; proprietary version payloads remain under A13 |
| EVision | UDP ingress, camera selection, 81-point calibration, interpolation, ignored regions, timing, and angular projection | Strong except for ThunderGaze payload dependency |
| EPathfinder | Complete Qt interface, sparse mode table, launch defaults, RC indexing, vehicle-role mapping, airspeed conversion, and state/forwarding facade | Scaffold for autonomous flight laws under A15 |
| ClientController | TCP service, cookie routing, concatenated JSON parsing, command dispatch, and status transport | Strong for the tested TCP path |
| VehicleController | Status/parameter/node JSON, lifecycle state, client answers, and orchestration facade | Functional/scaffold; production peripheral composition and host-management actions remain incomplete |
| Entrypoint | Version naming, logging, controller construction, request/answer graph, status graph, and Qt event loop | Strong for current reconstructed graph |

## Current verification

```text
Clean configure and compile                         PASS
Core/integration tests                              PASS
Qt interface comparison: 57 classes / 819 methods  PASS
Local TCP status request on port 8052               PASS
Vendored MAVLink definition-table verification     PASS
```

The tests exercise math, ABI-sensitive records, serial/UDP/TCP transports,
MAVLink traffic, mission/log/control transactions, ELink, VNav, Livox adapters,
cameras, scout state, vision calibration, road analysis, EPathfinder state,
controller dispatch, and Qt interface presence.

## Assumption register

### A1 — Native Qt 6 versus deployment Qt 5

Assumption: native Qt 6 tests establish source semantics but do not establish
the deployment Qt 5 cross-module ABI. Dependent results: container-bearing
layouts and queued signal behavior on the development host.

Probe: build with GCC 9.4.0, AArch64 Ubuntu 20.04 and
the deployment Qt 5; assert all recorded offsets and sizes. A divergence
falsifies ABI equivalence for that type, not its tested source semantics.

### A2 — Ray/plane semantic parameter names

Assumption: the four vectors are plane point, plane normal, ray origin, and ray
direction. Dependent result: source names only; algebra and operand order are
directly tested.

Probe: enumerate all caller coordinate frames or compare a non-axis-aligned
reference trace. A different vector role falsifies the names.

### A3 — External zoom maps

Assumption: `/zoom_00.dat` through `/zoom_20.dat` and `/ok` are deployment data
not present here. Dependent result: the 82,944,180-byte map storage is zero
until files are loaded, so arbitrary-pixel remapping is not equivalent.

Probe: locate the deployment files and validate their sizes, sentinel, 3 × 3
matrix prefix, and two 1,920 × 1,080 float32 planes.

### A4 — Erased private identifiers and enum labels

Assumption: descriptive private names and enum labels replace unavailable
source identifiers. Dependent result: spelling only; observed values, public
types, and tested behavior are retained.

Probe: compare original headers or preprocessed sources. Different spelling
falsifies labels; different values require contradictory runtime evidence.

### A5 — Malformed fixed-width MAVLink strings

Assumption: protocol-valid NUL-terminated or width-bounded fields define
equivalence. The reconstruction bounds `PARAM_VALUE.param_id` and
`STATUSTEXT.text`.

Probe: replay full-width non-NUL fields against a reference aircraft image under
memory instrumentation; compare output and fault behavior.

### A6 — Optimized implementation ambiguities

Assumption: low-level data flow, object construction, and tested behavior
override misleading high-level type inferences caused by optimization and Qt
implicit sharing. Dependent results: corrected internal types.

Probe: cross-compile candidate functions with GCC 9.4.0 `-O2` for AArch64 and
compare normalized data flow. A conflicting execution trace falsifies a
candidate type.

### A7 — Livox source replacement

Assumption: the vendored Livox SDK 2.3.0 tree is the source-level replacement
for the linked Livox subsystem.

Probe: cross-build it with the deployment toolchain and compare every observable
SDK call, device state transition, callback, and point record.

### A8 — MAVLink log identifier domain

Assumption: log IDs are positive, consecutive, and one-based and packets are
protocol-valid. The implementation retains the observed completion-index and
90-byte append behavior.

Probe: replay zero, skipped, terminal, and short log packets; compare file
bytes, item state, signals, and faults.

### A9 — Control parameter identifier length

Assumption: UTF-8 parameter names are at most 15 bytes. The reconstruction
bounds the 16-byte MAVLink field.

Probe: exercise 15-byte, 16-byte, and multibyte-overlength identifiers on the
deployment libc and record send, truncate, or abort behavior.

### A10 — Plan/container ABI

Assumption: Qt 5 assertions represent deployment layouts; Qt 6 tests do not
establish cross-module ABI for `PlanTargetImage`, `Plan`, `RoadElement`, or
`CameraParams`.

Probe: compile on the original Qt 5/AArch64 environment, assert each offset and
size, and copy populated records through every public boundary.

### A11 — Plan road topology

Assumption: reconstructed point/link topology does not reproduce every branch
of `PlanHandler::roadMapFromPlan`. Dependent results: neighbour classification
and grouping; JSON and mission transport are independent.

Probe: differential-test linear, branching, cyclic, disconnected, railway, and
duplicate-number plans and serialize every `RoadMap` field.

### A12 — Malformed VNav datagrams

Assumption: protocol-valid datagrams define equivalence. The reconstruction
checks every payload extent.

Probe: replay each identifier at every length around its valid payload boundary
under memory instrumentation.

### A13 — ThunderGaze version payloads

Assumption: fixed message extents identify packet boundaries, but
`parseMessage`/`parseDTP` for versions 1–5 do not fully decode proprietary
`FastData` fields. Dependent results: live target and road detections from UDP;
calibration, settings, camera forwarding, and injected tests are independent.

Probe: capture one valid datagram per version with labeled detections, trace
field reads and emitted `Target`/`RoadPointAngle` values, and add the captures
as differential fixtures.

### A14 — Road matching heuristics

Assumption: graph-element detection, nearest-segment projection, and bounded
offset averaging reproduce public invariants but not the full optimized matcher
and runner ingest behavior. Dependent results: ambiguous junction selection,
dispersion filtering, and multi-frame confidence.

Probe: replay timestamped road polylines over branching maps and compare every
candidate, confidence, accepted item, and timestamp.

### A15 — EPathfinder control laws

Assumption: the Qt/control boundary and simple functions are correct, while
autonomous target, launch, manual/copter, road, and timer-dispatch laws are not
statement-equivalent. Dependent results: physical control outputs and autonomous
mode transitions.

Probe: run a deterministic telemetry simulator against the reference system;
record every input and output event at 1 ms resolution and compare each mode.

### A16 — Vehicle host-management actions

Assumption: JSON state semantics can be represented without executing
deployment-specific reboot, poweroff, software replacement, screenshot, NTP,
or map-check processes. Dependent results: host filesystem and operating-system
side effects.

Probe: trace the reference service in an isolated deployment image and compare
arguments, environment, file mutations, exit mapping, and returned JSON.

### A17 — Client tablet transport

Assumption: TCP is the primary operational client path. Tablet serial settings
exist but the current controller does not open or transmit on that device.

Probe: attach a pseudo-terminal matching `tablet_com_port`, replay concatenated
JSON bidirectionally, and compare reconnect timing and bytes.

### A18 — Gerbera platform attribution

Assumption: the two images belong to Gerbera-family aircraft, as specified by
the forensic case context. Dependent result: platform naming in this
documentation; the software architecture is independent of that label.
The recovered application separately selects internal profile `SH10_5`; its
catalog also contains an unselected `GERBERA` enum. The application therefore
does not independently prove the case-context attribution.

Probe: correlate storage serials, unit IDs, acquisition chain, airframe
photographs, and physical labels. Contradictory chain-of-custody evidence
falsifies the platform label without changing the software findings.

### A19 — Physical carrier board

Assumption: the photographed LEETOP carrier belongs to the recovered hardware
set, and its commercial family is A603 V2.1. The vendor, marked PN
`900-14887-0000`, Rev. 2.1, and date `20230726` are direct photographic
observations. The A603-family result depends on layout/manual correspondence;
LEETOP's current A603 page lists PN `900-44887-0000`, so the exact catalog SKU
is not asserted as confirmed. P3768-0000 independently identifies the active
software/board-support configuration.

Probe: obtain the reverse-side silkscreen, carrier EEPROM, purchase record, or
vendor confirmation for PN `900-14887-0000`, then compare the complete PCB and
connector map with the A603 V2.1 manual.

### A20 — Camera internals

Assumption: `HJ-ZOOM10-4K` is a USB product descriptor, not a complete camera
part number. USB-IF assigns its VID to Ailipu Technology. The deployed empty
`camera_ip` setting selects the internal `CameraT205` USB/V4L2 backend; its
21-point table models full field of view from 60.0° × 32.3° at 0% to
9.0° × 5.0° at 100%. The retained 120.02° × 71.30° JSON grid is used only
by the no-camera fallback path. Shared deleted factory/test logs add USB
manufacturer string `HJ`, device revision 4.19, and two historical serials,
but cannot assign a serial to either aircraft. A separate shared test session
enumerates absolute zoom 0–736, absolute focus 0–289, continuous autofocus,
automatic exposure, and automatic white balance. Dependent results: sensor,
lens, focal lengths, optical/mechanical construction, and gimbal identity
remain unknown; only the interface identity, control surfaces, calibrated
effective field-of-view range, and 1080p30 MJPEG operation are asserted.

Probe: photograph labels, query full UVC extension controls, inspect USB strings
from the running device, or disassemble the camera module.

### A21 — Flight-controller and ELink PCBs

Assumption: `/dev/ttyAP` and `/dev/ttyFC` identify separate roles. A shared
deleted `/dev/ttyAP` session directly reports ArduPilot/ArduPlane 4.5.7,
custom tag `45INAV06`, and APJ board ID 1013 (`MATEKH743`). This establishes a
historical Matek H743 firmware target, not a particular H743-WING/WLITE/SLIM/MINI
PCB or revision. Both APP images contain the same fragment at the same physical
offset, its UID is zero, and its generic `1209:5740` values do not distinguish
hardware; assigning the result independently to aircraft 12702 and 12674 would
therefore be unsound.

Unit 12674's received ELink arm-test cycles separately prove a live peer on
`/dev/ttyFC`, but not its PCB, MCU, or downstream load. Shared deleted UDA
residue identifies one earlier matching `0483:5740` device's self-reported
strings as `Mordor` / `Flight adapter`, revision 2.00. It is not the main
autopilot. Dependent results: the historical autopilot target family is
identified; exact deployed autopilot PCB, fuze, and deployed ELink-board
assignments are not.

Probe: obtain board photographs/markings, bootloader responses, sensor device-ID
parameters, or independent per-unit telemetry with nonzero board UIDs.

### A22 — Optional hardware installation

Assumption: a compiled backend or retained setting does not prove a peripheral
was installed. Dependent results: ZR10, Livox, radar, remote-controller,
odometry, tablet, backup UART, and cellular hardware are classified as optional
or absent unless independently observed.

Probe: correlate kernel device enumeration, network captures, physical
connectors, process logs, and photographs for each candidate device.

### A23 — Photograph-to-unit association

Assumption: the supplied photographs document components from the recovered
hardware set, but do not prove that every photographed removable component was
installed in one aircraft simultaneously. The Intel 8265NGW WFM address
`00A5543FF84F` directly matches aircraft 12674. Dependent result: the photographs
confirm component models and physical carrier construction; unit-specific
assignments otherwise remain based on storage-image evidence.

Probe: inspect acquisition records and full-assembly photographs, and correlate
the Jetson serial, carrier PN, SSD serial, Wi-Fi address, and airframe identifier
before assigning all photographed parts to one unit.

### A24 — Operational vehicle profile

Assumption: the repeatedly logged `plane_ver=9` reflects the vehicle profile
used by the deployed applications. The recovered executable maps enum 9 to
`SH10_5` and serializes it as `TANDEM`, `ATTACK`, `BEV`, `weight=17.0`, and
`payload=5.0`. Dependent result: tandem layout, attack role, and
battery-electric propulsion are provisioned classifications, not independent
physical measurements. The two numeric fields have no encoded units and do not
prove an installed payload.

The mapping is direct for unit 12702. Unit 12674 logs enum 9 under the same
reported `3793M` build and the later `3815M` build, but its executable is
absent; applying the type record to that unit is a strong cross-version
inference rather than byte-level confirmation.

Probe: correlate a complete-aircraft photograph, data plate, harness and
propulsion inspection, measured mass, and deployment record with the profile.
Contradictory physical evidence would falsify the profile-to-airframe mapping
without changing the recovered software metadata.

## Resolved hardware ambiguities

- Jetson is resolved to Orin NX 16 GB P3767-0000; older Orin Nano strings are
  excluded as factory/test boot residue.
- Physical carrier markings resolve the vendor to LEETOP, PN
  `900-14887-0000`, Rev. 2.1. Its layout supports an A603 V2.1-family
  identification under A19; P3768-0000 remains the software configuration.
- The operating camera is the USB UVC `HJ-ZOOM10-4K`, not the failed IMX219
  device-tree entry and not the optional ZR10 backend.
- Active camera mode is 1080p30 MJPEG; the product string does not prove 4K use.
- USB VID `32e4` resolves to Ailipu Technology, while the sensor/lens remain
  unknown. The selected USB backend resolves the effective full-field model to
  60.0° × 32.3° at 0% through 9.0° × 5.0° at 100%; the wider JSON grid
  is a no-camera fallback. Historical V4L2 enumeration additionally proves
  absolute zoom/focus and autofocus control surfaces, not their optical parts.
- Intel AC 8265 supplies both Wi-Fi and Bluetooth; no Quectel modem was
  detected.
- `/dev/ttyAP` is the Tegra UART used for primary MAVLink;
  `/dev/ttyFC` is the USB CDC ACM path assigned to ELink. Six received arm-test
  cycles establish a responding peer. Historical USB strings resolve one
  factory/test peer to `Mordor` / `Flight adapter`, revision 2.00, but not its
  PCB/MCU or either deployed aircraft's serial.
- A shared deleted `/dev/ttyAP` session resolves the historical primary
  autopilot target to ArduPilot/ArduPlane 4.5.7, custom tag `45INAV06`, APJ ID
  1013 (`MATEKH743`), on an STM32H743-class target. It does not select a Matek
  retail PCB/revision or prove the installed board separately in either unit.
- That session proves live inertial, magnetic, absolute-pressure, and
  differential-pressure streams, plus a separate 26-satellite ELink GNSS
  solution. The exact sensor/receiver chips remain unknown; standard target
  definitions only bound candidate IMUs and make an external compass likely.
- Texas Instruments INA3221 is resolved as the active three-channel I²C power
  monitor on the Jetson module. The device tree identifies `VDD_IN`,
  `VDD_CPU_GPU_CV`, and `VDD_SOC`, each configured for a 5 mΩ shunt. It does
  not resolve the aircraft battery, BMS, regulator chain, or power distribution.
- Both units selected `SH10_5`, whose catalog record is tandem-layout,
  attack-role, and battery-electric. This resolves the provisioned vehicle
  class, not the physical manufacturer or component bill of materials.
- Five configured Jetson discrete-line numbers do not identify terminal
  hardware. Both the historical 3613 and original unit-12702 3793M executables
  make the LED methods immediately return and the arm/execute-facing methods
  only log; unit 12674's later 3815M executable is unavailable for comparison.
  Its live ELink arm-test peer remains separate evidence, but does not identify
  a downstream load.

## Bounded opportunities and risks

- **High impact:** deterministic flight-law telemetry would close A15.
- **High impact:** labeled ThunderGaze datagrams would close A13.
- **High impact:** the external zoom-map dataset would enable the full remapping
  path under A3.
- **High impact:** unauthenticated client/VNav/ELink paths expose command and
  state surfaces; transport authentication and sender binding are not observed.
- **Medium impact:** vendor confirmation, exact controller/camera inspection,
  and photograph chain-of-custody correlation would close A19–A21 and A23.
- **Medium impact:** road and plan differential corpora would constrain A11 and
  A14.
- **Medium impact:** isolated host-operation traces would close A16.
- **Low impact:** native Qt 6 testing validates source behavior; deployment ABI
  remains a Qt 5/AArch64 property under A1/A10.

## Quality gates

- **QG1 pass:** no normative conclusion is required.
- **QG2 pass:** A1–A24 state assumptions, dependent results, and falsification
  probes.
- **QG3 pass for the bounded deliverable:** all application component concepts,
  controller graph, build target, dependency integrations, tests, hardware
  inventory, and wiring documentation are present; incomplete behavior is
  explicitly graded.
- **QG4 pass:** coordinates, lengths, rates, storage sizes, ports, and timing
  retain explicit units and reproducible values.
- **QG5 pass:** installed hardware is separated from supported hardware; no
  known contradiction remains unresolved.
- **QG6 pass:** hardware claims are tied to supplied forensic log locations,
  supplied photographs, and official NVIDIA, LEETOP, KingSpec, Intel, Texas
  Instruments, USB-IF, SIYI, Livox, MAVLink, ArduPilot, and Matek
  documentation.
- **QG7 pass:** high/medium/low opportunities and risks are bounded above.
