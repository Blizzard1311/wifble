# Cardputer-Adv Handoff

Date: 2026-06-02
Workspace root: `/Users/Kenneth/Documents/M5 STACK`
App path: `/Users/Kenneth/Documents/M5 STACK/apps/heat-remote`

## Current state

### Device

The physical Cardputer-Adv has been restored to the user-defined original baseline:

- backup image:
  `/Users/Kenneth/Documents/M5 STACK/device/backups/cardputer-adv-flash-20260531-162550-8mb.bin`

The user confirmed the device is back to the expected original state and local code was not affected.

### Local source

The custom project source is still present locally and includes:

- full `HEAT` firmware implementation
- working `REMOTE` implementation with Samsung support
- cloud upload experiments for EzData
- preview UI assets and planning docs

Primary source entrypoint:

- `/Users/Kenneth/Documents/M5 STACK/apps/heat-remote/src/main.cpp`

## What was validated successfully

### 1. HEAT path

Confirmed in custom firmware:

- Wi-Fi scan
- BLE scan
- heat score calculation
- trend/level display
- readable list UI
- local browsing and logging flow

### 2. REMOTE path

Confirmed in custom firmware:

- IR emission works
- Samsung TV responds
- key mapping is comfortable using:
  - `Fn + ;`
  - `Fn + .`
  - `Fn + ,`
  - `Fn + /`

### 3. Restore path

Confirmed repeatedly:

- full 8MB backup readback existed
- reflash from backup succeeded
- user-defined original system was recoverable

## What remains unresolved

### EzData cloud path

This is the key unresolved area.

The custom direct MQTT path encountered identity inconsistency across:

- `Devices` page token
- `registerMac` token
- runtime token observed on device
- token binding in `my.m5stack.com -> Data`

Symptoms seen during testing included:

- `MQ5` unauthorized
- `application domain not found`
- runtime token tails not consistently matching web-side accepted token behavior

Conclusion:

- the direct custom EzData MQTT route should currently be treated as experimental
- do not treat it as a stable final integration yet

## Recommended next step

If the next agent continues the EzData work, the recommended route is:

### Step 1: do not start from the custom MQTT code

Instead, validate the official M5 flow first:

- flash a known-good official `UiFlow2` firmware for `Cardputer Adv`
- confirm the device can be selected as a true `Cloud` device in `UiFlow2`
- use `UiFlow2 WebIDE -> EzData Manager`
- create a simple key with `Add key`
- confirm official cloud-side data creation works

### Step 2: only after the official flow is stable

Reconnect the custom `HEAT` logic to the official EzData identity model.

That means:

- treat the official EzData Manager token as the only authoritative token
- do not continue mixing:
  - `Devices` page token
  - dynamic runtime token tails
  - custom fallback tokens

## Important caution

There was a later test where `UiFlow2` firmware:

- booted
- connected Wi-Fi
- but felt unresponsive / frozen in normal operation

So before any future EzData validation, confirm the chosen `UiFlow2` build is:

- actually stable on Cardputer-Adv
- not just bootable

If instability appears again, stop cloud testing and restore the original baseline before continuing.

## Files the next agent should read first

1. `/Users/Kenneth/Documents/M5 STACK/README.md`
2. `/Users/Kenneth/Documents/M5 STACK/apps/heat-remote/README.md`
3. `/Users/Kenneth/Documents/M5 STACK/apps/heat-remote/DEVELOPMENT_RECORD_2026-06-02.md`
4. `/Users/Kenneth/Documents/M5 STACK/apps/heat-remote/EZDATA2_FIELD_MAP.md`
5. `/Users/Kenneth/Documents/M5 STACK/apps/heat-remote/src/main.cpp`

## Practical recommendation

If the next step is not specifically EzData, do **not** flash anything immediately.

Safer order:

1. keep the current restored device untouched
2. review local source
3. decide whether to:
   - continue custom `HEAT / REMOTE` development, or
   - do a clean official `UiFlow2 + EzData Manager` validation pass

## User expectations to preserve

- dialogue in Simplified Chinese
- UI text on device should prefer English
- when user says `恢复原系统` or similar, default to the backup image above
- user values source-preserving safety over risky firmware experimentation once instability appears
