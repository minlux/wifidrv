# WiFi-Drive — A WiFi-backed USB Picture Frame Dongle

## Introduction

Digital picture frames are simple, self-contained devices: they read JPEG images
from a USB stick and display them in a slideshow. The content is static — whatever
photos are on the stick when you plug it in are the photos you get. Updating the
slideshow means physically swapping the stick.

**wifidrv removes that limitation.** Instead of a regular USB stick you plug in
this dongle. To the picture frame it looks identical — a standard USB mass storage
device with JPEG files on it. But behind the scenes the dongle is connected to your
WiFi network and fetches its images from a web server. The picture frame has no idea
it is talking to anything other than a stick; it just reads files and displays them.

This makes the picture frame effectively **cloud connected**: the web server decides
what image to show next. It could serve a fixed set of holiday photos, pull images
from an API, display a daily calendar entry, show a weather map, or rotate through
anything else that can be expressed as a JPEG. No firmware changes to the picture
frame, no new app, no screen mirroring — the existing USB slot is all that is needed.

## Concept

wifidrv turns a **LilyGO T-Dongle S3** (ESP32-S3) into a self-contained wireless
picture frame adapter. The device appears to any host computer as a small, read-only
**USB Mass Storage stick** (FAT16). The stick contains two JPEG image files. Whenever
the host reads the last sector of an image file, the device fetches a fresh image from
a configurable HTTP server and makes it available for the next read — without the host
ever noticing anything other than a normal USB drive.

## Hardware

- **Board:** LilyGO T-Dongle S3 (`ESP32-S3`, USB-OTG, APA102 status LED)
- **Interface to host:** USB Mass Storage (MSC) via TinyUSB / ESP32 Arduino core
- **Connectivity:** 802.11 WiFi (ESP32-S3 built-in)

## Virtual FAT16 Drive Layout

The USB drive is not backed by real flash storage. All filesystem structures
(MBR, VBR, FAT, root directory) are pre-built binary blobs compiled into flash.
The file data is served from RAM buffers at runtime.

| LBA range   | Content                              |
|-------------|--------------------------------------|
| 0           | MBR                                  |
| 2048        | VBR (Volume Boot Record)             |
| 2052–2083   | FAT #1 (32 sectors)                  |
| 2084–2115   | FAT #2 (32 sectors)                  |
| 2116        | Root Directory                       |
| 2152–2155   | `CREDS.JSN` — credentials (2 KB RAM) |
| 2156–2411   | `IMG1.JPG` — image slot (128 KB RAM) |
| 2412–2667   | `IMG2.JPG` — image slot (128 KB RAM) |

Both image slots serve the same RAM buffer. `CREDS.JSN` is a JSON file that
reflects the currently stored WiFi credentials and is visible on the USB drive.

Read more about that in [delopment.md](development.md).

## Boot Sequence

1. Serial CLI initialises, loads credentials from NVS, writes them to `CREDS.JSN`.
2. USB MSC starts; the host sees a read-only 32 MB stick.
3. Device connects to WiFi using stored SSID and password.

## Image Refresh Trigger

When the host reads **LBA 2411** (last sector of IMG1.JPG) or **LBA 2667**
(last sector of IMG2.JPG), the USB MSC callback sets a trigger flag.
The main loop detects the flag and performs an HTTP GET to the configured URL.
The response body (≤ 128 KB JPEG) is written into the RAM buffer. On the next
read cycle the host receives the newly fetched image.

## Serial CLI

The device exposes a USB CDC serial port (115200 baud) with a line-based CLI:

```
set ssid      <value>   — store WiFi SSID
set password  <value>   — store WiFi password (triggers reconnect)
set url       <value>   — store image server URL
get ssid|password|url  — read stored value
get wifi               — show current WiFi connection status and IP
```

All values persist across reboots via **ESP32 NVS** (Non-Volatile Storage).

Read more about that in [provisioning.md](provisioning.md)

## Source Layout

| File | Responsibility |
|---|---|
| `src/main.cpp` | Setup/loop, WiFi connect/status |
| `src/usb_msc.cpp` | USB MSC callbacks, LED feedback, fetch trigger |
| `src/storage.c` | Virtual FAT16 — LBA → RAM buffer mapping |
| `src/credentials.cpp` | NVS read/write, CREDS.JSN refresh |
| `src/cli.cpp` | Serial line buffer, command dispatch |
| `src/http_client.cpp` | HTTP GET, image buffer fill, trigger state |
| `src/mbr.c` / `vbr.c` / `fat.c` / `rootdir.c` | Pre-built FAT16 structures (flash) |
| `src/img1_jpg.c` / `img2_jpg.c` | Fallback images compiled into flash |

## Companion Tools

### `webserver/server.py`

A dependency-free Python HTTP server that serves images from `webserver/assets/`.
The `/picture-frame` endpoint returns a randomly selected image on each request.

```bash
python3 webserver/server.py --port 8080
```

### `webserver/prepare_assets.sh`

An ImageMagick bash script that batch-converts images in `webserver/assets/` to
800×600 JPEG at quality 65, center-cropping to fit. Keeps files well under the
128 KB limit imposed by the device's RAM buffer.

```bash
./webserver/prepare_assets.sh
```

## Build & Flash

```bash
source .env          # make pio available
pio run              # build
pio run -t upload    # flash
pio device monitor   # open serial CLI
```
