# Contributing to QtRNetAnalyzer

Thank you for improving QtRNetAnalyzer.

## Development branch

Use the `chatgpt` branch for ongoing development unless a maintainer says otherwise.

## Build before submitting

```bash
cmake -S . -B build/Desktop-Debug -DQTRNET_ENABLE_SOCKETCAN=ON
cmake --build build/Desktop-Debug -j"$(nproc)"
./build/Desktop-Debug/QtRNetAnalyzer
```

For simulator-only checks:

```bash
cmake -S . -B build/Desktop-Debug -DQTRNET_ENABLE_SOCKETCAN=OFF
cmake --build build/Desktop-Debug -j"$(nproc)"
```

## Code style

- Use C++20 and Qt 6 idioms.
- Prefer small, reviewed changes over large mixed refactors.
- Keep UI, model, decoder and hardware backend responsibilities separated.
- Avoid blocking hardware or file I/O on the GUI thread.
- Keep real CAN TX behavior explicit and guarded.

## Hardware path

The active hardware path is Linux SocketCAN. For Waveshare USBCAN-B / CANalyst-II compatible devices, use the external `waveUSBCAN_b` driver. Do not reintroduce proprietary ControlCAN SDK dependencies into the default build path.

## Documentation

Update `README.md` and files under `doc/` when changing build steps, driver assumptions, UI behavior or safety-relevant behavior.
