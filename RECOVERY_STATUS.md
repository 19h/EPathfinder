# Implementation status and evidence register

## Scope and completion boundary

This repository reconstructs the EPathfinder companion-computer application
used in two Gerbera-family aircraft. It covers the onboard Jetson application,
protocol adapters, state model, vision interfaces, and controller API. It does
not contain the unidentified flight-controller firmware, ThunderGaze source,
VNav implementation, radio/modem firmware, camera firmware, propulsion control,
or physical payload electronics.

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
| Support | Logging, remote controller, interception client, and LED/GPIO state | Strong protocols; physical GPIO writes remain inert |
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
part number. Dependent results: sensor, lens, zoom mechanism, and gimbal identity
remain unknown; only UVC identity and 1080p30 MJPEG operation are asserted.

Probe: photograph labels, query full UVC extension controls, inspect USB strings
from the running device, or disassemble the camera module.

### A21 — Flight-controller and ELink PCBs

Assumption: `/dev/ttyAP`, `/dev/ttyFC`, MAVLink, and the expected ST USB VCP
identify interfaces and roles but not board models. Dependent results: no
Pixhawk/Cube/Matek/CUAV or ELink hardware assignment is made.

Probe: obtain USB serial/product strings, board photographs, PCB markings,
firmware-identification messages, or bootloader responses.

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

## Resolved hardware ambiguities

- Jetson is resolved to Orin NX 16 GB P3767-0000; older Orin Nano strings are
  excluded as factory/test boot residue.
- Physical carrier markings resolve the vendor to LEETOP, PN
  `900-14887-0000`, Rev. 2.1. Its layout supports an A603 V2.1-family
  identification under A19; P3768-0000 remains the software configuration.
- The operating camera is the USB UVC `HJ-ZOOM10-4K`, not the failed IMX219
  device-tree entry and not the optional ZR10 backend.
- Active camera mode is 1080p30 MJPEG; the product string does not prove 4K use.
- Intel AC 8265 supplies both Wi-Fi and Bluetooth; no Quectel modem was
  detected.
- `/dev/ttyAP` is the Tegra UART used for primary MAVLink;
  `/dev/ttyFC` is the USB CDC ACM path assigned to ELink.

## Bounded opportunities and risks

- **High impact:** deterministic flight-law telemetry would close A15.
- **High impact:** labeled ThunderGaze datagrams would close A13.
- **High impact:** the external zoom-map dataset would enable the full remapping
  path under A3.
- **High impact:** unauthenticated client/VNav/ELink paths expose command and
  state surfaces; transport authentication and sender binding are not observed.
- **Medium impact:** vendor confirmation, controller/camera inspection, and
  photograph chain-of-custody correlation would close A19–A21 and A23.
- **Medium impact:** road and plan differential corpora would constrain A11 and
  A14.
- **Medium impact:** isolated host-operation traces would close A16.
- **Low impact:** native Qt 6 testing validates source behavior; deployment ABI
  remains a Qt 5/AArch64 property under A1/A10.

## Quality gates

- **QG1 pass:** no normative conclusion is required.
- **QG2 pass:** A1–A23 state assumptions, dependent results, and falsification
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
  supplied photographs, and official NVIDIA, LEETOP, KingSpec, Intel, SIYI,
  Livox, and ArduPilot documentation.
- **QG7 pass:** high/medium/low opportunities and risks are bounded above.
