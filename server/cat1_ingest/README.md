# CAT1 Ingest Dashboard

This is the smallest deployable server companion for the Cardputer-Adv `CAT1`
route.

It keeps the current behavior:

- incoming device uploads are appended to one `jsonl` file
- the service runs behind `nginx`
- no database is required

It adds:

- `GET /api/m5/cat1/latest?limit=50`
- `GET /api/m5/cat1/download?format=jsonl`
- `GET /api/m5/cat1/download?format=csv`
- `GET /m5/cat1`
- simplified Chinese metric labels and notes for:
  - `热力值`
  - `原始热力(未封顶)`
  - `近场 Wi-Fi 数`
- server-side receive times are rendered as `Asia/Shanghai` on the web page
- the page explicitly distinguishes:
  - server receive time
  - device uptime-based sampling time
- a compact single-device-friendly dashboard without device filters

## Expected runtime paths on Tencent Cloud

- service code: `/home/ubuntu/cat1-ingest/server.py`
- data dir: `/home/ubuntu/cat1-ingest/data`
- log file: `/home/ubuntu/cat1-ingest/data/cat1_test.jsonl`
- systemd service: `cat1-ingest.service`

## Update path

Replace the remote `server.py` with this file, then restart:

```bash
sudo systemctl restart cat1-ingest
sudo systemctl status cat1-ingest --no-pager
```

## Public routes

Recommended nginx routes:

- `location /api/m5/cat1/test`
- `location /api/m5/cat1/latest`
- `location /api/m5/cat1/download`
- `location /m5/cat1`

All can proxy to `http://127.0.0.1:3012`.

## Data model

The current device upload payload is aligned with the local CSV schema:

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

Test rows may still contain a legacy `message` field.
