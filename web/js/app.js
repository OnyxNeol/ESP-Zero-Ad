/* ============================================================
   ESP32 Ad Blocker — Main Application (app.js)
   Router, API client, toast system, state, helpers, init
   ============================================================ */

/* ===== State ===== */
const App = {
  apiKey: localStorage.getItem('esp32_api_key') || '',
  currentRoute: 'dashboard',
  cache: {},
  dashboardTimer: null,
  timelineData: [],
  sortState: {}
};

/* ===== Helper Functions ===== */
const Helpers = {
  formatNumber(n) {
    if (n === null || n === undefined) return '—';
    if (n >= 1e9) return (n / 1e9).toFixed(2) + 'B';
    if (n >= 1e6) return (n / 1e6).toFixed(2) + 'M';
    if (n >= 1e3) return (n / 1e3).toFixed(1) + 'K';
    return String(n);
  },

  formatUptime(seconds) {
    if (!seconds || seconds < 0) return '—';
    const d = Math.floor(seconds / 86400);
    const h = Math.floor((seconds % 86400) / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    if (d > 0) return `${d}d ${h}h ${m}m`;
    if (h > 0) return `${h}h ${m}m`;
    return `${m}m`;
  },

  formatBytes(bytes) {
    if (!bytes) return '—';
    if (bytes >= 1048576) return (bytes / 1048576).toFixed(1) + ' MB';
    if (bytes >= 1024) return (bytes / 1024).toFixed(0) + ' KB';
    return bytes + ' B';
  },

  formatTime(ts) {
    if (!ts) return '—';
    const d = new Date(ts);
    if (isNaN(d.getTime())) return '—';
    return d.toLocaleString(undefined, { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit' });
  },

  formatRelative(ts) {
    if (!ts) return '—';
    const d = new Date(ts);
    if (isNaN(d.getTime())) return '—';
    const diff = (Date.now() - d.getTime()) / 1000;
    if (diff < 60) return 'just now';
    if (diff < 3600) return Math.floor(diff / 60) + 'm ago';
    if (diff < 86400) return Math.floor(diff / 3600) + 'h ago';
    return Math.floor(diff / 86400) + 'd ago';
  },

  isValidDomain(domain) {
    if (!domain || typeof domain !== 'string') return false;
    domain = domain.trim().toLowerCase();
    if (domain.length < 3 || domain.length > 253) return false;
    return /^[a-z0-9]([a-z0-9-]*[a-z0-9])?(\.[a-z0-9]([a-z0-9-]*[a-z0-9])?)+$/.test(domain);
  },

  isValidIP(ip) {
    if (!ip) return false;
    return /^(\d{1,3}\.){3}\d{1,3}$/.test(ip) && ip.split('.').every(o => parseInt(o) >= 0 && parseInt(o) <= 255);
  },

  escapeHTML(str) {
    if (!str) return '';
    const div = document.createElement('div');
    div.textContent = str;
    return div.innerHTML;
  },

  debounce(fn, ms) {
    let timer;
    return function (...args) {
      clearTimeout(timer);
      timer = setTimeout(() => fn.apply(this, args), ms);
    };
  },

  animateNumber(el, target, suffix = '') {
    if (!el) return;
    const start = parseInt(el.dataset.value || '0', 10);
    const duration = 600;
    const startTime = performance.now();
    el.dataset.value = target;
    function tick(now) {
      const progress = Math.min((now - startTime) / duration, 1);
      const eased = 1 - Math.pow(1 - progress, 3);
      const current = Math.round(start + (target - start) * eased);
      el.textContent = Helpers.formatNumber(current) + suffix;
      if (progress < 1) requestAnimationFrame(tick);
      else el.textContent = Helpers.formatNumber(target) + suffix;
    }
    requestAnimationFrame(tick);
  }
};

/* ===== API Client ===== */
class APIClient {
  constructor() {
    this.baseURL = '';
  }

  async request(path, options = {}) {
    if (!App.apiKey) {
      App.promptAPIKey();
      throw new Error('Dashboard password required');
    }
    const headers = {
      'Content-Type': 'application/json',
      'X-API-Key': App.apiKey,
      ...(options.headers || {})
    };
    try {
      const res = await fetch(path, { ...options, headers });
      if (res.status === 401 || res.status === 403) {
        Toast.error('Authentication Failed', 'Invalid dashboard password. Please re-enter it.');
        App.promptAPIKey();
        throw new Error('Auth failed');
      }
      if (res.status === 204) return null;
      const text = await res.text();
      let data;
      try { data = text ? JSON.parse(text) : null; }
      catch { data = { raw: text }; }
      if (!res.ok) {
        const msg = (data && data.error) || `HTTP ${res.status}`;
        throw new Error(msg);
      }
      return data;
    } catch (err) {
      if (err.message === 'Failed to fetch') {
        Toast.error('Connection Error', 'Cannot reach ESP32 device. Check network.');
        App.setConnection(false);
      }
      throw err;
    }
  }

  get(path) { return this.request(path, { method: 'GET' }); }
  post(path, body) { return this.request(path, { method: 'POST', body: body ? JSON.stringify(body) : undefined }); }
  del(path, body) { return this.request(path, { method: 'DELETE', body: body ? JSON.stringify(body) : undefined }); }

  // Status & Stats
  getStatus() { return this.get('/api/status'); }
  getStats() { return this.get('/api/stats'); }

  // Blocklist
  getBlocklist(offset = 0, limit = 50, search = '') {
    let q = `/api/blocklist?offset=${offset}&limit=${limit}`;
    if (search) q += `&search=${encodeURIComponent(search)}`;
    return this.get(q);
  }
  addBlock(domain) { return this.post('/api/blocklist', { domain }); }
  removeBlock(domain) { return this.del('/api/blocklist', { domain }); }
  bulkBlock(domains) { return this.post('/api/blocklist/bulk', { domains }); }
  clearBlock() { return this.post('/api/blocklist/clear'); }

  // AdGuard
  adguardTest(domains, autoAdd = false) { return this.post('/api/adguard/test', { domains, autoAdd }); }
  adguardResults() { return this.get('/api/adguard/results'); }

  // Reports
  getReports() { return this.get('/api/reports'); }
  addReport(body) { return this.post('/api/reports', body); }
  verifyReport(domain) { return this.post('/api/reports/verify', { domain }); }
  dismissReport(domain) { return this.del('/api/reports', { domain }); }

  // Settings
  getSettings() { return this.get('/api/settings'); }
  saveSettings(body) { return this.post('/api/settings', body); }
}

const API = new APIClient();

/* ===== Toast System ===== */
const Toast = {
  show(title, msg, type = 'success', duration = 4000) {
    const container = document.getElementById('toast-container');
    if (!container) return;
    const icons = { success: '✓', error: '✕', warning: '⚠', info: 'ℹ' };
    const el = document.createElement('div');
    el.className = `toast ${type}`;
    el.innerHTML = `
      <span class="toast-icon">${icons[type] || icons.info}</span>
      <div class="toast-body">
        <div class="toast-title">${Helpers.escapeHTML(title)}</div>
        ${msg ? `<div class="toast-msg">${Helpers.escapeHTML(msg)}</div>` : ''}
      </div>`;
    container.appendChild(el);
    setTimeout(() => this.remove(el), duration);
    return el;
  },
  success(title, msg) { return this.show(title, msg, 'success'); },
  error(title, msg) { return this.show(title, msg, 'error', 6000); },
  warning(title, msg) { return this.show(title, msg, 'warning', 5000); },
  info(title, msg) { return this.show(title, msg, 'info'); },
  remove(el) {
    if (!el || !el.parentNode) return;
    el.classList.add('removing');
    setTimeout(() => el.remove(), 300);
  }
};

/* ===== Confirm Modal ===== */
App.confirm = function (title, message, onConfirm, okLabel = 'Confirm', okClass = 'btn-danger') {
  const overlay = document.getElementById('confirm-overlay');
  document.getElementById('confirm-title').textContent = title;
  document.getElementById('confirm-message').textContent = message;
  const okBtn = document.getElementById('confirm-ok');
  okBtn.textContent = okLabel;
  okBtn.className = 'btn ' + okClass;
  overlay.style.display = 'flex';
  const handler = () => {
    okBtn.removeEventListener('click', handler);
    overlay.style.display = 'none';
    onConfirm();
  };
  okBtn.addEventListener('click', handler);
  document.getElementById('confirm-cancel').onclick = () => {
    okBtn.removeEventListener('click', handler);
    overlay.style.display = 'none';
  };
};

/* ===== Connection Status ===== */
App.setConnection = function (connected) {
  const el = document.getElementById('connection-status');
  if (!el) return;
  const dot = el.querySelector('.status-dot');
  const text = el.querySelector('.status-text');
  dot.className = 'status-dot ' + (connected ? 'status-connected' : 'status-disconnected');
  text.textContent = connected ? 'Connected' : 'Disconnected';
};

/* ===== API Key Prompt ===== */
App.promptAPIKey = function () {
  const overlay = document.getElementById('api-key-overlay');
  const input = document.getElementById('api-key-input');
  overlay.style.display = 'flex';
  input.value = App.apiKey || '';
  input.focus();
};

App.saveAPIKey = function () {
  const input = document.getElementById('api-key-input');
  const key = input.value.trim();
  if (!key) {
    input.classList.add('error');
    Toast.error('Error', 'Please enter a dashboard password.');
    return;
  }
  input.classList.remove('error');
  App.apiKey = key;
  localStorage.setItem('esp32_api_key', key);
  document.getElementById('api-key-overlay').style.display = 'none';
  Toast.success('Connected', 'Dashboard password saved.');
  App.setConnection(true);
  App.router();
};

/* ===== Router ===== */
App.routes = {
  dashboard: { title: 'Dashboard', render: () => DashboardView.render() },
  overall: { title: 'Overall', render: () => OverallView.render() },
  blocklist: { title: 'Block List', render: () => BlocklistView.render() },
  router: { title: 'Router Reports', render: () => RouterView.render() },
  adguard: { title: 'Ad Blocker Test', render: () => AdguardView.render() },
  reports: { title: 'Ad Reports', render: () => ReportsView.render() },
  settings: { title: 'Settings', render: () => SettingsView.render() }
};

App.router = function () {
  const hash = (location.hash || '#dashboard').slice(1);
  const route = App.routes[hash] ? hash : 'dashboard';
  App.currentRoute = route;

  // Update nav active state
  document.querySelectorAll('.nav-item').forEach(item => {
    item.classList.toggle('active', item.dataset.route === route);
  });

  // Update title
  document.getElementById('page-title').textContent = App.routes[route].title;

  // Clear dashboard timer when leaving
  if (route !== 'dashboard' && App.dashboardTimer) {
    clearInterval(App.dashboardTimer);
    App.dashboardTimer = null;
  }

  // Close mobile sidebar
  document.getElementById('sidebar').classList.remove('open');
  document.getElementById('sidebar-backdrop').classList.remove('show');

  // Render
  if (!App.apiKey) {
    App.promptAPIKey();
    return;
  }
  try {
    App.routes[route].render();
  } catch (err) {
    console.error('Route render error:', err);
    const container = document.getElementById('view-container');
    container.innerHTML = `<div class="empty-state"><div class="empty-title">Error loading view</div><div class="muted">${Helpers.escapeHTML(err.message)}</div></div>`;
  }
};

window.addEventListener('hashchange', () => App.router());

/* ===== Mobile sidebar ===== */
document.getElementById('mobile-menu-toggle').addEventListener('click', () => {
  document.getElementById('sidebar').classList.toggle('open');
  document.getElementById('sidebar-backdrop').classList.toggle('show');
});
document.getElementById('sidebar-backdrop').addEventListener('click', () => {
  document.getElementById('sidebar').classList.remove('open');
  document.getElementById('sidebar-backdrop').classList.remove('show');
});

/* ===== API key buttons ===== */
document.getElementById('api-key-save').addEventListener('click', () => App.saveAPIKey());
document.getElementById('api-key-input').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') App.saveAPIKey();
});
document.getElementById('api-key-edit').addEventListener('click', () => App.promptAPIKey());


/* ===== Init ===== */
document.addEventListener('DOMContentLoaded', async () => {
  // Check if first-run wizard has been completed
  try {
    const resp = await fetch('/api/wizard/status');
    if (resp.ok) {
      const status = await resp.json();
      if (!status.wizardCompleted) {
        // Show first-run wizard (no API key needed)
        const sidebar = document.getElementById('sidebar');
        const topbar = document.getElementById('topbar');
        const pageTitle = document.getElementById('page-title');
        if (sidebar) sidebar.style.display = 'none';
        if (topbar) topbar.style.display = 'none';
        if (pageTitle) pageTitle.style.display = 'none';
        WizardView.render();
        return;
      }
    }
  } catch (err) {
    // If wizard check fails, continue with normal flow
  }

  // Normal flow: check API key
  if (!App.apiKey) {
    App.promptAPIKey();
  } else {
    App.setConnection(true);
    App.router();
  }
});
