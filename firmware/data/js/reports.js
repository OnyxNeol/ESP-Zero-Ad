/* ============================================================
   Ad Reports View (reports.js)
   ============================================================ */

const ReportsView = {
  reports: [],
  filter: 'all',
  search: '',

  async render() {
    const container = document.getElementById('view-container');
    container.innerHTML = `
      <div class="card mb-24">
        <div class="card-header"><span class="card-title">Report an Ad Domain</span></div>
        <div class="grid-2">
          <div class="form-group">
            <label class="form-label">Domain <span style="color:var(--danger)">*</span></label>
            <input type="text" id="rpt-domain" class="input" placeholder="ad.example.com" autocomplete="off">
          </div>
          <div class="form-group">
            <label class="form-label">Source / Website (optional)</label>
            <input type="text" id="rpt-source" class="input" placeholder="e.g. example.com" autocomplete="off">
          </div>
        </div>
        <div class="grid-2">
          <div class="form-group">
            <label class="form-label">Reported By (optional)</label>
            <input type="text" id="rpt-by" class="input" placeholder="Your name" autocomplete="off">
          </div>
          <div class="form-group" style="display:flex;align-items:flex-end">
            <button id="rpt-submit" class="btn btn-primary btn-block">Submit & Verify Report</button>
          </div>
        </div>
        <div class="form-hint">Submitting a report tests the domain against your router's ad blocker service. If the router CAN'T block it, the domain is added to the ESP32-S3 block list. If the router already handles it, the report is marked as router-blocked.</div>
      </div>

      <div class="card">
        <div class="card-header">
          <span class="card-title">Reports</span>
          <div class="flex gap-8 items-center">
            <button id="rpt-batch-verify" class="btn btn-sm btn-success">Verify All Pending</button>
            <input type="text" id="rpt-search" class="input" style="width:auto;max-width:220px" placeholder="Search reports…">
          </div>
        </div>
        <div class="filter-tabs" id="rpt-filters">
          <div class="filter-tab active" data-filter="all">All</div>
          <div class="filter-tab" data-filter="pending">Pending</div>
          <div class="filter-tab" data-filter="verified">Verified</div>
          <div class="filter-tab" data-filter="dismissed">Dismissed</div>
        </div>
        <div class="table-wrap">
          <table class="table" id="rpt-table">
            <thead>
              <tr>
                <th>Domain</th>
                <th>Source</th>
                <th>Reported By</th>
                <th>Status</th>
                <th>Date</th>
                <th style="width:140px">Actions</th>
              </tr>
            </thead>
            <tbody id="rpt-tbody">
              <tr><td colspan="6"><div class="loading-center"><div class="spinner"></div></div></td></tr>
            </tbody>
          </table>
        </div>
      </div>
    `;
    this.bindEvents();
    await this.load();
  },

  bindEvents() {
    document.getElementById('rpt-submit').addEventListener('click', () => this.submit());
    document.getElementById('rpt-batch-verify').addEventListener('click', () => this.batchVerify());

    const searchInput = document.getElementById('rpt-search');
    const debounced = Helpers.debounce((v) => {
      this.search = v;
      this.renderTable();
    }, 300);
    searchInput.addEventListener('input', (e) => debounced(e.target.value));

    document.querySelectorAll('#rpt-filters .filter-tab').forEach(tab => {
      tab.addEventListener('click', () => {
        document.querySelectorAll('#rpt-filters .filter-tab').forEach(t => t.classList.remove('active'));
        tab.classList.add('active');
        this.filter = tab.dataset.filter;
        this.renderTable();
      });
    });
  },

  async load() {
    try {
      const data = await API.getReports();
      this.reports = data.reports || [];
      this.renderTable();
    } catch (err) {
      const tbody = document.getElementById('rpt-tbody');
      tbody.innerHTML = `<tr><td colspan="6"><div class="empty-state"><div class="empty-title">Failed to load</div><div class="muted">${Helpers.escapeHTML(err.message)}</div></div></td></tr>`;
    }
  },

  getFilteredReports() {
    let list = this.reports;
    if (this.filter !== 'all') {
      list = list.filter(r => (r.status || 'pending') === this.filter);
    }
    if (this.search) {
      const q = this.search.toLowerCase();
      list = list.filter(r =>
        (r.domain || '').toLowerCase().includes(q) ||
        (r.source || '').toLowerCase().includes(q) ||
        (r.reportedBy || '').toLowerCase().includes(q)
      );
    }
    return list;
  },

  renderTable() {
    const tbody = document.getElementById('rpt-tbody');
    const list = this.getFilteredReports();
    if (!list.length) {
      tbody.innerHTML = `<tr><td colspan="6"><div class="empty-state"><div class="empty-title">No reports found</div><div class="muted">${this.search || this.filter !== 'all' ? 'Try changing the filter or search.' : 'Submit a report using the form above.'}</div></div></td></tr>`;
      return;
    }
    tbody.innerHTML = list.map(r => {
      const status = r.status || 'pending';
      const badge = {
        pending: '<span class="badge badge-warning">Pending</span>',
        verified: '<span class="badge badge-success">Verified</span>',
        dismissed: '<span class="badge badge-muted">Dismissed</span>'
      }[status] || '<span class="badge badge-muted">Unknown</span>';
      let actions = '';
      if (status === 'pending') {
        actions = `
          <button class="btn btn-sm btn-success" data-verify="${Helpers.escapeHTML(r.domain)}">Verify</button>
          <button class="btn btn-sm btn-danger" data-dismiss="${Helpers.escapeHTML(r.domain)}">Dismiss</button>
        `;
      } else if (status === 'verified') {
        actions = `<button class="btn btn-sm btn-danger" data-dismiss="${Helpers.escapeHTML(r.domain)}">Dismiss</button>`;
      } else {
        actions = `<button class="btn btn-sm" data-verify="${Helpers.escapeHTML(r.domain)}">Re-verify</button>`;
      }
      return `<tr>
        <td class="domain-cell">${Helpers.escapeHTML(r.domain)}</td>
        <td>${Helpers.escapeHTML(r.source || '—')}</td>
        <td>${Helpers.escapeHTML(r.reportedBy || '—')}</td>
        <td>${badge}</td>
        <td style="font-size:0.82rem">${Helpers.formatTime(r.createdAt)}</td>
        <td><div class="flex gap-8 flex-wrap">${actions}</div></td>
      </tr>`;
    }).join('');

    tbody.querySelectorAll('[data-verify]').forEach(btn => {
      btn.addEventListener('click', () => this.verifyReport(btn.dataset.verify));
    });
    tbody.querySelectorAll('[data-dismiss]').forEach(btn => {
      btn.addEventListener('click', () => this.dismissReport(btn.dataset.dismiss));
    });
  },

  async submit() {
    const domainEl = document.getElementById('rpt-domain');
    const sourceEl = document.getElementById('rpt-source');
    const byEl = document.getElementById('rpt-by');
    const domain = domainEl.value.trim().toLowerCase();
    const source = sourceEl.value.trim();
    const reportedBy = byEl.value.trim();

    if (!Helpers.isValidDomain(domain)) {
      Toast.error('Invalid Domain', 'Please enter a valid domain name.');
      domainEl.classList.add('error');
      return;
    }
    domainEl.classList.remove('error');

    const btn = document.getElementById('rpt-submit');
    btn.disabled = true;
    btn.innerHTML = '<div class="spinner spinner-sm"></div> Submitting…';

    try {
      // First, create the report
      await API.addReport({ domain, source, reportedBy });

      // Then verify it
      const result = await API.verifyReport(domain);

      if (result.exists) {
        if (result.added) {
          Toast.success('Verified & Added', `${domain} verified, resolved to ${result.ip || 'N/A'}, added to block list.`);
        } else {
          Toast.info('Already Blocked', `${domain} verified (${result.ip || 'N/A'}) but already on block list.`);
        }
      } else {
        Toast.warning('Domain Not Found', `${domain} could not be resolved. Report saved as pending.`);
      }

      domainEl.value = '';
      sourceEl.value = '';
      byEl.value = '';
      await this.load();
    } catch (err) {
      Toast.error('Report Failed', err.message);
    }
    btn.disabled = false;
    btn.textContent = 'Submit & Verify Report';
  },

  async verifyReport(domain) {
    try {
      const result = await API.verifyReport(domain);
      if (result.exists) {
        if (result.added) {
          Toast.success('Verified & Added', `${domain} added to block list (${result.ip || 'N/A'}).`);
        } else {
          Toast.info('Already Blocked', `${domain} is already on the block list.`);
        }
      } else {
        Toast.warning('Not Found', `${domain} does not resolve.`);
      }
      await this.load();
    } catch (err) {
      Toast.error('Verify Failed', err.message);
    }
  },

  async dismissReport(domain) {
    try {
      await API.dismissReport(domain);
      Toast.success('Dismissed', `Report for ${domain} dismissed.`);
      await this.load();
    } catch (err) {
      Toast.error('Dismiss Failed', err.message);
    }
  },

  async batchVerify() {
    const pending = this.reports.filter(r => (r.status || 'pending') === 'pending');
    if (pending.length === 0) {
      Toast.info('No Pending', 'There are no pending reports to verify.');
      return;
    }
    const btn = document.getElementById('rpt-batch-verify');
    btn.disabled = true;
    btn.innerHTML = `<div class="spinner spinner-sm"></div> Verifying 0/${pending.length}`;

    let verified = 0, added = 0, failed = 0;
    for (let i = 0; i < pending.length; i++) {
      btn.innerHTML = `<div class="spinner spinner-sm"></div> Verifying ${i + 1}/${pending.length}`;
      try {
        const result = await API.verifyReport(pending[i].domain);
        if (result.exists) {
          verified++;
          if (result.added) added++;
        }
      } catch {
        failed++;
      }
    }
    btn.disabled = false;
    btn.textContent = 'Verify All Pending';
    Toast.success('Batch Complete', `${verified} verified, ${added} added to block list${failed ? `, ${failed} failed` : ''}.`);
    await this.load();
  }
};
