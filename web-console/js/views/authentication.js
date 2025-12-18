/**
 * @brief Authentication documentation and live inspector.
 */

import { computeSignature, hasAuthKey } from "../core/auth.js";
import { fetchNonce } from "../core/api.js";

let container = null;
let currentNonce = null;

export function init() {
  container = document.getElementById("auth-view");
  bindEvents();
  updateSignaturePreview();
}

export function destroy() {}

function bindEvents() {
  const refresh = container?.querySelector("#auth-refresh");
  const uriInput = container?.querySelector("#sig-uri");
  const payloadInput = container?.querySelector("#sig-payload");

  refresh?.addEventListener("click", onFetchNonce);
  uriInput?.addEventListener("input", updateSignaturePreview);
  payloadInput?.addEventListener("input", updateSignaturePreview);
}

async function onFetchNonce() {
  const box = container?.querySelector("#nonce-box");
  if (!box) return;

  box.textContent = "Requesting challenge...";
  box.classList.remove("callout-error");

  try {
    currentNonce = await fetchNonce();
    box.textContent = `nonce = ${currentNonce}`;
    updateSignaturePreview();
  } catch (err) {
    box.textContent = `Challenge failed: ${err.message}`;
    box.classList.add("callout-error");
  }
}

async function updateSignaturePreview() {
  const nonceEl = container?.querySelector("#sig-nonce");
  const sigEl = container?.querySelector("#sig-value");
  if (!nonceEl || !sigEl) return;

  const nonce = currentNonce || "(fetch nonce)";
  nonceEl.textContent = nonce;

  if (!hasAuthKey()) {
    sigEl.textContent = "Missing auth key";
    return;
  }

  if (!currentNonce) {
    sigEl.textContent = "Fetch nonce to compute";
    return;
  }

  const uri = container.querySelector("#sig-uri")?.value || "/api/state";
  const payload = container.querySelector("#sig-payload")?.value || "";

  try {
    const signature = await computeSignature(nonce, uri, payload);
    sigEl.textContent = signature;
  } catch (err) {
    sigEl.textContent = err.message;
  }
}

export default { init, destroy };
