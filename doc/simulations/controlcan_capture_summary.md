# ControlCAN Capture Conversion

Source file: `controlcan_capture.csv`

Generated file: `controlcan_capture_converted.candump`


## Direction convention

All RX/TX labels in the converted `.candump` file are from the ESP/gateway point of view:

```text
RX = App -> ESP command; later real-CAN mode sends this CAN frame
TX = ESP -> App report; later real-CAN mode received this CAN frame
```

Legacy frames without explicit direction markers default to RX in QtRNetAnalyzer, but this converted file now writes RX/TX on every CAN line.

## Capture summary

- Rows: 337
- Converted frames: 337
- Duration: 3.400000 s
- Frame types: STD/DATA: 11, STD/RTR: 1, EXT/DATA: 325
- Error flags: 0: 337

## Most common CAN IDs

| CAN ID | Count |
|---|---:|
| `02000300` | 264 |
| `1C0C0300` | 15 |
| `03C30F0F` | 14 |
| `0C140300` | 14 |
| `14300300` | 14 |
| `7B3` | 2 |
| `043` | 2 |
| `00C` | 1 |
| `00E` | 1 |
| `783` | 1 |
| `793` | 1 |
| `051` | 1 |
| `050` | 1 |
| `061` | 1 |
| `060` | 1 |
| `1C240301` | 1 |
| `0A040300` | 1 |
| `1C300304` | 1 |
| `181C0D00` | 1 |

## Timing

All timestamps in the converted file are relative to the first valid CAN frame and start at `0.000000 s`.

| Interval µs | Count |
|---:|---:|
| 10000 | 332 |
| 20000 | 4 |


## Joystick frame

Primary joystick-like frame:

```text
02000300#XxYy
```

Observed joystick-frame count: 264

Most common joystick payloads:

| Data | Count |
|---|---:|
| `0000` | 63 |
| `0028` | 42 |
| `0014` | 3 |
| `0001` | 2 |
| `0002` | 2 |
| `0003` | 2 |
| `0004` | 2 |
| `0005` | 2 |
| `0006` | 2 |
| `0007` | 2 |
| `0008` | 2 |
| `0009` | 2 |
| `000A` | 2 |
| `000B` | 2 |
| `000C` | 2 |

Detected joystick payload changes, first 20:

```text
0.180000s 02000300#0000
0.640000s 02000300#0001
0.650000s 02000300#0002
0.660000s 02000300#0003
0.670000s 02000300#0004
0.680000s 02000300#0005
0.690000s 02000300#0006
0.700000s 02000300#0007
0.710000s 02000300#0008
0.720000s 02000300#0009
0.730000s 02000300#000A
0.780000s 02000300#000B
0.790000s 02000300#000C
0.800000s 02000300#000D
0.810000s 02000300#000E
0.820000s 02000300#000F
0.830000s 02000300#0010
0.840000s 02000300#0011
0.850000s 02000300#0012
0.860000s 02000300#0013
```


## Usage in QtRNetAnalyzer

1. Start QtRNetAnalyzer.
2. Open `Simulation -> Select source...`.
3. Select `doc/simulations/controlcan_capture_converted.candump`.
4. Run `Simulation -> Start once` or `Simulation -> Start repeat`.

This file is intended only for simulation/replay in the analyzer.
