# Cardputer-Adv Development Record

Date: 2026-06-02
Workspace: `/Users/Kenneth/Documents/M5 STACK`
Device: `M5Stack Cardputer-Adv`

## 1. Objective

This workstream explored three main directions on Cardputer-Adv:

- `HEAT`: Wi-Fi/BLE based area heat sensing
- `REMOTE`: IR remote control, validated first against a Samsung TV
- `EzData`: cloud upload path over Wi-Fi, with later discussion about LoRa and Cat.1

The practical goal was to turn the device into a small handheld retail/IoT tool with:

- on-device UI
- local scanning and browsing
- IR remote capability
- optional cloud upload

## 2. Main deliverables completed in local source

The local source tree currently contains a complete custom firmware codebase, including:

- `HEAT` module:
  - Wi-Fi scan
  - BLE scan
  - RSSI filtering
  - smoothed heat score
  - `SUMMARY / WIFI / BLE / LOG / SESSION / FILES / SESSION INFO`
  - local session logging
  - MicroSD CSV support
- `REMOTE` module:
  - working IR sender path
  - Samsung TV profile confirmed usable
  - fallback Samsung profiles retained
- browser preview and UI planning docs:
  - `preview/`
  - `CARDPUTER_ADV_UI_V1.md`
  - `CARDPUTER_ADV_V1_PLAN.md`
- EzData cloud upload experiments and notes:
  - `EZDATA2_FIELD_MAP.md`
  - `src/heat_cloud_config*.h`

## 3. Custom firmware status

### 3.1 HEAT

The custom firmware achieved a usable `HEAT` experience locally:

- menu-based UI
- readable list layout
- record browsing
- 1-minute scan interval
- 60-log rolling history
- near-device filtering
- trend and level display

### 3.2 REMOTE

The IR path was validated successfully:

- ADV IR LED emission confirmed
- Samsung TV responded successfully
- navigation was remapped from `WSAD` to keycap-aligned combinations:
  - `Fn + ;` = Up
  - `Fn + .` = Down
  - `Fn + ,` = Left
  - `Fn + /` = Right

### 3.3 EzData experiments

The EzData work produced useful findings but did **not** end in a stable production-grade integration.

Observed issues included:

- token mismatch between:
  - `Devices` page token
  - `registerMac` response token
  - runtime MQTT token observed on device
- `MQ5` unauthorized responses during custom MQTT login
- inconsistent behavior between runtime-success token tails and My M5Stack group binding
- `application domain not found` when trying to bind some runtime tokens in `Data`

This indicates the direct custom MQTT path was not fully aligned with the official M5 cloud identity flow.

## 4. UIFlow / official cloud path findings

We later tested the official `UiFlow2` route as a more canonical path for EzData.

Key findings:

- A device may show `Wi-Fi connected successfully` on-device while WebIDE still defaults to `USB`.
- In `UiFlow2`, the correct cloud device must be selected from the online device list, not the bottom `USB device` template row.
- `Private / Public / Token Required` belongs to device sharing/access control and is not the same issue as custom MQTT `MQ5`.
- The official docs indicate EzData should be used through `UiFlow2 WebIDE -> EzData Manager -> Add key`.

However, later testing with a flashed `UiFlow2` firmware led to a bad runtime state:

- the device could boot
- but the system felt unresponsive / stuck
- the user judged the firmware as unusable

Because of that, the workflow was stopped and the device was restored.

## 5. Restore / safety status

The device backup used as the user-defined original system baseline is:

- `/Users/Kenneth/Documents/M5 STACK/backups/cardputer-adv-flash-20260531-162550-8mb.bin`

This image was restored successfully multiple times.

Current confirmed state:

- device has been flashed back to the user-defined original baseline
- user confirmed the device returned to the familiar original system state
- local source files were not affected by reflash operations

## 6. Important file references

- Main firmware source:
  - `/Users/Kenneth/Documents/M5 STACK/src/main.cpp`
- Cloud config:
  - `/Users/Kenneth/Documents/M5 STACK/src/heat_cloud_config.h`
  - `/Users/Kenneth/Documents/M5 STACK/src/heat_cloud_config.local.h`
  - `/Users/Kenneth/Documents/M5 STACK/src/heat_cloud_config.local.example.h`
- Firmware project config:
  - `/Users/Kenneth/Documents/M5 STACK/platformio.ini`
- Main readme:
  - `/Users/Kenneth/Documents/M5 STACK/README.md`
- EzData notes:
  - `/Users/Kenneth/Documents/M5 STACK/EZDATA2_FIELD_MAP.md`
- UI preview:
  - `/Users/Kenneth/Documents/M5 STACK/preview/index.html`
  - `/Users/Kenneth/Documents/M5 STACK/preview/styles.css`
  - `/Users/Kenneth/Documents/M5 STACK/preview/app.js`

## 7. Repo / source control note

This folder is a Git repository, but it currently has:

- no commits yet
- all project files present only as local working tree files

That means:

- source code exists locally
- but there is no Git history checkpoint yet

## 8. Current conclusion

At the end of this round:

- local custom firmware source is preserved
- original device system baseline is restored
- `HEAT` and `REMOTE` custom work is available in source code
- the direct custom EzData MQTT route is not considered stable enough yet
- official `UiFlow2 + EzData Manager` is still the recommended next verification path if EzData must remain the target
