# QtRNetAnalyzer

Qt6 desktop analyzer for CAN and R-Net traffic with live tables, R-Net decoding, tagging, simulation replay, and live signal plots.

## Features

- live CAN table with sorting
- decoded R-Net table with aggregation by type key
- taggable R-Net messages
- Signal View for selected/tagged R-Net frames
- manual simulation replay from candump/text/Lua-style sources
- optional ControlCAN hardware support

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
./QtRNetAnalyzer
```

## Simulation replay

The application does **not** auto-start simulation.

Use the menu:

```text
Simulation
 ├── Select source...
 ├── Start once
 ├── Start repeat
 └── Stop
```

Supported input sources include candump-style text, for example:

```text
(1622641895.104166) can0 02000300#0000
can0 1C0C0000#32
```

Text or Lua files may also be used when they contain embedded CAN frame tokens in the same `ID#DATA` style.

## ControlCAN full version

ControlCAN support is optional, but automatic.

If this file exists:

```text
third_party/controlcan.h
```

CMake builds the **full ControlCAN-enabled version**.

If it does not exist, CMake builds the simulation/candump-only version.

Recommended layout:

```text
QtRNetAnalyzer/
 ├── CMakeLists.txt
 ├── src/
 ├── doc/
 └── third_party/
      ├── controlcan.h
      └── libcontrolcan.so        # optional, depending on SDK
```

Accepted library names in `third_party/`:

```text
libcontrolcan.so
libControlCAN.so
libcontrolcan.a
libControlCAN.a
```

If `third_party/controlcan.h` exists but no matching library is found, CMake still enables the full ControlCAN source path and prints a warning. If your SDK requires linking against a vendor library, the build will then fail at link time with missing `VCI_*` symbols until the library is added.

## Notes

- `CMakeLists.txt` target name is `QtRNetAnalyzer`.
- `src/` is the canonical source directory.
- ControlCAN must remain optional so the project can always be built and tested with simulation replay.
