/* ============================================================
   Ad Blocker Test View (adguard.js)
   ============================================================
   The test-first approach: the ad blocker test runs BEFORE the
   block list is created. It uses 129 domains from
   adblock.turtlecute.org (d3host.txt) to test whether the
   router ad blocker service can block them. Only domains the
   router CANNOT block get added to the ESP32-S3's block list.
   ============================================================ */

const AdguardView = {
  results: [],
  testedAt: null,

  // 129 test domains from adblock.turtlecute.org (d3host.txt)
  BUILTIN_TEST_DOMAINS: [
    // Ads
    'adtago.s3.amazonaws.com','analyticsengine.s3.amazonaws.com','analytics.s3.amazonaws.com',
    'advice-ads.s3.amazonaws.com','pagead2.googlesyndication.com','adservice.google.com',
    'pagead2.googleadservices.com','afs.googlesyndication.com','stats.g.doubleclick.net',
    'ad.doubleclick.net','static.doubleclick.net','m.doubleclick.net',
    'mediavisor.doubleclick.net','ads30.adcolony.com','adc3-launch.adcolony.com',
    'events3alt.adcolony.com','wd.adcolony.com','static.media.net',
    'media.net','adservetx.media.net',
    // Analytics
    'analytics.google.com','click.googleanalytics.com','google-analytics.com',
    'ssl.google-analytics.com','adm.hotjar.com','identify.hotjar.com',
    'insights.hotjar.com','script.hotjar.com','surveys.hotjar.com',
    'careers.hotjar.com','events.hotjar.io','mouseflow.com',
    'cdn.mouseflow.com','o2.mouseflow.com','gtm.mouseflow.com',
    'api.mouseflow.com','tools.mouseflow.com','cdn-test.mouseflow.com',
    'freshmarketer.com','claritybt.freshmarketer.com','fwtracks.freshmarketer.com',
    'luckyorange.com','api.luckyorange.com','realtime.luckyorange.com',
    'cdn.luckyorange.com','w1.luckyorange.com','upload.luckyorange.net',
    'cs.luckyorange.net','settings.luckyorange.net','stats.wp.com',
    // Error Trackers
    'notify.bugsnag.com','sessions.bugsnag.com','api.bugsnag.com',
    'app.bugsnag.com','browser.sentry-cdn.com','app.getsentry.com',
    // Social
    'pixel.facebook.com','an.facebook.com','static.ads-twitter.com',
    'ads-api.twitter.com','ads.linkedin.com','analytics.pointdrive.linkedin.com',
    'ads.pinterest.com','log.pinterest.com','trk.pinterest.com',
    'events.reddit.com','events.redditmedia.com','ads.youtube.com',
    'ads-api.tiktok.com','analytics.tiktok.com','ads-sg.tiktok.com',
    'analytics-sg.tiktok.com','business-api.tiktok.com','ads.tiktok.com',
    'log.byteoversea.com',
    // Mix
    'ads.yahoo.com','analytics.yahoo.com','geo.yahoo.com',
    'udcm.yahoo.com','analytics.query.yahoo.com','partnerads.ysm.yahoo.com',
    'log.fc.yahoo.com','gemini.yahoo.com','adtech.yahooinc.com',
    'extmaps-api.yandex.net','appmetrica.yandex.ru','adfstat.yandex.ru',
    'metrika.yandex.ru','offerwall.yandex.net','adfox.yandex.ru',
    'auction.unityads.unity3d.com','webview.unityads.unity3d.com',
    'config.unityads.unity3d.com','adserver.unityads.unity3d.com',
    // OEMs
    'iot-eu-logser.realme.com','iot-logser.realme.com','bdapi-ads.realmemobile.com',
    'bdapi-in-ads.realmemobile.com','api.ad.xiaomi.com','data.mistat.xiaomi.com',
    'data.mistat.india.xiaomi.com','data.mistat.rus.xiaomi.com','sdkconfig.ad.xiaomi.com',
    'sdkconfig.ad.intl.xiaomi.com','tracking.rus.miui.com','adsfs.oppomobile.com',
    'adx.ads.oppomobile.com','ck.ads.oppomobile.com','data.ads.oppomobile.com',
    'metrics.data.hicloud.com','metrics2.data.hicloud.com','grs.hicloud.com',
    'logservice.hicloud.com','logservice1.hicloud.com','logbak.hicloud.com',
    'click.oneplus.cn','open.oneplus.net','samsungads.com',
    'smetrics.samsung.com','nmetrics.samsung.com','samsung-com.112.2o7.net',
    'analytics-api.samsunghealthcn.com','iadsdk.apple.com','metrics.icloud.com',
    'metrics.mzstatic.com','api-adservices.apple.com','books-analytics-events.apple.com',
    'weather-analytics-events.apple.com','notes-analytics-events.apple.com'
  ],

  async render() {
    const container = document.getElementById('view-container');
    container.innerHTML = `
      <div class="card mb-24" style="border-color: var(--accent)">
        <div class="card-header">
          <span class="card-title">⚡ Built-in Ad Blocker Test</span>
          <span class="badge badge-accent">129 domains</span>
        </div>
        <div class="info-box" style="margin-bottom:16px">
          <strong>Test-first approach:</strong> This test runs 129 domains from
          <a href="https://adblock.turtlecute.org" target="_blank" style="color:var(--accent)">adblock.turtlecute.org</a>
          against your router ad blocker service. Only domains the router <strong>CANNOT</strong> block
          get added to the ESP32-S3 block list. This prevents duplicate blocking and saves memory.
        </div>
        <div class="flex gap-8 flex-wrap">
          <button id="ag-builtin-test-btn" class="btn btn-primary" style="background:var(--accent);color:#0a0a0f;border-color:var(--accent)">
            ⚡ Run Built-in Test & Auto-Add
          </button>
          <button id="ag-builtin-load-btn" class="btn btn-secondary">
            Load 129 Domains into Editor
          </button>
        </div>
        <div id="ag-builtin-progress" style="display:none" class="mt-16">
          <div class="progress-bar progress-bar-large">
            <div class="progress-fill" id="ag-builtin-progress-fill" style="width:0%"></div>
          </div>
          <div class="muted mt-16" style="font-size:0.82rem" id="ag-builtin-progress-text">Testing…</div>
        </div>
      </div>

      <div class="card mb-24">
        <div class="card-header"><span class="card-title">Custom Domain Test</span></div>
        <div class="form-group">
          <label class="form-label">Domains to Test</label>
          <textarea id="ag-input" class="textarea" style="min-height:120px" placeholder="Enter one domain per line or comma-separated&#10;e.g. doubleclick.net, googlesyndication.com"></textarea>
          <div class="form-hint">Test custom domains against your router's DNS. Domains not blocked by the router can be auto-added to your block list.</div>
        </div>
        <div class="flex gap-8 flex-wrap">
          <button id="ag-test-btn" class="btn btn-primary">Test All</button>
          <button id="ag-test-add-btn" class="btn btn-success">Test & Auto-Add</button>
          <button id="ag-clear-input-btn" class="btn btn-secondary">Clear Input</button>
        </div>
        <div id="ag-progress" style="display:none" class="mt-16">
          <div class="progress-bar progress-bar-large">
            <div class="progress-fill" id="ag-progress-fill" style="width:0%"></div>
          </div>
          <div class="muted mt-16" style="font-size:0.82rem" id="ag-progress-text">Testing…</div>
        </div>
      </div>

      <div class="card mb-24" id="ag-summary-card" style="display:none">
        <div class="card-header"><span class="card-title">Test Summary</span></div>
        <div class="grid-3" id="ag-summary"></div>
      </div>

      <div class="card">
        <div class="card-header">
          <span class="card-title">Results</span>
          <span class="muted" style="font-size:0.8rem" id="ag-tested-at"></span>
        </div>
        <div class="table-wrap">
          <table class="table" id="ag-table">
            <thead>
              <tr>
                <th>Domain</th>
                <th>Router Blocks?</th>
                <th>Resolved IP</th>
                <th style="width:140px">Action</th>
              </tr>
            </thead>
            <tbody id="ag-tbody">
              <tr><td colspan="4"><div class="empty-state"><div class="empty-title">No results yet</div><div class="muted">Run the built-in test above to get started.</div></div></td></tr>
            </tbody>
          </table>
        </div>
      </div>
    `;
    this.bindEvents();
    await this.loadHistory();
  },

  bindEvents() {
    // Built-in test button
    document.getElementById('ag-builtin-test-btn').addEventListener('click', () => this.runBuiltinTest());
    document.getElementById('ag-builtin-load-btn').addEventListener('click', () => {
      document.getElementById('ag-input').value = this.BUILTIN_TEST_DOMAINS.join('\n');
      Toast.info('Loaded', `${this.BUILTIN_TEST_DOMAINS.length} test domains loaded into editor.`);
    });

    // Custom test buttons
    document.getElementById('ag-test-btn').addEventListener('click', () => this.runTest(false));
    document.getElementById('ag-test-add-btn').addEventListener('click', () => this.runTest(true));
    document.getElementById('ag-clear-input-btn').addEventListener('click', () => {
      document.getElementById('ag-input').value = '';
    });
  },

  async loadHistory() {
    try {
      const data = await API.adguardResults();
      if (data && data.results && data.results.length) {
        this.results = data.results;
        this.testedAt = data.testedAt;
        this.renderResults();
        document.getElementById('ag-tested-at').textContent = 'Last tested: ' + Helpers.formatTime(this.testedAt);
      }
    } catch (err) {
      // No history available yet
    }
  },

  parseDomains() {
    const input = document.getElementById('ag-input').value.trim();
    if (!input) return [];
    return input.split(/[\n,]/).map(d => d.trim().toLowerCase()).filter(d => d && Helpers.isValidDomain(d));
  },

  async runBuiltinTest() {
    const domains = this.BUILTIN_TEST_DOMAINS;
    const btn = document.getElementById('ag-builtin-test-btn');
    btn.disabled = true;
    btn.innerHTML = '<div class="spinner spinner-sm"></div> Testing 129 domains…';

    const progressWrap = document.getElementById('ag-builtin-progress');
    const progressFill = document.getElementById('ag-builtin-progress-fill');
    const progressText = document.getElementById('ag-builtin-progress-text');
    progressWrap.style.display = 'block';
    progressFill.style.width = '10%';
    progressText.textContent = `Testing ${domains.length} domains from adblock.turtlecute.org…`;

    try {
      progressFill.style.width = '50%';
      progressText.textContent = 'Sending DNS queries to router, please wait…';
      const result = await API.adguardTest(domains, true);
      progressFill.style.width = '100%';
      progressText.textContent = 'Done!';

      this.results = result.results || [];
      this.testedAt = new Date().toISOString();
      const added = result.added || 0;

      this.renderResults();
      this.renderSummary(this.results, added, true);
      document.getElementById('ag-tested-at').textContent = 'Last tested: ' + Helpers.formatTime(this.testedAt);

      if (added > 0) {
        Toast.success('Test Complete', `${this.results.length} tested, ${added} domains the router couldn't block were auto-added.`);
      } else {
        Toast.success('Test Complete', `${this.results.length} domains tested. Your router blocks them all!`);
      }

      setTimeout(() => { progressWrap.style.display = 'none'; }, 2000);
    } catch (err) {
      Toast.error('Test Failed', err.message);
      progressWrap.style.display = 'none';
    }
    btn.disabled = false;
    btn.innerHTML = '⚡ Run Built-in Test & Auto-Add';
  },

  async runTest(autoAdd) {
    const domains = this.parseDomains();
    if (domains.length === 0) {
      Toast.warning('No Domains', 'Enter at least one valid domain to test.');
      return;
    }

    const testBtn = document.getElementById('ag-test-btn');
    const addBtn = document.getElementById('ag-test-add-btn');
    testBtn.disabled = true;
    addBtn.disabled = true;

    const progressWrap = document.getElementById('ag-progress');
    const progressFill = document.getElementById('ag-progress-fill');
    const progressText = document.getElementById('ag-progress-text');
    progressWrap.style.display = 'block';
    progressFill.style.width = '30%';
    progressText.textContent = `Testing ${domains.length} domains${autoAdd ? ' (auto-add enabled)' : ''}…`;

    try {
      progressFill.style.width = '60%';
      const result = await API.adguardTest(domains, autoAdd);
      progressFill.style.width = '100%';
      progressText.textContent = 'Done!';

      this.results = result.results || [];
      this.testedAt = new Date().toISOString();
      const added = result.added || 0;

      this.renderResults();
      this.renderSummary(this.results, added, autoAdd);
      document.getElementById('ag-tested-at').textContent = 'Last tested: ' + Helpers.formatTime(this.testedAt);

      if (autoAdd && added > 0) {
        Toast.success('Test Complete', `${this.results.length} tested, ${added} domains auto-added to block list.`);
      } else {
        Toast.success('Test Complete', `${this.results.length} domains tested.`);
      }

      setTimeout(() => { progressWrap.style.display = 'none'; }, 1500);
    } catch (err) {
      Toast.error('Test Failed', err.message);
      progressWrap.style.display = 'none';
    }
    testBtn.disabled = false;
    addBtn.disabled = false;
  },

  renderSummary(results, added, autoAdd) {
    const card = document.getElementById('ag-summary-card');
    card.style.display = 'block';
    const blockedByRouter = results.filter(r => r.routerBlocks).length;
    const notBlocked = results.length - blockedByRouter;
    const el = document.getElementById('ag-summary');
    el.innerHTML = `
      <div class="info-row">
        <span class="info-label">Domains Tested</span>
        <span class="info-value">${results.length}</span>
      </div>
      <div class="info-row">
        <span class="info-label">Blocked by Router</span>
        <span class="info-value" style="color:var(--success)">${blockedByRouter}</span>
      </div>
      <div class="info-row">
        <span class="info-label">NOT Blocked by Router</span>
        <span class="info-value" style="color:var(--danger)">${notBlocked}</span>
      </div>
      <div class="info-row">
        <span class="info-label">Auto-Added to List</span>
        <span class="info-value">${autoAdd ? added : '— (manual)'}</span>
      </div>
      <div class="info-row">
        <span class="info-label">Block Rate</span>
        <span class="info-value">${results.length ? Math.round((blockedByRouter / results.length) * 100) : 0}%</span>
      </div>
      <div class="info-row">
        <span class="info-label">Tested At</span>
        <span class="info-value">${Helpers.formatTime(this.testedAt)}</span>
      </div>
    `;
  },

  renderResults() {
    const tbody = document.getElementById('ag-tbody');
    if (!this.results.length) {
      tbody.innerHTML = `<tr><td colspan="4"><div class="empty-state"><div class="empty-title">No results yet</div><div class="muted">Run the built-in test to get started.</div></div></td></tr>`;
      return;
    }
    tbody.innerHTML = this.results.map(r => {
      const blocked = r.routerBlocks;
      const badge = blocked
        ? '<span class="badge badge-success">✓ Blocked</span>'
        : '<span class="badge badge-danger">✕ Not Blocked</span>';
      const rowStyle = blocked ? '' : 'style="background:rgba(255,71,87,0.05)"';
      const action = blocked
        ? '<span class="badge badge-muted">Router handles</span>'
        : `<button class="btn btn-sm btn-success" data-add="${Helpers.escapeHTML(r.domain)}">Add to Block List</button>`;
      return `<tr ${rowStyle}>
        <td class="domain-cell">${Helpers.escapeHTML(r.domain)}</td>
        <td>${badge}</td>
        <td class="domain-cell">${Helpers.escapeHTML(r.resolvedIP || '—')}</td>
        <td>${action}</td>
      </tr>`;
    }).join('');

    tbody.querySelectorAll('[data-add]').forEach(btn => {
      btn.addEventListener('click', async () => {
        const domain = btn.dataset.add;
        btn.disabled = true;
        btn.innerHTML = '<div class="spinner spinner-sm"></div>';
        try {
          await API.addBlock(domain);
          Toast.success('Added', `${domain} added to block list.`);
          btn.outerHTML = '<span class="badge badge-success">✓ Added</span>';
        } catch (err) {
          Toast.error('Add Failed', err.message);
          btn.disabled = false;
          btn.textContent = 'Add to Block List';
        }
      });
    });
  }
};
