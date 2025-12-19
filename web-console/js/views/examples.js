/**
 * @brief Dynamic curl examples tailored to current settings.
 */

import { RuntimeState, getLastResponse } from "../core/state.js";
import { buildCurl } from "../core/curl.js";
import { showJsonModal } from "../core/modal.js";

let container = null;
let payloadModel = null;
let latestResponse = null;

export function init() {
  container = document.getElementById("examples-view");
  container
    ?.querySelector("#examples-refresh")
    ?.addEventListener("click", () => {
      renderExamples();
      renderApiPanel();
    });

  container?.querySelector("#examples-payload-open")?.addEventListener("click", () => {
    showJsonModal("Payload /api/pin/set", payloadModel);
  });

  container?.querySelector("#examples-json-open")?.addEventListener("click", () => {
    showJsonModal("Firmware JSON", latestResponse);
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
  const curlPreview = container?.querySelector("#examples-curl");
  const responseHint = container?.querySelector("#examples-json-hint");
  const authBadge = container?.querySelector("#examples-auth-mode");

  const payload = { id: "GPIO12", mode: "Output", state: 0 };
  payloadModel = payload;

  if (curlPreview) {
    curlPreview.textContent = buildCurl("/api/pin/set", "PATCH", payload, RuntimeState.authEnabled);
  }

  if (authBadge) {
    authBadge.textContent = RuntimeState.authEnabled
      ? "Signing enabled per RuntimeState"
      : "Unsigned because auth is disabled";
  }

  latestResponse = getLastResponse("PATCH /api/pin/set") || getLastResponse("GET /api/state") || null;

  if (responseHint) {
    responseHint.textContent = latestResponse
      ? "Ultima risposta disponibile"
      : "Nessun JSON catturato";
  }
}

export default { init, destroy };
