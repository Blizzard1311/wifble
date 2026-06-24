# Cardputer-Adv UI V1

This file defines the approved V1 interaction model before on-device flashing.
The local preview and current firmware prototype now use simplified Chinese
labels while keeping this same structure.

## Goal

Build a handheld system UI rather than a single technical demo page.

The first complete module is `Heat`.
`Remote` and `Codex` keep the final launcher structure in place, but remain
placeholder pages in V1.

## Screen Map

### 1. Home

Purpose:

- act like the system launcher
- show the three main function entries
- show lightweight live status

Visible sections:

- header: `Cardputer-Adv | Launcher`
- status badges:
  - WiFi count
  - BLE count
  - last scan time
- menu entries:
  - `Heat`
  - `Remote`
  - `Codex`
- bottom hint bar

### 2. Heat

Purpose:

- serve as the technical workspace for signal scanning
- make records browsable on-device

Sub-pages:

1. `Summary`
2. `WiFi Records`
3. `BLE Records`
4. `Scan History`

### 3. Remote

Purpose:

- preserve final menu shape
- reserve slot for future IR module

V1 behavior:

- enter page
- show placeholder summary
- return to Home

### 4. Codex

Purpose:

- preserve final menu shape
- reserve slot for future Codex entry

V1 behavior:

- enter page
- show placeholder summary
- return to Home

## Heat Page Details

### Summary

Show:

- current scan status
- current heat score
- strongest WiFi record
- strongest BLE record

### WiFi Records

Show per row:

- index
- SSID
- RSSI
- channel
- auth type

Behavior:

- sorted by RSSI
- one current row highlighted
- user can scroll through list

### BLE Records

Show per row:

- index
- device name
- RSSI
- address

Behavior:

- sorted by RSSI
- one current row highlighted
- user can scroll through list

### Scan History

Show per row:

- scan time label
- WiFi count
- BLE count
- top WiFi summary
- top BLE summary

Behavior:

- newest first
- one current row highlighted
- user can scroll through history

## Key Mapping

### Home

- `W / S`: move selection
- `A / D`: quick previous / next selection
- `Enter`: open selected module
- `Tab`: quick next selection

### Heat

- `A / D`: previous / next sub-page
- `W / S`: move current list selection
- `Enter`: trigger manual scan
- `Tab`: quick next sub-page
- `Backspace`: return to Home

### Remote / Codex

- `Backspace`: return to Home

## V1 Completion Standard

V1 is complete when all of the following are true:

- device boots into `Home`
- `Heat / Remote / Codex` entries are navigable
- `Heat` opens and switches between sub-pages
- WiFi records can be browsed
- BLE records can be browsed
- history can be browsed
- manual scan works
- auto scan works
- returning to Home works consistently
