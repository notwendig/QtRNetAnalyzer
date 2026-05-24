# waveUSBCAN_b SocketCAN setup for QtRNetAnalyzer

QtRNetAnalyzer uses Linux SocketCAN for hardware capture. The USB-CAN adapter is handled by the external `waveUSBCAN_b` driver, which exposes interfaces such as `can0` and `can1`.

## Install or update driver

```bash
cd ~/AndroidStudioProjects/waveUSBCAN_b
sudo ./scripts/install.sh
sudo systemctl enable --now waveusbcan_b-auto.service
ip -details link show type can
```

## Manual R-Net listen-only setup

```bash
sudo ip link set can0 down 2>/dev/null || true
sudo ip link set can0 type can bitrate 125000 restart-ms 100 listen-only on
sudo ip link set can0 up

sudo ip link set can1 down 2>/dev/null || true
sudo ip link set can1 type can bitrate 125000 restart-ms 100 listen-only on
sudo ip link set can1 up
```

## QtRNetAnalyzer mapping

```text
Device index 0 -> can0 / can1
Device index 1 -> can2 / can3
```

QtRNetAnalyzer does not set the bitrate on SocketCAN interfaces. Configure bitrate and listen-only mode before opening the device in the GUI.

## Smoke test

```bash
ip -details link show type can
candump can0
```

If this works, the Qt application should be able to capture from the same interface.
