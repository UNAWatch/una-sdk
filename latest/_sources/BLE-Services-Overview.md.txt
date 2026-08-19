# UNA Watch — BLE Services Overview

This is the map of the GATT services a UNA Watch exposes over Bluetooth Low
Energy. Most are **standard Bluetooth SIG or vendor services** — any BLE tool
recognizes them. The File Transfer Service has its own reference:
[BLE-File-Transfer-Service.md](./BLE-File-Transfer-Service.md).

**Security.** The watch requires a **bonded, encrypted connection**. Pair with
the watch before reading or writing any characteristic.

## Services

The standard Bluetooth SIG and Nordic services follow their published
specifications — refer to those documents for field formats. The File Transfer
Service is UNA's (Adafruit-based); see its own reference.

| Service | UUID | Source | Purpose |
|---|---|---|---|
| Device Information | `0x180A` | Bluetooth SIG | Manufacturer, model, serial, firmware/hardware revisions |
| Current Time | `0x1805` | Bluetooth SIG | Time sync from the phone |
| Battery | `0x180F` | Bluetooth SIG | Battery level |
| File Transfer | `0xFEBB` | Adafruit | File read/write, OTA — see its own reference |
| Nordic UART | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` | Nordic | Serial-style data channel |

### Device Information (`0x180A`)

| Characteristic | UUID |
|---|---|
| Manufacturer Name | `0x2A29` |
| Model Number | `0x2A24` |
| Serial Number | `0x2A25` |
| Firmware Revision | `0x2A26` |
| Hardware Revision | `0x2A27` |

Reading `0x2A26` is the supported way to discover the watch's firmware version.

### Current Time (`0x1805`)

| Characteristic | UUID |
|---|---|
| Current Time | `0x2A2B` |
| Local Time Information | `0x2A0F` |

### Battery (`0x180F`)

| Characteristic | UUID |
|---|---|
| Battery Level | `0x2A19` |

Standard `uint8` percentage; subscribe for notifications on level change.

### Nordic UART (`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`)

A Nordic UART Service (serial-style channel). Note the two data characteristics
are **swapped relative to Nordic's usual convention**:

| Characteristic | UUID | Direction |
|---|---|---|
| Watch → client | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` | Notify |
| Client → watch | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` | Write / Write Without Response |

---

*Bluetooth SIG service/characteristic formats are defined by the Bluetooth SIG;
the File Transfer Service is based on the Adafruit protocol (see its reference).*
