from __future__ import annotations

import csv
import io
import json
import os
from collections import Counter
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from statistics import mean
from typing import Any
from urllib.parse import parse_qs, urlparse

DATA_DIR = Path(os.environ.get("CAT1_DATA_DIR", "/home/ubuntu/cat1-ingest/data"))
DATA_DIR.mkdir(parents=True, exist_ok=True)
LOG_FILE = DATA_DIR / "cat1_test.jsonl"
HOST = os.environ.get("CAT1_BIND_HOST", "127.0.0.1")
PORT = int(os.environ.get("CAT1_PORT", "3012"))

CSV_COLUMNS = [
    "server_time",
    "remote",
    "device_id",
    "session_name",
    "session_start",
    "time",
    "heat",
    "raw",
    "raw_uncapped",
    "level",
    "trend",
    "wifi_kept",
    "wifi_total",
    "ble_kept",
    "ble_total",
    "message",
]

HTML_PAGE = """<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>CAT1 数据看板</title>
  <style>
    :root {
      --bg: #0b1220;
      --panel: #111b2e;
      --panel-2: #17233a;
      --text: #e7edf8;
      --muted: #91a0bb;
      --line: #26344f;
      --brand: #5cc8ff;
      --brand-2: #7ef29a;
      --warn: #ffb65c;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background: linear-gradient(180deg, #09101c 0%, #11192b 100%);
      color: var(--text);
    }
    .wrap {
      max-width: 1180px;
      margin: 0 auto;
      padding: 24px;
    }
    .header {
      display: flex;
      justify-content: space-between;
      gap: 16px;
      align-items: flex-start;
      margin-bottom: 20px;
    }
    .header h1 {
      margin: 0;
      font-size: 28px;
    }
    .header p {
      margin: 8px 0 0;
      color: var(--muted);
    }
    .toolbar {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
    }
    .button {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      border: 1px solid var(--line);
      background: var(--panel);
      color: var(--text);
      padding: 10px 14px;
      border-radius: 12px;
      text-decoration: none;
      cursor: pointer;
    }
    .button:hover { border-color: var(--brand); }
    .grid {
      display: grid;
      grid-template-columns: repeat(4, minmax(0, 1fr));
      gap: 14px;
      margin-bottom: 16px;
    }
    .card {
      background: rgba(17, 27, 46, 0.92);
      border: 1px solid var(--line);
      border-radius: 16px;
      padding: 16px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.18);
    }
    .label {
      font-size: 12px;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }
    .value {
      margin-top: 10px;
      font-size: 30px;
      font-weight: 700;
    }
    .sub {
      margin-top: 8px;
      color: var(--muted);
      font-size: 13px;
    }
    .panel-row {
      display: grid;
      grid-template-columns: 2fr 1fr;
      gap: 14px;
      margin-bottom: 16px;
    }
    .section-title {
      margin: 0 0 12px;
      font-size: 16px;
    }
    .chart-wrap {
      background: rgba(23, 35, 58, 0.7);
      border-radius: 12px;
      padding: 12px;
      border: 1px solid var(--line);
      min-height: 270px;
    }
    svg {
      width: 100%;
      height: 230px;
      display: block;
    }
    .legend {
      display: flex;
      gap: 16px;
      margin-top: 8px;
      color: var(--muted);
      font-size: 13px;
    }
    .legend span::before {
      content: "";
      display: inline-block;
      width: 10px;
      height: 10px;
      border-radius: 999px;
      margin-right: 6px;
      vertical-align: middle;
    }
    .legend .heat::before { background: var(--brand); }
    .legend .raw::before { background: var(--brand-2); }
    .legend .wifi::before { background: var(--warn); }
    table {
      width: 100%;
      border-collapse: collapse;
      font-size: 13px;
    }
    th, td {
      padding: 10px 8px;
      border-bottom: 1px solid var(--line);
      text-align: left;
      vertical-align: top;
    }
    th { color: var(--muted); font-weight: 600; }
    .mono {
      font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
      font-size: 12px;
      word-break: break-all;
    }
    .muted { color: var(--muted); }
    @media (max-width: 960px) {
      .grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
      .panel-row { grid-template-columns: 1fr; }
    }
    @media (max-width: 640px) {
      .wrap { padding: 16px; }
      .grid { grid-template-columns: 1fr; }
      .header { flex-direction: column; }
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="header">
      <div>
        <h1>CAT1 数据看板</h1>
        <p>查看最近上传、下载原始数据，并对热力趋势做快速分析。</p>
      </div>
      <div class="toolbar">
        <a class="button" href="/api/m5/cat1/download?format=jsonl">下载 JSONL</a>
        <a class="button" href="/api/m5/cat1/download?format=csv">下载 CSV</a>
        <button class="button" id="refreshButton" type="button">刷新数据</button>
      </div>
    </div>

    <div class="grid">
      <div class="card">
        <div class="label">总记录数</div>
        <div class="value" id="totalRecords">-</div>
        <div class="sub" id="totalDevices">-</div>
      </div>
      <div class="card">
        <div class="label">最近热力</div>
        <div class="value" id="latestHeat">-</div>
        <div class="sub" id="latestLevel">-</div>
      </div>
      <div class="card">
        <div class="label">近 50 条平均热力</div>
        <div class="value" id="avgHeat">-</div>
        <div class="sub" id="peakHeat">-</div>
      </div>
      <div class="card">
        <div class="label">最高原始密度</div>
        <div class="value" id="maxRawUncapped">-</div>
        <div class="sub" id="latestServerTime">-</div>
      </div>
    </div>

    <div class="panel-row">
      <div class="card">
        <h2 class="section-title">最近记录趋势</h2>
        <div class="chart-wrap">
          <svg id="chart" viewBox="0 0 880 230" preserveAspectRatio="none">
            <line x1="0" y1="200" x2="880" y2="200" stroke="#26344f" stroke-width="1"></line>
            <polyline id="heatLine" fill="none" stroke="#5cc8ff" stroke-width="3" points=""></polyline>
            <polyline id="rawLine" fill="none" stroke="#7ef29a" stroke-width="2.5" points=""></polyline>
            <polyline id="wifiLine" fill="none" stroke="#ffb65c" stroke-width="2" points=""></polyline>
          </svg>
          <div class="legend">
            <span class="heat">heat</span>
            <span class="raw">raw_uncapped</span>
            <span class="wifi">wifi_kept</span>
          </div>
        </div>
      </div>

      <div class="card">
        <h2 class="section-title">设备汇总</h2>
        <table>
          <thead>
            <tr>
              <th>设备</th>
              <th>最近上传</th>
              <th>条数</th>
            </tr>
          </thead>
          <tbody id="deviceTable"></tbody>
        </table>
      </div>
    </div>

    <div class="card">
      <h2 class="section-title">最近 20 条原始记录</h2>
      <table>
        <thead>
          <tr>
            <th>server_time</th>
            <th>device_id</th>
            <th>session</th>
            <th>time</th>
            <th>heat</th>
            <th>raw_uncapped</th>
            <th>wifi/ble</th>
            <th>payload</th>
          </tr>
        </thead>
        <tbody id="recordTable"></tbody>
      </table>
    </div>
  </div>

  <script>
    const latestUrl = "/api/m5/cat1/latest?limit=50";

    function toText(value, fallback = "-") {
      return value === null || value === undefined || value === "" ? fallback : String(value);
    }

    function buildPolyline(values, maxValue, width, height) {
      if (!values.length || maxValue <= 0) return "";
      return values.map((value, index) => {
        const x = values.length === 1 ? 0 : (index * width) / (values.length - 1);
        const y = height - (Number(value || 0) / maxValue) * (height - 10) - 10;
        return `${x.toFixed(2)},${y.toFixed(2)}`;
      }).join(" ");
    }

    async function refreshData() {
      const response = await fetch(latestUrl, { cache: "no-store" });
      const data = await response.json();
      if (!data.ok) {
        throw new Error(data.error || "load failed");
      }

      const stats = data.stats || {};
      const records = data.records || [];
      const devices = data.devices || [];
      const latest = records.length ? records[records.length - 1].payload || {} : {};

      document.getElementById("totalRecords").textContent = toText(stats.total_records);
      document.getElementById("totalDevices").textContent = `${toText(stats.total_devices)} 台设备`;
      document.getElementById("latestHeat").textContent = toText(latest.heat);
      document.getElementById("latestLevel").textContent = `${toText(latest.level)} / ${toText(latest.trend)}`;
      document.getElementById("avgHeat").textContent = toText(stats.avg_recent_heat);
      document.getElementById("peakHeat").textContent = `最近峰值 ${toText(stats.max_recent_heat)}`;
      document.getElementById("maxRawUncapped").textContent = toText(stats.max_recent_raw_uncapped);
      document.getElementById("latestServerTime").textContent = `最近上传 ${toText(stats.latest_server_time)}`;

      const chartRecords = records.filter((record) => record.payload && record.payload.heat !== undefined);
      const heatValues = chartRecords.map((record) => Number(record.payload.heat || 0));
      const rawValues = chartRecords.map((record) => Number(record.payload.raw_uncapped || 0));
      const wifiValues = chartRecords.map((record) => Number(record.payload.wifi_kept || 0));
      const maxValue = Math.max(100, ...heatValues, ...rawValues, ...wifiValues);
      document.getElementById("heatLine").setAttribute("points", buildPolyline(heatValues, maxValue, 880, 230));
      document.getElementById("rawLine").setAttribute("points", buildPolyline(rawValues, maxValue, 880, 230));
      document.getElementById("wifiLine").setAttribute("points", buildPolyline(wifiValues, maxValue, 880, 230));

      const deviceRows = devices.map((device) => `
        <tr>
          <td class="mono">${toText(device.device_id)}</td>
          <td>${toText(device.latest_server_time)}</td>
          <td>${toText(device.count)}</td>
        </tr>
      `).join("");
      document.getElementById("deviceTable").innerHTML = deviceRows || `<tr><td colspan="3" class="muted">暂无数据</td></tr>`;

      const recordRows = records.slice(-20).reverse().map((record) => {
        const payload = record.payload || {};
        return `
          <tr>
            <td>${toText(record.server_time)}</td>
            <td class="mono">${toText(payload.device_id)}</td>
            <td class="mono">${toText(payload.session_name)}</td>
            <td>${toText(payload.time)}</td>
            <td>${toText(payload.heat)}</td>
            <td>${toText(payload.raw_uncapped)}</td>
            <td>${toText(payload.wifi_kept)}/${toText(payload.ble_kept)}</td>
            <td class="mono">${toText(JSON.stringify(payload))}</td>
          </tr>
        `;
      }).join("");
      document.getElementById("recordTable").innerHTML = recordRows || `<tr><td colspan="8" class="muted">暂无数据</td></tr>`;
    }

    document.getElementById("refreshButton").addEventListener("click", () => {
      refreshData().catch((error) => alert(`刷新失败: ${error.message}`));
    });

    refreshData().catch((error) => {
      alert(`加载失败: ${error.message}`);
    });
  </script>
</body>
</html>
"""


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def normalize_int(value: Any) -> int | None:
    try:
        if value is None or value == "":
            return None
        return int(value)
    except (TypeError, ValueError):
        return None


def load_records() -> list[dict[str, Any]]:
    if not LOG_FILE.exists():
        return []

    records: list[dict[str, Any]] = []
    with LOG_FILE.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                record = json.loads(line)
            except json.JSONDecodeError:
                continue
            if not isinstance(record, dict):
                continue
            if not isinstance(record.get("payload"), dict):
                record["payload"] = {}
            records.append(record)
    return records


def build_device_rows(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    latest_map: dict[str, dict[str, Any]] = {}
    counts: Counter[str] = Counter()

    for record in records:
        payload = record.get("payload", {})
        device_id = str(payload.get("device_id") or "-")
        counts[device_id] += 1
        latest_map[device_id] = {
            "device_id": device_id,
            "latest_server_time": record.get("server_time", "-"),
            "count": counts[device_id],
        }

    rows = list(latest_map.values())
    rows.sort(key=lambda row: row["latest_server_time"], reverse=True)
    return rows


def build_stats(records: list[dict[str, Any]]) -> dict[str, Any]:
    stats: dict[str, Any] = {
        "total_records": len(records),
        "total_devices": 0,
        "latest_server_time": "-",
        "avg_recent_heat": "-",
        "max_recent_heat": "-",
        "max_recent_raw_uncapped": "-",
    }
    if not records:
        return stats

    devices = {str(record.get("payload", {}).get("device_id") or "-") for record in records}
    stats["total_devices"] = len(devices)
    stats["latest_server_time"] = records[-1].get("server_time", "-")

    recent = records[-50:]
    heat_values = [
        value
        for value in (normalize_int(record.get("payload", {}).get("heat")) for record in recent)
        if value is not None
    ]
    raw_uncapped_values = [
        value
        for value in (normalize_int(record.get("payload", {}).get("raw_uncapped")) for record in recent)
        if value is not None
    ]

    if heat_values:
        stats["avg_recent_heat"] = round(mean(heat_values), 1)
        stats["max_recent_heat"] = max(heat_values)
    if raw_uncapped_values:
        stats["max_recent_raw_uncapped"] = max(raw_uncapped_values)

    return stats


def records_to_csv(records: list[dict[str, Any]]) -> str:
    buf = io.StringIO()
    writer = csv.DictWriter(buf, fieldnames=CSV_COLUMNS)
    writer.writeheader()

    for record in records:
        payload = record.get("payload", {})
        row = {
            "server_time": record.get("server_time", ""),
            "remote": record.get("remote", ""),
            "device_id": payload.get("device_id", ""),
            "session_name": payload.get("session_name", ""),
            "session_start": payload.get("session_start", ""),
            "time": payload.get("time", ""),
            "heat": payload.get("heat", ""),
            "raw": payload.get("raw", ""),
            "raw_uncapped": payload.get("raw_uncapped", ""),
            "level": payload.get("level", ""),
            "trend": payload.get("trend", ""),
            "wifi_kept": payload.get("wifi_kept", ""),
            "wifi_total": payload.get("wifi_total", ""),
            "ble_kept": payload.get("ble_kept", ""),
            "ble_total": payload.get("ble_total", ""),
            "message": payload.get("message", ""),
        }
        writer.writerow(row)

    return buf.getvalue()


class Handler(BaseHTTPRequestHandler):
    def _send_bytes(self, code: int, body: bytes, content_type: str, extra_headers: dict[str, str] | None = None) -> None:
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        if extra_headers:
            for key, value in extra_headers.items():
                self.send_header(key, value)
        self.end_headers()
        self.wfile.write(body)

    def _send_json(self, code: int, payload: dict[str, Any]) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self._send_bytes(code, body, "application/json; charset=utf-8")

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path != "/api/m5/cat1/test":
            self._send_json(404, {"ok": False, "error": "not found"})
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
            raw = self.rfile.read(length)
            payload = json.loads(raw.decode("utf-8"))
        except Exception as e:
            self._send_json(400, {"ok": False, "error": f"bad json: {e}"})
            return

        record = {
            "server_time": utc_now_iso(),
            "remote": self.client_address[0],
            "payload": payload,
        }

        with LOG_FILE.open("a", encoding="utf-8") as f:
            f.write(json.dumps(record, ensure_ascii=False) + "\n")

        self._send_json(200, {"ok": True, "saved": True})

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        query = parse_qs(parsed.query)

        if parsed.path == "/health":
            self._send_json(200, {"ok": True, "service": "cat1-dashboard"})
            return

        if parsed.path == "/api/m5/cat1/latest":
            limit = min(max(int(query.get("limit", ["50"])[0]), 1), 500)
            records = load_records()
            response = {
                "ok": True,
                "records": records[-limit:],
                "stats": build_stats(records),
                "devices": build_device_rows(records),
            }
            self._send_json(200, response)
            return

        if parsed.path == "/api/m5/cat1/download":
            records = load_records()
            fmt = (query.get("format", ["jsonl"])[0] or "jsonl").lower()
            if fmt == "csv":
                body = records_to_csv(records).encode("utf-8")
                self._send_bytes(
                    200,
                    body,
                    "text/csv; charset=utf-8",
                    {"Content-Disposition": 'attachment; filename="cat1_records.csv"'},
                )
                return

            body = b""
            if LOG_FILE.exists():
                body = LOG_FILE.read_bytes()
            self._send_bytes(
                200,
                body,
                "application/x-ndjson; charset=utf-8",
                {"Content-Disposition": 'attachment; filename="cat1_records.jsonl"'},
            )
            return

        if parsed.path == "/m5/cat1":
            self._send_bytes(200, HTML_PAGE.encode("utf-8"), "text/html; charset=utf-8")
            return

        self._send_json(404, {"ok": False, "error": "not found"})

    def log_message(self, format: str, *args: Any) -> None:
        return


if __name__ == "__main__":
    server = HTTPServer((HOST, PORT), Handler)
    server.serve_forever()
