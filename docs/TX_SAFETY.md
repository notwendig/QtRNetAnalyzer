# QtRNetAnalyzer TX safety

Transmit is disabled by default. This is deliberate: a real R-Net wheelchair CAN
bus is safety-critical.

Normal analysis build:

```bash
cmake -S . -B build/Desktop_Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Desktop_Debug
```

Labor-only transmit build, never on a wheelchair with wheels on the ground:

```bash
cmake -S . -B build/Desktop_Debug -DCMAKE_BUILD_TYPE=Debug -DQTRNET_ENABLE_DANGEROUS_TX=ON
cmake --build build/Desktop_Debug
```
