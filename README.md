# ESP-Zero-Ad

ESP32-S3 DNS sinkhole ad blocker — zero terminal, GUI-only setup.

## What It Does

ESP-Zero-Ad is a standalone DNS sinkhole running on an ESP32-S3. It blocks ads and tracking domains at the DNS level — no apps, no browser extensions, no terminal.

- **129 ad/tracking domains** loaded automatically on first boot (from adblock.turtlecute.org / d3host.txt)
- **Browser-based flashing** — no terminal or PlatformIO needed
- **6-step GUI wizard** — WiFi, password, flash, done
- **Web dashboard** at `http://esp32-pihole.local` — stats, block list management, ad reports
- **mDNS discovery** — just type the URL, no IP hunting
- **LittleFS config** — everything stored in files, no terminal editing

## Dashboard Views

1. **Dashboard** — Live DNS stats (queries, blocked, block rate), top blocked domains, query timeline
2. **Block List** — Add/remove domains, bulk import, search
3. **Ad Blocker Test** — Run the 129-domain test, view results
4. **Ad Reports** — Report ad domains, verify and auto-block
5. **Settings** — Network config, dashboard password, system actions

## Setup (No Terminal)

1. Open `wizard.html` in Chrome or Edge
2. Enter WiFi credentials
3. Set a dashboard password
4. Click "Flash" — browser flashes the ESP32-S3 via USB
5. Device boots, runs the ad blocker test, starts blocking
6. Open `http://esp32-pihole.local` from any device on your network

## Desktop App

Download the desktop app for your platform:
- **Windows**: `.exe` — double-click to run
- **Linux**: `.AppImage` — `chmod +x && ./ESP-Zero-Ad*.AppImage`
- **macOS**: `.dmg` — drag to Applications

## Hardware

- ESP32-S3-DevKitC-1
- 16MB flash (default partition scheme)

## Tech Stack

- ESP32-S3 (Arduino framework)
- LittleFS for persistent storage
- mDNS for local discovery
- ESP Web Tools for browser-based flashing
- Electron for desktop app packaging
- GitHub Actions for CI/CD

## License

MIT
