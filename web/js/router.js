/* ============================================================
   Router Reports View (router.js)
   Shows what the router's ad blocker service is blocking,
   based on the ad blocker test results.
   ============================================================ */

const RouterView = {
  results: [],
  lastTested: null,
  filter: 'all',

  async render() {
    const container = document.getElementById('view-container');
    container.innerHTML = `
      <div class="stat-grid" id="router-stat-grid">
        <div class="stat-card accent">
          <div class="stat-label">Router Blocks</div>
          <div class="stat-value" id="router-blocked-count">—</div>
        </div>
        <div class="stat-card danger">
          <div class="stat-label">Router Misses</div>
          <div class="stat-value" id="router-missed-count">—</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Router Block Rate</div>
          <div class="stat-value" id="router-block-rate">—</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Domains Tested</div>
          <div class="stat-value" id="router-tested-count">—</div>
        </div>
      </div>

      <div class="card mb-24">
        <div class="card-header">
          <span class="card-title">Router Ad Blocker Service Report</span>
          <button id="router-retest-btn" class="btn btn-primary btn-sm">🔄 Re-test Against Router</button>
        </div>
        <p class="muted" style="font-size:0.85rem; margin-bottom:16px">
          These are the results of testing 129 ad/tracking domains against your router's ad blocker service.
          Domains the router <strong style="color:var(--accent)">blocks</strong> are shown in green.
          Domains the router <strong style="color:var(--danger)">can't block</strong> are handled by the ESP32-S3.
        </p>

        <div class="filter-tabs" id="router-filters">
          <div class="filter-tab active" data-filter="all">All Domains</div>
          <div class="filter-tab" data-filter="blocked">Router Blocks</div>
          <div class="filter-tab" data-filter="escaped">Router Misses</div>
        </div>

        <div id="router-results" class="table-wrap">
          <div class="loading-center"><div class="spinner"></div></div>
        </div>
      </div>

      <div class="grid-2 mb-24">
        <div class="card">
          <div class="card-header"><span class="card-title">Block Rate by Category</span></div>
          <div id="router-category-chart" class="bar-chart">
            <div class="loading-center"><div class="spinner"></div></div>
          </div>
        </div>
        <div class="card">
          <div class="card-header"><span class="card-title">Router Coverage</span></div>
          <div id="router-coverage">
            <div class="loading-center"><div class="spinner"></div></div>
          </div>
        </div>
      </div>
    `;

    this.bindEvents();
    await this.loadResults();
  },

  bindEvents() {
    document.getElementById('router-retest-btn').addEventListener('click', () => this.retest());
    document.querySelectorAll('#router-filters .filter-tab').forEach(tab => {
      tab.addEventListener('click', () => {
        document.querySelectorAll('#router-filters .filter-tab').forEach(t => t.classList.remove('active'));
        tab.classList.add('active');
        this.filter = tab.dataset.filter;
        this.renderTable();
      });
    });
  },

  async loadResults() {
    try {
      const data = await API.adguardResults();
      this.results = Array.isArray(data) ? data : (data.results || []);
      this.renderStats();
      this.renderTable();
      this.renderCategoryChart();
      this.renderCoverage();
    } catch (err) {
      document.getElementById('router-results').innerHTML =
        `<div class="empty-state"><div class="empty-title">No test results</div><div class="muted">Run the ad blocker test to see router reports.</div></div>`;
    }
  },

  renderStats() {
    const total = this.results.length;
    const blocked = this.results.filter(r => r.routerBlocks).length;
    const escaped = total - blocked;
    const rate = total > 0 ? Math.round((blocked / total) * 100) : 0;

    document.getElementById('router-blocked-count').textContent = Helpers.formatNumber(blocked);
    document.getElementById('router-missed-count').textContent = Helpers.formatNumber(escaped);
    document.getElementById('router-block-rate').textContent = rate + '%';
    document.getElementById('router-tested-count').textContent = Helpers.formatNumber(total);
  },

  renderTable() {
    const el = document.getElementById('router-results');
    if (!el) return;

    let filtered = this.results;
    if (this.filter === 'blocked') filtered = this.results.filter(r => r.routerBlocks);
    else if (this.filter === 'escaped') filtered = this.results.filter(r => !r.routerBlocks);

    if (!filtered.length) {
      el.innerHTML = `<div class="empty-state"><div class="empty-title">No results</div><div class="muted">Run the ad blocker test first.</div></div>`;
      return;
    }

    el.innerHTML = `
      <table class="table">
        <thead>
          <tr>
            <th>Domain</th>
            <th style="width:120px">Status</th>
            <th style="width:140px">Resolved IP</th>
          </tr>
        </thead>
        <tbody>
          ${filtered.map(r => `
            <tr>
              <td class="domain-cell">${Helpers.escapeHTML(r.domain)}</td>
              <td>${r.routerBlocks
                ? '<span class="badge badge-success">✓ Blocked</span>'
                : '<span class="badge badge-danger">✗ Escaped</span>'}
              </td>
              <td style="font-family:var(--mono);font-size:0.8rem;color:var(--text-muted)">${Helpers.escapeHTML(r.resolvedIP || '—')}</td>
            </tr>
          `).join('')}
        </tbody>
      </table>
    `;
  },

  renderCategoryChart() {
    const el = document.getElementById('router-category-chart');
    if (!el || !this.results.length) {
      if (el) el.innerHTML = '<div class="empty-state"><div class="muted">No data</div></div>';
      return;
    }

    // Group by category (we don't have category in results, so group by domain TLD)
    const categories = {};
    this.results.forEach(r => {
      const parts = r.domain.split('.');
      const tld = parts.length > 2 ? parts.slice(-2).join('.') : r.domain;
      if (!categories[tld]) categories[tld] = { total: 0, blocked: 0 };
      categories[tld].total++;
      if (r.routerBlocks) categories[tld].blocked++;
    });

    const sorted = Object.entries(categories)
      .sort((a, b) => b[1].total - a[1].total)
      .slice(0, 10);

    const maxTotal = Math.max(...sorted.map(([_, c]) => c.total), 1);

    el.innerHTML = sorted.map(([tld, c]) => {
      const blockedPct = (c.blocked / c.total) * 100;
      const barWidth = (c.total / maxTotal) * 100;
      return `<div class="bar-row">
        <div class="bar-label" title="${tld}">${tld}</div>
        <div class="bar-track">
          <div class="bar-fill" style="width:${barWidth}%;background:${blockedPct > 50 ? 'var(--accent)' : 'var(--danger)'}">
            <span class="bar-count">${c.blocked}/${c.total}</span>
          </div>
        </div>
      </div>`;
    }).join('');
  },

  renderCoverage() {
    const el = document.getElementById('router-coverage');
    if (!el) return;

    const total = this.results.length;
    const blocked = this.results.filter(r => r.routerBlocks).length;
    const escaped = total - blocked;
    const rate = total > 0 ? Math.round((blocked / total) * 100) : 0;

    el.innerHTML = `
      <div style="text-align:center; padding:12px 0">
        <div style="font-size:2.5rem; font-weight:700; color:${rate >= 70 ? 'var(--accent)' : rate >= 40 ? 'var(--warning)' : 'var(--danger)'}">
          ${rate}%
        </div>
        <p class="muted" style="font-size:0.85rem; margin-top:4px">Router Ad Blocker Coverage</p>
      </div>
      <div class="progress-bar progress-bar-large mb-16">
        <div class="progress-fill" style="width:${rate}%"></div>
      </div>
      <div class="info-row">
        <span class="info-label">Router Blocks</span>
        <span class="info-value" style="color:var(--accent)">${blocked} domains</span>
      </div>
      <div class="info-row">
        <span class="info-label">Router Misses (ESP32 handles)</span>
        <span class="info-value" style="color:var(--danger)">${escaped} domains</span>
      </div>
      <div class="info-row">
        <span class="info-label">Total Domains Tested</span>
        <span class="info-value">${total} domains</span>
      </div>
      <div class="info-row">
        <span class="info-label">Combined Coverage</span>
        <span class="info-value" style="color:var(--accent)">${total > 0 ? '100%' : '—'}</span>
      </div>
    `;
  },

  async retest() {
    const btn = document.getElementById('router-retest-btn');
    btn.disabled = true;
    btn.innerHTML = '<div class="spinner spinner-sm"></div> Testing…';
    Toast.info('Testing', 'Testing 129 domains against router…');
    try {
      await API.adguardTest([], true);
      Toast.success('Test Complete', 'Router test complete — results updated.');
      await this.loadResults();
    } catch (err) {
      Toast.error('Test Failed', err.message);
    }
    btn.disabled = false;
    btn.textContent = '🔄 Re-test Against Router';
  }
};
