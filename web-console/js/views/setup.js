/**
 * @brief Firmware setup helper (POST /api/setup).
 */

import { applySetup } from "../core/api.js";
import { RuntimeState, getLastResponse, setAuthExpectation } from "../core/state.js";
import { buildCurl } from "../core/curl.js";
import { showJsonModal } from "../core/modal.js";
import { showToast } from "../core/toast.js";

let container = null;
let latestPayload = null;
let latestResponse = null;
const SETUP_SIGNATURE = "POST /api/setup";

export function init() {
  container = document.getElementById("setup-view");
  bindEvents();
  seedFromState();
  renderDocs();
  renderResponse();
}

export function destroy() {}

function bindEvents() {
  container?.querySelector("#setup-apply")?.addEventListener("click", onApply);

  container
    ?.querySelectorAll("#setup-auth, #setup-serial")
    .forEach((el) => el.addEventListener("change", renderDocs));

  container?.querySelector("#setup-payload-open")?.addEventListener("click", () => {
    showJsonModal("Payload /api/setup", latestPayload);
  });

  container?.querySelector("#setup-response-open")?.addEventListener("click", () => {
    showJsonModal("Risposta /api/setup", latestResponse);
  });
}

function seedFromState() {
  const authToggle = container?.querySelector("#setup-auth");
  const serialToggle = container?.querySelector("#setup-serial");

  if (!authToggle || !serialToggle) return;

  const device = RuntimeState.deviceState?.device;
  if (device) {
    authToggle.checked = !!device.auth;
    serialToggle.checked = !!device.serialDebug;
  }
}

async function onApply() {
  const status = container?.querySelector("#setup-status");
  if (status) {
    status.textContent = "Applying setup...";
  }

  const payload = collectPayload();

  try {
    await applySetup(payload);
    latestPayload = payload;
    status.textContent = "Setup applied";
    showToast("Setup applicato sul dispositivo", "success");
    setAuthExpectation(payload.auth);
    renderDocs();
    renderResponse();
  } catch (err) {
    status.textContent = err.message;
    showToast(`Setup fallito: ${err.message}`, "error");
  }
}

function renderDocs() {
  const payload = collectPayload();
  latestPayload = payload;

  const curlEl = container?.querySelector("#setup-curl");
  if (curlEl) {
    curlEl.textContent = buildCurl("/api/setup", "POST", payload, RuntimeState.authEnabled);
  }

  const metaEl = container?.querySelector("#setup-auth-mode");
  if (metaEl) {
    metaEl.textContent = RuntimeState.authEnabled
      ? "Richiesta firmata (auth abilitata)"
      : "Richiesta inviata senza firma (auth disabilitata)";
  }
}

function renderResponse() {
  latestResponse = getLastResponse(SETUP_SIGNATURE) || null;
  const hint = container?.querySelector("#setup-response-hint");
  if (hint) {
    hint.textContent = latestResponse ? "Ultima risposta disponibile" : "Nessuna chiamata ancora";
  }
}

function collectPayload() {
  const auth = container?.querySelector("#setup-auth")?.checked || false;
  const serial = container?.querySelector("#setup-serial")?.checked || false;
  return { auth, serialDebug: serial };
}

export default { init, destroy };
