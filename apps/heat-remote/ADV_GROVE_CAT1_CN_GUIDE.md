# ADV Grove + Unit Cat1-CN Guide

This note defines the intended `Cardputer-Adv + Unit Cat1-CN` prototype route
when only a `Grove` cable is available.

## Scope

- target: use `4G` as the later cloud uplink path
- current local base remains:
  - `SD` total log file
  - `/heatlogs/heat_all.csv`
- this document is only for:
  - hardware preparation
  - Grove wiring
  - first-stage 4G bring-up

The current firmware does **not** yet contain Cat.1 upload code.

## Required hardware

- `1 x Cardputer-Adv`
- `1 x Unit Cat1-CN`
- `1 x Nano-SIM` with active data service
- `1 x SMA antenna` for the Cat1 module
- `1 x HY2.0-4P Grove cable`
- `1 x microSD card`

## Why Grove is acceptable

`Cardputer-Adv` exposes one `HY2.0-4P` port, and its official pin map is:

- `GND`
- `5V`
- `G2`
- `G1`

`Unit Cat1-CN` uses a Grove pin map of:

- `GND`
- `5V`
- `UART_RX`
- `UART_TX`

Because the main-control Grove GPIOs can be remapped in software, the `ADV`
Grove port can be used as a UART-style prototype link.

## Direct Grove wiring

With one straight Grove cable, the pin relationship becomes:

- `ADV GND -> Cat1 GND`
- `ADV 5V -> Cat1 5V`
- `ADV G2 -> Cat1 UART_RX`
- `ADV G1 -> Cat1 UART_TX`

For firmware:

- `ADV TX = G2`
- `ADV RX = G1`

This is the key software mapping. Do **not** use the later `EXT` UART mapping
(`G13/G15`) when the module is physically connected through Grove.

## Practical impact of using Grove

### 1. The Grove port is occupied

Once `Unit Cat1-CN` is connected:

- the only built-in Grove port on `ADV` is no longer available for another
  Unit

### 2. UART pin assignment changes

Cat.1 firmware should initialize the UART on:

- `RX = G1`
- `TX = G2`

### 3. Power stability is the main risk

`Unit Cat1-CN` is not a low-power sensor. Its official documentation lists
transmit peaks up to `DC 5V@831.12mA` in one TCP transmit mode.

That means the Grove connection is acceptable for:

- bring-up
- AT testing
- SIM recognition
- first HTTP or MQTT tests

But if you later see:

- random reboot
- network drop during transmit
- AT communication instability
- upload failure only during send

then treat power delivery as the first suspect.

## SIM and network requirements

- use a `Nano-SIM`
- data service must be enabled
- `Unit Cat1-CN` is aimed at Chinese Cat.1 carrier bands
- antenna must be installed before field testing

## First-day validation checklist

When the module arrives, verify in this order:

1. Physical connection
   - antenna installed
   - SIM inserted
   - Grove cable fully seated

2. UART alive
   - send `AT`
   - expect `OK`

3. SIM status
   - check whether the SIM is recognized

4. Network registration
   - verify the module attaches to the carrier

5. Signal quality
   - read signal strength

6. Data readiness
   - confirm IP/data session readiness

7. One minimal upload
   - send one fixed test payload first
   - do not start with full `HEAT` data upload

## Recommended software rollout

### Phase 1

Bring up only:

- UART
- AT command round-trip
- SIM detection
- network registration

### Phase 2

Send one simple cloud payload, for example:

- device id
- session id
- test message

### Phase 3

Connect the current `SD` logging model to `4G`, keeping:

- local full CSV as the source of truth
- 4G upload as the cloud path

Recommended model:

- continue writing every scan to `SD`
- upload either:
  - one row per scan, or
  - batched summaries later

## Current project assumptions

- local CSV schema currently includes:
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
- current `session_name` is a persistent sequence:
  - `boot_000001`
  - `boot_000002`
  - ...

This is already suitable for later `4G + SD` integration.

## Next implementation target

When Cat.1 hardware is on hand, the next firmware task should be:

- add a dedicated Cat.1 transport layer using Grove UART
- keep the existing `HEAT` scanner and `SD` writer unchanged
- upload a minimal payload first before attempting full cloud sync
