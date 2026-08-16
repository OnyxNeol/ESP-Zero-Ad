/* ============================================================
   Settings View (settings.js)
   ============================================================ */

const SettingsView = {
  async render() {
    const container = document.getElementById('view-container');
    container.innerHTML = `
      <div class="grid-2 mb-24">
        <div class="card">
          <div class="card-header"><span class="card-title">Network Configuration</span></div>
<div class="form-group">
            <label class="form-label">Primary Upstream DNS</label>
            <input type="text" id="set-dns-primary" class="input" placeholder="8.8.8.8" autocomplete="off">
          </div>
          <div class="form-group">
            <label class="form-label">Secondary Upstream DNS</label>
            <input type="text" id="set-dns-secondary" class="input" placeholder="8.8.4.4" autocomplete="off">
          </div>
          <button id="set-save-network" class="btn btn-primary btn-block">Save Network Settings</button>
        </div>

        <div class="card">
          <div class="card-header"><span class="card-title">Dashboard Password</span></div>
          <div class="form-group">
            <label class="form-label">Current Dashboard Password</label>
            <input type="password" id="set-api-key" class="input" placeholder="••••••••" autocomplete="off">
            <div class="form-hint">Used for all API requests. Stored locally in your browser.</div>
          </div>
          <button id="set-save-apikey" class="btn btn-primary btn-block mb-16">Update Dashboard Password</button>
          <div class="info-row">
            <span class="info-label">Stored Key</span>
            <span class="info-value">${App.apiKey ? '••••••••' : 'Not set'}</span>
          </div>
        </div>
      </div>

      <div class="grid-2 mb-24">
        <div class="card">
          <div class="card-header"><span class="card-title">WiFi Information</span></div>
          <div id="set-wifi-info">
            <div class="loading-center"><div class="spinner"></div></div>
          </div>
        </div>

        <div class="card">
          <div class="card-header"><span class="card-title">System Actions</span></div>
          <div class="flex flex-col gap-12">
            <button id="set-restart" class="btn btn-block">Restart Device</button>
            <button id="set-reset-stats" class="btn btn-block">Reset Statistics</button>
            <button id="set-resync" class="btn btn-block">Re-sync Block List from Cloud</button>
          </div>
          <div class="mt-24" id="set-sys-status" style="display:none">
            <div class="info-row">
              <span class="info-label">Status</span>
              <span class="info-value" id="set-sys-status-val">—</span>
            </div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-header"><span class="card-title">About</span></div>
        <div class="grid-3">
          <div>
            <div class="info-row"><span class="info-label">Firmware</span><span class="info-value">ESP32-Block v1.0</span></div>
            <div class="info-row"><span class="info-label">Platform</span><span class="info-value">ESP32-S3</span></div>
          </div>
          <div>
            <div class="info-row"><span class="info-label">DNS Engine</span><span class="info-value">dnsproxy</span></div>
            <div class="info-row"><span class="info-label">Storage</span><span class="info-value">LittleFS</span></div>
          </div>
          <div>
            <div class="info-row"><span class="info-label">Web UI</span><span class="info-value">v1.0.0</span></div>
            <div class="info-row"><span class="info-label">License</span><span class="info-value">MIT</span></div>
          </div>
        </div>
        <div class="mt-16 muted" style="font-size:0.82rem">
          ESP32-Block is a lightweight DNS sinkhole for ESP32-S3, inspired by Pi-hole. It blocks ads and trackers at the DNS level.
        </div>
      </div>
    `;

    this.bindEvents();
    await this.loadSettings();
    await this.loadSysInfo();
  },

  bindEvents() {
    document.getElementById('set-save-network').addEventListener('click', () => this.saveNetwork());
    document.getElementById('set-save-apikey').addEventListener('click', () => this.saveAPIKey());
    document.getElementById('set-restart').addEventListener('click', () => this.systemAction('restart', 'Restart Device', 'Restart the ESP32-S3 device? This will temporarily disconnect all services.'));
    document.getElementById('set-reset-stats').addEventListener('click', () => this.systemAction('reset-stats', 'Reset Statistics', 'Reset all query statistics? This cannot be undone.'));
    document.getElementById('set-resync').addEventListener('click', () => this.systemAction('resync', 'Re-sync Block List', 'Re-sync the block list from cloud? This may take a moment.'));
  },

  async loadSettings() {
    try {
      const data = await API.getSettings();
      if (data.upstreamDNS) {
        const dns = data.upstreamDNS.split(',');
        document.getElementById('set-dns-primary').value = (dns[0] || '').trim();
        document.getElementById('set-dns-secondary').value = (dns[1] || '').trim();
      }
      if (data.apiKey) document.getElementById('set-api-key').value = data.apiKey;
    } catch (err) {
      // Settings might not be available; leave placeholders
    }
  },

  async loadSysInfo() {
    try {
      const status = await API.getStatus();
      const el = document.getElementById('set-wifi-info');
      el.innerHTML = `
        <div class="info-row"><span class="info-label">WiFi Signal</span><span class="info-value">${status.wifiSignal !== undefined ? status.wifiSignal + ' dBm' : '—'}</span></div>
        <div class="info-row"><span class="info-label">Free Heap</span><span class="info-value">${Helpers.formatBytes(status.freeHeap || 0)}</span></div>
        <div class="info-row"><span class="info-label">Flash Size</span><span class="info-value">${Helpers.formatBytes(status.flashSize || 0)}</span></div>
        <div class="info-row"><span class="info-label">Uptime</span><span class="info-value">${Helpers.formatUptime(status.uptime || 0)}</span></div>
      `;
    } catch (err) {
      document.getElementById('set-wifi-info').innerHTML = '<div class="empty-state"><div class="empty-title">Unable to load</div><div class="muted">Cannot reach device.</div></div>';
    }
  },

  async saveNetwork() {
    const dns1 = document.getElementById('set-dns-primary').value.trim();
    const dns2 = document.getElementById('set-dns-secondary').value.trim();

    

    if (dns1 && !Helpers.isValidIP(dns1)) {
      Toast.error('Invalid DNS', 'Primary DNS is not a valid IP.');
      document.getElementById('set-dns-primary').classList.add('error');
      return;
    }
    document.getElementById('set-dns-primary').classList.remove('error');

    if (dns2 && !Helpers.isValidIP(dns2)) {
      Toast.error('Invalid DNS', 'Secondary DNS is not a valid IP.');
      document.getElementById('set-dns-secondary').classList.add('error');
      return;
    }
    document.getElementById('set-dns-secondary').classList.remove('error');

    const upstreamDNS = [dns1, dns2].filter(Boolean).join(', ');
    const btn = document.getElementById('set-save-network');
    btn.disabled = true;
    btn.innerHTML = '<div class="spinner spinner-sm"></div> Saving…';

    try {
      await API.saveSettings({ upstreamDNS });
      Toast.success('Saved', 'Network settings updated successfully.');
    } catch (err) {
      Toast.error('Save Failed', err.message);
    }
    btn.disabled = false;
    btn.textContent = 'Save Network Settings';
  },

  saveAPIKey() {
    const input = document.getElementById('set-api-key');
    const key = input.value.trim();
    if (!key) {
      Toast.error('Error', 'Please enter a dashboard password.');
      input.classList.add('error');
      return;
    }
    input.classList.remove('error');
    App.apiKey = key;
    localStorage.setItem('esp32_api_key', key);
    Toast.success('Updated', 'Dashboard password updated and saved locally.');
    input.value = '';
  },

  systemAction(action, title, message) {
    App.confirm(title, message, async () => {
      const statusEl = document.getElementById('set-sys-status');
      const statusVal = document.getElementById('set-sys-status-val');
      statusEl.style.display = 'block';
      statusVal.textContent = 'Processing…';

      // Map to settings endpoint or dedicated action
      const actionMap = {
        'restart': { path: '/api/settings', body: { action: 'restart' } },
        'reset-stats': { path: '/api/settings', body: { action: 'reset-stats' } },
        'resync': { path: '/api/settings', body: { action: 'resync' } }
      };

      try {
        await API.post(actionMap[action].path, actionMap[action].body);
        const messages = {
          'restart': 'Device is restarting. Please wait ~30s for it to come back online.',
          'reset-stats': 'Statistics have been reset.',
          'resync': 'Block list re-synced from cloud.'
        };
        statusVal.textContent = '✓ ' + messages[action];
        Toast.success('Done', messages[action]);
        if (action === 'resync') await this.loadSysInfo();
      } catch (err) {
        statusVal.textContent = '✕ Failed';
        Toast.error('Action Failed', err.message);
      }
    }, action === 'restart' ? 'Restart' : 'Confirm', 'btn-primary');
  }
};
