# UNA Watch — BLE File Transfer Service (FTS)

This document is the wire protocol for reading and writing files on a UNA Watch
over Bluetooth Low Energy. It is what a companion app or integration implements
against; it does not describe the watch's internal implementation.

The base protocol is the [Adafruit CircuitPython BLE File Transfer
protocol](https://github.com/adafruit/Adafruit_CircuitPython_BLE_File_Transfer)
(service `0xFEBB`). UNA implements that command set and adds a small set of
backward-compatible **fast-transfer extensions** (protocol version 5): windowed
reads and writes, a file digest query, and safe resume. A client that only knows
the classic (version 4) protocol interoperates unchanged.

## Conventions

- All multi-byte integers are **little-endian**.
- All request/response layouts below are **packed** (no padding beyond the
  `reserved` bytes shown). Byte offsets are given for each field.
- **Paths** are UTF-8, not NUL-terminated on the wire; their length is carried in
  a `pathLength` field. Absolute paths, `/`-separated (e.g.
  `/Apps/Workout/Activity/202607/activity_...fit`).
- Times are **nanoseconds since the Unix epoch** (`uint64`).

## Service and characteristics

| Item | UUID | Properties |
|---|---|---|
| File Transfer Service | `0000FEBB-0000-1000-8000-00805F9B34FB` (`0xFEBB`) | — |
| Version | `ADAF0001-4669-6C65-5472-616E73666572` | Read |
| Raw Transfer | `ADAF0002-4669-6C65-5472-616E73666572` | Write Without Response, Notify |

> **Note.** The characteristic 16-bit IDs are UNA-specific (`ADAF0001` /
> `ADAF0002`) and differ from Adafruit's upstream (`ADAF0100` / `ADAF0200`). Use
> the UUIDs exactly as listed above — they are what the watch exposes.

All operations are a request written to **Raw Transfer** and one or more
responses delivered as **notifications** on the same characteristic. Enable
notifications (write the CCCD) before issuing commands.

**Security.** The watch requires a **bonded, encrypted connection** — pair with
the watch before using FTS. Reads and writes on the characteristics are rejected
on an unencrypted link.

## Capability negotiation

Read the **Version** characteristic — a single `uint32` (little-endian):

- **4** — classic protocol only (Adafruit-compatible, stop-and-wait).
- **≥ 5** — UNA fast-transfer extensions are available.

Gating rules for a version-5 client:

- **Read windowing** needs no gate — it degrades automatically against a v4
  watch (see *Read windowing* under the version-5 extensions below).
- **Write windowing** and **`DIGEST`** MUST be gated on version **≥ 5**.

## Command summary

| ID | Name | Direction |
|---|---|---|
| `0x10` | READ | request |
| `0x11` | READ_DATA | response |
| `0x12` | READ_PACING | request (more) |
| `0x20` | WRITE | request |
| `0x21` | WRITE_PACING | response |
| `0x22` | WRITE_DATA | request (data) |
| `0x30` / `0x31` | DELETE / status | request / response |
| `0x40` / `0x41` | MKDIR / status | request / response |
| `0x50` / `0x51` | LISTDIR / entry | request / response |
| `0x60` / `0x61` | MOVE / status | request / response |
| `0x70` / `0x71` | DIGEST / status | request / response **(UNA extension)** |

Status codes (the `status` byte in responses):

| Value | Meaning |
|---|---|
| `0x01` | OK |
| `0x02` | ERROR (generic) |
| `0x03` | ERROR_NO_FILE |
| `0x04` | ERROR_PROTOCOL (malformed/wrong length) |
| `0x05` | ERROR_READ_ONLY |

## Notification sizing — read this first

A GATT notification cannot carry more than the negotiated **ATT payload = MTU −
3** bytes. Every FTS response has a fixed header, so the usable *data* per
notification is smaller still. For `READ_DATA` (16-byte header) at MTU 220:

```text
data per notification ≤ (MTU − 3) − 16 = 201 bytes
```

Do **not** assume one request maps to one notification of the size you asked for.
A version-5 watch delivers a large read as multiple `READ_DATA` notifications (see
below); each `READ_DATA` is self-describing (`chunkOffset`, `chunkLength`), so
reassemble by offset and never trust a `chunkLength` beyond what the notification
actually delivered.

---

# Classic operations (protocol version 4)

## Read a file

**READ `0x10`** (request):

| Off | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | command = `0x10` | |
| 1 | 1 | reserved | 0 |
| 2 | 2 | pathLength | |
| 4 | 4 | chunkOffset | file offset to start at |
| 8 | 4 | chunkSize | bytes requested |
| 12 | pathLength | path | UTF-8 |

**READ_DATA `0x11`** (response, one or more notifications):

| Off | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | command = `0x11` | |
| 1 | 1 | status | |
| 2 | 2 | reserved | |
| 4 | 4 | chunkOffset | offset of this chunk |
| 8 | 4 | totalLength | total file size |
| 12 | 4 | chunkLength | bytes of data that follow |
| 16 | chunkLength | data | |

**READ_PACING `0x12`** (request more) — after consuming a chunk, ask for the next:

| Off | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | command = `0x12` | |
| 1 | 1 | status = `0x01` | |
| 2 | 2 | reserved | |
| 4 | 4 | chunkOffset | next offset = bytes received so far |
| 8 | 4 | chunkSize | bytes requested |

Classic flow: send `READ`, receive one `READ_DATA`, send `READ_PACING` for the
next chunk, repeat until `chunkOffset + chunkLength == totalLength`.

## Write a file

**WRITE `0x20`** (request):

| Off | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | command = `0x20` | |
| 1 | 1 | reserved | 0 |
| 2 | 2 | pathLength | |
| 4 | 4 | offset | start offset (**0 for a fresh write**; see *Resume* below) |
| 8 | 8 | currentTime | ns since epoch (file mtime) |
| 16 | 4 | totalSize | total bytes to be written |
| 20 | pathLength | path | |

**WRITE_PACING `0x21`** (response / ACK):

| Off | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | command = `0x21` | |
| 1 | 1 | status | |
| 2 | 2 | reserved | |
| 4 | 4 | offset | |
| 8 | 8 | truncatedTime | mtime as stored |
| 16 | 4 | freeSpace | bytes still expected (see below) |

**WRITE_DATA `0x22`** (request, carries data):

| Off | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | command = `0x22` | |
| 1 | 1 | status = `0x01` | |
| 2 | 2 | reserved | |
| 4 | 4 | offset | offset of this chunk |
| 8 | 4 | dataSize | bytes of data that follow |
| 12 | dataSize | data | |

Classic flow: `WRITE`, then send `WRITE_DATA` chunks in ascending order, waiting
for a `WRITE_PACING` ACK after each. `freeSpace == 0` is the terminal ACK.
`bytesAcked = totalSize − freeSpace`.

## Delete / MkDir / List / Move

**DELETE `0x30`** → `0x31`: request `{command, reserved, pathLength}` + path;
response `{command, status}`.

**MKDIR `0x40`** → `0x41`: request `{command, reserved, pathLength, reserved(4),
currentTime(8)}` + path; response `{command, status, reserved(6),
truncatedTime(8)}`.

**LISTDIR `0x50`** → `0x51`: request `{command, reserved, pathLength}` + path
(an absolute directory path, starting with `/`). One `0x51` notification per
entry, then a terminating entry with `entryNumber == totalEntries` and
`pathLength == 0`:

| Off | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | command = `0x51` | |
| 1 | 1 | status | |
| 2 | 2 | pathLength | name length that follows |
| 4 | 4 | entryNumber | 0-based |
| 8 | 4 | totalEntries | |
| 12 | 4 | flags | bit 0 = directory |
| 16 | 8 | modificationTime | ns since epoch |
| 24 | 4 | fileSize | (ignore for directories) |
| 28 | pathLength | name | entry name (not full path) |

**MOVE `0x60`** → `0x61`: request `{command, reserved, oldPathLength(2),
newPathLength(2)}` + oldPath + **one separator byte** + newPath; response
`{command, status}`. Both paths are absolute (start with `/`).

---

# Fast-transfer extensions (protocol version 5)

All of these run on the **same** `ADAF0002` characteristic — no new services or
characteristics.

## Read windowing

Instead of one chunk per round-trip, request a **large `chunkSize`** (e.g. 4096).
The watch answers a single `READ_PACING` with a **burst** of `READ_DATA`
notifications totalling up to `chunkSize`, then waits for the next pacing request.

- Each `READ_DATA` is self-describing — **reassemble by `chunkOffset`**, not
  arrival order.
- Pace on bytes actually received: the next `READ_PACING.chunkOffset` = total
  bytes received so far.
- Each notification is capped to the payload limit (see *Notification sizing*
  above); a large `chunkSize` simply arrives as several notifications.

**Auto-degrade:** a v4 watch answers a large `chunkSize` with a single packet, so
a v5 client falls back to classic pacing automatically — no version gate needed.

**Recommended window: 4096 bytes.** Larger adds little (reads are round-trip
bound).

## Write windowing (credit-based, with recovery)

Stream `WRITE_DATA` without waiting per-chunk, bounded by a credit window.

- Keep at most `WRITE_WINDOW` bytes unacked: send back-to-back while
  `bytesSent − bytesAcked < WRITE_WINDOW`; otherwise wait for an ACK.
- **`freeSpace` is a contiguous high-water-mark** (an intentional v5 refinement
  of the classic ACK — it makes loss detectable). `bytesAcked = totalSize −
  freeSpace` is the number of bytes held **with no gap** from the write's start;
  it never advances past a missing chunk. The terminal ACK (`freeSpace == 0`) is
  therefore only ever sent for a **hole-free** file.
- **Send `WRITE_DATA` in ascending offset order.**
- **Loss recovery (go-back-N).** If `bytesAcked` stops advancing while bytes are
  unacked — the window drains to `bytesAcked` with no terminal ACK — a chunk was
  lost. **Rewind `bytesSent = bytesAcked` and resume**; retransmits overwrite the
  gap. Arm a short "no ACK progress" timeout to trigger the rewind.
- **Recommended `WRITE_WINDOW`: 2048 bytes** (~10 chunks at MTU 220). Larger
  windows can *reduce* throughput (more drops → more retransmits) and don't help
  — writes are round-trip bound. Raise only with digest-verified testing.

Recommended integrity check: after a windowed write, `DIGEST` the file and
compare (below).

## DIGEST — integrity without read-back (UNA extension)

Verify a file's contents without transferring it back over BLE.

**DIGEST `0x70`** (request): `{command, reserved, pathLength}` + path.

**DIGEST_STATUS `0x71`** (response, 12 bytes):

| Off | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | command = `0x71` | |
| 1 | 1 | status | OK / ERROR_NO_FILE / ERROR |
| 2 | 2 | reserved | |
| 4 | 4 | fileSize | total bytes hashed |
| 8 | 4 | crc32 | see below |

`crc32` is **standard CRC-32** (IEEE 802.3 / zlib: reflected poly `0xEDB88320`,
init `0xFFFFFFFF`, final XOR `0xFFFFFFFF`) over the whole file — identical to
Java `java.util.zip.CRC32` and Python `zlib.crc32`. Compute the expected value
with a stock library and compare `crc32` + `fileSize`.

## Resume

A `WRITE` with `offset > 0` **preserves the existing file head** `[0, offset)` and
overwrites only from `offset` onward. So after a disconnect mid-write you may
reconnect and continue with `WRITE offset = bytesAcked` — no need to restart from
0. A fresh `WRITE offset = 0` truncates (so a full re-upload also works).

**`bytesAcked` durability.** ACKs can lead the watch's flush to storage. On a
**clean disconnect** the watch flushes before you reconnect, so `[0, bytesAcked)`
is committed and you can resume at raw `bytesAcked`. If the watch instead
**reset / lost power** mid-write, only data up to its last internal flush is
guaranteed — on reconnect, confirm the committed size (`LISTDIR` or `DIGEST`'s
`fileSize`) before resuming, and a `DIGEST` after any resumed transfer is cheap
insurance.

**Lost terminal ACK.** If the final `WRITE_PACING` is lost, your normal
stall→rewind resends the tail and the watch re-acknowledges the completed file
(`freeSpace == 0`) — the same loop concludes "done." After concluding done, drain
any leftover `WRITE_PACING` notifications before issuing the next command so a
stale one isn't mistaken for the next command's response.

## Compatibility matrix

| Client \ Watch | v4 watch | v5 watch |
|---|---|---|
| v4 client | classic | classic (v5 features simply unused) |
| v5 client | classic (windowing auto-degrades; gate writes/DIGEST off) | full fast transfer |

---

*Base protocol © Adafruit Industries (MIT), extended by UNA. Report protocol
issues on this repository.*
