/**
 * @brief Dynamic curl examples tailored to current settings.
 */

import { RuntimeState, getLastResponse } from "../core/state.js";
import { buildCurl } from "../core/curl.js";

let container = null;

export function init() {
  container = document.getElementById("examples-view");
  container
    ?.querySelector("#examples-refresh")
    ?.addEventListener("click", () => {
      renderExamples();
      renderApiPanel();
    });

  renderExamples();
  renderApiPanel();
}

export function destroy() {}

function renderExamples() {
  const grid = container?.querySelector("#examples-list");
  if (!grid) return;

  const authEnabled = RuntimeState.authEnabled;

  const examples = [
    {
      title: "Read device state",
      desc: "GET /api/state returns GPIO table and cron jobs.",
      cmd: buildCurl("/api/state", "GET", null, authEnabled),
    },
    {
      title: "Set GPIO output",
      desc: "PATCH /api/pin/set with deterministic JSON payload.",
      cmd: buildCurl("/api/pin/set", "PATCH", { id: "GPIO4", mode: "Output", state: 1 }, authEnabled),
    },
    {
      title: "Schedule cron job",
      desc: "PATCH /api/cron/set to toggle GPIO4 every 5 minutes.",
      cmd: buildCurl(
        "/api/cron/set",
        "PATCH",
        { id: 0, cron: "*/5 * * * *", action: "toggle", pin: "GPIO4" },
        authEnabled
      ),
    },
  ];

  grid.innerHTML = examples
    .map(
      (item) => `
        <article class="card surface">
          <div class="eyebrow">${item.title}</div>
          <p class="muted">${item.desc}</p>
          <pre class="code"><code>${item.cmd}</code></pre>
        </article>
      `
    )
    .join("");

  const status = document.createElement("p");
  status.className = "muted";
  status.textContent = authEnabled
    ? "Auth headers are included automatically based on RuntimeState.authEnabled."
    : "Auth is currently disabled according to the last /api/state payload.";

  grid.appendChild(status);
}

function renderApiPanel() {
  const payloadPreview = container?.querySelector("#examples-payload");
  const curlPreview = container?.querySelector("#examples-curl");
  const responsePreview = container?.querySelector("#examples-json");
  const authBadge = container?.querySelector("#examples-auth-mode");

  const payload = { id: "GPIO12", mode: "Output", state: 0 };

  if (payloadPreview) {
    payloadPreview.textContent = JSON.stringify(payload, null, 2);
  }

  if (curlPreview) {
    curlPreview.textContent = buildCurl("/api/pin/set", "PATCH", payload, RuntimeState.authEnabled);
  }

  if (authBadge) {
    authBadge.textContent = RuntimeState.authEnabled
      ? "Signing enabled per RuntimeState"
      : "Unsigned because auth is disabled";
  }

  if (responsePreview) {
    const raw = getLastResponse("PATCH /api/pin/set") || getLastResponse("GET /api/state");
    responsePreview.textContent = raw
      ? JSON.stringify(raw, null, 2)
      : "No firmware JSON captured yet. Use the other views to trigger calls.";
  }
}

export default { init, destroy };
