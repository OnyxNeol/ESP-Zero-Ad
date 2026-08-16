# ESP-Zero-Ad

ESP32-S3 DNS sinkhole ad blocker — zero terminal, Bluetooth-paired, GUI-only setup.

## What It Does

ESP-Zero-Ad is a standalone DNS sinkhole running on an ESP32-S3. It blocks ads and tracking domains at the DNS level — no apps, no browser extensions, no terminal.

- **129 ad/tracking domains** loaded automatically on first boot (from adblock.turtlecute.org / d3host.txt)
- **Browser-based flashing** — no terminal or PlatformIO needed
- **6-step GUI wizard** — WiFi, password, flash, done
- **Bluetooth pairing** — authenticate via BLE, no password typing
- **Web dashboard** at `http://esp32-pihole.local` — stats, block list management, ad reports
- **mDNS discovery** — just type the URL, no IP hunting
- **LittleFS config** — everything stored in files, no terminal editing

## How It Works

1. **Setup**: Open `wizard.html` in Chrome/Edge → enter WiFi → flash via USB → device boots and connects to WiFi
2. **Pairing**: Wizard redirects to `http://esp32-pihole.local?setup=true` → click "Connect via Bluetooth" → select "ESP-Zero-Ad" → BLE pairing exchanges the API key automatically
3. **Dashboard**: After pairing, see device details (IP, WiFi, block list, stats) → enter dashboard
4. **Normal use**: Visit `http://esp32-pihole.local` anytime → click "Connect via Bluetooth" if not already paired → dashboard loads

No passwords to type. The BLE connection IS the authentication.

## Powering the Device

**Recommended:** Plug your ESP32-S3 into a **USB port on your router**. Most routers have a USB port that provides always-on power — your ad blocker runs 24/7 without a laptop.

Alternative: Any USB wall adapter (5V, 500mA minimum).

## Dashboard Views

1. **Dashboard** — Live DNS stats (queries, blocked, block rate), top blocked domains
2. **Block List** — Add/remove domains, bulk import, search
3. **Ad Blocker Test** — Run the 129-domain test, view results
4. **Ad Reports** — Report ad domains, verify and auto-block
5. **Settings** — Network config, system actions

## Browser Support

Bluetooth pairing requires **Chrome or Edge** (Web Bluetooth API). The dashboard works in any modern browser once paired.

## Hardware

- ESP32-S3-DevKitC-1
- 16MB flash (default partition scheme)
- BLE 5.0 (built into ESP32-S3)

## Tech Stack

- ESP32-S3 (Arduino framework)
- BLE GATT server (Web Bluetooth pairing)
- LittleFS for persistent storage
- mDNS for local discovery
- ESP Web Tools for browser-based flashing
- GitHub Actions for CI/CD

## License

MIT
