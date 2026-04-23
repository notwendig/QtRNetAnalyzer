# QtRNetAnalyzer

Qt6 desktop analyzer for CAN and R-Net traffic with live tables, R-Net decoding, tagging, and live signal plots.

## Features

- live CAN table with sorting
- decoded R-Net table with aggregation by type key
- taggable R-Net messages
- live plot window for tagged frames including accumulated history
- simulator mode for development without hardware
- optional proprietary ControlCAN integration

## Screenshots

### Live Table
<img src="doc/pictures/Bildschirmfoto vom 2026-04-23 18-50-01.png" width="300">

### R-Net Decoder Table
<img src="doc/pictures/Bildschirmfoto vom 2026-04-23 18-58-44.png" width="300">

### R-Net Signal-Plot Table
<img src="doc/pictures/Bildschirmfoto vom 2026-04-23 18-57-49.png" width="300">
## Build

### Simulator-only build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
./QtRNetAnalyzer --input <candump.txt>
```

### Build with proprietary ControlCAN SDK

Place the vendor SDK header and shared library in `third_party/` and configure with:

```bash
mkdir -p build
cd build
cmake -DENABLE_CONTROLCAN=ON ..
cmake --build .
./QtRNetAnalyzer
```

Expected library path:

- `third_party/x86/64-linux/libcontrolcan.so`

## Repository layout

- `*.cpp`, `*.h` — application source code, licensed under GPL-3.0-only
- `README.md` and other documentation — licensed under CC BY-NC-SA 4.0
- `third_party/` — not included; vendor SDK files belong to their respective owners

## Licensing

### Code

This project's source code is licensed under the GNU General Public License v3.0 only. See `LICENSE`.

### Documentation and analysis text

Documentation, protocol notes, and explanatory text in this repository are licensed under
Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International. See `LICENSE.docs`.

## Authors

- ChatGPT (GPT-5.4 Thinking)
- Jürgen Willi Sievers <JSievers@NadiSoft.de>

## Note about proprietary SDK files

The optional ControlCAN integration depends on vendor-provided proprietary files that are **not** part of this repository.
You must obtain those files yourself and place them locally under `third_party/`.
