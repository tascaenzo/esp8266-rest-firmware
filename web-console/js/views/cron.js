/**
 * @brief Cron scheduler inspector.
 */

import { fetchDeviceState, setCron, deleteCron } from "../core/api.js";
import { RuntimeState, isStateFresh, getLastResponse } from "../core/state.js";
import { buildCurl } from "../core/curl.js";
import { showJsonModal } from "../core/modal.js";

let container = null;
const STATE_SIGNATURE = "GET /api/state";
const UPSERT_SIGNATURE = "PATCH /api/cron/set";
const DELETE_SIGNATURE = "DELETE /api/cron";
let latestReadResponse = null;
let latestUpsertResponse = null;
let latestDeleteResponse = null;
let latestPayload = null;

export function init() {
  container = document.getElementById("cron-view");
  bindActions();
  refresh();
}

export function destroy() {}

function bindActions() {
  container
    ?.querySelector("#cron-refresh")
    ?.addEventListener("click", () => refresh(true));
  container
    ?.querySelector("#cron-apply")
    ?.addEventListener("click", onApply);

  container
    ?.querySelectorAll(".cron-input")
    .forEach((input) => input.addEventListener("input", renderApiDocs));

  container?.querySelector("#cron-read-open")?.addEventListener("click", () => {
    showJsonModal("/api/state", latestReadResponse);
  });

  container?.querySelector("#cron-upsert-open")?.addEventListener("click", () => {
    showJsonModal("/api/cron/set", latestUpsertResponse);
  });

  container?.querySelector("#cron-delete-open")?.addEventListener("click", () => {
    showJsonModal("DELETE /api/cron", latestDeleteResponse);
  });

  container?.querySelector("#cron-payload-open")?.addEventListener("click", () => {
    showJsonModal("Payload /api/cron/set", latestPayload);
  });
}

async function refresh(force = false) {
  if (!container) return;
  try {
    if (force || !isStateFresh(2000)) {
      await fetchDeviceState();
    }
    renderTable(RuntimeState.deviceState?.cronJobs || {});
    renderApiDocs();
    renderResponsePanels();
  } catch (err) {
    renderStatus(`Failed to load jobs: ${err.message}`, true);
  }
}

function renderTable(jobs) {
  const body = container.querySelector("#cron-table tbody");
  const entries = Object.entries(jobs);

  if (entries.length === 0) {
    body.innerHTML = `<tr><td colspan="7" class="muted">No cron jobs configured.</td></tr>`;
    return;
  }

  body.innerHTML = entries
    .sort(([a], [b]) => Number(a) - Number(b))
    .map(([slot, job]) => {
      const badgeClass = job.state === "Active" ? "badge-ok" : "badge-warn";
      return `
        <tr>
          <td>${slot}</td>
          <td>${job.cron}</td>
          <td>${job.action}</td>
          <td>${job.pin || "—"}</td>
          <td>${job.value ?? "—"}</td>
          <td><span class="badge ${badgeClass}">${job.state}</span></td>
          <td><button class="btn-tertiary" data-slot="${slot}">Delete</button></td>
        </tr>
      `;
    })
    .join("");

  body.querySelectorAll("button[data-slot]").forEach((btn) => {
    btn.addEventListener("click", () => onDelete(btn.dataset.slot));
  });
}

async function onApply() {
  const slot = container.querySelector("#cron-slot").value;
  const cron = container.querySelector("#cron-string").value.trim();
  const action = container.querySelector("#cron-action").value.trim();
  const pin = container.querySelector("#cron-pin").value.trim();
  const valueRaw = container.querySelector("#cron-value").value;

  if (cron.length === 0 || action.length === 0) {
    renderStatus("Cron string and action are required", true);
    return;
  }

  const payload = {
    id: Number(slot),
    cron,
    action,
  };

  if (pin) payload.pin = pin;
  if (valueRaw !== "") payload.value = isNaN(valueRaw) ? valueRaw : Number(valueRaw);

  renderStatus("Applying...");
  try {
    await setCron(payload);
    renderStatus("Job stored");
    renderResponsePanels();
    refresh(true);
  } catch (err) {
    renderStatus(err.message, true);
  }
}

async function onDelete(slot) {
  renderStatus(`Deleting slot ${slot}...`);
  try {
    await deleteCron(slot);
    renderStatus("Job removed");
    renderResponsePanels();
    refresh(true);
  } catch (err) {
    renderStatus(err.message, true);
  }
}

function renderStatus(message, isError = false) {
  const el = container?.querySelector("#cron-status");
  if (!el) return;
  el.textContent = message;
  el.classList.toggle("error", isError);
}

function renderApiDocs() {
  const stateCurl = container?.querySelector("#cron-curl-state");
  const upsertCurl = container?.querySelector("#cron-curl-upsert");
  const deleteCurl = container?.querySelector("#cron-curl-delete");

  if (stateCurl) {
    stateCurl.textContent = buildCurl("/api/state", "GET", null, RuntimeState.authEnabled);
  }

  const payload = collectPayload();
  if (upsertCurl) {
    upsertCurl.textContent = buildCurl("/api/cron/set", "PATCH", payload, RuntimeState.authEnabled);
  }

  if (deleteCurl) {
    deleteCurl.textContent = buildCurl(`/api/cron?id=${payload.id}`, "DELETE", null, RuntimeState.authEnabled);
  }

  latestPayload = payload;
}

function renderResponsePanels() {
  const readHint = container?.querySelector("#cron-read-hint");
  const upsertHint = container?.querySelector("#cron-upsert-hint");
  const deleteHint = container?.querySelector("#cron-delete-hint");

  latestReadResponse = getLastResponse(STATE_SIGNATURE) || null;
  latestUpsertResponse = getLastResponse(UPSERT_SIGNATURE) || null;
  latestDeleteResponse = getLastResponse(DELETE_SIGNATURE) || null;

  if (readHint) {
    readHint.textContent = latestReadResponse ? "Ultima risposta disponibile" : "Nessuna risposta ancora";
  }

  if (upsertHint) {
    upsertHint.textContent = latestUpsertResponse ? "Ultima risposta disponibile" : "Nessuna scrittura ancora";
  }

  if (deleteHint) {
    deleteHint.textContent = latestDeleteResponse ? "Ultima risposta disponibile" : "Nessuna cancellazione ancora";
  }
}

function collectPayload() {
  const slot = container?.querySelector("#cron-slot")?.value || 0;
  const cron = container?.querySelector("#cron-string")?.value.trim() || "*/5 * * * *";
  const action = container?.querySelector("#cron-action")?.value.trim() || "toggle";
  const pin = container?.querySelector("#cron-pin")?.value.trim();
  const rawValue = container?.querySelector("#cron-value")?.value;

  const payload = { id: Number(slot), cron, action };
  if (pin) payload.pin = pin;
  if (rawValue !== undefined && rawValue !== "") {
    payload.value = isNaN(rawValue) ? rawValue : Number(rawValue);
  }

  return payload;
}

export default { init, destroy };
