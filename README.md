# QtRNetAnalyzer

QtRNetAnalyzer is a Qt 6 desktop analyzer for CAN and R-Net traffic.
It provides live CAN tables, decoded R-Net views, frame tagging, manual replay, and signal plotting for development and laboratory analysis without requiring a connected wheelchair.

> Safety note: the included wheelchair/JSM material is for **simulation and analyzer replay only**. It is not a real JSM registration procedure and must not be used as a direct control sequence for a powered wheelchair.

## Features

- Live CAN frame table
- R-Net decoded frame table
- R-Net frame aggregation by type/key
- Taggable R-Net frames
- Live signal plotting for selected/tagged frames
- Manual simulation replay from candump/Lua/text sources
- Synthetic R-Net wheelchair simulation with simulated JSM login and drive frames
- Optional proprietary ControlCAN hardware integration when the SDK files are present
- Simulation-first workflow for development without hardware

## Build modes

### Simulation-only build

The project always builds without the proprietary ControlCAN SDK.
In this mode, hardware capture is disabled, but the simulation menu can replay `candump`, Lua, or text files containing CAN frame tokens.

```bash
cmake -S . -B build/Desktop-Debug
cmake --build build/Desktop-Debug -j"$(nproc)"
./build/Desktop-Debug/QtRNetAnalyzer
```

### Full ControlCAN hardware build

The full hardware version is enabled automatically only when both the ControlCAN header and library are available.
Accepted layouts include:

```text
third_party/controlcan/controlcan.h
third_party/controlcan/libcontrolcan.so

third_party/ControlCAN/controlcan.h
third_party/ControlCAN/libControlCAN.so

third_party/controlcan.h
third_party/libcontrolcan.so
```

If only `controlcan.h` exists but the library is missing, the build intentionally falls back to simulation-only mode.
This avoids linker errors such as unresolved references to `VCI_OpenDevice`, `VCI_InitCAN`, `VCI_Receive`, or `VCI_Transmit`.

## Simulation menu

The simulation menu is manual by design:

```text
Simulation
├── Select source...
├── Load R-Net wheelchair simulation (JSM login)
├── Start once
├── Start repeat
└── Stop
```

No simulation starts automatically.
Select or load a source first, then start it manually.

## Included R-Net wheelchair simulation

The simulator can replay a synthetic R-Net wheelchair startup and drive sequence.
The sequence is intended to exercise the analyzer UI, CAN table, R-Net decoder, signal history, and plotting logic.

It includes simulated examples of:

- CAN bus startup/test frames such as `00C#`
- simulated JSM login/authentication phases
- simulated parameter/mode frames
- simulated ready/status frames
- simulated speed-limit/status frames
- simulated joystick frames such as `02000300#XxYy`
- simulated battery/load/heartbeat frames

This is not a real wheelchair startup sequence.
It is intentionally a laboratory simulation.

## Simulation files

Simulation files are stored below:

```text
doc/simulations/
├── README.md
├── FILES.txt
├── rnet_wheelchair_jsm_login_drive.candump
├── controlcan_capture_converted.candump
├── controlcan_capture_summary.md
└── controlcan_capture.csv
```

Typical usage:

```text
Simulation -> Select source...
doc/simulations/controlcan_capture_converted.candump
Simulation -> Start once
```

Or use the built-in synthetic source:

```text
Simulation -> Load R-Net wheelchair simulation (JSM login)
Simulation -> Start once
```

## RX/TX direction convention

Direction labels are written from the ESP/gateway point of view and are included directly in simulation/replay files:

```text
RX = App -> ESP command; later real-CAN mode sends this CAN frame
TX = ESP -> App report; later real-CAN mode received this CAN frame
```

In short: **RX CAN frames are later sent by the ESP**, and **TX CAN frames are later received/reported back to the app/analyzer**.
This convention is used consistently in the built-in simulator, replay logs, and generated `.candump` files.

## Relative simulation timing

Simulation and replay timestamps are normalized to the first valid frame.
The first frame is shown as:

```text
0.000000 s
```

All following times are relative to that zero point.
This makes captures easier to compare regardless of their original absolute timestamp source.

## Candump examples

The analyzer accepts candump-style CAN frame tokens with optional timestamp and RX/TX marker, for example:

```text
(0.000000) RX can0 00C#
(0.020000) RX can0 02000100#0000
(0.030000) RX can0 02000100#0064
(0.040000) RX can0 02000100#6400
(0.050000) TX can0 1C0C0300#60
```

Legacy bare frame tokens are still accepted for quick manual tests:

```text
00C#
02000300#0000
```

For R-Net joystick test traffic, the common synthetic form is:

```text
02000M00#XxYy
```

Where:

```text
M    = simulated device/module slot
Xx   = signed int8 X axis encoded as one byte
Yy   = signed int8 Y axis encoded as one byte
```

## Development workflow

Recommended workflow for this branch:

```bash
git checkout chatgpt
git pull --rebase origin chatgpt
cmake -S . -B build/Desktop-Debug
cmake --build build/Desktop-Debug -j"$(nproc)"
./build/Desktop-Debug/QtRNetAnalyzer
```

After changing this README:

```bash
git add README.md
git commit -m "Update README for R-Net wheelchair simulation"
git push origin chatgpt
```

## Current branch policy

All generated fixes are based on:

```text
https://github.com/notwendig/QtRNetAnalyzer/tree/chatgpt
```

Generated ZIP files should preserve the real project structure and contain only the affected complete files unless a larger project export is explicitly requested.
