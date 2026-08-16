/* ============================================================
   Dashboard View (dashboard.js)
   ============================================================ */

const DashboardView = {
  lastStats: null,

  async render() {
    const container = document.getElementById('view-container');
    container.innerHTML = `
      <div class="stat-grid" id="stat-grid">
        <div class="stat-card">
          <div class="stat-label">Total Queries</div>
          <div class="stat-value" id="stat-total" data-value="0">0</div>
        </div>
        <div class="stat-card danger">
          <div class="stat-label">Blocked Queries</div>
          <div class="stat-value" id="stat-blocked" data-value="0">0</div>
        </div>
        <div class="stat-card accent">
          <div class="stat-label">Block Rate</div>
          <div class="stat-value" id="stat-percent" data-value="0">0%</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Domains on List</div>
          <div class="stat-value" id="stat-domains" data-value="0">0</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Uptime</div>
          <div class="stat-value" id="stat-uptime" style="font-size:1.3rem">—</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Free Memory</div>
          <div class="stat-value" id="stat-heap" style="font-size:1.3rem">—</div>
        </div>
      </div>

      <div class="grid-2 mb-24">
        <div class="card">
          <div class="card-header">
            <span class="card-title">Top Blocked Domains</span>
          </div>
          <div id="top-blocked" class="bar-chart">
            <div class="loading-center"><div class="spinner"></div></div>
          </div>
        </div>
        <div class="card">
          <div class="card-header">
            <span class="card-title">Query Timeline</span>
            <span class="muted" style="font-size:0.8rem" id="timeline-window">Last 60 polls</span>
          </div>
          <div id="timeline" class="timeline-chart">
            <div class="loading-center"><div class="spinner"></div></div>
          </div>
          <div class="timeline-legend">
            <span><span class="legend-dot" style="background:var(--accent)"></span>Forwarded</span>
            <span><span class="legend-dot" style="background:var(--danger)"></span>Blocked</span>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-header">
          <span class="card-title">System Information</span>
        </div>
        <div id="sys-info" class="grid-3">
          <div class="loading-center"><div class="spinner"></div></div>
        </div>
      </div>
    `;
    await this.refresh();
    this.startTimer();
  },

  startTimer() {
    if (App.dashboardTimer) clearInterval(App.dashboardTimer);
    App.dashboardTimer = setInterval(() => this.refresh(), 5000);
  },

  async refresh() {
    try {
      const [status, stats] = await Promise.all([
        API.getStatus(),
        API.getStats()
      ]);
      App.setConnection(true);
      this.updateStats(stats, status);
    } catch (err) {
      App.setConnection(false);
      // Only show toast if this isn't the first load
      if (this.lastStats) {
        Toast.error('Update Failed', 'Could not fetch latest stats.');
      }
      // Still render skeleton system info on first load
      if (!this.lastStats) {
        const si = document.getElementById('sys-info');
        if (si) si.innerHTML = '<div class="empty-state"><div class="empty-title">Cannot reach device</div><div class="muted">Check connection and dashboard password.</div></div>';
      }
    }
  },

  updateStats(stats, status) {
    const prev = this.lastStats;
    this.lastStats = stats;

    // Animated stat cards
    Helpers.animateNumber(document.getElementById('stat-total'), stats.totalQueries || 0);
    Helpers.animateNumber(document.getElementById('stat-blocked'), stats.blockedQueries || 0);

    const pct = Math.round(stats.blockedPercent || 0);
    const pctEl = document.getElementById('stat-percent');
    if (pctEl) {
      const start = parseInt(pctEl.dataset.value || '0', 10);
      pctEl.dataset.value = pct;
      const startTime = performance.now();
      function tickPct(now) {
        const progress = Math.min((now - startTime) / 600, 1);
        const eased = 1 - Math.pow(1 - progress, 3);
        const current = Math.round(start + (pct - start) * eased);
        pctEl.textContent = current + '%';
        if (progress < 1) requestAnimationFrame(tickPct);
      }
      requestAnimationFrame(tickPct);
    }

    Helpers.animateNumber(document.getElementById('stat-domains'), stats.uniqueDomains || status.totalQueries || 0);

    const uptimeEl = document.getElementById('stat-uptime');
    if (uptimeEl) uptimeEl.textContent = Helpers.formatUptime(status.uptime || 0);

    const heapEl = document.getElementById('stat-heap');
    if (heapEl) heapEl.textContent = Helpers.formatBytes(status.freeHeap || 0);

    // Top blocked domains bar chart
    this.renderTopBlocked(stats.topBlockedDomains || []);

    // Timeline
    const blocked = stats.blockedQueries || 0;
    const forwarded = stats.forwardedQueries || 0;
    const prevBlocked = prev ? prev.blockedQueries || 0 : 0;
    const prevForwarded = prev ? prev.forwardedQueries || 0 : 0;
    const deltaBlocked = Math.max(0, blocked - prevBlocked);
    const deltaForwarded = Math.max(0, forwarded - prevForwarded);
    App.timelineData.push({ blocked: deltaBlocked, forwarded: deltaForwarded });
    if (App.timelineData.length > 60) App.timelineData.shift();
    this.renderTimeline();

    // System info
    this.renderSysInfo(status, stats);
  },

  renderTopBlocked(domains) {
    const el = document.getElementById('top-blocked');
    if (!el) return;
    if (!domains.length) {
      el.innerHTML = '<div class="empty-state"><div class="empty-title">No blocked domains yet</div><div class="muted">Data will appear after queries are made.</div></div>';
      return;
    }
    const max = Math.max(...domains.map(d => d.count), 1);
    el.innerHTML = domains.slice(0, 10).map(d => {
      const w = Math.max(5, (d.count / max) * 100);
      return `<div class="bar-row">
        <div class="bar-label" title="${Helpers.escapeHTML(d.domain)}">${Helpers.escapeHTML(d.domain)}</div>
        <div class="bar-track"><div class="bar-fill" style="width:${w}%"><span class="bar-count">${d.count}</span></div></div>
      </div>`;
    }).join('');
  },

  renderTimeline() {
    const el = document.getElementById('timeline');
    if (!el) return;
    if (App.timelineData.length === 0) {
      el.innerHTML = '<div class="empty-state" style="padding:10px"><div class="muted">Collecting data…</div></div>';
      return;
    }
    const max = Math.max(...App.timelineData.map(p => p.blocked + p.forwarded), 1);
    el.innerHTML = App.timelineData.map(p => {
      const totalH = ((p.blocked + p.forwarded) / max) * 100;
      const blockedH = totalH > 0 && p.blocked > 0 ? (p.blocked / (p.blocked + p.forwarded)) * totalH : 0;
      const forwardedH = totalH - blockedH;
      return `<div class="timeline-bar">
        <div class="tl-forwarded" style="height:${forwardedH}%"></div>
        <div class="tl-blocked" style="height:${blockedH}%"></div>
      </div>`;
    }).join('');
  },

  renderSysInfo(status, stats) {
    const el = document.getElementById('sys-info');
    if (!el) return;
    const wifiQuality = status.wifiSignal !== undefined ? this.wifiQuality(status.wifiSignal) : null;
    el.innerHTML = `
      <div>
        <div class="info-row">
          <span class="info-label">WiFi Signal</span>
          <span class="info-value">${status.wifiSignal !== undefined ? status.wifiSignal + ' dBm' : '—'}</span>
        </div>
        <div class="info-row">
          <span class="info-label">WiFi Quality</span>
          <span class="info-value">${wifiQuality !== null ? wifiQuality.text : '—'}</span>
        </div>
      </div>
      <div>
        <div class="info-row">
          <span class="info-label">Free Heap</span>
          <span class="info-value">${Helpers.formatBytes(status.freeHeap || 0)}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Flash Size</span>
          <span class="info-value">${Helpers.formatBytes(status.flashSize || 0)}</span>
        </div>
      </div>
      <div>
        <div class="info-row">
          <span class="info-label">Uptime</span>
          <span class="info-value">${Helpers.formatUptime(status.uptime || 0)}</span>
        </div>
        <div class="info-row">
          <span class="info-label">Forwarded Queries</span>
          <span class="info-value">${Helpers.formatNumber(stats.forwardedQueries || 0)}</span>
        </div>
      </div>
    `;
  },

  wifiQuality(rssi) {
    if (rssi >= -50) return { text: 'Excellent', color: 'var(--success)' };
    if (rssi >= -60) return { text: 'Good', color: 'var(--success)' };
    if (rssi >= -70) return { text: 'Fair', color: 'var(--warning)' };
    if (rssi >= -80) return { text: 'Weak', color: 'var(--danger)' };
    return { text: 'Very Weak', color: 'var(--danger)' };
  }
};
