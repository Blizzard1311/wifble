# Cardputer-Adv V1 Plan

## Goal

Build a practical `Cardputer-Adv` handheld focused on three functions:

1. `Area heat sensing`
2. `Universal IR remote`
3. `Codex mobile console`

This V1 does **not** target:

- precise customer counting
- shelf/SKU-level sell-through measurement
- replenishment calculation
- RFID or camera-based product recognition

The product target is a portable edge terminal, not a full retail analytics engine.

## Product Positioning

`Cardputer-Adv` should be treated as:

- a portable signal-sensing node
- an IR control device
- a lightweight terminal for remote Codex usage

It should **not** be treated as:

- a local AI workstation
- a precise people-counter
- a shelf sensor replacement

## V1 Scope

### 1. Area Heat Sensing

Use passive `Wi-Fi` and `BLE` scanning to estimate area heat in a broad sense.

V1 output:

- current heat score
- last 60 minutes heat trend
- peak time periods
- relative area ranking
- device scan status

V1 non-goals:

- exact headcount
- unique visitor count
- individual customer tracking
- cross-store identity tracking

Recommended wording in product/UI:

- use `heat`, `signal density`, `area popularity`, `activity index`
- avoid `exact people count`

### 2. Universal IR Remote

Use the built-in IR emitter to control common store devices:

- AC
- TV
- advertising display
- projector
- fan

V1 output:

- preset remote profiles
- custom code library
- favorite actions
- room/device grouping

V1 non-goals:

- full learning mode without extra receiver hardware
- non-IR protocols like 433MHz, Zigbee, or proprietary BLE controls

### 3. Codex Mobile Console

Use the device as a remote terminal to connect to:

- local Mac
- home server
- cloud VM

V1 output:

- quick-connect host list
- project shortcuts
- preset command launcher
- last task status
- recent output view

V1 non-goals:

- local code compilation for real projects
- full IDE workflow on-device
- large multi-pane code editing

## Recommended Priority

Implementation order:

1. `IR remote`
2. `Area heat sensing`
3. `Codex mobile console`

Reason:

- `IR remote` is the fastest path to a stable daily-use feature.
- `Heat sensing` is the strongest business-fit feature for the current direction.
- `Codex console` is useful, but mostly personal productivity rather than external product value.

## V1 Screens

### Home

Show three primary entries:

- `Heat`
- `Remote`
- `Codex`

Show small status badges:

- battery
- Wi-Fi state
- last sync time

### Heat Screen

Show:

- current heat score
- Wi-Fi detections
- BLE detections
- current scan state
- last upload result

Actions:

- start scan
- stop scan
- upload now
- switch area
- view trend

### Heat Trend Screen

Show:

- 5-minute bucket heat trend
- strongest and weakest periods
- area comparison

### Remote Screen

Show:

- room/device selector
- preset action buttons
- favorites

Example actions:

- AC on/off
- temperature up/down
- TV power
- input switch
- display mute

### Remote Library Screen

Show:

- device type
- brand/model
- code set
- test button

### Codex Screen

Show:

- saved hosts
- saved workspaces
- preset commands
- recent sessions

Actions:

- connect
- run preset
- view output
- reconnect

## Firmware Architecture

Use modular architecture instead of one large sketch.

Suggested modules:

- `app_main`
- `ui_manager`
- `input_manager`
- `wifi_ble_scanner`
- `heat_aggregator`
- `ir_remote_manager`
- `codex_terminal_client`
- `storage_manager`
- `network_sync`
- `settings_manager`

### Core responsibilities

`ui_manager`

- screen routing
- drawing widgets
- handling status bars

`input_manager`

- keyboard input
- shortcuts
- menu navigation

`wifi_ble_scanner`

- periodic Wi-Fi scan
- BLE scan window scheduling
- RSSI filtering
- duplicate suppression in time windows

`heat_aggregator`

- convert scan events into short-window heat score
- bucket trend data
- keep rolling local cache

`ir_remote_manager`

- code library loading
- action mapping
- IR send pipeline

`codex_terminal_client`

- SSH or WebSocket client bridge
- preset command execution
- stream output rendering

`storage_manager`

- save settings
- save remote profiles
- save recent heat buckets

`network_sync`

- upload heat summaries
- fetch config
- retry failed sync

## Heat Sensing Logic

### What to measure

Use both:

- `Wi-Fi scan detections`
- `BLE advertisement detections`

For each event store only short-lived anonymous data:

- protocol type
- hashed temporary identifier
- RSSI
- timestamp
- area ID

Do not store raw MAC addresses long-term in V1.

### Recommended V1 algorithm

Per 1-minute window:

1. collect Wi-Fi and BLE detections
2. filter weak RSSI values below threshold
3. de-duplicate within a rolling time window
4. compute weighted heat score

Example score model:

```text
heat_score =
  wifi_unique_nearby * 1.0 +
  ble_unique_nearby * 0.7 +
  dwell_bonus * 0.5
```

Suggested practical filters:

- Wi-Fi RSSI threshold: around `-75 dBm`
- BLE RSSI threshold: around `-80 dBm`
- duplicate merge window: `2-5 minutes`

### Output model

Do not expose raw counts as guaranteed people counts.

Expose:

- `heat_score`
- `heat_level`: low / medium / high
- `trend_delta`
- `peak_window`

### Multi-area deployment

For real store use, one device is not enough for full-floor heat mapping.

Recommended deployment models:

- `single-device mode`: local hotspot or one zone trend only
- `multi-node mode`: multiple Adv or ESP32 nodes per zone
- `gateway mode`: one device aggregates summaries from area nodes

## IR Remote Logic

### V1 approach

Start with a code-library sender.

Required data fields per action:

- device ID
- room ID
- protocol
- address
- command
- repeat count

Suggested categories:

- AC
- TV
- display
- projector
- fan

### V1 user flow

1. choose room
2. choose device
3. choose action
4. send IR
5. optionally favorite the action

### V2 extension

If needed later, add:

- external IR receiver
- learn mode
- import/export code library

## Codex Mobile Console Logic

### Recommended approach

Do not try to run the full coding workflow locally on-device.

Instead:

- connect to a prepared host
- launch preset commands
- stream compact output
- allow simple text input

### Good V1 use cases

- open a project session
- run `git status`
- run tests
- tail logs
- restart dev server
- view last task result

### Bad V1 use cases

- editing large code files
- complex multi-step merges
- full browser-based local development

### Connectivity options

Best order:

1. `SSH-based terminal access`
2. `simple WebSocket bridge to a host-side helper`

SSH is simpler and more standard.

## Data and Config

Suggested local config objects:

- device settings
- Wi-Fi credentials
- area profile
- heat scan policy
- IR library
- host list
- command presets

Suggested remote sync payloads:

- timestamp
- device ID
- area ID
- heat score
- wifi count summary
- ble count summary
- battery state
- firmware version

## Milestones

### M1. Base Shell

Deliver:

- boot screen
- home screen
- settings storage
- keyboard navigation

### M2. IR Remote

Deliver:

- remote UI
- preset code library
- send/test actions
- favorites

This is the best first milestone for quick success.

### M3. Heat Sensing

Deliver:

- Wi-Fi scan
- BLE scan
- heat score
- 60-minute trend cache
- simple upload API

### M4. Codex Console

Deliver:

- host list
- SSH connect
- preset command launcher
- recent output panel

## Recommended Build Strategy

If the goal is fastest delivery:

1. build `IR remote` first
2. build `heat sensing` second
3. build `Codex console` third

If the goal is strongest business validation:

1. build `heat sensing` first
2. build `IR remote` second
3. build `Codex console` third

My recommendation remains:

1. `IR remote`
2. `Heat sensing`
3. `Codex console`

## Hardware Notes

Use built-in hardware first.

Built-in for V1:

- display
- keyboard
- Wi-Fi
- BLE
- IR emitter
- battery

Optional add-ons later:

- IR receiver for learn mode
- external power for fixed node use
- extra ESP32 scan nodes for multi-area coverage

## Risks

### Heat sensing risk

`Wi-Fi/BLE detections do not equal exact people counts.`

Mitigation:

- position the feature as `heat sensing`
- calibrate per area
- compare trend and relative differences, not exact headcount

### IR risk

`Device compatibility depends on code library coverage.`

Mitigation:

- start with the most common store device brands
- keep code sets configurable

### Codex risk

`Text UI and connectivity quality may limit usability.`

Mitigation:

- focus on host shortcuts and preset commands
- avoid making on-device editing a core promise

## Clear V1 Recommendation

Build the first usable version as:

- `Remote-first`
- `Heat-second`
- `Codex-third`

That gives:

- one immediate daily utility function
- one business-relevant sensing function
- one personal productivity function

## Immediate Next Step

The best next implementation target is:

`M2 IR remote`

Reason:

- easiest to verify on-device
- lowest ambiguity
- highest chance of becoming daily-use immediately

After that, implement:

`M3 heat sensing`

Then finish with:

`M4 Codex console`
