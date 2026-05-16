# R-Net Wheelchair Simulation Files

These files are lab-only simulation/replay sources for QtRNetAnalyzer.
They are not real R-Net/JSM startup instructions for a wheelchair.

## Direction convention

All RX/TX labels are from the ESP/gateway module point of view and are mandatory in the simulation files:

```text
RX = App -> ESP command; later real-CAN mode sends this CAN frame
TX = ESP -> App report; later real-CAN mode received this CAN frame
```

In short: **RX CAN frames are later sent by the ESP**, and **TX CAN frames are later received/reported by the ESP/App path**.

The parser accepts these markers in candump-like files:

```text
(0.000000) RX can0 00C#
(0.040000) TX can0 7B3#R
```

Legacy replay lines without a direction marker are still accepted and default to RX, but new simulation files should always include RX or TX.

## Timing convention

All simulation/replay timestamps are relative to the first valid CAN frame.
The first replayed frame is normalized to:

```text
0.000000 s
```

QtRNetAnalyzer v5 also reads the timestamp inside candump parentheses and no longer replaces it with a fixed synthetic 100 us step.

## Files

```text
rnet_wheelchair_jsm_login_drive.candump
controlcan_capture_converted.candump
controlcan_capture_summary.md
controlcan_capture.csv
```

## Usage

1. `Simulation -> Select source...`
2. choose a file from `doc/simulations/`
3. `Simulation -> Start once` or `Start repeat`

The login sequence is intentionally synthetic/non-authentic. It is for analyzer/emulation tests only.
