# QtRNetAnalyzer

Qt6 desktop analyzer for CAN and R-Net traffic with live tables, R-Net decoding,
tagging, live signal plots, and manual simulation replay from candump/lua text files.

## Build modes

### Simulation-only build

The project always builds without the proprietary ControlCAN SDK. In this mode,
hardware capture is disabled, but the simulation menu can replay `candump.txt` or
Lua/text files containing CAN frame tokens.

```bash
cmake -S . -B build/Desktop-Debug
cmake --build build/Desktop-Debug
```

### Full ControlCAN hardware build

The full hardware version is enabled automatically only when both files exist:

```text
third_party/controlcan/controlcan.h
third_party/controlcan/libcontrolcan.so
```

Alternative accepted paths/names:

```text
third_party/ControlCAN/controlcan.h
third_party/ControlCAN/libControlCAN.so
third_party/controlcan.h
third_party/libcontrolcan.so
```

If only `controlcan.h` exists but the library is missing, the build intentionally
falls back to simulation-only mode to avoid linker errors such as undefined
references to `VCI_OpenDevice`, `VCI_InitCAN`, `VCI_Receive`, or `VCI_Transmit`.

## Simulation menu

```text
Simulation
 ├── Select source...
 ├── Start once
 ├── Start repeat
 └── Stop
```

No simulation starts automatically. Select a source first, then start once or
repeat manually.

## Current branch policy

All generated fixes are based on:

```text
https://github.com/notwendig/QtRNetAnalyzer/tree/chatgpt
```
