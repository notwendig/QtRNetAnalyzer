# Open R-Net decoder attribution

QtRNetAnalyzer includes a table-based decoder extension derived from the public frame dictionary and protocol notes in:

- Repository: https://github.com/redragonx/open-rnet
- Authors credited upstream: Stephen Chavez and Specter
- License upstream: GPL-3.0

The imported knowledge is intentionally represented as decoder patterns rather than transmit automation. QtRNetAnalyzer remains an analyzer/lab tool and should be used in listen-only mode when connected to real powered wheelchair hardware.

## Imported decoder families

The decoder table in `src/rnetframe.cpp` adds recognition for additional known R-Net CAN messages documented by Open R-Net, including:

- sleep/wake/config RTR variants
- extended serial/authentication exchange families `0x1F...`
- cJSM authentication retry / slot-8 frames
- POP Quick programmer request/response markers
- POP segmented request/response and configuration transfer frames
- Bluetooth module status/control frames
- lamp-control status frames
- UI/status/error-trigger frame families
- partially decoded module/motor/diagnostic families

## Safety

R-Net is used in powered wheelchair systems. Treat decoded control, authentication and configuration frames as protocol analysis data. Do not use this information to transmit frames on a real chair bus without an isolated bench setup, physical emergency stop and a full understanding of the safety consequences.
