# Cardputer-Adv Heat Remote

This app lives under:

- `/Users/Kenneth/Documents/M5 STACK/apps/heat-remote`

The overall Cardputer-Adv workspace root is:

- `/Users/Kenneth/Documents/M5 STACK`

## Detected device

- Port: `/dev/cu.usbmodem2101`
- USB ID: `303A:1001`
- Description: `USB JTAG/serial debug unit`

## Local toolchain

- Python virtualenv: `/Users/Kenneth/Documents/M5 STACK/.venv`
- Serial tools: `pyserial`
- Build/flash tools: `platformio`

## Commands

Activate the local environment:

```bash
cd "/Users/Kenneth/Documents/M5 STACK/apps/heat-remote"
source ../../.venv/bin/activate
```

List serial devices:

```bash
../../.venv/bin/pio device list
```

Build firmware:

```bash
../../.venv/bin/pio run
```

Flash firmware:

```bash
../../.venv/bin/pio run -t upload
```

Open serial monitor:

```bash
../../.venv/bin/pio device monitor
```

## Current firmware prototype

The current `src/main.cpp` is a `launcher + Wi-Fi/BLE heat scanner + IR remote`
prototype with an English on-device UI.

### UI structure

- `HOME`
  - `HEAT`
  - `REMOTE`
  - `CODEX`
- `HEAT`
  - `SUMMARY`
  - `WIFI`
  - `BLE`
  - `LOG`
  - `SESSION`
  - `FILES`
  - `SESSION INFO`

Implemented working modules are `HEAT` and `REMOTE`. `CODEX` remains a
placeholder entry.

### Keyboard controls

On `HOME`:

- `Fn + ;`: move up
- `Fn + .`: move down
- `Fn + ,`: move left / previous
- `Fn + /`: move right / next
- `Enter`: open selected function

On `HEAT`:

- `Fn + ,` / `Fn + /`: previous / next technical page
- `Fn + ;` / `Fn + .`: browse records on list pages
- `Enter`:
  - on `SUMMARY / WIFI / BLE / LOG`: run a manual scan cycle
  - on `SESSION`: retry SD initialization and auto logging
  - on `FILES`: open the total CSV summary
- `Tab`:
  - on `FILES`: refresh total file status
  - on other `HEAT` pages: refresh immediately
- `Backspace`: return to `HOME`

On `SESSION INFO`:

- no auto refresh is triggered while viewing the saved summary
- `Backspace`: return to `FILES`

On `REMOTE`:

- `Fn + ,` / `Fn + /`: previous / next brand profile
- `Fn + ;` / `Fn + .`: browse commands inside current profile
- `Enter`: send current IR command
- `Tab`: resend current IR command
- `Backspace`: return to `HOME`

Detailed UI definition is captured in:

- [CARDPUTER_ADV_UI_V1.md](/Users/Kenneth/Documents/M5%20STACK/apps/heat-remote/CARDPUTER_ADV_UI_V1.md)

### Current behavior

- scans `Wi-Fi` first and `BLE` second
- refreshes automatically every `1 minute` while inside `HEAT`
- stores the last `60` scan snapshots in local memory
- can write all `HEAT` data into a single `CSV` on the MicroSD card
- uses `/heatlogs/heat_all.csv` as the total log file
- creates an automatic boot session when SD is available
- retries SD card initialization when opening `SESSION` / `FILES` later
- filters nearby signals with thresholds:
  - `Wi-Fi >= -82 dBm`
  - `BLE >= -88 dBm`
- allows scrolling through `WIFI`, `BLE`, `LOG`, and the total file summary
- computes a smoothed `HEAT SCORE` and trend
- shows strongest nearby entries sorted by RSSI
- can optionally upload `HEAT` summary data to M5Stack `EzData2`
- supports a `regional TV test pack v1` in `REMOTE`:
  - `SAMSUNG` (primary usable profile)
  - `SSG ALT` (Samsung backup)
  - `SSG LEG` (Samsung legacy/MSB backup)
  - `TCL`
  - `HISENSE`
  - `HAIER`
  - `XIAOMI`

This version is intended as a practical first-stage signal scanner and IR test
tool, not an exact people counter or a finalized universal remote.

## SD logging model

- all saved scan rows are appended into one file:
  - `/heatlogs/heat_all.csv`
- the firmware no longer creates one CSV per manual recording
- each device boot starts one automatic logical session
- each CSV row includes:
  - `session_name`
  - `session_start`
  - `time`
  - `heat`
  - `raw`
  - `raw_uncapped`
  - `level`
  - `trend`
  - `wifi_kept`
  - `wifi_total`
  - `ble_kept`
  - `ble_total`
- on-device UI still uses the capped `0..99` heat model for readability
- `raw_uncapped` keeps the original unbounded density score for later analysis
- `session_name` uses a persistent boot sequence:
  - example: `boot_000001`
  - the sequence is stored persistently in device NVS
  - each new boot session increments the number even after power cycling

## Planned 4G route

- the current firmware includes a `CAT1` debug page for `Unit Cat1-CN`
- the current field route is:
  - local `SD` logging stays as the primary record
  - `CAT1` can POST the current `HEAT` snapshot as JSON over `4G`
- the current upload cadence is:
  - one automatic `CAT1` upload every `15 minutes`
  - the first automatic upload also waits for the first `15-minute` interval instead of uploading immediately at boot
  - background scan/upload continues regardless of whether the UI is left on `HEAT`, `CAT1`, or `HOME`
  - the `15-minute` cadence is based on the last successful upload; if a scheduled upload fails, later background scans continue retrying instead of waiting a full extra `15 minutes`
  - one immediate extra upload if the current `heat` value changes by more than `50`
  - manual upload from the `CAT1` page still works with `Enter`
- if only a `Grove` cable is available, the intended prototype wiring is:
  - `ADV G2 -> Cat1 UART_RX`
  - `ADV G1 -> Cat1 UART_TX`
  - code side should use:
    - `TX = G2`
    - `RX = G1`
- the current CAT1 JSON payload matches the core CSV schema:
  - `device_id`
  - `session_name`
  - `session_start`
  - `time`
  - `heat`
  - `raw`
  - `raw_uncapped`
  - `level`
  - `trend`
  - `wifi_kept`
  - `wifi_total`
  - `ble_kept`
  - `ble_total`
- the current web dashboard uses simplified Chinese labels for:
  - `热力值` (`heat`)
  - `原始热力(未封顶)` (`raw_uncapped`)
  - `近场 Wi-Fi 数` (`wifi_kept`)
- see the detailed wiring and validation guide:
- [ADV_GROVE_CAT1_CN_GUIDE.md](/Users/Kenneth/Documents/M5%20STACK/apps/heat-remote/ADV_GROVE_CAT1_CN_GUIDE.md)

## Optional EzData2 cloud upload

The firmware can optionally upload `HEAT` summary data to M5Stack's official
`EzData2` service over `Wi-Fi + MQTT`.

Create a local config file:

- copy [src/heat_cloud_config.local.example.h](/Users/Kenneth/Documents/M5%20STACK/apps/heat-remote/src/heat_cloud_config.local.example.h)
  to `src/heat_cloud_config.local.h`
- fill in:
  - `HEAT_CLOUD_WIFI_SSID`
  - `HEAT_CLOUD_WIFI_PASSWORD`
  - `HEAT_CLOUD_EZDATA_TOKEN` (`EzData2` device token)
  - optional `HEAT_CLOUD_TOPIC_PREFIX`

This local file is ignored by git on purpose.

Current upload behavior:

- current minimal verification mode uploads only one field:
  - `adv_heat_heat`
- after each completed `HEAT` scan, the device uploads the current `HEAT SCORE`
- on `SESSION` stop, it reuses the same single-field upload path
- this reduced mode is intentional to verify that `ADV -> Wi-Fi -> EzData`
  is stable before adding more fields back

Notes:

- this implementation targets `EzData2` device tokens and uses the official
  EzData2 MQTT protocol
- the current firmware is configured in fixed-token mode first:
  - it uses `HEAT_CLOUD_EZDATA_TOKEN` as the primary runtime token
  - this keeps the on-device cloud status tail aligned with the token you bind
    in `my.m5stack.com`
- `registerMac` is only used as a fallback when no token is configured
- data is uploaded as named fields, not as full CSV text
- local `microSD` logging remains the full-fidelity storage path
- `SUMMARY` and `SESSION` pages display a compact cloud status field:
  `OFF / JOIN / READY / SEND / OK / ERR`
- the last 6 characters of the active `deviceToken` are shown after the cloud
  state when available
- the active runtime token shown on the device is the authoritative token hint;
  it may differ from the token shown on the `Devices` page in `my.m5stack.com`

Field and status reference:

- [EZDATA2_FIELD_MAP.md](/Users/Kenneth/Documents/M5%20STACK/apps/heat-remote/EZDATA2_FIELD_MAP.md)
- note: the current firmware may upload only the `heat` field during
  single-field verification mode, even though the full field map documents the
  intended expanded schema

My M5Stack viewing note:

- in the current web UI, left sidebar `Data` is the `EzData2` workflow page
- the URL is typically:
  - `https://my.m5stack.com/ezdata2/workflow`
- if the device already shows `C OK xxxxxx`, focus on the `Data` workspace,
  not the old `EzData debugger` page

## Local preview

A browser mock is available for UI review before flashing:

- [preview/index.html](/Users/Kenneth/Documents/M5%20STACK/apps/heat-remote/preview/index.html)

Serve the folder locally, then open the page in a browser:

```bash
cd "/Users/Kenneth/Documents/M5 STACK/apps/heat-remote/preview"
python3 -m http.server 4173
```

Then visit:

```text
http://127.0.0.1:4173
```

## Download mode

According to M5Stack's official Cardputer-Adv docs, to enter download mode:

1. Set the side power switch to `OFF`.
2. Hold the `G0` button.
3. Connect USB-C power/data.
4. Release `G0` after power is applied.

## Notes

- `platformio.ini` is preconfigured with the official M5Stack PlatformIO settings for Cardputer-Adv.
- If macOS assigns a different serial port after reconnecting, update `upload_port` and `monitor_port` in `platformio.ini`.
