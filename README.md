# QtRNetAnalyzer

Qt6 desktop analyzer for CAN and R-Net traffic with live tables, R-Net decoding, tagging, and live signal plotting.

## Features

- Live CAN table with sorting
- Decoded R-Net table with aggregation by type key
- Taggable R-Net messages
- Live plot window for tagged frames including accumulated history
- Simulator mode for development without hardware
- Optional proprietary ControlCAN integration

## Screenshots

### Live Table
Real-time CAN frame capture with timestamp, ID, DLC and raw payload view.  
Optimized for high bus load and continuous monitoring.

<img src="doc/pictures/Bildschirmfoto vom 2026-04-23 18-50-01.png" width="900">

---

### R-Net Decoder Table
Decoded R-Net frames with type grouping, counters and parameter extraction.

<img src="doc/pictures/Bildschirmfoto vom 2026-04-23 18-58-44.png" width="900">

---

### R-Net Signal Plot
Interactive live visualization of tagged R-Net messages.  
Tracks payload evolution over time with full history support.

<img src="doc/pictures/Bildschirmfoto vom 2026-04-23 18-57-49.png" width="900">

---

## Build

### Simulator-only build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
./QtRNetAnalyzer --input <candump.txt>
