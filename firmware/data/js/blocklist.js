/* ============================================================
   Block List View (blocklist.js)
   ============================================================ */

const BlocklistView = {
  page: 0,
  pageSize: 50,
  search: '',
  total: 0,
  domains: [],
  sortBy: 'domain',
  sortDir: 'asc',

  async render() {
    const container = document.getElementById('view-container');
    container.innerHTML = `
      <div class="grid-2 mb-24">
        <div class="card">
          <div class="card-header"><span class="card-title">Add Domain</span></div>
          <div class="form-group">
            <label class="form-label">Single Domain</label>
            <div class="input-group">
              <input type="text" id="add-domain-input" class="input" placeholder="example.com" autocomplete="off">
              <button id="add-domain-btn" class="btn btn-primary">Add</button>
            </div>
          </div>
          <div class="form-group">
            <label class="form-label">Bulk Add (one per line)</label>
            <textarea id="bulk-domain-input" class="textarea" placeholder="ad1.com&#10;ad2.com&#10;ad3.com"></textarea>
          </div>
          <button id="bulk-add-btn" class="btn btn-primary btn-block">Add All Domains</button>
        </div>

        <div class="card">
          <div class="card-header"><span class="card-title">Quick Actions</span></div>
          <p class="muted mb-16" style="font-size:0.85rem">Test common ad domains against your router first. Only domains the router can't block get added.</p>
          <button id="import-common-btn" class="btn btn-block mb-16">Test & Import Common Ad Domains</button>
          <button id="clear-all-btn" class="btn btn-danger btn-block">Clear All Blocked Domains</button>
          <div class="mt-24">
            <div class="info-row">
              <span class="info-label">Total Domains</span>
              <span class="info-value" id="bl-total">—</span>
            </div>
            <div class="info-row">
              <span class="info-label">Current Page</span>
              <span class="info-value" id="bl-page">—</span>
            </div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-header">
          <span class="card-title">Blocked Domains</span>
          <input type="text" id="bl-search" class="input" style="width:auto;max-width:280px" placeholder="Search domains…">
        </div>
        <div class="table-wrap">
          <table class="table" id="bl-table">
            <thead>
              <tr>
                <th class="sortable" data-sort="domain" id="th-domain">Domain</th>
                <th style="width:100px">Actions</th>
              </tr>
            </thead>
            <tbody id="bl-tbody">
              <tr><td colspan="2"><div class="loading-center"><div class="spinner"></div></div></td></tr>
            </tbody>
          </table>
        </div>
        <div class="pagination" id="bl-pagination"></div>
      </div>
    `;

    this.bindEvents();
    await this.load();
  },

  bindEvents() {
    document.getElementById('add-domain-btn').addEventListener('click', () => this.addSingle());
    document.getElementById('add-domain-input').addEventListener('keydown', (e) => {
      if (e.key === 'Enter') this.addSingle();
    });
    document.getElementById('bulk-add-btn').addEventListener('click', () => this.addBulk());
    document.getElementById('clear-all-btn').addEventListener('click', () => this.clearAll());
    document.getElementById('import-common-btn').addEventListener('click', () => this.importCommon());

    const searchInput = document.getElementById('bl-search');
    const debouncedSearch = Helpers.debounce((v) => {
      this.search = v;
      this.page = 0;
      this.load();
    }, 350);
    searchInput.addEventListener('input', (e) => debouncedSearch(e.target.value));

    document.getElementById('th-domain').addEventListener('click', () => {
      this.sortDir = this.sortBy === 'domain' && this.sortDir === 'asc' ? 'desc' : 'asc';
      this.sortBy = 'domain';
      this.updateSortIndicators();
      this.renderTable();
    });
  },

  updateSortIndicators() {
    const th = document.getElementById('th-domain');
    th.classList.remove('sort-asc', 'sort-desc');
    th.classList.add(this.sortDir === 'asc' ? 'sort-asc' : 'sort-desc');
  },

  async load() {
    try {
      const data = await API.getBlocklist(this.page * this.pageSize, this.pageSize, this.search);
      this.domains = data.domains || [];
      this.total = data.total || 0;
      this.updateSortIndicators();
      this.renderTable();
      this.renderPagination();
      document.getElementById('bl-total').textContent = Helpers.formatNumber(this.total);
      document.getElementById('bl-page').textContent = `${this.page + 1} / ${Math.max(1, Math.ceil(this.total / this.pageSize))}`;
    } catch (err) {
      const tbody = document.getElementById('bl-tbody');
      tbody.innerHTML = `<tr><td colspan="2"><div class="empty-state"><div class="empty-title">Failed to load</div><div class="muted">${Helpers.escapeHTML(err.message)}</div></div></td></tr>`;
    }
  },

  renderTable() {
    const tbody = document.getElementById('bl-tbody');
    if (!this.domains.length) {
      tbody.innerHTML = `<tr><td colspan="2"><div class="empty-state"><div class="empty-title">No domains found</div><div class="muted">${this.search ? 'Try a different search.' : 'Add domains using the form above.'}</div></div></td></tr>`;
      return;
    }
    const sorted = [...this.domains].sort((a, b) => {
      const va = (a.domain || a || '').toLowerCase();
      const vb = (b.domain || b || '').toLowerCase();
      return this.sortDir === 'asc' ? va.localeCompare(vb) : vb.localeCompare(va);
    });
    tbody.innerHTML = sorted.map(d => {
      const domain = d.domain || d;
      return `<tr>
        <td class="domain-cell">${Helpers.escapeHTML(domain)}</td>
        <td><button class="btn btn-danger btn-sm" data-remove="${Helpers.escapeHTML(domain)}">Remove</button></td>
      </tr>`;
    }).join('');
    tbody.querySelectorAll('[data-remove]').forEach(btn => {
      btn.addEventListener('click', () => this.removeDomain(btn.dataset.remove));
    });
  },

  renderPagination() {
    const el = document.getElementById('bl-pagination');
    const totalPages = Math.max(1, Math.ceil(this.total / this.pageSize));
    const start = this.total === 0 ? 0 : this.page * this.pageSize + 1;
    const end = Math.min((this.page + 1) * this.pageSize, this.total);
    el.innerHTML = `
      <div class="pagination-info">${this.total === 0 ? 'No results' : `${start}–${end} of ${this.total}`}</div>
      <div class="pagination-controls">
        <button class="btn btn-sm" id="bl-prev" ${this.page === 0 ? 'disabled' : ''}>Prev</button>
        <span class="muted" style="font-size:0.82rem">${this.page + 1} / ${totalPages}</span>
        <button class="btn btn-sm" id="bl-next" ${this.page >= totalPages - 1 ? 'disabled' : ''}>Next</button>
      </div>
    `;
    const prev = document.getElementById('bl-prev');
    const next = document.getElementById('bl-next');
    if (prev && !prev.disabled) prev.addEventListener('click', () => { this.page--; this.load(); });
    if (next && !next.disabled) next.addEventListener('click', () => { this.page++; this.load(); });
  },

  async addSingle() {
    const input = document.getElementById('add-domain-input');
    const domain = input.value.trim().toLowerCase();
    if (!Helpers.isValidDomain(domain)) {
      Toast.error('Invalid Domain', 'Please enter a valid domain name.');
      input.classList.add('error');
      return;
    }
    input.classList.remove('error');
    const btn = document.getElementById('add-domain-btn');
    btn.disabled = true;
    btn.innerHTML = '<div class="spinner spinner-sm"></div> Testing…';
    try {
      const result = await API.addBlock(domain);
      if (result.routerBlocks) {
        Toast.info('Router Handles It', `${domain} is already blocked by your router's ad blocker service — no need to add.`);
      } else if (result.success === false && result.message) {
        Toast.warning('Not Added', result.message);
      } else {
        Toast.success('Added', `${domain} escaped the router — added to ESP32-S3 block list.`);
        input.value = '';
        this.page = 0;
        this.search = '';
        document.getElementById('bl-search').value = '';
        await this.load();
      }
    } catch (err) {
      Toast.error('Add Failed', err.message);
    }
    btn.disabled = false;
    btn.textContent = 'Add';
  },

  async addBulk() {
    const textarea = document.getElementById('bulk-domain-input');
    const raw = textarea.value.trim();
    if (!raw) {
      Toast.warning('Empty', 'Enter at least one domain.');
      return;
    }
    const domains = raw.split(/[\n,]/).map(d => d.trim().toLowerCase()).filter(d => d);
    const valid = domains.filter(d => Helpers.isValidDomain(d));
    const invalid = domains.filter(d => !Helpers.isValidDomain(d));
    if (valid.length === 0) {
      Toast.error('No Valid Domains', 'None of the entered domains are valid.');
      return;
    }
    const btn = document.getElementById('bulk-add-btn');
    btn.disabled = true;
    btn.innerHTML = '<div class="spinner spinner-sm"></div> Adding…';
    try {
      const result = await API.bulkBlock(valid);
      const added = (result && result.added !== undefined) ? result.added : valid.length;
      Toast.success('Bulk Add Complete', `${added} domains added.${invalid.length ? ` ${invalid.length} skipped (invalid).` : ''}`);
      textarea.value = '';
      this.page = 0;
      await this.load();
    } catch (err) {
      Toast.error('Bulk Add Failed', err.message);
    }
    btn.disabled = false;
    btn.textContent = 'Add All Domains';
  },

  async removeDomain(domain) {
    try {
      await API.removeBlock(domain);
      Toast.success('Removed', `${domain} removed from block list.`);
      await this.load();
    } catch (err) {
      Toast.error('Remove Failed', err.message);
    }
  },

  clearAll() {
    App.confirm(
      'Clear All Blocked Domains',
      `This will remove all ${this.total} domains from the block list. This cannot be undone.`,
      async () => {
        try {
          await API.clearBlock();
          Toast.success('Cleared', 'All domains removed from block list.');
          this.page = 0;
          await this.load();
        } catch (err) {
          Toast.error('Clear Failed', err.message);
        }
      },
      'Clear All',
      'btn-danger'
    );
  },

  importCommon() {
    const common = [
      'doubleclick.net', 'googlesyndication.com', 'googleadservices.com',
      'google-analytics.com', 'adservice.google.com', 'adnxs.com',
      'ads.yahoo.com', 'amazon-adsystem.com', 'adsystem.com',
      'scorecardresearch.com', 'quantserve.com', 'adsrvr.org',
      'criteo.com', 'criteo.net', 'pubmatic.com',
      'rubiconproject.com', 'openx.net', 'adform.net',
      'moatads.com', 'taboola.com', 'outbrain.com',
      'adsafeprotected.com', 'contextweb.com', 'bidswitch.net',
      'serving-sys.com', '2o7.net', 'dmtry.com',
      'bluekai.com', 'chartbeat.com', 'chartbeat.net',
      'adsymptotic.com', 'demdex.net', 'omtrdc.net',
      'adservice.com', 'appsflyer.com', 'branch.io',
      'facebook.net', 'ads.facebook.com', 'analytics.facebook.com'
    ];
    App.confirm(
      'Test & Import Common Ad Domains',
      `This will add ${common.length} common ad/tracking domains to your block list.`,
      async () => {
        try {
          const result = await API.bulkBlock(common);
          const added = (result && result.added !== undefined) ? result.added : common.length;
          Toast.success('Import Complete', `${added} common ad domains added.`);
          await this.load();
        } catch (err) {
          Toast.error('Import Failed', err.message);
        }
      },
      'Import',
      'btn-primary'
    );
  }
};
