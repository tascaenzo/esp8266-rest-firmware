/**
 * @brief Live device state inspector.
 *
 * Responsibilities:
 *  - Fetch and cache /api/state
 *  - Render device info, GPIO table and cron jobs
 *  - Update topbar badges
 *  - Surface errors recorded in RuntimeState
 */

import { fetchDeviceState } from "../core/api.js";
import { RuntimeState, isStateFresh, getLastResponse } from "../core/state.js";
import { buildCurl } from "../core/curl.js";

let container = null;
let refreshBtn = null;
const STATE_SIGNATURE = "GET /api/state";

export function init() {
  container = document.getElementById("device-state-view");
  refreshBtn = container?.querySelector("#state-refresh");

  bindEvents();
  renderFromCache();
  loadState();
}

/**
 * @brief Unbind transient listeners.
 */
export function destroy() {
  refreshBtn?.removeEventListener("click", loadState);
}

function bindEvents() {
  if (refreshBtn) {
    refreshBtn.addEventListener("click", loadState);
  }
}

/**
 * @brief Render using cached state (if any) without hitting the device.
 */
function renderFromCache() {
  if (RuntimeState.deviceState) {
    render(RuntimeState.deviceState);
  }

  renderApiDetails(RuntimeState.deviceState);
}

/**
 * @brief Fetch /api/state and refresh UI.
 */
async function loadState() {
  if (!container) return;

  setBusy(true);
  try {
    if (!isStateFresh(1500)) {
      await fetchDeviceState();
    }
    render(RuntimeState.deviceState);
    renderApiDetails(RuntimeState.deviceState);
  } catch (err) {
    renderError(err.message);
    renderApiDetails(RuntimeState.deviceState);
  } finally {
    setBusy(false);
  }
}

function setBusy(isBusy) {
  if (!refreshBtn) return;
  refreshBtn.disabled = isBusy;
  refreshBtn.textContent = isBusy ? "Refreshing..." : "Refresh";
}

/**
 * @brief Render main device info, GPIO and cron tables.
 */
function render(state) {
  if (!container) return;

  const infoBox = container.querySelector("#device-meta");
  const gpioBody = container.querySelector("#gpio-table tbody");
  const cronBody = container.querySelector("#cron-table tbody");
  const freshness = container.querySelector("#state-freshness");

  const errorBox = container.querySelector("#state-error");
  if (RuntimeState.lastError) {
    errorBox.textContent = `${RuntimeState.lastError.source || "API"}: ${RuntimeState.lastError.message}`;
    errorBox.classList.remove("hidden");
  } else {
    errorBox.classList.add("hidden");
  }

  if (!state) {
    infoBox.innerHTML = `<p class="muted">No device state loaded yet.</p>`;
    gpioBody.innerHTML = "";
    cronBody.innerHTML = "";
    freshness.textContent = "—";
    updateTopbar(null);
    renderApiDetails(null);
    return;
  }

  const device = state.device || {};
  infoBox.innerHTML = `
    <div class="meta">
      <div><span class="label">IP</span><strong>${device.ip || "?"}</strong></div>
      <div><span class="label">Chip</span>${device.device || "ESP8266"}</div>
      <div><span class="label">RSSI</span>${device.rssi ?? "?"} dBm</div>
      <div><span class="label">Uptime</span>${formatUptime(device.uptime)}</div>
      <div><span class="label">Auth</span>${device.auth ? "enabled" : "disabled"}</div>
    </div>
  `;

  updateTopbar(device);

  gpioBody.innerHTML = renderGpioRows(state.pins || {});
  cronBody.innerHTML = renderCronRows(state.cronJobs || {});

  freshness.textContent = new Date(RuntimeState.lastUpdateTs).toLocaleTimeString();
}

function renderApiDetails(state) {
  if (!container) return;

  const curlEl = container.querySelector("#state-curl");
  const responseEl = container.querySelector("#state-json");
  const metaEl = container.querySelector("#state-auth-mode");

  if (curlEl) {
    curlEl.textContent = buildCurl("/api/state", "GET", null, RuntimeState.authEnabled);
  }

  if (metaEl) {
    metaEl.textContent = RuntimeState.authEnabled
      ? "Auth headers are attached automatically"
      : "Request sent unsigned (device reports auth disabled)";
  }

  if (responseEl) {
    const raw = getLastResponse(STATE_SIGNATURE) || state;
    responseEl.textContent = raw
      ? JSON.stringify(raw, null, 2)
      : "No response captured yet.";
  }
}

function renderError(message) {
  const errorBox = container?.querySelector("#state-error");
  if (!errorBox) return;

  errorBox.textContent = message;
  errorBox.classList.remove("hidden");
}

function renderGpioRows(pins) {
  const entries = Object.entries(pins);
  if (entries.length === 0) {
    return `<tr><td colspan="5" class="muted">No GPIO entries reported.</td></tr>`;
  }

  return entries
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([id, pin]) => {
      const caps = Array.isArray(pin.capabilities)
        ? pin.capabilities.join(", ")
        : "—";
      const safetyClass = safetyToClass(pin.safety);
      return `
        <tr>
          <td>${id}</td>
          <td>${pin.mode || "?"}</td>
          <td>${pin.state ?? "?"}</td>
          <td>${caps}</td>
          <td><span class="badge ${safetyClass}">${pin.safety || "Unknown"}</span></td>
        </tr>
      `;
    })
    .join("");
}

function renderCronRows(jobs) {
  const entries = Object.entries(jobs);
  if (entries.length === 0) {
    return `<tr><td colspan="5" class="muted">No cron jobs configured.</td></tr>`;
  }

  return entries
    .sort(([a], [b]) => Number(a) - Number(b))
    .map(([slot, job]) => {
      const statusClass = job.state === "Active" ? "badge-ok" : "badge-warn";
      const target = job.pin ? `${job.action} ${job.pin}` : job.action;
      const value = job.value !== undefined ? ` → ${job.value}` : "";
      return `
        <tr>
          <td>${slot}</td>
          <td>${job.cron}</td>
          <td>${target}${value}</td>
          <td>${job.state || "?"}</td>
          <td><span class="badge ${statusClass}">${job.action}</span></td>
        </tr>
      `;
    })
    .join("");
}

function safetyToClass(level) {
  switch (level) {
    case "Safe":
      return "badge-ok";
    case "Warn":
      return "badge-warn";
    case "BootSensitive":
      return "badge-error";
    default:
      return "";
  }
}

function formatUptime(seconds) {
  if (!Number.isFinite(seconds)) return "?";
  const hrs = Math.floor(seconds / 3600);
  const mins = Math.floor((seconds % 3600) / 60);
  const secs = Math.floor(seconds % 60);
  return `${hrs}h ${mins}m ${secs}s`;
}

function updateTopbar(device) {
  const ipEl = document.getElementById("device-ip");
  const authEl = document.getElementById("auth-status");

  if (!ipEl || !authEl) return;

  const ipText = device?.ip || "—";
  const authEnabled = device?.auth ?? RuntimeState.authEnabled;

  ipEl.textContent = ipText;
  authEl.textContent = authEnabled ? "Auth: enabled" : "Auth: disabled";
  authEl.classList.toggle("badge-ok", authEnabled);
  authEl.classList.toggle("badge-warn", !authEnabled);
}

export default { init, destroy };
