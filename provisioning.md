# Device Provisioning

Provisioning is done over the **USB CDC serial interface** using the built-in CLI.
No special app or driver is required — any serial terminal works.

## Connect to the Serial Interface

Plug the dongle into a USB port. A serial device appears alongside the USB drive:

```bash
# Linux / macOS
pio device monitor          # via PlatformIO

# or directly
screen /dev/ttyACM0 115200
minicom -D /dev/ttyACM0 -b 115200
```

On Windows use PuTTY or the Arduino Serial Monitor on the COM port assigned by
Device Manager.

When the connection is established the device greets you:

```
CLI ready.
Commands:
 - set <ssid|password|url> <value>
 - get <ssid|password|url|wifi>
```

## Set Credentials

Enter the three required values one by one:

```
set ssid       YourNetworkName
set password   YourPassword
set url        http://192.168.1.100:8080/picture-frame/
```

Values are written to **NVS (Non-Volatile Storage)** immediately and survive
power cycles. The `CREDS.JSN` file on the USB drive is updated at the same time.

## Verify WiFi Connection

After setting SSID and password the device connects automatically. Query the
current status with:

```
get wifi
```

Expected output when connected:

```
WiFi: connected, IP 192.168.1.42
```

If the connection is still in progress or the credentials are wrong:

```
WiFi: not connected (status 1)
```

In that case double-check SSID and password with `get ssid` / `get password`
and re-enter them with `set`.

## Read Back Stored Values

```
get ssid
get password
get url
```

## Verify via USB Drive — CREDS.JSN

The USB drive exposed by the device contains a file called **`CREDS.JSN`**.
Its content is a JSON object that always reflects the currently stored credentials:

```json
{"ssid":"YourNetworkName","password":"YourPassword","url":"http://192.168.1.100:8080/picture-frame/"}
```

Open this file in any text editor or with `cat` to confirm that provisioning
was successful without needing an active serial connection:

```bash
cat /media/$USER/WIFIDRV/CREDS.JSN
```

## Full Provisioning Example

```
set ssid       Gammablitz
set password   12345678
set url        http://192.168.178.33:8080/picture-frame/
get wifi
WiFi: connected, IP 192.168.178.42
get url
url = "http://192.168.178.33:8080/picture-frame/"
```

## Notes

- All three values (`ssid`, `password`, `url`) must be set for the device to
  function correctly. They persist across reboots — provisioning is a one-time
  step unless the network changes.
- Setting `ssid` or `password` triggers an immediate reconnect attempt.
- The `url` must point to an HTTP endpoint that returns a JPEG image of at most
  128 KB. Use `webserver/prepare_assets.sh` to pre-process images to the correct
  size before serving them.
