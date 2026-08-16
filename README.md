# ESP32-S3 Pi-Hole

A Pi-Hole-style DNS ad blocker built for the ESP32-S3, with a twist: it **tests your router's built-in ad blocker service first** using 129 domains from [adblock.turtlecute.org](https://adblock.turtlecute.org), and only blocks the ads your router *can't* handle.

**Zero terminal. Zero hotspots. All real.**

## How It Works

The ESP32-S3 acts as a **DNS server in the middle** of your network:

```
  Your Devices  ──DNS queries──>  ESP32-S3  ──forward──>  Router / Upstream DNS
                    │                │
                    │           Block ad domains
                    │           (returns 0.0.0.0)
                    v
              Ads filtered out
```

1. You fill in `wifi_config.json` with your WiFi credentials **before flashing**
2. The ESP32-S3 boots, reads the config, joins your existing WiFi network
3. It automatically runs the AdBlock test (129 domains from adblock.turtlecute.org) against your router ad blocker service
4. Only domains the router **cannot** block get added to the ESP32-S3's block list
5. It starts the DNS sinkhole (port 53) + web dashboard (port 80)
6. You open `http://esp32-pihole.local` in your browser — a first-run wizard shows your IP, test results, and next steps
7. You point your devices' DNS to the ESP32-S3's IP — ads are now filtered

## Setup (No Terminal Required)

### Step 1: Open the Setup Wizard (GUI)

Open `wizard.html` in your browser (just double-click it). Fill in the step-by-step form:

```json
{
  "ssid": "YourWiFiName",
  "password": "YourWiFiPassword",
  "routerIP": "192.168.1.1",
  "apiKey": "auto"
}
```

- `ssid` / `password`: Your existing WiFi network
- `routerIP`: Your your router's IP (for ad blocker testing)
- `apiKey`: Set to `"auto"` to generate one, or choose your own

### Step 2: Flash

```bash
pip install platformio
cd esp32-pihole

# Flash the firmware:
pio run --target upload

# Upload the filesystem (includes wifi_config.json + web dashboard):
pio run --target uploadfs
```

### Step 3: Open the Dashboard

After flashing, the ESP32-S3:
1. Connects to your WiFi (no terminal needed to see the IP — it uses mDNS)
2. Runs the AdBlock test automatically (129 domains tested against your router)
3. Starts the DNS sinkhole + web dashboard

Open `http://esp32-pihole.local` in your browser. The first-run wizard shows:
- ✓ Your ESP32-S3's IP address and WiFi signal
- ⚡ AdBlock test results (how many domains tested, how many the router blocks vs. can't block)
- 📋 Instructions to set DNS on your devices to the ESP32-S3's IP
- 🚀 "Get Started" button → takes you to the main dashboard

### Step 4: Point Your Devices to the ESP32-S3

Set your devices' DNS server to the ESP32-S3's IP:
- **Per-device**: Change DNS settings on individual phones/computers
- **Network-wide**: Set the ESP32-S3's IP as the primary DNS in your router's DHCP settings
- **Router DNS**: Set your router's DNS to the ESP32-S3's IP

## The Dashboard

Once you click "Get Started" and enter your API key, you get:
- **Dashboard** — real-time stats (queries, blocked %, top domains, uptime, WiFi signal)
- **Block List** — add/remove/search blocked domains, see which came from the test
- **Ad Blocker Test** — re-run the built-in test (129 domains) or test custom domains
- **Ad Reports** — report ads you encounter, system verifies via DNS lookup and adds them
- **Settings** — change router IP, DNS servers, factory reset

## AdBlock Test (Test-First Approach)

The test runs **before** the block list is created, using 129 domains from [adblock.turtlecute.org](https://adblock.turtlecute.org) (d3host.txt):

1. ESP32-S3 sends a DNS query for each domain to your router (e.g., 192.168.1.1)
2. If the router returns `0.0.0.0`, `NXDOMAIN`, or a sinkhole IP → **router CAN block it** → skip
3. If the router returns a real IP → **router CANNOT block it** → add to ESP32-S3 block list
4. The ESP32-S3 only handles domains that slip through the router ad blocker service

**Categories tested**: ads (Google, DoubleClick, AdColony, Media.net), analytics (Google Analytics, Hotjar, MouseFlow, LuckyOrange), error trackers (Bugsnag, Sentry), social trackers (Facebook, Twitter, TikTok, Reddit, Pinterest), mix (Yahoo, Yandex, Unity ads), and OEM trackers (Xiaomi, Samsung, Apple, Huawei, Oppo, Realme, OnePlus).

## What's NOT Required

- ❌ No terminal/serial monitor access needed
- ❌ No WiFi hotspot created by the ESP32-S3
- ❌ No serial input during setup
- ❌ No JSON file editing by hand (wizard GUI writes the config)
- ❌ No CLI flashing (browser-based via ESP Web Tools)
- ❌ No OLED display needed
- ❌ No cloud dependency (works fully standalone)

## File Structure

```
esp32-pihole/
├── firmware/
│   ├── esp32_pihole.ino      # Main sketch (boot → WiFi → test → DNS → web)
│   ├── config.h              # Configuration defaults
│   ├── setup_wizard.*        # File-based config + first-run wizard status
│   ├── test_domains.h        # 129 test domains from adblock.turtlecute.org
│   ├── dns_server.*          # DNS sinkhole (port 53)
│   ├── block_list.*           # Block list management (LittleFS)
│   ├── adguard_tester.*      # Ad blocker pre-testing + built-in test runner
│   ├── web_server.*          # REST API + dashboard + wizard endpoints
│   ├── storage.*              # LittleFS storage manager
│   ├── data/
│   │   └── wifi_config.json  # ← EDIT THIS before flashing
│   └── platformio.ini
├── web/                       # Web dashboard (served from LittleFS)
│   ├── index.html            # SPA shell + sidebar + modals
│   ├── css/style.css         # Dark theme
│   └── js/
│       ├── app.js            # Router, API client, init (wizard-first)
│       ├── wizard.js         # First-run wizard (shows IP + test results)
│       ├── dashboard.js     # Real-time stats
│       ├── blocklist.js     # Block list management
│       ├── adguard.js       # Built-in test + custom domain testing
│       ├── reports.js       # Ad reporting + verification
│       └── settings.js      # Settings + factory reset
├── docs/
│   ├── adguard-research.md  # Ad blocker service on routers research
│   └── ad-domains-list.json # 129 test domains by category
├── platformio.ini
└── README.md
```

## Hardware

- **ESP32-S3 DevKitC-1** (16MB flash recommended)
- Micro USB cable for power + flashing
- WiFi network with an your router running ad blocker service
- No other hardware required

## License

MIT — build on it, modify it, share it.
Test domains sourced from [adblock.turtlecute.org](https://github.com/Turtlecute33/adblocktest) (CC BY-NC-SA).
