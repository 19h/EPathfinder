# Forensic hardware inventory

This document records hardware attributable to two recovered Gerbera aircraft
storage images and the supplied hardware photographs. It separates direct
operating-system observations, physical markings, vendor identification, and
unsupported software options.

Confidence terms:

- **Confirmed:** direct descriptor, kernel, firmware, or application-log
  evidence identifies the component or active mode.
- **High:** the observed configuration has a unique or strongly constrained
  vendor mapping, but a compatible clone cannot be excluded.
- **Unknown:** the available evidence does not identify the physical part.

## Shared onboard bill of materials

| Component | Identification | Confidence and evidence |
|---|---|---|
| Computer module | NVIDIA Jetson Orin NX 16 GB, P3767-0000, photographed PN `900-13767-0000-000` and serial `1423223010849`; operating-system report includes `699-13767-0000-300`; Tegra234 SKU 211, eight CPU cores | Confirmed from platform identification in both images and module markings; NVIDIA maps P3767-0000 and `900-13767-0000-000` to Orin NX 16 GB |
| Physical carrier board | LEETOP, marked PN `900-14887-0000`, Rev. 2.1, date code `20230726` | Manufacturer, marked PN, and revision confirmed photographically. Connector layout and release timing match LEETOP A603 V2.1; commercial model is high-confidence because LEETOP's current A603 page instead lists PN `900-44887-0000` |
| Carrier software configuration | NVIDIA P3768-0000 Orin Nano reference-carrier configuration | Confirmed at `etc/nv_boot_control.conf:1` in both images. This is the active board-support configuration, not an identification of the physical carrier manufacturer |
| Cooling | Carrier-mounted active fan and module heat spreader | Confirmed photographically; the exact fan manufacturer and electrical rating are unknown |
| Camera descriptor | USB Video Class 1.10 `HJ-ZOOM10-4K`, VID:PID `32e4:9415` | Confirmed descriptor/interface; `var/log/dmesg:802` for unit 12702 and `var/log/dmesg:824` for unit 12674 |
| Active camera mode | 1,920 × 1,080 pixels, 30 frames/s, MJPEG, hardware decode | Confirmed in application logs at `var/log/syslog:2660523` and `var/log/syslog:2858051`; 4K capture was not observed |
| Wi-Fi/Bluetooth | Intel Dual Band Wireless-AC 8265, PCI ID `8086:24fd`, integrated Intel Bluetooth | Confirmed in both kernel logs. Intel specifies 2×2 Wi-Fi 5 on 2.4/5 GHz and Bluetooth 4.2 |
| Ethernet | Realtek RTL8111/8168/8411-family Gigabit Ethernet, PCI ID `10ec:8168`, `r8168` driver | Controller family confirmed; exact revision unavailable |
| NVMe controller | MAXIO MAP1202, PCI ID `1e4b:1202`, PCIe Gen3 ×2 negotiated | Confirmed in both kernel logs at `var/log/dmesg:437` |
| USB expansion | Four-port high-speed USB 2.0 hub | Confirmed; hub-controller identity and downstream physical port assignment unavailable |
| Primary autopilot interface | Tegra UART `ttyTHS1`, persistent name `/dev/ttyAP` | Confirmed interface. `settings.json` assigns this path to the primary MAVLink connection |
| Secondary ELink/controller interface | USB CDC ACM, persistent name `/dev/ttyFC`; device rule expects STMicroelectronics VCP VID:PID `0483:5740` | Interface and rule confirmed. `settings.json` assigns this path to ELink; the attached PCB is unknown |

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

## Storage identification

The unit-12702 device reports `NE-128 2242`. KingSpec's official NE-series
specification defines an M.2 2242 NVMe Rev. 1.3 device using PCIe Gen3 ×2 and
includes a 128 GB capacity. This makes the KingSpec identification strong. Unit
12674 exposes the same controller family and capacity but lacks a retained
retail model string or physical-label record; it is not assigned the KingSpec
brand in this document.

## Imaging interpretation

`HJ-ZOOM10-4K` is the device-reported USB product string, not a verified sensor
or optical assembly part number. The observed stream was 1080p30 MJPEG. The
camera name therefore does not establish a 4K operating mode, 10× optical zoom,
sensor manufacturer, focal-length range, or gimbal model.

The application source contains a `CameraT205` USB/V4L2 backend and a separate
SIYI ZR10 network/serial backend. The recovered aircraft used the USB UVC path.
The ZR10 class proves software compatibility only and is not evidence that a
ZR10 was physically installed.

## Flight-control interpretation

The primary `/dev/ttyAP` path carries the application's MAVLink/ArduPilot-facing
traffic. `/dev/ttyFC` is assigned to the proprietary ELink stack. These names
and protocols identify logical roles, not the attached controller boards.
ArduPilot compatibility does not identify a Pixhawk, Cube, Matek, CUAV, or any
other flight-controller family.

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
- The flight-controller PCB, GNSS receiver, IMU, compass, barometer, engine,
  motor controller, battery, power distribution, antennas, airframe structure,
  camera sensor/lens, and terminal payload hardware remain unknown.

## Primary references

- [NVIDIA module/carrier mapping](https://docs.nvidia.com/jetson/archives/r36.4.3/DeveloperGuide/IN/QuickStart.html#jetson-modules-and-configurations)
- [NVIDIA Orin Nano/NX module part-number table](https://developer.download.nvidia.com/assets/embedded/secure/jetson/orin_nano/docs/Jetson-Orin-Nano-NX-CoV_DA-11856-001v01.pdf)
- [LEETOP A603 product specification](https://www.leetop.top/products/a603-carrier-board-for-orin-nx-orin-nano)
- [LEETOP A603 Carrier Board V2.1 manual](https://files.seeedstudio.com/Leetop_A603_Carrier_Board_V2.1%200228.pdf)
- [KingSpec NE 2242 specification](https://www.kingspec.com/product/m2-nvme-pcie-gen3-ssd-ne-2242mm.html)
- [Intel Wireless-AC 8265 specification](https://www.intel.com/content/www/us/en/products/sku/94150/intel-dual-band-wirelessac-8265/specifications.html)
