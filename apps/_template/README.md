# Cardputer-Adv App Template

This folder is the minimal starter for a new Cardputer-Adv application.

## Copy workflow

Create a new app by copying this folder:

```bash
cd "/Users/Kenneth/Documents/M5 STACK"
cp -R apps/_template apps/my-new-app
```

Then update:

- the app title in `README.md`
- the on-device text in `src/main.cpp`
- `platformio.ini` if the new app needs more libraries or a fixed serial port

## Commands

Activate the shared workspace environment:

```bash
cd "/Users/Kenneth/Documents/M5 STACK/apps/my-new-app"
source ../../.venv/bin/activate
```

Build:

```bash
../../.venv/bin/pio run
```

Flash:

```bash
../../.venv/bin/pio run -t upload
```

Monitor:

```bash
../../.venv/bin/pio device monitor
```

## Template intent

This template only proves:

- Cardputer-Adv display init
- keyboard scan loop
- a tiny on-device status screen

It does not include:

- SD card logic
- Wi-Fi or BLE logic
- IR logic
- cloud upload
- server code

Add those only when the new app actually needs them.
