/**
 * @brief GPIO inspector and writer.
 */

import { fetchDeviceState, setPin } from "../core/api.js";
import { RuntimeState, isStateFresh, getLastResponse } from "../core/state.js";
import { buildCurl } from "../core/curl.js";

let container = null;
const STATE_SIGNATURE = "GET /api/state";
const WRITE_SIGNATURE = "PATCH /api/pin/set";

export function init() {
  container = document.getElementById("gpio-view");
  bindActions();
  refresh();
}

export function destroy() {}

function bindActions() {
  container
    ?.querySelector("#gpio-refresh")
    ?.addEventListener("click", () => refresh(true));
  container
    ?.querySelector("#gpio-apply")
    ?.addEventListener("click", onManualWrite);

  container
    ?.querySelectorAll(".gpio-input")
    .forEach((input) => input.addEventListener("input", renderApiDocs));
}

async function refresh(force = false) {
  if (!container) return;

  try {
    if (force || !isStateFresh(2000)) {
      await fetchDeviceState();
    }
    renderTable(RuntimeState.deviceState?.pins || {});
    renderResponsePanels();
    renderApiDocs();
  } catch (err) {
    renderStatus(`Failed to load pins: ${err.message}`, true);
  }
}

function renderTable(pins) {
  const body = container.querySelector("#gpio-table tbody");
  const entries = Object.entries(pins);

  if (entries.length === 0) {
    body.innerHTML = `<tr><td colspan="5" class="muted">No GPIO data available.</td></tr>`;
    return;
  }

  body.innerHTML = entries
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([id, pin]) => {
      const safetyClass = safetyToClass(pin.safety);
      return `
        <tr>
          <td>${id}</td>
          <td>${pin.mode || "?"}</td>
          <td>${pin.state ?? "?"}</td>
          <td><span class="badge ${safetyClass}">${pin.safety || "Unknown"}</span></td>
          <td><button class="btn-tertiary" data-pin="${id}" data-mode="${pin.mode || ""}" data-state="${pin.state ?? ""}">Load</button></td>
        </tr>
      `;
    })
    .join("");

  body.querySelectorAll("button[data-pin]").forEach((btn) => {
    btn.addEventListener("click", () => populateForm(btn.dataset));
  });
}

function populateForm({ pin, mode, state }) {
  container.querySelector("#gpio-id").value = pin || "";
  container.querySelector("#gpio-mode").value = mode || "";
  container.querySelector("#gpio-value").value = state ?? "";
  renderStatus(`Loaded ${pin} into manual form`, false);
}

async function onManualWrite() {
  const pin = container.querySelector("#gpio-id").value.trim();
  const mode = container.querySelector("#gpio-mode").value.trim();
  const value = container.querySelector("#gpio-value").value;

  if (!pin) {
    renderStatus("Pin ID is required", true);
    return;
  }

  const payload = { id: pin };
  if (mode) payload.mode = mode;
  if (value !== "") payload.state = Number(value);

  renderStatus("Applying...");
  try {
    await setPin(payload);
    renderStatus("Pin updated");
    renderResponsePanels();
    refresh(true);
  } catch (err) {
    renderStatus(err.message, true);
  }
}

function renderStatus(message, isError = false) {
  const el = container?.querySelector("#gpio-status");
  if (!el) return;
  el.textContent = message;
  el.classList.toggle("error", isError);
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

function renderApiDocs() {
  const readCurl = container?.querySelector("#gpio-curl-read");
  const writeCurl = container?.querySelector("#gpio-curl-write");
  const payloadEl = container?.querySelector("#gpio-payload");

  if (readCurl) {
    readCurl.textContent = buildCurl("/api/state", "GET", null, RuntimeState.authEnabled);
  }

  const payload = collectManualPayload();
  if (writeCurl) {
    writeCurl.textContent = buildCurl("/api/pin/set", "PATCH", payload, RuntimeState.authEnabled);
  }

  if (payloadEl) {
    payloadEl.textContent = JSON.stringify(payload, null, 2);
  }
}

function renderResponsePanels() {
  const readPre = container?.querySelector("#gpio-read-json");
  const writePre = container?.querySelector("#gpio-write-json");

  if (readPre) {
    const raw = getLastResponse(STATE_SIGNATURE);
    readPre.textContent = raw ? JSON.stringify(raw, null, 2) : "No state response yet.";
  }

  if (writePre) {
    const raw = getLastResponse(WRITE_SIGNATURE);
    writePre.textContent = raw ? JSON.stringify(raw, null, 2) : "No write response yet.";
  }
}

function collectManualPayload() {
  const pin = container?.querySelector("#gpio-id")?.value.trim() || "GPIO4";
  const mode = container?.querySelector("#gpio-mode")?.value.trim();
  const value = container?.querySelector("#gpio-value")?.value;

  const payload = { id: pin };
  if (mode) payload.mode = mode;
  if (value !== undefined && value !== "") payload.state = Number(value);

  return payload;
}

export default { init, destroy };
