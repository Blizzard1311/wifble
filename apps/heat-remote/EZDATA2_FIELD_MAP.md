# EzData2 field map

This firmware uploads `HEAT` summary data to M5Stack `EzData2` under a topic
prefix. The current default prefix is:

- `adv_heat`

That means the actual cloud field names look like:

- `adv_heat_heat`
- `adv_heat_raw`
- `adv_heat_level`
- `adv_heat_trend`
- `adv_heat_wifi_near`
- `adv_heat_wifi_total`
- `adv_heat_ble_near`
- `adv_heat_ble_total`
- `adv_heat_heat_log`
- `adv_heat_session_rows`
- `adv_heat_session_avg`
- `adv_heat_session_peak`
- `adv_heat_session_last`

## Scan summary fields

These are uploaded after each completed `HEAT` scan cycle.

| Field | Meaning | Example |
|---|---|---|
| `adv_heat_heat` | Smoothed `HEAT SCORE` shown on `SUMMARY` | `42` |
| `adv_heat_raw` | Raw, unsmoothed score from the latest scan | `47` |
| `adv_heat_level` | Heat level code | `0..3` |
| `adv_heat_trend` | Trend code versus previous score | `-1 / 0 / 1` |
| `adv_heat_wifi_near` | Nearby Wi-Fi count after RSSI filter | `6` |
| `adv_heat_wifi_total` | Total Wi-Fi count before filter | `15` |
| `adv_heat_ble_near` | Nearby BLE count after RSSI filter | `5` |
| `adv_heat_ble_total` | Total BLE count before filter | `11` |
| `adv_heat_heat_log` | Compact log entry written as `heat|time` | `42|10:13` |

## Session summary fields

These are uploaded when a `SESSION` is stopped.

| Field | Meaning | Example |
|---|---|---|
| `adv_heat_session_rows` | Number of recorded rows in the saved session | `18` |
| `adv_heat_session_avg` | Average `HEAT SCORE` across the session | `34` |
| `adv_heat_session_peak` | Peak `HEAT SCORE` seen in the session | `61` |
| `adv_heat_session_last` | Last `HEAT SCORE` at session stop | `29` |

## Value codes

`adv_heat_level` uses:

- `0` = `LOW`
- `1` = `MID`
- `2` = `HIGH`
- `3` = `BUSY`

`adv_heat_trend` uses:

- `-1` = `DOWN`
- `0` = `HOLD`
- `1` = `UP`

## My M5Stack viewing notes

The current M5Stack web UI does not expose a separate `EzData2` menu label.
In practice:

- left sidebar `Data` = `EzData2`
- URL is typically:
  - `https://my.m5stack.com/ezdata2/workflow`

Important:

- the `Devices` page token is not always the same token the firmware is
  actively using after auto-registration
- the device screen is authoritative for the runtime token tail
- on `HEAT -> SUMMARY`, the cloud status appears like:
  - `C OK 993f31`
- in that example, `993f31` is the last 6 characters of the active runtime
  token

## Device cloud status meanings

`SUMMARY` and `SESSION` show a compact cloud status:

- `OFF` = cloud upload disabled in firmware config
- `JOIN` = connecting Wi-Fi / cloud
- `READY` = Wi-Fi and MQTT ready
- `SEND` = upload in progress
- `OK xxxxxx` = upload succeeded, token tail shown

Diagnostic states:

- `WIFI` = Wi-Fi connect failed
- `REG` = `registerMac` failed or no valid `deviceToken`
- `MQx` = MQTT connection failed, `x` is the client return code
- `SUB` = down-topic subscribe failed
- `PUB` = MQTT publish failed
- `ACK` = no valid cloud reply matched the current field upload
- `Rxxx` = reply arrived but returned a non-200 code
- `R500` = server-side operation message was received while waiting

## Practical workflow

1. Enter `HEAT -> SUMMARY`.
2. Trigger a scan or wait for the next 1-minute cycle.
3. Confirm the device shows `C OK xxxxxx`.
4. Open `my.m5stack.com`.
5. Use left sidebar `Data`.
6. Refresh the active workspace/group.
7. Look for `adv_heat_*` fields listed above.

If cloud data is missing but the device shows `C OK`, the next thing to verify
is whether the current workspace/group is bound to the same runtime token tail
shown on the device.
