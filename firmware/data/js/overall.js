/* ============================================================
   Overall View (overall.js)
   Combined stats from BOTH the router's ad blocker AND ESP-Zero-Ad.
   Shows the full picture of ad blocking on your network.
   ============================================================ */

const OverallView = {
  async render() {
    const container = document.getElementById('view-container');
    container.innerHTML = `
      <div class="stat-grid">
        <div class="stat-card accent">
          <div class="stat-label">Total Ads Blocked</div>
          <div class="stat-value" id="ov-total-blocked">—</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">By Router</div>
          <div class="stat-value" id="ov-router-blocked" style="color:var(--accent)">—</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">By ESP-Zero-Ad</div>
          <div class="stat-value" id="ov-esp-blocked" style="color:var(--warning)">—</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Combined Coverage</div>
          <div class="stat-value" id="ov-coverage">—</div>
        </div>
      </div>

      <div class="card mb-24">
        <div class="card-header">
          <span class="card-title">Network Ad Blocking Overview</span>
          <span class="muted" style="font-size:0.8rem" id="ov-last-updated">—</span>
        </div>
        <div id="ov-overview">
          <div class="loading-center"><div class="spinner"></div></div>
        </div>
      </div>

      <div class="grid-2 mb-24">
        <!-- Router breakdown -->
        <div class="card">
          <div class="card-header">
            <span class="card-title">🛡 Router Ad Blocker Service</span>
          </div>
          <div id="ov-router-panel">
            <div class="loading-center"><div class="spinner"></div></div>
          </div>
        </div>

        <!-- ESP32 breakdown -->
        <div class="card">
          <div class="card-header">
            <span class="card-title">⚡ ESP-Zero-Ad</span>
          </div>
          <div id="ov-esp-panel">
            <div class="loading-center"><div class="spinner"></div></div>
          </div>
        </div>
      </div>

      <div class="card mb-24">
        <div class="card-header">
          <span class="card-title">Coverage Distribution</span>
        </div>
        <div id="ov-distribution">
          <div class="loading-center"><div class="spinner"></div></div>
        </div>
      </div>

      <div class="card">
        <div class="card-header">
          <span class="card-title">Live DNS Query Stats (ESP-Zero-Ad)</span>
        </div>
        <div id="ov-live-stats">
          <div class="loading-center"><div class="spinner"></div></div>
        </div>
      </div>
    `;

    await this.load();
  },

  async load() {
    try {
      const [testResults, stats, status] = await Promise.all([
        API.adguardResults().catch(() => []),
        API.getStats(),
        API.getStatus()
      ]);

      const results = Array.isArray(testResults) ? testResults : (testResults.results || []);
      const routerBlocked = results.filter(r => r.routerBlocks).length;
      const routerEscaped = results.filter(r => !r.routerBlocks).length;
      const totalTested = results.length;
      const espBlockedQueries = stats.blockedQueries || 0;
      const espForwardedQueries = stats.forwardedQueries || 0;
      const espTotalQueries = stats.totalQueries || 0;
      const espBlockListSize = status.blockedCount || 0;

      // Combined stats
      const totalAdsBlocked = routerBlocked + espBlockedQueries;
      const routerCoverage = totalTested > 0 ? Math.round((routerBlocked / totalTested) * 100) : 0;
      const espCoverage = totalTested > 0 ? Math.round((routerEscaped / totalTested) * 100) : 0;

      // Animate stat cards
      Helpers.animateNumber(document.getElementById('ov-total-blocked'), totalAdsBlocked);
      Helpers.animateNumber(document.getElementById('ov-router-blocked'), routerBlocked);
      Helpers.animateNumber(document.getElementById('ov-esp-blocked'), espBlockedQueries);
      const covEl = document.getElementById('ov-coverage');
      if (covEl) covEl.textContent = totalTested > 0 ? '100%' : '—';

      // Overview panel
      document.getElementById('ov-overview').innerHTML = `
        <div class="info-row">
          <span class="info-label">Total Unique Domains Tested</span>
          <span class="info-value">${totalTested}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Router Ad Blocker Handles</span>
          <span class="info-value" style="color:var(--accent)">${routerBlocked} (${routerCoverage}%)</span>
        </div>
        <div class="info-row">
          <span class="info-label">ESP-Zero-Ad Handles</span>
          <span class="info-value" style="color:var(--warning)">${routerEscaped} unique domains (${espCoverage}%)</span>
        </div>
        <div class="info-row">
          <span class="info-label">ESP-Zero-Ad Live Blocks</span>
          <span class="info-value">${espBlockedQueries} queries blocked</span>
        </div>
        <div class="info-row">
          <span class="info-label">ESP-Zero-Ad Forwarded</span>
          <span class="info-value">${espForwardedQueries} queries forwarded</span>
        </div>
        <div style="margin-top:16px">
          <div class="progress-bar progress-bar-large" style="height:14px">
            <div class="progress-fill" style="width:${routerCoverage}%; background:var(--accent)"></div>
          </div>
          <div style="display:flex; justify-content:space-between; margin-top:6px; font-size:0.78rem; color:var(--text-muted)">
            <span style="color:var(--accent)">■ Router: ${routerCoverage}%</span>
            <span style="color:var(--warning)">■ ESP-Zero-Ad: ${espCoverage}%</span>
          </div>
        </div>
      `;

      // Router panel
      document.getElementById('ov-router-panel').innerHTML = `
        <div class="info-row">
          <span class="info-label">Status</span>
          <span class="badge badge-success">Active</span>
        </div>
        <div class="info-row">
          <span class="info-label">Domains Blocked</span>
          <span class="info-value">${routerBlocked}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Coverage Rate</span>
          <span class="info-value" style="color:var(--accent)">${routerCoverage}%</span>
        </div>
        <div class="info-row">
          <span class="info-label">Domains Escaped</span>
          <span class="info-value" style="color:var(--danger)">${routerEscaped}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Test Source</span>
          <span class="info-value" style="font-size:0.8rem">adblock.turtlecute.org</span>
        </div>
      `;

      // ESP panel
      document.getElementById('ov-esp-panel').innerHTML = `
        <div class="info-row">
          <span class="info-label">Status</span>
          <span class="badge badge-success">Active</span>
        </div>
        <div class="info-row">
          <span class="info-label">Block List Size</span>
          <span class="info-value">${espBlockListSize} domains</span>
        </div>
        <div class="info-row">
          <span class="info-label">Total Queries</span>
          <span class="info-value">${Helpers.formatNumber(espTotalQueries)}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Queries Blocked</span>
          <span class="info-value" style="color:var(--warning)">${Helpers.formatNumber(espBlockedQueries)}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Block Rate</span>
          <span class="info-value">${Math.round(stats.blockedPercent || 0)}%</span>
        </div>
        <div class="info-row">
          <span class="info-label">WiFi Signal</span>
          <span class="info-value">${status.wifiRSSI || '—'} dBm</span>
        </div>
      `;

      // Distribution chart
      document.getElementById('ov-distribution').innerHTML = `
        <div style="display:flex; gap:4px; height:32px; border-radius:8px; overflow:hidden; margin-bottom:16px">
          <div style="width:${routerCoverage}%; background:var(--accent); display:flex; align-items:center; justify-content:center; font-size:0.75rem; font-weight:600; color:#000">
            ${routerCoverage > 10 ? 'Router ' + routerCoverage + '%' : ''}
          </div>
          <div style="width:${espCoverage}%; background:var(--warning); display:flex; align-items:center; justify-content:center; font-size:0.75rem; font-weight:600; color:#000">
            ${espCoverage > 10 ? 'ESP ' + espCoverage + '%' : ''}
          </div>
        </div>
        <div class="grid-2">
          <div style="text-align:center; padding:12px; background:var(--bg-tertiary); border-radius:8px">
            <div style="font-size:1.8rem; font-weight:700; color:var(--accent)">${routerBlocked}</div>
            <p class="muted" style="font-size:0.8rem">Router Blocked</p>
          </div>
          <div style="text-align:center; padding:12px; background:var(--bg-tertiary); border-radius:8px">
            <div style="font-size:1.8rem; font-weight:700; color:var(--warning)">${routerEscaped}</div>
            <p class="muted" style="font-size:0.8rem">ESP-Zero-Ad Handled</p>
          </div>
        </div>
      `;

      // Live stats
      const topBlocked = stats.topBlockedDomains || [];
      document.getElementById('ov-live-stats').innerHTML = `
        <div class="info-row">
          <span class="info-label">Total Queries</span>
          <span class="info-value">${Helpers.formatNumber(espTotalQueries)}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Blocked Queries</span>
          <span class="info-value" style="color:var(--warning)">${Helpers.formatNumber(espBlockedQueries)}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Forwarded Queries</span>
          <span class="info-value">${Helpers.formatNumber(espForwardedQueries)}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Unique Domains Seen</span>
          <span class="info-value">${stats.uniqueDomains || 0}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Uptime</span>
          <span class="info-value">${Helpers.formatUptime(status.uptime || 0)}</span>
        </div>
        ${topBlocked.length > 0 ? `
        <div style="margin-top:16px">
          <p style="font-size:0.85rem; color:var(--text-secondary); margin-bottom:8px">Top Blocked by ESP-Zero-Ad:</p>
          <div class="bar-chart">
            ${topBlocked.slice(0, 5).map(d => {
              const max = Math.max(...topBlocked.map(t => t.count), 1);
              const w = Math.max(5, (d.count / max) * 100);
              return `<div class="bar-row">
                <div class="bar-label">${Helpers.escapeHTML(d.domain)}</div>
                <div class="bar-track"><div class="bar-fill" style="width:${w}%; background:var(--warning)"><span class="bar-count">${d.count}</span></div></div>
              </div>`;
            }).join('')}
          </div>
        </div>
        ` : ''}
      `;

      document.getElementById('ov-last-updated').textContent = 'Updated ' + new Date().toLocaleTimeString();

    } catch (err) {
      document.getElementById('ov-overview').innerHTML =
        `<div class="empty-state"><div class="empty-title">Cannot load data</div><div class="muted">${Helpers.escapeHTML(err.message)}</div></div>`;
    }
  }
};
