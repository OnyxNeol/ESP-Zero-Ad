/* ============================================================
   First-Run Wizard View (wizard.js)
   ============================================================
   Shows on first access to the dashboard. Displays:
   - Welcome + ESP32-S3 IP address and mDNS hostname
   - Ad blocker test results (how many domains tested, router blocks vs. not)
   - Block list size (domains added that the router couldn't block)
   - Instructions to set DNS on devices to the ESP32-S3's IP
   - "Get Started" button → marks wizard complete → dashboard

   No auth required — this is the landing page before login.
   ============================================================ */

const WizardView = {
  status: null,

  async render() {
    const container = document.getElementById('view-container');
    container.innerHTML = `
      <div style="max-width:640px;margin:0 auto;padding:24px 16px">
        <!-- Loading state -->
        <div id="wizard-loading" style="text-align:center;padding:60px 0">
          <div class="spinner" style="width:32px;height:32px;border-width:3px;margin:0 auto"></div>
          <p class="muted mt-16" style="font-size:0.9rem">Checking setup status…</p>
        </div>

        <!-- Wizard content (filled after status check) -->
        <div id="wizard-content" style="display:none"></div>
      </div>
    `;

    await this.loadStatus();
  },

  async loadStatus() {
    try {
      const resp = await fetch('/api/wizard/status');
      const data = await resp.json();
      this.status = data;
      this.renderWizard();
    } catch (err) {
      // If wizard endpoint fails, show error with retry
      document.getElementById('wizard-loading').innerHTML = `
        <p style="color:var(--danger);margin-bottom:16px">Could not reach the ESP32-S3.</p>
        <p class="muted" style="margin-bottom:16px">Make sure you are connected to the same WiFi network as the ESP32-S3.</p>
        <button class="btn btn-primary" onclick="location.reload()">Retry</button>
      `;
    }
  },

  renderWizard() {
    const s = this.status;
    const loading = document.getElementById('wizard-loading');
    const content = document.getElementById('wizard-content');

    if (!s.connected) {
      loading.innerHTML = `
        <p style="color:var(--danger);font-size:1.1rem;margin-bottom:16px">ESP32-S3 is not connected to WiFi</p>
        <p class="muted" style="margin-bottom:8px">Edit <code>firmware/data/wifi_config.json</code> with your WiFi credentials,</p>
        <p class="muted" style="margin-bottom:16px">then reflash: <code>pio run --target uploadfs</code></p>
        <button class="btn btn-primary" onclick="location.reload()">Retry Connection</button>
      `;
      return;
    }

    loading.style.display = 'none';
    content.style.display = 'block';

    const ip = s.ip || 'unknown';
    const hostname = s.hostname || 'esp32-pihole.local';
    const testRun = s.testRun;
    const blockListSize = s.blockListSize || 0;
    const testTotal = s.testTotal || 0;
    const routerBlocks = s.testRouterBlocks || 0;
    const notBlocked = s.testNotBlocked || 0;
    const blockRate = testTotal > 0 ? Math.round((routerBlocks / testTotal) * 100) : 0;

    content.innerHTML = `
      <!-- Welcome Header -->
      <div style="text-align:center;margin-bottom:32px">
        <div style="font-size:2.5rem;margin-bottom:8px">🛡️</div>
        <h1 style="font-size:1.6rem;color:var(--text);margin-bottom:8px">ESP32-S3 Pi-Hole</h1>
        <p class="muted" style="font-size:0.95rem">Your DNS ad-blocker is running and ready to go.</p>
      </div>

      <!-- Connection Info -->
      <div class="card mb-24" style="border-color:var(--accent)">
        <div class="card-header">
          <span class="card-title">✓ Connected to WiFi</span>
          <span class="badge badge-success" style="font-size:0.75rem">${s.ssid}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Dashboard URL</span>
          <span class="info-value" style="color:var(--accent);font-family:monospace">http://${hostname}</span>
        </div>
        <div class="info-row">
          <span class="info-label">IP Address</span>
          <span class="info-value" style="font-family:monospace">${ip}</span>
        </div>
        <div class="info-row">
          <span class="info-label">DNS Server</span>
          <span class="info-value" style="font-family:monospace">${ip}:53</span>
        </div>
        <div class="info-row">
          <span class="info-label">WiFi Signal</span>
          <span class="info-value">${s.rssi} dBm</span>
        </div>
      </div>

      <!-- Ad Blocker Test Results -->
      ${testRun ? `
      <div class="card mb-24">
        <div class="card-header">
          <span class="card-title">⚡ Ad Blocker Test Results</span>
          <span class="badge badge-success">Complete</span>
        </div>
        <div class="info-box" style="margin-bottom:16px">
          Tested <strong>${testTotal} domains</strong> from
          <a href="https://adblock.turtlecute.org" target="_blank" style="color:var(--accent)">adblock.turtlecute.org</a>
          against your router ad blocker service. Only domains the router <strong>couldn't block</strong>
          were added to the ESP32-S3's block list.
        </div>
        <div class="grid-3">
          <div class="stat-mini">
            <div class="stat-label">Tested</div>
            <div class="stat-value">${testTotal}</div>
          </div>
          <div class="stat-mini">
            <div class="stat-label">Router Blocks</div>
            <div class="stat-value" style="color:var(--success)">${routerBlocks}</div>
          </div>
          <div class="stat-mini">
            <div class="stat-label">Added to ESP32-S3</div>
            <div class="stat-value" style="color:var(--danger)">${notBlocked}</div>
          </div>
        </div>
        <div class="info-row" style="margin-top:12px">
          <span class="info-label">Router Block Rate</span>
          <span class="info-value">${blockRate}%</span>
        </div>
        <div class="info-row">
          <span class="info-label">ESP32-S3 Block List</span>
          <span class="info-value">${blockListSize} domains</span>
        </div>
      </div>
      ` : `
      <div class="card mb-24" style="border-color:var(--warning)">
        <div class="card-header">
          <span class="card-title">⚠ Ad Blocker Test Not Run</span>
        </div>
        <p class="muted">The ad blocker test has not been run yet. The block list may be empty.</p>
      </div>
      `}

      <!-- Setup Instructions -->
      <div class="card mb-24">
        <div class="card-header">
          <span class="card-title">📋 Next Step: Point Your Devices to the ESP32-S3</span>
        </div>
        <p style="margin-bottom:12px">To start blocking ads, set your devices' DNS server to the ESP32-S3's IP address:</p>
        <div style="background:var(--bg-secondary);border-radius:8px;padding:16px;font-family:monospace;font-size:0.9rem;margin-bottom:12px">
          DNS Server: <span style="color:var(--accent)">${ip}</span>
        </div>
        <p class="muted" style="font-size:0.85rem;margin-bottom:8px"><strong>Option A — Per device:</strong> Change DNS settings on individual phones/computers to ${ip}</p>
        <p class="muted" style="font-size:0.85rem;margin-bottom:8px"><strong>Option B — Network-wide:</strong> Set ${ip} as the primary DNS in your router's DHCP settings</p>
        <p class="muted" style="font-size:0.85rem"><strong>Option C — Router DNS:</strong> Set your router's DNS to ${ip} so all connected devices use it automatically</p>
      </div>

      <!-- Dashboard Password Notice -->
      <div class="card mb-24">
        <div class="card-header">
          <span class="card-title">🔑 Dashboard Access</span>
        </div>
        <p style="margin-bottom:8px">When you access the dashboard, you'll need an dashboard password to log in.</p>
        <p class="muted" style="font-size:0.85rem">Your dashboard password was set in wifi_config.json. If you used "auto", it was generated on first boot.</p>
      </div>

      <!-- Get Started -->
      <button id="wizard-get-started" class="btn btn-primary" style="width:100%;padding:16px;font-size:1.1rem">
        🚀 Get Started — Open Dashboard
      </button>

      <p class="muted" style="text-align:center;margin-top:16px;font-size:0.8rem">
        ESP32-S3 Pi-Hole • ${testTotal} domains tested • ${blockListSize} domains blocked • Uptime: ${Math.floor((s.uptime || 0) / 60)} min
      </p>
    `;

    // Bind get started button
    document.getElementById('wizard-get-started').addEventListener('click', () => {
      this.completeWizard();
    });
  },

  async completeWizard() {
    const btn = document.getElementById('wizard-get-started');
    btn.disabled = true;
    btn.innerHTML = '<div class="spinner spinner-sm"></div> Setting up…';

    try {
      await fetch('/api/wizard/complete', { method: 'POST' });
      // Reload the app — will show the login screen for the main dashboard
      window.location.hash = '#dashboard';
      location.reload();
    } catch (err) {
      // Even if the API call fails, reload to show the dashboard
      window.location.hash = '#dashboard';
      location.reload();
    }
  }
};
