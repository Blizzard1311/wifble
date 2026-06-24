const wifiRecords = [
  { name: "STORE-GW", value: "-36" },
  { name: "DISPLAY-AP", value: "-48" },
  { name: "SENSOR", value: "-52" },
  { name: "LOGI-PAD", value: "-58" },
  { name: "CASHIER", value: "-61" },
];

const bleRecords = [
  { name: "IPHONE", value: "-41" },
  { name: "WATCH", value: "-47" },
  { name: "EARBUDS", value: "-54" },
  { name: "ANDROID", value: "-59" },
  { name: "BEACON", value: "-64" },
];

const logRecords = [
  { name: "14:38", value: "W16 B13" },
  { name: "14:18", value: "W14 B12" },
  { name: "13:58", value: "W11 B09" },
  { name: "13:38", value: "W12 B08" },
  { name: "13:18", value: "W09 B07" },
];

const state = {
  screen: "home",
  homeIndex: 0,
  heatMenuIndex: 0,
  wifiCursor: 0,
  bleCursor: 0,
  logCursor: 0,
  wifiCount: 16,
  bleCount: 13,
  heatScore: 29,
  status: "DONE",
  lastScanTime: "14:38",
};

const screenTitle = document.getElementById("screen-title");
const screenSubtitle = document.getElementById("screen-subtitle");
const screenContent = document.getElementById("screen-content");
const screen = document.getElementById("screen");
const modeButtons = [...document.querySelectorAll("[data-mode]")];

const modeStorageKey = "cardputer-preview-mode";

function preferredMode() {
  const saved = window.localStorage.getItem(modeStorageKey);
  if (saved === "device" || saved === "full") return saved;
  return window.innerWidth <= 1180 ? "device" : "full";
}

function applyMode(mode) {
  document.body.classList.toggle("device-only", mode === "device");
  modeButtons.forEach((button) => {
    button.classList.toggle("is-active", button.dataset.mode === mode);
  });
}

function clampCursor(value, max) {
  return Math.max(0, Math.min(value, max));
}

function trim(text, max) {
  return text.length <= max ? text : `${text.slice(0, max - 1)}~`;
}

function listRow(label, value, selected = false, meta = "") {
  return `
    <div class="simple-item ${selected ? "selected" : ""}">
      <div class="simple-main">
        <span class="simple-key">${label}</span>
        <span class="simple-value">${value}</span>
      </div>
      ${meta ? `<div class="simple-meta">${meta}</div>` : ""}
    </div>
  `;
}

function visibleRows(items, cursor, count = 4) {
  const start = Math.max(0, Math.min(cursor - 1, items.length - count));
  return items.slice(start, start + count).map((item, offset) => ({
    item,
    index: start + offset,
  }));
}

function renderHome() {
  screenTitle.textContent = "HOME";
  screenSubtitle.textContent = "MAIN MENU";

  const items = [
    { label: "HEAT", value: `${state.heatScore}` },
    { label: "REMOTE", value: "HOLD" },
    { label: "CODEX", value: "HOLD" },
  ];

  screenContent.innerHTML = `
    <div class="simple-status">W ${state.wifiCount} · B ${state.bleCount} · ${state.lastScanTime}</div>
    <div class="simple-list">
      ${items
        .map((item, index) => listRow(item.label, item.value, index === state.homeIndex))
        .join("")}
    </div>
  `;
}

function renderHeatMenu() {
  screenTitle.textContent = "HEAT";
  screenSubtitle.textContent = "HEAT MENU";

  const items = [
    { label: "SUMMARY", value: `${state.heatScore}` },
    { label: "WIFI", value: `${state.wifiCount}` },
    { label: "BLE", value: `${state.bleCount}` },
    { label: "LOG", value: `${logRecords.length}` },
  ];

  screenContent.innerHTML = `
    <div class="simple-status">STATUS ${state.status}</div>
    <div class="simple-list">
      ${items
        .map((item, index) => listRow(item.label, item.value, index === state.heatMenuIndex))
        .join("")}
    </div>
  `;
}

function renderSummary() {
  screenTitle.textContent = "HEAT";
  screenSubtitle.textContent = "SUMMARY";

  const rows = [
    { label: "HEAT", value: `${state.heatScore}` },
    { label: "WIFI", value: `${state.wifiCount}` },
    { label: "BLE", value: `${state.bleCount}` },
    { label: "STATUS", value: state.status },
    { label: "LAST", value: state.lastScanTime },
  ];

  screenContent.innerHTML = `
    <div class="simple-list">
      ${rows.map((row, index) => listRow(row.label, row.value, index === 0, index === 0 ? "Current score" : "")).join("")}
    </div>
  `;
}

function renderSignalList(kind, items, cursor) {
  screenTitle.textContent = "HEAT";
  screenSubtitle.textContent = kind;

  screenContent.innerHTML = `
    <div class="simple-status">COUNT ${items.length}</div>
    <div class="simple-list">
      ${visibleRows(items, cursor)
        .map(({ item, index }) => listRow(trim(item.name, 10), item.value, index === cursor))
        .join("")}
    </div>
  `;
}

function renderPlaceholder(title, subtitle, copy) {
  screenTitle.textContent = title;
  screenSubtitle.textContent = subtitle;

  screenContent.innerHTML = `
    <div class="simple-list">
      ${listRow(title, "HOLD", true, copy)}
    </div>
  `;
}

function render() {
  if (state.screen === "home") renderHome();
  if (state.screen === "heatMenu") renderHeatMenu();
  if (state.screen === "heatSummary") renderSummary();
  if (state.screen === "heatWifi") renderSignalList("WIFI LIST", wifiRecords, state.wifiCursor);
  if (state.screen === "heatBle") renderSignalList("BLE LIST", bleRecords, state.bleCursor);
  if (state.screen === "heatLog") renderSignalList("SCAN LOG", logRecords, state.logCursor);
  if (state.screen === "remote") renderPlaceholder("REMOTE", "RESERVED", "IR module later");
  if (state.screen === "codex") renderPlaceholder("CODEX", "RESERVED", "Console later");
}

function runScan() {
  const wifiDelta = Math.floor(Math.random() * 5) - 2;
  const bleDelta = Math.floor(Math.random() * 5) - 2;
  state.wifiCount = Math.max(6, state.wifiCount + wifiDelta);
  state.bleCount = Math.max(4, state.bleCount + bleDelta);
  state.heatScore = state.wifiCount + state.bleCount;
  state.lastScanTime = `${String(Math.floor(Math.random() * 46) + 10).padStart(2, "0")}:${String(Math.floor(Math.random() * 59)).padStart(2, "0")}`;
  state.status = "DONE";
  logRecords.unshift({ name: state.lastScanTime, value: `W${state.wifiCount} B${state.bleCount}` });
  logRecords.splice(5);
}

function openHomeSelection() {
  if (state.homeIndex === 0) state.screen = "heatMenu";
  if (state.homeIndex === 1) state.screen = "remote";
  if (state.homeIndex === 2) state.screen = "codex";
}

function openHeatSelection() {
  if (state.heatMenuIndex === 0) state.screen = "heatSummary";
  if (state.heatMenuIndex === 1) state.screen = "heatWifi";
  if (state.heatMenuIndex === 2) state.screen = "heatBle";
  if (state.heatMenuIndex === 3) state.screen = "heatLog";
}

function nextHeatPage() {
  if (state.screen === "heatSummary") state.screen = "heatWifi";
  else if (state.screen === "heatWifi") state.screen = "heatBle";
  else if (state.screen === "heatBle") state.screen = "heatLog";
  else if (state.screen === "heatLog") state.screen = "heatSummary";
}

function backFromHeatDetail() {
  if (state.screen === "heatSummary" || state.screen === "heatWifi" || state.screen === "heatBle" || state.screen === "heatLog") {
    state.screen = "heatMenu";
  } else {
    state.screen = "home";
  }
}

function handleAction(action) {
  if (state.screen === "home") {
    if (action === "up" || action === "left") state.homeIndex = (state.homeIndex + 2) % 3;
    if (action === "down" || action === "right" || action === "tab") state.homeIndex = (state.homeIndex + 1) % 3;
    if (action === "enter") openHomeSelection();
    render();
    return;
  }

  if (state.screen === "heatMenu") {
    if (action === "up" || action === "left") state.heatMenuIndex = (state.heatMenuIndex + 3) % 4;
    if (action === "down" || action === "right" || action === "tab") state.heatMenuIndex = (state.heatMenuIndex + 1) % 4;
    if (action === "enter") openHeatSelection();
    if (action === "back") state.screen = "home";
    render();
    return;
  }

  if (state.screen === "heatSummary") {
    if (action === "enter") runScan();
    if (action === "tab" || action === "right") nextHeatPage();
    if (action === "back") backFromHeatDetail();
    render();
    return;
  }

  if (state.screen === "heatWifi") {
    if (action === "up") state.wifiCursor = clampCursor(state.wifiCursor - 1, wifiRecords.length - 1);
    if (action === "down") state.wifiCursor = clampCursor(state.wifiCursor + 1, wifiRecords.length - 1);
    if (action === "tab" || action === "right") nextHeatPage();
    if (action === "back") backFromHeatDetail();
    render();
    return;
  }

  if (state.screen === "heatBle") {
    if (action === "up") state.bleCursor = clampCursor(state.bleCursor - 1, bleRecords.length - 1);
    if (action === "down") state.bleCursor = clampCursor(state.bleCursor + 1, bleRecords.length - 1);
    if (action === "tab" || action === "right") nextHeatPage();
    if (action === "back") backFromHeatDetail();
    render();
    return;
  }

  if (state.screen === "heatLog") {
    if (action === "up") state.logCursor = clampCursor(state.logCursor - 1, logRecords.length - 1);
    if (action === "down") state.logCursor = clampCursor(state.logCursor + 1, logRecords.length - 1);
    if (action === "tab" || action === "right") nextHeatPage();
    if (action === "back") backFromHeatDetail();
    render();
    return;
  }

  if (action === "back") {
    state.screen = "home";
    render();
  }
}

document.addEventListener("keydown", (event) => {
  if (["ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", "Enter", "Backspace", "Tab", "w", "W", "a", "A", "s", "S", "d", "D"].includes(event.key)) {
    event.preventDefault();
  }

  if (event.key === "ArrowUp" || event.key === "w" || event.key === "W") handleAction("up");
  if (event.key === "ArrowDown" || event.key === "s" || event.key === "S") handleAction("down");
  if (event.key === "ArrowLeft" || event.key === "a" || event.key === "A") handleAction("left");
  if (event.key === "ArrowRight" || event.key === "d" || event.key === "D") handleAction("right");
  if (event.key === "Enter") handleAction("enter");
  if (event.key === "Backspace") handleAction("back");
  if (event.key === "Tab") handleAction("tab");
});

modeButtons.forEach((button) => {
  button.addEventListener("click", () => {
    const mode = button.dataset.mode;
    window.localStorage.setItem(modeStorageKey, mode);
    applyMode(mode);
  });
});

window.addEventListener("resize", () => {
  if (!window.localStorage.getItem(modeStorageKey)) {
    applyMode(preferredMode());
  }
});

applyMode(preferredMode());
render();
screen.focus();
