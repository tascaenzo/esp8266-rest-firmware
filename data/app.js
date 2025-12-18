/* eslint-disable no-console */

// Configurable base URL for the device REST API
const BASE_URL = "http://192.168.1.8";

// LocalStorage key
const STORAGE_KEY = "esp8266AuthKey";

// Cached DOM references
const els = {
  authStatus: document.getElementById("auth-status"),
  baseUrl: document.getElementById("base-url"),
  setupAuth: document.getElementById("setup-auth"),
  setupSerial: document.getElementById("setup-serial"),
  applySetup: document.getElementById("apply-setup"),
  setupAuthKey: document.getElementById("setup-auth-key"),
  setupAuthKeyValue: document.getElementById("setup-auth-key-value"),
  authInput: document.getElementById("auth-key-input"),
  saveAuthKey: document.getElementById("save-auth-key"),
  clearAuthKey: document.getElementById("clear-auth-key"),
  refreshState: document.getElementById("refresh-state"),
  stateGrid: document.getElementById("state-grid"),
  gpioList: document.getElementById("gpio-list"),
  cronList: document.getElementById("cron-list"),
  refreshGpio: document.getElementById("refresh-gpio"),
  refreshCron: document.getElementById("refresh-cron"),
  cronExpression: document.getElementById("cron-expression"),
  cronAction: document.getElementById("cron-action"),
  cronPin: document.getElementById("cron-pin"),
  cronValue: document.getElementById("cron-value"),
  addCron: document.getElementById("add-cron"),
  logOutput: document.getElementById("log-output"),
};

let lastState = null;

function addLog(message, type = "info") {
  const entry = document.createElement("div");
  entry.className = `log-entry ${type === "error" ? "log-error" : ""}`;
  const time = new Date().toLocaleTimeString();
  entry.innerHTML = `<span class="log-time">${time}</span>${message}`;
  els.logOutput.prepend(entry);
}

function getAuthKey() {
  return (els.authInput.value || localStorage.getItem(STORAGE_KEY) || "").trim().toLowerCase();
}

function hasAuthKey() {
  return /^[0-9a-f]{64}$/.test(getAuthKey());
}

function updateAuthBadge() {
  if (hasAuthKey()) {
    els.authStatus.textContent = "Auth key: stored";
    els.authStatus.classList.remove("badge-warn");
    els.authStatus.classList.add("badge-muted");
  } else {
    els.authStatus.textContent = "Auth key: missing";
    els.authStatus.classList.add("badge-warn");
    els.authStatus.classList.remove("badge-muted");
  }
}

function showSetupKey(key) {
  if (key) {
    els.setupAuthKey.classList.remove("hidden");
    els.setupAuthKeyValue.textContent = key;
  } else {
    els.setupAuthKey.classList.add("hidden");
    els.setupAuthKeyValue.textContent = "";
  }
}

function formatUptime(seconds) {
  const s = Number(seconds) || 0;
  const d = Math.floor(s / 86400);
  const h = Math.floor((s % 86400) / 3600);
  const m = Math.floor((s % 3600) / 60);
  const parts = [];
  if (d) parts.push(`${d}d`);
  if (h) parts.push(`${h}h`);
  if (m || parts.length === 0) parts.push(`${m}m`);
  return parts.join(" ");
}

async function requestNonce() {
  const res = await fetch(`${BASE_URL}/api/auth/challenge`);
  if (!res.ok) {
    throw new Error(`Nonce request failed (${res.status})`);
  }
  const data = await res.json();
  return String(data.nonce);
}

async function apiFetch(path, opts = {}) {
  const { method = "GET", body = null, requireAuth = false, preferAuth = false } = opts;
  const headers = opts.headers ? { ...opts.headers } : {};
  let payload = "";

  if (body !== null && body !== undefined) {
    payload = JSON.stringify(body);
    headers["Content-Type"] = "application/json";
  }

  const authKey = getAuthKey();
  const shouldSign = requireAuth || (preferAuth && authKey);

  if (shouldSign && !authKey) {
    throw new Error("Auth key required for this action");
  }

  if (shouldSign) {
    const nonce = await requestNonce();
    const signature = await hmacSha256Hex(authKey, nonce + path + payload);
    headers["X-Nonce"] = nonce;
    headers["X-Auth"] = signature;
  }

  const res = await fetch(`${BASE_URL}${path}`, {
    method,
    headers,
    body: payload === "" ? undefined : payload,
  });

  const contentType = res.headers.get("content-type") || "";
  const isJson = contentType.includes("application/json");
  const data = isJson ? await res.json() : await res.text();

  if (!res.ok) {
    const detail = typeof data === "string" ? data : JSON.stringify(data);
    throw new Error(`HTTP ${res.status}: ${detail}`);
  }
  return data;
}

function renderState(state) {
  els.stateGrid.innerHTML = "";
  const { device = {} } = state || {};
  const entries = [
    { label: "IP", value: device.ip || "—" },
    { label: "Chip ID", value: device.chip ?? "—" },
    { label: "RSSI", value: device.rssi != null ? `${device.rssi} dBm` : "—" },
    { label: "Uptime", value: formatUptime(device.uptime) },
    { label: "Auth enabled", value: device.auth ? "Yes" : "No" },
    { label: "Serial debug", value: device.serialDebug ? "On" : "Off" },
  ];

  entries.forEach((entry) => {
    const item = document.createElement("div");
    item.className = "info-item";
    item.innerHTML = `<div class="label">${entry.label}</div><div class="value">${entry.value}</div>`;
    els.stateGrid.appendChild(item);
  });

  if (typeof device.auth === "boolean") {
    els.setupAuth.checked = !!device.auth;
  }
  if (typeof device.serialDebug === "boolean") {
    els.setupSerial.checked = !!device.serialDebug;
  }
}

function renderPins(state) {
  els.gpioList.innerHTML = "";
  const pins = state?.pins || {};
  const hasKey = hasAuthKey();

  Object.entries(pins).forEach(([id, pin]) => {
    const card = document.createElement("div");
    card.className = "pin-card";

    const safetyClass =
      pin.safety === "Safe"
        ? "pill-success"
        : pin.safety === "Warn"
          ? "pill-warn"
          : "pill-danger";

    card.innerHTML = `
      <div class="pin-header">
        <div>
          <h3>${id}</h3>
          <div class="small-text">${(pin.capabilities || []).join(" · ")}</div>
        </div>
        <div class="pill ${safetyClass}">${pin.safety || "Unknown"}</div>
      </div>
      <div class="controls"></div>
    `;

    const controls = card.querySelector(".controls");

    // Mode selector
    const modeWrap = document.createElement("div");
    const modeLabel = document.createElement("label");
    modeLabel.textContent = "Mode";
    const modeSelect = document.createElement("select");
    const modes = ["Disabled", ...(pin.capabilities || [])];
    const currentMode = pin.mode || "Disabled";
    modes.forEach((mode) => {
      const opt = document.createElement("option");
      opt.value = mode;
      opt.textContent = mode;
      if (mode === currentMode) opt.selected = true;
      modeSelect.appendChild(opt);
    });
    modeSelect.disabled = !hasKey;
    modeWrap.append(modeLabel, modeSelect);
    controls.appendChild(modeWrap);

    // State / PWM control
    const stateWrap = document.createElement("div");
    const stateLabel = document.createElement("label");
    stateLabel.textContent = "State";

    const modeLower = currentMode.toLowerCase();
    if (modeLower === "output") {
      const btn = document.createElement("button");
      btn.className = "primary";
      btn.textContent = pin.state ? "Turn OFF" : "Turn ON";
      btn.disabled = !hasKey;
      btn.addEventListener("click", async () => {
        const next = pin.state ? 0 : 1;
        await updatePin(id, "Output", next);
      });
      stateWrap.append(stateLabel, btn);
    } else if (modeLower === "pwm") {
      const rangeRow = document.createElement("div");
      rangeRow.className = "range-row";
      const range = document.createElement("input");
      range.type = "range";
      range.min = "0";
      range.max = "1023";
      range.value = pin.state ?? 0;
      range.disabled = !hasKey;
      const valueLabel = document.createElement("span");
      valueLabel.textContent = range.value;
      range.addEventListener("input", () => {
        valueLabel.textContent = range.value;
      });
      range.addEventListener("change", async () => {
        await updatePin(id, "Pwm", Number(range.value));
      });
      rangeRow.append(range, valueLabel);
      stateWrap.append(stateLabel, rangeRow);
    } else {
      const readOnly = document.createElement("div");
      readOnly.className = "pill";
      readOnly.textContent = pin.state != null ? pin.state : "—";
      stateWrap.append(stateLabel, readOnly);
    }
    controls.appendChild(stateWrap);

    modeSelect.addEventListener("change", async () => {
      const newMode = modeSelect.value;
      if (newMode.toLowerCase() === "output") {
        await updatePin(id, newMode, pin.state ?? 0);
      } else if (newMode.toLowerCase() === "pwm") {
        await updatePin(id, newMode, Number(pin.state ?? 0));
      } else {
        await updatePin(id, newMode, undefined);
      }
    });

    els.gpioList.appendChild(card);
  });

  if (Object.keys(pins).length === 0) {
    els.gpioList.innerHTML = "<div class='small-text'>No pins reported.</div>";
  }
}

function renderCron(state) {
  els.cronList.innerHTML = "";
  const cron = state?.cronJobs || {};
  const hasKey = hasAuthKey();

  Object.entries(cron).forEach(([id, job]) => {
    const card = document.createElement("div");
    card.className = "cron-card";
    card.innerHTML = `
      <div class="cron-header">
        <div>
          <h3>Cron #${id}</h3>
          <div class="small-text">${job.cron || "?"}</div>
        </div>
        <div class="pill ${job.state === "Active" ? "pill-success" : "pill-warn"}">${job.state || "Unknown"}</div>
      </div>
      <div class="small-text">Action: ${job.action || "?"} · Pin: ${job.pin ?? "—"} · Value: ${job.value ?? "—"}</div>
      <div class="actions">
        <button class="danger" ${hasKey ? "" : "disabled"} data-id="${id}">Delete</button>
      </div>
    `;
    const delBtn = card.querySelector("button");
    delBtn.addEventListener("click", async () => {
      await deleteCron(id);
    });
    els.cronList.appendChild(card);
  });

  if (Object.keys(cron).length === 0) {
    els.cronList.innerHTML = "<div class='small-text'>No cron jobs configured.</div>";
  }
}

async function fetchState() {
  try {
    const data = await apiFetch("/api/state", { preferAuth: true });
    lastState = data;
    renderState(data);
    renderPins(data);
    renderCron(data);
    addLog("State updated");
  } catch (err) {
    console.error(err);
    addLog(err.message, "error");
  }
}

async function applySetup() {
  try {
    const payload = {
      auth: !!els.setupAuth.checked,
      serialDebug: !!els.setupSerial.checked,
    };
    const data = await apiFetch("/api/setup", { method: "POST", body: payload, requireAuth: payload.auth });
    showSetupKey(data.authKey);
    addLog("Setup applied");
    await fetchState();
  } catch (err) {
    console.error(err);
    showSetupKey(null);
    addLog(`Setup error: ${err.message}`, "error");
  }
}

async function updatePin(id, mode, state) {
  try {
    const payload = { id, mode };
    if (state !== undefined) payload.state = state;
    await apiFetch("/api/pin/set", { method: "PATCH", body: payload, requireAuth: true });
    addLog(`Updated ${id} → ${mode}${state !== undefined ? ` (${state})` : ""}`);
    await fetchState();
  } catch (err) {
    console.error(err);
    addLog(`Pin update failed: ${err.message}`, "error");
  }
}

async function addCronJob() {
  try {
    const payload = {
      cron: els.cronExpression.value.trim(),
      action: els.cronAction.value,
      pin: els.cronPin.value.trim(),
      value: els.cronValue.value === "" ? undefined : Number(els.cronValue.value),
    };
    await apiFetch("/api/cron/set", { method: "PATCH", body: payload, requireAuth: true });
    addLog("Cron job added");
    els.cronExpression.value = "";
    els.cronPin.value = "";
    els.cronValue.value = "";
    await fetchState();
  } catch (err) {
    console.error(err);
    addLog(`Cron add failed: ${err.message}`, "error");
  }
}

async function deleteCron(id) {
  try {
    await apiFetch(`/api/cron?id=${encodeURIComponent(id)}`, { method: "DELETE", requireAuth: true });
    addLog(`Cron #${id} deleted`);
    await fetchState();
  } catch (err) {
    console.error(err);
    addLog(`Cron delete failed: ${err.message}`, "error");
  }
}

function initAuthControls() {
  const stored = localStorage.getItem(STORAGE_KEY) || "";
  if (stored) {
    els.authInput.value = stored;
  }
  updateAuthBadge();

  els.saveAuthKey.addEventListener("click", () => {
    const key = els.authInput.value.trim().toLowerCase();
    if (!/^[0-9a-f]{64}$/.test(key)) {
      addLog("Invalid key. Must be 64 hex chars.", "error");
      return;
    }
    localStorage.setItem(STORAGE_KEY, key);
    updateAuthBadge();
    addLog("Auth key saved");
  });

  els.clearAuthKey.addEventListener("click", () => {
    localStorage.removeItem(STORAGE_KEY);
    els.authInput.value = "";
    updateAuthBadge();
    addLog("Auth key cleared");
  });
}

function initEventListeners() {
  els.applySetup.addEventListener("click", applySetup);
  els.refreshState.addEventListener("click", fetchState);
  els.refreshGpio.addEventListener("click", fetchState);
  els.refreshCron.addEventListener("click", fetchState);
  els.addCron.addEventListener("click", addCronJob);
}

function init() {
  els.baseUrl.textContent = BASE_URL;
  initAuthControls();
  initEventListeners();
  fetchState();
}

document.addEventListener("DOMContentLoaded", init);
