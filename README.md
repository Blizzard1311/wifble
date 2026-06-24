# Cardputer-Adv Workspace

This repository is organized as a single Cardputer-Adv workspace with one
folder per application.

## Layout

- `apps/`: app projects that can evolve independently.
- `device/`: device-level assets that are shared across apps, especially
  backups and restore notes.
- `ESP32-Paxcounter/`: separate reference project left untouched during this
  reorganization.

## Current apps

- `apps/heat-remote/`: the current custom firmware project for `HEAT`,
  `REMOTE`, EzData experiments, preview assets, and the Cat.1 ingest server.
- `apps/_template/`: the minimal starter you can copy when beginning a new
  Cardputer-Adv app.

## Device safety baseline

- Canonical restore image:
  `/Users/Kenneth/Documents/M5 STACK/device/backups/cardputer-adv-flash-20260531-162550-8mb.bin`
- SHA-256:
  `cb403bb8c29dfe61861163a981b8bc3af81c587ccaa47eac7dc1aa267aaa3953`

When future work needs flashing or firmware replacement, keep this backup path
as the default rollback target unless you explicitly replace it with a newer
baseline.

## Working rule

For each new app:

1. create a new folder under `apps/`
2. keep its `platformio.ini`, `src/`, docs, preview assets, and any app-local
   server code inside that folder
3. only move files into `device/` when they belong to the physical
   Cardputer-Adv itself rather than one specific app

This keeps unrelated experiments isolated and avoids guessing at a shared
library too early.

## Starting a new app

Use `apps/_template/` as the starting point for a new firmware app.

Suggested flow:

1. copy `apps/_template/` to `apps/<new-app-name>/`
2. edit the app name inside `README.md` and `src/main.cpp`
3. if needed, add app-local `preview/`, `server/`, or extra docs inside that
   same app folder
4. only add dependencies to that app's `platformio.ini` when the app actually
   needs them

The template is intentionally small so each new app starts clean.
