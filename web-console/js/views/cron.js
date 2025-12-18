/**
 * @brief Cron scheduler inspector.
 */

import { fetchDeviceState, setCron, deleteCron } from "../core/api.js";
import { RuntimeState, isStateFresh } from "../core/state.js";

let container = null;

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
}

async function refresh(force = false) {
  if (!container) return;
  try {
    if (force || !isStateFresh(2000)) {
      await fetchDeviceState();
    }
    renderTable(RuntimeState.deviceState?.cronJobs || {});
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

export default { init, destroy };
