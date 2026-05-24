## Next

- Extend the R-Net decoder with additional known CAN frame families derived from the public Open R-Net frame dictionary and protocol notes.
- Add Open R-Net attribution and safety notes under `doc/open_rnet_decoder_attribution.md`.
- CSV logging now records to an in-memory buffer after **Start CSV Log** and asks for the output filename only on **Stop CSV Log**.
- Default CSV filename is `<current-path>/R-Netlog-YYYYMMDD-HHMMSS.csv`.


## Unreleased

- Added Qt SerialBus/QCanBus based Device menu for SocketCAN interface selection.
- Linked QtRNetAnalyzer against Qt6::SerialBus so Qt's SocketCAN plugin/factory can enumerate available CAN devices.

# Changelog

## Unreleased

- Switch active hardware capture path to Linux SocketCAN.
- Document `waveUSBCAN_b` as the recommended Waveshare USBCAN-B / CANalyst-II driver.
- Add missing built-in R-Net wheelchair simulation source files.
- Add GitHub CI workflow for Qt6/CMake Linux builds.
- Add issue templates, pull request template, contribution notes and safety policy.
