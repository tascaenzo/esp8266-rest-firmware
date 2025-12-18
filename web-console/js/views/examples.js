/**
 * @brief Dynamic curl examples tailored to current settings.
 */

import { RuntimeState } from "../core/state.js";
import { getBaseUrl } from "../core/config.js";

let container = null;

export function init() {
  container = document.getElementById("examples-view");
  container
    ?.querySelector("#examples-refresh")
    ?.addEventListener("click", renderExamples);
  renderExamples();
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
        <article class="card">
          <h3>${item.title}</h3>
          <p class="muted">${item.desc}</p>
          <pre><code>${item.cmd}</code></pre>
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

function buildCurl(path, method, body, authEnabled) {
  const base = getBaseUrl() || "";
  const url = `${base}${path}` || path;
  const headers = [];

  if (authEnabled) {
    headers.push('-H "X-Nonce: <nonce>"');
    headers.push('-H "X-Auth: <signature>"');
  }

  if (body) {
    headers.push('-H "Content-Type: application/json"');
  }

  const headerStr = headers.length ? `\\
  ${headers.join(" \\\n  ")}` : "";
  const dataStr = body ? ` \\\n  -d '${JSON.stringify(body)}'` : "";

  return `curl -X ${method} ${url}${headerStr}${dataStr}`;
}

export default { init, destroy };
