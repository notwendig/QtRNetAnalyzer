# QtRNetAnalyzer

QtRNetAnalyzer is a Qt 6 desktop analyzer for CAN and R-Net traffic. It provides live CAN tables, decoded R-Net aggregation, taggable R-Net messages, a live signal plot window and simulator/replay support.

The current hardware path is **Linux SocketCAN**. For Waveshare USBCAN-B / CANalyst-II compatible hardware, use the external [`waveUSBCAN_b`](https://github.com/notwendig/waveUSBCAN_b) driver to expose normal Linux CAN interfaces such as `can0` and `can1`. QtRNetAnalyzer then opens those SocketCAN interfaces directly.

## Current status

- Primary branch for ongoing development: `chatgpt`
- Default hardware backend: SocketCAN on Linux
- Recommended USB-CAN driver: `waveUSBCAN_b`
- Proprietary ControlCAN/VCI userspace integration: removed from the active path
- Simulator/replay mode: available without hardware

## Features

- Live CAN frame table with timestamp, channel, direction, ID, DLC and payload
- Decoded R-Net table with aggregation by R-Net type key
- Taggable R-Net messages for signal plotting
- Live signal plot window with accumulated history
- Simulator menu: select source, start once, start repeat, stop
- Built-in synthetic R-Net wheelchair/JSM login lab scenario for UI testing
- Linux SocketCAN capture via `can0` / `can1`
- CSV/log support for captured CAN frames

## Safety note

R-Net is used in powered wheelchair systems. Treat this tool as an analyzer and lab tool. Do not transmit frames on a real wheelchair bus unless you fully understand the electrical, safety and legal consequences. For real hardware analysis, prefer listen-only CAN configuration whenever possible.

## Repository layout

```text
.
├── .github/                 GitHub CI, issue templates and PR template
├── doc/                     Additional documentation and notes
├── src/                     Qt/C++ source code
├── tools/                   Helper scripts
├── CMakeLists.txt           Top-level Qt/CMake build
├── README.md                Project overview and setup notes
├── LICENSE                  GPL-3.0-only for source code
└── LICENSE.docs             CC BY-NC-SA 4.0 for documentation/analysis text
```

## Requirements

### QtRNetAnalyzer

- Linux recommended for hardware capture
- CMake >= 3.21
- C++20 compiler
- Qt >= 6.5 with Core, Widgets and SerialBus modules
- Optional for hardware capture: Linux SocketCAN headers, Qt SocketCAN plugin and a working CAN interface

Fedora example:

```bash
sudo dnf install -y cmake ninja-build gcc-c++ qt6-qtbase-devel qt6-qtserialbus-devel
```

Ubuntu/Debian example:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build qt6-base-dev libgl1-mesa-dev
sudo apt install -y qt6-serialbus-dev || sudo apt install -y libqt6serialbus6-dev libqt6serialbus6-plugins
```

### Hardware driver

For Waveshare USBCAN-B / CANalyst-II compatible devices, install and start the external driver:

```bash
cd ~/AndroidStudioProjects/waveUSBCAN_b
sudo ./scripts/install.sh
sudo systemctl enable --now waveusbcan_b-auto.service
ip -details link show type can
```

Expected result: Linux CAN interfaces such as `can0` and `can1` are visible.


### Device selection menu

QtRNetAnalyzer now adds a **Device** menu at startup. It uses Qt SerialBus/QCanBus to ask the Qt SocketCAN plugin for currently available CAN interfaces and offers:

- **Auto**: legacy index mapping, for example device index `0` => `can0`/`can1`.
- **waveUSBCAN_b pair**: detected two-channel pairs such as `can0` + `can1`.
- **Single channel**: open only one detected SocketCAN interface as CAN1.

Use **Device -> Refresh SocketCAN devices** after plugging in the adapter or after running `sudo ./scripts/install.sh` in `waveUSBCAN_b`. The menu changes the next capture open; if capture is already running, close and open again.

Internally this uses `QCanBus::availableDevices("socketcan")`. Qt loads the SocketCAN CAN-bus plugin through its QCanBusFactory plugin mechanism; QtRNetAnalyzer does not implement a custom CAN plugin.

## Build

Recommended local build from your project root:

```bash
cd ~/AndroidStudioProjects/QtRNetAnalyzer
git checkout chatgpt

cmake -S . -B build/Desktop-Debug -G Ninja -DQTRNET_ENABLE_SOCKETCAN=ON
cmake --build build/Desktop-Debug -j"$(nproc)"
./build/Desktop-Debug/QtRNetAnalyzer
```

If you do not use Ninja:

```bash
cmake -S . -B build/Desktop-Debug -DQTRNET_ENABLE_SOCKETCAN=ON
cmake --build build/Desktop-Debug -j"$(nproc)"
./build/Desktop-Debug/QtRNetAnalyzer
```

Simulator-only build:

```bash
cmake -S . -B build/Desktop-Debug -DQTRNET_ENABLE_SOCKETCAN=OFF
cmake --build build/Desktop-Debug -j"$(nproc)"
./build/Desktop-Debug/QtRNetAnalyzer --input candump.txt
```

## SocketCAN / waveUSBCAN_b usage

QtRNetAnalyzer no longer initializes the USB-CAN adapter directly. It expects SocketCAN interfaces to already exist and to be configured by Linux or by the `waveUSBCAN_b` service.

Interface mapping in the current UI:

```text
Device index 0 -> can0 / can1
Device index 1 -> can2 / can3
```

R-Net is typically analyzed at 125000 bit/s. A manual listen-only setup can look like this:

```bash
sudo ip link set can0 down 2>/dev/null || true
sudo ip link set can0 type can bitrate 125000 restart-ms 100 listen-only on
sudo ip link set can0 up

sudo ip link set can1 down 2>/dev/null || true
sudo ip link set can1 type can bitrate 125000 restart-ms 100 listen-only on
sudo ip link set can1 up
```

For active lab tests where TX is intentionally required, omit `listen-only on` and use an isolated bench setup.

## Quick hardware smoke test

Before starting QtRNetAnalyzer, verify the driver and interfaces outside the GUI:

```bash
ip -details link show type can
candump can0
```

If `candump` sees frames, QtRNetAnalyzer should also be able to open the corresponding channel.

## Simulation and replay

QtRNetAnalyzer can be used without hardware:

```bash
./build/Desktop-Debug/QtRNetAnalyzer --input path/to/candump.txt
```

Inside the GUI, use the Simulation menu:

- Select
- Load R-Net wheelchair simulation (JSM login)
- Start (repeat)
- Start (once)
- Stop

The built-in wheelchair/JSM scenario is synthetic lab data. It is intended to exercise the UI, aggregation and plotting paths; it is not an authentic R-Net login sequence.

## Troubleshooting

### `SocketCAN interface can0 not found`

The kernel driver has not created the interface, or the interface name is different.

```bash
sudo systemctl status waveusbcan_b-auto.service
ip -details link show type can
```

### `can0 is down`

Bring the interface up before pressing Open in QtRNetAnalyzer:

```bash
sudo ip link set can0 type can bitrate 125000 restart-ms 100
sudo ip link set can0 up
```

### Build fails after old ZIPs or local experiments

Start from the current `chatgpt` branch and rebuild cleanly:

```bash
cd ~/AndroidStudioProjects/QtRNetAnalyzer
git checkout chatgpt
rm -rf build/Desktop-Debug
cmake -S . -B build/Desktop-Debug -DQTRNET_ENABLE_SOCKETCAN=ON
cmake --build build/Desktop-Debug -j"$(nproc)"
```

### Old ControlCAN or submodule files are still visible

The active SocketCAN path does not need proprietary ControlCAN SDK files and does not need the old `waveshares_USBCAN_B` userspace submodule. If those files are still tracked locally and you want a clean SocketCAN-only branch, remove them explicitly:

```bash
git rm -f .gitmodules third_party/waveshares_USBCAN_B 2>/dev/null || true
rm -rf .git/modules/third_party/waveshares_USBCAN_B
```

## GitHub CI

The repository contains a GitHub Actions workflow under `.github/workflows/ci.yml`. It performs a Linux Qt6/CMake build for pushes and pull requests against `main` and `chatgpt`.

## Licensing

Source code is licensed under GPL-3.0-only. See `LICENSE`.

Documentation, protocol notes and analysis text are licensed under CC BY-NC-SA 4.0. See `LICENSE.docs`.

Third-party drivers, vendor SDKs, datasheets and hardware documentation belong to their respective owners and should only be redistributed when their license allows it.

## Authors / credits

- Jürgen Willi Sievers, JSievers@NadiSoft.de
- ChatGPT-assisted development and analysis
