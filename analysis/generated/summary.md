# Component inventory summary

This public summary reports architectural counts and component coverage.

- Application source concepts: 61
- Qt meta-object generation concepts: 53
- Vendored Livox SDK source concepts: 22
- Supporting runtime/toolchain concepts: 9
- Qt meta-objects: 57
- Qt meta-object methods: 819
- Qt signals: 284

## Largest application class surfaces

| Class | Recovered function concepts |
|---|---:|
| `EPathfinder` | 274 |
| `EScout` | 164 |
| `VehicleController` | 113 |
| `ELinkTelemetry` | 70 |
| `ClientController` | 66 |
| `ControlHandler` | 65 |
| `EVision` | 65 |
| `RoadRunner` | 53 |
| `HeartbeatHandler` | 42 |
| `PlanHandler` | 41 |
| `ThunderGaze` | 37 |
| `Camera` | 35 |
| `VNav` | 28 |
| `CameraZR10` | 22 |
| `LdsLidar` | 21 |
| `MavLinkCommunicator` | 21 |
| `LogHandler` | 19 |
| `ELogger` | 18 |

The exhaustive Qt interface inventory is in
[moc_interfaces.md](moc_interfaces.md). Hardware and integration context is in
[../../docs/HARDWARE_ARCHITECTURE.md](../../docs/HARDWARE_ARCHITECTURE.md).
