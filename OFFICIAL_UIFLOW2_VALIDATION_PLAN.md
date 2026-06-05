# Official UiFlow2 Validation Plan

Date: 2026-06-02
Workspace: `/Users/Kenneth/Documents/M5 STACK`
Device: `M5Stack Cardputer-Adv`

## Goal

Run one clean validation pass using only the official `UiFlow2` path.

This validation must **not** mix in the custom firmware developed in this workspace.

The only objectives are:

1. verify a stable official `UiFlow2` firmware on `Cardputer-Adv`
2. verify `Cloud` device connection in `UiFlow2 WebIDE`
3. verify `EzData Manager`
4. verify `Add key`
5. verify the official device token used by that flow

Only after all five steps succeed should the custom `HEAT` firmware be reconnected to cloud work.

## Scope boundary

### In scope

- official `UiFlow2` firmware selection
- official `M5Burner` flash/config flow
- on-device usability after flash
- `UiFlow2 WebIDE` cloud device selection
- `EzData Manager` entry
- `Add key`
- official token visibility

### Out of scope

- custom `HEAT` firmware
- custom MQTT / `registerMac` experiments
- custom EzData field uploads
- LoRa
- Cat.1
- Samsung remote

## Current known state

At the time this plan was written:

- custom firmware source is preserved locally
- the device has been restored to the user-defined original baseline
- at least one attempted `UiFlow2` flash booted but felt unresponsive / frozen
- therefore the first blocker is not `EzData`, but whether the official `UiFlow2` build is actually usable on this device

## Validation sequence

### Phase 1: Firmware sanity

Target:

- find an official `UiFlow2` firmware build for `Cardputer-Adv` that is not just bootable, but operable

Checks:

1. device powers on normally
2. keyboard input works
3. menu navigation works
4. no apparent freeze / dead UI state
5. Wi-Fi configuration can be entered and saved

Exit condition:

- device can be used normally without freezing

Failure condition:

- if `UiFlow2` still behaves like a dead/frozen system, stop cloud validation and revert to the known original baseline

### Phase 2: Official cloud device visibility

Target:

- confirm the device appears as an online `Cloud` device in `UiFlow2 WebIDE`

Checks:

1. device is connected to Wi-Fi
2. same account is used in `M5Burner` and `UiFlow2`
3. `Select Device` shows the online device
4. the online cloud device is selected, not the `USB` template row
5. WebIDE enters `Cloud` path instead of only `USB`

Exit condition:

- cloud device is selectable and enters the cloud session path

### Phase 3: EzData Manager

Target:

- verify the official `EzData Manager` path exists and works

Checks:

1. locate the `EzData` entry in the official WebIDE flow
2. open `EzData Manager`
3. select the device inside `EzData Manager`
4. confirm the official token shown there

Exit condition:

- `EzData Manager` opens successfully and shows the token for this device

### Phase 4: Add key

Target:

- create the first official cloud-side data item using the M5 flow

Recommended first key:

- `heat`

Checks:

1. click `Add key`
2. create one simple key-value item
3. confirm it is stored successfully
4. confirm it can be read back through the official flow

Exit condition:

- at least one cloud item exists and is readable through the official path

## Evidence to capture

For each phase, capture lightweight evidence:

- photo of device screen
- screenshot of `UiFlow2`
- exact firmware name/version if visible
- whether the device was in:
  - `USB`
  - `Cloud`

Most important evidence:

1. the exact official `UiFlow2` firmware name/version that is stable
2. the exact official token shown by `EzData Manager`
3. proof that `Add key` succeeds

## Decision gate before resuming custom firmware work

Do **not** reconnect the custom `HEAT` firmware to EzData until all of the following are true:

- stable official `UiFlow2` firmware found
- cloud device connection works
- `EzData Manager` works
- `Add key` works
- official token is known

If any of those fail, the correct conclusion is:

- official path is not stable yet
- custom EzData reintegration should remain blocked

## Next step after this plan succeeds

Only after this official validation succeeds:

1. preserve the official token from `EzData Manager`
2. return to the custom firmware source in this workspace
3. reconnect custom `HEAT` upload using only that official token
4. retest with the smallest possible cloud payload first

## Related files

- `/Users/Kenneth/Documents/M5 STACK/DEVELOPMENT_RECORD_2026-06-02.md`
- `/Users/Kenneth/Documents/M5 STACK/NEXT_HANDOFF_2026-06-02.md`
- `/Users/Kenneth/Documents/M5 STACK/README.md`
