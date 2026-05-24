# Security and safety policy

QtRNetAnalyzer is a CAN/R-Net analysis tool. Incorrect use on real mobility hardware can be dangerous.

## Supported branch

Security and safety-relevant fixes should target the active development branch `chatgpt` first and then be merged to `main` after validation.

## Reporting

Please report security or safety-relevant issues privately to the maintainer when possible. Include:

- affected commit or release,
- exact build configuration,
- hardware setup,
- whether CAN TX was enabled,
- reproduction steps,
- logs or candump snippets where safe to share.

## Safety expectations

- Prefer listen-only CAN mode on real wheelchair/R-Net buses.
- Do not transmit frames on real hardware unless the setup is isolated and intentional.
- Treat simulator data as synthetic lab data, not as a guarantee of real device behavior.
